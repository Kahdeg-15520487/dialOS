---
applyTo: '**'
---

# DialScript Language Syntax Reference

DialScript is a JavaScript-like scripting language designed for the dialOS operating system on M5 Dial hardware. This reference provides comprehensive syntax documentation for code generation and analysis.

## Language Overview

- **File Extension**: `.ds`
- **Paradigm**: Object-oriented with classes, functions, and variables
- **Type System**: Static typing with explicit type annotations
- **Syntax Style**: JavaScript-like with some unique dialOS-specific features

## Comments

```javascript
// Single-line comment

/*
 * Multi-line comment
 * Supports multiple lines
 */
```

## Variables

### Declaration Syntax
```javascript
var name: value;
var message: "Hello World";
var count: 42;
var sum: 10 + 20;
var max: (a > b) ? a : b;
```

### Assignment Syntax
```javascript
assign variable newValue;
assign count count + 1;
assign this.value 42;
assign array[0] "first";
```

**Note**: DialScript uses `assign` keyword instead of `=` for assignments.

## Data Types

### Primitive Types
- `int` - 32-bit signed integer
- `uint` - 32-bit unsigned integer  
- `byte` - 8-bit unsigned integer
- `short` - 16-bit signed integer
- `float` - 32-bit floating point
- `bool` - Boolean (true/false)
- `string` - Text string
- `void` - No return value (functions only)
- `any` - Any type (dynamic)

### Type Annotations
```javascript
var number: int;
var text: string;
var flag: bool;
var data: any;
```

### Array Types
```javascript
var numbers: int[];
var messages: string[];
var flags: bool[];
```

### Nullable Types
```javascript
var optional: int?;
var maybeText: string?;
```

## Literals

### Numbers
```javascript
var decimal: 42;
var negative: -17;
var floating: 3.14;
var hex: 0xFF;
```

### Strings
```javascript
var simple: "Hello";
var escaped: "Line 1\nLine 2\tTabbed";
var quote: "She said \"Hello\"";
```

### Template Literals
```javascript
var formatted: `Score: ${score}, Status: ${status}`;
var time: `${minutes}:${seconds < 10 ? "0" : ""}${seconds}`;
```

### Booleans
```javascript
var isTrue: true;
var isFalse: false;
```

### Null
```javascript
var empty: null;
```

### Arrays
```javascript
var numbers: [1, 2, 3, 4, 5];
var mixed: [42, "hello", true, null];
var empty: [];
```

## Operators

### Arithmetic
```javascript
var sum: a + b;
var difference: a - b;
var product: a * b;
var quotient: a / b;
var remainder: a % b;
var negative: -value;
var positive: +value;
```

### Comparison
**Note**: DialScript uses `=` (not `==`) for equality comparison.

```javascript
var isEqual: x = y;
var notEqual: x != y;
var less: x < y;
var greater: x > y;
var lessEqual: x <= y;
var greaterEqual: x >= y;
```

### Logical
```javascript
var andResult: (x > 0) and (y < 100);
var orResult: condition1 or condition2;
var notResult: not flag;
```

### Ternary
```javascript
var result: condition ? valueIfTrue : valueIfFalse;
var status: (score >= 60) ? "pass" : "fail";
var grade: (score >= 90) ? "A" : (score >= 80) ? "B" : "C";
```

### Operator Precedence (highest to lowest)
1. Member access (`.`), Array access (`[]`)
2. Function calls `()`, Constructor calls
3. Unary (`not`, `-`, `+`)
4. Multiplicative (`*`, `/`, `%`)
5. Additive (`+`, `-`)
6. Comparison (`<`, `>`, `<=`, `>=`)
7. Equality (`=`, `!=`)
8. Logical AND (`and`)
9. Logical OR (`or`)
10. Ternary (`? :`)

## Functions

### Function Declaration
```javascript
function functionName(param1: type1, param2: type2): returnType {
    // function body
    return value;
}
```

### Examples
```javascript
function add(x: int, y: int): int {
    return x + y;
}

function greet(name: string): void {
    os.console.log(`Hello, ${name}!`);
}

function max(a: int, b: int): int {
    return (a > b) ? a : b;
}
```

### Function Calls
```javascript
var sum: add(10, 20);
greet("Alice");
var maximum: max(15, 25);
```

## Classes

### Class Declaration
```javascript
class ClassName {
    field1: type;
    field2: type;
    
    constructor(param1: type, param2: type) {
        assign this.field1 param1;
        assign this.field2 param2;
    }
    
    methodName(param: type): returnType {
        // method body
        return value;
    }
}
```

### Example
```javascript
class Counter {
    value: int;
    
    constructor(initial: int) {
        assign this.value initial;
    }
    
    increment(): void {
        assign this.value this.value + 1;
    }
    
    decrement(): void {
        assign this.value this.value - 1;
    }
    
    getValue(): int {
        return this.value;
    }
    
    reset(): void {
        assign this.value 0;
    }
}
```

### Object Creation
```javascript
var counter: Counter(42);
var timer: Timer(120);
```

### Member Access
```javascript
var value: counter.getValue();
assign counter.value 100;
counter.increment();
```

## Control Flow

### If-Else Statements
```javascript
if (condition) {
    // statements
}

if (condition) {
    // statements
} else {
    // alternative statements
}

if (condition1) {
    // statements
} else if (condition2) {
    // statements
} else {
    // default statements
}
```

### While Loops
```javascript
while (condition) {
    // loop body
    // don't forget to modify condition variable
}
```

### For Loops
```javascript
for (initialization; condition; update) {
    // loop body
}

// Example
for (var i: 0; i < 10; assign i i + 1;) {
    os.console.log(i);
}
```

### Try-Catch-Finally
```javascript
try {
    // risky code
    riskyOperation();
} catch (error) {
    // error handling
    os.console.error(error);
} finally {
    // cleanup code (always runs)
    cleanup();
}
```

### Return Statements
```javascript
return; // void return
return value; // return value
return (condition) ? value1 : value2; // conditional return
```

## Expressions

### Member Access
```javascript
var length: message.length;
var result: object.method();
var nested: object.property.subProperty;
```

### Array Access
```javascript
var first: array[0];
var last: array[array.length - 1];
var dynamic: array[index];
```

### Function Calls
```javascript
var result: functionName(arg1, arg2);
var value: object.method(parameter);
```

### Constructor Calls
```javascript
var instance: ClassName(arg1, arg2);
var number: int(42);
var text: string("hello");
```

### Parenthesized Expressions
```javascript
var result: (a + b) * (c - d);
var complex: ((x * y) + z) / (a - b);
```

## Hardware API Integration

### Display Operations
```javascript
os.display.clear(0x0000);
os.display.drawText(x, y, text, color, size);
os.display.drawRect(x, y, width, height, color, filled);
os.display.drawCircle(x, y, radius, color, filled);
```

### Console Output
```javascript
os.console.log("Debug message");
os.console.error("Error occurred");
os.console.warn("Warning message");
```

### Input Handling
```javascript
// Encoder events
encoder.onTurn(callback);
encoder.onButton(callback);

// Touch events  
touch.onPress(callback);
touch.onRelease(callback);
```

## Block Structure

### Code Blocks
```javascript
{
    // statements grouped in block
    var localVar: 42;
    someFunction();
}
```

### Scope Rules
- Variables declared in blocks have block scope
- Class fields are accessible throughout the class
- Function parameters are accessible throughout the function

## Error Handling

### Common Syntax Patterns
- Always use `:` after variable names in declarations
- Use `assign` keyword for all assignments
- Use `=` (not `==`) for equality comparisons
- Use `and`/`or` instead of `&&`/`||`
- Use `not` instead of `!`

### Example Complete Program
```javascript
/*
 * Complete DialScript Example
 */

// Global variables
var appName: "Timer App";
var version: "1.0";

// Timer class
class Timer {
    seconds: int;
    running: bool;
    
    constructor(initial: int) {
        assign this.seconds initial;
        assign this.running false;
    }
    
    start(): void {
        assign this.running true;
        os.console.log("Timer started");
    }
    
    stop(): void {
        assign this.running false;
        os.console.log("Timer stopped");
    }
    
    tick(): void {
        if (this.running and this.seconds > 0) {
            assign this.seconds this.seconds - 1;
        }
    }
    
    isFinished(): bool {
        return this.seconds = 0;
    }
    
    getDisplay(): string {
        var minutes: this.seconds / 60;
        var secs: this.seconds % 60;
        return `${minutes}:${secs < 10 ? "0" : ""}${secs}`;
    }
}

// Main function
function main(): void {
    var timer: Timer(300); // 5 minutes
    
    os.display.clear(0x0000);
    os.display.drawText(70, 40, appName, 0xFFFF, 2);
    
    timer.start();
    
    while (not timer.isFinished()) {
        timer.tick();
        var display: timer.getDisplay();
        os.display.drawText(60, 120, display, 0xFFFF, 3);
        os.system.sleep(1000); // 1 second delay
    }
    
    os.console.log("Timer finished!");
}

// Start the application
main();
```

## Code Generation Guidelines

When generating DialScript code:

1. **Always use explicit type annotations** for variables and function parameters
2. **Use `assign` keyword** for all variable assignments
3. **Use `=` for equality** comparison (not `==`)
4. **Use `and`/`or`/`not`** for logical operations
5. **Include proper error handling** with try-catch blocks where appropriate
6. **Follow consistent indentation** (4 spaces recommended)
7. **Add meaningful comments** for complex logic
8. **Use descriptive variable and function names**
9. **Group related functionality** into classes
10. **Leverage hardware APIs** through the `os` namespace

Provide project context and coding guidelines that AI should follow when generating code, answering questions, or reviewing changes.

