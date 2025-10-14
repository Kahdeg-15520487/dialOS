# dialScript Parser Build Notes

## Files Structure

```
compiler/
├── lexer.h              # Lexical analyzer header
├── lexer.cpp            # Lexical analyzer implementation
├── ast.h                # AST node definitions
├── ast.cpp              # AST node implementations
├── parser.h             # Recursive descent parser header
├── parser.cpp           # Recursive descent parser implementation
├── ast_printer.h        # AST debugging/visualization header
├── ast_printer.cpp      # AST debugging/visualization implementation
├── test_parser.cpp      # Example usage and testing
├── README.md            # Documentation
└── BUILD.md             # This file
```

## Building as Standalone (PC/Linux/macOS)

The parser is now **platform-independent** and uses standard C++ instead of Arduino libraries.

### Prerequisites

- C++11 compatible compiler (GCC 4.8+, Clang 3.3+, MSVC 2015+)
- Standard C++ library support

### Compile and Run

```bash
# Compile the test program
g++ -std=c++11 -o test_parser \
    test_parser.cpp \
    lexer.cpp \
    ast.cpp \
    parser.cpp \
    ast_printer.cpp

# Run the test
./test_parser
```

**Windows (MSVC):**
```cmd
cl /EHsc /std:c++11 test_parser.cpp lexer.cpp ast.cpp parser.cpp ast_printer.cpp
test_parser.exe
```

**Cross-platform with CMake:**
```cmake
cmake_minimum_required(VERSION 3.10)
project(dialscript_parser)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test_parser
    test_parser.cpp
    lexer.cpp
    ast.cpp
    parser.cpp
    ast_printer.cpp
)
```

## Building with PlatformIO (Arduino/ESP32)

## Building with PlatformIO (Arduino/ESP32)

While the parser uses standard C++, it can still be integrated into Arduino/ESP32 projects:

### 1. Add to platformio.ini

The parser uses C++11 features (smart pointers, move semantics). Ensure your build flags include:

```ini
[env:m5stack-stamps3]
platform = espressif32
board = m5stack-stamps3
framework = arduino
lib_deps = m5stack/M5Dial@^1.0.3
build_flags = 
    -std=c++11
    -Wall
```

### 2. Include in Your Project

In your main sketch or kernel code:

```cpp
#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "compiler/ast_printer.h"  // Optional, for debugging

void parseScript(const std::string& source) {
    dialos::compiler::Lexer lexer(source);
    dialos::compiler::Parser parser(lexer);
    auto ast = parser.parse();
    
    if (!parser.hasErrors()) {
        // Process AST...
    }
}
```

### 3. Build Commands

From project root:

```bash
# Build the project
pio run -e m5stack-stamps3

# Upload to device
pio run -e m5stack-stamps3 -t upload

# Monitor output
pio device monitor
```

## Memory Considerations

The parser uses dynamic memory allocation for AST nodes. On ESP32-S3:
- **Available heap**: ~320KB DRAM
- **Typical AST node**: 16-48 bytes
- **Recommended max program size**: Keep scripts under 10KB source

For larger programs:
1. Parse and compile in chunks
2. Stream bytecode generation
3. Don't keep entire AST in memory
4. Use a custom allocator with memory pooling

## Dependencies

The parser depends only on:
- **C++ Standard Library**: `<string>`, `<vector>`, `<memory>`, `<cstddef>` for standard types and containers

No external dependencies required. Works on any platform with C++11 support.

## Testing Strategy

### Unit Testing (TODO)
Create individual test cases for each parser component:
- Lexer token recognition
- Expression precedence
- Statement parsing
- Error recovery

### Integration Testing
Use the actual test files:
```cpp
#include <fstream>
#include <sstream>

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void testFile(const std::string& filename) {
    std::string source = readFile(filename);
    
    dialos::compiler::Lexer lexer(source);
    dialos::compiler::Parser parser(lexer);
    auto ast = parser.parse();
    
    // Verify no errors
    assert(!parser.hasErrors());
}

void runTests() {
    testFile("lsp/test.ds");
    testFile("lsp/test_expressions.ds");
    testFile("lsp/timer.ds");
}
```

## Compiler Warnings

The parser is designed to compile without warnings. If you see warnings:

**Common issues:**
- Unused parameters in visitor methods → expected, can use `(void)param;`
- Signed/unsigned comparisons → use explicit casts
- Switch without default → add default cases

**Recommended flags:**
```ini
build_flags = 
    -Wall
    -Wextra
    -Wno-unused-parameter  # Visitor pattern has many unused params
```

## Performance Profiling

To measure parser performance:

```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();
auto ast = parser.parse();
auto end = std::chrono::high_resolution_clock::now();

auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
std::cout << "Parse time: " << duration.count() << " microseconds" << std::endl;
```

Expected performance on ESP32-S3 @ 240MHz:
- Small program (< 100 lines): < 10ms
- Medium program (< 500 lines): < 50ms
- Large program (< 2000 lines): < 200ms

## Next Steps

After building the parser, integrate with:

1. **Type Checker**: Create `TypeChecker` visitor to validate types
2. **Bytecode Compiler**: Create `BytecodeGenerator` visitor to emit VM instructions
3. **Optimizer**: Create `ASTOptimizer` visitor for constant folding, dead code elimination
4. **Interpreter**: Direct AST execution for debugging/testing

See Phase 6 of the project roadmap in main `.github/copilot-instructions.md`.
