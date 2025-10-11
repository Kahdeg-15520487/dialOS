#ifndef DIALOS_TASK_H
#define DIALOS_TASK_H

#include <Arduino.h>

namespace dialOS {

// Task states
enum class TaskState {
    READY,      // Ready to run
    RUNNING,    // Currently executing
    FROZEN,     // Suspended (preserves state)
    SLEEPING,   // Sleeping for specified time
    TERMINATED  // Finished execution
};

// Task priority levels
enum class TaskPriority {
    SYSTEM = 0,         // System tasks (highest)
    HIGH_PRIORITY = 1,  // High priority
    NORMAL = 2,         // Normal priority
    LOW_PRIORITY = 3    // Low priority (lowest)
};

/**
 * @brief Task Control Block - represents a single task
 */
class Task {
public:
    Task(const char* name, void (*function)(byte, void*), void* param, 
         size_t stackSize, TaskPriority priority);
    ~Task();
    
    // Task information
    const char* getName() const { return name; }
    uint32_t getId() const { return id; }
    TaskState getState() const { return state; }
    TaskPriority getPriority() const { return priority; }
    
    // State management
    void setState(TaskState newState) { state = newState; }
    void setSleepUntil(unsigned long time) { wakeTime = time; }
    unsigned long getWakeTime() const { return wakeTime; }
    
    // Memory management
    size_t getStackSize() const { return stackSize; }
    void* getParameter() const { return parameter; }
    
    // Execution
    void (*getFunction())(byte, void*) { return taskFunction; }
    
private:
    static uint32_t nextId;
    
    uint32_t id;
    char name[32];
    TaskState state;
    TaskPriority priority;
    
    void (*taskFunction)(byte, void*);
    void* parameter;
    
    size_t stackSize;
    unsigned long wakeTime;  // For sleeping tasks
};

/**
 * @brief Cooperative task scheduler
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
    bool sleepTask(uint32_t taskId, unsigned long ms);
    
    // Scheduler operations
    void schedule();  // Select next task to run
    void yield();     // Current task yields CPU
    
    // Task queries
    Task* getCurrentTask() { return currentTask; }
    Task* getTask(uint32_t taskId);
    size_t getTaskCount() const { return taskCount; }
    
private:
    static const size_t MAX_TASKS = 16;
    
    Task* tasks[MAX_TASKS];
    Task* currentTask;
    size_t taskCount;
    
    Task* selectNextTask();
    void switchTask(Task* nextTask);
};

} // namespace dialOS

#endif // DIALOS_TASK_H
