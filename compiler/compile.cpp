/**
 * Compile dialScript to bytecode
 * Usage: compile <input.ds> <output.dsb>
 */

#include "lexer.h"
#include "parser.h"
#include "bytecode_compiler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cctype>

using namespace dialos::compiler;

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filename << "'" << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool writeFile(const std::string& filename, const std::vector<uint8_t>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not create file '" << filename << "'" << std::endl;
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.ds|input.dsb> [output.dsb] [--c-array] [--debug]" << std::endl;
        std::cerr << "  input.ds:  Compile dialScript source to bytecode" << std::endl;
        std::cerr << "  input.dsb: Disassemble bytecode file" << std::endl;
        std::cerr << "  --c-array: Output as C/C++ byte array instead of binary file" << std::endl;
        std::cerr << "  --debug:   Include debug line information in bytecode" << std::endl;
        return 1;
    }
    
    std::string inputFile = argv[1];
    
    // Check if input is a .dsb file (disassemble mode)
    if (inputFile.length() >= 4 && inputFile.substr(inputFile.length() - 4) == ".dsb") {
        // Disassemble mode
        std::cout << "=== dialScript Bytecode Disassembler ===" << std::endl;
        std::cout << "Input:  " << inputFile << std::endl << std::endl;
        
        // Read bytecode file
        std::ifstream file(inputFile, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open bytecode file '" << inputFile << "'" << std::endl;
            return 1;
        }
        
        std::vector<uint8_t> bytecode((std::istreambuf_iterator<char>(file)),
                                       std::istreambuf_iterator<char>());
        file.close();
        
        if (bytecode.empty()) {
            std::cerr << "Error: Bytecode file is empty" << std::endl;
            return 1;
        }
        
        std::cout << "Bytecode: " << bytecode.size() << " bytes" << std::endl << std::endl;
        
        // Deserialize and disassemble
        try {
            BytecodeModule module = BytecodeModule::deserialize(bytecode);
            std::cout << module.disassemble() << std::endl;
            std::cout << "=== Disassembly Complete ===" << std::endl;
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error: Failed to deserialize bytecode: " << e.what() << std::endl;
            return 1;
        }
    }
    
    // Compile mode
    std::string outputFile = (argc >= 3 && std::string(argv[2]) != "--c-array" && std::string(argv[2]) != "--debug") ? argv[2] : "output.dsb";
    bool outputCArray = false;
    bool debugInfo = false;
    
    // Check for flags
    for (int i = 2; i < argc; i++) {
        if (std::string(argv[i]) == "--c-array") {
            outputCArray = true;
        } else if (std::string(argv[i]) == "--debug") {
            debugInfo = true;
        }
    }
    
    std::cout << "=== dialScript Bytecode Compiler ===" << std::endl;
    std::cout << "Input:  " << inputFile << std::endl;
    std::cout << "Output: " << outputFile << std::endl << std::endl;
    
    // Read source file
    std::string source = readFile(inputFile);
    if (source.empty()) {
        return 1;
    }
    
    std::cout << "Source: " << source.length() << " bytes" << std::endl << std::endl;
    
    // Parse
    std::cout << "Parsing..." << std::endl;
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parse();
    
    if (parser.hasErrors()) {
        std::cerr << "Parse errors:" << std::endl;
        for (const auto& error : parser.getErrors()) {
            std::cerr << "  " << error << std::endl;
        }
        return 1;
    }
    std::cout << "✓ Parse successful" << std::endl << std::endl;
    
    // Compile to bytecode
    std::cout << "Compiling to bytecode..." << std::endl;
    if (debugInfo) {
        std::cout << "Debug info: Enabled" << std::endl;
    }
    BytecodeCompiler compiler;
    compiler.setDebugInfo(debugInfo);
    BytecodeModule module = compiler.compile(*program);
    
    if (compiler.hasErrors()) {
        std::cerr << "Compilation errors:" << std::endl;
        for (const auto& error : compiler.getErrors()) {
            std::cerr << "  " << error << std::endl;
        }
        std::cerr << std::endl;
        return 1;
    }
    
    std::cout << "✓ Bytecode generated" << std::endl << std::endl;
    
    // Show statistics
    std::cout << "=== Bytecode Statistics ===" << std::endl;
    std::cout << "Code size:  " << module.code.size() << " bytes" << std::endl;
    std::cout << "Constants:  " << module.constants.size() << std::endl;
    std::cout << "Globals:    " << module.globals.size() << std::endl;
    std::cout << "Functions:  " << module.functions.size() << std::endl;
    std::cout << std::endl;
    
    // Update integrity before displaying
    module.updateIntegrity();
    
    // Disassemble
    std::cout << module.disassemble() << std::endl;
    
    // Serialize to file
    std::cout << "Writing bytecode to " << outputFile << "..." << std::endl;
    std::vector<uint8_t> bytecode = module.serialize();
    
    if (outputCArray) {
        // Generate array name from input filename
        std::string arrayName;
        size_t lastSlash = inputFile.find_last_of("/\\");
        std::string baseName = (lastSlash != std::string::npos) ? inputFile.substr(lastSlash + 1) : inputFile;
        size_t lastDot = baseName.find_last_of('.');
        if (lastDot != std::string::npos) {
            baseName = baseName.substr(0, lastDot);
        }
        
        // Convert to uppercase and replace non-alphanumeric with underscore
        for (char c : baseName) {
            if (std::isalnum(c)) {
                arrayName += std::toupper(c);
            } else {
                arrayName += '_';
            }
        }
        
        // Output as C array
        std::ofstream file(outputFile);
        if (!file.is_open()) {
            std::cerr << "Error: Could not create file '" << outputFile << "'" << std::endl;
            return 1;
        }
        
        file << "// Generated bytecode array from " << inputFile << std::endl;
        file << "// Total size: " << bytecode.size() << " bytes" << std::endl;
        file << std::endl;
        file << "const unsigned char " << arrayName << "[] = {" << std::endl;
        
        for (size_t i = 0; i < bytecode.size(); i++) {
            if (i % 12 == 0) {
                file << "    ";
            }
            file << "0x" << std::hex << std::setw(2) << std::setfill('0') 
                 << static_cast<int>(bytecode[i]);
            if (i < bytecode.size() - 1) {
                file << ",";
                if ((i + 1) % 12 == 0) {
                    file << std::endl;
                } else {
                    file << " ";
                }
            }
        }
        file << std::endl << "};" << std::endl;
        file << std::endl;
        file << "const unsigned int " << arrayName << "_SIZE = " << std::dec << bytecode.size() << ";" << std::endl;
        
        std::cout << "✓ C array written to " << outputFile << " (" << bytecode.size() << " bytes)" << std::endl;
    } else {
        // Binary output
        if (!writeFile(outputFile, bytecode)) {
            return 1;
        }
        std::cout << "✓ Bytecode file written (" << bytecode.size() << " bytes)" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "=== Compilation Complete ===" << std::endl;
    
    return 0;
}
