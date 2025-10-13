/**
 * dialScript Platform Interface
 * 
 * Abstract interface for platform-specific operations
 */

#ifndef DIALOS_VM_PLATFORM_H
#define DIALOS_VM_PLATFORM_H

#include <cstdint>
#include <string>

namespace dialos {
namespace vm {

// Platform abstraction interface
class PlatformInterface {
public:
    virtual ~PlatformInterface() = default;
    
    // Display operations
    virtual void display_clear(uint32_t color) = 0;
    virtual void display_drawText(int x, int y, const std::string& text, 
                                  uint32_t color, int size) = 0;
    
    // Input operations
    virtual bool encoder_getButton() = 0;
    virtual int encoder_getDelta() = 0;
    
    // Timing operations
    virtual uint32_t system_getTime() = 0;
    virtual void system_sleep(uint32_t ms) = 0;
    
    // Debug/Console
    virtual void console_log(const std::string& msg) = 0;
};

} // namespace vm
} // namespace dialos

#endif // DIALOS_VM_PLATFORM_H
