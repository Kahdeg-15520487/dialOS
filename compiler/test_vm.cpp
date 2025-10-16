/**
 * Simple VM Test - Console-based VM execution
 * 
 * Tests the VM core with bytecode files
 */

#include "vm_core.h"
#include "vm_value.h"
#include "platform.h"
#include "bytecode.h"
#include <iostream>
#include <fstream>
#include <vector>

using namespace dialos;

// Simple console platform for testing
class ConsolePlatform : public vm::PlatformInterface {
public:
    // Console
    void console_print(const std::string& msg) override {
        std::cout << msg;
    }
    void console_log(const std::string& msg) override {
        std::cout << "[INFO] " << msg << std::endl;
    }
    void console_warn(const std::string& msg) override {
        std::cout << "[WARN] " << msg << std::endl;
    }
    void console_error(const std::string& msg) override {
        std::cerr << "[ERROR] " << msg << std::endl;
    }
    void console_clear() override {
        std::cout << "[Console] Clear" << std::endl;
    }
    
    // Display
    void display_clear(uint32_t color) override {
        std::cout << "[Display] Clear: 0x" << std::hex << color << std::dec << std::endl;
    }
    void display_drawText(int x, int y, const std::string& text, 
                         uint32_t color, int size) override {
        std::cout << "[Display] Text at (" << x << "," << y << "): \"" 
                  << text << "\" (color=0x" << std::hex << color << std::dec 
                  << ", size=" << size << ")" << std::endl;
    }
    void display_drawPixel(int /*x*/, int /*y*/, uint32_t /*color*/) override {}
    void display_drawLine(int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/, uint32_t /*color*/) override {}
    void display_drawRect(int /*x*/, int /*y*/, int /*w*/, int /*h*/, uint32_t /*color*/, bool /*filled*/) override {}
    void display_drawCircle(int /*x*/, int /*y*/, int /*radius*/, uint32_t /*color*/, bool /*filled*/) override {}
    void display_setBrightness(int /*brightness*/) override {}
    int display_getWidth() override { return 240; }
    int display_getHeight() override { return 240; }
    void display_setTitle(const std::string& /*title*/) override {}
    void display_drawImage(int /*x*/, int /*y*/, const std::vector<uint8_t>& /*imageData*/) override {}
    
    // Encoder
    bool encoder_getButton() override { return false; }
    int encoder_getDelta() override { return 0; }
    int encoder_getPosition() override { return 0; }
    void encoder_reset() override {}
    
    // System
    uint32_t system_getTime() override { return 0; }
    void system_sleep(uint32_t ms) override {
        std::cout << "[System] Sleep: " << ms << "ms" << std::endl;
    }
    uint32_t system_getRTC() override { return 0; }
    void system_setRTC(uint32_t /*timestamp*/) override {}
    void system_yield() override {}
    
    // Touch
    int touch_getX() override { return 0; }
    int touch_getY() override { return 0; }
    bool touch_isPressed() override { return false; }
    
    // RFID
    std::string rfid_read() override { return ""; }
    bool rfid_isPresent() override { return false; }
    
    // File (stubs)
    int file_open(const std::string& /*path*/, const std::string& /*mode*/) override { return -1; }
    std::string file_read(int /*handle*/, int /*size*/) override { return ""; }
    int file_write(int /*handle*/, const std::string& /*data*/) override { return -1; }
    void file_close(int /*handle*/) override {}
    bool file_exists(const std::string& /*path*/) override { return false; }
    bool file_delete(const std::string& /*path*/) override { return false; }
    int file_size(const std::string& /*path*/) override { return -1; }
    
    // GPIO (stubs)
    void gpio_pinMode(int /*pin*/, int /*mode*/) override {}
    void gpio_digitalWrite(int /*pin*/, int /*value*/) override {}
    int gpio_digitalRead(int /*pin*/) override { return 0; }
    void gpio_analogWrite(int /*pin*/, int /*value*/) override {}
    int gpio_analogRead(int /*pin*/) override { return 0; }
    
    // I2C (stubs)
    std::vector<int> i2c_scan() override { return {}; }
    bool i2c_write(int /*address*/, const std::vector<uint8_t>& /*data*/) override { return false; }
    std::vector<uint8_t> i2c_read(int /*address*/, int /*length*/) override { return {}; }
    
    // Buzzer (stubs)
    void buzzer_beep(int /*frequency*/, int /*duration*/) override {}
    void buzzer_playMelody(const std::vector<int>& /*notes*/) override {}
    void buzzer_stop() override {}
    
    // Timer (stubs)
    int timer_setTimeout(int /*ms*/) override { return -1; }
    int timer_setInterval(const vm::Value& /*callback*/, int /*ms*/) override { return -1; }
    void timer_clearTimeout(int /*id*/) override {}
    void timer_clearInterval(int /*id*/) override {}
    
    // Memory (stubs)
    int memory_getAvailable() override { return 0; }
    int memory_getUsage() override { return 0; }
    int memory_allocate(int /*size*/) override { return -1; }
    void memory_free(int /*handle*/) override {}
    
    // Directory (stubs)
    std::vector<std::string> dir_list(const std::string& /*path*/) override { return {}; }
    bool dir_create(const std::string& /*path*/) override { return false; }
    bool dir_delete(const std::string& /*path*/) override { return false; }
    bool dir_exists(const std::string& /*path*/) override { return false; }
    
    // Power (stubs)
    void power_sleep() override {}
    int power_getBatteryLevel() override { return 100; }
    bool power_isCharging() override { return false; }
    
    // App (stubs)
    void app_exit() override {}
    std::string app_getInfo() override { return "{}"; }
    
    // Storage (stubs)
    std::vector<std::string> storage_getMounted() override { return {}; }
    std::string storage_getInfo(const std::string& /*device*/) override { return "{}"; }
    
    // Sensor (stubs)
    int sensor_attach(const std::string& /*port*/, const std::string& /*type*/) override { return -1; }
    std::string sensor_read(int /*handle*/) override { return "{}"; }
    void sensor_detach(int /*handle*/) override {}
    
    // WiFi (stubs)
    bool wifi_connect(const std::string& /*ssid*/, const std::string& /*password*/) override { return false; }
    void wifi_disconnect() override {}
    std::string wifi_getStatus() override { return "{}"; }
    std::string wifi_getIP() override { return ""; }
    
    // IPC (stubs)
    bool ipc_send(const std::string& /*appId*/, const std::string& /*message*/) override { return false; }
    void ipc_broadcast(const std::string& /*message*/) override {}
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <bytecode.dsb>" << std::endl;
        return 1;
    }
    
    const char* filename = argv[1];
    
    // Load bytecode file
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file '" << filename << "'" << std::endl;
        return 1;
    }
    
    // Read entire file
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    file.close();
    
    std::cout << "=== dialScript VM Test ===" << std::endl;
    std::cout << "Loaded: " << filename << " (" << fileSize << " bytes)" << std::endl;
    std::cout << std::endl;
    
    // Deserialize bytecode
    compiler::BytecodeModule module;
    try {
        module = compiler::BytecodeModule::deserialize(data);
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to deserialize bytecode: " << e.what() << std::endl;
        return 1;
    }
    
    // Show metadata
    std::cout << "Metadata:" << std::endl;
    std::cout << "  App:      " << module.metadata.appName << " v" << module.metadata.appVersion << std::endl;
    std::cout << "  Author:   " << (module.metadata.author.empty() ? "(none)" : module.metadata.author) << std::endl;
    std::cout << "  Heap:     " << module.metadata.heapSize << " bytes" << std::endl;
    std::cout << "  Version:  " << module.metadata.version << std::endl;
    std::cout << std::endl;
    
    std::cout << "Resources:" << std::endl;
    std::cout << "  Constants:  " << module.constants.size() << std::endl;
    std::cout << "  Globals:    " << module.globals.size() << std::endl;
    std::cout << "  Functions:  " << module.functions.size() << std::endl;
    std::cout << "  Code size:  " << module.code.size() << " bytes" << std::endl;
    std::cout << std::endl;
    
    // Create VM
    vm::ValuePool pool(module.metadata.heapSize);
    ConsolePlatform platform;
    vm::VMState vm(module, pool, platform);
    
    std::cout << "=== Executing Bytecode ===" << std::endl;
    std::cout << std::endl;
    
    vm.reset();
    
    // Execute VM
    int cycles = 0;
    const int maxCycles = 10000;
    
    while (vm.isRunning() && cycles < maxCycles) {
        vm::VMResult result = vm.execute(100); // Execute 100 instructions at a time
        
        if (result == vm::VMResult::FINISHED) {
            std::cout << std::endl;
            std::cout << "=== Execution Finished ===" << std::endl;
            break;
        } else if (result == vm::VMResult::ERROR) {
            std::cout << std::endl;
            std::cerr << "=== Runtime Error ===" << std::endl;
            std::cerr << "Error: " << vm.getError() << std::endl;
            std::cerr << "PC: " << vm.getPC() << std::endl;
            std::cerr << "Stack size: " << vm.getStackSize() << std::endl;
            return 1;
        } else if (result == vm::VMResult::OUT_OF_MEMORY) {
            std::cout << std::endl;
            std::cerr << "=== Out of Memory ===" << std::endl;
            std::cerr << "Heap exhausted at PC: " << vm.getPC() << std::endl;
            return 1;
        }
        
        cycles++;
    }
    
    if (cycles >= maxCycles) {
        std::cerr << "Warning: Execution limit reached (" << maxCycles << " cycles)" << std::endl;
    }
    
    // Show final state
    std::cout << std::endl;
    std::cout << "Final State:" << std::endl;
    std::cout << "  PC: " << vm.getPC() << std::endl;
    std::cout << "  Stack size: " << vm.getStackSize() << std::endl;
    std::cout << "  Call stack depth: " << vm.getCallStack().size() << std::endl;
    std::cout << "  Heap used: " << pool.getAllocated() << "/" << pool.getHeapSize() << " bytes" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Global Variables:" << std::endl;
    const auto& globals = vm.getGlobals();
    for (const auto& pair : globals) {
        std::cout << "  " << pair.first << " = " << pair.second.toString() << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "=== Success ===" << std::endl;
    
    return 0;
}
