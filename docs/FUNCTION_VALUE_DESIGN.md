# Function Value Design - dialOS VM

## Overview
This document outlines the design for adding first-class function support to the dialOS VM, enabling callbacks, event handlers, and functional programming patterns.

## Current State Analysis

### What We Have
✅ **Function Definitions**: Functions can be defined with parameters and called
✅ **Function Registry**: `BytecodeModule` tracks function names and entry points
✅ **Call Frames**: Stack-based function call mechanism with local variables
✅ **Native Functions**: Can call platform functions via CALL_NATIVE
✅ **Placeholder Type**: `ValueType::NATIVE_FN` exists but unused

### What's Missing
❌ **Function as Value**: Can't store/pass functions as first-class values
❌ **Closures**: Can't capture surrounding scope
❌ **Callback Registry**: No mechanism to store callbacks for events
❌ **Anonymous Functions**: No lambda/arrow function syntax
❌ **Function Invocation**: Can't call a function stored in a variable

## Design Options

### Option 1: Simple Function References (MINIMAL)
**Concept**: Functions are just indices into the function table

**Pros**:
- Simplest to implement
- Minimal memory overhead
- Fast function calls
- No closure complexity

**Cons**:
- No closures (can't capture variables)
- No anonymous functions
- Limited use cases

**Implementation**:
```cpp
// In vm_value.h
enum class ValueType : uint8_t {
    // ... existing types ...
    FUNCTION     // Function reference
};

struct Function {
    uint16_t functionIndex;  // Index into module.functions
    uint8_t paramCount;      // For validation
};

// Add to Value union
union {
    // ... existing fields ...
    Function* functionVal;
};
```

**Syntax Example**:
```javascript
// Store function reference
var callback: myFunction;

// Pass as parameter
os.encoder.onTurn(myFunction);

// Can't do this yet:
var lambda: function(x) { return x * 2; };  // ❌ No anonymous functions
```

---

### Option 2: Function References with Closures (RECOMMENDED)
**Concept**: Functions can capture variables from surrounding scope

**Pros**:
- Full functional programming support
- Can create closures and anonymous functions
- More flexible event handlers
- Industry standard approach

**Cons**:
- More complex implementation
- Memory overhead for captured variables
- Garbage collection considerations

**Implementation**:
```cpp
// In vm_value.h
struct Function {
    uint16_t functionIndex;          // Index into module.functions
    uint8_t paramCount;               // Parameter count
    std::map<std::string, Value>* capturedVars;  // Closure environment (nullptr if none)
};

// Add to ValuePool
Function* allocateFunction(uint16_t funcIndex, uint8_t paramCount, 
                          const std::map<std::string, Value>* captureEnv = nullptr);
```

**Syntax Example**:
```javascript
var counter: 0;

function makeIncrementer() {
    // This would capture 'counter' from outer scope
    return function() {
        assign counter counter + 1;
        return counter;
    };
}

var inc: makeIncrementer();
inc();  // counter = 1
inc();  // counter = 2
```

---

### Option 3: Full Lambda Calculus (ADVANCED)
**Concept**: Full functional programming with higher-order functions

**Pros**:
- Maximum flexibility
- Powerful abstractions
- Modern programming paradigm

**Cons**:
- Very complex implementation
- Performance overhead
- Overkill for embedded system?

**Not recommended for dialOS at this stage**

---

## Recommended Approach: **Option 2 (Phased)**

### Phase 1: Basic Function References
Implement Option 1 first to unblock callback APIs quickly

### Phase 2: Add Closures
Extend to support captured variables

### Phase 3: Anonymous Functions
Add syntax for inline function definitions

---

## Detailed Implementation Plan (Phase 1)

### 1. Extend Value Type System

**File**: `src/vm/vm_value.h`

```cpp
// Add to ValueType enum
enum class ValueType : uint8_t {
    NULL_VAL,
    BOOL,
    INT32,
    FLOAT32,
    STRING,
    OBJECT,
    ARRAY,
    FUNCTION,    // NEW: Function reference
    NATIVE_FN    // Keep for future native callbacks
};

// Function structure
struct Function {
    uint16_t functionIndex;  // Index into BytecodeModule.functions
    uint8_t paramCount;      // Number of parameters (for validation)
    
    Function(uint16_t idx, uint8_t params) 
        : functionIndex(idx), paramCount(params) {}
};

// Add to Value union
union {
    bool boolVal;
    int32_t int32Val;
    float float32Val;
    std::string* stringVal;
    Object* objVal;
    Array* arrayVal;
    Function* functionVal;  // NEW
    void* nativeFn;
};

// Add factory method
static Value Function(Function* fn);

// Add type check
bool isFunction() const { return type == ValueType::FUNCTION; }
```

### 2. Update ValuePool

**File**: `src/vm/vm_value.h`

```cpp
class ValuePool {
public:
    // Add function allocation
    Function* allocateFunction(uint16_t funcIndex, uint8_t paramCount) {
        size_t size = sizeof(Function);
        if (allocated_ + size > heapSize_) {
            return nullptr;
        }
        
        auto* fn = new Function(funcIndex, paramCount);
        functions_.push_back(fn);
        allocated_ += size;
        return fn;
    }
    
    ~ValuePool() {
        // Clean up all allocated memory
        for (auto* str : strings_) delete str;
        for (auto* obj : objects_) delete obj;
        for (auto* arr : arrays_) delete arr;
        for (auto* fn : functions_) delete fn;  // NEW
    }
    
private:
    std::vector<Function*> functions_;  // NEW
};
```

### 3. Add New Opcodes

**File**: `src/vm/bytecode.h`

```cpp
enum class Opcode : uint8_t {
    // ... existing opcodes ...
    
    // Function operations
    LOAD_FUNCTION,    // Push function reference onto stack (operand: funcIndex u16)
    CALL_INDIRECT,    // Call function from stack (operand: argCount u8)
    
    // ... rest of opcodes ...
};
```

**Opcode Descriptions**:

- **LOAD_FUNCTION**: 
  - Operands: `funcIndex (u16)`
  - Behavior: Create Function value and push onto stack
  - Use: `var callback: myFunction` → `LOAD_FUNCTION funcIndex`

- **CALL_INDIRECT**:
  - Operands: `argCount (u8)`
  - Stack Layout: `[arg1, arg2, ..., argN, functionValue]`
  - Behavior: Pop function value, validate, create call frame, jump to function
  - Use: `callback(10, 20)` → `PUSH args, LOAD callback, CALL_INDIRECT 2`

### 4. VM Core Changes

**File**: `src/vm/vm_core.cpp`

```cpp
case compiler::Opcode::LOAD_FUNCTION: {
    uint16_t funcIndex = readU16();
    
    if (funcIndex >= module_.functions.size()) {
        setError("Invalid function index: " + std::to_string(funcIndex));
        return VMResult::ERROR;
    }
    
    // Get parameter count from function metadata
    // TODO: Need to store this in BytecodeModule
    uint8_t paramCount = 0;  // For now, assume stored somewhere
    
    Function* fn = pool_.allocateFunction(funcIndex, paramCount);
    if (!fn) {
        setError("Out of memory allocating function");
        return VMResult::OUT_OF_MEMORY;
    }
    
    push(Value::Function(fn));
    break;
}

case compiler::Opcode::CALL_INDIRECT: {
    uint8_t argCount = readU8();
    
    // Pop function value
    Value funcVal = pop();
    if (!funcVal.isFunction()) {
        setError("CALL_INDIRECT: value is not a function");
        return VMResult::ERROR;
    }
    
    Function* fn = funcVal.functionVal;
    
    // Validate argument count
    if (argCount != fn->paramCount) {
        setError("Function expects " + std::to_string(fn->paramCount) + 
                " arguments, got " + std::to_string(argCount));
        return VMResult::ERROR;
    }
    
    // Get function entry point
    uint16_t funcIndex = fn->functionIndex;
    if (funcIndex >= module_.functionEntryPoints.size()) {
        setError("Invalid function entry point");
        return VMResult::ERROR;
    }
    
    uint32_t entryPoint = module_.functionEntryPoints[funcIndex];
    
    // Create call frame (similar to CALL opcode)
    CallFrame frame;
    frame.returnPC = pc_;
    frame.stackBase = stack_.size() - argCount;
    frame.functionName = module_.functions[funcIndex];
    
    // Store arguments in local variables
    for (uint8_t i = 0; i < argCount; i++) {
        frame.locals[i] = stack_[frame.stackBase + i];
    }
    
    // Remove arguments from stack
    stack_.resize(frame.stackBase);
    
    // Push call frame and jump
    callStack_.push_back(frame);
    pc_ = entryPoint;
    
    break;
}
```

### 5. Compiler Changes

**File**: `compiler/bytecode_compiler.cpp`

Need to:
1. Detect when a function name is used as a value (not called)
2. Emit `LOAD_FUNCTION` instead of `CALL`
3. Store parameter count in BytecodeModule

**Example**:
```javascript
var callback: myFunction;  // Compile to: LOAD_FUNCTION funcIndex, STORE_LOCAL
callback(10);              // Compile to: PUSH 10, LOAD_LOCAL, CALL_INDIRECT 1
```

### 6. Extend BytecodeModule

**File**: `src/vm/bytecode.h`

```cpp
struct BytecodeModule {
    // ... existing fields ...
    std::vector<uint8_t> functionParamCounts;  // NEW: Parameter count for each function
    
    // Update addFunction
    uint16_t addFunction(const std::string& name, uint8_t paramCount) {
        for (size_t i = 0; i < functions.size(); i++) {
            if (functions[i] == name) {
                return static_cast<uint16_t>(i);
            }
        }
        functions.push_back(name);
        functionEntryPoints.push_back(0);
        functionParamCounts.push_back(paramCount);  // NEW
        return static_cast<uint16_t>(functions.size() - 1);
    }
};
```

---

## Callback Registry System

### Platform-Side Event System

**File**: `src/vm/platform.h`

```cpp
class PlatformInterface {
public:
    // Callback registry (can be nullptr if not set)
    struct CallbackRegistry {
        Value encoderTurnCallback;
        Value encoderButtonCallback;
        Value touchPressCallback;
        Value touchReleaseCallback;
        Value touchDragCallback;
        // ... more callbacks
    };
    
protected:
    CallbackRegistry callbacks_;
    VMState* vm_;  // Reference to VM for invoking callbacks
    
public:
    // Set VM reference (called during initialization)
    void setVM(VMState* vm) { vm_ = vm; }
    
    // Register callback
    virtual void encoder_onTurn(const Value& callback) {
        if (callback.isFunction()) {
            callbacks_.encoderTurnCallback = callback;
        }
    }
    
    // Trigger callback (called from platform when event occurs)
    void triggerEncoderTurn(int delta) {
        if (vm_ && callbacks_.encoderTurnCallback.isFunction()) {
            // Push arguments
            vm_->push(Value::Int32(delta));
            
            // Push function
            vm_->push(callbacks_.encoderTurnCallback);
            
            // Call function
            // TODO: Need mechanism to invoke from outside normal execution
            // Option 1: Queue event for next VM tick
            // Option 2: Immediate invocation (might interrupt current execution)
        }
    }
};
```

### Event Queue Approach (RECOMMENDED)

**File**: `src/vm/vm_core.h`

```cpp
struct Event {
    Value callback;
    std::vector<Value> arguments;
};

class VMState {
private:
    std::queue<Event> eventQueue_;  // NEW
    
public:
    // Queue event for processing
    void queueEvent(const Value& callback, const std::vector<Value>& args) {
        eventQueue_.push({callback, args});
    }
    
    // Process pending events (call at end of execute())
    void processEvents() {
        while (!eventQueue_.empty() && running_) {
            Event event = eventQueue_.front();
            eventQueue_.pop();
            
            // Push arguments
            for (const auto& arg : event.arguments) {
                push(arg);
            }
            
            // Push function
            push(event.callback);
            
            // Call function (use CALL_INDIRECT logic)
            // ... invoke function ...
        }
    }
};
```

---

## Usage Examples

### Example 1: Encoder Callback

```javascript
var position: 0;

function handleTurn(delta: int): void {
    assign position position + delta;
    os.console.print(`Position: ${position}`);
}

// Register callback
os.encoder.onTurn(handleTurn);

// Main loop continues
while (true) {
    os.system.sleep(100);
    // Events processed automatically
}
```

### Example 2: Timer Callback

```javascript
var ticks: 0;

function onTick(): void {
    assign ticks ticks + 1;
    os.console.print(`Tick ${ticks}`);
}

var timerId: os.timer.setInterval(onTick, 1000);

// After 10 ticks, stop
while (ticks < 10) {
    os.system.sleep(100);
}

os.timer.clearInterval(timerId);
```

### Example 3: App Lifecycle

```javascript
function onLoad(): void {
    os.console.print("App started!");
    os.display.clear(0x0000);
}

function onSuspend(): void {
    os.console.print("App paused");
}

function onResume(): void {
    os.console.print("App resumed");
}

// Register lifecycle callbacks
os.app.onLoad(onLoad);
os.app.onSuspend(onSuspend);
os.app.onResume(onResume);

// ... app code ...
```

---

## Implementation Roadmap

### Milestone 1: Core Function Values ✅ COMPLETE
- [x] Add `ValueType::FUNCTION` to vm_value.h
- [x] Add `Function` struct and factory methods
- [x] Update `ValuePool` to allocate functions
- [x] Add `functionParamCounts` to BytecodeModule
- [x] Implement `LOAD_FUNCTION` opcode in VM (0x83)
- [x] Test: Create and pass function references

### Milestone 2: Function Invocation ✅ COMPLETE
- [x] Implement `CALL_INDIRECT` opcode in VM (0x84)
- [x] Update compiler to detect function-as-value usage
- [x] Emit correct opcodes for indirect calls
- [x] Test: Call functions stored in variables

### Milestone 3: Callback Registry 🔄 IN PROGRESS
- [x] Add `CallbackRegistry` to PlatformInterface (callbacks_ member exists)
- [x] Add `setVM()` to link platform to VM
- [ ] Implement event queue in VMState
- [ ] Add `processEvents()` to VM execution loop
- [ ] Test: Basic callback invocation

### Milestone 4: Event APIs ⏳ BLOCKED (needs Milestone 3)
- [ ] Implement `encoder.onTurn()` / `encoder.onButton()`
- [ ] Implement `touch.onPress()` / `onRelease()` / `onDrag()`
- [ ] Implement `timer.setTimeout()` / `setInterval()`
- [ ] Implement `app.onLoad()` / lifecycle callbacks
- [ ] Test: All callback-based APIs working

### Milestone 5: Advanced Features ⏳ FUTURE
- [ ] Add closure support (captured variables)
- [ ] Add anonymous function syntax
- [ ] Implement remaining event APIs
- [ ] Performance optimization
- [ ] Memory management improvements

---

## Testing Strategy

### Unit Tests
1. **Value System**: Test Function value creation, storage, retrieval
2. **Opcodes**: Test LOAD_FUNCTION and CALL_INDIRECT independently
3. **Event Queue**: Test event queueing and processing
4. **Callback Registry**: Test callback registration and invocation

### Integration Tests
1. **Simple Callback**: Register and trigger encoder callback
2. **Multiple Callbacks**: Multiple events firing in sequence
3. **Callback with Arguments**: Pass different argument types
4. **Error Handling**: Invalid callbacks, wrong argument counts
5. **Memory Stress**: Many callbacks, deep call stacks

### Performance Tests
1. **Callback Overhead**: Measure indirect call vs direct call cost
2. **Event Queue**: Test with high event frequency
3. **Memory Usage**: Track function value allocation

---

## Potential Challenges

### Challenge 1: Compiler Complexity
**Issue**: Detecting when function name is value vs call
**Solution**: Parse context - if followed by `(`, it's a call; otherwise, it's a reference

### Challenge 2: Garbage Collection
**Issue**: Function values in callbacks might never be freed
**Solution**: 
- Simple: Accept memory leak for long-running callbacks
- Better: Reference counting for Function values
- Best: Full garbage collector (future work)

### Challenge 3: Recursive Callbacks
**Issue**: Callback triggers event that triggers same callback
**Solution**: 
- Event queue prevents stack overflow
- Add recursion depth limit
- Document best practices

### Challenge 4: Thread Safety
**Issue**: ESP32 hardware interrupts might trigger callbacks
**Solution**:
- Queue events from ISRs
- Process events only during VM execution
- Use mutex if needed

---

## Alternative Designs Considered

### Native Callback Pointers
**Idea**: Store C++ function pointers instead of bytecode
**Rejected**: Breaks VM sandboxing, security concerns

### String-Based Callbacks
**Idea**: Store function name as string, look up at runtime
**Rejected**: Slower, more error-prone, no type safety

### Inline Bytecode
**Idea**: Embed function bytecode directly in callback
**Rejected**: Code duplication, harder to debug

---

## Conclusion

**Recommended Path**: 
1. Start with **Phase 1** (simple function references)
2. Implement **event queue** for callback dispatch
3. Unlock **35 blocked APIs** incrementally
4. Add **closures** in Phase 2 when needed

This approach balances:
- ✅ Quick wins (unblock APIs fast)
- ✅ Solid architecture (extensible for future)
- ✅ Reasonable complexity (phased implementation)
- ✅ Embedded constraints (memory efficiency)

**Estimated Total Effort**: 8-12 weeks for full implementation
**Quick Win**: Milestone 1-3 can unblock most APIs in 4-6 weeks
