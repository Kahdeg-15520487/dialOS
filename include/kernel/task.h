#ifndef DIALOS_TASK_H
#define DIALOS_TASK_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace dialOS {

// Task priority levels mapped to FreeRTOS priorities
enum class TaskPriority {
    SYSTEM = 4,         // System tasks (highest)
    HIGH_PRIORITY = 3,  // High priority
    NORMAL = 2,         // Normal priority
    LOW_PRIORITY = 1    // Low priority (lowest)
};

/**
 * @brief Task Control Block - represents a FreeRTOS task
 */
class Task {
public:
    Task(const char* name, void (*function)(byte, void*), void* param, 
         size_t stackSize, TaskPriority priority);
    ~Task();
    
    // Task information
    const char* getName() const { return name; }
    uint32_t getId() const { return id; }
    TaskPriority getPriority() const { return priority; }
    TaskHandle_t getHandle() const { return taskHandle; }
    
    // Task control
    void suspend();
    void resume();
    void terminate();
    bool isRunning() const;
    
    // Memory management
    size_t getStackSize() const { return stackSize; }
    void* getParameter() const { return parameter; }
    
    // Static task wrapper for FreeRTOS
    static void taskWrapper(void* param);
    
private:
    static uint32_t nextId;
    
    uint32_t id;
    char name[32];
    TaskPriority priority;
    TaskHandle_t taskHandle;
    
    void (*taskFunction)(byte, void*);
    void* parameter;
    
    size_t stackSize;
};

/**
 * @brief FreeRTOS-based task scheduler
 */
class TaskScheduler {
public:
    TaskScheduler();
    ~TaskScheduler();
    
    bool init();
    
    // Task management
    Task* createTask(const char* name, void (*function)(byte, void*), 
                     void* param = nullptr, size_t stackSize = 4096,
                     TaskPriority priority = TaskPriority::NORMAL);
    bool destroyTask(uint32_t taskId);
    
    // Task control
    bool freezeTask(uint32_t taskId);
    bool resumeTask(uint32_t taskId);
    
    // Sleep utility (for use within tasks)
    static void sleepMs(unsigned long ms);
    static void yield();
    
    // Task queries
    Task* getTask(uint32_t taskId);
    size_t getTaskCount() const { return taskCount; }
    
    // Current task info
    uint32_t getCurrentTaskId() const;
    
    // System info
    static uint32_t getFreeHeapSize();
    
private:
    static const size_t MAX_TASKS = 16;
    
    Task* tasks[MAX_TASKS];
    size_t taskCount;
};

} // namespace dialOS

#endif // DIALOS_TASK_H
