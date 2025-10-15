/**
 * SDL Platform Emulator for dialOS
 * 
 * Emulates M5 Dial hardware using SDL2:
 * - 240x240 circular display (simulated as circular region in square window)
 * - Rotary encoder with button
 * - Touch interface
 * - RFID simulation via keyboard
 * - Buzzer audio simulation
 * - RTC and system timing
 */

#ifndef DIALOS_SDL_PLATFORM_H
#define DIALOS_SDL_PLATFORM_H

#include "../src/vm/platform.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <cstdint>
#include <string>
#include <chrono>
#include <map>
#include <vector>
#include <memory>

namespace dialos {
namespace vm {

// Color utilities
struct Color {
    uint8_t r, g, b, a;
    
    // Constructor: Convert RGB565 format to RGB888
    // RGB565: RRRRRGGGGGGBBBBB (5 bits red, 6 bits green, 5 bits blue)
    Color(uint32_t rgb565 = 0x0000) {
        // Extract 5-bit red, 6-bit green, 5-bit blue
        uint8_t r5 = (rgb565 >> 11) & 0x1F;  // Top 5 bits
        uint8_t g6 = (rgb565 >> 5) & 0x3F;   // Middle 6 bits
        uint8_t b5 = rgb565 & 0x1F;           // Bottom 5 bits
        
        // Convert to 8-bit values by scaling
        r = (r5 * 255) / 31;  // 5-bit to 8-bit
        g = (g6 * 255) / 63;  // 6-bit to 8-bit  
        b = (b5 * 255) / 31;  // 5-bit to 8-bit
        a = 255;              // Fully opaque
    }
    
    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) :
        r(red), g(green), b(blue), a(alpha) {}
    
    SDL_Color toSDL() const { return {r, g, b, a}; }
};

// M5 Dial hardware specifications
class SDLPlatform : public PlatformInterface {
public:
    static const int DISPLAY_WIDTH = 240;
    static const int DISPLAY_HEIGHT = 240;
    static const int WINDOW_SCALE = 2;  // Scale factor for easier viewing
    static const int DISPLAY_SCALED_WIDTH = DISPLAY_WIDTH * WINDOW_SCALE;
    static const int DISPLAY_SCALED_HEIGHT = DISPLAY_HEIGHT * WINDOW_SCALE;
    static const int CONSOLE_WIDTH = 300;  // Width of console area
    static const int WINDOW_WIDTH = DISPLAY_SCALED_WIDTH + CONSOLE_WIDTH;
    static const int WINDOW_HEIGHT = DISPLAY_SCALED_HEIGHT;
    static const int CENTER_X = DISPLAY_WIDTH / 2;
    static const int CENTER_Y = DISPLAY_HEIGHT / 2;
    static const int DISPLAY_RADIUS = 120;  // Circular display radius
    
    // Constructor/Destructor
    SDLPlatform();
    ~SDLPlatform();
    
    // Platform initialization
    bool initialize(const std::string& title = "dialOS Emulator");
    void cleanup();
    
    // Main emulator loop control
    bool pollEvents();
    void present();
    bool shouldQuit() const { return shouldQuit_; }
    
    // === PlatformInterface Implementation ===
    
    // === Console Operations ===
    void console_log(const std::string& msg) override;
    void console_warn(const std::string& msg) override;
    void console_error(const std::string& msg) override;
    
    // === Display Operations ===
    void display_clear(uint32_t color) override;
    void display_drawText(int x, int y, const std::string& text, 
                          uint32_t color, int size) override;
    void display_drawPixel(int x, int y, uint32_t color) override;
    void display_drawLine(int x1, int y1, int x2, int y2, uint32_t color) override;
    void display_drawRect(int x, int y, int w, int h, uint32_t color, bool filled) override;
    void display_drawCircle(int x, int y, int radius, uint32_t color, bool filled) override;
    void display_setBrightness(int brightness) override;
    int display_getWidth() override;
    int display_getHeight() override;
    
    // === Encoder Operations ===
    bool encoder_getButton() override;
    int encoder_getDelta() override;
    int encoder_getPosition() override;
    void encoder_reset() override;
    
    // === System Operations ===
    uint32_t system_getTime() override;
    void system_sleep(uint32_t ms) override;
    uint32_t system_getRTC() override;
    void system_setRTC(uint32_t timestamp) override;
    
    // === Touch Operations ===
    int touch_getX() override;
    int touch_getY() override;
    bool touch_isPressed() override;
    
    // === RFID Operations ===
    std::string rfid_read() override;
    bool rfid_isPresent() override;
    
    // === File Operations ===
    int file_open(const std::string& path, const std::string& mode) override;
    std::string file_read(int handle, int size) override;
    int file_write(int handle, const std::string& data) override;
    void file_close(int handle) override;
    bool file_exists(const std::string& path) override;
    bool file_delete(const std::string& path) override;
    int file_size(const std::string& path) override;
    
    // === GPIO Operations ===
    void gpio_pinMode(int pin, int mode) override;
    void gpio_digitalWrite(int pin, int value) override;
    int gpio_digitalRead(int pin) override;
    void gpio_analogWrite(int pin, int value) override;
    int gpio_analogRead(int pin) override;
    
    // === I2C Operations ===
    std::vector<int> i2c_scan() override;
    bool i2c_write(int address, const std::vector<uint8_t>& data) override;
    std::vector<uint8_t> i2c_read(int address, int length) override;
    
    // === Buzzer Operations ===
    void buzzer_beep(int frequency, int duration) override;
    void buzzer_stop() override;
    
    // === Timer Operations ===
    int timer_setTimeout(int ms) override;
    int timer_setInterval(int ms) override;
    void timer_clear(int id) override;
    
    // === Memory Operations ===
    int memory_getAvailable() override;
    int memory_getUsage() override;
    
    // === Extended Emulator Features ===
    void program_output(const std::string& msg);
    void touch_getPosition(int& x, int& y);
    bool touch_isInDisplay(int x, int y); // Check if touch is within circular display
    
    // Power management simulation
    uint8_t power_getBatteryLevel();
    bool power_isCharging();
    void power_sleep();
    
    // Debug overlay
    void debug_showInfo(bool show) { showDebugInfo_ = show; }
    void debug_drawOverlay();
    
    // Output capture for VM programs
    void captureOutput(const std::string& output) { outputLog_.addLine(output); }
    
private:
    // SDL components
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    TTF_Font* font_;
    bool initialized_;
    bool shouldQuit_;
    
    // Display state
    uint32_t backgroundColor_;
    uint8_t brightness_;
    
    // Input state
    struct {
        bool pressed;
        bool lastPressed;
        int position;
        int lastPosition;
        std::chrono::steady_clock::time_point lastUpdate;
    } encoder_;
    
    struct {
        bool pressed;
        bool lastPressed;
        int x, y;
        std::chrono::steady_clock::time_point lastUpdate;
    } touch_;
    
    // RFID simulation
    struct {
        bool cardPresent;
        std::string cardUID;
        std::chrono::steady_clock::time_point lastUpdate;
    } rfid_;
    
    // Audio
    struct {
        bool isPlaying;
        uint32_t frequency;
        std::chrono::steady_clock::time_point endTime;
    } buzzer_;
    
    // Timing
    std::chrono::steady_clock::time_point startTime_;
    uint64_t rtcOffset_;
    
    // GPIO simulation
    struct GPIOPin {
        uint8_t mode;      // INPUT, OUTPUT, INPUT_PULLUP
        bool digitalValue;
        uint16_t analogValue;
    };
    std::map<uint8_t, GPIOPin> gpioPins_;
    
    // I2C simulation
    struct {
        uint8_t currentAddress;
        std::vector<uint8_t> writeBuffer;
        std::vector<uint8_t> readBuffer;
        size_t readIndex;
        std::map<uint8_t, std::vector<uint8_t>> deviceMemory;
    } i2c_;
    
    // Power simulation
    struct {
        uint8_t batteryLevel;
        bool charging;
        std::chrono::steady_clock::time_point lastUpdate;
    } power_;
    
    // Debug overlay
    bool showDebugInfo_;
    std::vector<std::string> debugMessages_;
    
    // Console logging
    struct ConsoleLog {
        std::vector<std::string> lines;
        size_t maxLines;
        size_t scrollOffset;
        
        ConsoleLog(size_t max = 50) : maxLines(max), scrollOffset(0) {}
        
        void addLine(const std::string& line) {
            lines.push_back(line);
            if (lines.size() > maxLines) {
                lines.erase(lines.begin());
            }
        }
        
        void clear() {
            lines.clear();
            scrollOffset = 0;
        }
    };
    
    ConsoleLog consoleLog_;
    ConsoleLog outputLog_;
    
    // Helper methods
    bool isInCircularDisplay(int x, int y) const;
    void scaleCoordinates(int& x, int& y) const;
    void drawCircularMask();
    void drawEncoderIndicator();
    void updateInputs();
    void renderText(int x, int y, const std::string& text, const Color& color, int size);
    void addDebugMessage(const std::string& msg);
    void renderConsoleArea();
    void renderLogWindow(int x, int y, int width, int height, const std::string& title, const ConsoleLog& log);
};

} // namespace vm
} // namespace dialos

#endif // DIALOS_SDL_PLATFORM_H