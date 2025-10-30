#include "applet_manager.h"

#include <cstdio>
#include <cstring>

using namespace dialOS;

AppletManager* AppletManager::instance = nullptr;

AppletManager::AppletManager()
: selectionIndex(0), lastChangeMs(0), lastEncoderValue(0)
{
    // applets.push_back({"hello_world", "Builtin hello world applet", 1024, true, false, 0});
    // applets.push_back({"counter_applet", "Builtin counter applet", 2048, true, true, 1000});
    // applets.push_back({"weather_ds", "Weather fetcher (requires network)", 4096, false, false, 0});
    // applets.push_back({"music_player", "Simple melody player", 8192, false, false, 0});
    // applets.push_back({"file_browser", "Browse mounted storages", 5120, false, false, 0});
}

AppletManager::~AppletManager() {
    // nothing to free yet
}

// Generalized VM task for running any applet
// param should be a pointer to VMApplet from the registry
void vmAppletTask(byte taskId, void *param) {
  
  // Get or create per-task state
  static std::map<byte, VMTaskState*> taskStates;
  
  if (taskStates.find(taskId) == taskStates.end()) {
    taskStates[taskId] = new VMTaskState();
  }
  
  VMTaskState* state = taskStates[taskId];
  SystemServices *sys = Kernel::instance().getSystemServices();

  // Prevent re-execution after finishing for one-shot applets
  if (state->hasFinished) {
    TaskScheduler::sleepMs(1000); // Sleep if finished
    return;
  }

  // Get applet descriptor
  VMApplet *applet = (VMApplet *)param;
  if (!applet) {
    sys->log(LogLevel::ERROR, "VM task started with null applet");
    return;
  }

  // Initialize platform (shared across all applets)
  if (!state->platform) {
    state->platform = new ESP32Platform();
  }

  // Initialize VM on first run
  if (!state->initialized) {
    sys->logf(LogLevel::INFO, "Starting VM task for applet: %s (%d bytes)", applet->name,
              applet->bytecodeSize);

    // Deserialize bytecode
    std::vector<uint8_t> bytecode(applet->bytecode,
                                  applet->bytecode + applet->bytecodeSize);
    state->module = new dialos::compiler::BytecodeModule();
    try {
      *state->module = dialos::compiler::BytecodeModule::deserialize(bytecode);
      sys->logf(LogLevel::INFO, "Applet '%s' loaded successfully",
                applet->name);
    } catch (const std::exception &e) {
      sys->logf(LogLevel::ERROR, "Failed to load applet '%s': %s",
                applet->name, e.what());
      delete state->module;
      state->module = nullptr;
      return;
    }

    // Create value pool and VM state
    state->pool = new dialos::vm::ValuePool(state->module->metadata.heapSize);
    state->vmState = new dialos::vm::VMState(*state->module, *state->pool, *state->platform);
    state->vmState->reset();
    sys->logf(LogLevel::INFO, "VM initialized for '%s', heap: %d bytes",
              applet->name, state->module->metadata.heapSize);
    // Invoke app.onLoad callback if registered so the app can perform startup setup
    // (e.g., register encoders, timers). Use an empty args vector.
    state->platform->invokeCallback("app.onLoad", std::vector<dialos::vm::Value>());
    state->platform->console_log("Invoked app.onLoad callback if registered");
    state->initialized = true;
  }
  
  // FreeRTOS tasks run in infinite loops
  while (true) {
    // Prevent re-execution after finishing for one-shot applets
    if (state->hasFinished) {
      TaskScheduler::sleepMs(1000); // Sleep if finished
      continue;
    }
    
    if (!state->initialized) {
      // Should not happen, but safety check
      TaskScheduler::sleepMs(100);
      continue;
    }
    
    // Reset VM for repeated execution (except first time)
    // Only reset for applets that are designed to repeat
    if (state->executionCount > 0 && applet->repeat) {
      state->vmState->reset();
    }

    // Execute VM (up to 1000 instructions per task tick)
    dialos::vm::VMResult result = state->vmState->execute(1000);

    // Handle execution result
    if (result == dialos::vm::VMResult::FINISHED) {
      state->executionCount++;

      // Handle repeat or one-shot execution
      if (applet->repeat && applet->executeInterval > 0) {
        // For repeating applets with intervals, reset and wait
        state->vmState->reset();
        TaskScheduler::sleepMs(applet->executeInterval);
      } else if (!applet->repeat) {
        // One-shot applet finished - don't reset, just mark as finished
        sys->logf(LogLevel::INFO, "Applet '%s' finished (task %d)",
                  applet->name, taskId);
        state->hasFinished = true;
        // Sleep for a while before checking again
        TaskScheduler::sleepMs(1000);
      } else {
        // repeat=true but interval=0, run continuously with reset
        state->vmState->reset();
        // Small delay to prevent busy loop
        TaskScheduler::sleepMs(10);
      }
    } else if (result == dialos::vm::VMResult::ERROR) {
      const char *errMsg = state->vmState->getError().c_str();
      sys->logf(LogLevel::ERROR, "VM error in '%s' (task %d): %s", applet->name, taskId,
                errMsg && *errMsg ? errMsg : "Unknown error");
      // Sleep and retry
      TaskScheduler::sleepMs(5000);
    } else if (result == dialos::vm::VMResult::OUT_OF_MEMORY) {
      sys->logf(LogLevel::ERROR, "Applet '%s' (task %d) out of memory", applet->name, taskId);
      state->vmState->reset();
      TaskScheduler::sleepMs(5000);
    } else if (result == dialos::vm::VMResult::YIELD) {
      // VM yielded, continue soon
      TaskScheduler::sleepMs(10);
    }
    // VMResult::OK - continue executing immediately next tick with small delay
    else {
      TaskScheduler::sleepMs(1);
    }
  }
}

// Helper function to create a VM task for an applet by name
Task *createVMTask(const char *appletName) {
  TaskScheduler *scheduler = Kernel::instance().getScheduler();
  SystemServices *sys = Kernel::instance().getSystemServices();

  // Find applet in registry
  VMApplet *applet = nullptr;
  for (int i = 0; i < BUILTIN_APPLET_REGISTRY_SIZE; i++) {
    if (strcmp(BUILTIN_APPLET_REGISTRY[i].name, appletName) == 0) {
      applet = &BUILTIN_APPLET_REGISTRY[i];
      break;
    }
  }

  if (!applet) {
    sys->logf(LogLevel::ERROR, "Applet '%s' not found in registry",
              appletName);
    return nullptr;
  }

  // Create task with applet-specific name
  char taskName[32];
  snprintf(taskName, sizeof(taskName), "VM_%s", applet->name);

  Task *task = scheduler->createTask(taskName, vmAppletTask, (void *)applet,
                                     16384, TaskPriority::NORMAL);

  if (task) {
    sys->logf(LogLevel::INFO, "Created VM task for applet: %s", applet->name);
  } else {
    sys->logf(LogLevel::ERROR, "Failed to create task for applet: %s",
              applet->name);
  }

  return task;
}

void AppletManager::start() {
    if (instance) return;
    instance = new AppletManager();
    TaskScheduler *scheduler = Kernel::instance().getScheduler();
    // create kernel task
    scheduler->createTask("AppletMgr", AppletManager::taskEntry, nullptr, 4096, TaskPriority::NORMAL);
}

void AppletManager::taskEntry(byte taskId, void *param) {
    if (!instance) {
        instance = new AppletManager();
    }
    instance->run(taskId);
}

void AppletManager::refreshInstalledState() {
    RamFS *ramfs = Kernel::instance().getRamFS();
    // Check builtin registry first
    for (size_t i = 0; i < applets.size(); ++i) {
        applets[i].installed = false;
        for (int j = 0; j < BUILTIN_APPLET_REGISTRY_SIZE; ++j) {
            if (strcmp(BUILTIN_APPLET_REGISTRY[j].name, applets[i].name) == 0) {
                applets[i].installed = true;
                break;
            }
        }
        if (!applets[i].installed) {
            char path[96];
            snprintf(path, sizeof(path), "/applets/%s.dsb", applets[i].name);
            if (ramfs->exists(path)) {
                applets[i].installed = true;
            }
        }
    }
}

bool AppletManager::installAppletToRamFS(const char *name) {
    RamFS *ramfs = Kernel::instance().getRamFS();
    SystemServices *sys = Kernel::instance().getSystemServices();

    char path[96];
    snprintf(path, sizeof(path), "/applets/%s.dsb", name);

    int fd = ramfs->open(path, FileMode::WRITE, 0);
    if (fd < 0) {
        sys->logf(LogLevel::ERROR, "AppletMgr: failed to open %s", path);
        return false;
    }

    // Placeholder bytecode blob until network/download implemented
    const uint8_t dummy[] = {0x00, 0x01, 0x02, 0x03};
    size_t written = ramfs->write(fd, (const char*)dummy, sizeof(dummy));
    ramfs->close(fd);

    if (written != sizeof(dummy)) {
        sys->logf(LogLevel::ERROR, "AppletMgr: incomplete write for %s", name);
        return false;
    }

    sys->logf(LogLevel::INFO, "AppletMgr: installed %s", name);
    return true;
}

void AppletManager::renderMenu(int sel) {
    M5Dial.Display.fillScreen(TFT_BLACK);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Dial.Display.setCursor(12, 8);
    M5Dial.Display.printf("App Manager");

    int y = 40;
    for (size_t i = 0; i < applets.size(); ++i) {
        if ((int)i == sel) {
            M5Dial.Display.setTextColor(TFT_BLACK, TFT_WHITE);
            M5Dial.Display.fillRect(8, y - 2, 224, 18, TFT_WHITE);
        } else {
            M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        M5Dial.Display.setCursor(12, y);
        M5Dial.Display.printf("%s %s", applets[i].name, applets[i].installed ? "(I)" : "( )");
        y += 20;
    }

    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    M5Dial.Display.setCursor(12, 200);
    M5Dial.Display.printf("Rotate to select, hold for %.1fs to action", selectionConfirmMs / 1000.0f);
}

void AppletManager::run(byte taskId) {
    SystemServices *sys = Kernel::instance().getSystemServices();

    refreshInstalledState();
    lastEncoderValue = get_encoder();
    lastChangeMs = millis();
    renderMenu(selectionIndex);

    while (true) {
        int enc = get_encoder();
        if (enc != lastEncoderValue) {
            int delta = enc - lastEncoderValue;
            if (delta > 8) delta = 1;
            if (delta < -8) delta = -1;
            if (delta != 0) {
                selectionIndex -= delta;
                if (selectionIndex < 0) selectionIndex = (int)applets.size() - 1;
                if (selectionIndex >= (int)applets.size()) selectionIndex = 0;
                lastChangeMs = millis();
                renderMenu(selectionIndex);
            }
            lastEncoderValue = enc;
        }

        if ((millis() - lastChangeMs) >= selectionConfirmMs) {
            AvailableApplet &app = applets[selectionIndex];
            if (app.installed) {
                sys->logf(LogLevel::INFO, "AppletMgr: launching '%s'", app.name);
                Task *t = createVMTask(app.name);
                if (t) {
                    sys->logf(LogLevel::INFO, "AppletMgr: launched '%s'", app.name);
                } else {
                    sys->logf(LogLevel::WARNING, "AppletMgr: failed to launch '%s' via registry", app.name);
                }
            } else {
                sys->logf(LogLevel::INFO, "AppletMgr: installing '%s'...", app.name);
                M5Dial.Display.setCursor(12, 220);
                M5Dial.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
                M5Dial.Display.printf("Installing %s...", app.name);
                bool ok = installAppletToRamFS(app.name);
                if (ok) {
                    app.installed = true;
                    M5Dial.Display.setTextColor(TFT_GREEN, TFT_BLACK);
                    M5Dial.Display.setCursor(12, 220);
                    M5Dial.Display.printf("%s installed", app.name);
                    Task *t = createVMTask(app.name);
                    if (t) {
                        sys->logf(LogLevel::INFO, "AppletMgr: started installed '%s'", app.name);
                    }
                } else {
                    M5Dial.Display.setTextColor(TFT_RED, TFT_BLACK);
                    M5Dial.Display.setCursor(12, 220);
                    M5Dial.Display.printf("Install failed");
                }
            }

            TaskScheduler::sleepMs(800);
            refreshInstalledState();
            renderMenu(selectionIndex);
            lastChangeMs = millis();
        }

        TaskScheduler::sleepMs(50);
    }
}