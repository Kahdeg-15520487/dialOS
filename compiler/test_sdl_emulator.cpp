/**
 * SDL dialOS Emulator
 * 
 * Complete M5 Dial hardware emulation using SDL2
 */

#include "vm_core.h"
#include "vm_value.h"
#include "sdl_platform.h"
#include "bytecode.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>

using namespace dialos;

int main(int argc, char** argv) {
    std::cout << "=== dialOS SDL Emulator ===" << std::endl;
    std::cout << "M5 Dial Hardware Simulation" << std::endl;
    std::cout << std::endl;
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <bytecode.dsb>" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Controls:" << std::endl;
        std::cerr << "  Mouse Wheel    - Rotary encoder" << std::endl;
        std::cerr << "  Left Click     - Touch display / Encoder button" << std::endl;
        std::cerr << "  R Key          - Toggle RFID card simulation" << std::endl;
        std::cerr << "  B Key          - Test buzzer beep" << std::endl;
        std::cerr << "  D Key          - Toggle debug overlay" << std::endl;
        std::cerr << "  ESC Key        - Exit emulator" << std::endl;
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
    
    std::cout << "Loaded: " << filename << " (" << fileSize << " bytes)" << std::endl;
    
    // Deserialize bytecode
    compiler::BytecodeModule module;
    try {
        module = compiler::BytecodeModule::deserialize(data);
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to deserialize bytecode: " << e.what() << std::endl;
        return 1;
    }
    
    // Show app metadata
    std::cout << std::endl;
    std::cout << "App Info:" << std::endl;
    std::cout << "  Name:     " << module.metadata.appName << " v" << module.metadata.appVersion << std::endl;
    std::cout << "  Author:   " << (module.metadata.author.empty() ? "(none)" : module.metadata.author) << std::endl;
    std::cout << "  Heap:     " << module.metadata.heapSize << " bytes" << std::endl;
    std::cout << "  Version:  " << module.metadata.version << std::endl;
    std::cout << std::endl;
    
    // Initialize SDL platform
    vm::SDLPlatform platform;
    if (!platform.initialize("dialOS Emulator - " + module.metadata.appName)) {
        std::cerr << "Failed to initialize SDL platform" << std::endl;
        return 1;
    }
    
    // Enable debug overlay by default for emulator
    platform.debug_showInfo(false);
    
    // Create VM
    vm::ValuePool pool(module.metadata.heapSize);
    vm::VMState vm(module, pool, platform);
    
    std::cout << "Starting emulation..." << std::endl;
    std::cout << "Close window or press ESC to exit" << std::endl;
    std::cout << std::endl;
    
    vm.reset();
    
    // Main emulation loop
    const uint32_t TARGET_FPS = 60;
    const uint32_t FRAME_TIME_MS = 1000 / TARGET_FPS;
    const uint32_t VM_CYCLES_PER_FRAME = 1000; // Execute up to 1000 instructions per frame
    
    auto lastFrameTime = std::chrono::steady_clock::now();
    bool running = true;
    bool vmPaused = false;
    bool finalStateShown = false;
    
    while (running && !platform.shouldQuit()) {
        auto frameStart = std::chrono::steady_clock::now();
        
        // Poll SDL events
        if (!platform.pollEvents()) {
            break;
        }
        
        // Execute VM if still running and not paused
        if (vm.isRunning() && !vmPaused) {
            vm::VMResult result = vm.execute(VM_CYCLES_PER_FRAME);
            
            switch (result) {
                case vm::VMResult::FINISHED:
                    std::cout << std::endl;
                    std::cout << "Program finished normally" << std::endl;
                    std::cout << "Emulator paused. Press ESC or close window to exit." << std::endl;
                    vmPaused = true;
                    break;
                    
                case vm::VMResult::ERROR:
                    std::cout << std::endl;
                    std::cerr << "Runtime Error: " << vm.getError() << std::endl;
                    std::cerr << "PC: " << vm.getPC() << ", Stack: " << vm.getStackSize() << std::endl;
                    std::cout << "Emulator paused. Press ESC or close window to exit." << std::endl;
                    vmPaused = true;
                    break;
                    
                case vm::VMResult::OUT_OF_MEMORY:
                    std::cout << std::endl;
                    std::cerr << "Out of Memory at PC: " << vm.getPC() << std::endl;
                    std::cout << "Emulator paused. Press ESC or close window to exit." << std::endl;
                    vmPaused = true;
                    break;
                    
                case vm::VMResult::YIELD:
                    // VM yielded (sleep/pause), continue normally
                    break;
                    
                case vm::VMResult::OK:
                    // Normal execution, continue
                    break;
            }
        }
        
        // Show final state once when VM first pauses
        if (vmPaused && !finalStateShown) {
            std::cout << std::endl;
            std::cout << "=== Final State ===" << std::endl;
            std::cout << "  PC: " << vm.getPC() << std::endl;
            std::cout << "  Stack size: " << vm.getStackSize() << std::endl;
            std::cout << "  Call stack depth: " << vm.getCallStack().size() << std::endl;
            std::cout << "  Heap used: " << pool.getAllocated() << "/" << pool.getHeapSize() << " bytes" << std::endl;
            
            // Show globals
            const auto& globals = vm.getGlobals();
            if (!globals.empty()) {
                std::cout << std::endl;
                std::cout << "Global Variables:" << std::endl;
                for (const auto& pair : globals) {
                    std::cout << "  " << pair.first << " = " << pair.second.toString() << std::endl;
                }
            }
            
            std::cout << std::endl;
            finalStateShown = true;
        }
        
        // Present frame
        platform.present();
        
        // Frame rate limiting
        auto frameEnd = std::chrono::steady_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        
        if (frameDuration.count() < FRAME_TIME_MS) {
            std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_TIME_MS - frameDuration.count()));
        }
    }
    
    std::cout << "Emulation complete." << std::endl;
    
    return 0;
}