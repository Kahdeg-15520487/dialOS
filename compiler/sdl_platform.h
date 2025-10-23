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

#include "vm/platform.h"
#include "vm/vm_value.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <cstdint>
#include <string>
#include <chrono>
#include <map>
#include <vector>
#include <memory>
#include <fstream>
#include <filesystem>

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
    static const int DEBUG_PANEL_WIDTH = 250;  // Width of debug panel on left
    static const int CONSOLE_WIDTH = 300;  // Width of console area on right
    static const int WINDOW_WIDTH = DEBUG_PANEL_WIDTH + DISPLAY_SCALED_WIDTH + CONSOLE_WIDTH;
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
    
    // Debug overlay control
    void debug_showInfo(bool show);
    void debug_drawOverlay();
    
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
    // New: accept callback Value and interval ms
    int timer_setInterval(const Value& callback, int ms) override;
    void timer_clearTimeout(int id) override;
    void timer_clearInterval(int id) override;
    
    // === Memory Operations ===
    int memory_getAvailable() override;
    int memory_getUsage() override;
    int memory_allocate(int size) override;
    void memory_free(int handle) override;
    
    // === Console Operations ===
    void console_print(const std::string& msg) override;
    void console_println(const std::string& msg) override;
    void console_clear() override;
    
    // === Display Operations ===
    void display_setTitle(const std::string& title) override;
    void display_drawImage(int x, int y, const std::vector<uint8_t>& imageData) override;
    
    // === System Operations ===
    void system_yield() override;
    
    // === Directory Operations ===
    std::vector<std::string> dir_list(const std::string& path) override;
    bool dir_create(const std::string& path) override;
    bool dir_delete(const std::string& path) override;
    bool dir_exists(const std::string& path) override;
    
    // === Power Operations ===
    void power_sleep() override;
    int power_getBatteryLevel() override;
    bool power_isCharging() override;
    
    // === App Operations ===
    void app_exit() override;
    std::string app_getInfo() override;
    
    // === Storage Operations ===
    std::vector<std::string> storage_getMounted() override;
    std::string storage_getInfo(const std::string& device) override;
    
    // === Sensor Operations ===
    int sensor_attach(const std::string& port, const std::string& type) override;
    std::string sensor_read(int handle) override;
    void sensor_detach(int handle) override;
    
    // === WiFi Operations ===
    bool wifi_connect(const std::string& ssid, const std::string& password) override;
    void wifi_disconnect() override;
    std::string wifi_getStatus() override;
    std::string wifi_getIP() override;
    std::string wifi_scan() override;
    
    // === HTTP Operations ===
    std::string http_get(const std::string& url) override;
    std::string http_post(const std::string& url, const std::string& data) override;
    
    // === IPC Operations ===
    bool ipc_send(const std::string& appId, const std::string& message) override;
    void ipc_broadcast(const std::string& message) override;
    
    // === Buzzer Operations ===
    void buzzer_playMelody(const std::vector<int>& notes) override;
    
    // === Extended Emulator Features ===
    void touch_getPosition(int& x, int& y);
    bool touch_isInDisplay(int x, int y); // Check if touch is within circular display

    // Source lookup & runtime error printing (platform-level diagnostics)
    std::string locateSourceFile(const std::string &bytecodePath) override;
    void printRuntimeError(const VMState &vm, const std::string &bytecodePath, size_t pc, const std::string &errorMessage, uint32_t sourceLine = 0) override;
    
    // Output capture for VM programs
    void captureOutput(const std::string& output) { outputLog_.addText(output); }
    
private:
    // Internal helpers (private static)
    static std::string getSourceContextFromFile(const std::string &sourceFile, uint32_t errorLine, int contextLines = 5);
    static std::string locateSourceForBytecode(const std::string &bytecodePath);
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
    
    // WiFi simulation state
    bool wifiConnected_ = false;
    std::string wifiSSID_;
    std::string wifiIP_;
    
    // HTTP helper functions
    bool parseURL(const std::string& url, std::string& host, std::string& path);
    std::string executeHTTPRequest(const std::string& method, const std::string& host, 
                                   const std::string& path, const std::string& data);
    
    // File system simulation
    struct FileHandle {
        std::fstream stream;
        std::string path;
        std::string mode;
        bool isOpen;
        
        FileHandle() : isOpen(false) {}
    };
    std::map<int, FileHandle> fileHandles_;
    int nextFileHandle_ = 1;
    std::string fileSystemRoot_ = "./sdl_filesystem/";  // Local directory for simulated files
    
    std::vector<std::string> debugMessages_;
    
    // Console logging
    struct ConsoleLog {
        std::string buffer;
        size_t maxLines;
        
        ConsoleLog(size_t max = 50) : maxLines(max) {}
        
        void addText(const std::string& text) {
            buffer += text;
            // Keep only last maxLines lines
            size_t lines = 0;
            size_t pos = 0;
            while ((pos = buffer.find('\n', pos)) != std::string::npos) {
                lines++;
                pos++;
            }
            if (lines > maxLines) {
                // Find the position after maxLines newlines
                size_t cutPos = 0;
                for (size_t i = 0; i < lines - maxLines + 1; i++) {
                    cutPos = buffer.find('\n', cutPos) + 1;
                }
                buffer = buffer.substr(cutPos);
            }
        }
        
        std::vector<std::string> getLines() const {
            std::vector<std::string> lines;
            size_t start = 0;
            size_t end = buffer.find('\n');
            while (end != std::string::npos) {
                lines.push_back(buffer.substr(start, end - start));
                start = end + 1;
                end = buffer.find('\n', start);
            }
            if (start < buffer.size()) {
                lines.push_back(buffer.substr(start));
            }
            return lines;
        }
        
        void clear() {
            buffer.clear();
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
    void renderDebugPanel();
    // Timer processing called from pollEvents/present
    void processTimers();

    // Timer entry
    struct TimerEntry {
        int id;
        Value callback;
        int intervalMs;
        std::chrono::steady_clock::time_point nextFire;
        bool repeating;
    };

    // Timer container
    std::map<int, TimerEntry> timers_;
    int nextTimerId_ = 1;
};

} // namespace vm
} // namespace dialos

#endif // DIALOS_SDL_PLATFORM_H