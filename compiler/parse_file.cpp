/**
 * Parse dialScript file and display AST
 * Usage: parse_file <filename.ds> [--json]
 */

#include "lexer.h"
#include "parser.h"
#include "ast_printer.h"
#include "ast_json.h"
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
        std::cerr << "Usage: " << argv[0] << " <filename.ds> [--json]" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    bool jsonOutput = false;
    
    // Check for --json flag
    if (argc >= 3 && std::string(argv[2]) == "--json") {
        jsonOutput = true;
    }
    
    if (!jsonOutput) {
        std::cout << "=== dialScript File Parser ===" << std::endl;
        std::cout << "File: " << filename << std::endl << std::endl;
    }
    
    // Read source file
    std::string source = readFile(filename);
    if (source.empty()) {
        return 1;
    }
    
    if (!jsonOutput) {
        std::cout << "Source length: " << source.length() << " characters" << std::endl;
        std::cout << std::endl;
    }
    
    // Parse
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parse();
    
    // Check for errors
    if (parser.hasErrors()) {
        if (jsonOutput) {
            // Output errors as JSON
            std::cout << "{" << std::endl;
            std::cout << "  \"success\": false," << std::endl;
            std::cout << "  \"errors\": [" << std::endl;
            const auto& errors = parser.getErrors();
            for (size_t i = 0; i < errors.size(); i++) {
                std::cout << "    \"" << errors[i] << "\"";
                if (i < errors.size() - 1) std::cout << ",";
                std::cout << std::endl;
            }
            std::cout << "  ]" << std::endl;
            std::cout << "}" << std::endl;
        } else {
            std::cerr << "=== Parse Errors ===" << std::endl;
            for (const auto& error : parser.getErrors()) {
                std::cerr << "  " << error << std::endl;
            }
            std::cerr << std::endl;
        }
    }
    
    if (jsonOutput) {
        // Output AST as JSON
        if (!parser.hasErrors()) {
            ASTJsonExporter exporter;
            std::cout << exporter.toJson(*program) << std::endl;
        }
    } else {
        // Print AST (human-readable)
        std::cout << "=== Abstract Syntax Tree ===" << std::endl;
        ASTPrinter printer;
        std::cout << printer.print(*program) << std::endl;
        
        // Summary
        std::cout << std::endl << "=== Summary ===" << std::endl;
        std::cout << "Parse errors: " << parser.getErrors().size() << std::endl;
        std::cout << "Top-level declarations: " << program->statements.size() << std::endl;
    }
    
    return parser.hasErrors() ? 1 : 0;
}
