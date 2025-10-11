#include "kernel/task.h"
#include "kernel/kernel.h"
#include "kernel/system.h"

namespace dialOS {

uint32_t Task::nextId = 1;

Task::Task(const char* taskName, void (*function)(byte, void*), void* param, 
           size_t stack, TaskPriority prio)
    : id(nextId++)
    , state(TaskState::READY)
    , priority(prio)
    , taskFunction(function)
    , parameter(param)
    , stackSize(stack)
    , wakeTime(0) {
    
    strncpy(name, taskName, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
}

Task::~Task() {
    // Cleanup if needed
}

// TaskScheduler implementation

TaskScheduler::TaskScheduler()
    : currentTask(nullptr)
    , taskCount(0) {
    
    for (size_t i = 0; i < MAX_TASKS; i++) {
        tasks[i] = nullptr;
    }
}

TaskScheduler::~TaskScheduler() {
    for (size_t i = 0; i < MAX_TASKS; i++) {
        if (tasks[i]) {
            delete tasks[i];
        }
    }
}

bool TaskScheduler::init() {
    // Scheduler is ready
    return true;
}

Task* TaskScheduler::createTask(const char* name, void (*function)(byte, void*), 
                                void* param, size_t stackSize, TaskPriority priority) {
    if (taskCount >= MAX_TASKS) {
        Kernel::instance().getSystemServices()->log(LogLevel::ERROR, "Maximum tasks reached");
        return nullptr;
    }
    
    if (!function) {
        Kernel::instance().getSystemServices()->log(LogLevel::ERROR, "Task function is null");
        return nullptr;
    }
    
    Task* task = new Task(name, function, param, stackSize, priority);
    if (!task) {
        Kernel::instance().getSystemServices()->log(LogLevel::ERROR, "Failed to allocate task");
        return nullptr;
    }
    
    // Find empty slot
    for (size_t i = 0; i < MAX_TASKS; i++) {
        if (!tasks[i]) {
            tasks[i] = task;
            taskCount++;
            
            Kernel::instance().getSystemServices()->logf(LogLevel::INFO, 
                "Task created: %s (ID: %lu)", task->getName(), task->getId());
            
            return task;
        }
    }
    
    delete task;
    return nullptr;
}

bool TaskScheduler::destroyTask(uint32_t taskId) {
    for (size_t i = 0; i < MAX_TASKS; i++) {
        if (tasks[i] && tasks[i]->getId() == taskId) {
            Kernel::instance().getSystemServices()->logf(LogLevel::INFO, 
                "Destroying task: %s", tasks[i]->getName());
            
            delete tasks[i];
            tasks[i] = nullptr;
            taskCount--;
            return true;
        }
    }
    return false;
}

bool TaskScheduler::freezeTask(uint32_t taskId) {
    Task* task = getTask(taskId);
    if (task && task->getState() == TaskState::RUNNING) {
        task->setState(TaskState::FROZEN);
        return true;
    }
    return false;
}

bool TaskScheduler::resumeTask(uint32_t taskId) {
    Task* task = getTask(taskId);
    if (task && task->getState() == TaskState::FROZEN) {
        task->setState(TaskState::READY);
        return true;
    }
    return false;
}

bool TaskScheduler::sleepTask(uint32_t taskId, unsigned long ms) {
    Task* task = getTask(taskId);
    if (task) {
        task->setState(TaskState::SLEEPING);
        task->setSleepUntil(millis() + ms);
        return true;
    }
    return false;
}

void TaskScheduler::schedule() {
    // Wake sleeping tasks
    unsigned long now = millis();
    for (size_t i = 0; i < MAX_TASKS; i++) {
        if (tasks[i] && tasks[i]->getState() == TaskState::SLEEPING) {
            if (now >= tasks[i]->getWakeTime()) {
                tasks[i]->setState(TaskState::READY);
            }
        }
    }
    
    // Set current task back to READY before selecting next
    if (currentTask && currentTask->getState() == TaskState::RUNNING) {
        currentTask->setState(TaskState::READY);
    }
    
    // Select next task to run
    Task* nextTask = selectNextTask();
    if (nextTask) {
        currentTask = nextTask;
        currentTask->setState(TaskState::RUNNING);
        
        // Execute current task
        currentTask->getFunction()(currentTask->getId(), currentTask->getParameter());
    }
}

void TaskScheduler::yield() {
    // Current task yields, allow scheduler to pick next task
    if (currentTask) {
        currentTask->setState(TaskState::READY);
    }
}

Task* TaskScheduler::getTask(uint32_t taskId) {
    for (size_t i = 0; i < MAX_TASKS; i++) {
        if (tasks[i] && tasks[i]->getId() == taskId) {
            return tasks[i];
        }
    }
    return nullptr;
}

Task* TaskScheduler::selectNextTask() {
    Task* selected = nullptr;
    TaskPriority highestPriority = TaskPriority::LOW_PRIORITY;
    
    // Simple priority-based scheduling with round-robin for same priority
    static size_t lastTaskIndex = 0;
    
    // Start from next task after last scheduled
    for (size_t offset = 0; offset < MAX_TASKS; offset++) {
        size_t i = (lastTaskIndex + 1 + offset) % MAX_TASKS;
        
        if (tasks[i] && tasks[i]->getState() == TaskState::READY) {
            if (!selected || tasks[i]->getPriority() <= highestPriority) {
                selected = tasks[i];
                highestPriority = tasks[i]->getPriority();
                lastTaskIndex = i;
                break;  // Found a task, use it
            }
        }
    }
    
    return selected;
}

void TaskScheduler::switchTask(Task* nextTask) {
    if (currentTask) {
        // Save current task state if needed
        if (currentTask->getState() == TaskState::RUNNING) {
            currentTask->setState(TaskState::READY);
        }
    }
    
    currentTask = nextTask;
    if (currentTask) {
        currentTask->setState(TaskState::RUNNING);
    }
}

} // namespace dialOS
