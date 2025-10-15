# dialOS Changelog

## [Unreleased] - 2025-10-13

### Added - VM Applet System Generalization

#### New Features
- **Applet Registry System**: Generic framework for managing multiple dialScript applets
  - `VMApplet` struct for applet metadata (name, bytecode, timing, repeat mode)
  - Registry array for centralized applet management
  - `createVMTask()` helper function for easy applet loading by name

- **Generalized VM Task**: `vmAppletTask()` replaces hardcoded `vmCounterTask()`
  - Supports arbitrary applets via parameter passing
  - Configurable execution intervals (0ms to any duration)
  - Repeat modes: one-shot or continuous execution
  - Improved error handling and logging

- **Compiler Enhancement**: `--c-array` output option
  - Generates C/C++ header files with bytecode arrays
  - Eliminates PowerShell file caching issues
  - Format: `const unsigned char APPLET_NAME[] = {...};`
  - Usage: `compile.exe input.ds output.h --c-array`

#### Bug Fixes
- **CRITICAL**: Fixed dangling reference bug in VM task
  - Issue: Local `BytecodeModule` variable destroyed after initialization
  - Symptom: ESP32 crash or PC corruption after first execution
  - Fix: Heap-allocated module with static pointer storage
  - Result: Stable repeated execution (182+ cycles verified)

- **Fixed**: CALL opcode stack layout for method calls
  - Corrected receiver-on-top convention: `[..., arg1, argN, receiver]`
  - Proper handling of `os.console.print()` native function calls

#### Architecture Changes
- **VM Task Structure**:
  ```cpp
  // Old (hardcoded):
  void vmCounterTask(byte taskId, void *param)
  
  // New (generalized):
  void vmAppletTask(byte taskId, void *param)
  Task* createVMTask(const char *appletName)
  ```

- **Applet Registration**:
  ```cpp
  static VMApplet appletRegistry[] = {
      {"counter", COUNTER_APPLET, COUNTER_APPLET_SIZE, 2000, true},
      // Easy to add more applets
  };
  ```

#### Documentation
- Created `docs/VM_APPLET_GUIDE.md` with:
  - Step-by-step applet creation guide
  - Execution mode documentation
  - dialScript language reference
  - Debugging tips and troubleshooting
  - Example applets and patterns

#### Verified Functionality
- ✅ Counter applet executes every 2 seconds indefinitely
- ✅ No crashes with proper memory management
- ✅ Clean logs showing applet name and execution count
- ✅ Task creation via `createVMTask()` helper
- ✅ 182+ successful execution cycles tested

#### Performance
- Firmware size: 673KB flash (20.1%), 28KB RAM (8.6%)
- VM heap: 8192 bytes per applet
- Task stack: 16KB per VM task
- Execution: ~1ms per cycle (PC:0-29)

### Technical Details

#### Memory Management
- BytecodeModule: Heap-allocated with static pointer
- VMState: Persistent across executions
- ValuePool: 8192 bytes default heap size
- Platform: Shared ESP32Platform instance (thread-safe)

#### Execution Flow
1. First run: Deserialize bytecode, create VM state
2. Subsequent runs: Reset VM (clear stack/PC, preserve `os` global)
3. Execute up to 1000 instructions per task tick
4. Handle result: FINISHED → reset + sleep, ERROR → retry, YIELD → continue

#### dialScript Limitations
- ❌ Template literals cause compiler bug (no `${variable}` syntax)
- ✅ Use separate `os.console.print()` calls instead

### Migration Guide

#### For Adding New Applets
1. Write dialScript code in `scripts/`
2. Compile with `--c-array` flag
3. Include header in `main.cpp`
4. Add entry to `appletRegistry[]`
5. Create task with `createVMTask("name")`

#### Code Changes
Replace:
```cpp
Task *task6 = scheduler->createTask("VMCounter", vmCounterTask, nullptr, 
                                   16384, TaskPriority::NORMAL);
```

With:
```cpp
Task *vmTask = createVMTask("counter");
```

### Future Roadmap
- [ ] Dynamic applet loading from RamFS
- [ ] I2C EEPROM applet distribution
- [ ] Network applet download
- [ ] Display/graphics API bindings
- [ ] Encoder/input API bindings
- [ ] Inter-applet communication (IPC)

---

## Development Notes

### Build System
- Platform: PlatformIO with Arduino framework
- Board: `m5stack-stamps3` (ESP32-S3)
- Dependencies: `M5Dial@^1.0.3`

### Build Commands
```bash
# Compile applet
cd compiler
.\compile.exe ..\scripts\applet.ds ..\src\applet_data.h --c-array

# Build firmware
pio run -e m5stack-stamps3

# Upload and monitor
pio run -e m5stack-stamps3 --target upload --target monitor
```

### Debugging
- Serial monitor at 115200 baud
- Look for applet load/execution messages
- Check for VM errors with descriptive messages
- Monitor memory usage via kernel logs
