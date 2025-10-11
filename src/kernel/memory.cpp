#include "kernel/memory.h"
#include "kernel/kernel.h"
#include "kernel/system.h"

namespace dialOS {

MemoryManager::MemoryManager()
    : heap(nullptr)
    , heapSize(0)
    , freeList(nullptr)
    , usedList(nullptr)
    , totalAllocated(0)
    , allocationCount(0) {
}

MemoryManager::~MemoryManager() {
    if (heap) {
        ::free(heap);  // Use global free, not our member function
    }
}

bool MemoryManager::init(size_t size) {
    if (heap) {
        return false;  // Already initialized
    }
    
    heapSize = size;
    heap = (uint8_t*)malloc(heapSize);
    
    if (!heap) {
        Kernel::instance().getSystemServices()->log(LogLevel::ERROR, 
            "Failed to allocate heap");
        return false;
    }
    
    // Initialize free list with entire heap
    freeList = (MemoryBlock*)heap;
    freeList->taskId = 0;
    freeList->size = heapSize - sizeof(MemoryBlock);
    freeList->inUse = false;
    freeList->next = nullptr;
    
    usedList = nullptr;
    
    Kernel::instance().getSystemServices()->logf(LogLevel::INFO, 
        "Memory heap initialized: %u bytes", heapSize);
    
    return true;
}

void* MemoryManager::allocate(size_t size, uint32_t taskId) {
    if (!heap || size == 0) {
        return nullptr;
    }
    
    // Check task memory limit
    size_t taskUsed = getUsedByTask(taskId);
    if (taskUsed + size > MAX_TASK_MEMORY) {
        Kernel::instance().getSystemServices()->logf(LogLevel::ERROR, 
            "Task %lu exceeded memory limit", taskId);
        return nullptr;
    }
    
    MemoryBlock* block = findFreeBlock(size + sizeof(MemoryBlock));
    if (!block) {
        Kernel::instance().getSystemServices()->log(LogLevel::ERROR, "Out of memory");
        return nullptr;
    }
    
    // Split block if necessary
    size_t requiredSize = size + sizeof(MemoryBlock);
    if (block->size > requiredSize + sizeof(MemoryBlock) + 16) {
        MemoryBlock* newBlock = (MemoryBlock*)((uint8_t*)block + requiredSize);
        newBlock->size = block->size - requiredSize;
        newBlock->inUse = false;
        newBlock->next = block->next;
        
        block->size = size;
        block->next = newBlock;
    }
    
    block->taskId = taskId;
    block->inUse = true;
    
    totalAllocated += size;
    allocationCount++;
    
    // Return pointer after header
    return (void*)((uint8_t*)block + sizeof(MemoryBlock));
}

bool MemoryManager::free(void* ptr, uint32_t taskId) {
    if (!ptr || !isValidPointer(ptr)) {
        return false;
    }
    
    MemoryBlock* block = (MemoryBlock*)((uint8_t*)ptr - sizeof(MemoryBlock));
    
    // Verify ownership
    if (block->taskId != taskId) {
        Kernel::instance().getSystemServices()->log(LogLevel::ERROR, 
            "Task attempting to free memory it doesn't own");
        return false;
    }
    
    if (!block->inUse) {
        Kernel::instance().getSystemServices()->log(LogLevel::ERROR, 
            "Double free detected");
        return false;
    }
    
    block->inUse = false;
    block->taskId = 0;
    totalAllocated -= block->size;
    
    // Coalesce adjacent free blocks
    coalesceBlocks();
    
    return true;
}

size_t MemoryManager::getAvailable() const {
    return heapSize - totalAllocated;
}

size_t MemoryManager::getUsedByTask(uint32_t taskId) const {
    size_t used = 0;
    MemoryBlock* current = (MemoryBlock*)heap;
    
    while ((uint8_t*)current < heap + heapSize) {
        if (current->inUse && current->taskId == taskId) {
            used += current->size;
        }
        
        if (current->next) {
            current = current->next;
        } else {
            break;
        }
    }
    
    return used;
}

size_t MemoryManager::getTotalUsed() const {
    return totalAllocated;
}

void MemoryManager::freeAllForTask(uint32_t taskId) {
    MemoryBlock* current = (MemoryBlock*)heap;
    
    while ((uint8_t*)current < heap + heapSize) {
        if (current->inUse && current->taskId == taskId) {
            current->inUse = false;
            current->taskId = 0;
            totalAllocated -= current->size;
        }
        
        if (current->next) {
            current = current->next;
        } else {
            break;
        }
    }
    
    coalesceBlocks();
}

MemoryManager::MemoryStats MemoryManager::getStats() const {
    MemoryStats stats;
    stats.totalHeap = heapSize;
    stats.usedHeap = totalAllocated;
    stats.freeHeap = heapSize - totalAllocated;
    stats.largestFreeBlock = 0;
    stats.allocations = allocationCount;
    
    MemoryBlock* current = (MemoryBlock*)heap;
    while ((uint8_t*)current < heap + heapSize) {
        if (!current->inUse && current->size > stats.largestFreeBlock) {
            stats.largestFreeBlock = current->size;
        }
        
        if (current->next) {
            current = current->next;
        } else {
            break;
        }
    }
    
    return stats;
}

MemoryBlock* MemoryManager::findFreeBlock(size_t size) {
    MemoryBlock* current = (MemoryBlock*)heap;
    
    while ((uint8_t*)current < heap + heapSize) {
        if (!current->inUse && current->size >= size) {
            return current;
        }
        
        if (current->next) {
            current = current->next;
        } else {
            break;
        }
    }
    
    return nullptr;
}

void MemoryManager::coalesceBlocks() {
    MemoryBlock* current = (MemoryBlock*)heap;
    
    while ((uint8_t*)current < heap + heapSize && current->next) {
        if (!current->inUse && !current->next->inUse) {
            // Merge with next block
            current->size += current->next->size + sizeof(MemoryBlock);
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

bool MemoryManager::isValidPointer(void* ptr) const {
    uint8_t* p = (uint8_t*)ptr;
    return (p > heap && p < heap + heapSize);
}

} // namespace dialOS
