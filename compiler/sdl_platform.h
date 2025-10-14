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
    
    Color(uint32_t rgba = 0x000000FF) : 
        r((rgba >> 24) & 0xFF), 
        g((rgba >> 16) & 0xFF), 
        b((rgba >> 8) & 0xFF), 
        a(rgba & 0xFF) {}
    
    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) :
        r(red), g(green), b(blue), a(alpha) {}
    
    SDL_Color toSDL() const { return {r, g, b, a}; }
};

// M5 Dial hardware specifications
class SDLPlatform : public PlatformInterface {
public:
    static const int DISPLAY_WIDTH = 240;
    static const int DISPLAY_HEIGHT = 240;
    static const int WINDOW_SCALE = 3;  // Scale factor for easier viewing
    static const int WINDOW_WIDTH = DISPLAY_WIDTH * WINDOW_SCALE;
    static const int WINDOW_HEIGHT = DISPLAY_HEIGHT * WINDOW_SCALE;
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
    
    // Display operations
    void display_clear(uint32_t color) override;
    void display_drawText(int x, int y, const std::string& text, 
                          uint32_t color, int size) override;
    void display_drawPixel(int x, int y, uint32_t color);
    void display_drawLine(int x1, int y1, int x2, int y2, uint32_t color);
    void display_drawRect(int x, int y, int w, int h, uint32_t color, bool filled = false);
    void display_drawCircle(int x, int y, int radius, uint32_t color, bool filled = false);
    void display_setBrightness(uint8_t brightness);
    
    // Input operations
    bool encoder_getButton() override;
    int encoder_getDelta() override;
    void encoder_reset();
    
    // Touch operations
    bool touch_isPressed();
    void touch_getPosition(int& x, int& y);
    bool touch_isInDisplay(int x, int y); // Check if touch is within circular display
    
    // RFID simulation (keyboard input)
    bool rfid_isCardPresent();
    std::string rfid_getCardUID();
    
    // Audio/Buzzer operations
    void buzzer_beep(uint32_t frequency, uint32_t duration);
    void buzzer_stop();
    
    // Timing operations
    uint32_t system_getTime() override;
    void system_sleep(uint32_t ms) override;
    uint64_t system_getRTC();
    void system_setRTC(uint64_t timestamp);
    
    // Debug/Console
    void console_log(const std::string& msg) override;
    
    // === Extended Emulator Features ===
    
    // GPIO simulation for expansion ports
    void gpio_pinMode(uint8_t pin, uint8_t mode);
    void gpio_digitalWrite(uint8_t pin, bool value);
    bool gpio_digitalRead(uint8_t pin);
    void gpio_analogWrite(uint8_t pin, uint16_t value);
    uint16_t gpio_analogRead(uint8_t pin);
    
    // I2C simulation for storage expansion
    void i2c_beginTransmission(uint8_t address);
    void i2c_write(uint8_t data);
    uint8_t i2c_read();
    void i2c_endTransmission();
    std::vector<uint8_t> i2c_scanDevices();
    
    // Power management simulation
    uint8_t power_getBatteryLevel();
    bool power_isCharging();
    void power_sleep();
    
    // Debug overlay
    void debug_showInfo(bool show) { showDebugInfo_ = show; }
    void debug_drawOverlay();
    
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
    
    // Helper methods
    bool isInCircularDisplay(int x, int y) const;
    void scaleCoordinates(int& x, int& y) const;
    void drawCircularMask();
    void updateInputs();
    void renderText(int x, int y, const std::string& text, const Color& color, int size);
    void addDebugMessage(const std::string& msg);
};

} // namespace vm
} // namespace dialos

#endif // DIALOS_SDL_PLATFORM_H