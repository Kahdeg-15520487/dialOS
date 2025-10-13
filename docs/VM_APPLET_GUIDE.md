# VM Applet Development Guide

## Overview
dialOS now supports running arbitrary dialScript applets through a generalized VM task system. This guide explains how to create, compile, and integrate new applets into dialOS.

## Quick Start: Adding a New Applet

### 1. Write Your dialScript Code
Create a `.ds` file in the `scripts/` directory:

```javascript
// scripts/hello_world.ds
os.console.log("Hello from dialOS!");
```

### 2. Compile to Bytecode with C Array Output
Use the compiler's `--c-array` flag to generate a C header file:

```bash
cd compiler
.\compile.exe ..\scripts\hello_world.ds ..\src\hello_world_data.h --c-array
```

This generates a header file with:
```cpp
const unsigned char HELLO_WORLD_APPLET[] = {
    0x44, 0x53, 0x42, 0x43, 0x00, 0x01, ...
};
const size_t HELLO_WORLD_APPLET_SIZE = sizeof(HELLO_WORLD_APPLET);
```

### 3. Include Bytecode in Firmware
Add the header to `src/main.cpp`:

```cpp
#include "hello_world_data.h"  // Add with other includes
```

### 4. Register the Applet
Add your applet to the registry in `src/main.cpp`:

```cpp
static VMApplet appletRegistry[] = {
    {"counter", COUNTER_APPLET, COUNTER_APPLET_SIZE, 2000, true},
    {"hello", HELLO_WORLD_APPLET, HELLO_WORLD_APPLET_SIZE, 0, false},
    // Add more applets here
};
```

**VMApplet Parameters:**
- `name`: String identifier for the applet (used with `createVMTask()`)
- `bytecode`: Pointer to the bytecode array
- `bytecodeSize`: Size of the bytecode array
- `executeInterval`: Milliseconds between executions (0 = run immediately after completion)
- `repeat`: `true` = repeat indefinitely, `false` = run once and stop

### 5. Create a VM Task for Your Applet
In `setup()`, create a task:

```cpp
Task *helloTask = createVMTask("hello");
```

### 6. Build and Upload
```bash
pio run -e m5stack-stamps3 --target upload --target monitor
```

## Applet Execution Modes

### Continuous Repeat
Executes every N milliseconds indefinitely:
```cpp
{"blinker", BLINKER_APPLET, BLINKER_SIZE, 1000, true}  // Every 1 second
```

### One-Shot
Executes once then stops:
```cpp
{"setup", SETUP_APPLET, SETUP_SIZE, 0, false}  // Run once
```

### Continuous (No Delay)
Executes as fast as possible:
```cpp
{"game_loop", GAME_APPLET, GAME_SIZE, 0, true}  // Continuous
```

## dialScript Language Features

### Variables
```javascript
var count: 0;
var name: "dialOS";
```

### Assignments
```javascript
assign count count + 1;
assign name "New Name";
```

### Console Output
```javascript
os.console.log("Hello");
os.console.log(count);
```

### Known Limitations
- **No template literals** (causes compiler bug): ❌ `"Count: ${count}"`
- Use separate log calls instead: ✅ `os.console.log(count);`

## Platform API Access

Your applets can access M5Dial hardware through the `os` object:

### Display
```javascript
// Coming soon - display APIs
```

### Encoder
```javascript
// Coming soon - encoder APIs
```

### Timing
```javascript
// Coming soon - timing APIs
```

## Architecture Notes

### VM State Management
- Each applet gets its own `VMState` instance
- Module bytecode is allocated on heap and persists for VM lifetime
- `reset()` is called between executions to clear stack/PC
- Globals are reset except for `os` object (platform interface)

### Memory Management
- Default heap size: 8192 bytes (configurable in bytecode metadata)
- Each VM task uses 16KB stack
- Multiple applets can run concurrently (separate tasks)

### Cooperative Execution
- VM executes up to 1000 instructions per task tick
- `VMResult::YIELD` allows long-running applets to cooperate
- `VMResult::OK` continues execution on next tick
- `VMResult::FINISHED` triggers reset and sleep

## Example Applets

### Counter (Included)
```javascript
// scripts/counter_applet.ds
var count: 0;
assign count count + 1;
os.console.log(count);
```

Registry entry:
```cpp
{"counter", COUNTER_APPLET, COUNTER_APPLET_SIZE, 2000, true}
```

### Clock (Example)
```javascript
// scripts/clock_applet.ds
var seconds: 0;
assign seconds seconds + 1;
os.console.log(seconds);
```

Registry entry:
```cpp
{"clock", CLOCK_APPLET, CLOCK_APPLET_SIZE, 1000, true}
```

## Debugging Tips

### Check Compilation
Verify bytecode was generated correctly:
```bash
# Should output header file with const unsigned char array
cat src/your_applet_data.h
```

### Monitor Serial Output
```bash
pio device monitor
```

Look for:
- `Loading applet: <name> (<size> bytes)` - Applet loaded
- `Applet '<name>' loaded successfully` - Deserialization OK
- `VM initialized for '<name>'` - VM ready
- `Applet '<name>' execution #N completed` - Successful execution
- `VM error in '<name>': <message>` - Runtime error

### Common Issues

**"Applet not found in registry"**
- Check spelling in `createVMTask("name")` matches registry entry

**"Failed to deserialize bytecode"**
- Recompile with `--c-array` flag
- Verify bytecode array is included in header

**"VM out of memory"**
- Increase heap size in bytecode metadata
- Reduce memory usage in applet

**Crash after sleep/reset**
- Ensure BytecodeModule is static or heap-allocated
- Don't use local variables for module storage

## Advanced: Multiple Concurrent Applets

You can run multiple applets simultaneously:

```cpp
void setup() {
  // ... kernel initialization ...
  
  Task *counterTask = createVMTask("counter");
  Task *clockTask = createVMTask("clock");
  Task *gameTask = createVMTask("game");
  
  // All three run concurrently with cooperative scheduling
}
```

Each applet gets:
- Separate `VMState` instance
- Isolated globals and stack
- Independent execution timing
- Shared `ESP32Platform` instance (thread-safe)

## Future Enhancements

Coming soon:
- [ ] Dynamic applet loading from RamFS
- [ ] I2C EEPROM applet distribution
- [ ] Network download and installation
- [ ] Inter-applet communication (IPC)
- [ ] Display/graphics API bindings
- [ ] Encoder/input API bindings
- [ ] File system API access

---

**Ready to create your first applet?** Start with a simple `os.console.log()` example and build from there!
