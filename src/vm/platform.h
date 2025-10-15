/**
 * dialScript Platform Interface
 * 
 * Abstract interface for platform-specific operations
 */

#ifndef DIALOS_VM_PLATFORM_H
#define DIALOS_VM_PLATFORM_H

#include <cstdint>
#include <string>
#include <vector>

namespace dialos {
namespace vm {

// Native function IDs
// Organization: High byte = namespace, Low byte = function within namespace
enum class NativeFunctionID : uint16_t {
    // Console namespace (0x00xx)
    CONSOLE_LOG         = 0x0000,
    CONSOLE_WARN        = 0x0001,
    CONSOLE_ERROR       = 0x0002,
    
    // Display namespace (0x01xx)
    DISPLAY_CLEAR       = 0x0100,
    DISPLAY_DRAW_TEXT   = 0x0101,
    DISPLAY_DRAW_RECT   = 0x0102,
    DISPLAY_DRAW_CIRCLE = 0x0103,
    DISPLAY_DRAW_LINE   = 0x0104,
    DISPLAY_DRAW_PIXEL  = 0x0105,
    DISPLAY_SET_BRIGHTNESS = 0x0106,
    DISPLAY_GET_WIDTH   = 0x0107,
    DISPLAY_GET_HEIGHT  = 0x0108,
    
    // Encoder namespace (0x02xx)
    ENCODER_GET_BUTTON  = 0x0200,
    ENCODER_GET_DELTA   = 0x0201,
    ENCODER_GET_POSITION = 0x0202,
    ENCODER_RESET       = 0x0203,
    
    // System namespace (0x03xx)
    SYSTEM_GET_TIME     = 0x0300,
    SYSTEM_SLEEP        = 0x0301,
    SYSTEM_GET_RTC      = 0x0302,
    SYSTEM_SET_RTC      = 0x0303,
    
    // Touch namespace (0x04xx)
    TOUCH_GET_X         = 0x0400,
    TOUCH_GET_Y         = 0x0401,
    TOUCH_IS_PRESSED    = 0x0402,
    
    // RFID namespace (0x05xx)
    RFID_READ           = 0x0500,
    RFID_IS_PRESENT     = 0x0501,
    
    // File namespace (0x06xx)
    FILE_OPEN           = 0x0600,
    FILE_READ           = 0x0601,
    FILE_WRITE          = 0x0602,
    FILE_CLOSE          = 0x0603,
    FILE_EXISTS         = 0x0604,
    FILE_DELETE         = 0x0605,
    FILE_SIZE           = 0x0606,
    
    // GPIO namespace (0x07xx)
    GPIO_PIN_MODE       = 0x0700,
    GPIO_DIGITAL_WRITE  = 0x0701,
    GPIO_DIGITAL_READ   = 0x0702,
    GPIO_ANALOG_WRITE   = 0x0703,
    GPIO_ANALOG_READ    = 0x0704,
    
    // I2C namespace (0x08xx)
    I2C_SCAN            = 0x0800,
    I2C_WRITE           = 0x0801,
    I2C_READ            = 0x0802,
    
    // Buzzer namespace (0x09xx)
    BUZZER_BEEP         = 0x0900,
    BUZZER_STOP         = 0x0901,
    
    // Timer namespace (0x0Axx)
    TIMER_SET_TIMEOUT   = 0x0A00,
    TIMER_SET_INTERVAL  = 0x0A01,
    TIMER_CLEAR         = 0x0A02,
    
    // Memory namespace (0x0Bxx)
    MEMORY_GET_AVAILABLE = 0x0B00,
    MEMORY_GET_USAGE    = 0x0B01,
    
    UNKNOWN             = 0xFFFF
};

// Helper function to map function name to ID
inline NativeFunctionID getNativeFunctionID(const std::string& name) {
    // Console functions
    if (name == "log") return NativeFunctionID::CONSOLE_LOG;
    if (name == "warn") return NativeFunctionID::CONSOLE_WARN;
    if (name == "error") return NativeFunctionID::CONSOLE_ERROR;
    
    // Display functions
    if (name == "clear") return NativeFunctionID::DISPLAY_CLEAR;
    if (name == "drawText") return NativeFunctionID::DISPLAY_DRAW_TEXT;
    if (name == "drawRect") return NativeFunctionID::DISPLAY_DRAW_RECT;
    if (name == "drawCircle") return NativeFunctionID::DISPLAY_DRAW_CIRCLE;
    if (name == "drawLine") return NativeFunctionID::DISPLAY_DRAW_LINE;
    if (name == "drawPixel") return NativeFunctionID::DISPLAY_DRAW_PIXEL;
    if (name == "setBrightness") return NativeFunctionID::DISPLAY_SET_BRIGHTNESS;
    if (name == "getWidth") return NativeFunctionID::DISPLAY_GET_WIDTH;
    if (name == "getHeight") return NativeFunctionID::DISPLAY_GET_HEIGHT;
    
    // Encoder functions
    if (name == "getButton") return NativeFunctionID::ENCODER_GET_BUTTON;
    if (name == "getDelta") return NativeFunctionID::ENCODER_GET_DELTA;
    if (name == "getPosition") return NativeFunctionID::ENCODER_GET_POSITION;
    if (name == "reset") return NativeFunctionID::ENCODER_RESET;
    
    // System functions
    if (name == "getTime") return NativeFunctionID::SYSTEM_GET_TIME;
    if (name == "sleep") return NativeFunctionID::SYSTEM_SLEEP;
    if (name == "getRTC") return NativeFunctionID::SYSTEM_GET_RTC;
    if (name == "setRTC") return NativeFunctionID::SYSTEM_SET_RTC;
    
    // Touch functions
    if (name == "getX") return NativeFunctionID::TOUCH_GET_X;
    if (name == "getY") return NativeFunctionID::TOUCH_GET_Y;
    if (name == "isPressed") return NativeFunctionID::TOUCH_IS_PRESSED;
    
    // RFID functions
    if (name == "read") return NativeFunctionID::RFID_READ;
    if (name == "isPresent") return NativeFunctionID::RFID_IS_PRESENT;
    
    // File functions
    if (name == "open") return NativeFunctionID::FILE_OPEN;
    if (name == "write") return NativeFunctionID::FILE_WRITE;
    if (name == "close") return NativeFunctionID::FILE_CLOSE;
    if (name == "exists") return NativeFunctionID::FILE_EXISTS;
    if (name == "delete") return NativeFunctionID::FILE_DELETE;
    if (name == "size") return NativeFunctionID::FILE_SIZE;
    
    // GPIO functions
    if (name == "pinMode") return NativeFunctionID::GPIO_PIN_MODE;
    if (name == "digitalWrite") return NativeFunctionID::GPIO_DIGITAL_WRITE;
    if (name == "digitalRead") return NativeFunctionID::GPIO_DIGITAL_READ;
    if (name == "analogWrite") return NativeFunctionID::GPIO_ANALOG_WRITE;
    if (name == "analogRead") return NativeFunctionID::GPIO_ANALOG_READ;
    
    // I2C functions
    if (name == "scan") return NativeFunctionID::I2C_SCAN;
    
    // Buzzer functions
    if (name == "beep") return NativeFunctionID::BUZZER_BEEP;
    if (name == "stop") return NativeFunctionID::BUZZER_STOP;
    
    // Timer functions
    if (name == "setTimeout") return NativeFunctionID::TIMER_SET_TIMEOUT;
    if (name == "setInterval") return NativeFunctionID::TIMER_SET_INTERVAL;
    if (name == "clearTimer") return NativeFunctionID::TIMER_CLEAR;
    
    // Memory functions
    if (name == "getAvailable") return NativeFunctionID::MEMORY_GET_AVAILABLE;
    if (name == "getUsage") return NativeFunctionID::MEMORY_GET_USAGE;
    
    return NativeFunctionID::UNKNOWN;
}

// Get function name from ID (for debugging/disassembly)
inline const char* getNativeFunctionName(NativeFunctionID id) {
    switch (id) {
        case NativeFunctionID::CONSOLE_LOG: return "log";
        case NativeFunctionID::CONSOLE_WARN: return "warn";
        case NativeFunctionID::CONSOLE_ERROR: return "error";
        case NativeFunctionID::DISPLAY_CLEAR: return "clear";
        case NativeFunctionID::DISPLAY_DRAW_TEXT: return "drawText";
        case NativeFunctionID::DISPLAY_DRAW_RECT: return "drawRect";
        case NativeFunctionID::DISPLAY_DRAW_CIRCLE: return "drawCircle";
        case NativeFunctionID::DISPLAY_DRAW_LINE: return "drawLine";
        case NativeFunctionID::DISPLAY_DRAW_PIXEL: return "drawPixel";
        case NativeFunctionID::DISPLAY_SET_BRIGHTNESS: return "setBrightness";
        case NativeFunctionID::DISPLAY_GET_WIDTH: return "getWidth";
        case NativeFunctionID::DISPLAY_GET_HEIGHT: return "getHeight";
        case NativeFunctionID::ENCODER_GET_BUTTON: return "getButton";
        case NativeFunctionID::ENCODER_GET_DELTA: return "getDelta";
        case NativeFunctionID::ENCODER_GET_POSITION: return "getPosition";
        case NativeFunctionID::ENCODER_RESET: return "reset";
        case NativeFunctionID::SYSTEM_GET_TIME: return "getTime";
        case NativeFunctionID::SYSTEM_SLEEP: return "sleep";
        case NativeFunctionID::SYSTEM_GET_RTC: return "getRTC";
        case NativeFunctionID::SYSTEM_SET_RTC: return "setRTC";
        case NativeFunctionID::TOUCH_GET_X: return "getX";
        case NativeFunctionID::TOUCH_GET_Y: return "getY";
        case NativeFunctionID::TOUCH_IS_PRESSED: return "isPressed";
        case NativeFunctionID::RFID_READ: return "read";
        case NativeFunctionID::RFID_IS_PRESENT: return "isPresent";
        case NativeFunctionID::FILE_OPEN: return "open";
        case NativeFunctionID::FILE_READ: return "read";
        case NativeFunctionID::FILE_WRITE: return "write";
        case NativeFunctionID::FILE_CLOSE: return "close";
        case NativeFunctionID::FILE_EXISTS: return "exists";
        case NativeFunctionID::FILE_DELETE: return "delete";
        case NativeFunctionID::FILE_SIZE: return "size";
        case NativeFunctionID::GPIO_PIN_MODE: return "pinMode";
        case NativeFunctionID::GPIO_DIGITAL_WRITE: return "digitalWrite";
        case NativeFunctionID::GPIO_DIGITAL_READ: return "digitalRead";
        case NativeFunctionID::GPIO_ANALOG_WRITE: return "analogWrite";
        case NativeFunctionID::GPIO_ANALOG_READ: return "analogRead";
        case NativeFunctionID::I2C_SCAN: return "scan";
        case NativeFunctionID::I2C_WRITE: return "write";
        case NativeFunctionID::I2C_READ: return "read";
        case NativeFunctionID::BUZZER_BEEP: return "beep";
        case NativeFunctionID::BUZZER_STOP: return "stop";
        case NativeFunctionID::TIMER_SET_TIMEOUT: return "setTimeout";
        case NativeFunctionID::TIMER_SET_INTERVAL: return "setInterval";
        case NativeFunctionID::TIMER_CLEAR: return "clearTimer";
        case NativeFunctionID::MEMORY_GET_AVAILABLE: return "getAvailable";
        case NativeFunctionID::MEMORY_GET_USAGE: return "getUsage";
        default: return "unknown";
    }
}

// Platform abstraction interface
class PlatformInterface {
public:
    virtual ~PlatformInterface() = default;
    
    // ===== Console Operations =====
    virtual void console_log(const std::string& msg) = 0;
    virtual void console_warn(const std::string& msg) { console_log("[WARN] " + msg); }
    virtual void console_error(const std::string& msg) { console_log("[ERROR] " + msg); }
    virtual void program_output(const std::string& msg) { console_log(msg); }
    
    // ===== Display Operations =====
    virtual void display_clear(uint32_t color) = 0;
    virtual void display_drawText(int x, int y, const std::string& text, 
                                  uint32_t color, int size) = 0;
    virtual void display_drawRect(int x, int y, int w, int h, uint32_t color, bool filled) = 0;
    virtual void display_drawCircle(int x, int y, int r, uint32_t color, bool filled) = 0;
    virtual void display_drawLine(int x1, int y1, int x2, int y2, uint32_t color) = 0;
    virtual void display_drawPixel(int x, int y, uint32_t color) = 0;
    virtual void display_setBrightness(int level) = 0;
    virtual int display_getWidth() = 0;
    virtual int display_getHeight() = 0;
    
    // ===== Encoder Operations =====
    virtual bool encoder_getButton() = 0;
    virtual int encoder_getDelta() = 0;
    virtual int encoder_getPosition() { return 0; }
    virtual void encoder_reset() {}
    
    // ===== System Operations =====
    virtual uint32_t system_getTime() = 0;
    virtual void system_sleep(uint32_t ms) = 0;
    virtual uint32_t system_getRTC() { return 0; }
    virtual void system_setRTC(uint32_t timestamp) {}
    
    // ===== Touch Operations =====
    virtual int touch_getX() { return 0; }
    virtual int touch_getY() { return 0; }
    virtual bool touch_isPressed() { return false; }
    
    // ===== RFID Operations =====
    virtual std::string rfid_read() { return ""; }
    virtual bool rfid_isPresent() { return false; }
    
    // ===== File Operations =====
    virtual int file_open(const std::string& path, const std::string& mode) { return -1; }
    virtual std::string file_read(int handle, int size) { return ""; }
    virtual int file_write(int handle, const std::string& data) { return -1; }
    virtual void file_close(int handle) {}
    virtual bool file_exists(const std::string& path) { return false; }
    virtual bool file_delete(const std::string& path) { return false; }
    virtual int file_size(const std::string& path) { return -1; }
    
    // ===== GPIO Operations =====
    virtual void gpio_pinMode(int pin, int mode) {}
    virtual void gpio_digitalWrite(int pin, int value) {}
    virtual int gpio_digitalRead(int pin) { return 0; }
    virtual void gpio_analogWrite(int pin, int value) {}
    virtual int gpio_analogRead(int pin) { return 0; }
    
    // ===== I2C Operations =====
    virtual std::vector<int> i2c_scan() { return {}; }
    virtual bool i2c_write(int address, const std::vector<uint8_t>& data) { return false; }
    virtual std::vector<uint8_t> i2c_read(int address, int length) { return {}; }
    
    // ===== Buzzer Operations =====
    virtual void buzzer_beep(int frequency, int duration) {}
    virtual void buzzer_stop() {}
    
    // ===== Timer Operations =====
    virtual int timer_setTimeout(int ms) { return -1; }
    virtual int timer_setInterval(int ms) { return -1; }
    virtual void timer_clear(int id) {}
    
    // ===== Memory Operations =====
    virtual int memory_getAvailable() { return 0; }
    virtual int memory_getUsage() { return 0; }
};

} // namespace vm
} // namespace dialos

#endif // DIALOS_VM_PLATFORM_H
