#ifndef DIALOS_APPLET_MANAGER_H
#define DIALOS_APPLET_MANAGER_H

// ...existing code...
#include <vector>
#include <cstdint>
#include <M5Dial.h>
#include "Encoder.h"

#include "kernel/task.h"
#include "kernel/ramfs.h"
#include "kernel/system.h"
#include "kernel/kernel.h"
#include "vm/platform.h"
#include "vm/vm_core.h"
#include "vm_builtin_applets.h"
#include "esp32_platform.h"


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

class AppletManager {
public:
    static void start(); // create the kernel task

private:
    AppletManager();
    ~AppletManager();

    static void taskEntry(byte taskId, void *param);
    void run(byte taskId);

    struct AvailableApplet {
        const char *name;
        const char *description;
        uint32_t sizeBytes;
        bool installed;
        bool repeat;
        uint32_t executeInterval;
    };

    void refreshInstalledState();
    bool installAppletToRamFS(const char *name);
    void renderMenu(int selection);

    // applet list (can be extended or loaded dynamically later)
    std::vector<AvailableApplet> applets;

    // UI / state
    int selectionIndex;
    unsigned long lastChangeMs;
    const unsigned long selectionConfirmMs = 1500;
    int lastEncoderValue;

    static AppletManager *instance;
};

#endif // DIALOS_APPLET_MANAGER_H