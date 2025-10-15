# dialOS Kernel API Specification

## Table of Contents

| Namespace | Description | Implemented | Planned | Blocked |
|-----------|-------------|-------------|---------|---------|
| [`os.console.*`](#1-console-apis-osconsole) | Console logging and output | 1 | 3 | 0 |
| [`os.display.*`](#2-display-apis-osdisplay) | Display and graphics rendering | 2 | 8 | 0 |
| [`os.encoder.*`](#3-encoder-apis-osencoder) | Rotary encoder input | 2 | 2 | 2 |
| [`os.touch.*`](#4-touch-apis-ostouch) | Touchscreen input | 0 | 1 | 3 |
| [`os.system.*`](#5-system-apis-ossystem) | System time and control | 2 | 3 | 0 |
| [`os.memory.*`](#6-memory-apis-osmemory) | Memory management | 0 | 4 | 0 |
| [`os.file.*`](#7-file-apis-osfile) | File operations | 0 | 7 | 0 |
| [`os.dir.*`](#8-directory-apis-osdir) | Directory operations | 0 | 4 | 0 |
| [`os.gpio.*`](#9-gpio-apis-osgpio) | GPIO pin control | 0 | 5 | 0 |
| [`os.i2c.*`](#10-i2c-apis-osi2c) | I2C communication | 0 | 3 | 2 |
| [`os.buzzer.*`](#11-buzzer-apis-osbuzzer) | Buzzer/audio output | 0 | 3 | 0 |
| [`os.timer.*`](#12-timer-apis-ostimer) | Timers and intervals | 0 | 0 | 4 |
| [`os.rfid.*`](#13-rfid-apis-osrfid) | RFID card reader | 0 | 2 | 2 |
| [`os.power.*`](#14-power-apis-ospower) | Power management | 0 | 2 | 1 |
| [`os.app.*`](#15-app-apis-osapp) | Application lifecycle | 0 | 2 | 4 |
| [`os.storage.*`](#16-storage-apis-osstorage) | Storage device management | 0 | 2 | 2 |
| [`os.sensor.*`](#17-sensor-apis-ossensor) | Hardware sensor interface | 0 | 2 | 2 |
| [`os.events.*`](#18-events-apis-osevents) | Event system | 0 | 0 | 3 |
| [`os.wifi.*`](#19-wifi-apis-oswifi) | WiFi connectivity | 0 | 4 | 0 |
| [`os.http.*`](#20-http-apis-oshttp) | HTTP client | 0 | 0 | 2 |
| [`os.ipc.*`](#21-ipc-apis-osipc) | Inter-process communication | 0 | 1 | 2 |
| **Total** | **21 namespaces** | **130 functions** | **7** | **88** | **35** |

**Legend:**
- ✅ **Implemented** - Function is working and tested
- 🔜 **Planned** - Needs implementation
- 🔒 **Blocked** - Requires function/callback support in VM

---

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

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.console.log()` | `message: string` | `null` | ✅ Implemented |
| `os.console.warn()` | `message: string` | `null` | 🔜 Planned |
| `os.console.error()` | `message: string` | `null` | 🔜 Planned |
| `os.console.clear()` | none | `null` | 🔜 Planned |

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

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.display.clear()` | `color: int` | `null` | ✅ Implemented |
| `os.display.drawText()` | `x: int, y: int, text: string, color: int, size: int` | `null` | ✅ Implemented |
| `os.display.drawPixel()` | `x: int, y: int, color: int` | `null` | 🔜 Planned |
| `os.display.drawLine()` | `x1: int, y1: int, x2: int, y2: int, color: int` | `null` | 🔜 Planned |
| `os.display.drawRect()` | `x: int, y: int, w: int, h: int, color: int, filled: bool` | `null` | 🔜 Planned |
| `os.display.drawCircle()` | `x: int, y: int, r: int, color: int, filled: bool` | `null` | 🔜 Planned |
| `os.display.drawImage()` | `x: int, y: int, imageData: object` | `null` | 🔜 Planned |
| `os.display.setBrightness()` | `level: int` | `null` | 🔜 Planned |
| `os.display.getSize()` | none | `object` | 🔜 Planned |
| `os.display.setTitle()` | `text: string` | `null` | 🔜 Planned |

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

### `os.display.drawImage(x: int, y: int, imageData: object) -> null`
Draw bitmap image
- **Parameters**:
  - `x, y` (int) - Top-left position
  - `imageData` (object) - Image data object
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

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.encoder.getButton()` | none | `bool` | ✅ Implemented |
| `os.encoder.getDelta()` | none | `int` | ✅ Implemented |
| `os.encoder.getPosition()` | none | `int` | 🔜 Planned |
| `os.encoder.reset()` | none | `null` | 🔜 Planned |
| `os.encoder.onTurn()` | `callback: function` | `null` | 🔒 Blocked |
| `os.encoder.onButton()` | `callback: function` | `null` | 🔒 Blocked |

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

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.touch.getPosition()` | none | `object` | 🔜 Planned |
| `os.touch.onPress()` | `callback: function` | `null` | 🔒 Blocked |
| `os.touch.onRelease()` | `callback: function` | `null` | 🔒 Blocked |
| `os.touch.onDrag()` | `callback: function` | `null` | 🔒 Blocked |

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

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.system.getTime()` | none | `int` | ✅ Implemented |
| `os.system.sleep()` | `ms: int` | `null` | ✅ Implemented |
| `os.system.yield()` | none | `null` | 🔜 Planned |
| `os.system.getRTC()` | none | `object` | 🔜 Planned |
| `os.system.setRTC()` | `datetime: object` | `null` | 🔜 Planned |

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

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.memory.getAvailable()` | none | `int` | 🔜 Planned |
| `os.memory.getUsage()` | none | `int` | 🔜 Planned |
| `os.memory.allocate()` | `size: int` | `int` | 🔜 Planned |
| `os.memory.free()` | `handle: int` | `null` | 🔜 Planned |

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

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.file.open()` | `path: string, mode: string` | `int` | 🔜 Planned |
| `os.file.read()` | `handle: int, size: int` | `string` | 🔜 Planned |
| `os.file.write()` | `handle: int, data: string` | `int` | 🔜 Planned |
| `os.file.close()` | `handle: int` | `null` | 🔜 Planned |
| `os.file.exists()` | `path: string` | `bool` | 🔜 Planned |
| `os.file.delete()` | `path: string` | `bool` | 🔜 Planned |
| `os.file.size()` | `path: string` | `int` | 🔜 Planned |

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

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.dir.list()` | `path: string` | `array` | 🔜 Planned |
| `os.dir.create()` | `path: string` | `bool` | 🔜 Planned |
| `os.dir.delete()` | `path: string` | `bool` | 🔜 Planned |
| `os.dir.exists()` | `path: string` | `bool` | 🔜 Planned |

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

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.gpio.pinMode()` | `pin: int, mode: string` | `null` | 🔜 Planned |
| `os.gpio.digitalWrite()` | `pin: int, value: int` | `null` | 🔜 Planned |
| `os.gpio.digitalRead()` | `pin: int` | `int` | 🔜 Planned |
| `os.gpio.analogWrite()` | `pin: int, value: int` | `null` | 🔜 Planned |
| `os.gpio.analogRead()` | `pin: int` | `int` | 🔜 Planned |

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

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.i2c.scan()` | none | `array` | 🔜 Planned |
| `os.i2c.write()` | `address: int, data: string` | `bool` | 🔜 Planned |
| `os.i2c.read()` | `address: int, length: int` | `string` | 🔜 Planned |
| `os.i2c.onDeviceAdded()` | `callback: function` | `null` | 🔒 Blocked |
| `os.i2c.onDeviceRemoved()` | `callback: function` | `null` | 🔒 Blocked |

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

### `os.i2c.onDeviceAdded(callback: function) -> null`
New device detected event
- **Parameters**: `callback` (function) - Called when I2C device detected
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

### `os.i2c.onDeviceRemoved(callback: function) -> null`
Device removed event
- **Parameters**: `callback` (function) - Called when I2C device removed
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

---

## 11. Buzzer APIs (`os.buzzer.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.buzzer.beep()` | `frequency: int, duration: int` | `null` | 🔜 Planned |
| `os.buzzer.playMelody()` | `notes: array` | `null` | 🔜 Planned |
| `os.buzzer.stop()` | none | `null` | 🔜 Planned |

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

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.timer.setTimeout()` | `callback: function, ms: int` | `int` | 🔒 Blocked |
| `os.timer.setInterval()` | `callback: function, ms: int` | `int` | 🔒 Blocked |
| `os.timer.clearTimeout()` | `id: int` | `null` | 🔒 Blocked |
| `os.timer.clearInterval()` | `id: int` | `null` | 🔒 Blocked |

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

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.rfid.read()` | none | `string` | 🔜 Planned |
| `os.rfid.isPresent()` | none | `bool` | 🔜 Planned |
| `os.rfid.onCardDetected()` | `callback: function` | `null` | 🔒 Blocked |
| `os.rfid.onCardRemoved()` | `callback: function` | `null` | 🔒 Blocked |

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

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.power.sleep()` | none | `null` | 🔜 Planned |
| `os.power.getBatteryLevel()` | none | `int` | 🔜 Planned |
| `os.power.isCharging()` | none | `bool` | 🔜 Planned |
| `os.power.onLowBattery()` | `callback: function` | `null` | 🔒 Blocked |

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

### `os.power.onLowBattery(callback: function) -> null`
Low battery event
- **Parameters**: `callback` (function) - Called when battery is low
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

---

## 15. App APIs (`os.app.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.app.exit()` | none | `null` | 🔜 Planned |
| `os.app.getInfo()` | none | `object` | 🔜 Planned |
| `os.app.onLoad()` | `callback: function` | `null` | 🔒 Blocked |
| `os.app.onSuspend()` | `callback: function` | `null` | 🔒 Blocked |
| `os.app.onResume()` | `callback: function` | `null` | 🔒 Blocked |
| `os.app.onUnload()` | `callback: function` | `null` | 🔒 Blocked |

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

### `os.app.onLoad(callback: function) -> null`
Called when app starts
- **Parameters**: `callback` (function) - Initialization callback
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

### `os.app.onSuspend(callback: function) -> null`
Called when app is frozen
- **Parameters**: `callback` (function) - Suspend callback
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

### `os.app.onResume(callback: function) -> null`
Called when app becomes active again
- **Parameters**: `callback` (function) - Resume callback
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

### `os.app.onUnload(callback: function) -> null`
Called when app exits
- **Parameters**: `callback` (function) - Cleanup callback
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

---

## 16. Storage APIs (`os.storage.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.storage.getMounted()` | none | `array` | 🔜 Planned |
| `os.storage.getInfo()` | `device: string` | `object` | 🔜 Planned |
| `os.storage.onMount()` | `callback: function` | `null` | 🔒 Blocked |
| `os.storage.onUnmount()` | `callback: function` | `null` | 🔒 Blocked |

### `os.storage.getMounted() -> array`
List mounted storage devices
- **Parameters**: none
- **Returns**: array - Array of mounted device names
- **Status**: 🔜 Planned

### `os.storage.getInfo(device: string) -> object`
Get device info (size, free space, type)
- **Parameters**: `device` (string) - Device name
- **Returns**: object with `{size, free, type}`
- **Status**: 🔜 Planned

### `os.storage.onMount(callback: function) -> null`
Storage device mounted event
- **Parameters**: `callback` (function) - Called when device mounted
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

### `os.storage.onUnmount(callback: function) -> null`
Storage device removed event
- **Parameters**: `callback` (function) - Called when device unmounted
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

---

## 17. Sensor APIs (`os.sensor.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.sensor.attach()` | `port: string, type: string` | `int` | 🔜 Planned |
| `os.sensor.read()` | `handle: int` | `object` | 🔜 Planned |
| `os.sensor.detach()` | `handle: int` | `null` | 🔒 Blocked |
| `os.sensor.onData()` | `handle: int, callback: function` | `null` | 🔒 Blocked |

### `os.sensor.attach(port: string, type: string) -> int`
Attach sensor to PORT.A/B
- **Parameters**:
  - `port` (string) - "PORT.A" or "PORT.B"
  - `type` (string) - Sensor type identifier
- **Returns**: int - Sensor handle (or -1 on failure)
- **Status**: 🔜 Planned

### `os.sensor.read(handle: int) -> object`
Read sensor value
- **Parameters**: `handle` (int) - Sensor handle
- **Returns**: object - Sensor data
- **Status**: 🔜 Planned

### `os.sensor.detach(handle: int) -> null`
Detach sensor
- **Parameters**: `handle` (int) - Sensor handle
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

### `os.sensor.onData(handle: int, callback: function) -> null`
Sensor data event
- **Parameters**:
  - `handle` (int) - Sensor handle
  - `callback` (function) - Called when sensor has new data
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

---

## 18. Events APIs (`os.events.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.events.emit()` | `eventName: string, data: any` | `null` | 🔒 Blocked |
| `os.events.on()` | `eventName: string, callback: function` | `null` | 🔒 Blocked |
| `os.events.off()` | `eventName: string, callback: function` | `null` | 🔒 Blocked |

### `os.events.emit(eventName: string, data: any) -> null`
Emit custom event
- **Parameters**:
  - `eventName` (string) - Event name
  - `data` (any) - Event data
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

### `os.events.on(eventName: string, callback: function) -> null`
Listen for event
- **Parameters**:
  - `eventName` (string) - Event name
  - `callback` (function) - Event handler
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

### `os.events.off(eventName: string, callback: function) -> null`
Remove event listener
- **Parameters**:
  - `eventName` (string) - Event name
  - `callback` (function) - Event handler to remove
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

---

## 19. WiFi APIs (`os.wifi.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.wifi.connect()` | `ssid: string, password: string` | `bool` | 🔜 Planned |
| `os.wifi.disconnect()` | none | `null` | 🔜 Planned |
| `os.wifi.getStatus()` | none | `object` | 🔜 Planned |
| `os.wifi.getIP()` | none | `string` | 🔜 Planned |

### `os.wifi.connect(ssid: string, password: string) -> bool`
Connect to WiFi
- **Parameters**:
  - `ssid` (string) - Network SSID
  - `password` (string) - Network password
- **Returns**: bool - Connection success
- **Status**: 🔜 Planned (Future)

### `os.wifi.disconnect() -> null`
Disconnect WiFi
- **Parameters**: none
- **Returns**: null
- **Status**: 🔜 Planned (Future)

### `os.wifi.getStatus() -> object`
Get connection status
- **Parameters**: none
- **Returns**: object with `{connected: bool, ssid: string, rssi: int}`
- **Status**: 🔜 Planned (Future)

### `os.wifi.getIP() -> string`
Get IP address
- **Parameters**: none
- **Returns**: string - IP address or empty string
- **Status**: 🔜 Planned (Future)

---

## 20. HTTP APIs (`os.http.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.http.get()` | `url: string, callback: function` | `null` | 🔒 Blocked |
| `os.http.post()` | `url: string, data: string, callback: function` | `null` | 🔒 Blocked |

### `os.http.get(url: string, callback: function) -> null`
HTTP GET request
- **Parameters**:
  - `url` (string) - Request URL
  - `callback` (function) - Called with response
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support, Future)

### `os.http.post(url: string, data: string, callback: function) -> null`
HTTP POST request
- **Parameters**:
  - `url` (string) - Request URL
  - `data` (string) - Request body
  - `callback` (function) - Called with response
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support, Future)

---

## 21. IPC APIs (`os.ipc.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.ipc.send()` | `appId: string, message: any` | `bool` | 🔜 Planned |
| `os.ipc.broadcast()` | `message: any` | `null` | 🔒 Blocked |
| `os.ipc.onMessage()` | `callback: function` | `null` | 🔒 Blocked |

### `os.ipc.send(appId: string, message: any) -> bool`
Send message to another app
- **Parameters**:
  - `appId` (string) - Target app ID
  - `message` (any) - Message data
- **Returns**: bool - Send success
- **Status**: 🔜 Planned

### `os.ipc.broadcast(message: any) -> null`
Broadcast to all apps
- **Parameters**: `message` (any) - Message data
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

### `os.ipc.onMessage(callback: function) -> null`
Receive messages
- **Parameters**: `callback` (function) - Message handler
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

---

## Implementation Status Summary

### ✅ Implemented (7 functions)
- `os.console.log()`
- `os.display.clear()`
- `os.display.drawText()`
- `os.encoder.getButton()`
- `os.encoder.getDelta()`
- `os.system.getTime()`
- `os.system.sleep()`

### 🔜 Planned (88 functions)
All other non-callback APIs require:
1. Platform interface expansion
2. VM CALL_NATIVE dispatch updates
3. Compiler recognition of os.* calls

### 🔒 Blocked (35 callback-based APIs)
Require function/callback support in VM:
- **App Lifecycle**: onLoad, onSuspend, onResume, onUnload (4)
- **Input Events**: encoder.onTurn, encoder.onButton, touch events (6)
- **Timers**: setTimeout, setInterval, clear functions (4)
- **I2C Events**: onDeviceAdded, onDeviceRemoved (2)
- **RFID Events**: onCardDetected, onCardRemoved (2)
- **Power Events**: onLowBattery (1)
- **Storage Events**: onMount, onUnmount (2)
- **Sensor Events**: onData, detach (2)
- **Event System**: emit, on, off (3)
- **HTTP**: get, post (2)
- **IPC**: broadcast, onMessage (2)
- **Timer Clears**: clearTimeout, clearInterval (2)
- **Sensor**: detach (1)
- **Timer Intervals**: setInterval (already counted above)
- **Total Blocked**: 35 functions

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
