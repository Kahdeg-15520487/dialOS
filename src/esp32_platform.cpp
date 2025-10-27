#include "esp32_platform.h"
#include "Encoder.h"
#include "kernel/kernel.h"
#include "kernel/system.h"
#include "kernel/ramfs.h"
#include "kernel/task.h"
#include <Arduino.h>
#include <M5Dial.h>
#include <Wire.h>
#include <vector>

using namespace dialos::vm;

// ===== Console Operations =====
void ESP32Platform::console_print(const std::string &message) {
  Serial.print(message.c_str());
}

void ESP32Platform::console_println(const std::string &message) {
  Serial.println(message.c_str());
}

void ESP32Platform::console_log(const std::string &message) {
  Serial.println(message.c_str());
  dialOS::Kernel::instance().getSystemServices()->log(dialOS::LogLevel::INFO, message.c_str());
}

void ESP32Platform::console_warn(const std::string &message) {
  Serial.print("[WARN] ");
  Serial.println(message.c_str());
  dialOS::Kernel::instance().getSystemServices()->log(dialOS::LogLevel::WARNING, message.c_str());
}

void ESP32Platform::console_error(const std::string &message) {
  Serial.print("[ERROR] ");
  Serial.println(message.c_str());
  dialOS::Kernel::instance().getSystemServices()->log(dialOS::LogLevel::ERROR, message.c_str());
}

// ===== Display Operations =====
void ESP32Platform::display_clear(uint32_t color) {
  M5Dial.Display.fillScreen(color);
  console_log("Display cleared: " + std::to_string(color));
}

void ESP32Platform::display_drawText(int x, int y, const std::string &text, uint32_t color,
                      int size) {
  M5Dial.Display.setTextSize(size);
  M5Dial.Display.setTextColor(color);
  M5Dial.Display.setCursor(x, y);
  M5Dial.Display.print(text.c_str());
}

void ESP32Platform::display_drawRect(int x, int y, int w, int h, uint32_t color, bool filled) {
  if (filled) {
    M5Dial.Display.fillRect(x, y, w, h, color);
  } else {
    M5Dial.Display.drawRect(x, y, w, h, color);
  }
}

void ESP32Platform::display_drawCircle(int x, int y, int r, uint32_t color, bool filled) {
  if (filled) {
    M5Dial.Display.fillCircle(x, y, r, color);
  } else {
    M5Dial.Display.drawCircle(x, y, r, color);
  }
}

void ESP32Platform::display_drawLine(int x1, int y1, int x2, int y2, uint32_t color) {
  M5Dial.Display.drawLine(x1, y1, x2, y2, color);
}

void ESP32Platform::display_drawPixel(int x, int y, uint32_t color) {
  M5Dial.Display.drawPixel(x, y, color);
}

void ESP32Platform::display_setBrightness(int level) {
  M5Dial.Display.setBrightness(level);
}

int ESP32Platform::display_getWidth() {
  return M5Dial.Display.width();
}

int ESP32Platform::display_getHeight() {
  return M5Dial.Display.height();
}

void ESP32Platform::display_setTitle(const std::string& title) {
  // Draw title at top of screen with background
  M5Dial.Display.fillRect(0, 0, M5Dial.Display.width(), 20, 0x0000); // Black background
  M5Dial.Display.setTextSize(1);
  M5Dial.Display.setTextColor(0xFFFF); // White text
  M5Dial.Display.setCursor(5, 5);
  M5Dial.Display.print(title.c_str());
}

void ESP32Platform::display_drawImage(int x, int y, const std::vector<uint8_t>& imageData) {
  // For now, implement as a simple bitmap drawing
  // Assumes imageData is in a simple format: width(2), height(2), RGB565 pixel data
  if (imageData.size() < 4) return;
  
  uint16_t width = (imageData[0] << 8) | imageData[1];
  uint16_t height = (imageData[2] << 8) | imageData[3];
  
  if (imageData.size() < 4 + (width * height * 2)) return; // RGB565 = 2 bytes per pixel
  
  // Draw the image pixel by pixel
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      int index = 4 + ((row * width + col) * 2);
      uint16_t color = (imageData[index] << 8) | imageData[index + 1];
      M5Dial.Display.drawPixel(x + col, y + row, color);
    }
  }
}

// ===== Encoder Operations =====
bool ESP32Platform::encoder_getButton() {
  return M5Dial.BtnA.isPressed();
}

int ESP32Platform::encoder_getDelta() {
  int delta = get_encoder();
  encoderPosition += delta;
  return delta;
}

int ESP32Platform::encoder_getPosition() {
  return encoderPosition;
}

void ESP32Platform::encoder_reset() {
  encoderPosition = 0;
}

// ===== System Operations =====
uint32_t ESP32Platform::system_getTime() {
  return millis();
}

void ESP32Platform::system_sleep(uint32_t ms) {
  // Sleep is now handled by VM state - this is a no-op
  // The VM will yield and resume after the specified time
  delay(ms);
}

void ESP32Platform::system_yield() {
  yield();
}

uint32_t ESP32Platform::system_getRTC() {
  // M5Dial uses BM8563 RTC - use M5.Rtc if available
  // For now, use the ESP32's built-in RTC approximation
  return millis() / 1000; // Return seconds since boot as fallback
}

void ESP32Platform::system_setRTC(uint32_t timestamp) {
  // M5Dial uses BM8563 RTC - use M5.Rtc if available
  // For now, this is a stub since we'd need to track offset from boot time
  // TODO: Implement proper BM8563 RTC setting with M5.Rtc API
}

// ===== Touch Operations =====
int ESP32Platform::touch_getX() {
  auto touch = M5Dial.Touch.getDetail();
  return touch.x;
}

int ESP32Platform::touch_getY() {
  auto touch = M5Dial.Touch.getDetail();
  return touch.y;
}

bool ESP32Platform::touch_isPressed() {
  // M5Dial.Touch.isPressed() or M5Dial.Touch.getDetail().state can be used
  // return M5Dial.Touch.isPressed();
  return false;
}

// ===== Memory Operations =====
int ESP32Platform::memory_getAvailable() {
  return ESP.getFreeHeap();
}

int ESP32Platform::memory_getUsage() {
  return ESP.getHeapSize() - ESP.getFreeHeap();
}

// ===== GPIO Operations =====
void ESP32Platform::gpio_pinMode(int pin, int mode) {
  pinMode(pin, mode);
}

void ESP32Platform::gpio_digitalWrite(int pin, int value) {
  digitalWrite(pin, value);
}

int ESP32Platform::gpio_digitalRead(int pin) {
  return digitalRead(pin);
}

void ESP32Platform::gpio_analogWrite(int pin, int value) {
  analogWrite(pin, value);
}

int ESP32Platform::gpio_analogRead(int pin) {
  return analogRead(pin);
}

// ===== I2C Operations =====
std::vector<int> ESP32Platform::i2c_scan() {
  std::vector<int> devices;
  for (int address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      devices.push_back(address);
    }
  }
  return devices;
}

bool ESP32Platform::i2c_write(int address, const std::vector<uint8_t>& data) {
  Wire.beginTransmission(address);
  for (uint8_t byte : data) {
    Wire.write(byte);
  }
  return Wire.endTransmission() == 0;
}

std::vector<uint8_t> ESP32Platform::i2c_read(int address, int length) {
  std::vector<uint8_t> data;
  Wire.requestFrom(address, length);
  while (Wire.available()) {
    data.push_back(Wire.read());
  }
  return data;
}

// ===== Buzzer Operations =====
void ESP32Platform::buzzer_beep(int frequency, int duration) {
  // M5Dial has a buzzer on GPIO3
  tone(3, frequency, duration);
}

void ESP32Platform::buzzer_playMelody(const std::vector<int>& notes) {
  // Play melody with notes array: frequency1, duration1, frequency2, duration2, ...
  for (size_t i = 0; i < notes.size(); i += 2) {
    if (i + 1 < notes.size()) {
      int frequency = notes[i];
      int duration = notes[i + 1];
      
      if (frequency > 0) {
        tone(3, frequency, duration);
      } else {
        // Rest (silence)
        noTone(3);
      }
      delay(duration);
      
      // Small gap between notes
      noTone(3);
      delay(50);
    }
  }
}

void ESP32Platform::buzzer_stop() {
  noTone(3);
}

// ===== RFID Operations =====
std::string ESP32Platform::rfid_read() {
  // M5Dial has WS1850S RFID reader on I2C
  // WS1850S I2C address is typically 0x24
  const int RFID_ADDRESS = 0x24;
  const int UID_LENGTH = 4; // 4 byte UID for most cards
  
  Wire.beginTransmission(RFID_ADDRESS);
  Wire.write(0x01); // Command to read UID
  if (Wire.endTransmission() != 0) {
    return ""; // Communication failed
  }
  
  delay(10); // Wait for processing
  
  Wire.requestFrom(RFID_ADDRESS, UID_LENGTH);
  if (Wire.available() < UID_LENGTH) {
    return ""; // No card present or read failed
  }
  
  std::string uid = "";
  for (int i = 0; i < UID_LENGTH; i++) {
    uint8_t byte = Wire.read();
    if (i > 0) uid += ":";
    if (byte < 0x10) uid += "0";
    uid += String(byte, HEX).c_str();
  }
  
  return uid;
}

bool ESP32Platform::rfid_isPresent() {
  // Try to communicate with WS1850S to check if card is present
  const int RFID_ADDRESS = 0x24;
  
  Wire.beginTransmission(RFID_ADDRESS);
  Wire.write(0x02); // Command to check presence
  if (Wire.endTransmission() != 0) {
    return false; // RFID module not responding
  }
  
  delay(5);
  
  Wire.requestFrom(RFID_ADDRESS, 1);
  if (Wire.available()) {
    uint8_t status = Wire.read();
    return (status == 0x01); // 0x01 indicates card present
  }
  
  return false;
}

// ===== Power Operations =====
void ESP32Platform::power_sleep() {
  // Put M5Dial into deep sleep mode
  // Wake up sources: button press, touch, or timer
  M5Dial.Power.deepSleep();
}

int ESP32Platform::power_getBatteryLevel() {
  // M5Dial has battery monitoring via AXP2101
  // Read battery voltage and convert to percentage
  int batteryLevel = M5Dial.Power.getBatteryLevel();
  if (batteryLevel >= 0) {
    return batteryLevel; // M5Dial library returns percentage directly
  }
  
  // Fallback: estimate from voltage if available
  float voltage = M5Dial.Power.getBatteryVoltage() / 1000.0; // Convert mV to V
  if (voltage > 4.1) return 100;
  else if (voltage > 4.0) return 90;
  else if (voltage > 3.9) return 70;
  else if (voltage > 3.8) return 50;
  else if (voltage > 3.7) return 30;
  else if (voltage > 3.6) return 10;
  else return 5;
}

bool ESP32Platform::power_isCharging() {
  // Check if external power is connected and charging
  return M5Dial.Power.isCharging();
}

// ===== File Operations using RAMFS =====
int ESP32Platform::file_open(const std::string& path, const std::string& mode) {
  // Convert string mode to RAMFS FileMode
  dialOS::FileMode fileMode;
  if (mode == "r" || mode == "read") {
    fileMode = dialOS::FileMode::READ;
  } else if (mode == "w" || mode == "write") {
    fileMode = dialOS::FileMode::WRITE;
  } else if (mode == "a" || mode == "append") {
    fileMode = dialOS::FileMode::APPEND;
  } else {
    // Invalid mode
    return -1;
  }
  
  // Get current task ID (using 0 as default for now - could be improved)
  uint32_t taskId = 0;
  auto* kernel = &dialOS::Kernel::instance();
  if (kernel && kernel->getScheduler()) {
    taskId = kernel->getScheduler()->getCurrentTaskId();
  }
  
  // Use RAMFS to open the file
  auto* ramfs = kernel->getRamFS();
  if (!ramfs) {
    return -1;
  }
  
  return ramfs->open(path.c_str(), fileMode, taskId);
}

std::string ESP32Platform::file_read(int handle, int size) {
  auto* ramfs = dialOS::Kernel::instance().getRamFS();
  if (!ramfs) {
    return "";
  }
  
  // Allocate buffer for reading
  std::vector<char> buffer(size);
  size_t bytesRead = ramfs->read(handle, buffer.data(), size);
  
  // Convert to string (ensure null termination)
  return std::string(buffer.data(), bytesRead);
}

int ESP32Platform::file_write(int handle, const std::string& data) {
  auto* ramfs = dialOS::Kernel::instance().getRamFS();
  if (!ramfs) {
    return -1;
  }
  
  size_t bytesWritten = ramfs->write(handle, data.c_str(), data.length());
  return static_cast<int>(bytesWritten);
}

void ESP32Platform::file_close(int handle) {
  auto* ramfs = dialOS::Kernel::instance().getRamFS();
  if (ramfs) {
    ramfs->close(handle);
  }
}

bool ESP32Platform::file_exists(const std::string& path) {
  auto* ramfs = dialOS::Kernel::instance().getRamFS();
  if (!ramfs) {
    return false;
  }
  
  return ramfs->exists(path.c_str());
}

bool ESP32Platform::file_delete(const std::string& path) {
  // Get current task ID
  uint32_t taskId = 0;
  auto* kernel = &dialOS::Kernel::instance();
  if (kernel && kernel->getScheduler()) {
    taskId = kernel->getScheduler()->getCurrentTaskId();
  }
  
  auto* ramfs = kernel->getRamFS();
  if (!ramfs) {
    return false;
  }
  
  return ramfs->remove(path.c_str(), taskId);
}

int ESP32Platform::file_size(const std::string& path) {
  auto* ramfs = dialOS::Kernel::instance().getRamFS();
  if (!ramfs) {
    return -1;
  }
  
  return static_cast<int>(ramfs->getSize(path.c_str()));
}

// ===== Directory Operations =====
std::vector<std::string> ESP32Platform::dir_list(const std::string& path) {
  auto* ramfs = dialOS::Kernel::instance().getRamFS();
  if (!ramfs) {
    return {};
  }
  
  // RAMFS is a flat file system, so we'll simulate directory behavior
  // For now, if path is "/" or empty, list all files
  // Later we could implement path prefix filtering
  
  const size_t MAX_FILES = 64;
  char* fileList[MAX_FILES];
  
  int fileCount = ramfs->listFiles(fileList, MAX_FILES);
  
  std::vector<std::string> result;
  for (int i = 0; i < fileCount; i++) {
    if (path.empty() || path == "/" || path == ".") {
      // Root directory - add all files
      result.push_back(std::string(fileList[i]));
    } else {
      // Check if file starts with the path prefix
      std::string fileName(fileList[i]);
      if (fileName.find(path) == 0) {
        // Extract the part after the path
        std::string remainder = fileName.substr(path.length());
        if (!remainder.empty() && remainder[0] == '/') {
          remainder = remainder.substr(1); // Remove leading slash
        }
        
        // Only add if there's no further '/' (i.e., it's directly in this directory)
        if (!remainder.empty() && remainder.find('/') == std::string::npos) {
          result.push_back(remainder);
        }
      }
    }
  }
  
  return result;
}

bool ESP32Platform::dir_create(const std::string& path) {
  // RAMFS is a flat file system, so directories don't really exist
  // We'll simulate this by creating a hidden marker file
  // For simplicity, we'll just return true (directories are virtual)
  return true;
}

bool ESP32Platform::dir_delete(const std::string& path) {
  // For RAMFS, we could delete all files that start with the path prefix
  auto* ramfs = dialOS::Kernel::instance().getRamFS();
  if (!ramfs) {
    return false;
  }
  
  // Get current task ID
  uint32_t taskId = 0;
  auto* kernel = &dialOS::Kernel::instance();
  if (kernel && kernel->getScheduler()) {
    taskId = kernel->getScheduler()->getCurrentTaskId();
  }
  
  // List all files and delete those that start with the path
  const size_t MAX_FILES = 64;
  char* fileList[MAX_FILES];
  int fileCount = ramfs->listFiles(fileList, MAX_FILES);
  
  bool deletedAny = false;
  for (int i = 0; i < fileCount; i++) {
    std::string fileName(fileList[i]);
    if (fileName.find(path) == 0) {
      if (ramfs->remove(fileName.c_str(), taskId)) {
        deletedAny = true;
      }
    }
  }
  
  return deletedAny;
}

bool ESP32Platform::dir_exists(const std::string& path) {
  // For RAMFS, we'll check if any files start with the path prefix
  auto* ramfs = dialOS::Kernel::instance().getRamFS();
  if (!ramfs) {
    return false;
  }
  
  // Root directory always exists
  if (path.empty() || path == "/" || path == ".") {
    return true;
  }
  
  // Check if any files start with this path
  const size_t MAX_FILES = 64;
  char* fileList[MAX_FILES];
  int fileCount = ramfs->listFiles(fileList, MAX_FILES);
  
  for (int i = 0; i < fileCount; i++) {
    std::string fileName(fileList[i]);
    if (fileName.find(path) == 0) {
      return true;
    }
  }
  
  return false;
}