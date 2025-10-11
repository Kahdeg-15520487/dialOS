#ifndef DIALOS_RAMFS_H
#define DIALOS_RAMFS_H

#include <Arduino.h>

namespace dialOS {

// File modes
enum class FileMode {
    READ,
    WRITE,
    APPEND
};

// File entry
struct FileEntry {
    char name[32];
    uint8_t* data;
    size_t size;
    size_t capacity;
    uint32_t taskId;      // Owner task
    bool inUse;
    FileEntry* next;
};

// File handle
struct FileHandle {
    FileEntry* entry;
    size_t position;
    FileMode mode;
    bool isOpen;
};

/**
 * @brief RAM-based file system for temporary storage
 */
class RamFS {
public:
    RamFS();
    ~RamFS();
    
    bool init(size_t maxFiles = 16, size_t maxTotalSize = 16384);
    
    // File operations
    int open(const char* path, FileMode mode, uint32_t taskId);
    bool close(int handle);
    size_t read(int handle, void* buffer, size_t size);
    size_t write(int handle, const void* data, size_t size);
    bool seek(int handle, size_t position);
    size_t tell(int handle);
    
    // File management
    bool exists(const char* path);
    bool remove(const char* path, uint32_t taskId);
    size_t getSize(const char* path);
    
    // Directory operations (simple flat structure)
    int listFiles(char** buffer, size_t maxFiles);
    
    // Statistics
    struct Stats {
        size_t totalFiles;
        size_t totalSize;
        size_t freeSpace;
        size_t maxFiles;
    };
    
    Stats getStats() const;
    
    // Cleanup
    void removeAllForTask(uint32_t taskId);
    
private:
    static const size_t MAX_HANDLES = 8;
    
    FileEntry* files;
    FileHandle handles[MAX_HANDLES];
    
    size_t maxFileCount;
    size_t currentFileCount;
    size_t maxTotalStorage;
    size_t currentStorageUsed;
    
    FileEntry* findFile(const char* path);
    FileEntry* createFile(const char* path, uint32_t taskId);
    int allocateHandle();
    bool isValidHandle(int handle) const;
    bool canAllocate(size_t size) const;
};

} // namespace dialOS

#endif // DIALOS_RAMFS_H
