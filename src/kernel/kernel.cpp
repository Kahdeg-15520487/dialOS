#include "kernel/kernel.h"
#include "kernel/task.h"
#include "kernel/memory.h"
#include "kernel/system.h"
#include "kernel/ramfs.h"

namespace dialOS {

Kernel::Kernel() 
    : scheduler(nullptr)
    , memoryManager(nullptr)
    , systemServices(nullptr)
    , ramFS(nullptr)
    , initialized(false) {
}

Kernel::~Kernel() {
    if (scheduler) delete scheduler;
    if (memoryManager) delete memoryManager;
    if (systemServices) delete systemServices;
    if (ramFS) delete ramFS;
}

Kernel& Kernel::instance() {
    static Kernel instance;
    return instance;
}

bool Kernel::init() {
    if (initialized) {
        return true;
    }
    
    Serial.begin(115200);
    Serial.println("dialOS Kernel initializing...");
    
    // Initialize system services first
    systemServices = new SystemServices();
    if (!systemServices || !systemServices->init()) {
        Serial.println("Failed to initialize system services");
        return false;
    }
    systemServices->log(LogLevel::INFO, "System services initialized");
    
    // Initialize memory manager with available ESP32 heap
    size_t availableHeap = ESP.getFreeHeap();
    // Reserve 50% of available heap for kernel memory manager
    // Keep other 50% for system overhead, stack, etc.
    size_t kernelHeap = availableHeap / 2;
    
    memoryManager = new MemoryManager();
    if (!memoryManager || !memoryManager->init(kernelHeap)) {
        systemServices->log(LogLevel::ERROR, "Failed to initialize memory manager");
        return false;
    }
    systemServices->logf(LogLevel::INFO, "Memory manager initialized: %d KB available (%.1f KB total heap)", 
                        kernelHeap / 1024, availableHeap / 1024.0);
    
    // Initialize task scheduler
    scheduler = new TaskScheduler();
    if (!scheduler || !scheduler->init()) {
        systemServices->log(LogLevel::ERROR, "Failed to initialize task scheduler");
        return false;
    }
    systemServices->log(LogLevel::INFO, "Task scheduler initialized");
    
    // Initialize RamFS
    ramFS = new RamFS();
    if (!ramFS || !ramFS->init(16, 16 * 1024)) {  // 16 max files, 16KB storage
        systemServices->log(LogLevel::ERROR, "Failed to initialize RamFS");
        return false;
    }
    systemServices->log(LogLevel::INFO, "RamFS initialized");
    
    initialized = true;
    systemServices->log(LogLevel::INFO, "dialOS Kernel ready");
    
    return true;
}

void Kernel::run() {
    if (!initialized) {
        Serial.println("ERROR: Kernel not initialized!");
        return;
    }
    
    // With FreeRTOS, the scheduler runs automatically
    // This method is no longer needed but kept for compatibility
    Serial.println("Kernel: FreeRTOS scheduler is running tasks automatically");
}

} // namespace dialOS
