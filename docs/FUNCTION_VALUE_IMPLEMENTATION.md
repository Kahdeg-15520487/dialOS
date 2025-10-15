# Function Value Implementation - Minimal Approach

## Design Decisions ✅

✅ **No Closures** - Functions cannot capture surrounding scope (compiler error if undeclared variable used)  
✅ **No Anonymous Functions** - Only named functions can be used as values  
✅ **Immediate Invocation** - Callbacks execute immediately when triggered (no event queue)  
✅ **No GC** - Manual memory management via ValuePool (same as strings/objects/arrays)  
✅ **Simple Function References** - Just store function index + parameter count

---

## Implementation Steps

### Step 1: Extend Value Type System

**File**: `src/vm/vm_value.h`

#### 1.1 Add FUNCTION to ValueType enum
```cpp
enum class ValueType : uint8_t {
    NULL_VAL,
    BOOL,
    INT32,
    FLOAT32,
    STRING,
    OBJECT,
    ARRAY,
    FUNCTION,    // NEW: Function reference
    NATIVE_FN    // Keep for future
};
```

#### 1.2 Add Function struct (before Value struct)
```cpp
// Function reference - just index + param count
struct Function {
    uint16_t functionIndex;  // Index into BytecodeModule.functions
    uint8_t paramCount;      // Number of parameters (for validation)
    
    Function(uint16_t idx, uint8_t params) 
        : functionIndex(idx), paramCount(params) {}
};
```

#### 1.3 Add functionVal to Value union
```cpp
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
```

#### 1.4 Add Function factory method (after other factory methods)
```cpp
static Value Function(Function* fn) {
    Value v;
    v.type = ValueType::FUNCTION;
    v.functionVal = fn;
    return v;
}
```

#### 1.5 Add isFunction() check (after other type checks)
```cpp
bool isFunction() const { return type == ValueType::FUNCTION; }
```

#### 1.6 Add asFunction() getter (after other getters)
```cpp
Function* asFunction() const { return functionVal; }
```

#### 1.7 Update ValuePool::allocateFunction()
```cpp
// Add to ValuePool class (after allocateArray)
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
```

#### 1.8 Add functions_ vector to ValuePool private members
```cpp
private:
    size_t heapSize_;
    size_t allocated_;
    std::vector<std::string*> strings_;
    std::vector<Object*> objects_;
    std::vector<Array*> arrays_;
    std::vector<Function*> functions_;  // NEW
```

#### 1.9 Update ValuePool destructor
```cpp
~ValuePool() {
    // Clean up all allocated memory
    for (auto* str : strings_) delete str;
    for (auto* obj : objects_) delete obj;
    for (auto* arr : arrays_) delete arr;
    for (auto* fn : functions_) delete fn;  // NEW
}
```

---

### Step 2: Add New Opcodes

**File**: `src/vm/bytecode.h`

#### 2.1 Add opcodes to enum (after existing opcodes, before last entry)
```cpp
enum class Opcode : uint8_t {
    // ... existing opcodes ...
    
    LOAD_FUNCTION = 0x2A,  // Push function reference (u16 funcIndex)
    CALL_INDIRECT = 0x2B,  // Call function from stack (u8 argCount)
    
    // ... rest of opcodes ...
};
```

#### 2.2 Update opcode name mapping (in getOpcodeName function)
```cpp
case Opcode::LOAD_FUNCTION: return "LOAD_FUNCTION";
case Opcode::CALL_INDIRECT: return "CALL_INDIRECT";
```

---

### Step 3: Extend BytecodeModule

**File**: `src/vm/bytecode.h`

#### 3.1 Add functionParamCounts vector to BytecodeModule
```cpp
struct BytecodeModule {
    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::map<std::string, Value> globals;
    std::vector<std::string> functions;
    std::vector<uint32_t> functionEntryPoints;
    std::vector<uint8_t> functionParamCounts;  // NEW: Parameter count for each function
    
    // ... rest of struct ...
};
```

#### 3.2 Update addFunction() to store param count
```cpp
// Find and modify existing addFunction method
uint16_t addFunction(const std::string& name, uint8_t paramCount = 0) {
    // Check if already exists
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
```

---

### Step 4: VM Core Implementation

**File**: `src/vm/vm_core.cpp`

#### 4.1 Implement LOAD_FUNCTION opcode
```cpp
case compiler::Opcode::LOAD_FUNCTION: {
    uint16_t funcIndex = readU16();
    
    if (funcIndex >= module_.functions.size()) {
        setError("Invalid function index: " + std::to_string(funcIndex));
        return VMResult::ERROR;
    }
    
    // Get parameter count from module
    uint8_t paramCount = (funcIndex < module_.functionParamCounts.size()) 
                        ? module_.functionParamCounts[funcIndex] 
                        : 0;
    
    Function* fn = pool_.allocateFunction(funcIndex, paramCount);
    if (!fn) {
        setError("Out of memory allocating function");
        return VMResult::OUT_OF_MEMORY;
    }
    
    push(Value::Function(fn));
    break;
}
```

#### 4.2 Implement CALL_INDIRECT opcode
```cpp
case compiler::Opcode::CALL_INDIRECT: {
    uint8_t argCount = readU8();
    
    // Pop function value (on top of stack)
    Value funcVal = pop();
    if (!funcVal.isFunction()) {
        setError("CALL_INDIRECT: value is not a function");
        return VMResult::ERROR;
    }
    
    Function* fn = funcVal.asFunction();
    
    // Validate argument count
    if (argCount != fn->paramCount) {
        setError("Function '" + module_.functions[fn->functionIndex] + 
                "' expects " + std::to_string(fn->paramCount) + 
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
    
    // Create call frame (same as CALL opcode)
    CallFrame frame;
    frame.returnPC = pc_;
    frame.stackBase = stack_.size() - argCount;
    frame.functionName = module_.functions[funcIndex];
    
    // Store arguments in locals
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

---

### Step 5: Compiler Changes

**File**: `compiler/bytecode_compiler.cpp`

#### 5.1 Track function parameter counts during compilation
When compiling function declarations, store parameter count:
```cpp
// In compileFunctionDeclaration or similar:
uint8_t paramCount = functionNode->parameters.size();
uint16_t funcIndex = module.addFunction(functionName, paramCount);
```

#### 5.2 Detect function-as-value usage
When visiting an identifier, check if it's a function being used as value:
```cpp
// In visitIdentifier or similar:
if (isFunctionName(name) && !isBeingCalled()) {
    // Emit LOAD_FUNCTION instead of regular variable load
    uint16_t funcIndex = getFunctionIndex(name);
    emitU8(Opcode::LOAD_FUNCTION);
    emitU16(funcIndex);
} else {
    // Regular identifier access
    // ... existing code ...
}
```

#### 5.3 Compile indirect calls
When calling a variable that holds a function:
```cpp
// In visitCallExpression when callee is not a simple identifier:
if (isVariableHoldingFunction(callee)) {
    // Compile arguments first
    for (auto* arg : callNode->arguments) {
        visit(arg);
    }
    
    // Load function value
    visit(callee);
    
    // Emit CALL_INDIRECT
    emitU8(Opcode::CALL_INDIRECT);
    emitU8(argCount);
}
```

---

### Step 6: Callback System (Immediate Invocation)

**File**: `src/vm/platform.h`

#### 6.1 Add callback registry to PlatformInterface
```cpp
class PlatformInterface {
protected:
    // Callback storage (simple map)
    std::map<std::string, Value> callbacks_;
    
    // VM reference for invoking callbacks
    VMState* vm_;
    
public:
    // Set VM reference (called during VM initialization)
    void setVM(VMState* vm) { vm_ = vm; }
    
    // Register callback helper
    void registerCallback(const std::string& eventName, const Value& callback) {
        if (callback.isFunction()) {
            callbacks_[eventName] = callback;
        }
    }
    
    // Get callback helper
    Value getCallback(const std::string& eventName) const {
        auto it = callbacks_.find(eventName);
        return (it != callbacks_.end()) ? it->second : Value::Null();
    }
    
    // Invoke callback helper (immediate execution)
    bool invokeCallback(const std::string& eventName, const std::vector<Value>& args) {
        if (!vm_) return false;
        
        Value callback = getCallback(eventName);
        if (!callback.isFunction()) return false;
        
        // Push arguments onto stack
        for (const auto& arg : args) {
            vm_->push(arg);
        }
        
        // Push function
        vm_->push(callback);
        
        // Execute CALL_INDIRECT logic inline
        Function* fn = callback.asFunction();
        
        // Validate arg count
        if (args.size() != fn->paramCount) {
            return false;
        }
        
        // Create call frame and invoke
        // (This requires exposing some VM internals or using a helper method)
        return vm_->invokeFunction(fn->functionIndex, args.size());
    }
};
```

#### 6.2 Add invokeFunction helper to VMState
**File**: `src/vm/vm_core.h`
```cpp
class VMState {
public:
    // ... existing methods ...
    
    // Helper for immediate callback invocation
    bool invokeFunction(uint16_t funcIndex, uint8_t argCount);
};
```

**File**: `src/vm/vm_core.cpp`
```cpp
bool VMState::invokeFunction(uint16_t funcIndex, uint8_t argCount) {
    if (funcIndex >= module_.functionEntryPoints.size()) {
        return false;
    }
    
    uint32_t entryPoint = module_.functionEntryPoints[funcIndex];
    
    // Create call frame
    CallFrame frame;
    frame.returnPC = pc_;  // Will return to current location
    frame.stackBase = stack_.size() - argCount;
    frame.functionName = module_.functions[funcIndex];
    
    // Store arguments in locals
    for (uint8_t i = 0; i < argCount; i++) {
        frame.locals[i] = stack_[frame.stackBase + i];
    }
    
    // Remove arguments from stack
    stack_.resize(frame.stackBase);
    
    // Push call frame and jump
    size_t savedPC = pc_;
    callStack_.push_back(frame);
    pc_ = entryPoint;
    
    // Execute until RETURN (need to track depth)
    size_t initialDepth = callStack_.size();
    while (running_ && callStack_.size() >= initialDepth) {
        VMResult result = step();
        if (result != VMResult::OK && result != VMResult::FINISHED) {
            return false;
        }
    }
    
    return true;
}
```

---

### Step 7: Implement Callback-Based Native APIs

#### 7.1 Encoder Callbacks
```cpp
// encoder.onTurn(callback)
virtual void encoder_onTurn(const Value& callback) {
    registerCallback("encoder.onTurn", callback);
}

// In ESP32Platform, when encoder turns:
void checkEncoder() {
    int delta = getEncoderDelta();
    if (delta != 0) {
        invokeCallback("encoder.onTurn", {Value::Int32(delta)});
    }
}
```

#### 7.2 Touch Callbacks
```cpp
// touch.onPress(callback)
virtual void touch_onPress(const Value& callback) {
    registerCallback("touch.onPress", callback);
}

// In platform update loop:
void checkTouch() {
    if (touchPressed()) {
        auto [x, y] = getTouchPosition();
        invokeCallback("touch.onPress", {Value::Int32(x), Value::Int32(y)});
    }
}
```

#### 7.3 Timer Callbacks
```cpp
struct TimerInfo {
    Value callback;
    uint32_t interval;
    uint32_t lastTrigger;
    bool repeating;
};

std::map<int, TimerInfo> timers_;
int nextTimerId_ = 1;

virtual int timer_setTimeout(const Value& callback, int ms) {
    int id = nextTimerId_++;
    timers_[id] = {callback, (uint32_t)ms, millis(), false};
    return id;
}

virtual int timer_setInterval(const Value& callback, int ms) {
    int id = nextTimerId_++;
    timers_[id] = {callback, (uint32_t)ms, millis(), true};
    return id;
}

void updateTimers() {
    uint32_t now = millis();
    std::vector<int> toRemove;
    
    for (auto& [id, timer] : timers_) {
        if (now - timer.lastTrigger >= timer.interval) {
            invokeCallback("timer." + std::to_string(id), {});
            timer.lastTrigger = now;
            
            if (!timer.repeating) {
                toRemove.push_back(id);
            }
        }
    }
    
    for (int id : toRemove) {
        timers_.erase(id);
    }
}
```

#### 7.4 App Lifecycle Callbacks
```cpp
virtual void app_onLoad(const Value& callback) {
    registerCallback("app.onLoad", callback);
}

// In VM startup:
void VMState::start() {
    // ... initialization ...
    
    // Trigger onLoad if registered
    platform_->invokeCallback("app.onLoad", {});
    
    // ... continue execution ...
}
```

---

## Testing Plan

### Test 1: Basic Function Reference
```javascript
function greet(name: string): void {
    os.console.print(`Hello, ${name}!`);
}

var callback: greet;  // Store function reference
callback("World");    // Call via reference
```

### Test 2: Encoder Callback
```javascript
var position: 0;

function handleTurn(delta: int): void {
    assign position position + delta;
    os.console.print(`Position: ${position}`);
}

os.encoder.onTurn(handleTurn);

// Keep running
while (true) {
    os.system.sleep(100);
}
```

### Test 3: Timer Callback
```javascript
var ticks: 0;

function onTick(): void {
    assign ticks ticks + 1;
    os.console.print(`Tick ${ticks}`);
}

var timerId: os.timer.setInterval(onTick, 1000);
os.system.sleep(10000);
os.timer.clearInterval(timerId);
```

### Test 4: Error Cases
```javascript
// Should fail: undeclared variable in function
function bad(): void {
    os.console.print(undeclaredVar);  // ❌ Compiler error
}

// Should fail: wrong argument count
function add(a: int, b: int): int {
    return a + b;
}

var callback: add;
callback(5);  // ❌ Runtime error: expects 2 args, got 1
```

---

## Implementation Checklist

### Phase 1: Core Infrastructure (Week 1)
- [ ] Add FUNCTION type to ValueType enum
- [ ] Add Function struct
- [ ] Update Value union and factory methods
- [ ] Update ValuePool with allocateFunction()
- [ ] Add LOAD_FUNCTION and CALL_INDIRECT opcodes
- [ ] Extend BytecodeModule with functionParamCounts
- [ ] Test: Create and store function values

### Phase 2: VM Execution (Week 2)
- [ ] Implement LOAD_FUNCTION in vm_core.cpp
- [ ] Implement CALL_INDIRECT in vm_core.cpp
- [ ] Add invokeFunction() helper method
- [ ] Test: Call functions via references

### Phase 3: Compiler Support (Week 3)
- [ ] Track parameter counts during function compilation
- [ ] Detect function-as-value usage
- [ ] Emit LOAD_FUNCTION for function references
- [ ] Emit CALL_INDIRECT for indirect calls
- [ ] Add compiler error for undeclared variables
- [ ] Test: Compile and run function reference code

### Phase 4: Callback System (Week 4)
- [ ] Add callback registry to PlatformInterface
- [ ] Add setVM() and invokeCallback() helpers
- [ ] Implement encoder callbacks
- [ ] Implement touch callbacks
- [ ] Implement timer callbacks
- [ ] Implement app lifecycle callbacks
- [ ] Test: All callback-based APIs

### Phase 5: Testing & Documentation (Week 5)
- [ ] Write comprehensive test suite
- [ ] Test error handling
- [ ] Performance testing
- [ ] Update API documentation
- [ ] Write usage examples
- [ ] Code review and cleanup

---

## Estimated Timeline
- **Total**: 5 weeks
- **Quick Win**: Weeks 1-3 enable basic callbacks (60% of use cases)
- **Full Feature**: Week 4-5 complete all callback APIs

---

## Memory Overhead
- **Function value**: 3 bytes (2 bytes index + 1 byte param count)
- **Per callback registration**: ~20 bytes (map entry + string key)
- **Total for 10 callbacks**: ~230 bytes (negligible on ESP32)

---

## Next Steps
1. Review and approve this plan
2. Start with Step 1 (Value Type System)
3. Incremental testing after each step
4. Regular commits to track progress
