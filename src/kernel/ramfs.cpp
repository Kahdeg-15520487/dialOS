#include "kernel/ramfs.h"
#include "kernel/kernel.h"
#include "kernel/system.h"
#include <cstring>

namespace dialOS {

RamFS::RamFS()
    : files(nullptr)
    , maxFileCount(0)
    , currentFileCount(0)
    , maxTotalStorage(0)
    , currentStorageUsed(0) {
    
    for (size_t i = 0; i < MAX_HANDLES; i++) {
        handles[i].entry = nullptr;
        handles[i].position = 0;
        handles[i].mode = FileMode::READ;
        handles[i].isOpen = false;
    }
}

RamFS::~RamFS() {
    if (files) {
        FileEntry* current = files;
        while (current) {
            FileEntry* next = current->next;
            if (current->data) {
                ::free(current->data);
            }
            delete current;
            current = next;
        }
    }
}

bool RamFS::init(size_t maxFiles, size_t maxTotalSize) {
    maxFileCount = maxFiles;
    maxTotalStorage = maxTotalSize;
    files = nullptr;
    
    Kernel::instance().getSystemServices()->logf(LogLevel::INFO,
        "RamFS initialized: %u files, %u bytes total", maxFiles, maxTotalSize);
    
    return true;
}

FileEntry* RamFS::findFile(const char* path) {
    FileEntry* current = files;
    while (current) {
        if (strcmp(current->name, path) == 0 && current->inUse) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

FileEntry* RamFS::createFile(const char* path, uint32_t taskId) {
    if (currentFileCount >= maxFileCount) {
        Kernel::instance().getSystemServices()->log(LogLevel::ERROR,
            "RamFS: Maximum file count reached");
        return nullptr;
    }
    
    FileEntry* entry = new FileEntry();
    if (!entry) {
        return nullptr;
    }
    
    strncpy(entry->name, path, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';
    entry->data = nullptr;
    entry->size = 0;
    entry->capacity = 0;
    entry->taskId = taskId;
    entry->inUse = true;
    entry->next = files;
    
    files = entry;
    currentFileCount++;
    
    Kernel::instance().getSystemServices()->logf(LogLevel::DEBUG,
        "RamFS: Created file '%s' for task %lu", path, taskId);
    
    return entry;
}

int RamFS::allocateHandle() {
    for (size_t i = 0; i < MAX_HANDLES; i++) {
        if (!handles[i].isOpen) {
            return i;
        }
    }
    return -1;
}

bool RamFS::isValidHandle(int handle) const {
    return handle >= 0 && handle < MAX_HANDLES && handles[handle].isOpen;
}

bool RamFS::canAllocate(size_t size) const {
    return (currentStorageUsed + size) <= maxTotalStorage;
}

int RamFS::open(const char* path, FileMode mode, uint32_t taskId) {
    if (!path || strlen(path) == 0) {
        return -1;
    }
    
    int handleId = allocateHandle();
    if (handleId < 0) {
        Kernel::instance().getSystemServices()->log(LogLevel::ERROR,
            "RamFS: No available file handles");
        return -1;
    }
    
    FileEntry* entry = findFile(path);
    
    if (mode == FileMode::WRITE || mode == FileMode::APPEND) {
        if (!entry) {
            entry = createFile(path, taskId);
            if (!entry) {
                return -1;
            }
        } else {
            // Verify ownership for write operations
            if (entry->taskId != taskId) {
                Kernel::instance().getSystemServices()->logf(LogLevel::ERROR,
                    "RamFS: Task %lu cannot write to file owned by task %lu",
                    taskId, entry->taskId);
                return -1;
            }
        }
        
        if (mode == FileMode::WRITE) {
            // Truncate file
            entry->size = 0;
        }
    } else {
        // Read mode
        if (!entry) {
            Kernel::instance().getSystemServices()->logf(LogLevel::ERROR,
                "RamFS: File '%s' not found", path);
            return -1;
        }
    }
    
    handles[handleId].entry = entry;
    handles[handleId].mode = mode;
    handles[handleId].position = (mode == FileMode::APPEND) ? entry->size : 0;
    handles[handleId].isOpen = true;
    
    return handleId;
}

bool RamFS::close(int handle) {
    if (!isValidHandle(handle)) {
        return false;
    }
    
    handles[handle].entry = nullptr;
    handles[handle].position = 0;
    handles[handle].isOpen = false;
    
    return true;
}

size_t RamFS::read(int handle, void* buffer, size_t size) {
    if (!isValidHandle(handle) || !buffer) {
        return 0;
    }
    
    FileHandle& fh = handles[handle];
    
    if (fh.mode != FileMode::READ) {
        return 0;
    }
    
    if (!fh.entry->data || fh.position >= fh.entry->size) {
        return 0;
    }
    
    size_t available = fh.entry->size - fh.position;
    size_t toRead = (size < available) ? size : available;
    
    memcpy(buffer, fh.entry->data + fh.position, toRead);
    fh.position += toRead;
    
    return toRead;
}

size_t RamFS::write(int handle, const void* data, size_t size) {
    if (!isValidHandle(handle) || !data || size == 0) {
        return 0;
    }
    
    FileHandle& fh = handles[handle];
    
    if (fh.mode == FileMode::READ) {
        return 0;
    }
    
    size_t newSize = fh.position + size;
    
    // Check if we need to expand the buffer
    if (newSize > fh.entry->capacity) {
        size_t newCapacity = newSize * 2;  // Double the capacity
        
        if (!canAllocate(newCapacity - fh.entry->capacity)) {
            Kernel::instance().getSystemServices()->log(LogLevel::ERROR,
                "RamFS: Insufficient storage space");
            return 0;
        }
        
        uint8_t* newData = (uint8_t*)realloc(fh.entry->data, newCapacity);
        if (!newData) {
            Kernel::instance().getSystemServices()->log(LogLevel::ERROR,
                "RamFS: Memory allocation failed");
            return 0;
        }
        
        currentStorageUsed += (newCapacity - fh.entry->capacity);
        fh.entry->data = newData;
        fh.entry->capacity = newCapacity;
    }
    
    memcpy(fh.entry->data + fh.position, data, size);
    fh.position += size;
    
    if (fh.position > fh.entry->size) {
        fh.entry->size = fh.position;
    }
    
    return size;
}

bool RamFS::seek(int handle, size_t position) {
    if (!isValidHandle(handle)) {
        return false;
    }
    
    handles[handle].position = position;
    return true;
}

size_t RamFS::tell(int handle) {
    if (!isValidHandle(handle)) {
        return 0;
    }
    
    return handles[handle].position;
}

bool RamFS::exists(const char* path) {
    return findFile(path) != nullptr;
}

bool RamFS::remove(const char* path, uint32_t taskId) {
    FileEntry* prev = nullptr;
    FileEntry* current = files;
    
    while (current) {
        if (strcmp(current->name, path) == 0 && current->inUse) {
            // Verify ownership
            if (current->taskId != taskId) {
                Kernel::instance().getSystemServices()->logf(LogLevel::ERROR,
                    "RamFS: Task %lu cannot delete file owned by task %lu",
                    taskId, current->taskId);
                return false;
            }
            
            // Close any open handles
            for (size_t i = 0; i < MAX_HANDLES; i++) {
                if (handles[i].isOpen && handles[i].entry == current) {
                    close(i);
                }
            }
            
            // Remove from list
            if (prev) {
                prev->next = current->next;
            } else {
                files = current->next;
            }
            
            // Free memory
            if (current->data) {
                currentStorageUsed -= current->capacity;
                ::free(current->data);
            }
            
            delete current;
            currentFileCount--;
            
            Kernel::instance().getSystemServices()->logf(LogLevel::DEBUG,
                "RamFS: Deleted file '%s'", path);
            
            return true;
        }
        
        prev = current;
        current = current->next;
    }
    
    return false;
}

size_t RamFS::getSize(const char* path) {
    FileEntry* entry = findFile(path);
    return entry ? entry->size : 0;
}

int RamFS::listFiles(char** buffer, size_t maxFiles) {
    if (!buffer) {
        return 0;
    }
    
    int count = 0;
    FileEntry* current = files;
    
    while (current && count < maxFiles) {
        if (current->inUse) {
            buffer[count] = current->name;
            count++;
        }
        current = current->next;
    }
    
    return count;
}

RamFS::Stats RamFS::getStats() const {
    Stats stats;
    stats.totalFiles = currentFileCount;
    stats.totalSize = currentStorageUsed;
    stats.freeSpace = maxTotalStorage - currentStorageUsed;
    stats.maxFiles = maxFileCount;
    
    return stats;
}

void RamFS::removeAllForTask(uint32_t taskId) {
    FileEntry* current = files;
    
    while (current) {
        FileEntry* next = current->next;
        
        if (current->taskId == taskId) {
            remove(current->name, taskId);
        }
        
        current = next;
    }
}

} // namespace dialOS
