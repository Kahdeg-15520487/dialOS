/**
 * Parse dialScript file and display AST
 * Usage: parse_file <filename.ds>
 */

#include "lexer.h"
#include "parser.h"
#include "ast_printer.h"
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename.ds>" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    std::cout << "=== dialScript File Parser ===" << std::endl;
    std::cout << "File: " << filename << std::endl << std::endl;
    
    // Read source file
    std::string source = readFile(filename);
    if (source.empty()) {
        return 1;
    }
    
    std::cout << "Source length: " << source.length() << " characters" << std::endl;
    std::cout << std::endl;
    
    // Parse
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parse();
    
    // Check for errors
    if (parser.hasErrors()) {
        std::cerr << "=== Parse Errors ===" << std::endl;
        for (const auto& error : parser.getErrors()) {
            std::cerr << "  " << error << std::endl;
        }
        std::cerr << std::endl;
    }
    
    // Print AST
    std::cout << "=== Abstract Syntax Tree ===" << std::endl;
    ASTPrinter printer;
    std::cout << printer.print(*program) << std::endl;
    
    // Summary
    std::cout << std::endl << "=== Summary ===" << std::endl;
    std::cout << "Parse errors: " << parser.getErrors().size() << std::endl;
    std::cout << "Top-level declarations: " << program->statements.size() << std::endl;
    
    return parser.hasErrors() ? 1 : 0;
}
