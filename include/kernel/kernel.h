#ifndef DIALOS_KERNEL_H
#define DIALOS_KERNEL_H

#include <Arduino.h>

namespace dialOS {

// Forward declarations
class TaskScheduler;
class MemoryManager;
class SystemServices;
class RamFS;

/**
 * @brief Main kernel class - coordinates all OS subsystems
 */
class Kernel {
public:
    static Kernel& instance();
    
    // Initialization
    bool init();
    void run();
    
    // Subsystem access
    TaskScheduler* getScheduler() { return scheduler; }
    MemoryManager* getMemoryManager() { return memoryManager; }
    SystemServices* getSystemServices() { return systemServices; }
    RamFS* getRamFS() { return ramFS; }
    
    // System info
    unsigned long getUptime() { return millis(); }
    
private:
    Kernel();
    ~Kernel();
    Kernel(const Kernel&) = delete;
    Kernel& operator=(const Kernel&) = delete;
    
    TaskScheduler* scheduler;
    MemoryManager* memoryManager;
    SystemServices* systemServices;
    RamFS* ramFS;
    
    bool initialized;
};

} // namespace dialOS

#endif // DIALOS_KERNEL_H
