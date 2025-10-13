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
        std::cerr << "Usage: " << argv[0] << " <input.ds> [output.dsb]" << std::endl;
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "output.dsb";
    
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
    BytecodeCompiler compiler;
    BytecodeModule module = compiler.compile(*program);
    
    if (compiler.hasErrors()) {
        std::cerr << "Compilation errors:" << std::endl;
        for (const auto& error : compiler.getErrors()) {
            std::cerr << "  " << error << std::endl;
        }
        std::cerr << std::endl;
        std::cerr << "Note: Some features not yet implemented" << std::endl;
        // Continue anyway to show what was compiled
    }
    
    std::cout << "✓ Bytecode generated" << std::endl << std::endl;
    
    // Show statistics
    std::cout << "=== Bytecode Statistics ===" << std::endl;
    std::cout << "Code size:  " << module.code.size() << " bytes" << std::endl;
    std::cout << "Constants:  " << module.constants.size() << std::endl;
    std::cout << "Globals:    " << module.globals.size() << std::endl;
    std::cout << "Functions:  " << module.functions.size() << std::endl;
    std::cout << std::endl;
    
    // Disassemble
    std::cout << module.disassemble() << std::endl;
    
    // Serialize to file
    std::cout << "Writing bytecode to " << outputFile << "..." << std::endl;
    std::vector<uint8_t> bytecode = module.serialize();
    
    if (!writeFile(outputFile, bytecode)) {
        return 1;
    }
    
    std::cout << "✓ Bytecode file written (" << bytecode.size() << " bytes)" << std::endl;
    std::cout << std::endl;
    std::cout << "=== Compilation Complete ===" << std::endl;
    
    return 0;
}
