/**
 * SDL Platform Emulator Implementation
 */

#include "sdl_platform.h"
#include "vm/vm_core.h"
#include "vm/vm_value.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#include <codecvt>
#include <locale>
#undef min
#undef max
#endif

namespace dialos {
namespace vm {

// Constants for hardware simulation
const uint8_t GPIO_INPUT = 0;
const uint8_t GPIO_OUTPUT = 1;
const uint8_t GPIO_INPUT_PULLUP = 2;

SDLPlatform::SDLPlatform()
    : window_(nullptr), renderer_(nullptr), font_(nullptr), fontPath_(""), initialized_(false),
      shouldQuit_(false), backgroundColor_(0x000000FF), brightness_(255),
      encoder_{false, false, 0, 0, std::chrono::steady_clock::now()},
      touch_{false, false, 0, 0, std::chrono::steady_clock::now()},
      rfid_{false, "", std::chrono::steady_clock::now()},
      buzzer_{false, 0, std::chrono::steady_clock::now()},
      startTime_(std::chrono::steady_clock::now()), rtcOffset_(0),
      i2c_{0, {}, {}, 0, {}},
      power_{75, false, std::chrono::steady_clock::now()}, consoleLog_(50),
      outputLog_(50) {}

SDLPlatform::~SDLPlatform() { cleanup(); }

bool SDLPlatform::initialize(const std::string &title) {
  if (initialized_)
    return true;

  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
    std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
    return false;
  }

  // Initialize SDL_ttf
  if (TTF_Init() < 0) {
    std::cerr << "SDL_ttf initialization failed: " << TTF_GetError()
              << std::endl;
    SDL_Quit();
    return false;
  }

  // Initialize SDL_mixer for audio
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    std::cerr << "SDL_mixer initialization failed: " << Mix_GetError()
              << std::endl;
    // Continue without audio
  }

  // Create window
  window_ = SDL_CreateWindow(
      title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  if (!window_) {
    std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
    cleanup();
    return false;
  }

  // Create renderer
  renderer_ = SDL_CreateRenderer(
      window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer_) {
    std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
    cleanup();
    return false;
  }

  // Try to load a font with multiple fallbacks
  const char *fontPaths[] = {
      "C:/Windows/Fonts/arial.ttf",                      // Windows Arial
      "C:/Windows/Fonts/calibri.ttf",                    // Windows Calibri
      "C:/Windows/Fonts/consola.ttf",                    // Windows Console font
      "C:/Windows/Fonts/cour.ttf",                       // Windows Courier
      "/System/Library/Fonts/Arial.ttf",                 // macOS
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", // Linux
      "arial.ttf",                                       // Local file
      NULL};

  font_ = nullptr;
  for (int i = 0; fontPaths[i] != nullptr; i++) {
    font_ = TTF_OpenFont(fontPaths[i], 14);
    if (font_) {
      fontPath_ = fontPaths[i]; // Store the font path for dynamic loading
      fontCache_[14] = font_; // Cache the default size font
      std::cout << "Loaded font: " << fontPaths[i] << std::endl;
      break;
    }
  }

  if (!font_) {
    std::cout
        << "Warning: Could not load any font, text rendering will not work"
        << std::endl;
    std::cout << "Please ensure you have system fonts installed." << std::endl;
  }

  initialized_ = true;
  console_log("dialOS SDL Emulator initialized");
  console_log("Hardware simulation active:");
  console_log("- Display: 240x240 circular TFT (scaled 2x)");
  console_log("- Encoder: Mouse wheel rotation + right click button");
  console_log("- Touch: Left click in circular display area");
  console_log("- RFID: Press 'R' to simulate card");
  console_log("- Buzzer: Press 'B' for test beep");
  console_log("- Debug: Press 'D' to toggle debug overlay");
  console_log("- Quit: Press ESC or close window");

  return true;
}

void SDLPlatform::cleanup() {
  // Clean up font cache
  for (auto& fontPair : fontCache_) {
    if (fontPair.second) {
      TTF_CloseFont(fontPair.second);
    }
  }
  fontCache_.clear();
  font_ = nullptr;

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

TTF_Font* SDLPlatform::getFontOfSize(int size) {
  // // Clamp size to reasonable range
  // size = std::max(8, std::min(72, size));
  
  // // Check if we already have this size cached
  // auto it = fontCache_.find(size);
  // if (it != fontCache_.end() && it->second != nullptr) {
  //   return it->second;
  // }
  
  // // Load a new font of the specified size
  // if (!fontPath_.empty()) {
  //   TTF_Font* newFont = TTF_OpenFont(fontPath_.c_str(), size);
  //   if (newFont) {
  //     fontCache_[size] = newFont;
  //     return newFont;
  //   }
  // }
  
  // Fallback to default font if loading failed
  return font_;
}

bool SDLPlatform::pollEvents() {
  if (!initialized_)
    return false;

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
          console_log("RFID card detected: " + rfid_.cardUID);
        } else {
          console_log("RFID card removed");
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
      {
        int oldPosition = encoder_.position;
        encoder_.position += event.wheel.y;
        encoder_.lastUpdate = std::chrono::steady_clock::now();

        // Invoke encoder.onTurn callback if registered
        int delta = encoder_.position - oldPosition;
        if (delta != 0) {
          std::vector<Value> args = {Value::Int32(delta)};
          invokeCallback("encoder.onTurn", args);
        }
      }
      break;

    case SDL_MOUSEBUTTONDOWN:
      if (event.button.button == SDL_BUTTON_LEFT) {
        // Left mouse button - touch screen in display area
        int mouseX = (event.button.x - DEBUG_PANEL_WIDTH) / WINDOW_SCALE;
        int mouseY = event.button.y / WINDOW_SCALE;

        if (isInCircularDisplay(mouseX, mouseY)) {
          touch_.pressed = true;
          touch_.x = mouseX;
          touch_.y = mouseY;
          touch_.lastUpdate = std::chrono::steady_clock::now();

          // Invoke touch.onPress callback
          std::vector<Value> args = {Value::Int32(mouseX),
                                     Value::Int32(mouseY)};
          invokeCallback("touch.onPress", args);
        }
      } else if (event.button.button == SDL_BUTTON_RIGHT) {
        // Right mouse button - encoder button
        encoder_.pressed = true;
        encoder_.lastUpdate = std::chrono::steady_clock::now();

        // Invoke encoder.onButton callback
        std::vector<Value> args = {Value::Bool(true)};
        invokeCallback("encoder.onButton", args);
      }
      break;

    case SDL_MOUSEBUTTONUP:
      if (event.button.button == SDL_BUTTON_LEFT) {
        int mouseX = (event.button.x - DEBUG_PANEL_WIDTH) / WINDOW_SCALE;
        int mouseY = event.button.y / WINDOW_SCALE;

        touch_.pressed = false;

        // Invoke touch.onRelease callback if in display
        if (isInCircularDisplay(mouseX, mouseY)) {
          std::vector<Value> args = {Value::Int32(mouseX),
                                     Value::Int32(mouseY)};
          invokeCallback("touch.onRelease", args);
        }
      } else if (event.button.button == SDL_BUTTON_RIGHT) {
        encoder_.pressed = false;

        // Invoke encoder.onButton callback for release
        std::vector<Value> args = {Value::Bool(false)};
        invokeCallback("encoder.onButton", args);
      }
      break;

    case SDL_MOUSEMOTION:
      if (touch_.pressed) {
        int mouseX = (event.motion.x - DEBUG_PANEL_WIDTH) / WINDOW_SCALE;
        int mouseY = event.motion.y / WINDOW_SCALE;

        if (isInCircularDisplay(mouseX, mouseY)) {
          touch_.x = mouseX;
          touch_.y = mouseY;
          touch_.lastUpdate = std::chrono::steady_clock::now();

          // Invoke touch.onDrag callback
          std::vector<Value> args = {Value::Int32(mouseX),
                                     Value::Int32(mouseY)};
          invokeCallback("touch.onDrag", args);
        }
      }
      break;
    }
  }

  // Process timers once per poll cycle (after all events handled)
  processTimers();

  // SDLPlatform does not trigger lifecycle callbacks itself; the host
  // (test harness or main runtime) should invoke app.onLoad when the
  // VM is ready. SDLPlatform only delivers input/timer events.

  updateInputs();
  return true;
}

void SDLPlatform::present() {
  if (!initialized_)
    return;

  // Note: We don't clear here - display_clear() does that explicitly
  // This allows drawing commands to accumulate in the render buffer

  // Draw debug panel
  renderDebugPanel();

  // Draw circular mask for display
  drawCircularMask();

  // Draw encoder position indicator
  drawEncoderIndicator();

  // Draw console area
  renderConsoleArea();

  SDL_RenderPresent(renderer_);
}

void SDLPlatform::display_clear(uint32_t color) {
  if (!initialized_)
    return;

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
        SDL_Rect rect = {DEBUG_PANEL_WIDTH + x * WINDOW_SCALE, y * WINDOW_SCALE,
                         WINDOW_SCALE, WINDOW_SCALE};
        SDL_RenderFillRect(renderer_, &rect);
      }
    }
  }
}

void SDLPlatform::display_drawText(int x, int y, const std::string &text,
                                   uint32_t color, int size) {
  if (!initialized_ || !font_)
    return;

  renderText(DEBUG_PANEL_WIDTH + x * WINDOW_SCALE, y * WINDOW_SCALE, text, Color(color), size);
}

void SDLPlatform::display_drawPixel(int x, int y, uint32_t color) {
  if (!initialized_ || !isInCircularDisplay(x, y))
    return;

  Color pixelColor(color);
  SDL_SetRenderDrawColor(renderer_, pixelColor.r, pixelColor.g, pixelColor.b,
                         pixelColor.a);

  SDL_Rect rect = {DEBUG_PANEL_WIDTH + x * WINDOW_SCALE, y * WINDOW_SCALE,
                   WINDOW_SCALE, WINDOW_SCALE};
  SDL_RenderFillRect(renderer_, &rect);
}

void SDLPlatform::display_drawLine(int x1, int y1, int x2, int y2,
                                   uint32_t color) {
  if (!initialized_) {
    console_error("Error: display_drawLine called before initialization");
    return;
  }

  Color lineColor(color);
  SDL_SetRenderDrawColor(renderer_, lineColor.r, lineColor.g, lineColor.b,
                         lineColor.a);

  // Scale coordinates and offset
  x1 = DEBUG_PANEL_WIDTH + x1 * WINDOW_SCALE;
  y1 = y1 * WINDOW_SCALE;
  x2 = DEBUG_PANEL_WIDTH + x2 * WINDOW_SCALE;
  y2 = y2 * WINDOW_SCALE;

  SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
}

void SDLPlatform::display_drawRect(int x, int y, int w, int h, uint32_t color,
                                   bool filled) {
  if (!initialized_)
    return;

  Color rectColor(color);
  SDL_SetRenderDrawColor(renderer_, rectColor.r, rectColor.g, rectColor.b,
                         rectColor.a);

  SDL_Rect rect = {DEBUG_PANEL_WIDTH + x * WINDOW_SCALE, y * WINDOW_SCALE,
                   w * WINDOW_SCALE, h * WINDOW_SCALE};

  if (filled) {
    SDL_RenderFillRect(renderer_, &rect);
  } else {
    SDL_RenderDrawRect(renderer_, &rect);
  }
}

void SDLPlatform::display_drawCircle(int x, int y, int radius, uint32_t color,
                                     bool filled) {
  if (!initialized_)
    return;

  Color circleColor(color);
  SDL_SetRenderDrawColor(renderer_, circleColor.r, circleColor.g, circleColor.b,
                         circleColor.a);

  if (filled) {
    // Filled circle using midpoint circle algorithm
    int cx = 0;
    int cy = radius;
    int d = 1 - radius;

    auto drawHorizontalLine = [&](int x1, int x2, int scanY) {
      for (int i = x1; i <= x2; i++) {
        display_drawPixel(i, scanY, color);
      }
    };

    // Draw initial horizontal lines
    drawHorizontalLine(x - cy, x + cy, y); // Center horizontal line

    while (cx < cy) {
      if (d < 0) {
        d += 2 * cx + 3;
      } else {
        d += 2 * (cx - cy) + 5;
        cy--;
      }
      cx++;

      // Draw horizontal lines at each y level (4-way symmetry)
      drawHorizontalLine(x - cx, x + cx, y + cy); // Bottom
      drawHorizontalLine(x - cx, x + cx, y - cy); // Top
      drawHorizontalLine(x - cy, x + cy, y + cx); // Lower middle
      drawHorizontalLine(x - cy, x + cy, y - cx); // Upper middle
    }
  } else {
    // Outline circle using Bresenham's circle algorithm
    int cx = 0;
    int cy = radius;
    int d = 3 - 2 * radius;

    auto drawCirclePoints = [&](int cx, int cy) {
      display_drawPixel(x + cx, y + cy, color);
      display_drawPixel(x - cx, y + cy, color);
      display_drawPixel(x + cx, y - cy, color);
      display_drawPixel(x - cx, y - cy, color);
      display_drawPixel(x + cy, y + cx, color);
      display_drawPixel(x - cy, y + cx, color);
      display_drawPixel(x + cy, y - cx, color);
      display_drawPixel(x - cy, y - cx, color);
    };

    drawCirclePoints(cx, cy);

    while (cy >= cx) {
      cx++;
      if (d > 0) {
        cy--;
        d = d + 4 * (cx - cy) + 10;
      } else {
        d = d + 4 * cx + 6;
      }
      drawCirclePoints(cx, cy);
    }
  }
}

void SDLPlatform::display_setBrightness(int brightness) {
  brightness_ = static_cast<uint8_t>(std::max(0, std::min(255, brightness)));
  // In a real implementation, this would affect the backlight
  // Here we could modify the alpha channel of all rendering
}

int SDLPlatform::display_getWidth() { return DISPLAY_WIDTH; }

int SDLPlatform::display_getHeight() { return DISPLAY_HEIGHT; }

// === Encoder Operations ===

bool SDLPlatform::encoder_getButton() { return encoder_.pressed; }

int SDLPlatform::encoder_getDelta() {
  int delta = encoder_.position - encoder_.lastPosition;
  encoder_.lastPosition = encoder_.position;
  return delta;
}

int SDLPlatform::encoder_getPosition() { return encoder_.position; }

void SDLPlatform::encoder_reset() {
  encoder_.position = 0;
  encoder_.lastPosition = 0;
}

// === Touch Operations ===

int SDLPlatform::touch_getX() { return touch_.x; }

int SDLPlatform::touch_getY() { return touch_.y; }

bool SDLPlatform::touch_isPressed() { return touch_.pressed; }

void SDLPlatform::touch_getPosition(int &x, int &y) {
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

bool SDLPlatform::rfid_isPresent() { return rfid_.cardPresent; }

// === File Operations ===

int SDLPlatform::file_open(const std::string &path, const std::string &mode) {
  console_log("File open: " + path + " mode: " + mode);

  // Create filesystem root if it doesn't exist
  std::filesystem::create_directories(fileSystemRoot_);

  // Convert dialOS path to full filesystem path
  std::string fullPath = fileSystemRoot_ + path;

  // Create parent directories if they don't exist
  std::filesystem::path filePath(fullPath);
  if (filePath.has_parent_path()) {
    std::filesystem::create_directories(filePath.parent_path());
  }

  // Convert mode string to appropriate fstream openmode
  std::ios_base::openmode openMode;
  if (mode == "r" || mode == "read") {
    openMode = std::ios_base::in;
  } else if (mode == "w" || mode == "write") {
    openMode = std::ios_base::out | std::ios_base::trunc;
  } else if (mode == "a" || mode == "append") {
    openMode = std::ios_base::out | std::ios_base::app;
  } else {
    console_error("Invalid file mode: " + mode);
    return -1;
  }

  // Allocate a file handle
  int handle = nextFileHandle_++;
  FileHandle &fh = fileHandles_[handle];

  // Open the file
  fh.stream.open(fullPath, openMode);
  if (!fh.stream.is_open()) {
    console_error("Failed to open file: " + fullPath);
    fileHandles_.erase(handle);
    return -1;
  }

  fh.path = fullPath;
  fh.mode = mode;
  fh.isOpen = true;

  console_log("File opened successfully, handle: " + std::to_string(handle));
  return handle;
}

std::string SDLPlatform::file_read(int handle, int size) {
  auto it = fileHandles_.find(handle);
  if (it == fileHandles_.end() || !it->second.isOpen) {
    console_error("Invalid file handle: " + std::to_string(handle));
    return "";
  }

  FileHandle &fh = it->second;
  if (fh.mode != "r" && fh.mode != "read") {
    console_error("File not opened for reading");
    return "";
  }

  std::vector<char> buffer(size);
  fh.stream.read(buffer.data(), size);

  std::streamsize bytesRead = fh.stream.gcount();
  std::string result(buffer.data(), bytesRead);

  console_log("File read: " + std::to_string(bytesRead) + " bytes");
  return result;
}

int SDLPlatform::file_write(int handle, const std::string &data) {
  auto it = fileHandles_.find(handle);
  if (it == fileHandles_.end() || !it->second.isOpen) {
    console_error("Invalid file handle: " + std::to_string(handle));
    return -1;
  }

  FileHandle &fh = it->second;
  if (fh.mode != "w" && fh.mode != "write" && fh.mode != "a" &&
      fh.mode != "append") {
    console_error("File not opened for writing");
    return -1;
  }

  fh.stream.write(data.c_str(), data.length());
  fh.stream.flush(); // Ensure data is written immediately

  if (fh.stream.fail()) {
    console_error("Failed to write to file");
    return -1;
  }

  console_log("File write: " + std::to_string(data.length()) + " bytes");
  return static_cast<int>(data.length());
}

void SDLPlatform::file_close(int handle) {
  auto it = fileHandles_.find(handle);
  if (it == fileHandles_.end()) {
    console_warn("Attempt to close invalid file handle: " +
                 std::to_string(handle));
    return;
  }

  FileHandle &fh = it->second;
  if (fh.isOpen) {
    fh.stream.close();
    fh.isOpen = false;
    console_log("File closed: " + fh.path);
  }

  fileHandles_.erase(it);
}

bool SDLPlatform::file_exists(const std::string &path) {
  std::string fullPath = fileSystemRoot_ + path;
  bool exists = std::filesystem::exists(fullPath);
  console_log("File exists check: " + path + " -> " +
              (exists ? "true" : "false"));
  return exists;
}

bool SDLPlatform::file_delete(const std::string &path) {
  std::string fullPath = fileSystemRoot_ + path;

  if (!std::filesystem::exists(fullPath)) {
    console_warn("Cannot delete non-existent file: " + path);
    return false;
  }

  try {
    bool deleted = std::filesystem::remove(fullPath);
    console_log("File delete: " + path + " -> " +
                (deleted ? "success" : "failed"));
    return deleted;
  } catch (const std::exception &e) {
    console_error("Error deleting file " + path + ": " + e.what());
    return false;
  }
}

int SDLPlatform::file_size(const std::string &path) {
  std::string fullPath = fileSystemRoot_ + path;

  if (!std::filesystem::exists(fullPath)) {
    console_warn("Cannot get size of non-existent file: " + path);
    return -1;
  }

  try {
    std::uintmax_t size = std::filesystem::file_size(fullPath);
    console_log("File size: " + path + " -> " + std::to_string(size) +
                " bytes");
    return static_cast<int>(size);
  } catch (const std::exception &e) {
    console_error("Error getting file size " + path + ": " + e.what());
    return -1;
  }
}

// === GPIO Operations ===

void SDLPlatform::gpio_pinMode(int pin, int mode) {
  gpioPins_[static_cast<uint8_t>(pin)].mode = static_cast<uint8_t>(mode);
  console_log("GPIO" + std::to_string(pin) + " mode: " + std::to_string(mode));
}

void SDLPlatform::gpio_digitalWrite(int pin, int value) {
  uint8_t pinKey = static_cast<uint8_t>(pin);
  if (gpioPins_[pinKey].mode == GPIO_OUTPUT) {
    gpioPins_[pinKey].digitalValue = (value != 0);
    console_log("GPIO" + std::to_string(pin) +
                " write: " + (value ? "HIGH" : "LOW"));
  }
}

int SDLPlatform::gpio_digitalRead(int pin) {
  return gpioPins_[static_cast<uint8_t>(pin)].digitalValue ? 1 : 0;
}

void SDLPlatform::gpio_analogWrite(int pin, int value) {
  uint8_t pinKey = static_cast<uint8_t>(pin);
  if (gpioPins_[pinKey].mode == GPIO_OUTPUT) {
    gpioPins_[pinKey].analogValue = static_cast<uint16_t>(value);
    console_log("GPIO" + std::to_string(pin) +
                " PWM: " + std::to_string(value));
  }
}

int SDLPlatform::gpio_analogRead(int pin) {
  return static_cast<int>(gpioPins_[static_cast<uint8_t>(pin)].analogValue);
}

// === I2C Operations (Stubs) ===

std::vector<int> SDLPlatform::i2c_scan() {
  console_log("I2C scan");
  console_warn("NOT IMPLEMENTED: os.i2c.scan - I2C simulation missing");
  return {}; // Not implemented
}

bool SDLPlatform::i2c_write(int address,
                            const std::vector<uint8_t> & /*data*/) {
  console_log("I2C write to 0x" + std::to_string(address));
  console_warn("NOT IMPLEMENTED: os.i2c.write - I2C simulation missing");
  return false; // Not implemented
}

std::vector<uint8_t> SDLPlatform::i2c_read(int address, int /*length*/) {
  console_log("I2C read from 0x" + std::to_string(address));
  console_warn("NOT IMPLEMENTED: os.i2c.read - I2C simulation missing");
  return {}; // Not implemented
}

// === Buzzer Operations ===

void SDLPlatform::buzzer_beep(int frequency, int duration) {
  buzzer_.isPlaying = true;
  buzzer_.frequency = static_cast<uint32_t>(frequency);
  buzzer_.endTime =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(duration);

  // Simple audio simulation - just log for now
  // In a full implementation, you'd generate audio waveforms
  console_log("Buzzer: " + std::to_string(frequency) + "Hz for " +
              std::to_string(duration) + "ms");
}

void SDLPlatform::buzzer_stop() { buzzer_.isPlaying = false; }

// === Timer Operations (Stubs) ===

int SDLPlatform::timer_setTimeout(int ms) {
  // TODO: Implement timer system
  console_log("setTimeout: " + std::to_string(ms) + "ms");
  console_warn("NOT IMPLEMENTED: os.timer.setTimeout - timer system missing");
  return -1; // Not implemented
}

// New: register interval callback
int SDLPlatform::timer_setInterval(const Value &callback, int ms) {
  // console_log("[DEBUG] setInterval: " + std::to_string(ms) + "ms");

  if (!callback.isFunction()) {
    console_warn("timer_setInterval called with non-function callback");
    return -1;
  }

  // console_log("[DEBUG] Callback is function: " + callback.toString());

  TimerEntry entry;
  entry.id = nextTimerId_++;
  entry.callback = callback; // copy Value
  entry.intervalMs = std::max(1, ms);
  entry.nextFire = std::chrono::steady_clock::now() +
                   std::chrono::milliseconds(entry.intervalMs);
  entry.repeating = true;

  timers_[entry.id] = entry;

  // console_log("[DEBUG] Registered timer id=" + std::to_string(entry.id) + "
  // with " + std::to_string(timers_.size()) + " total timers");
  return entry.id;
}

void SDLPlatform::timer_clearTimeout(int id) {
  console_log("clearTimeout: " + std::to_string(id));
  console_warn("NOT IMPLEMENTED: os.timer.clearTimeout - timer system missing");
  // Not implemented
}

void SDLPlatform::timer_clearInterval(int id) {
  console_log("clearInterval: " + std::to_string(id));
  auto it = timers_.find(id);
  if (it != timers_.end()) {
    timers_.erase(it);
    console_log("Cleared timer id=" + std::to_string(id));
  } else {
    console_warn("clearInterval: timer id not found: " + std::to_string(id));
  }
}

void SDLPlatform::processTimers() {
  if (timers_.empty()) {
    // console_log("[DEBUG] No active timers to process");
    return;
  }

  // console_log("[DEBUG] Processing " + std::to_string(timers_.size()) + "
  // timers");

  auto now = std::chrono::steady_clock::now();
  std::vector<int> firedIds;

  for (auto &kv : timers_) {
    TimerEntry &entry = kv.second;
    // console_log(std::string("[DEBUG] Timer ") + std::to_string(entry.id) + "
    // check: now >= nextFire = " +
    //            (now >= entry.nextFire ? "true" : "false"));

    if (now >= entry.nextFire) {
      // console_log("[DEBUG] Timer " + std::to_string(entry.id) + " firing!");

      // Fire the callback synchronously via VM
      // Build empty args vector (timers don't pass args by default)
      std::vector<Value> args;

      // Use PlatformInterface invocation which will call vm_->invokeFunction
      bool attempted = false;
      bool success = false;

      // console_log(std::string("[DEBUG] VM state: vm_=") + (vm_ ? "valid" :
      // "null") +
      //            " isRunning=" + (vm_ && vm_->isRunning() ? "true" : "false")
      //            + " hasError=" + (vm_ && vm_->hasError() ? "true" :
      //            "false"));

      if (vm_ != nullptr && vm_->isRunning() && !vm_->hasError()) {
        attempted = true;
        // console_log("[DEBUG] Attempting to invoke timer callback");
        // Directly invoke the function value
        success = vm_->invokeFunction(entry.callback, args);
        // console_log(std::string("[DEBUG] Callback invocation result: ") +
        // (success ? "success" : "failed"));

        // If the VM entered an error state during invocation, stop scheduling
        // further callbacks
        if (vm_->hasError() && !vm_->isRunning()) {
          console_warn(
              "VM entered error state during timer callback; clearing timers");
          // Clear all timers to avoid further invocations
          timers_.clear();
          break;
        }
      } else {
        // console_log("[DEBUG] Skipping timer callback - VM not ready");
      }

      // Only warn if we attempted invocation and it failed. If we didn't
      // attempt because the VM isn't running, avoid spamming the log.
      if (attempted && !success) {
        console_warn("Timer callback id=" + std::to_string(entry.id) +
                     " failed to invoke");
      }

      // Schedule next fire
      entry.nextFire = now + std::chrono::milliseconds(entry.intervalMs);
    }
  }
}

// === Memory Operations (Stubs) ===

int SDLPlatform::memory_getAvailable() {
  // If VM is attached, return the VM ValuePool available bytes
  if (vm_ != nullptr) {
    return static_cast<int>(vm_->getHeapAvailable());
  }
  console_warn(
      "NOT IMPLEMENTED: os.memory.getAvailable - memory tracking missing");
  return 0; // Not implemented - would need to track VM heap
}

int SDLPlatform::memory_getUsage() {
  if (vm_ != nullptr) {
    return static_cast<int>(vm_->getHeapUsage());
  }
  console_warn("NOT IMPLEMENTED: os.memory.getUsage - memory tracking missing");
  return 0; // Not implemented - would need to track VM allocations
}

// === System Operations ===

uint32_t SDLPlatform::system_getTime() {
  auto now = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_);
  return static_cast<uint32_t>(duration.count());
}

void SDLPlatform::system_sleep(uint32_t ms) {
  // Sleep is now handled by VM state - this is a no-op
  // The VM will yield and resume after the specified time
}

uint32_t SDLPlatform::system_getRTC() {
  auto now = std::chrono::system_clock::now();
  auto timestamp =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
  return static_cast<uint32_t>(timestamp.count()) +
         static_cast<uint32_t>(rtcOffset_);
}

void SDLPlatform::system_setRTC(uint32_t timestamp) {
  auto now = std::chrono::system_clock::now();
  auto current =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
  rtcOffset_ =
      static_cast<int64_t>(timestamp) - static_cast<int64_t>(current.count());
}

// === Console Operations ===

void SDLPlatform::console_print(const std::string &msg) {
  std::cout << msg << std::flush;
  outputLog_.addText(msg);
}

void SDLPlatform::console_println(const std::string &msg) {
  std::cout << msg << std::endl;
  outputLog_.addText(msg + "\n");
}

void SDLPlatform::console_log(const std::string &msg) {
  std::cout << "[INFO] " << msg << std::endl;
  consoleLog_.addText("[INFO] " + msg + "\n");
  addDebugMessage(msg);
}

void SDLPlatform::console_warn(const std::string &msg) {
  std::cout << "[WARN] " << msg << std::endl;
  outputLog_.addText("[WARN] " + msg + "\n");
}

void SDLPlatform::console_error(const std::string &msg) {
  std::cerr << "[ERROR] " << msg << std::endl;
  outputLog_.addText("[ERROR] " + msg + "\n");
}

void SDLPlatform::console_clear() { outputLog_.clear(); }

// Private helper methods

bool SDLPlatform::isInCircularDisplay(int x, int y) const {
  int dx = x - CENTER_X;
  int dy = y - CENTER_Y;
  return (dx * dx + dy * dy) <= (DISPLAY_RADIUS * DISPLAY_RADIUS);
}

void SDLPlatform::drawCircularMask() {
  if (!initialized_)
    return;

  // Draw border around circular display (gray ring at the edge)
  SDL_SetRenderDrawColor(renderer_, 64, 64, 64, 255);

  // Border: 2-pixel ring just inside the display radius
  // This keeps it fully visible within the display bounds
  int outerRadius = DISPLAY_RADIUS;     // 120 - edge of circle
  int innerRadius = DISPLAY_RADIUS - 2; // 118 - 2 pixels inward

  for (int y = 0; y < DISPLAY_HEIGHT; y++) {
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
      int dx = x - CENTER_X;
      int dy = y - CENTER_Y;
      int distSq = dx * dx + dy * dy;

      // Draw pixels in the border ring (between inner and outer radius)
      if (distSq >= (innerRadius * innerRadius) &&
          distSq <= (outerRadius * outerRadius)) {

        SDL_Rect rect = {DEBUG_PANEL_WIDTH + x * WINDOW_SCALE, y * WINDOW_SCALE,
                         WINDOW_SCALE, WINDOW_SCALE};
        SDL_RenderFillRect(renderer_, &rect);
      }
    }
  }
}

void SDLPlatform::drawEncoderIndicator() {
  if (!initialized_)
    return;

  // Calculate angle from encoder position
  // M5 Dial has 64 pulses per revolution, so normalize to 0-360 degrees
  const int PULSES_PER_REV = 64;
  float angle =
      (encoder_.position % PULSES_PER_REV) * (2.0f * 3.14159f / PULSES_PER_REV);

  // Draw a small arc section of the border in orange to indicate position
  // Orange color (RGB565: 0xFD20 -> RGB888: 255, 165, 0)
  SDL_SetRenderDrawColor(renderer_, 255, 165, 0, 255);

  // Use same radius range as the border
  int outerRadius = DISPLAY_RADIUS;     // 120 - edge of circle
  int innerRadius = DISPLAY_RADIUS - 2; // 118 - 2 pixels inward

  // Draw a small arc (about 5 degrees on each side of the position)
  const float arcSpan = 0.08f; // radians (~5 degrees)

  for (int y = 0; y < DISPLAY_HEIGHT; y++) {
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
      int dx = x - CENTER_X;
      int dy = y - CENTER_Y;
      int distSq = dx * dx + dy * dy;

      // Check if pixel is in the border ring
      if (distSq >= (innerRadius * innerRadius) &&
          distSq <= (outerRadius * outerRadius)) {

        // Calculate angle of this pixel
        float pixelAngle = atan2((float)dy, (float)dx);

        // Normalize angle difference to -PI to PI range
        float angleDiff = pixelAngle - angle;
        while (angleDiff > 3.14159f)
          angleDiff -= 2.0f * 3.14159f;
        while (angleDiff < -3.14159f)
          angleDiff += 2.0f * 3.14159f;

        // Draw if within arc span
        if (fabs(angleDiff) <= arcSpan) {
          SDL_Rect rect = {DEBUG_PANEL_WIDTH + x * WINDOW_SCALE,
                           y * WINDOW_SCALE, WINDOW_SCALE, WINDOW_SCALE};
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
  auto duration =
      std::chrono::duration_cast<std::chrono::seconds>(now - power_.lastUpdate);

  if (duration.count() > 60 && !power_.charging) { // Update every minute
    if (power_.batteryLevel > 0) {
      power_.batteryLevel = static_cast<uint8_t>(
          std::max(0, static_cast<int>(power_.batteryLevel) - 1));
    }
    power_.lastUpdate = now;
  }
}

void SDLPlatform::renderText(int x, int y, const std::string &text,
                             const Color &color, int size) {
  // Get font of appropriate size
  TTF_Font* textFont = getFontOfSize(size);
  if (!textFont)
    return;

  SDL_Surface *surface =
      TTF_RenderText_Solid(textFont, text.c_str(), color.toSDL());
  if (!surface)
    return;

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer_, surface);
  if (!texture) {
    SDL_FreeSurface(surface);
    return;
  }

  // Use actual coordinates without scaling for console text
  SDL_Rect destRect = {x, y, surface->w*size, surface->h*size};

  SDL_RenderCopy(renderer_, texture, nullptr, &destRect);

  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
}

void SDLPlatform::addDebugMessage(const std::string &msg) {
  debugMessages_.push_back(msg);

  // Keep only last 20 messages
  if (debugMessages_.size() > 20) {
    debugMessages_.erase(debugMessages_.begin());
  }
}

void SDLPlatform::debug_showInfo(bool /*show*/) {
  // Placeholder - debug info always shown in current implementation
  // Could be extended to toggle debug overlay visibility
}

void SDLPlatform::debug_drawOverlay() {
  if (!initialized_ || !font_)
    return;

  // Draw semi-transparent background for debug info
  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 128);

  SDL_Rect overlayRect = {0, 0, WINDOW_WIDTH, 200};
  SDL_RenderFillRect(renderer_, &overlayRect);

  // Draw debug information
  int yPos = 10;
  Color textColor(255, 255, 255, 255);

  // System info
  renderText(10, yPos, "=== dialOS Debug Info ===", textColor, 1);
  yPos += 20;

  renderText(10, yPos, "Time: " + std::to_string(system_getTime()) + "ms",
             textColor, 1);
  yPos += 15;

  renderText(10, yPos,
             "Encoder: pos=" + std::to_string(encoder_.position) +
                 " btn=" + std::string(encoder_.pressed ? "ON" : "OFF"),
             textColor, 1);
  yPos += 15;

  renderText(10, yPos,
             "Touch: " + std::string(touch_.pressed ? "ON" : "OFF") + " (" +
                 std::to_string(touch_.x) + "," + std::to_string(touch_.y) +
                 ")",
             textColor, 1);
  yPos += 15;

  renderText(10, yPos,
             "RFID: " + std::string(rfid_.cardPresent
                                        ? "CARD(" + rfid_.cardUID + ")"
                                        : "NONE"),
             textColor, 1);
  yPos += 15;

  renderText(10, yPos,
             "Battery: " + std::to_string(power_.batteryLevel) + "% " +
                 std::string(power_.charging ? "CHARGING" : ""),
             textColor, 1);
  yPos += 15;

  renderText(10, yPos,
             "Buzzer: " + std::string(buzzer_.isPlaying ? "ACTIVE" : "IDLE") +
                 " " + std::to_string(buzzer_.frequency) + "Hz",
             textColor, 1);

  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
}

// Additional peripheral simulation methods (not part of PlatformInterface)

int SDLPlatform::power_getBatteryLevel() { return power_.batteryLevel; }

bool SDLPlatform::power_isCharging() { return power_.charging; }

void SDLPlatform::power_sleep() {
  console_log("System entering sleep mode...");
  // In a real implementation, this would put the system into low-power mode
}

// === Display Extended Operations ===

void SDLPlatform::display_setTitle(const std::string &title) {
  if (window_) {
    SDL_SetWindowTitle(window_, title.c_str());
  }
}

void SDLPlatform::display_drawImage(int x, int y,
                                    const std::vector<uint8_t> &imageData) {
  // Implement simple image drawing from raw data
  // Format: width(2), height(2), RGB565 pixel data
  if (imageData.size() < 4)
    return;

  uint16_t width = (imageData[0] << 8) | imageData[1];
  uint16_t height = (imageData[2] << 8) | imageData[3];

  if (imageData.size() < 4 + (width * height * 2))
    return; // RGB565 = 2 bytes per pixel

  // Draw the image pixel by pixel
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      if (!isInCircularDisplay(x + col, y + row))
        continue;

      int index = 4 + ((row * width + col) * 2);
      uint16_t rgb565 = (imageData[index] << 8) | imageData[index + 1];

      Color color(rgb565);
      SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);

      // Scale coordinates for display
      int scaledX = DEBUG_PANEL_WIDTH + (x + col) * WINDOW_SCALE;
      int scaledY = (y + row) * WINDOW_SCALE;
      SDL_Rect pixel = {scaledX, scaledY, WINDOW_SCALE, WINDOW_SCALE};
      SDL_RenderFillRect(renderer_, &pixel);
    }
  }

  console_log("display_drawImage: drew " + std::to_string(width) + "x" +
              std::to_string(height) + " image at (" + std::to_string(x) + "," +
              std::to_string(y) + ")");
}

// === System Extended Operations ===

void SDLPlatform::system_yield() {
  SDL_Delay(1); // Small delay to allow other processes to run
}

// === Memory Extended Operations ===

int SDLPlatform::memory_allocate(int size) {
  // TODO: Implement memory allocation tracking
  console_log("memory_allocate: " + std::to_string(size) + " bytes");
  console_warn(
      "NOT IMPLEMENTED: os.memory.allocate - memory allocation not simulated");
  return -1; // Not implemented
}

void SDLPlatform::memory_free(int handle) {
  // TODO: Implement memory deallocation tracking
  console_log("memory_free: handle " + std::to_string(handle));
  console_warn(
      "NOT IMPLEMENTED: os.memory.free - memory deallocation not simulated");
}

// === Directory Operations ===

std::vector<std::string> SDLPlatform::dir_list(const std::string &path) {
  std::string fullPath = fileSystemRoot_ + path;
  console_log("dir_list: " + path);

  std::vector<std::string> result;

  try {
    if (!std::filesystem::exists(fullPath)) {
      console_warn("Directory does not exist: " + path);
      return result;
    }

    if (!std::filesystem::is_directory(fullPath)) {
      console_warn("Path is not a directory: " + path);
      return result;
    }

    for (const auto &entry : std::filesystem::directory_iterator(fullPath)) {
      std::string filename = entry.path().filename().string();
      result.push_back(filename);
    }

    console_log("Listed " + std::to_string(result.size()) + " entries in " +
                path);
  } catch (const std::exception &e) {
    console_error("Error listing directory " + path + ": " + e.what());
  }

  return result;
}

bool SDLPlatform::dir_create(const std::string &path) {
  std::string fullPath = fileSystemRoot_ + path;
  console_log("dir_create: " + path);

  try {
    bool created = std::filesystem::create_directories(fullPath);
    console_log("Directory create: " + path + " -> " +
                (created ? "success" : "already exists"));
    return true; // Return true if created or already exists
  } catch (const std::exception &e) {
    console_error("Error creating directory " + path + ": " + e.what());
    return false;
  }
}

bool SDLPlatform::dir_delete(const std::string &path) {
  std::string fullPath = fileSystemRoot_ + path;
  console_log("dir_delete: " + path);

  try {
    if (!std::filesystem::exists(fullPath)) {
      console_warn("Cannot delete non-existent directory: " + path);
      return false;
    }

    std::uintmax_t deletedCount = std::filesystem::remove_all(fullPath);
    console_log("Directory delete: " + path + " -> deleted " +
                std::to_string(deletedCount) + " items");
    return deletedCount > 0;
  } catch (const std::exception &e) {
    console_error("Error deleting directory " + path + ": " + e.what());
    return false;
  }
}

bool SDLPlatform::dir_exists(const std::string &path) {
  std::string fullPath = fileSystemRoot_ + path;

  try {
    bool exists = std::filesystem::exists(fullPath) &&
                  std::filesystem::is_directory(fullPath);
    console_log("Directory exists check: " + path + " -> " +
                (exists ? "true" : "false"));
    return exists;
  } catch (const std::exception &e) {
    console_error("Error checking directory existence " + path + ": " +
                  e.what());
    return false;
  }
}

// === App Operations ===

void SDLPlatform::app_exit() {
  console_log("App exit requested");
  shouldQuit_ = true;
}

std::string SDLPlatform::app_getInfo() {
  return "{\"name\":\"dialOS "
         "Emulator\",\"version\":\"1.0\",\"platform\":\"SDL2\"}";
}

// === Storage Operations ===

std::vector<std::string> SDLPlatform::storage_getMounted() {
  // TODO: Implement storage device listing
  console_warn("NOT IMPLEMENTED: os.storage.getMounted - storage enumeration "
               "partial/simulated");
  return {"internal"}; // Simulated internal storage
}

std::string SDLPlatform::storage_getInfo(const std::string &device) {
  // TODO: Implement storage info retrieval
  console_warn(
      "NOT IMPLEMENTED: os.storage.getInfo - storage info partial/simulated");
  if (device == "internal") {
    return "{\"device\":\"internal\",\"type\":\"flash\",\"size\":8388608,"
           "\"free\":4194304}";
  }
  return "{}";
}

// === Sensor Operations ===

int SDLPlatform::sensor_attach(const std::string &port,
                               const std::string &type) {
  // TODO: Implement sensor attachment simulation
  console_log("sensor_attach: port=" + port + " type=" + type);
  console_warn("NOT IMPLEMENTED: os.sensor.attach - sensor simulation missing");
  return -1; // Not implemented
}

std::string SDLPlatform::sensor_read(int handle) {
  // TODO: Implement sensor data reading
  console_log("sensor_read: handle=" + std::to_string(handle));
  console_warn("NOT IMPLEMENTED: os.sensor.read - sensor simulation missing");
  return "{}";
}

void SDLPlatform::sensor_detach(int handle) {
  // TODO: Implement sensor detachment
  console_log("sensor_detach: handle=" + std::to_string(handle));
  console_warn("NOT IMPLEMENTED: os.sensor.detach - sensor simulation missing");
}

// === WiFi Operations ===

bool SDLPlatform::wifi_connect(const std::string &ssid,
                               const std::string &password) {
  console_log("wifi_connect: ssid=" + ssid);

  // Fake WiFi connection - always succeeds in simulator
  wifiConnected_ = true;
  wifiSSID_ = ssid;
  wifiIP_ = "192.168.1.42"; // Fake IP address

  console_log("WiFi connected to '" + ssid + "' (simulated)");
  return true;
}

void SDLPlatform::wifi_disconnect() {
  console_log("wifi_disconnect");
  wifiConnected_ = false;
  wifiSSID_ = "";
  wifiIP_ = "";
  console_log("WiFi disconnected (simulated)");
}

std::string SDLPlatform::wifi_getStatus() {
  if (wifiConnected_) {
    return "{\"status\":\"connected\",\"ssid\":\"" + wifiSSID_ +
           "\",\"signal\":-45,\"quality\":85}";
  } else {
    return "{\"status\":\"disconnected\"}";
  }
}

std::string SDLPlatform::wifi_getIP() { return wifiConnected_ ? wifiIP_ : ""; }

std::string SDLPlatform::wifi_scan() {
  console_log("wifi_scan: scanning for access points");

  // Simulate WiFi AP scan with fake access points
  std::string scanResults = R"([
                {
                    "ssid": "HomeNetwork",
                    "bssid": "aa:bb:cc:dd:ee:ff",
                    "signal": -42,
                    "channel": 6,
                    "security": "WPA2",
                    "quality": 85
                },
                {
                    "ssid": "OfficeWiFi",
                    "bssid": "11:22:33:44:55:66",
                    "signal": -55,
                    "channel": 11,
                    "security": "WPA3",
                    "quality": 72
                },
                {
                    "ssid": "CafeGuest",
                    "bssid": "77:88:99:aa:bb:cc",
                    "signal": -68,
                    "channel": 1,
                    "security": "Open",
                    "quality": 45
                },
                {
                    "ssid": "AndroidAP",
                    "bssid": "dd:ee:ff:00:11:22",
                    "signal": -72,
                    "channel": 3,
                    "security": "WPA2",
                    "quality": 38
                }
            ])";

  console_log("WiFi scan completed: found 4 access points (simulated)");
  return scanResults;
}

// === HTTP Operations ===

std::string SDLPlatform::http_get(const std::string &url) {
  console_log("http_get: url=" + url);

  if (!wifiConnected_) {
    console_warn("HTTP GET failed: WiFi not connected");
    return "{\"status\":\"error\",\"message\":\"WiFi not connected\"}";
  }

// Use Windows HTTP API for real HTTP requests
#ifdef _WIN32
  try {
    // Parse URL to extract host and path
    std::string host, path;
    if (!parseURL(url, host, path)) {
      return "{\"status\":\"error\",\"message\":\"Invalid URL format\"}";
    }

    // Execute HTTP GET request
    std::string response = executeHTTPRequest("GET", host, path, "");
    console_log("HTTP GET completed: " + std::to_string(response.length()) +
                " bytes received");
    return response;

  } catch (const std::exception &e) {
    console_error("HTTP GET error: " + std::string(e.what()));
    return "{\"status\":\"error\",\"message\":\"" + std::string(e.what()) +
           "\"}";
  }
#else
  console_warn("HTTP GET not supported on this platform");
  return "{\"status\":\"error\",\"message\":\"HTTP not supported on this "
         "platform\"}";
#endif
}

std::string SDLPlatform::http_post(const std::string &url,
                                   const std::string &data) {
  console_log("http_post: url=" + url +
              " data_length=" + std::to_string(data.length()));

  if (!wifiConnected_) {
    console_warn("HTTP POST failed: WiFi not connected");
    return "{\"status\":\"error\",\"message\":\"WiFi not connected\"}";
  }

// Use Windows HTTP API for real HTTP requests
#ifdef _WIN32
  try {
    // Parse URL to extract host and path
    std::string host, path;
    if (!parseURL(url, host, path)) {
      return "{\"status\":\"error\",\"message\":\"Invalid URL format\"}";
    }

    // Execute HTTP POST request
    std::string response = executeHTTPRequest("POST", host, path, data);
    console_log("HTTP POST completed: " + std::to_string(response.length()) +
                " bytes received");
    return response;

  } catch (const std::exception &e) {
    console_error("HTTP POST error: " + std::string(e.what()));
    return "{\"status\":\"error\",\"message\":\"" + std::string(e.what()) +
           "\"}";
  }
#else
  console_warn("HTTP POST not supported on this platform");
  return "{\"status\":\"error\",\"message\":\"HTTP not supported on this "
         "platform\"}";
#endif
}

std::string SDLPlatform::http_download(const std::string &url,
                                       const std::string &filepath) {
  console_log("http_download: url=" + url + " filepath=" + filepath);

  if (!wifiConnected_) {
    console_warn("HTTP DOWNLOAD failed: WiFi not connected");
    return "{\"status\":\"error\",\"message\":\"WiFi not connected\"}";
  }

// Use Windows HTTP API for real HTTP download
#ifdef _WIN32
  try {
    // Parse URL to extract host and path
    std::string host, path;
    if (!parseURL(url, host, path)) {
      return "{\"status\":\"error\",\"message\":\"Invalid URL format\"}";
    }

    // Download file using HTTP GET and save to disk
    std::string downloadResult = executeHTTPDownload(host, path, filepath);
    return downloadResult;

  } catch (const std::exception &e) {
    console_error("HTTP DOWNLOAD error: " + std::string(e.what()));
    return "{\"status\":\"error\",\"message\":\"" + std::string(e.what()) +
           "\"}";
  }
#else
  console_warn("HTTP DOWNLOAD not supported on this platform");
  return "{\"status\":\"error\",\"message\":\"HTTP not supported on this "
         "platform\"}";
#endif
}

// === IPC Operations ===

bool SDLPlatform::ipc_send(const std::string &appId,
                           const std::string &message) {
  // TODO: Implement IPC simulation
  console_log("ipc_send: to=" + appId + " msg=" + message);
  console_warn("NOT IMPLEMENTED: os.ipc.send - IPC simulation missing");
  return false; // Not implemented
}

void SDLPlatform::ipc_broadcast(const std::string &message) {
  console_log("ipc_broadcast: " + message);
  console_warn("NOT IMPLEMENTED: os.ipc.broadcast - IPC simulation missing");
}

// Attempt to locate a source file for a given bytecode path
std::string SDLPlatform::locateSourceFile(const std::string &bytecodePath) {
  return locateSourceForBytecode(bytecodePath);
}

// Print runtime error with optional source context. This performs file I/O
// and uses the same search logic as the emulator harness.
void SDLPlatform::printRuntimeError(const VMState &vm,
                                    const std::string &bytecodePath, size_t pc,
                                    const std::string &errorMessage,
                                    uint32_t sourceLine) {
  console_error("Runtime Error: " + errorMessage);
  console_log(std::string("PC: ") + std::to_string(pc) +
              ", Stack: " + std::to_string(vm.getStackSize()));

  // Try to locate source and print context
  std::string sourceFile = locateSourceForBytecode(bytecodePath);
  if (!sourceFile.empty()) {
    if (sourceLine > 0) {
      std::string ctx = getSourceContextFromFile(sourceFile, sourceLine, 5);
      console_println(ctx);
    } else {
      console_log("Debug info available but no line mapping for PC " +
                  std::to_string(pc));
    }
  } else {
    console_log("Debug info available but source file not found");
  }
}

// Helper: attempt to find a .ds source file corresponding to a bytecode path
std::string
SDLPlatform::locateSourceForBytecode(const std::string &bytecodePath) {
  namespace fs = std::filesystem;

  try {
    fs::path p(bytecodePath);

    // 1) If the bytecode path has an accompanying .ds in the same folder, try
    // it
    if (p.has_parent_path()) {
      fs::path cand = p.parent_path() / (p.stem().string() + ".ds");
      if (fs::exists(cand) && fs::is_regular_file(cand))
        return cand.string();
    }

    // 2) Try replacing extension in-place (handles cases where bytecode
    // includes ext)
    fs::path replaced = p;
    replaced.replace_extension(".ds");
    if (fs::exists(replaced) && fs::is_regular_file(replaced))
      return replaced.string();

    // 3) Try scripts/ directory next to repository (common location for .ds
    // sources)
    fs::path repoScripts =
        fs::current_path() / "scripts" / (p.stem().string() + ".ds");
    if (fs::exists(repoScripts) && fs::is_regular_file(repoScripts))
      return repoScripts.string();

    // 4) Finally try current working directory with the stem
    fs::path local = fs::current_path() / (p.stem().string() + ".ds");
    if (fs::exists(local) && fs::is_regular_file(local))
      return local.string();
  } catch (const std::exception &ex) {
    std::cerr << "locateSourceForBytecode error: " << ex.what() << std::endl;
  }

  return std::string();
}

// Helper: read a source file and return a small context block around errorLine
std::string SDLPlatform::getSourceContextFromFile(const std::string &sourceFile,
                                                  uint32_t errorLine,
                                                  int contextLines) {
  std::ifstream in(sourceFile);
  if (!in)
    return std::string();

  std::vector<std::string> lines;
  std::string line;
  while (std::getline(in, line))
    lines.push_back(line);

  if (lines.empty())
    return std::string();

  // errorLine is 1-based. Clamp it.
  size_t total = lines.size();
  size_t idx = (errorLine == 0)
                   ? 0
                   : (std::min<uint32_t>(errorLine, (uint32_t)total) - 1);

  int start = static_cast<int>(idx) - contextLines;
  int end = static_cast<int>(idx) + contextLines;
  if (start < 0)
    start = 0;
  if (end > static_cast<int>(total) - 1)
    end = static_cast<int>(total) - 1;

  std::ostringstream oss;
  oss << "Source: " << sourceFile << "\n";
  for (int i = start; i <= end; ++i) {
    // line numbers are 1-based for display
    oss << std::setw(4) << (i + 1) << ": " << lines[i] << "\n";
    if (i == static_cast<int>(idx)) {
      // simple caret indicator on next line
      oss << " ^--- error on this line\n";
    }
  }
  return oss.str();
}

// === Buzzer Extended Operations ===

void SDLPlatform::buzzer_playMelody(const std::vector<int> &notes) {
  // Implement melody playback simulation
  // Notes array format: frequency1, duration1, frequency2, duration2, ...
  console_log("buzzer_playMelody: playing " + std::to_string(notes.size() / 2) +
              " notes");

  for (size_t i = 0; i < notes.size(); i += 2) {
    if (i + 1 < notes.size()) {
      int frequency = notes[i];
      int duration = notes[i + 1];

      if (frequency > 0) {
        console_log("  Note: " + std::to_string(frequency) + "Hz for " +
                    std::to_string(duration) + "ms");
        buzzer_.isPlaying = true;
        buzzer_.frequency = frequency;
        buzzer_.endTime = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(duration);

        // Simple audio feedback (beep simulation)
        // In a real implementation, you'd use SDL_mixer to generate audio
        SDL_Delay(duration / 10); // Quick visual feedback
      } else {
        // Rest (silence)
        console_log("  Rest: " + std::to_string(duration) + "ms");
        SDL_Delay(duration / 10);
      }
    }
  }

  buzzer_.isPlaying = false;
}

void SDLPlatform::renderDebugPanel() {
  if (!initialized_ || !vm_)
    return;

  // Debug panel background
  SDL_SetRenderDrawColor(renderer_, 20, 20, 20, 255); // Dark background
  SDL_Rect panelRect = {0, 0, DEBUG_PANEL_WIDTH, WINDOW_HEIGHT};
  SDL_RenderFillRect(renderer_, &panelRect);

  // Border
  SDL_SetRenderDrawColor(renderer_, 100, 100, 100, 255);
  SDL_RenderDrawRect(renderer_, &panelRect);

  // Title
  int yPos = 10;
  renderText(10, yPos, "VM State", Color(0xFF00FFFF), 1);
  yPos += 25;

  // Separator line
  SDL_SetRenderDrawColor(renderer_, 100, 100, 100, 255);
  SDL_RenderDrawLine(renderer_, 10, yPos, DEBUG_PANEL_WIDTH - 10, yPos);
  yPos += 10;

  // VM state info
  std::stringstream ss;

  // Running state
  yPos += 15;
  ss.str("");
  ss << "Running: " << (vm_->isRunning() ? "Yes" : "No");
  renderText(10, yPos, ss.str(),
             Color(vm_->isRunning() ? 0x00FF00FF : 0xFF0000FF), 1);

  // Program counter
  yPos += 20;
  ss.str("");
  ss << "PC: " << vm_->getPC();
  renderText(10, yPos, ss.str(), Color(0xFFFFFFFF), 1);

  // Stack size
  yPos += 20;
  ss.str("");
  ss << "Stack: " << vm_->getStackSize();
  renderText(10, yPos, ss.str(), Color(0xFFFFFFFF), 1);

  // Call stack depth
  yPos += 20;
  ss.str("");
  ss << "Calls: " << vm_->getCallStackDepth();
  renderText(10, yPos, ss.str(), Color(0xFFFFFFFF), 1);

  // Heap usage
  yPos += 20;
  ss.str("");
  ss << "Heap: " << vm_->getHeapUsage() << " bytes";
  renderText(10, yPos, ss.str(), Color(0xFFFFFFFF), 1);

  // Error state
  if (vm_->hasError()) {
    yPos += 25;
    renderText(10, yPos, "ERROR:", Color(0xFF0000FF), 1);
    yPos += 20;

    std::string error = vm_->getError();
    // Wrap error text if too long
    size_t maxLen = 25;
    size_t pos = 0;
    while (pos < error.length()) {
      std::string line = error.substr(pos, maxLen);
      renderText(10, yPos, line, Color(0xFF8800FF), 1);
      yPos += 15;
      pos += maxLen;
      if (yPos > WINDOW_HEIGHT - 20)
        break;
    }
  }

  // Hardware state section separator
  yPos += 25;
  SDL_SetRenderDrawColor(renderer_, 100, 100, 100, 255);
  SDL_RenderDrawLine(renderer_, 10, yPos, DEBUG_PANEL_WIDTH - 10, yPos);
  yPos += 10;

  // Hardware state title
  yPos += 15;
  renderText(10, yPos, "Hardware State", Color(0xFF00FFFF), 1);
  yPos += 20;

  // System time
  ss.str("");
  ss << "Time: " << system_getTime() << "ms";
  renderText(10, yPos, ss.str(), Color(0xFFFFFFFF), 1);
  yPos += 15;

  // Encoder state
  ss.str("");
  ss << "Encoder: " << encoder_.position;
  renderText(10, yPos, ss.str(), Color(0xFFFFFFFF), 1);
  yPos += 15;
  ss.str("");
  ss << "  btn=" << (encoder_.pressed ? "ON" : "OFF");
  renderText(10, yPos, ss.str(),
             Color(encoder_.pressed ? 0x00FF00FF : 0x888888FF), 1);
  yPos += 15;

  // Touch state
  ss.str("");
  ss << "Touch: " << (touch_.pressed ? "ON" : "OFF");
  renderText(10, yPos, ss.str(),
             Color(touch_.pressed ? 0x00FF00FF : 0x888888FF), 1);
  yPos += 15;
  ss.str("");
  ss << "  (" << touch_.x << "," << touch_.y << ")";
  renderText(10, yPos, ss.str(), Color(0xFFFFFFFF), 1);
  yPos += 15;

  // RFID state
  ss.str("");
  ss << "RFID: " << (rfid_.cardPresent ? "CARD" : "NONE");
  renderText(10, yPos, ss.str(),
             Color(rfid_.cardPresent ? 0x00FF00FF : 0x888888FF), 1);
  if (rfid_.cardPresent && !rfid_.cardUID.empty()) {
    yPos += 15;
    ss.str("");
    ss << "  " << rfid_.cardUID;
    renderText(10, yPos, ss.str(), Color(0xFFFFFFFF), 1);
  }
  yPos += 15;

  // Battery state
  ss.str("");
  ss << "Battery: " << static_cast<int>(power_.batteryLevel) << "%";
  if (power_.charging) {
    ss << " CHG";
  }
  renderText(10, yPos, ss.str(), Color(0xFFFFFFFF), 1);
  yPos += 15;

  // Buzzer state
  ss.str("");
  ss << "Buzzer: " << (buzzer_.isPlaying ? "ACTIVE" : "IDLE");
  renderText(10, yPos, ss.str(),
             Color(buzzer_.isPlaying ? 0xFFFF00FF : 0x888888FF), 1);
  if (buzzer_.isPlaying) {
    yPos += 15;
    ss.str("");
    ss << "  " << buzzer_.frequency << "Hz";
    renderText(10, yPos, ss.str(), Color(0xFFFFFFFF), 1);
  }

  // Separator
  yPos = WINDOW_HEIGHT - 100;
  SDL_SetRenderDrawColor(renderer_, 100, 100, 100, 255);
  SDL_RenderDrawLine(renderer_, 10, yPos, DEBUG_PANEL_WIDTH - 10, yPos);
  yPos += 10;

  // Controls info
  yPos += 15;
  renderText(10, yPos, "Controls:", Color(0x88FF88FF), 1);
  yPos += 20;
  renderText(10, yPos, "R: RFID card", Color(0xCCCCCCFF), 1);
  yPos += 15;
  renderText(10, yPos, "B: Buzzer test", Color(0xCCCCCCFF), 1);
  yPos += 15;
  renderText(10, yPos, "ESC: Exit", Color(0xCCCCCCFF), 1);
}

void SDLPlatform::renderConsoleArea() {
  if (!initialized_)
    return;

  // Get actual current window size
  int windowWidth, windowHeight;
  SDL_GetWindowSize(window_, &windowWidth, &windowHeight);

  // Console area starts to the right of the debug panel and display
  int consoleX = DEBUG_PANEL_WIDTH + DISPLAY_SCALED_WIDTH;
  int consoleY = 0;
  int consoleWidth = CONSOLE_WIDTH;
  int consoleHeight = windowHeight;

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
  renderLogWindow(consoleX + 5, consoleY + 5, consoleWidth - 10,
                  halfHeight - 10, "Console Log", consoleLog_);

  // Draw separator line
  SDL_SetRenderDrawColor(renderer_, 100, 100, 100, 255);
  SDL_RenderDrawLine(renderer_, consoleX + 5, consoleY + halfHeight,
                     consoleX + consoleWidth - 5, consoleY + halfHeight);

  // Render output log (bottom half)
  renderLogWindow(consoleX + 5, consoleY + halfHeight + 5, consoleWidth - 10,
                  halfHeight - 10, "Program Output", outputLog_);
}

void SDLPlatform::renderLogWindow(int x, int y, int width, int height,
                                  const std::string &title,
                                  const ConsoleLog &log) {
  if (!font_)
    return;

  // Render title
  Color titleColor(200, 200, 255); // Light blue
  renderText(x, y, title, titleColor, 1);

  // Calculate text area
  int textY = y + 20;
  int textHeight = height - 25;
  int lineHeight = 14;
  int maxLines = textHeight / lineHeight;

  // Get lines from log
  auto lines = log.getLines();

  // Render log lines (most recent at bottom)
  Color textColor(220, 220, 220); // Light gray
  int startLine = std::max(0, static_cast<int>(lines.size()) - maxLines);

  for (size_t i = static_cast<size_t>(startLine);
       i < lines.size() && i < static_cast<size_t>(startLine + maxLines); i++) {
    int lineY = textY + static_cast<int>(i - static_cast<size_t>(startLine)) *
                            lineHeight;

    // Truncate long lines to fit width
    std::string line = lines[i];
    if (line.length() > 35) { // Rough character limit for console width
      line = line.substr(0, 32) + "...";
    }

    renderText(x + 2, lineY, line, textColor, 1);
  }

  // Show scroll indicator if there are more lines
  if (lines.size() > maxLines) {
    Color scrollColor(150, 150, 150);
    renderText(x + width - 20, y, "^", scrollColor, 1);
  }
}

// === HTTP Helper Functions ===

bool SDLPlatform::parseURL(const std::string &url, std::string &host,
                           std::string &path) {
  // Simple URL parser for http://host:port/path format
  const std::string prefix = "http://";
  const std::string httpsPrefix = "https://";

  std::string workingUrl = url;

  // Remove protocol prefix
  if (workingUrl.find(prefix) == 0) {
    workingUrl = workingUrl.substr(prefix.length());
  } else if (workingUrl.find(httpsPrefix) == 0) {
    workingUrl = workingUrl.substr(httpsPrefix.length());
  }

  // Find first slash to separate host from path
  size_t slashPos = workingUrl.find('/');
  if (slashPos == std::string::npos) {
    host = workingUrl;
    path = "/";
  } else {
    host = workingUrl.substr(0, slashPos);
    path = workingUrl.substr(slashPos);
  }

  return !host.empty();
}

std::string SDLPlatform::executeHTTPRequest(const std::string &method,
                                            const std::string &host,
                                            const std::string &path,
                                            const std::string &data) {
#ifdef _WIN32
  try {
    // Parse host and port
    std::string hostname = host;
    int port = 80; // default HTTP port

    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos) {
      hostname = host.substr(0, colonPos);
      std::string portStr = host.substr(colonPos + 1);
      port = std::stoi(portStr);
    }

    console_log("HTTP connecting to host: " + hostname +
                " port: " + std::to_string(port));

    // Convert strings to wide strings for Windows API
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wHost = converter.from_bytes(hostname);
    std::wstring wPath = converter.from_bytes(path);
    std::wstring wMethod = converter.from_bytes(method);

    // Initialize WinHTTP
    HINTERNET hSession = WinHttpOpen(
        L"dialOS HTTP Client/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    if (!hSession) {
      throw std::runtime_error("Failed to initialize WinHTTP session");
    }

    // Connect to the server with correct port
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(),
                                        static_cast<INTERNET_PORT>(port), 0);

    if (!hConnect) {
      WinHttpCloseHandle(hSession);
      throw std::runtime_error("Failed to connect to host: " + host);
    }

    // Create HTTP request
    HINTERNET hRequest =
        WinHttpOpenRequest(hConnect, wMethod.c_str(), wPath.c_str(), NULL,
                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);

    if (!hRequest) {
      WinHttpCloseHandle(hConnect);
      WinHttpCloseHandle(hSession);
      throw std::runtime_error("Failed to create HTTP request");
    }

    // Send the request
    BOOL bResults = FALSE;
    if (method == "POST" && !data.empty()) {
      std::string headers = "Content-Type: application/json\r\n";
      std::wstring wHeaders = converter.from_bytes(headers);

      bResults = WinHttpSendRequest(hRequest, wHeaders.c_str(),
                                    wHeaders.length(), (LPVOID)data.c_str(),
                                    data.length(), data.length(), 0);
    } else {
      bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                    WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    }

    if (!bResults) {
      WinHttpCloseHandle(hRequest);
      WinHttpCloseHandle(hConnect);
      WinHttpCloseHandle(hSession);
      throw std::runtime_error("Failed to send HTTP request");
    }

    // Receive response
    bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResults) {
      WinHttpCloseHandle(hRequest);
      WinHttpCloseHandle(hConnect);
      WinHttpCloseHandle(hSession);
      throw std::runtime_error("Failed to receive HTTP response");
    }

    // Read response data
    std::string response;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;

    do {
      // Check for available data
      dwSize = 0;
      if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
        throw std::runtime_error("Error in WinHttpQueryDataAvailable");
      }

      if (dwSize == 0)
        break;

      // Allocate space for the buffer
      std::vector<char> buffer(dwSize + 1);

      // Read the data
      if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
        throw std::runtime_error("Error in WinHttpReadData");
      }

      buffer[dwDownloaded] = '\0';
      response.append(buffer.data(), dwDownloaded);

    } while (dwSize > 0);

    // Clean up
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return response;

  } catch (const std::exception &e) {
    throw; // Re-throw the exception to be handled by caller
  }
#else
  throw std::runtime_error("HTTP requests not supported on this platform");
#endif
}

std::string SDLPlatform::executeHTTPDownload(const std::string &host,
                                             const std::string &path,
                                             const std::string &filepath) {
#ifdef _WIN32
  try {
    // Parse host and port
    std::string hostname = host;
    int port = 80; // default HTTP port

    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos) {
      hostname = host.substr(0, colonPos);
      std::string portStr = host.substr(colonPos + 1);
      port = std::stoi(portStr);
    }

    console_log("HTTP download connecting to host: " + hostname +
                " port: " + std::to_string(port));

    // Convert strings to wide strings for Windows API
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wHost = converter.from_bytes(hostname);
    std::wstring wPath = converter.from_bytes(path);

    // Initialize WinHTTP
    HINTERNET hSession = WinHttpOpen(
        L"dialOS HTTP Client/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    if (!hSession) {
      throw std::runtime_error("Failed to initialize WinHTTP session");
    }

    // Connect to the server with correct port
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(),
                                        static_cast<INTERNET_PORT>(port), 0);

    if (!hConnect) {
      WinHttpCloseHandle(hSession);
      throw std::runtime_error("Failed to connect to host: " + hostname + ":" +
                               std::to_string(port));
    }

    // Create an HTTP request handle for GET
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"GET", wPath.c_str(), NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_REFRESH);

    if (!hRequest) {
      WinHttpCloseHandle(hConnect);
      WinHttpCloseHandle(hSession);
      throw std::runtime_error("Failed to create HTTP request");
    }

    // Send the request
    BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS,
                                       0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    if (!bResults) {
      WinHttpCloseHandle(hRequest);
      WinHttpCloseHandle(hConnect);
      WinHttpCloseHandle(hSession);
      throw std::runtime_error("Failed to send HTTP request");
    }

    // End the request
    bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResults) {
      WinHttpCloseHandle(hRequest);
      WinHttpCloseHandle(hConnect);
      WinHttpCloseHandle(hSession);
      throw std::runtime_error("Failed to receive HTTP response");
    }

    // Create filesystem root if it doesn't exist
    std::filesystem::create_directories(fileSystemRoot_);

    // Convert dialOS path to full filesystem path
    std::string fullPath = fileSystemRoot_ + filepath;

    // Create parent directories if they don't exist
    std::filesystem::path filePath(fullPath);
    if (filePath.has_parent_path()) {
      std::filesystem::create_directories(filePath.parent_path());
    }

    // Open output file for binary write
    std::ofstream outFile(fullPath, std::ios::binary);
    if (!outFile.is_open()) {
      WinHttpCloseHandle(hRequest);
      WinHttpCloseHandle(hConnect);
      WinHttpCloseHandle(hSession);
      throw std::runtime_error("Failed to open output file: " + fullPath);
    }

    // Read and write data in chunks
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    size_t totalBytes = 0;

    do {
      // Check for available data
      if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
        outFile.close();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        throw std::runtime_error("Error in WinHttpQueryDataAvailable");
      }

      if (dwSize == 0)
        break;

      // Allocate buffer for this chunk
      std::vector<char> buffer(dwSize);

      // Read the data
      if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
        outFile.close();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        throw std::runtime_error("Error in WinHttpReadData");
      }

      // Write to file
      outFile.write(buffer.data(), dwDownloaded);
      totalBytes += dwDownloaded;

    } while (dwSize > 0);

    outFile.close();

    // Clean up
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    console_log("HTTP download completed: " + std::to_string(totalBytes) +
                " bytes written to " + fullPath);
    return "{\"status\":\"success\",\"bytes\":" + std::to_string(totalBytes) +
           ",\"filepath\":\"" + filepath + "\"}";

  } catch (const std::exception &e) {
    console_error("HTTP download error: " + std::string(e.what()));
    return "{\"status\":\"error\",\"message\":\"" + std::string(e.what()) +
           "\"}";
  }
#else
  return "{\"status\":\"error\",\"message\":\"HTTP download not supported on "
         "this platform\"}";
#endif
}

// ===== App Management Operations =====
// Cross-platform implementation designed for both SDL and ESP32

std::string SDLPlatform::app_install(const std::string &dsbFilePath,
                                     const std::string &appId) {
  try {
    // Validate DSB file exists and is readable
    std::string fullPath = fileSystemRoot_ + "/" + dsbFilePath;
    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
      return "{\"status\":\"error\",\"message\":\"DSB file not found: " +
             dsbFilePath + "\"}";
    }

    // Check if it's a valid DSB file by trying to load metadata
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize < 16) { // Minimum DSB header size
      return "{\"status\":\"error\",\"message\":\"Invalid DSB file: too "
             "small\"}";
    }

    // Create apps directory if it doesn't exist
    std::string appsDir = fileSystemRoot_ + "/apps";
    std::string installedDir = appsDir + "/installed";

#ifdef _WIN32
    CreateDirectoryA(appsDir.c_str(), NULL);
    CreateDirectoryA(installedDir.c_str(), NULL);
#else
    mkdir(appsDir.c_str(), 0755);
    mkdir(installedDir.c_str(), 0755);
#endif

    // Copy DSB file to installed directory
    std::string targetPath = installedDir + "/" + appId + ".dsb";
    std::ofstream target(targetPath, std::ios::binary);
    if (!target.is_open()) {
      return "{\"status\":\"error\",\"message\":\"Failed to create app file\"}";
    }

    target << file.rdbuf();
    file.close();
    target.close();

    // Update registry (simple JSON file)
    std::string registryPath = appsDir + "/registry.json";
    std::string registryContent = "[]"; // Default empty array

    // Try to read existing registry
    std::ifstream registryFile(registryPath);
    if (registryFile.is_open()) {
      std::string line;
      registryContent = "";
      while (std::getline(registryFile, line)) {
        registryContent += line + "\n";
      }
      registryFile.close();

      // Remove trailing newline and whitespace
      registryContent.erase(registryContent.find_last_not_of(" \t\n\r") + 1);
    }

    // Simple JSON manipulation (add new app entry)
    // For ESP32 compatibility, we avoid complex JSON libraries
    if (registryContent == "[]" || registryContent.empty()) {
      registryContent = "[{\"id\":\"" + appId + "\",\"file\":\"" + appId +
                        ".dsb\",\"installed\":\"" +
                        std::to_string(system_getTime()) + "\"}]";
    } else {
      // Remove closing bracket and add new entry
      size_t closeBracket = registryContent.find_last_of(']');
      if (closeBracket != std::string::npos) {
        std::string newEntry = ",{\"id\":\"" + appId + "\",\"file\":\"" +
                               appId + ".dsb\",\"installed\":\"" +
                               std::to_string(system_getTime()) + "\"}]";
        registryContent = registryContent.substr(0, closeBracket) + newEntry;
      }
    }

    // Write updated registry
    std::ofstream registryOut(registryPath);
    if (registryOut.is_open()) {
      registryOut << registryContent;
      registryOut.close();
    }

    console_log("App installed: " + appId + " from " + dsbFilePath);
    return "{\"status\":\"success\",\"appId\":\"" + appId + "\",\"file\":\"" +
           appId + ".dsb\"}";

  } catch (const std::exception &e) {
    console_error("App install error: " + std::string(e.what()));
    return "{\"status\":\"error\",\"message\":\"" + std::string(e.what()) +
           "\"}";
  }
}

std::string SDLPlatform::app_uninstall(const std::string &appId) {
  try {
    std::string appsDir = fileSystemRoot_ + "/apps";
    std::string installedDir = appsDir + "/installed";
    std::string appFilePath = installedDir + "/" + appId + ".dsb";

    // Check if app exists
    if (!file_exists(appFilePath)) {
      return "{\"status\":\"error\",\"message\":\"App not found: " + appId +
             "\"}";
    }

// Delete app file
#ifdef _WIN32
    if (!DeleteFileA(appFilePath.c_str())) {
      return "{\"status\":\"error\",\"message\":\"Failed to delete app file\"}";
    }
#else
    if (remove(appFilePath.c_str()) != 0) {
      return "{\"status\":\"error\",\"message\":\"Failed to delete app file\"}";
    }
#endif

    // Update registry (remove app entry)
    std::string registryPath = appsDir + "/registry.json";
    std::ifstream registryFile(registryPath);
    if (registryFile.is_open()) {
      std::string registryContent;
      std::string line;
      while (std::getline(registryFile, line)) {
        registryContent += line + "\n";
      }
      registryFile.close();

      // Simple removal: find and remove the app entry
      // This is a simplified implementation for cross-platform compatibility
      std::string searchPattern = "\"id\":\"" + appId + "\"";
      size_t pos = registryContent.find(searchPattern);
      if (pos != std::string::npos) {
        // Find the start and end of this JSON object
        size_t objStart = registryContent.rfind('{', pos);
        size_t objEnd = registryContent.find('}', pos);
        if (objStart != std::string::npos && objEnd != std::string::npos) {
          // Remove the object and any trailing comma
          std::string before = registryContent.substr(0, objStart);
          std::string after = registryContent.substr(objEnd + 1);

          // Clean up commas
          if (!before.empty() && before.back() == ',') {
            before.pop_back();
          } else if (!after.empty() && after.front() == ',') {
            after = after.substr(1);
          }

          registryContent = before + after;
        }
      }

      // Write updated registry
      std::ofstream registryOut(registryPath);
      if (registryOut.is_open()) {
        registryOut << registryContent;
        registryOut.close();
      }
    }

    console_log("App uninstalled: " + appId);
    return "{\"status\":\"success\",\"appId\":\"" + appId + "\"}";

  } catch (const std::exception &e) {
    console_error("App uninstall error: " + std::string(e.what()));
    return "{\"status\":\"error\",\"message\":\"" + std::string(e.what()) +
           "\"}";
  }
}

std::string SDLPlatform::app_list() {
  try {
    std::string appsDir = fileSystemRoot_ + "/apps";
    std::string registryPath = appsDir + "/registry.json";

    std::ifstream registryFile(registryPath);
    if (!registryFile.is_open()) {
      return "[]"; // Empty list if no registry exists
    }

    std::string registryContent;
    std::string line;
    while (std::getline(registryFile, line)) {
      registryContent += line + "\n";
    }
    registryFile.close();

    // Remove trailing whitespace
    registryContent.erase(registryContent.find_last_not_of(" \t\n\r") + 1);

    return registryContent.empty() ? "[]" : registryContent;

  } catch (const std::exception &e) {
    console_error("App list error: " + std::string(e.what()));
    return "[]";
  }
}

std::string SDLPlatform::app_getMetadata(const std::string &dsbFilePath) {
  try {
    std::string fullPath = fileSystemRoot_ + "/" + dsbFilePath;
    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
      return "{\"status\":\"error\",\"message\":\"DSB file not found\"}";
    }

    // Read file into memory
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char *>(data.data()), fileSize);
    file.close();

    // Try to deserialize and extract metadata
    try {
      compiler::BytecodeModule module =
          compiler::BytecodeModule::deserialize(data);

      std::string metadata = "{\"status\":\"success\",";
      metadata += "\"appName\":\"" + module.metadata.appName + "\",";
      metadata += "\"appVersion\":\"" + module.metadata.appVersion + "\",";
      metadata += "\"author\":\"" + module.metadata.author + "\",";
      metadata +=
          "\"heapSize\":" + std::to_string(module.metadata.heapSize) + ",";
      metadata +=
          "\"version\":" + std::to_string(module.metadata.version) + ",";
      metadata += "\"codeSize\":" + std::to_string(module.code.size()) + ",";
      metadata +=
          "\"constants\":" + std::to_string(module.constants.size()) + ",";
      metadata +=
          "\"functions\":" + std::to_string(module.functions.size()) + "}";

      return metadata;

    } catch (const std::exception &e) {
      return "{\"status\":\"error\",\"message\":\"Invalid DSB file: " +
             std::string(e.what()) + "\"}";
    }

  } catch (const std::exception &e) {
    console_error("App metadata error: " + std::string(e.what()));
    return "{\"status\":\"error\",\"message\":\"" + std::string(e.what()) +
           "\"}";
  }
}

std::string SDLPlatform::app_launch(const std::string &appId) {
  try {
    std::string appsDir = fileSystemRoot_ + "/apps";
    std::string installedDir = appsDir + "/installed";
    std::string appFilePath = installedDir + "/" + appId + ".dsb";

    // Check if app exists
    if (!file_exists(appFilePath)) {
      return "{\"status\":\"error\",\"message\":\"App not found: " + appId +
             "\"}";
    }

    console_log("App launch requested: " + appId + " at " + appFilePath);

    // For now, just return success - actual VM restart would be handled by the
    // dialOS kernel In a real implementation, this would:
    // 1. Save current VM state
    // 2. Load new DSB file
    // 3. Initialize new VM with the app
    return "{\"status\":\"success\",\"appId\":\"" + appId +
           "\",\"message\":\"Launch requested - VM restart required\"}";

  } catch (const std::exception &e) {
    console_error("App launch error: " + std::string(e.what()));
    return "{\"status\":\"error\",\"message\":\"" + std::string(e.what()) +
           "\"}";
  }
}

std::string SDLPlatform::app_validate(const std::string &dsbFilePath) {
  try {
    std::string fullPath = fileSystemRoot_ + "/" + dsbFilePath;
    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
      return "{\"status\":\"error\",\"message\":\"DSB file not found\"}";
    }

    // Read file into memory
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize < 16) {
      return "{\"status\":\"error\",\"message\":\"File too small to be valid "
             "DSB\"}";
    }

    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char *>(data.data()), fileSize);
    file.close();

    // Try to deserialize to validate structure
    try {
      compiler::BytecodeModule module =
          compiler::BytecodeModule::deserialize(data);

      // Basic validation checks
      if (module.metadata.appName.empty()) {
        return "{\"status\":\"error\",\"message\":\"Missing app name in "
               "metadata\"}";
      }

      if (module.code.empty()) {
        return "{\"status\":\"error\",\"message\":\"No bytecode found in DSB "
               "file\"}";
      }

      return "{\"status\":\"success\",\"message\":\"DSB file is "
             "valid\",\"size\":" +
             std::to_string(fileSize) + "}";

    } catch (const std::exception &e) {
      return "{\"status\":\"error\",\"message\":\"DSB validation failed: " +
             std::string(e.what()) + "\"}";
    }

  } catch (const std::exception &e) {
    console_error("App validation error: " + std::string(e.what()));
    return "{\"status\":\"error\",\"message\":\"" + std::string(e.what()) +
           "\"}";
  }
}

} // namespace vm
} // namespace dialos