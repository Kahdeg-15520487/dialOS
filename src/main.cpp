#include "Encoder.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "kernel/ramfs.h"
#include "kernel/system.h"
#include "kernel/task.h"
#include "vm_builtin_applets.h"
#include <Arduino.h>
#include <M5Dial.h>
#include <map>

// VM includes
#include "vm/platform.h"
#include "vm/vm_core.h"
#include "esp32_platform.h"


using namespace dialOS;
using namespace dialos::vm;

// Global variables for encoder interaction
int encoderValue = 0;
bool kernelEnabled = false;
bool systemInitialized = false; // Flag to signal when system is ready for VM tasks

// Applet registry is now auto-generated in vm_builtin_applets.h

void memoryTestTask(byte taskId, void *param) {
  static bool initialized = false;
  static void *allocations[10] = {nullptr};
  static int allocationSizes[10] = {512, 768,  1024, 256, 2048,
                                    512, 1024, 384,  640, 896};
  static int currentStep = 0;
  static bool allocating = true;

  MemoryManager *mem = Kernel::instance().getMemoryManager();
  SystemServices *sys = Kernel::instance().getSystemServices();

  // Initialize on first run
  if (!initialized) {
    sys->log(LogLevel::INFO,
             "Memory Test: Starting allocation/deallocation cycle");
    initialized = true;
    TaskScheduler::sleepMs(500);
    return;
  }

  if (allocating) {
    // Allocation phase
    if (currentStep < 10) {
      allocations[currentStep] =
          mem->allocate(allocationSizes[currentStep], taskId);
      if (allocations[currentStep]) {
        // Use the allocated memory - fill it with test pattern
        uint8_t *data = (uint8_t *)allocations[currentStep];
        for (int i = 0; i < allocationSizes[currentStep]; i++) {
          data[i] = (uint8_t)(i % 256); // Fill with repeating pattern 0-255
        }
        sys->logf(LogLevel::INFO, "Allocated and filled %d bytes at step %d",
                  allocationSizes[currentStep], currentStep);
      } else {
        sys->logf(LogLevel::WARNING, "Failed to allocate %d bytes at step %d",
                  allocationSizes[currentStep], currentStep);
      }
      currentStep++;
    } else {
      // Switch to deallocation phase
      allocating = false;
      currentStep = 0;
      sys->log(LogLevel::INFO, "Memory Test: Switching to verification phase");
    }
  } else {
    // Verification and deallocation phase
    if (currentStep < 10) {
      if (allocations[currentStep]) {
        // Verify the memory contents before freeing
        uint8_t *data = (uint8_t *)allocations[currentStep];
        bool dataValid = true;
        for (int i = 0; i < allocationSizes[currentStep]; i++) {
          if (data[i] != (uint8_t)(i % 256)) {
            dataValid = false;
            break;
          }
        }

        if (dataValid) {
          sys->logf(LogLevel::INFO,
                    "Verified %d bytes at step %d - data intact",
                    allocationSizes[currentStep], currentStep);
        } else {
          sys->logf(LogLevel::WARNING,
                    "Data corruption detected in %d bytes at step %d",
                    allocationSizes[currentStep], currentStep);
        }

        mem->free(allocations[currentStep], taskId);
        sys->logf(LogLevel::INFO, "Freed %d bytes at step %d",
                  allocationSizes[currentStep], currentStep);
        allocations[currentStep] = nullptr;
      }
      currentStep++;
    } else {
      // Reset for next cycle
      allocating = true;
      currentStep = 0;
      sys->log(LogLevel::INFO, "Memory Test: Cycle complete, restarting");
    }
  }

  // Sleep for 500ms before next action
  TaskScheduler::sleepMs(500);
}

void memoryGaugeTask(byte taskId, void *param) {
  // Get memory stats
  MemoryManager::MemoryStats stats =
      Kernel::instance().getMemoryManager()->getStats();

  // Calculate memory usage percentage
  float usagePercent = (float)stats.usedHeap / (float)stats.totalHeap * 100.0f;

  // Screen dimensions
  const int centerX = 120;
  const int centerY = 120;
  const int radius = 119;
  const int arcWidth = 9;

  // Arc angles for bottom third (240 degrees to 300 degrees, covering 60
  // degrees) In M5GFX, angles start at 3 o'clock and go clockwise
  const int startAngle = 30;
  const int endAngle = 150;

  // Calculate fill angle based on memory usage (120 degrees total)
  int fillAngle = (int)(120.0f * (usagePercent / 100.0f));

  // Draw background arc (dark gray)
  M5Dial.Display.fillArc(centerX, centerY, radius, radius - arcWidth,
                         startAngle, endAngle, TFT_BLACK);
  M5Dial.Display.drawArc(centerX, centerY, radius, radius - arcWidth,
                         startAngle, endAngle, TFT_DARKGREY);

  // Choose color based on usage
  uint16_t fillColor;
  if (usagePercent < 50.0f) {
    fillColor = TFT_GREEN;
  } else if (usagePercent < 75.0f) {
    fillColor = TFT_YELLOW;
  } else {
    fillColor = TFT_RED;
  }

  // Draw filled arc showing usage
  if (fillAngle < endAngle) {
    M5Dial.Display.fillArc(centerX, centerY, radius, radius - arcWidth,
                           endAngle - fillAngle, endAngle, fillColor);
  }

  // Display memory stats text at bottom
  M5Dial.Display.setTextSize(1);
  M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Dial.Display.setCursor(60, 200);
  M5Dial.Display.printf("RAM: %d%% %d/%d", (int)usagePercent, stats.usedHeap,
                        stats.totalHeap);
}

void displayTask(byte taskId, void *param) {
  // Wait for system to be ready
  while (!systemInitialized) {
    delay(100); // Use regular delay before system is ready
  }
  
  int counter = 0;
  
  while (true) { // FreeRTOS tasks run in infinite loops
    // Update display
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextColor(TFT_GREEN, TFT_BLACK);
    M5Dial.Display.setCursor(40, 80);
    M5Dial.Display.printf("dialOS\n\tTask: %d", counter++);

    // Sleep for 1 second before next update
    TaskScheduler::sleepMs(1000);
  }
}

void encoderTask(byte taskId, void *param) {
  // Wait for system to be ready
  while (!systemInitialized) {
    delay(100); // Use regular delay before system is ready
  }
  
  while (true) { // FreeRTOS tasks run in infinite loops
    int newValue = get_encoder();
    if (newValue != encoderValue) {
      encoderValue = newValue;
      Serial.print("Encoder: ");
      Serial.println(encoderValue);
    }
    
    // Small delay to prevent busy waiting
    TaskScheduler::sleepMs(50);
  }
}

void ramfsTestTask(byte taskId, void *param) {
  static bool testRun = false;
  if (testRun)
    return; // Only run once

  RamFS *ramfs = Kernel::instance().getRamFS();
  SystemServices *sys = Kernel::instance().getSystemServices();

  sys->log(LogLevel::INFO, "=== RamFS Test Starting ===");

  // Test 1: Create and write a file
  const char *testFile = "test.txt";
  const char *testData = "Hello from dialOS RamFS!";
  int fd = ramfs->open(testFile, FileMode::WRITE, 0); // Task ID 0 for test

  if (fd >= 0) {
    size_t written = ramfs->write(fd, testData, strlen(testData));
    sys->logf(LogLevel::INFO, "Wrote %d bytes to %s", written, testFile);
    ramfs->close(fd);
  } else {
    sys->log(LogLevel::ERROR, "Failed to create file");
  }

  // Test 2: Read the file back
  fd = ramfs->open(testFile, FileMode::READ, 0);
  if (fd >= 0) {
    char buffer[64] = {0};
    size_t bytesRead = ramfs->read(fd, buffer, sizeof(buffer) - 1);
    sys->logf(LogLevel::INFO, "Read %d bytes: %s", bytesRead, buffer);
    ramfs->close(fd);
  } else {
    sys->log(LogLevel::ERROR, "Failed to open file for reading");
  }

  // Test 3: Get file stats
  uint32_t fileSize = ramfs->getSize(testFile);
  sys->logf(LogLevel::INFO, "File size: %d bytes", fileSize);

  // Test 4: Get filesystem stats
  RamFS::Stats stats = ramfs->getStats();
  sys->logf(LogLevel::INFO, "RamFS: %d/%d bytes used, %d/%d files",
            stats.totalSize, stats.totalSize + stats.freeSpace,
            stats.totalFiles, stats.maxFiles);

  sys->log(LogLevel::INFO, "=== RamFS Test Complete ===");
  testRun = true;
}

// VM task state structure to avoid static variable conflicts
struct VMTaskState {
  dialos::vm::VMState *vmState = nullptr;
  dialos::vm::ValuePool *pool = nullptr;
  ESP32Platform *platform = nullptr;
  dialos::compiler::BytecodeModule *module = nullptr;
  int executionCount = 0;
  bool initialized = false;
  bool hasFinished = false;
};

// Generalized VM task for running any applet
// param should be a pointer to VMApplet from the registry
void vmAppletTask(byte taskId, void *param) {
  // Wait for system initialization to complete
  while (!systemInitialized) {
    delay(100); // Use regular delay before system is ready
  }
  
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

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========== dialOS Boot ==========");

  // Initialize M5Dial hardware
  M5Dial.begin(true, true);
  M5Dial.Display.setTextSize(2);
  M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Dial.Display.fillScreen(TFT_BLACK);
  M5Dial.Display.setCursor(20, 100);
  M5Dial.Display.println("Booting...");

  // Initialize custom encoder driver
  init_encoder();

  Serial.println("M5Dial hardware initialized");

  // Initialize kernel
  if (!Kernel::instance().init()) {
    Serial.println("FATAL: Kernel initialization failed!");
    M5Dial.Display.fillScreen(TFT_RED);
    M5Dial.Display.setCursor(20, 100);
    M5Dial.Display.println("KERNEL\nFAIL!");
    while (true)
      delay(1000);
  }

  Serial.println("Kernel initialized successfully");
  kernelEnabled = true;

  // Create kernel tasks
  TaskScheduler *scheduler = Kernel::instance().getScheduler();

  Task *task1 = scheduler->createTask("Display", displayTask, nullptr, 2048,
                                      TaskPriority::NORMAL);
  Task *task2 = scheduler->createTask("Encoder", encoderTask, nullptr, 2048,
                                      TaskPriority::HIGH_PRIORITY);
  // Task *task3 = scheduler->createTask("RamFS_Test", ramfsTestTask, nullptr,
  //                                     4096, TaskPriority::NORMAL);
  // Task *task4 = scheduler->createTask("MemoryGauge", memoryGaugeTask,
  // nullptr,
  //                                     2048, TaskPriority::LOW_PRIORITY);
  // Task *task5 = scheduler->createTask("MemoryTest", memoryTestTask, nullptr,
  //                                     2048, TaskPriority::NORMAL);
  
  // Create VM task(s) for applets
  Task *vmTask1 = createVMTask("hello_world");
  Task *vmTask2 = createVMTask("counter_applet");

  if (task1 && task2 
    && vmTask1
    && vmTask2
  ) {
    Serial.println("Kernel tasks created");
  }

  // Print memory stats
  MemoryManager::MemoryStats stats =
      Kernel::instance().getMemoryManager()->getStats();
  Serial.print("Memory: ");
  Serial.print(stats.usedHeap);
  Serial.print(" / ");
  Serial.print(stats.totalHeap);
  Serial.println(" bytes used");

  // Initialize encoder value

  encoderValue = get_encoder();

  M5Dial.Display.fillScreen(TFT_BLACK);
  M5Dial.Display.setCursor(20, 100);
  M5Dial.Display.println("dialOS\nReady!");
  delay(1000);
  M5Dial.Display.fillScreen(TFT_BLACK);

  // Signal that system initialization is complete
  systemInitialized = true;
  Serial.println("System initialization complete - VM tasks can now start");

  Serial.println("=================================\n");
}

void loop() {
  // Update M5Dial state
  M5Dial.update();

  if (!kernelEnabled) {
    // Fallback: simple display update
    int newEncoderValue = get_encoder();
    if (newEncoderValue != encoderValue) {
      encoderValue = newEncoderValue;
      M5Dial.Display.fillScreen(TFT_BLACK);
      M5Dial.Display.setCursor(20, 80);
      M5Dial.Display.printf("dialOS\nEncoder: %d", encoderValue);
    }
  }

  // FreeRTOS handles task scheduling automatically
  // Just a small delay to prevent busy waiting
  delay(1);
}