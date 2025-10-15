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
#include <memory>

namespace dialos
{
    namespace vm
    {
        // Forward declarations to avoid circular dependencies
        class VMState;
        struct Value;

        // Native function IDs
        // Organization: High byte = namespace, Low byte = function within namespace
        enum class NativeFunctionID : uint16_t
        {
            // Console namespace (0x00xx)
            CONSOLE_PRINT = 0x0000,
            CONSOLE_PRINTLN = 0x0001,
            CONSOLE_LOG = 0x0002,
            CONSOLE_WARN = 0x0003,
            CONSOLE_ERROR = 0x0004,
            CONSOLE_CLEAR = 0x0005,

            // Display namespace (0x01xx)
            DISPLAY_CLEAR = 0x0100,
            DISPLAY_DRAW_TEXT = 0x0101,
            DISPLAY_DRAW_RECT = 0x0102,
            DISPLAY_DRAW_CIRCLE = 0x0103,
            DISPLAY_DRAW_LINE = 0x0104,
            DISPLAY_DRAW_PIXEL = 0x0105,
            DISPLAY_SET_BRIGHTNESS = 0x0106,
            DISPLAY_GET_WIDTH = 0x0107,
            DISPLAY_GET_HEIGHT = 0x0108,
            DISPLAY_SET_TITLE = 0x0109,
            DISPLAY_GET_SIZE = 0x010A,
            DISPLAY_DRAW_IMAGE = 0x010B,

            // Encoder namespace (0x02xx)
            ENCODER_GET_BUTTON = 0x0200,
            ENCODER_GET_DELTA = 0x0201,
            ENCODER_GET_POSITION = 0x0202,
            ENCODER_RESET = 0x0203,
            ENCODER_ON_TURN = 0x0204,
            ENCODER_ON_BUTTON = 0x0205,

            // System namespace (0x03xx)
            SYSTEM_GET_TIME = 0x0300,
            SYSTEM_SLEEP = 0x0301,
            SYSTEM_YIELD = 0x0302,
            SYSTEM_GET_RTC = 0x0303,
            SYSTEM_SET_RTC = 0x0304,

            // Touch namespace (0x04xx)
            TOUCH_GET_X = 0x0400,
            TOUCH_GET_Y = 0x0401,
            TOUCH_IS_PRESSED = 0x0402,
            TOUCH_GET_POSITION = 0x0403,
            TOUCH_ON_PRESS = 0x0404,
            TOUCH_ON_RELEASE = 0x0405,
            TOUCH_ON_DRAG = 0x0406,

            // RFID namespace (0x05xx)
            RFID_READ = 0x0500,
            RFID_IS_PRESENT = 0x0501,

            // File namespace (0x06xx)
            FILE_OPEN = 0x0600,
            FILE_READ = 0x0601,
            FILE_WRITE = 0x0602,
            FILE_CLOSE = 0x0603,
            FILE_EXISTS = 0x0604,
            FILE_DELETE = 0x0605,
            FILE_SIZE = 0x0606,

            // Directory namespace (0x07xx)
            DIR_LIST = 0x0700,
            DIR_CREATE = 0x0701,
            DIR_DELETE = 0x0702,
            DIR_EXISTS = 0x0703,

            // GPIO namespace (0x08xx)
            GPIO_PIN_MODE = 0x0800,
            GPIO_DIGITAL_WRITE = 0x0801,
            GPIO_DIGITAL_READ = 0x0802,
            GPIO_ANALOG_WRITE = 0x0803,
            GPIO_ANALOG_READ = 0x0804,

            // I2C namespace (0x09xx)
            I2C_SCAN = 0x0900,
            I2C_WRITE = 0x0901,
            I2C_READ = 0x0902,

            // Buzzer namespace (0x0Axx)
            BUZZER_BEEP = 0x0A00,
            BUZZER_PLAY_MELODY = 0x0A01,
            BUZZER_STOP = 0x0A02,

            // Timer namespace (0x0Bxx)
            TIMER_SET_TIMEOUT = 0x0B00,
            TIMER_SET_INTERVAL = 0x0B01,
            TIMER_CLEAR_TIMEOUT = 0x0B02,
            TIMER_CLEAR_INTERVAL = 0x0B03,

            // Memory namespace (0x0Cxx)
            MEMORY_GET_AVAILABLE = 0x0C00,
            MEMORY_GET_USAGE = 0x0C01,
            MEMORY_ALLOCATE = 0x0C02,
            MEMORY_FREE = 0x0C03,

            // Power namespace (0x0Dxx)
            POWER_SLEEP = 0x0D00,
            POWER_GET_BATTERY_LEVEL = 0x0D01,
            POWER_IS_CHARGING = 0x0D02,

            // App namespace (0x0Exx)
            APP_EXIT = 0x0E00,
            APP_GET_INFO = 0x0E01,
            APP_ON_LOAD = 0x0E02,
            APP_ON_SUSPEND = 0x0E03,
            APP_ON_RESUME = 0x0E04,
            APP_ON_UNLOAD = 0x0E05,

            // Storage namespace (0x0Fxx)
            STORAGE_GET_MOUNTED = 0x0F00,
            STORAGE_GET_INFO = 0x0F01,

            // Sensor namespace (0x10xx)
            SENSOR_ATTACH = 0x1000,
            SENSOR_READ = 0x1001,
            SENSOR_DETACH = 0x1002,

            // WiFi namespace (0x11xx)
            WIFI_CONNECT = 0x1100,
            WIFI_DISCONNECT = 0x1101,
            WIFI_GET_STATUS = 0x1102,
            WIFI_GET_IP = 0x1103,

            // IPC namespace (0x12xx)
            IPC_SEND = 0x1200,
            IPC_BROADCAST = 0x1201,

            UNKNOWN = 0xFFFF
        };

        // Helper function to map function name to ID
        inline NativeFunctionID getNativeFunctionID(const std::string &name)
        {
            // Console functions
            if (name == "print")
                return NativeFunctionID::CONSOLE_PRINT;
            if (name == "println")
                return NativeFunctionID::CONSOLE_PRINTLN;
            if (name == "log")
                return NativeFunctionID::CONSOLE_LOG;
            if (name == "warn")
                return NativeFunctionID::CONSOLE_WARN;
            if (name == "error")
                return NativeFunctionID::CONSOLE_ERROR;
            if (name == "clear")
                return NativeFunctionID::CONSOLE_CLEAR;

            // Display functions
            if (name == "clear")
                return NativeFunctionID::DISPLAY_CLEAR;
            if (name == "drawText")
                return NativeFunctionID::DISPLAY_DRAW_TEXT;
            if (name == "drawRect")
                return NativeFunctionID::DISPLAY_DRAW_RECT;
            if (name == "drawCircle")
                return NativeFunctionID::DISPLAY_DRAW_CIRCLE;
            if (name == "drawLine")
                return NativeFunctionID::DISPLAY_DRAW_LINE;
            if (name == "drawPixel")
                return NativeFunctionID::DISPLAY_DRAW_PIXEL;
            if (name == "setBrightness")
                return NativeFunctionID::DISPLAY_SET_BRIGHTNESS;
            if (name == "getWidth")
                return NativeFunctionID::DISPLAY_GET_WIDTH;
            if (name == "getHeight")
                return NativeFunctionID::DISPLAY_GET_HEIGHT;
            if (name == "setTitle")
                return NativeFunctionID::DISPLAY_SET_TITLE;
            if (name == "getSize")
                return NativeFunctionID::DISPLAY_GET_SIZE;
            if (name == "drawImage")
                return NativeFunctionID::DISPLAY_DRAW_IMAGE;

            // Encoder functions
            if (name == "getButton")
                return NativeFunctionID::ENCODER_GET_BUTTON;
            if (name == "getDelta")
                return NativeFunctionID::ENCODER_GET_DELTA;
            if (name == "getPosition")
                return NativeFunctionID::ENCODER_GET_POSITION;
            if (name == "reset")
                return NativeFunctionID::ENCODER_RESET;
            if (name == "onTurn")
                return NativeFunctionID::ENCODER_ON_TURN;
            if (name == "onButton")
                return NativeFunctionID::ENCODER_ON_BUTTON;

            // System functions
            if (name == "getTime")
                return NativeFunctionID::SYSTEM_GET_TIME;
            if (name == "sleep")
                return NativeFunctionID::SYSTEM_SLEEP;
            if (name == "yield")
                return NativeFunctionID::SYSTEM_YIELD;
            if (name == "getRTC")
                return NativeFunctionID::SYSTEM_GET_RTC;
            if (name == "setRTC")
                return NativeFunctionID::SYSTEM_SET_RTC;

            // Touch functions
            if (name == "getX")
                return NativeFunctionID::TOUCH_GET_X;
            if (name == "getY")
                return NativeFunctionID::TOUCH_GET_Y;
            if (name == "isPressed")
                return NativeFunctionID::TOUCH_IS_PRESSED;
            if (name == "getPosition")
                return NativeFunctionID::TOUCH_GET_POSITION;
            if (name == "onPress")
                return NativeFunctionID::TOUCH_ON_PRESS;
            if (name == "onRelease")
                return NativeFunctionID::TOUCH_ON_RELEASE;
            if (name == "onDrag")
                return NativeFunctionID::TOUCH_ON_DRAG;

            // RFID functions
            if (name == "read")
                return NativeFunctionID::RFID_READ;
            if (name == "isPresent")
                return NativeFunctionID::RFID_IS_PRESENT;

            // File functions
            if (name == "open")
                return NativeFunctionID::FILE_OPEN;
            if (name == "read")
                return NativeFunctionID::FILE_READ;
            if (name == "write")
                return NativeFunctionID::FILE_WRITE;
            if (name == "close")
                return NativeFunctionID::FILE_CLOSE;
            if (name == "exists")
                return NativeFunctionID::FILE_EXISTS;
            if (name == "delete")
                return NativeFunctionID::FILE_DELETE;
            if (name == "size")
                return NativeFunctionID::FILE_SIZE;

            // Directory functions
            if (name == "list")
                return NativeFunctionID::DIR_LIST;
            if (name == "create")
                return NativeFunctionID::DIR_CREATE;
            if (name == "delete")
                return NativeFunctionID::DIR_DELETE;
            if (name == "exists")
                return NativeFunctionID::DIR_EXISTS;

            // GPIO functions
            if (name == "pinMode")
                return NativeFunctionID::GPIO_PIN_MODE;
            if (name == "digitalWrite")
                return NativeFunctionID::GPIO_DIGITAL_WRITE;
            if (name == "digitalRead")
                return NativeFunctionID::GPIO_DIGITAL_READ;
            if (name == "analogWrite")
                return NativeFunctionID::GPIO_ANALOG_WRITE;
            if (name == "analogRead")
                return NativeFunctionID::GPIO_ANALOG_READ;

            // I2C functions
            if (name == "scan")
                return NativeFunctionID::I2C_SCAN;
            if (name == "write")
                return NativeFunctionID::I2C_WRITE;
            if (name == "read")
                return NativeFunctionID::I2C_READ;

            // Buzzer functions
            if (name == "beep")
                return NativeFunctionID::BUZZER_BEEP;
            if (name == "playMelody")
                return NativeFunctionID::BUZZER_PLAY_MELODY;
            if (name == "stop")
                return NativeFunctionID::BUZZER_STOP;

            // Timer functions
            if (name == "setTimeout")
                return NativeFunctionID::TIMER_SET_TIMEOUT;
            if (name == "setInterval")
                return NativeFunctionID::TIMER_SET_INTERVAL;
            if (name == "clearTimeout")
                return NativeFunctionID::TIMER_CLEAR_TIMEOUT;
            if (name == "clearInterval")
                return NativeFunctionID::TIMER_CLEAR_INTERVAL;

            // Memory functions
            if (name == "getAvailable")
                return NativeFunctionID::MEMORY_GET_AVAILABLE;
            if (name == "getUsage")
                return NativeFunctionID::MEMORY_GET_USAGE;
            if (name == "allocate")
                return NativeFunctionID::MEMORY_ALLOCATE;
            if (name == "free")
                return NativeFunctionID::MEMORY_FREE;

            // Power functions
            if (name == "sleep")
                return NativeFunctionID::POWER_SLEEP;
            if (name == "getBatteryLevel")
                return NativeFunctionID::POWER_GET_BATTERY_LEVEL;
            if (name == "isCharging")
                return NativeFunctionID::POWER_IS_CHARGING;

            // App functions
            if (name == "exit")
                return NativeFunctionID::APP_EXIT;
            if (name == "getInfo")
                return NativeFunctionID::APP_GET_INFO;
            if (name == "onLoad")
                return NativeFunctionID::APP_ON_LOAD;
            if (name == "onSuspend")
                return NativeFunctionID::APP_ON_SUSPEND;
            if (name == "onResume")
                return NativeFunctionID::APP_ON_RESUME;
            if (name == "onUnload")
                return NativeFunctionID::APP_ON_UNLOAD;

            // Storage functions
            if (name == "getMounted")
                return NativeFunctionID::STORAGE_GET_MOUNTED;
            if (name == "getInfo")
                return NativeFunctionID::STORAGE_GET_INFO;

            // Sensor functions
            if (name == "attach")
                return NativeFunctionID::SENSOR_ATTACH;
            if (name == "read")
                return NativeFunctionID::SENSOR_READ;
            if (name == "detach")
                return NativeFunctionID::SENSOR_DETACH;

            // WiFi functions
            if (name == "connect")
                return NativeFunctionID::WIFI_CONNECT;
            if (name == "disconnect")
                return NativeFunctionID::WIFI_DISCONNECT;
            if (name == "getStatus")
                return NativeFunctionID::WIFI_GET_STATUS;
            if (name == "getIP")
                return NativeFunctionID::WIFI_GET_IP;

            // IPC functions
            if (name == "send")
                return NativeFunctionID::IPC_SEND;
            if (name == "broadcast")
                return NativeFunctionID::IPC_BROADCAST;

            return NativeFunctionID::UNKNOWN;
        }

        // Get function name from ID (for debugging/disassembly)
        inline const char *getNativeFunctionName(NativeFunctionID id)
        {
            switch (id)
            {
            // Console
            case NativeFunctionID::CONSOLE_PRINT:
                return "print";
            case NativeFunctionID::CONSOLE_PRINTLN:
                return "println";
            case NativeFunctionID::CONSOLE_LOG:
                return "log";
            case NativeFunctionID::CONSOLE_WARN:
                return "warn";
            case NativeFunctionID::CONSOLE_ERROR:
                return "error";
            case NativeFunctionID::CONSOLE_CLEAR:
                return "clear";

            // Display
            case NativeFunctionID::DISPLAY_CLEAR:
                return "clear";
            case NativeFunctionID::DISPLAY_DRAW_TEXT:
                return "drawText";
            case NativeFunctionID::DISPLAY_DRAW_RECT:
                return "drawRect";
            case NativeFunctionID::DISPLAY_DRAW_CIRCLE:
                return "drawCircle";
            case NativeFunctionID::DISPLAY_DRAW_LINE:
                return "drawLine";
            case NativeFunctionID::DISPLAY_DRAW_PIXEL:
                return "drawPixel";
            case NativeFunctionID::DISPLAY_SET_BRIGHTNESS:
                return "setBrightness";
            case NativeFunctionID::DISPLAY_GET_WIDTH:
                return "getWidth";
            case NativeFunctionID::DISPLAY_GET_HEIGHT:
                return "getHeight";
            case NativeFunctionID::DISPLAY_SET_TITLE:
                return "setTitle";
            case NativeFunctionID::DISPLAY_GET_SIZE:
                return "getSize";
            case NativeFunctionID::DISPLAY_DRAW_IMAGE:
                return "drawImage";

            // Encoder
            case NativeFunctionID::ENCODER_GET_BUTTON:
                return "getButton";
            case NativeFunctionID::ENCODER_GET_DELTA:
                return "getDelta";
            case NativeFunctionID::ENCODER_GET_POSITION:
                return "getPosition";
            case NativeFunctionID::ENCODER_RESET:
                return "reset";

            // System
            case NativeFunctionID::SYSTEM_GET_TIME:
                return "getTime";
            case NativeFunctionID::SYSTEM_SLEEP:
                return "sleep";
            case NativeFunctionID::SYSTEM_YIELD:
                return "yield";
            case NativeFunctionID::SYSTEM_GET_RTC:
                return "getRTC";
            case NativeFunctionID::SYSTEM_SET_RTC:
                return "setRTC";

            // Touch
            case NativeFunctionID::TOUCH_GET_X:
                return "getX";
            case NativeFunctionID::TOUCH_GET_Y:
                return "getY";
            case NativeFunctionID::TOUCH_IS_PRESSED:
                return "isPressed";
            case NativeFunctionID::TOUCH_GET_POSITION:
                return "getPosition";

            // RFID
            case NativeFunctionID::RFID_READ:
                return "read";
            case NativeFunctionID::RFID_IS_PRESENT:
                return "isPresent";

            // File
            case NativeFunctionID::FILE_OPEN:
                return "open";
            case NativeFunctionID::FILE_READ:
                return "read";
            case NativeFunctionID::FILE_WRITE:
                return "write";
            case NativeFunctionID::FILE_CLOSE:
                return "close";
            case NativeFunctionID::FILE_EXISTS:
                return "exists";
            case NativeFunctionID::FILE_DELETE:
                return "delete";
            case NativeFunctionID::FILE_SIZE:
                return "size";

            // Directory
            case NativeFunctionID::DIR_LIST:
                return "list";
            case NativeFunctionID::DIR_CREATE:
                return "create";
            case NativeFunctionID::DIR_DELETE:
                return "delete";
            case NativeFunctionID::DIR_EXISTS:
                return "exists";

            // GPIO
            case NativeFunctionID::GPIO_PIN_MODE:
                return "pinMode";
            case NativeFunctionID::GPIO_DIGITAL_WRITE:
                return "digitalWrite";
            case NativeFunctionID::GPIO_DIGITAL_READ:
                return "digitalRead";
            case NativeFunctionID::GPIO_ANALOG_WRITE:
                return "analogWrite";
            case NativeFunctionID::GPIO_ANALOG_READ:
                return "analogRead";

            // I2C
            case NativeFunctionID::I2C_SCAN:
                return "scan";
            case NativeFunctionID::I2C_WRITE:
                return "write";
            case NativeFunctionID::I2C_READ:
                return "read";

            // Buzzer
            case NativeFunctionID::BUZZER_BEEP:
                return "beep";
            case NativeFunctionID::BUZZER_PLAY_MELODY:
                return "playMelody";
            case NativeFunctionID::BUZZER_STOP:
                return "stop";

            // Timer
            case NativeFunctionID::TIMER_SET_TIMEOUT:
                return "setTimeout";
            case NativeFunctionID::TIMER_SET_INTERVAL:
                return "setInterval";
            case NativeFunctionID::TIMER_CLEAR_TIMEOUT:
                return "clearTimeout";
            case NativeFunctionID::TIMER_CLEAR_INTERVAL:
                return "clearInterval";

            // Memory
            case NativeFunctionID::MEMORY_GET_AVAILABLE:
                return "getAvailable";
            case NativeFunctionID::MEMORY_GET_USAGE:
                return "getUsage";
            case NativeFunctionID::MEMORY_ALLOCATE:
                return "allocate";
            case NativeFunctionID::MEMORY_FREE:
                return "free";

            // Power
            case NativeFunctionID::POWER_SLEEP:
                return "sleep";
            case NativeFunctionID::POWER_GET_BATTERY_LEVEL:
                return "getBatteryLevel";
            case NativeFunctionID::POWER_IS_CHARGING:
                return "isCharging";

            // App
            case NativeFunctionID::APP_EXIT:
                return "exit";
            case NativeFunctionID::APP_GET_INFO:
                return "getInfo";

            // Storage
            case NativeFunctionID::STORAGE_GET_MOUNTED:
                return "getMounted";
            case NativeFunctionID::STORAGE_GET_INFO:
                return "getInfo";

            // Sensor
            case NativeFunctionID::SENSOR_ATTACH:
                return "attach";
            case NativeFunctionID::SENSOR_READ:
                return "read";
            case NativeFunctionID::SENSOR_DETACH:
                return "detach";

            // WiFi
            case NativeFunctionID::WIFI_CONNECT:
                return "connect";
            case NativeFunctionID::WIFI_DISCONNECT:
                return "disconnect";
            case NativeFunctionID::WIFI_GET_STATUS:
                return "getStatus";
            case NativeFunctionID::WIFI_GET_IP:
                return "getIP";

            // IPC
            case NativeFunctionID::IPC_SEND:
                return "send";
            case NativeFunctionID::IPC_BROADCAST:
                return "broadcast";

            default:
                return "unknown";
            }
        }

        // Platform abstraction interface
        class PlatformInterface
        {
        public:
            PlatformInterface();
            virtual ~PlatformInterface();

            // ===== Console Operations =====
            virtual void console_print(const std::string &msg) = 0;
            virtual void console_println(const std::string &msg) { console_print(msg + "\n"); }
            virtual void console_log(const std::string &msg) { console_print("[INFO] " + msg); }
            virtual void console_warn(const std::string &msg) { console_log("[WARN] " + msg); }
            virtual void console_error(const std::string &msg) { console_log("[ERROR] " + msg); }
            virtual void console_clear() {}

            // ===== Display Operations =====
            virtual void display_clear(uint32_t color) = 0;
            virtual void display_drawText(int x, int y, const std::string &text,
                                          uint32_t color, int size) = 0;
            virtual void display_drawRect(int x, int y, int w, int h, uint32_t color, bool filled) = 0;
            virtual void display_drawCircle(int x, int y, int r, uint32_t color, bool filled) = 0;
            virtual void display_drawLine(int x1, int y1, int x2, int y2, uint32_t color) = 0;
            virtual void display_drawPixel(int x, int y, uint32_t color) = 0;
            virtual void display_setBrightness(int level) = 0;
            virtual int display_getWidth() = 0;
            virtual int display_getHeight() = 0;
            virtual void display_setTitle(const std::string & /*title*/) {}
            virtual void display_drawImage(int /*x*/, int /*y*/, const std::vector<uint8_t> & /*imageData*/) {}

            // ===== Encoder Operations =====
            virtual bool encoder_getButton() = 0;
            virtual int encoder_getDelta() = 0;
            virtual int encoder_getPosition() { return 0; }
            virtual void encoder_reset() {}

            // ===== System Operations =====
            virtual uint32_t system_getTime() = 0;
            virtual void system_sleep(uint32_t ms) = 0;
            virtual void system_yield() {}
            virtual uint32_t system_getRTC() { return 0; }
            virtual void system_setRTC(uint32_t /*timestamp*/) {}

            // ===== Touch Operations =====
            virtual int touch_getX() { return 0; }
            virtual int touch_getY() { return 0; }
            virtual bool touch_isPressed() { return false; }

            // ===== RFID Operations =====
            virtual std::string rfid_read() { return ""; }
            virtual bool rfid_isPresent() { return false; }

            // ===== File Operations =====
            virtual int file_open(const std::string & /*path*/, const std::string & /*mode*/) { return -1; }
            virtual std::string file_read(int /*handle*/, int /*size*/) { return ""; }
            virtual int file_write(int /*handle*/, const std::string & /*data*/) { return -1; }
            virtual void file_close(int /*handle*/) {}
            virtual bool file_exists(const std::string & /*path*/) { return false; }
            virtual bool file_delete(const std::string & /*path*/) { return false; }
            virtual int file_size(const std::string & /*path*/) { return -1; }

            // ===== Directory Operations =====
            virtual std::vector<std::string> dir_list(const std::string & /*path*/) { return {}; }
            virtual bool dir_create(const std::string & /*path*/) { return false; }
            virtual bool dir_delete(const std::string & /*path*/) { return false; }
            virtual bool dir_exists(const std::string & /*path*/) { return false; }

            // ===== GPIO Operations =====
            virtual void gpio_pinMode(int /*pin*/, int /*mode*/) {}
            virtual void gpio_digitalWrite(int /*pin*/, int /*value*/) {}
            virtual int gpio_digitalRead(int /*pin*/) { return 0; }
            virtual void gpio_analogWrite(int /*pin*/, int /*value*/) {}
            virtual int gpio_analogRead(int /*pin*/) { return 0; }

            // ===== I2C Operations =====
            virtual std::vector<int> i2c_scan() { return {}; }
            virtual bool i2c_write(int /*address*/, const std::vector<uint8_t> & /*data*/) { return false; }
            virtual std::vector<uint8_t> i2c_read(int /*address*/, int /*length*/) { return {}; }

            // ===== Buzzer Operations =====
            virtual void buzzer_beep(int /*frequency*/, int /*duration*/) {}
            virtual void buzzer_playMelody(const std::vector<int> & /*notes*/) {}
            virtual void buzzer_stop() {}

            // ===== Timer Operations =====
            virtual int timer_setTimeout(int /*ms*/) { return -1; }
            virtual int timer_setInterval(int /*ms*/) { return -1; }
            virtual void timer_clearTimeout(int /*id*/) {}
            virtual void timer_clearInterval(int /*id*/) {}

            // ===== Memory Operations =====
            virtual int memory_getAvailable() { return 0; }
            virtual int memory_getUsage() { return 0; }
            virtual int memory_allocate(int /*size*/) { return -1; }
            virtual void memory_free(int /*handle*/) {}

            // ===== Power Operations =====
            virtual void power_sleep() {}
            virtual int power_getBatteryLevel() { return 100; }
            virtual bool power_isCharging() { return false; }

            // ===== App Operations =====
            virtual void app_exit() {}
            virtual std::string app_getInfo() { return "{}"; }

            // ===== Storage Operations =====
            virtual std::vector<std::string> storage_getMounted() { return {}; }
            virtual std::string storage_getInfo(const std::string & /*device*/) { return "{}"; }

            // ===== Sensor Operations =====
            virtual int sensor_attach(const std::string & /*port*/, const std::string & /*type*/) { return -1; }
            virtual std::string sensor_read(int /*handle*/) { return "{}"; }
            virtual void sensor_detach(int /*handle*/) {}

            // ===== WiFi Operations =====
            virtual bool wifi_connect(const std::string & /*ssid*/, const std::string & /*password*/) { return false; }
            virtual void wifi_disconnect() {}
            virtual std::string wifi_getStatus() { return "{}"; }
            virtual std::string wifi_getIP() { return ""; }

            // ===== IPC Operations =====
            virtual bool ipc_send(const std::string & /*appId*/, const std::string & /*message*/) { return false; }
            virtual void ipc_broadcast(const std::string & /*message*/) {}

            // ===== Callback System =====
            /**
             * Set the VM instance for callback invocation
             * Must be called during VM initialization before callbacks can be invoked
             */
            void setVM(VMState* vm) { vm_ = vm; }

            /**
             * Register a callback function for an event
             * @param eventName The name of the event (e.g., "encoder.onTurn", "touch.onPress")
             * @param callback The function value to call when event occurs
             */
            void registerCallback(const std::string& eventName, const Value& callback);

            /**
             * Get a registered callback
             * @param eventName The name of the event
             * @return Pointer to the callback Value, or nullptr if not registered
             */
            const Value* getCallback(const std::string& eventName) const;

            /**
             * Invoke a callback immediately with given arguments
             * Uses immediate invocation (no event queue) as per minimal design
             * @param eventName The name of the event
             * @param args Vector of argument values to pass to callback
             * @return true if callback was invoked, false if not registered or invocation failed
             */
            bool invokeCallback(const std::string& eventName, const std::vector<Value>& args);

        protected:
            // VM reference for callback invocation
            VMState* vm_ = nullptr;

            // Callback registry: eventName -> callback function
            // Using unique_ptr to avoid needing complete Value type in header
            struct CallbackRegistry;
            std::unique_ptr<CallbackRegistry> callbacks_;
        };

    } // namespace vm
} // namespace dialos

#endif // DIALOS_VM_PLATFORM_H
