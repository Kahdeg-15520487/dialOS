/**
 * dialScript Parser Test Example
 * 
 * This example demonstrates how to use the recursive descent parser
 * to parse dialScript source code and print the resulting AST.
 * 
 * Compile with: g++ -std=c++11 -o test_parser test_parser.cpp lexer.cpp ast.cpp parser.cpp ast_printer.cpp
 */

#include "lexer.h"
#include "parser.h"
#include "ast_printer.h"
#include <iostream>

using namespace dialos::compiler;

// Test source code
const char* testSource = R"(
// Counter class example
var count: 0;
var message: "Hello dialOS";

class Counter {
    value: int;
    
    constructor(initial: int) {
        assign this.value initial;
    }
    
    increment(): void {
        assign this.value this.value + 1;
    }
    
    getValue(): int {
        return this.value;
    }
}

function main(): void {
    var counter: Counter(42);
    
    if (count = 0) {
        counter.increment();
        os.console.print(message);
    }
    
    while (counter.getValue() < 50) {
        counter.increment();
    }
    
    var result: (counter.getValue() >= 50) ? "done" : "continue";
    os.console.print(`Result: ${result}`);
}
)";

int main() {
    std::cout << "=== dialScript Parser Test ===" << std::endl << std::endl;
    
    // Create lexer
    std::string source(testSource);
    Lexer lexer(source);
    
    std::cout << "Source code:" << std::endl;
    std::cout << source << std::endl;
    std::cout << std::endl << "--- Lexical Analysis ---" << std::endl << std::endl;
    
    // Show tokens (optional)
    /*
    Token token;
    while ((token = lexer.nextToken()).type != TokenType::END_OF_FILE) {
        std::cout << "Token: " << token.value 
                  << " (" << (int)token.type << ")"
                  << " at line " << token.line 
                  << ":" << token.column << std::endl;
    }
    
    // Re-create lexer for parsing
    lexer = Lexer(source);
    */
    
    std::cout << std::endl << "--- Parsing ---" << std::endl << std::endl;
    
    // Parse
    Parser parser(lexer);
    auto program = parser.parse();
    
    // Check for errors
    if (parser.hasErrors()) {
        std::cout << "Parse errors encountered:" << std::endl;
        for (const auto& error : parser.getErrors()) {
            std::cout << "  ERROR: " << error << std::endl;
        }
        std::cout << std::endl;
    } else {
        std::cout << "Parse successful! No errors." << std::endl << std::endl;
    }
    
    // Print AST
    std::cout << "--- Abstract Syntax Tree ---" << std::endl << std::endl;
    ASTPrinter printer;
    std::string ast = printer.print(*program);
    std::cout << ast << std::endl;
    
    std::cout << std::endl << "=== Test Complete ===" << std::endl;
    
    return 0;
}
