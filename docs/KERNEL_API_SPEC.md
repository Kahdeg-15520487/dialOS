# dialOS Kernel API Specification

## Data Types
All parameters use the VM Value system:
- `int` - Int32 value
- `float` - Float32 value
- `string` - String value
- `bool` - Boolean value
- `object` - Object reference
- `array` - Array reference
- `callback` - Function reference (future)

## API Namespace: `os.*`
All APIs under `os.*` namespace are **CALL_NATIVE** opcodes.

---

## 1. Console APIs (`os.console.*`)

### `os.console.log(message: string) -> null`
Log informational message to console
- **Parameters**: `message` (string) - Message to log
- **Returns**: null
- **Status**: ✅ Implemented

### `os.console.warn(message: string) -> null`
Log warning message
- **Parameters**: `message` (string) - Warning message
- **Returns**: null
- **Status**: 🔜 Planned

### `os.console.error(message: string) -> null`
Log error message
- **Parameters**: `message` (string) - Error message
- **Returns**: null
- **Status**: 🔜 Planned

### `os.console.clear() -> null`
Clear console buffer
- **Parameters**: none
- **Returns**: null
- **Status**: 🔜 Planned

---

## 2. Display APIs (`os.display.*`)

### `os.display.clear(color: int) -> null`
Clear screen with specified color
- **Parameters**: `color` (int) - RGB565 color value (0x0000-0xFFFF)
- **Returns**: null
- **Status**: ✅ Implemented

### `os.display.drawText(x: int, y: int, text: string, color: int, size: int) -> null`
Render text at position
- **Parameters**:
  - `x` (int) - X coordinate (0-239)
  - `y` (int) - Y coordinate (0-239)
  - `text` (string) - Text to display
  - `color` (int) - RGB565 color
  - `size` (int) - Font size (1-8)
- **Returns**: null
- **Status**: ✅ Implemented

### `os.display.drawPixel(x: int, y: int, color: int) -> null`
Draw single pixel
- **Parameters**:
  - `x` (int) - X coordinate
  - `y` (int) - Y coordinate
  - `color` (int) - RGB565 color
- **Returns**: null
- **Status**: 🔜 Planned

### `os.display.drawLine(x1: int, y1: int, x2: int, y2: int, color: int) -> null`
Draw line between two points
- **Parameters**:
  - `x1, y1` (int) - Start coordinates
  - `x2, y2` (int) - End coordinates
  - `color` (int) - RGB565 color
- **Returns**: null
- **Status**: 🔜 Planned

### `os.display.drawRect(x: int, y: int, w: int, h: int, color: int, filled: bool) -> null`
Draw rectangle
- **Parameters**:
  - `x, y` (int) - Top-left corner
  - `w, h` (int) - Width and height
  - `color` (int) - RGB565 color
  - `filled` (bool) - Fill rectangle if true
- **Returns**: null
- **Status**: 🔜 Planned

### `os.display.drawCircle(x: int, y: int, r: int, color: int, filled: bool) -> null`
Draw circle
- **Parameters**:
  - `x, y` (int) - Center coordinates
  - `r` (int) - Radius
  - `color` (int) - RGB565 color
  - `filled` (bool) - Fill circle if true
- **Returns**: null
- **Status**: 🔜 Planned

### `os.display.setBrightness(level: int) -> null`
Adjust backlight brightness
- **Parameters**: `level` (int) - Brightness level (0-255)
- **Returns**: null
- **Status**: 🔜 Planned

### `os.display.getSize() -> object`
Get display dimensions
- **Parameters**: none
- **Returns**: object with `{width: 240, height: 240}`
- **Status**: 🔜 Planned

### `os.display.setTitle(text: string) -> null`
Set app title bar text
- **Parameters**: `text` (string) - Title text
- **Returns**: null
- **Status**: 🔜 Planned

---

## 3. Encoder APIs (`os.encoder.*`)

### `os.encoder.getButton() -> bool`
Get current button state
- **Parameters**: none
- **Returns**: bool - true if pressed, false otherwise
- **Status**: ✅ Implemented

### `os.encoder.getDelta() -> int`
Get encoder position change
- **Parameters**: none
- **Returns**: int - Position delta since last call
- **Status**: ✅ Implemented

### `os.encoder.getPosition() -> int`
Get absolute encoder position
- **Parameters**: none
- **Returns**: int - Current encoder position
- **Status**: 🔜 Planned

### `os.encoder.reset() -> null`
Reset encoder position to zero
- **Parameters**: none
- **Returns**: null
- **Status**: 🔜 Planned

### `os.encoder.onTurn(callback: function) -> null`
Register encoder rotation callback
- **Parameters**: `callback` (function) - Called with delta: +/-
- **Returns**: null
- **Status**: 🔜 Planned (requires function support)

### `os.encoder.onButton(callback: function) -> null`
Register button press callback
- **Parameters**: `callback` (function) - Called on button press
- **Returns**: null
- **Status**: 🔜 Planned (requires function support)

---

## 4. Touch APIs (`os.touch.*`)

### `os.touch.getPosition() -> object`
Get current touch coordinates
- **Parameters**: none
- **Returns**: object with `{x: int, y: int, pressed: bool}`
- **Status**: 🔜 Planned

### `os.touch.onPress(callback: function) -> null`
Register touch press callback
- **Parameters**: `callback` (function) - Called with {x, y}
- **Returns**: null
- **Status**: 🔜 Planned (requires function support)

### `os.touch.onRelease(callback: function) -> null`
Register touch release callback
- **Parameters**: `callback` (function)
- **Returns**: null
- **Status**: 🔜 Planned (requires function support)

### `os.touch.onDrag(callback: function) -> null`
Register touch drag callback
- **Parameters**: `callback` (function) - Called with {x, y, dx, dy}
- **Returns**: null
- **Status**: 🔜 Planned (requires function support)

---

## 5. System APIs (`os.system.*`)

### `os.system.getTime() -> int`
Get system uptime in milliseconds
- **Parameters**: none
- **Returns**: int - Milliseconds since boot
- **Status**: ✅ Implemented

### `os.system.sleep(ms: int) -> null`
Yield CPU for specified time
- **Parameters**: `ms` (int) - Milliseconds to sleep
- **Returns**: null
- **Status**: ✅ Implemented

### `os.system.yield() -> null`
Cooperative yield to scheduler
- **Parameters**: none
- **Returns**: null
- **Status**: 🔜 Planned

### `os.system.getRTC() -> object`
Get real-time clock value
- **Parameters**: none
- **Returns**: object with `{year, month, day, hour, minute, second}`
- **Status**: 🔜 Planned

### `os.system.setRTC(datetime: object) -> null`
Set RTC
- **Parameters**: `datetime` (object) - Date/time object
- **Returns**: null
- **Status**: 🔜 Planned

---

## 6. Memory APIs (`os.memory.*`)

### `os.memory.getAvailable() -> int`
Query available heap
- **Parameters**: none
- **Returns**: int - Available bytes
- **Status**: 🔜 Planned

### `os.memory.getUsage() -> int`
Get current app memory usage
- **Parameters**: none
- **Returns**: int - Used bytes
- **Status**: 🔜 Planned

### `os.memory.allocate(size: int) -> int`
Request memory allocation
- **Parameters**: `size` (int) - Bytes to allocate
- **Returns**: int - Handle/address (or -1 on failure)
- **Status**: 🔜 Planned

### `os.memory.free(handle: int) -> null`
Release memory
- **Parameters**: `handle` (int) - Memory handle
- **Returns**: null
- **Status**: 🔜 Planned

---

## 7. File APIs (`os.file.*`)

### `os.file.open(path: string, mode: string) -> int`
Open file
- **Parameters**:
  - `path` (string) - File path
  - `mode` (string) - 'r', 'w', or 'a'
- **Returns**: int - File handle (or -1 on failure)
- **Status**: 🔜 Planned

### `os.file.read(handle: int, size: int) -> string`
Read from file
- **Parameters**:
  - `handle` (int) - File handle
  - `size` (int) - Bytes to read
- **Returns**: string - Data read
- **Status**: 🔜 Planned

### `os.file.write(handle: int, data: string) -> int`
Write to file
- **Parameters**:
  - `handle` (int) - File handle
  - `data` (string) - Data to write
- **Returns**: int - Bytes written
- **Status**: 🔜 Planned

### `os.file.close(handle: int) -> null`
Close file
- **Parameters**: `handle` (int) - File handle
- **Returns**: null
- **Status**: 🔜 Planned

### `os.file.exists(path: string) -> bool`
Check if file exists
- **Parameters**: `path` (string) - File path
- **Returns**: bool
- **Status**: 🔜 Planned

### `os.file.delete(path: string) -> bool`
Delete file
- **Parameters**: `path` (string) - File path
- **Returns**: bool - Success status
- **Status**: 🔜 Planned

### `os.file.size(path: string) -> int`
Get file size
- **Parameters**: `path` (string) - File path
- **Returns**: int - File size in bytes
- **Status**: 🔜 Planned

---

## 8. Directory APIs (`os.dir.*`)

### `os.dir.list(path: string) -> array`
List directory contents
- **Parameters**: `path` (string) - Directory path
- **Returns**: array - Array of filenames
- **Status**: 🔜 Planned

### `os.dir.create(path: string) -> bool`
Create directory
- **Parameters**: `path` (string) - Directory path
- **Returns**: bool - Success status
- **Status**: 🔜 Planned

### `os.dir.delete(path: string) -> bool`
Delete directory
- **Parameters**: `path` (string) - Directory path
- **Returns**: bool - Success status
- **Status**: 🔜 Planned

### `os.dir.exists(path: string) -> bool`
Check if directory exists
- **Parameters**: `path` (string) - Directory path
- **Returns**: bool
- **Status**: 🔜 Planned

---

## 9. GPIO APIs (`os.gpio.*`)

### `os.gpio.pinMode(pin: int, mode: string) -> null`
Set pin mode
- **Parameters**:
  - `pin` (int) - GPIO pin number
  - `mode` (string) - "INPUT", "OUTPUT", "INPUT_PULLUP"
- **Returns**: null
- **Status**: 🔜 Planned

### `os.gpio.digitalWrite(pin: int, value: int) -> null`
Write digital value
- **Parameters**:
  - `pin` (int) - GPIO pin number
  - `value` (int) - HIGH (1) or LOW (0)
- **Returns**: null
- **Status**: 🔜 Planned

### `os.gpio.digitalRead(pin: int) -> int`
Read digital value
- **Parameters**: `pin` (int) - GPIO pin number
- **Returns**: int - HIGH (1) or LOW (0)
- **Status**: 🔜 Planned

### `os.gpio.analogWrite(pin: int, value: int) -> null`
PWM output
- **Parameters**:
  - `pin` (int) - GPIO pin number
  - `value` (int) - PWM value (0-255)
- **Returns**: null
- **Status**: 🔜 Planned

### `os.gpio.analogRead(pin: int) -> int`
ADC read
- **Parameters**: `pin` (int) - GPIO pin number
- **Returns**: int - ADC value (0-4095)
- **Status**: 🔜 Planned

---

## 10. I2C APIs (`os.i2c.*`)

### `os.i2c.scan() -> array`
Scan for I2C devices
- **Parameters**: none
- **Returns**: array - Array of I2C addresses found
- **Status**: 🔜 Planned

### `os.i2c.write(address: int, data: string) -> bool`
Write to I2C device
- **Parameters**:
  - `address` (int) - I2C device address
  - `data` (string) - Data to write
- **Returns**: bool - Success status
- **Status**: 🔜 Planned

### `os.i2c.read(address: int, length: int) -> string`
Read from I2C device
- **Parameters**:
  - `address` (int) - I2C device address
  - `length` (int) - Bytes to read
- **Returns**: string - Data read
- **Status**: 🔜 Planned

---

## 11. Buzzer APIs (`os.buzzer.*`)

### `os.buzzer.beep(frequency: int, duration: int) -> null`
Play tone
- **Parameters**:
  - `frequency` (int) - Frequency in Hz
  - `duration` (int) - Duration in milliseconds
- **Returns**: null
- **Status**: 🔜 Planned

### `os.buzzer.playMelody(notes: array) -> null`
Play note sequence
- **Parameters**: `notes` (array) - Array of {freq, duration} objects
- **Returns**: null
- **Status**: 🔜 Planned

### `os.buzzer.stop() -> null`
Stop buzzing
- **Parameters**: none
- **Returns**: null
- **Status**: 🔜 Planned

---

## 12. Timer APIs (`os.timer.*`)

### `os.timer.setTimeout(callback: function, ms: int) -> int`
One-shot timer
- **Parameters**:
  - `callback` (function) - Function to call
  - `ms` (int) - Delay in milliseconds
- **Returns**: int - Timer ID
- **Status**: 🔜 Planned (requires function support)

### `os.timer.setInterval(callback: function, ms: int) -> int`
Recurring timer
- **Parameters**:
  - `callback` (function) - Function to call
  - `ms` (int) - Interval in milliseconds
- **Returns**: int - Timer ID
- **Status**: 🔜 Planned (requires function support)

### `os.timer.clearTimeout(id: int) -> null`
Cancel timeout
- **Parameters**: `id` (int) - Timer ID
- **Returns**: null
- **Status**: 🔜 Planned

### `os.timer.clearInterval(id: int) -> null`
Cancel interval
- **Parameters**: `id` (int) - Timer ID
- **Returns**: null
- **Status**: 🔜 Planned

---

## 13. RFID APIs (`os.rfid.*`)

### `os.rfid.read() -> string`
Read card UID
- **Parameters**: none
- **Returns**: string - Card UID or empty string
- **Status**: 🔜 Planned

### `os.rfid.isPresent() -> bool`
Check if card is present
- **Parameters**: none
- **Returns**: bool
- **Status**: 🔜 Planned

### `os.rfid.onCardDetected(callback: function) -> null`
Card detected event
- **Parameters**: `callback` (function) - Called with UID
- **Returns**: null
- **Status**: 🔜 Planned (requires function support)

### `os.rfid.onCardRemoved(callback: function) -> null`
Card removed event
- **Parameters**: `callback` (function)
- **Returns**: null
- **Status**: 🔜 Planned (requires function support)

---

## 14. Power APIs (`os.power.*`)

### `os.power.sleep() -> null`
Enter sleep mode (wake on button/RTC)
- **Parameters**: none
- **Returns**: null
- **Status**: 🔜 Planned

### `os.power.getBatteryLevel() -> int`
Get battery percentage
- **Parameters**: none
- **Returns**: int - Battery level (0-100)
- **Status**: 🔜 Planned

### `os.power.isCharging() -> bool`
Check if charging
- **Parameters**: none
- **Returns**: bool
- **Status**: 🔜 Planned

---

## 15. App APIs (`os.app.*`)

### `os.app.exit() -> null`
Terminate current app
- **Parameters**: none
- **Returns**: null (does not return)
- **Status**: 🔜 Planned

### `os.app.getInfo() -> object`
Get app metadata
- **Parameters**: none
- **Returns**: object with `{name, version, memoryUsage}`
- **Status**: 🔜 Planned

---

## Implementation Status Summary

### ✅ Implemented (8 functions)
- `os.console.log()`
- `os.display.clear()`
- `os.display.drawText()`
- `os.encoder.getButton()`
- `os.encoder.getDelta()`
- `os.system.getTime()`
- `os.system.sleep()`

### 🔜 Planned (80+ functions)
All other APIs require:
1. Platform interface expansion
2. VM CALL_NATIVE dispatch updates
3. Compiler recognition of os.* calls

### 🔒 Blocked (15+ callback-based APIs)
Require function/callback support in VM:
- Event handlers (onTurn, onPress, etc.)
- Timers (setTimeout, setInterval)
- IPC message handlers

---

## Adding New Native Functions

### Step 1: Update PlatformInterface
Add method to `src/vm/platform.h`:
```cpp
virtual void newFunction(params...) = 0;
```

### Step 2: Implement in ESP32Platform
Add implementation to `src/main.cpp`:
```cpp
void newFunction(params...) override {
  // Implementation
}
```

### Step 3: Add VM Dispatch
Add case to `CALL_NATIVE` in `src/vm/vm_core.cpp`:
```cpp
else if (funcName == "newFunction") {
  // Pop arguments, call platform, push result
}
```

### Step 4: Compiler Recognition
The compiler already handles all `os.*` calls as `CALL_NATIVE` via `isOsNamespaceCall()`.

### Step 5: Test
Write dialScript code using new API and verify.
