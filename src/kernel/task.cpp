#include "kernel/task.h"
#include "kernel/kernel.h"
#include "kernel/system.h"

namespace dialOS {

uint32_t Task::nextId = 1;

Task::Task(const char* taskName, void (*function)(byte, void*), void* param, 
           size_t stack, TaskPriority prio)
    : id(nextId++)
    , priority(prio)
    , taskHandle(nullptr)
    , taskFunction(function)
    , parameter(param)
    , stackSize(stack) {
    
    strncpy(name, taskName, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    
    // Create FreeRTOS task
    BaseType_t result = xTaskCreate(
        taskWrapper,                    // Task function
        taskName,                       // Task name
        stackSize / sizeof(StackType_t), // Stack size in words
        this,                           // Task parameters (this Task object)
        static_cast<UBaseType_t>(priority), // Priority
        &taskHandle                     // Task handle
    );
    
    if (result != pdPASS) {
        taskHandle = nullptr;
        Kernel::instance().getSystemServices()->logf(LogLevel::ERROR, 
            "Failed to create FreeRTOS task: %s", taskName);
    }
}

Task::~Task() {
    if (taskHandle) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
}

void Task::suspend() {
    if (taskHandle) {
        vTaskSuspend(taskHandle);
    }
}

void Task::resume() {
    if (taskHandle) {
        vTaskResume(taskHandle);
    }
}

void Task::terminate() {
    if (taskHandle) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
}

bool Task::isRunning() const {
    if (!taskHandle) return false;
    
    eTaskState state = eTaskGetState(taskHandle);
    return (state == eRunning || state == eReady);
}

void Task::taskWrapper(void* param) {
    Task* task = static_cast<Task*>(param);
    if (!task || !task->taskFunction) {
        vTaskDelete(nullptr); // Delete self
        return;
    }
    
    // Call the task function once - it should handle its own loop if needed
    task->taskFunction(static_cast<byte>(task->id), task->parameter);
    
    // If the task function returns (shouldn't happen with infinite loops),
    // delete the task
    vTaskDelete(nullptr);
}

// TaskScheduler implementation

TaskScheduler::TaskScheduler()
    : taskCount(0) {
    
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
    // FreeRTOS scheduler is already running
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
    if (!task || !task->getHandle()) {
        Kernel::instance().getSystemServices()->log(LogLevel::ERROR, "Failed to create task");
        if (task) delete task;
        return nullptr;
    }
    
    // Find empty slot
    for (size_t i = 0; i < MAX_TASKS; i++) {
        if (!tasks[i]) {
            tasks[i] = task;
            taskCount++;
            
            Kernel::instance().getSystemServices()->logf(LogLevel::INFO, 
                "FreeRTOS task created: %s (ID: %lu)", task->getName(), task->getId());
            
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
                "Destroying FreeRTOS task: %s", tasks[i]->getName());
            
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
    if (task && task->isRunning()) {
        task->suspend();
        return true;
    }
    return false;
}

bool TaskScheduler::resumeTask(uint32_t taskId) {
    Task* task = getTask(taskId);
    if (task) {
        task->resume();
        return true;
    }
    return false;
}

void TaskScheduler::sleepMs(unsigned long ms) {
    // Safety check: ensure we're running in a task context
    if (xTaskGetCurrentTaskHandle() == NULL) {
        // Not in a task context, use regular delay
        delay(ms);
        return;
    }
    
    // Use FreeRTOS delay for proper task switching
    if (ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    } else {
        // For zero delay, just yield
        taskYIELD();
    }
}

void TaskScheduler::yield() {
    taskYIELD();
}

Task* TaskScheduler::getTask(uint32_t taskId) {
    for (size_t i = 0; i < MAX_TASKS; i++) {
        if (tasks[i] && tasks[i]->getId() == taskId) {
            return tasks[i];
        }
    }
    return nullptr;
}

uint32_t TaskScheduler::getCurrentTaskId() const {
    TaskHandle_t currentHandle = xTaskGetCurrentTaskHandle();
    if (!currentHandle) return 0;
    
    // Find the task with matching handle
    for (size_t i = 0; i < MAX_TASKS; i++) {
        if (tasks[i] && tasks[i]->getHandle() == currentHandle) {
            return tasks[i]->getId();
        }
    }
    
    // Return 0 if not found (could be a system task or untracked task)
    return 0;
}

uint32_t TaskScheduler::getFreeHeapSize() {
    return xPortGetFreeHeapSize();
}

} // namespace dialOS
