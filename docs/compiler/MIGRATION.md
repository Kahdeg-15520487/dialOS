# Migration Guide: Arduino String → std::string

This guide helps you migrate code that used the Arduino-dependent version of the parser to the new platform-independent version.

## What Changed

The parser has been updated to use **standard C++** instead of Arduino-specific types:

| Old (Arduino)          | New (Standard C++)    |
|------------------------|----------------------|
| `String`               | `std::string`        |
| `Serial.print()`       | `std::cout <<`       |
| `Serial.println()`     | `std::cout << ... << std::endl` |
| `#include <Arduino.h>` | `#include <string>`  |
| Arduino `delay()`      | Not needed           |

## Code Migration Examples

### Example 1: Basic Parsing

**Before (Arduino):**
```cpp
#include "lexer.h"

void parseCode() {
    String code = "var x: 42;";
    dialos::compiler::Lexer lexer(code);
    
    Token token = lexer.nextToken();
    Serial.println(token.value);
}
```

**After (Standard C++):**
```cpp
#include "lexer.h"
#include <iostream>

void parseCode() {
    std::string code = "var x: 42;";
    dialos::compiler::Lexer lexer(code);
    
    Token token = lexer.nextToken();
    std::cout << token.value << std::endl;
}
```

### Example 2: Error Handling

**Before (Arduino):**
```cpp
if (parser.hasErrors()) {
    for (const auto& error : parser.getErrors()) {
        Serial.println(error);
    }
}
```

**After (Standard C++):**
```cpp
if (parser.hasErrors()) {
    for (const auto& error : parser.getErrors()) {
        std::cout << error << std::endl;
    }
}
```

### Example 3: String Concatenation

**Before (Arduino):**
```cpp
String msg = "Error at line " + String(line) + ": " + message;
errors.push_back(msg);
```

**After (Standard C++):**
```cpp
std::string msg = "Error at line " + std::to_string(line) + ": " + message;
errors.push_back(msg);
```

### Example 4: AST Printing

**Before (Arduino):**
```cpp
ASTPrinter printer;
String ast = printer.print(*program);
Serial.println(ast);
```

**After (Standard C++):**
```cpp
ASTPrinter printer;
std::string ast = printer.print(*program);
std::cout << ast << std::endl;
```

## Benefits of Migration

✅ **Platform Independence**
- Run on Linux, macOS, Windows without Arduino IDE
- Develop and test on your PC
- Deploy to embedded systems when ready

✅ **Better Tooling**
- Use standard debuggers (gdb, lldb, MSVC debugger)
- IDE support (VSCode, CLion, Visual Studio)
- Profiling tools (valgrind, perf, Instruments)

✅ **Easier Testing**
- Write unit tests with standard frameworks
- Automated CI/CD pipelines
- Faster iteration without hardware

✅ **Still Compatible with Arduino/ESP32**
- Include in Arduino sketches
- Works with PlatformIO
- No breaking changes to API structure

## Building for Different Platforms

### Desktop Development (Testing)
```bash
g++ -std=c++11 -o parser_test my_test.cpp lexer.cpp ast.cpp parser.cpp
./parser_test
```

### Arduino/ESP32 (Deployment)
Add to your `platformio.ini`:
```ini
build_flags = -std=c++11
```

Include in your sketch:
```cpp
#include "compiler/lexer.h"
#include "compiler/parser.h"

void setup() {
    std::string code = "var x: 42;";
    dialos::compiler::Lexer lexer(code);
    // ... use parser as normal
}
```

## Common Migration Patterns

### 1. Replace Serial Output

**Pattern:**
```cpp
// Before
Serial.print("Value: ");
Serial.println(value);

// After
std::cout << "Value: " << value << std::endl;
```

### 2. String Type Declarations

**Pattern:**
```cpp
// Before
String name;
String getValue() { return name; }
void setValue(const String& val) { name = val; }

// After
std::string name;
std::string getValue() { return name; }
void setValue(const std::string& val) { name = val; }
```

### 3. Number to String Conversion

**Pattern:**
```cpp
// Before
String numStr = String(42);

// After
std::string numStr = std::to_string(42);
```

### 4. String Length and Comparison

**Pattern:**
```cpp
// Before
if (str.length() > 0) { ... }
if (str == "test") { ... }

// After (same!)
if (str.length() > 0) { ... }
if (str == "test") { ... }
```

### 5. Character Access

**Pattern:**
```cpp
// Before
char c = str[0];
str += 'x';

// After (same!)
char c = str[0];
str += 'x';
```

## Checklist

Use this checklist to migrate your code:

- [ ] Replace `#include <Arduino.h>` with `#include <string>`
- [ ] Change `String` to `std::string`
- [ ] Replace `Serial.print()` with `std::cout <<`
- [ ] Replace `Serial.println()` with `std::cout << ... << std::endl`
- [ ] Use `std::to_string()` for number-to-string conversion
- [ ] Update compilation commands to use standard C++ compiler
- [ ] Remove Arduino-specific delays and setup/loop functions (if testing standalone)
- [ ] Test on PC before deploying to embedded

## Gradual Migration

You can migrate gradually:

1. **Phase 1**: Keep using Arduino for final deployment, but test parser on PC
2. **Phase 2**: Write new code using std::string
3. **Phase 3**: Refactor old code incrementally
4. **Phase 4**: Fully platform-independent codebase

## Still Need Arduino String?

If you absolutely need Arduino `String` compatibility, create a wrapper:

```cpp
// arduino_compat.h
#ifdef ARDUINO
    #include <Arduino.h>
    using PlatformString = String;
#else
    #include <string>
    using PlatformString = std::string;
#endif

// Then use PlatformString in your code
```

But we recommend fully embracing `std::string` for better portability!

## Questions?

See:
- `README.md` - Full documentation
- `QUICKSTART.md` - Getting started guide
- `BUILD.md` - Build instructions
