#ifndef DIALOS_MEMORY_H
#define DIALOS_MEMORY_H

#include <Arduino.h>

namespace dialOS {

/**
 * @brief Memory block header for tracking allocations
 */
struct MemoryBlock {
    uint32_t taskId;     // Owner task ID
    size_t size;         // Block size
    bool inUse;          // Allocation status
    MemoryBlock* next;   // Next block in list
};

/**
 * @brief Memory manager with task isolation and bounds checking
 */
class MemoryManager {
public:
    MemoryManager();
    ~MemoryManager();
    
    bool init(size_t heapSize);
    
    // Memory allocation
    void* allocate(size_t size, uint32_t taskId);
    bool free(void* ptr, uint32_t taskId);
    
    // Memory queries
    size_t getAvailable() const;
    size_t getUsedByTask(uint32_t taskId) const;
    size_t getTotalUsed() const;
    
    // Task cleanup
    void freeAllForTask(uint32_t taskId);
    
    // Memory statistics
    struct MemoryStats {
        size_t totalHeap;
        size_t usedHeap;
        size_t freeHeap;
        size_t largestFreeBlock;
        size_t allocations;
    };
    
    MemoryStats getStats() const;
    
private:
    static const size_t DEFAULT_HEAP_SIZE = 32768;  // 32KB heap
    static const size_t MAX_TASK_MEMORY = 16384;    // 16KB per task max
    
    uint8_t* heap;
    size_t heapSize;
    MemoryBlock* freeList;
    MemoryBlock* usedList;
    
    size_t totalAllocated;
    size_t allocationCount;
    
    MemoryBlock* findFreeBlock(size_t size);
    void coalesceBlocks();
    bool isValidPointer(void* ptr) const;
};

} // namespace dialOS

#endif // DIALOS_MEMORY_H
