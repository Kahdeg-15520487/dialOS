/**
 * Simple VM Test - Console-based VM execution
 * 
 * Tests the VM core with bytecode files
 */

#include "../include/vm_core.h"
#include "../include/vm_value.h"
#include "../include/platform.h"
#include "../include/bytecode.h"
#include <iostream>
#include <fstream>
#include <vector>

using namespace dialos;

// Simple console platform for testing
class ConsolePlatform : public vm::PlatformInterface {
public:
    void display_clear(uint32_t color) override {
        std::cout << "[Display] Clear: 0x" << std::hex << color << std::dec << std::endl;
    }
    
    void display_drawText(int x, int y, const std::string& text, 
                         uint32_t color, int size) override {
        std::cout << "[Display] Text at (" << x << "," << y << "): \"" 
                  << text << "\" (color=0x" << std::hex << color << std::dec 
                  << ", size=" << size << ")" << std::endl;
    }
    
    bool encoder_getButton() override {
        return false;
    }
    
    int encoder_getDelta() override {
        return 0;
    }
    
    uint32_t system_getTime() override {
        return 0; // Simple implementation
    }
    
    void system_sleep(uint32_t ms) override {
        std::cout << "[System] Sleep: " << ms << "ms" << std::endl;
    }
    
    void console_log(const std::string& msg) override {
        std::cout << "[Console] " << msg << std::endl;
    }
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
