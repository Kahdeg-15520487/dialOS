#ifndef ESP32_PLATFORM_H
#define ESP32_PLATFORM_H

#include "vm/platform.h"
#include <vector>
#include <string>

namespace dialos {
namespace vm {

// ESP32 Platform implementation for M5 Dial
class ESP32Platform : public PlatformInterface {
private:
  int encoderPosition = 0;

public:
  // ===== Console Operations =====
  void console_print(const std::string &message) override;
  void console_println(const std::string &message) override;
  void console_log(const std::string &message) override;
  void console_warn(const std::string &message) override;
  void console_error(const std::string &message) override;

  // ===== Display Operations =====
  void display_clear(uint32_t color) override;
  void display_drawText(int x, int y, const std::string &text, uint32_t color,
                        int size) override;
  void display_drawRect(int x, int y, int w, int h, uint32_t color, bool filled) override;
  void display_drawCircle(int x, int y, int r, uint32_t color, bool filled) override;
  void display_drawLine(int x1, int y1, int x2, int y2, uint32_t color) override;
  void display_drawPixel(int x, int y, uint32_t color) override;
  void display_setBrightness(int level) override;
  int display_getWidth() override;
  int display_getHeight() override;

  // ===== Encoder Operations =====
  bool encoder_getButton() override;
  int encoder_getDelta() override;
  int encoder_getPosition() override;
  void encoder_reset() override;

  // ===== System Operations =====
  uint32_t system_getTime() override;
  void system_sleep(uint32_t ms) override;
  void system_yield() override;
  uint32_t system_getRTC() override;
  void system_setRTC(uint32_t timestamp) override;

  // ===== Touch Operations =====
  int touch_getX() override;
  int touch_getY() override;
  bool touch_isPressed() override;

  // ===== Memory Operations =====
  int memory_getAvailable() override;
  int memory_getUsage() override;

  // ===== GPIO Operations =====
  void gpio_pinMode(int pin, int mode) override;
  void gpio_digitalWrite(int pin, int value) override;
  int gpio_digitalRead(int pin) override;
  void gpio_analogWrite(int pin, int value) override;
  int gpio_analogRead(int pin) override;

  // ===== I2C Operations =====
  std::vector<int> i2c_scan() override;
  bool i2c_write(int address, const std::vector<uint8_t>& data) override;
  std::vector<uint8_t> i2c_read(int address, int length) override;

  // ===== Buzzer Operations =====
  void buzzer_beep(int frequency, int duration) override;
  void buzzer_stop() override;

  // ===== RFID Operations =====
  std::string rfid_read() override;
  bool rfid_isPresent() override;

  // ===== Power Operations =====
  void power_sleep() override;
  int power_getBatteryLevel() override;
  bool power_isCharging() override;

  // ===== File Operations =====
  int file_open(const std::string& path, const std::string& mode) override;
  std::string file_read(int handle, int size) override;
  int file_write(int handle, const std::string& data) override;
  void file_close(int handle) override;
  bool file_exists(const std::string& path) override;
  bool file_delete(const std::string& path) override;
  int file_size(const std::string& path) override;

  // ===== Directory Operations =====
  std::vector<std::string> dir_list(const std::string& path) override;
  bool dir_create(const std::string& path) override;
  bool dir_delete(const std::string& path) override;
  bool dir_exists(const std::string& path) override;
};

} // namespace vm
} // namespace dialos

#endif // ESP32_PLATFORM_H