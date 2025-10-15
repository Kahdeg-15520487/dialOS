/**
 * SDL Platform Emulator Implementation
 */

#include "sdl_platform.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace dialos {
namespace vm {

// Constants for hardware simulation
const uint8_t GPIO_INPUT = 0;
const uint8_t GPIO_OUTPUT = 1;
const uint8_t GPIO_INPUT_PULLUP = 2;

SDLPlatform::SDLPlatform() : 
    window_(nullptr),
    renderer_(nullptr),
    font_(nullptr),
    initialized_(false),
    shouldQuit_(false),
    backgroundColor_(0x000000FF),
    brightness_(255),
    encoder_{false, false, 0, 0, std::chrono::steady_clock::now()},
    touch_{false, false, 0, 0, std::chrono::steady_clock::now()},
    rfid_{false, "", std::chrono::steady_clock::now()},
    buzzer_{false, 0, std::chrono::steady_clock::now()},
    startTime_(std::chrono::steady_clock::now()),
    rtcOffset_(0),
    i2c_{0, {}, {}, 0, {}},
    power_{75, false, std::chrono::steady_clock::now()},
    showDebugInfo_(false) {
}

SDLPlatform::~SDLPlatform() {
    cleanup();
}

bool SDLPlatform::initialize(const std::string& title) {
    if (initialized_) return true;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf initialization failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return false;
    }
    
    // Initialize SDL_mixer for audio
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer initialization failed: " << Mix_GetError() << std::endl;
        // Continue without audio
    }
    
    // Create window
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window_) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }
    
    // Create renderer
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        cleanup();
        return false;
    }
    
    // Try to load a font with multiple fallbacks
    const char* fontPaths[] = {
        "C:/Windows/Fonts/arial.ttf",      // Windows Arial
        "C:/Windows/Fonts/calibri.ttf",    // Windows Calibri
        "C:/Windows/Fonts/consola.ttf",    // Windows Console font
        "C:/Windows/Fonts/cour.ttf",       // Windows Courier
        "/System/Library/Fonts/Arial.ttf", // macOS
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", // Linux
        "arial.ttf",                       // Local file
        NULL
    };
    
    font_ = nullptr;
    for (int i = 0; fontPaths[i] != nullptr; i++) {
        font_ = TTF_OpenFont(fontPaths[i], 14);
        if (font_) {
            std::cout << "Loaded font: " << fontPaths[i] << std::endl;
            break;
        }
    }
    
    if (!font_) {
        std::cout << "Warning: Could not load any font, text rendering will not work" << std::endl;
        std::cout << "Please ensure you have system fonts installed." << std::endl;
    }
    
    initialized_ = true;
    program_output("dialOS SDL Emulator initialized");
    program_output("Hardware simulation active:");
    program_output("- Display: 240x240 circular TFT (scaled 2x)");
    program_output("- Encoder: Mouse wheel rotation + right click button");
    program_output("- Touch: Left click in circular display area");
    program_output("- RFID: Press 'R' to simulate card");
    program_output("- Buzzer: Press 'B' for test beep");
    program_output("- Debug: Press 'D' to toggle debug overlay");
    program_output("- Quit: Press ESC or close window");

    return true;
}

void SDLPlatform::cleanup() {
    if (font_) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
    
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    
    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();
    
    initialized_ = false;
}

bool SDLPlatform::pollEvents() {
    if (!initialized_) return false;
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                shouldQuit_ = true;
                return false;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        shouldQuit_ = true;
                        return false;
                        
                    case SDLK_d:
                        showDebugInfo_ = !showDebugInfo_;
                        program_output(showDebugInfo_ ? "Debug overlay ON" : "Debug overlay OFF");
                        break;
                        
                    case SDLK_r:
                        // Simulate RFID card
                        rfid_.cardPresent = !rfid_.cardPresent;
                        if (rfid_.cardPresent) {
                            // Generate a random UID
                            std::stringstream ss;
                            ss << std::hex << std::setfill('0');
                            for (int i = 0; i < 4; i++) {
                                ss << std::setw(2) << (rand() % 256);
                            }
                            rfid_.cardUID = ss.str();
                            program_output("RFID card detected: " + rfid_.cardUID);
                        } else {
                            program_output("RFID card removed");
                        }
                        break;
                        
                    case SDLK_b:
                        // Simulate buzzer beep
                        buzzer_beep(1000, 200);
                        break;
                }
                break;
                
            case SDL_MOUSEWHEEL:
                // Simulate rotary encoder
                encoder_.position += event.wheel.y;
                encoder_.lastUpdate = std::chrono::steady_clock::now();
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // Left mouse button - touch screen in display area
                    int mouseX = event.button.x / WINDOW_SCALE;
                    int mouseY = event.button.y / WINDOW_SCALE;
                    
                    if (isInCircularDisplay(mouseX, mouseY)) {
                        touch_.pressed = true;
                        touch_.x = mouseX;
                        touch_.y = mouseY;
                        touch_.lastUpdate = std::chrono::steady_clock::now();
                    }
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    // Right mouse button - encoder button
                    encoder_.pressed = true;
                    encoder_.lastUpdate = std::chrono::steady_clock::now();
                }
                break;
                
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    touch_.pressed = false;
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    encoder_.pressed = false;
                }
                break;
                
            case SDL_MOUSEMOTION:
                if (touch_.pressed) {
                    int mouseX = event.motion.x / WINDOW_SCALE;
                    int mouseY = event.motion.y / WINDOW_SCALE;
                    
                    if (isInCircularDisplay(mouseX, mouseY)) {
                        touch_.x = mouseX;
                        touch_.y = mouseY;
                        touch_.lastUpdate = std::chrono::steady_clock::now();
                    }
                }
                break;
        }
    }
    
    updateInputs();
    return true;
}

void SDLPlatform::present() {
    if (!initialized_) return;
    
    // Note: We don't clear here - display_clear() does that explicitly
    // This allows drawing commands to accumulate in the render buffer
    
    // Draw circular mask for display
    drawCircularMask();
    
    // Draw encoder position indicator
    drawEncoderIndicator();
    
    // Draw console area
    renderConsoleArea();
    
    // Draw debug overlay if enabled
    if (showDebugInfo_) {
        debug_drawOverlay();
    }
    
    SDL_RenderPresent(renderer_);
}

void SDLPlatform::display_clear(uint32_t color) {
    if (!initialized_) return;
    
    backgroundColor_ = color;
    Color bgColor(color);
    
    // Clear entire window with black
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    
    // Fill circular display area with specified color
    SDL_SetRenderDrawColor(renderer_, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            if (isInCircularDisplay(x, y)) {
                SDL_Rect rect = {
                    x * WINDOW_SCALE, 
                    y * WINDOW_SCALE, 
                    WINDOW_SCALE, 
                    WINDOW_SCALE
                };
                SDL_RenderFillRect(renderer_, &rect);
            }
        }
    }
}

void SDLPlatform::display_drawText(int x, int y, const std::string& text, 
                                   uint32_t color, int size) {
    if (!initialized_ || !font_) return;
    
    renderText(x, y, text, Color(color), size);
}

void SDLPlatform::display_drawPixel(int x, int y, uint32_t color) {
    if (!initialized_ || !isInCircularDisplay(x, y)) return;
    
    Color pixelColor(color);
    SDL_SetRenderDrawColor(renderer_, pixelColor.r, pixelColor.g, pixelColor.b, pixelColor.a);
    
    SDL_Rect rect = {
        x * WINDOW_SCALE, 
        y * WINDOW_SCALE, 
        WINDOW_SCALE, 
        WINDOW_SCALE
    };
    SDL_RenderFillRect(renderer_, &rect);
}

void SDLPlatform::display_drawLine(int x1, int y1, int x2, int y2, uint32_t color) {
    if (!initialized_) {
        program_output("Error: display_drawLine called before initialization");
        return;
    }
    else{
        program_output("Should draw line from " + std::to_string(x1) + "," + std::to_string(y1) + " to " + std::to_string(x2) + "," + std::to_string(y2));
    }
    
    Color lineColor(color);
    SDL_SetRenderDrawColor(renderer_, lineColor.r, lineColor.g, lineColor.b, lineColor.a);
    
    // Scale coordinates
    x1 *= WINDOW_SCALE; y1 *= WINDOW_SCALE;
    x2 *= WINDOW_SCALE; y2 *= WINDOW_SCALE;
    
    SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
}

void SDLPlatform::display_drawRect(int x, int y, int w, int h, uint32_t color, bool filled) {
    if (!initialized_) return;
    
    Color rectColor(color);
    SDL_SetRenderDrawColor(renderer_, rectColor.r, rectColor.g, rectColor.b, rectColor.a);
    
    SDL_Rect rect = {
        x * WINDOW_SCALE, 
        y * WINDOW_SCALE, 
        w * WINDOW_SCALE, 
        h * WINDOW_SCALE
    };
    
    if (filled) {
        SDL_RenderFillRect(renderer_, &rect);
    } else {
        SDL_RenderDrawRect(renderer_, &rect);
    }
}

void SDLPlatform::display_drawCircle(int x, int y, int radius, uint32_t color, bool filled) {
    if (!initialized_) return;
    
    Color circleColor(color);
    SDL_SetRenderDrawColor(renderer_, circleColor.r, circleColor.g, circleColor.b, circleColor.a);
    
    // Simple circle drawing algorithm
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx*dx + dy*dy) <= (radius * radius)) {
                int px = x + dx;
                int py = y + dy;
                
                if (filled || (dx*dx + dy*dy) >= ((radius-1) * (radius-1))) {
                    display_drawPixel(px, py, color);
                }
            }
        }
    }
}

void SDLPlatform::display_setBrightness(int brightness) {
    brightness_ = static_cast<uint8_t>(std::max(0, std::min(255, brightness)));
    // In a real implementation, this would affect the backlight
    // Here we could modify the alpha channel of all rendering
}

int SDLPlatform::display_getWidth() {
    return DISPLAY_WIDTH;
}

int SDLPlatform::display_getHeight() {
    return DISPLAY_HEIGHT;
}

// === Encoder Operations ===

bool SDLPlatform::encoder_getButton() {
    return encoder_.pressed;
}

int SDLPlatform::encoder_getDelta() {
    int delta = encoder_.position - encoder_.lastPosition;
    encoder_.lastPosition = encoder_.position;
    return delta;
}

int SDLPlatform::encoder_getPosition() {
    return encoder_.position;
}

void SDLPlatform::encoder_reset() {
    encoder_.position = 0;
    encoder_.lastPosition = 0;
}

// === Touch Operations ===

int SDLPlatform::touch_getX() {
    return touch_.x;
}

int SDLPlatform::touch_getY() {
    return touch_.y;
}

bool SDLPlatform::touch_isPressed() {
    return touch_.pressed;
}

void SDLPlatform::touch_getPosition(int& x, int& y) {
    x = touch_.x;
    y = touch_.y;
}

bool SDLPlatform::touch_isInDisplay(int x, int y) {
    return isInCircularDisplay(x, y);
}

// === RFID Operations ===

std::string SDLPlatform::rfid_read() {
    return rfid_.cardPresent ? rfid_.cardUID : "";
}

bool SDLPlatform::rfid_isPresent() {
    return rfid_.cardPresent;
}

// === File Operations (Stubs) ===

int SDLPlatform::file_open(const std::string& path, const std::string& mode) {
    // TODO: Implement file system simulation
    program_output("File open: " + path + " mode: " + mode);
    return -1; // Not implemented
}

std::string SDLPlatform::file_read(int handle, int size) {
    return ""; // Not implemented
}

int SDLPlatform::file_write(int handle, const std::string& data) {
    return -1; // Not implemented
}

void SDLPlatform::file_close(int handle) {
    // Not implemented
}

bool SDLPlatform::file_exists(const std::string& path) {
    return false; // Not implemented
}

bool SDLPlatform::file_delete(const std::string& path) {
    return false; // Not implemented
}

int SDLPlatform::file_size(const std::string& path) {
    return -1; // Not implemented
}

// === GPIO Operations ===

void SDLPlatform::gpio_pinMode(int pin, int mode) {
    gpioPins_[pin].mode = static_cast<uint8_t>(mode);
    program_output("GPIO" + std::to_string(pin) + " mode: " + std::to_string(mode));
}

void SDLPlatform::gpio_digitalWrite(int pin, int value) {
    if (gpioPins_[pin].mode == GPIO_OUTPUT) {
        gpioPins_[pin].digitalValue = (value != 0);
        program_output("GPIO" + std::to_string(pin) + " write: " + (value ? "HIGH" : "LOW"));
    }
}

int SDLPlatform::gpio_digitalRead(int pin) {
    return gpioPins_[pin].digitalValue ? 1 : 0;
}

void SDLPlatform::gpio_analogWrite(int pin, int value) {
    if (gpioPins_[pin].mode == GPIO_OUTPUT) {
        gpioPins_[pin].analogValue = static_cast<uint16_t>(value);
        program_output("GPIO" + std::to_string(pin) + " PWM: " + std::to_string(value));
    }
}

int SDLPlatform::gpio_analogRead(int pin) {
    return static_cast<int>(gpioPins_[pin].analogValue);
}

// === I2C Operations (Stubs) ===

std::vector<int> SDLPlatform::i2c_scan() {
    program_output("I2C scan");
    return {}; // Not implemented
}

bool SDLPlatform::i2c_write(int address, const std::vector<uint8_t>& data) {
    program_output("I2C write to 0x" + std::to_string(address));
    return false; // Not implemented
}

std::vector<uint8_t> SDLPlatform::i2c_read(int address, int length) {
    program_output("I2C read from 0x" + std::to_string(address));
    return {}; // Not implemented
}

// === Buzzer Operations ===

void SDLPlatform::buzzer_beep(int frequency, int duration) {
    buzzer_.isPlaying = true;
    buzzer_.frequency = static_cast<uint32_t>(frequency);
    buzzer_.endTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration);
    
    // Simple audio simulation - just log for now
    // In a full implementation, you'd generate audio waveforms
    program_output("Buzzer: " + std::to_string(frequency) + "Hz for " + std::to_string(duration) + "ms");
}

void SDLPlatform::buzzer_stop() {
    buzzer_.isPlaying = false;
}

// === Timer Operations (Stubs) ===

int SDLPlatform::timer_setTimeout(int ms) {
    // TODO: Implement timer system
    program_output("setTimeout: " + std::to_string(ms) + "ms");
    return -1; // Not implemented
}

int SDLPlatform::timer_setInterval(int ms) {
    program_output("setInterval: " + std::to_string(ms) + "ms");
    return -1; // Not implemented
}

void SDLPlatform::timer_clear(int id) {
    program_output("clearTimer: " + std::to_string(id));
    // Not implemented
}

// === Memory Operations (Stubs) ===

int SDLPlatform::memory_getAvailable() {
    return 0; // Not implemented - would need to track VM heap
}

int SDLPlatform::memory_getUsage() {
    return 0; // Not implemented - would need to track VM allocations
}

// === System Operations ===

uint32_t SDLPlatform::system_getTime() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_);
    return static_cast<uint32_t>(duration.count());
}

void SDLPlatform::system_sleep(uint32_t ms) {
    SDL_Delay(ms);
}

uint32_t SDLPlatform::system_getRTC() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    return static_cast<uint32_t>(timestamp.count()) + static_cast<uint32_t>(rtcOffset_);
}

void SDLPlatform::system_setRTC(uint32_t timestamp) {
    auto now = std::chrono::system_clock::now();
    auto current = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    rtcOffset_ = static_cast<int64_t>(timestamp) - static_cast<int64_t>(current.count());
}

// === Console Operations ===

void SDLPlatform::console_log(const std::string& msg) {
    std::cout << msg << std::endl;
    outputLog_.addLine(msg);
}

void SDLPlatform::console_warn(const std::string& msg) {
    std::cout << "[WARN] " << msg << std::endl;
    outputLog_.addLine("[WARN] " + msg);
}

void SDLPlatform::console_error(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
    outputLog_.addLine("[ERROR] " + msg);
}

void SDLPlatform::program_output(const std::string& msg) {
    std::cout << "[dialOS] " << msg << std::endl;
    consoleLog_.addLine("[dialOS] " + msg);
    addDebugMessage(msg);
}

// Private helper methods

bool SDLPlatform::isInCircularDisplay(int x, int y) const {
    int dx = x - CENTER_X;
    int dy = y - CENTER_Y;
    return (dx * dx + dy * dy) <= (DISPLAY_RADIUS * DISPLAY_RADIUS);
}

void SDLPlatform::drawCircularMask() {
    if (!initialized_) return;
    
    // Draw border around circular display (gray ring at the edge)
    SDL_SetRenderDrawColor(renderer_, 64, 64, 64, 255);
    
    // Border: 2-pixel ring just inside the display radius
    // This keeps it fully visible within the display bounds
    int outerRadius = DISPLAY_RADIUS;      // 120 - edge of circle
    int innerRadius = DISPLAY_RADIUS - 2;  // 118 - 2 pixels inward
    
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int dx = x - CENTER_X;
            int dy = y - CENTER_Y;
            int distSq = dx*dx + dy*dy;
            
            // Draw pixels in the border ring (between inner and outer radius)
            if (distSq >= (innerRadius * innerRadius) && 
                distSq <= (outerRadius * outerRadius)) {
                
                SDL_Rect rect = {
                    x * WINDOW_SCALE, 
                    y * WINDOW_SCALE, 
                    WINDOW_SCALE, 
                    WINDOW_SCALE
                };
                SDL_RenderFillRect(renderer_, &rect);
            }
        }
    }
}

void SDLPlatform::drawEncoderIndicator() {
    if (!initialized_) return;
    
    // Calculate angle from encoder position
    // M5 Dial has 64 pulses per revolution, so normalize to 0-360 degrees
    const int PULSES_PER_REV = 64;
    float angle = (encoder_.position % PULSES_PER_REV) * (2.0f * 3.14159f / PULSES_PER_REV);
    
    // Draw a small arc section of the border in orange to indicate position
    // Orange color (RGB565: 0xFD20 -> RGB888: 255, 165, 0)
    SDL_SetRenderDrawColor(renderer_, 255, 165, 0, 255);
    
    // Use same radius range as the border
    int outerRadius = DISPLAY_RADIUS;      // 120 - edge of circle
    int innerRadius = DISPLAY_RADIUS - 2;  // 118 - 2 pixels inward
    
    // Draw a small arc (about 5 degrees on each side of the position)
    const float arcSpan = 0.08f;  // radians (~5 degrees)
    
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int dx = x - CENTER_X;
            int dy = y - CENTER_Y;
            int distSq = dx*dx + dy*dy;
            
            // Check if pixel is in the border ring
            if (distSq >= (innerRadius * innerRadius) && 
                distSq <= (outerRadius * outerRadius)) {
                
                // Calculate angle of this pixel
                float pixelAngle = atan2((float)dy, (float)dx);
                
                // Normalize angle difference to -PI to PI range
                float angleDiff = pixelAngle - angle;
                while (angleDiff > 3.14159f) angleDiff -= 2.0f * 3.14159f;
                while (angleDiff < -3.14159f) angleDiff += 2.0f * 3.14159f;
                
                // Draw if within arc span
                if (fabs(angleDiff) <= arcSpan) {
                    SDL_Rect rect = {
                        x * WINDOW_SCALE, 
                        y * WINDOW_SCALE, 
                        WINDOW_SCALE, 
                        WINDOW_SCALE
                    };
                    SDL_RenderFillRect(renderer_, &rect);
                }
            }
        }
    }
}

void SDLPlatform::updateInputs() {
    // Update buzzer state
    if (buzzer_.isPlaying && std::chrono::steady_clock::now() > buzzer_.endTime) {
        buzzer_.isPlaying = false;
    }
    
    // Update power simulation (slowly drain battery when not charging)
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - power_.lastUpdate);
    
    if (duration.count() > 60 && !power_.charging) { // Update every minute
        if (power_.batteryLevel > 0) {
            power_.batteryLevel = std::max(0, static_cast<int>(power_.batteryLevel) - 1);
        }
        power_.lastUpdate = now;
    }
}

void SDLPlatform::renderText(int x, int y, const std::string& text, const Color& color, int size) {
    if (!font_) return;
    
    SDL_Surface* surface = TTF_RenderText_Solid(font_, text.c_str(), color.toSDL());
    if (!surface) return;
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }
    
    // Use actual coordinates without scaling for console text
    SDL_Rect destRect = {
        x,
        y,
        surface->w,
        surface->h
    };
    
    SDL_RenderCopy(renderer_, texture, nullptr, &destRect);
    
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void SDLPlatform::addDebugMessage(const std::string& msg) {
    debugMessages_.push_back(msg);
    
    // Keep only last 20 messages
    if (debugMessages_.size() > 20) {
        debugMessages_.erase(debugMessages_.begin());
    }
}

void SDLPlatform::debug_drawOverlay() {
    if (!initialized_ || !font_) return;
    
    // Draw semi-transparent background for debug info
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 128);
    
    SDL_Rect overlayRect = {0, 0, WINDOW_WIDTH, 200};
    SDL_RenderFillRect(renderer_, &overlayRect);
    
    // Draw debug information
    int yPos = 10;
    Color textColor(255, 255, 255, 255);
    
    // System info
    renderText(10, yPos, "=== dialOS Debug Info ===", textColor, 8);
    yPos += 20;
    
    renderText(10, yPos, "Time: " + std::to_string(system_getTime()) + "ms", textColor, 6);
    yPos += 15;
    
    renderText(10, yPos, "Encoder: pos=" + std::to_string(encoder_.position) + 
               " btn=" + std::string(encoder_.pressed ? "ON" : "OFF"), textColor, 6);
    yPos += 15;
    
    renderText(10, yPos, "Touch: " + std::string(touch_.pressed ? "ON" : "OFF") + 
               " (" + std::to_string(touch_.x) + "," + std::to_string(touch_.y) + ")", textColor, 6);
    yPos += 15;
    
    renderText(10, yPos, "RFID: " + std::string(rfid_.cardPresent ? "CARD(" + rfid_.cardUID + ")" : "NONE"), textColor, 6);
    yPos += 15;
    
    renderText(10, yPos, "Battery: " + std::to_string(power_.batteryLevel) + "% " + 
               std::string(power_.charging ? "CHARGING" : ""), textColor, 6);
    yPos += 15;
    
    renderText(10, yPos, "Buzzer: " + std::string(buzzer_.isPlaying ? "ACTIVE" : "IDLE") + 
               " " + std::to_string(buzzer_.frequency) + "Hz", textColor, 6);
    
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
}

// Additional peripheral simulation methods (not part of PlatformInterface)

uint8_t SDLPlatform::power_getBatteryLevel() {
    return power_.batteryLevel;
}

bool SDLPlatform::power_isCharging() {
    return power_.charging;
}

void SDLPlatform::power_sleep() {
    program_output("System entering sleep mode...");
    // In a real implementation, this would put the system into low-power mode
}

void SDLPlatform::renderConsoleArea() {
    if (!initialized_) return;
    
    // Get actual current window size
    int windowWidth, windowHeight;
    SDL_GetWindowSize(window_, &windowWidth, &windowHeight);
    
    // Console area starts to the right of the display
    int consoleX = DISPLAY_SCALED_WIDTH;
    int consoleY = 0;
    int consoleWidth = windowWidth - DISPLAY_SCALED_WIDTH;
    int consoleHeight = windowHeight;
    
    // Ensure console has minimum width
    if (consoleWidth < 200) {
        consoleWidth = 200;
    }
    
    // Fill console background
    SDL_SetRenderDrawColor(renderer_, 30, 30, 30, 255); // Dark gray
    SDL_Rect consoleRect = {consoleX, consoleY, consoleWidth, consoleHeight};
    SDL_RenderFillRect(renderer_, &consoleRect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer_, 100, 100, 100, 255); // Light gray
    SDL_RenderDrawRect(renderer_, &consoleRect);
    
    // Split console area into two sections
    int halfHeight = consoleHeight / 2;
    
    // Render console log (top half)
    renderLogWindow(consoleX + 5, consoleY + 5, consoleWidth - 10, halfHeight - 10, 
                   "Console Log", consoleLog_);
    
    // Draw separator line
    SDL_SetRenderDrawColor(renderer_, 100, 100, 100, 255);
    SDL_RenderDrawLine(renderer_, consoleX + 5, consoleY + halfHeight, 
                      consoleX + consoleWidth - 5, consoleY + halfHeight);
    
    // Render output log (bottom half)
    renderLogWindow(consoleX + 5, consoleY + halfHeight + 5, consoleWidth - 10, halfHeight - 10,
                   "Program Output", outputLog_);
}

void SDLPlatform::renderLogWindow(int x, int y, int width, int height, 
                                 const std::string& title, const ConsoleLog& log) {
    if (!font_) return;
    
    // Render title
    Color titleColor(200, 200, 255); // Light blue
    renderText(x, y, title, titleColor, 12);
    
    // Calculate text area
    int textY = y + 20;
    int textHeight = height - 25;
    int lineHeight = 14;
    int maxLines = textHeight / lineHeight;
    
    // Render log lines (most recent at bottom)
    Color textColor(220, 220, 220); // Light gray
    int startLine = std::max(0, (int)log.lines.size() - maxLines);
    
    for (size_t i = startLine; i < log.lines.size() && i < startLine + maxLines; i++) {
        int lineY = textY + (i - startLine) * lineHeight;
        
        // Truncate long lines to fit width
        std::string line = log.lines[i];
        if (line.length() > 35) { // Rough character limit for console width
            line = line.substr(0, 32) + "...";
        }
        
        renderText(x + 2, lineY, line, textColor, 10);
    }
    
    // Show scroll indicator if there are more lines
    if (log.lines.size() > maxLines) {
        Color scrollColor(150, 150, 150);
        renderText(x + width - 20, y, "^", scrollColor, 10);
    }
}

} // namespace vm
} // namespace dialos