# dialScript VM Architecture - Implementation Plan

## Overview
Portable virtual machine that runs on both PC (simulator with ImGui) and ESP32 (M5 Dial hardware) with identical bytecode execution.

## Design Decisions ✅

1. **Graphics**: ImGui for PC simulator
2. **Binary Format**: Extended .dsb with metadata (app name, version, author, heap size, timestamp)
3. **Memory**: Fixed heap size (declared in metadata)
4. **Threading**: Single-threaded cooperative multitasking
5. **Heap Declaration**: Required in dialScript source code

## Updated Bytecode Format (.dsb v2)

```
[Magic: "DSBC"]              4 bytes
[Format Version: uint16]     2 bytes  (e.g., 0x0001)
[Flags: uint16]              2 bytes  (reserved)

=== METADATA SECTION ===
[Heap Size: uint32]          4 bytes  (e.g., 8192)
[App Name Length: uint16]    2 bytes
[App Name: string]           variable
[App Version Length: uint16] 2 bytes
[App Version: string]        variable (e.g., "1.0.0")
[Author Length: uint16]      2 bytes
[Author: string]             variable
[Timestamp: uint32]          4 bytes  (Unix timestamp)

=== CONSTANTS SECTION ===
[Constant Count: uint32]     4 bytes
  [String Length: uint16]    2 bytes
  [String Data]              variable
  ... (repeat for each constant)

=== GLOBALS SECTION ===
[Global Count: uint32]       4 bytes
  [Name Length: uint16]      2 bytes
  [Name Data]                variable
  ... (repeat for each global)

=== FUNCTIONS SECTION ===
[Function Count: uint32]     4 bytes
  [Name Length: uint16]      2 bytes
  [Name Data]                variable
  ... (repeat for each function)

=== CODE SECTION ===
[Code Size: uint32]          4 bytes
[Bytecode Instructions]      variable
```

## Components Implemented

### ✅ Updated Bytecode Format
**Files**: `compiler/bytecode.h`, `compiler/bytecode.cpp`

- Added `Metadata` struct to `BytecodeModule`
- Fields: version, heapSize, appName, appVersion, author, timestamp
- Updated serialization/deserialization
- Enhanced disassembler to show metadata

**Default Values**:
- Version: 1
- Heap Size: 8192 bytes (8KB)
- App Name: "untitled"
- App Version: "1.0.0"
- Author: "" (empty)
- Timestamp: 0 (set during compilation)

### ✅ VM Value System
**Files**: `src/vm/vm_value.h`, `src/vm/vm_value.cpp`

**Value Types**:
- NULL_VAL, BOOL, INT32, FLOAT32, STRING
- OBJECT (key-value map with className)
- ARRAY (dynamic array of values)
- NATIVE_FN (function pointer for os.* API)

**ValuePool** (Fixed-Size Heap):
- Constructor takes heap size from metadata
- Tracks allocated memory
- Pre-allocates strings, objects, arrays
- Returns nullptr on out-of-memory
- Auto-cleanup on destruction

**Features**:
- Tagged union (8 bytes per value)
- Truthiness evaluation
- String conversion (toString)
- Equality comparison
- Smart memory tracking

### ⏳ VM Core (Header Created)
**File**: `src/vm/vm_core.h`

**VMState Class**:
- Loads BytecodeModule
- Uses ValuePool for heap management
- Platform-agnostic interface
- Cooperative execution (max instructions per slice)

**Execution State**:
- Program counter (pc)
- Value stack
- Call stack (CallFrame)
- Global variables map
- Exception handlers

**VMResult Enum**:
- OK: Normal execution
- YIELD: Cooperative yield (sleep/pause)
- FINISHED: Program completed
- ERROR: Runtime error
- OUT_OF_MEMORY: Heap exhausted

**CallFrame**:
- Return PC
- Local variables (uint8 → Value map)
- Stack base
- Function name (debugging)

## Next Steps

### 1. Complete VM Core Implementation
**File**: `src/vm/vm_core.cpp`

Implement:
- [ ] VMState constructor
- [ ] execute() main loop
- [ ] executeInstruction() dispatcher (switch on opcode)
- [ ] All arithmetic operations
- [ ] All comparison operations
- [ ] All logical operations
- [ ] Stack operations (PUSH, POP, DUP, SWAP)
- [ ] Jump operations with PC updates
- [ ] Function call/return
- [ ] Object/array operations
- [ ] Exception handling (TRY/THROW/END_TRY)

### 2. Platform Abstraction Layer
**Files**: `src/vm/platform.h`

```cpp
class PlatformInterface {
public:
    // Display
    virtual void display_clear(uint32_t color) = 0;
    virtual void display_drawText(int x, int y, const char* text, 
                                  uint32_t color, int size) = 0;
    
    // Input
    virtual bool encoder_getButton() = 0;
    virtual int encoder_getDelta() = 0;
    
    // Timing
    virtual uint32_t system_getTime() = 0;
    virtual void system_sleep(uint32_t ms) = 0;
    
    // Debug
    virtual void console_log(const char* msg) = 0;
};
```

### 3. PC Simulator (ImGui)
**Files**: `simulator/main.cpp`, `simulator/pc_platform.h/cpp`

**ImGui Window Layout**:
```
┌─────────────────────────────────────────────────────────┐
│ dialOS Simulator - [timer.dsb]                       × │
├─────────────────────────────────────────────────────────┤
│ ┌───────────────┐ ┌─────────────────────────────────┐  │
│ │               │ │ Debugger                        │  │
│ │    M5 Dial    │ ├─────────────────────────────────┤  │
│ │   Display     │ │ Status: Running                 │  │
│ │               │ │ PC: 0x00A4 (164)                │  │
│ │   240x240     │ │ Stack: 3 items                  │  │
│ │               │ │                                 │  │
│ │               │ │ Stack:                          │  │
│ │               │ │   [2] Int32: 120                │  │
│ │   [Button]    │ │   [1] String: "Timer"           │  │
│ │   Encoder     │ │   [0] Bool: true                │  │
│ │   <  ●  >     │ │                                 │  │
│ └───────────────┘ │ Globals (6):                    │  │
│                   │   timer = [Object Timer]        │  │
│ ┌───────────────┐ │   display = [Object Display]    │  │
│ │ Controls      │ │   count = Int32: 0              │  │
│ │ [▶ Run]       │ │                                 │  │
│ │ [❚❚ Pause]    │ │ Heap: 1024/8192 bytes          │  │
│ │ [↻ Reset]     │ │                                 │  │
│ │ [📂 Load]     │ │ [Step] [Run] [Pause] [Reset]   │  │
│ └───────────────┘ └─────────────────────────────────┘  │
│                                                         │
│ Console:                                                │
│ > Loaded: timer.dsb (v1.0.0)                           │
│ > Heap allocated: 8192 bytes                            │
│ > Execution started                                     │
│ > os.display.clear(0)                                   │
│ _                                                       │
└─────────────────────────────────────────────────────────┘
```

**Features**:
- SDL2 + ImGui rendering
- Simulated M5 Dial display (240x240 circle)
- Interactive encoder (mouse wheel + click)
- Step-by-step debugging
- Breakpoints
- Hot reload (.dsb file watcher)
- Console output

### 4. Task Scheduler
**File**: `src/vm/vm_scheduler.h/cpp`

```cpp
enum class TaskStatus {
    RUNNING,
    FROZEN,
    SLEEPING
};

class VMTask {
    VMState state;
    TaskStatus status;
    uint32_t sleepUntil;
    std::string appName;
    
    VMResult tick(uint32_t instructionsPerSlice);
};

class TaskScheduler {
    std::vector<VMTask*> tasks;
    VMTask* activeTask;
    
    void tick();
    void addTask(VMTask* task);
    void removeTask(VMTask* task);
};
```

### 5. Native Function Bridge
**File**: `src/vm/vm_native.h/cpp`

```cpp
typedef Value (*NativeFunction)(const std::vector<Value>& args, 
                                 PlatformInterface& platform);

class NativeFunctionRegistry {
    std::map<std::string, NativeFunction> functions;
    
    void registerFunction(const char* name, NativeFunction fn);
    Value call(const char* name, const std::vector<Value>& args);
};

// Example native functions
Value native_display_clear(const std::vector<Value>& args, 
                           PlatformInterface& platform) {
    if (args.size() < 1 || !args[0].isInt32()) {
        return Value::Null();
    }
    platform.display_clear(args[0].int32Val);
    return Value::Null();
}
```

### 6. ESP32 Integration
**Files**: `src/platform/esp32_platform.h/cpp`, `src/main.cpp`

```cpp
class ESP32Platform : public PlatformInterface {
    M5Dial& m5;
    
public:
    void display_clear(uint32_t color) override {
        M5.Display.fillScreen(color);
    }
    
    uint32_t system_getTime() override {
        return millis();
    }
    
    // ... other implementations
};

// In main.cpp
void setup() {
    M5Dial.begin();
    
    // Load bytecode from SPIFFS
    File f = SPIFFS.open("/apps/timer.dsb");
    std::vector<uint8_t> data = readFile(f);
    BytecodeModule module = BytecodeModule::deserialize(data);
    
    // Create VM with heap from metadata
    ValuePool pool(module.metadata.heapSize);
    ESP32Platform platform(M5);
    VMState vm(module, pool, platform);
    
    // Run VM in loop
}

void loop() {
    vm.execute(1000); // Execute 1000 instructions
    M5.update();
}
```

## dialScript Heap Declaration Syntax

We need to add heap size declaration to dialScript. Options:

**Option 1: Pragma-style comment**
```javascript
//@ heap 8192

var timer: Timer = new Timer(60);
```

**Option 2: Metadata block**
```javascript
metadata {
    heap: 8192,
    name: "Timer App",
    version: "1.0.0"
}

var timer: Timer = new Timer(60);
```

**Option 3: Magic variable**
```javascript
var __heap_size__: int = 8192;

var timer: Timer = new Timer(60);
```

**Recommendation**: Option 2 (metadata block) - most explicit and allows future expansion.

## Build System Updates

### CMakeLists.txt additions:
```cmake
# VM Core
add_library(dialos_vm
    src/vm/vm_value.cpp
    src/vm/vm_core.cpp
    src/vm/vm_scheduler.cpp
    src/vm/vm_native.cpp
)

# PC Simulator
add_executable(simulator
    simulator/main.cpp
    simulator/pc_platform.cpp
)
target_link_libraries(simulator dialos_vm ImGui SDL2)
```

### PlatformIO for ESP32:
```ini
[env:m5stack-stamps3]
lib_deps =
    m5stack/M5Dial@^1.0.3
build_src_filter = 
    +<vm/>
    +<platform/esp32/>
    +<main.cpp>
```

## Testing Strategy

1. **Unit Tests**: Test VM operations in isolation
2. **Integration Tests**: Full bytecode execution
3. **Benchmark**: Performance on PC vs ESP32
4. **Memory Tests**: Heap exhaustion handling

## Performance Targets

- **PC**: 100,000+ instructions/second
- **ESP32**: 10,000+ instructions/second
- **Heap**: Efficient allocation (< 10% overhead)
- **Startup**: < 100ms bytecode loading

## Current Status

✅ Bytecode format updated with metadata
✅ Value system with fixed-size heap
✅ VM core architecture designed
⏳ VM implementation in progress
⏳ Platform abstraction layer
⏳ ImGui simulator
⏳ Task scheduler
⏳ Native function bridge
⏳ ESP32 integration

---

**Next Immediate Task**: Implement `vm_core.cpp` with instruction dispatcher and all operations.
