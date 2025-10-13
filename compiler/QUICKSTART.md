# Quick Start Guide - dialScript Parser

## Installation

No installation required! Just include the source files in your project.

## Compilation

### Quick Build (Linux/macOS)
```bash
cd compiler/
make
./test_parser
```

### Windows (MSVC)
```cmd
cd compiler
cl /EHsc /std:c++11 test_parser.cpp lexer.cpp ast.cpp parser.cpp ast_printer.cpp
test_parser.exe
```

### CMake (Cross-platform)
```bash
cd compiler/
mkdir build && cd build
cmake ..
cmake --build .
./test_parser  # or test_parser.exe on Windows
```

## Basic Usage

### 1. Parse a Simple Program

```cpp
#include "lexer.h"
#include "parser.h"
#include <iostream>

int main() {
    using namespace dialos::compiler;
    
    std::string code = "var x: 42;";
    
    Lexer lexer(code);
    Parser parser(lexer);
    auto ast = parser.parse();
    
    if (parser.hasErrors()) {
        std::cerr << "Errors found!" << std::endl;
        for (const auto& error : parser.getErrors()) {
            std::cerr << "  " << error << std::endl;
        }
        return 1;
    }
    
    std::cout << "Success!" << std::endl;
    return 0;
}
```

### 2. Print the AST

```cpp
#include "lexer.h"
#include "parser.h"
#include "ast_printer.h"
#include <iostream>

int main() {
    using namespace dialos::compiler;
    
    std::string code = R"(
        var count: 0;
        
        function increment(): void {
            assign count count + 1;
        }
    )";
    
    Lexer lexer(code);
    Parser parser(lexer);
    auto ast = parser.parse();
    
    if (!parser.hasErrors()) {
        ASTPrinter printer;
        std::cout << printer.print(*ast) << std::endl;
    }
    
    return 0;
}
```

### 3. Parse from File

```cpp
#include "lexer.h"
#include "parser.h"
#include <iostream>
#include <fstream>
#include <sstream>

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    using namespace dialos::compiler;
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <script.ds>" << std::endl;
        return 1;
    }
    
    try {
        std::string code = readFile(argv[1]);
        
        Lexer lexer(code);
        Parser parser(lexer);
        auto ast = parser.parse();
        
        if (parser.hasErrors()) {
            for (const auto& error : parser.getErrors()) {
                std::cerr << error << std::endl;
            }
            return 1;
        }
        
        std::cout << "Parse successful!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

## API Reference

### Lexer

```cpp
dialos::compiler::Lexer lexer(source_code);

Token token = lexer.nextToken();     // Get next token
Token peek = lexer.peekToken();      // Peek at next token without consuming
```

### Parser

```cpp
dialos::compiler::Parser parser(lexer);

auto ast = parser.parse();                    // Parse entire program
bool hasErrors = parser.hasErrors();          // Check for errors
auto& errors = parser.getErrors();            // Get error messages
```

### AST Printer

```cpp
dialos::compiler::ASTPrinter printer;

std::string output = printer.print(*ast_node);  // Convert AST to string
```

### Token Types

```cpp
enum class TokenType {
    // Literals
    NUMBER, STRING, BOOLEAN, NULL_LIT, IDENTIFIER,
    
    // Keywords
    VAR, ASSIGN, FUNCTION, CLASS, CONSTRUCTOR,
    IF, ELSE, WHILE, FOR, TRY, CATCH, FINALLY, RETURN,
    
    // Types
    INT, UINT, BYTE, SHORT, FLOAT, BOOL, STRING_TYPE, VOID, ANY,
    
    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQUAL, NOT_EQUAL, LESS, GREATER, LESS_EQUAL, GREATER_EQUAL,
    AND, OR, NOT,
    
    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    SEMICOLON, COMMA, DOT, COLON, QUESTION,
    
    // Special
    END_OF_FILE, ERROR
};
```

## Testing Your Changes

Run the provided test:
```bash
make test
```

Or create your own test file `my_test.ds`:
```javascript
var greeting: "Hello World";
os.console.log(greeting);
```

Then parse it:
```bash
./test_parser  # Modify test_parser.cpp to read from file
```

## Next Steps

1. **Type Checking**: Create an `ASTVisitor` to validate types
2. **Bytecode Generation**: Walk the AST and emit VM instructions
3. **Optimization**: Add constant folding, dead code elimination
4. **Error Recovery**: Improve error messages with suggestions

See `README.md` for complete documentation.

## Common Issues

### Compilation Error: "unknown type name 'String'"
✅ Fixed! The parser now uses `std::string` instead of Arduino's `String`.

### Linking Error
Make sure to compile all source files:
```bash
g++ -std=c++11 test_parser.cpp lexer.cpp ast.cpp parser.cpp ast_printer.cpp
```

### Parse Errors
Check the error messages - they include line and column numbers:
```
Line 5:10 - Expected ';' after variable declaration
```

## Support

- See `README.md` for detailed documentation
- Check `BUILD.md` for build instructions
- Review `SUMMARY.md` for architecture overview
- Examine `test_parser.cpp` for usage examples
