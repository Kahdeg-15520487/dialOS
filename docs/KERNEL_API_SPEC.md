# dialOS Kernel API Specification

## Table of Contents

| Namespace | Description | Implemented | Planned | Blocked |
|-----------|-------------|-------------|---------|---------|
| [`os.console.*`](#1-console-apis-osconsole) | Console logging and output | 5 | 0 | 0 |
| [`os.display.*`](#2-display-apis-osdisplay) | Display and graphics rendering | 10 | 0 | 0 |
| [`os.encoder.*`](#3-encoder-apis-osencoder) | Rotary encoder input | 4 | 0 | 2 |
| [`os.touch.*`](#4-touch-apis-ostouch) | Touchscreen input | 3 | 1 | 3 |
| [`os.system.*`](#5-system-apis-ossystem) | System time and control | 5 | 0 | 0 |
| [`os.memory.*`](#6-memory-apis-osmemory) | Memory management | 2 | 2 | 0 |
| [`os.file.*`](#7-file-apis-osfile) | File operations | 7 | 0 | 0 |
| [`os.dir.*`](#8-directory-apis-osdir) | Directory operations | 4 | 0 | 0 |
| [`os.gpio.*`](#9-gpio-apis-osgpio) | GPIO pin control | 5 | 0 | 0 |
| [`os.i2c.*`](#10-i2c-apis-osi2c) | I2C communication | 3 | 0 | 2 |
| [`os.buzzer.*`](#11-buzzer-apis-osbuzzer) | Buzzer/audio output | 3 | 0 | 0 |
| [`os.timer.*`](#12-timer-apis-ostimer) | Timers and intervals | 0 | 0 | 4 |
| [`os.rfid.*`](#13-rfid-apis-osrfid) | RFID card reader | 2 | 0 | 2 |
| [`os.power.*`](#14-power-apis-ospower) | Power management | 3 | 0 | 1 |
| [`os.app.*`](#15-app-apis-osapp) | Application lifecycle | 2 | 0 | 4 |
| [`os.storage.*`](#16-storage-apis-osstorage) | Storage device management | 2 | 0 | 2 |
| [`os.sensor.*`](#17-sensor-apis-ossensor) | Hardware sensor interface | 0 | 2 | 2 |
| [`os.events.*`](#18-events-apis-osevents) | Event system | 0 | 0 | 3 |
| [`os.wifi.*`](#19-wifi-apis-oswifi) | WiFi connectivity | 0 | 5 | 0 |
| [`os.http.*`](#20-http-apis-oshttp) | HTTP client | 0 | 0 | 3 |
| [`os.ipc.*`](#21-ipc-apis-osipc) | Inter-process communication | 2 | 0 | 1 |
| [`os.spi.*`](#22-spi-apis-osspi) | SPI bus for external modules | 4 | 0 | 0 |
| [`os.device.*`](#23-device-apis-osdevice) | Device driver model (discovery, handles) | 5 | 0 | 0 |
| **Total** | **23 namespaces** | **139 functions** | **64** | **4** | **31** |

**Legend:**
- ✅ **Implemented** - Function is working and tested
- 🔜 **Planned** - Needs implementation
- 🔒 **Blocked** - Requires function/callback support in VM

---

Defined in [include/vm/platform.h](../include/vm/platform.h)

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
| `os.console.print()` | `message: string` | `null` | ✅ Implemented |
| `os.console.log()` | `message: string` | `null` | ✅ Implemented |
| `os.console.warn()` | `message: string` | `null` | ✅ Implemented |
| `os.console.error()` | `message: string` | `null` | ✅ Implemented |
| `os.console.clear()` | none | `null` | ✅ Implemented |

### `os.console.print(message: string) -> null`
Print message to console
- **Parameters**: `message` (string) - Message to print
- **Returns**: null
- **Status**: ✅ Implemented

### `os.console.log(message: string) -> null`
Log informational message to console
- **Parameters**: `message` (string) - Message to log
- **Returns**: null
- **Status**: ✅ Implemented

### `os.console.warn(message: string) -> null`
Log warning message
- **Parameters**: `message` (string) - Warning message
- **Returns**: null
- **Status**: ✅ Implemented

### `os.console.error(message: string) -> null`
Log error message
- **Parameters**: `message` (string) - Error message
- **Returns**: null
- **Status**: ✅ Implemented

### `os.console.clear() -> null`
Clear console buffer
- **Parameters**: none
- **Returns**: null
- **Status**: ✅ Implemented (no-op)

---

## 2. Display APIs (`os.display.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.display.clear()` | `color: int` | `null` | ✅ Implemented |
| `os.display.drawText()` | `x: int, y: int, text: string, color: int, size: int` | `null` | ✅ Implemented |
| `os.display.drawPixel()` | `x: int, y: int, color: int` | `null` | ✅ Implemented |
| `os.display.drawLine()` | `x1: int, y1: int, x2: int, y2: int, color: int` | `null` | ✅ Implemented |
| `os.display.drawRect()` | `x: int, y: int, w: int, h: int, color: int, filled: bool` | `null` | ✅ Implemented |
| `os.display.drawCircle()` | `x: int, y: int, r: int, color: int, filled: bool` | `null` | ✅ Implemented |
| `os.display.drawImage()` | `x: int, y: int, imageData: object` | `null` | ✅ Implemented |
| `os.display.setBrightness()` | `level: int` | `null` | ✅ Implemented |
| `os.display.getSize()` | none | `object` | ✅ Implemented |
| `os.display.setTitle()` | `text: string` | `null` | ✅ Implemented |

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
- **Status**: ✅ Implemented

### `os.display.drawLine(x1: int, y1: int, x2: int, y2: int, color: int) -> null`
Draw line between two points
- **Parameters**:
  - `x1, y1` (int) - Start coordinates
  - `x2, y2` (int) - End coordinates
  - `color` (int) - RGB565 color
- **Returns**: null
- **Status**: ✅ Implemented

### `os.display.drawRect(x: int, y: int, w: int, h: int, color: int, filled: bool) -> null`
Draw rectangle
- **Parameters**:
  - `x, y` (int) - Top-left corner
  - `w, h` (int) - Width and height
  - `color` (int) - RGB565 color
  - `filled` (bool) - Fill rectangle if true
- **Returns**: null
- **Status**: ✅ Implemented

### `os.display.drawCircle(x: int, y: int, r: int, color: int, filled: bool) -> null`
Draw circle
- **Parameters**:
  - `x, y` (int) - Center coordinates
  - `r` (int) - Radius
  - `color` (int) - RGB565 color
  - `filled` (bool) - Fill circle if true
- **Returns**: null
- **Status**: ✅ Implemented

### `os.display.drawImage(x: int, y: int, imageData: object) -> null`
Draw bitmap image
- **Parameters**:
  - `x, y` (int) - Top-left position
  - `imageData` (object) - Image data object
- **Returns**: null
- **Status**: ✅ Implemented

### `os.display.setBrightness(level: int) -> null`
Adjust backlight brightness
- **Parameters**: `level` (int) - Brightness level (0-255)
- **Returns**: null
- **Status**: ✅ Implemented

### `os.display.getSize() -> object`
Get display dimensions
- **Parameters**: none
- **Returns**: object with `{width: 240, height: 240}`
- **Status**: ✅ Implemented (via getWidth/getHeight)

### `os.display.setTitle(text: string) -> null`
Set app title bar text
- **Parameters**: `text` (string) - Title text
- **Returns**: null
- **Status**: ✅ Implemented

---

## 3. Encoder APIs (`os.encoder.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.encoder.getButton()` | none | `bool` | ✅ Implemented |
| `os.encoder.getDelta()` | none | `int` | ✅ Implemented |
| `os.encoder.getPosition()` | none | `int` | ✅ Implemented |
| `os.encoder.reset()` | none | `null` | ✅ Implemented |
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
- **Status**: ✅ Implemented

### `os.encoder.reset() -> null`
Reset encoder position to zero
- **Parameters**: none
- **Returns**: null
- **Status**: ✅ Implemented

### `os.encoder.onTurn(callback: function) -> null`
Register encoder rotation callback
- **Parameters**: `callback` (function) - Called with delta: +/-
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

### `os.encoder.onButton(callback: function) -> null`
Register button press callback
- **Parameters**: `callback` (function) - Called on button press
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

---

## 4. Touch APIs (`os.touch.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.touch.getX()` | none | `int` | ✅ Implemented |
| `os.touch.getY()` | none | `int` | ✅ Implemented |
| `os.touch.isPressed()` | none | `bool` | ✅ Implemented (stub) |
| `os.touch.getPosition()` | none | `object` | 🔜 Planned |
| `os.touch.onPress()` | `callback: function` | `null` | 🔒 Blocked |
| `os.touch.onRelease()` | `callback: function` | `null` | 🔒 Blocked |
| `os.touch.onDrag()` | `callback: function` | `null` | 🔒 Blocked |

### `os.touch.getX() -> int`
Get touch X coordinate
- **Parameters**: none
- **Returns**: int - X coordinate
- **Status**: ✅ Implemented

### `os.touch.getY() -> int`
Get touch Y coordinate
- **Parameters**: none
- **Returns**: int - Y coordinate
- **Status**: ✅ Implemented

### `os.touch.isPressed() -> bool`
Check if touch is active
- **Parameters**: none
- **Returns**: bool - true if touched
- **Status**: ✅ Implemented (stub - returns false)

### `os.touch.getPosition() -> object`
Get current touch coordinates
- **Parameters**: none
- **Returns**: object with `{x: int, y: int, pressed: bool}`
- **Status**: 🔜 Planned

### `os.touch.onPress(callback: function) -> null`
Register touch press callback
- **Parameters**: `callback` (function) - Called with {x, y}
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

### `os.touch.onRelease(callback: function) -> null`
Register touch release callback
- **Parameters**: `callback` (function)
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

### `os.touch.onDrag(callback: function) -> null`
Register touch drag callback
- **Parameters**: `callback` (function) - Called with {x, y, dx, dy}
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

---

## 5. System APIs (`os.system.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.system.getTime()` | none | `int` | ✅ Implemented |
| `os.system.sleep()` | `ms: int` | `null` | ✅ Implemented |
| `os.system.yield()` | none | `null` | ✅ Implemented |
| `os.system.getRTC()` | none | `object` | ✅ Implemented (stub) |
| `os.system.setRTC()` | `datetime: object` | `null` | ✅ Implemented (stub) |

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
- **Status**: ✅ Implemented

### `os.system.getRTC() -> object`
Get real-time clock value
- **Parameters**: none
- **Returns**: object with `{year, month, day, hour, minute, second}`
- **Status**: ✅ Implemented (stub - returns millis/1000)

### `os.system.setRTC(datetime: object) -> null`
Set RTC
- **Parameters**: `datetime` (object) - Date/time object
- **Returns**: null
- **Status**: ✅ Implemented (stub - no-op)

---

## 6. Memory APIs (`os.memory.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.memory.getAvailable()` | none | `int` | ✅ Implemented |
| `os.memory.getUsage()` | none | `int` | ✅ Implemented |
| `os.memory.allocate()` | `size: int` | `int` | 🔜 Planned |
| `os.memory.free()` | `handle: int` | `null` | 🔜 Planned |

### `os.memory.getAvailable() -> int`
Query available heap
- **Parameters**: none
- **Returns**: int - Available bytes
- **Status**: ✅ Implemented

### `os.memory.getUsage() -> int`
Get current app memory usage
- **Parameters**: none
- **Returns**: int - Used bytes
- **Status**: ✅ Implemented

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
| `os.file.open()` | `path: string, mode: string` | `int` | ✅ Implemented |
| `os.file.read()` | `handle: int, size: int` | `string` | ✅ Implemented |
| `os.file.write()` | `handle: int, data: string` | `int` | ✅ Implemented |
| `os.file.close()` | `handle: int` | `null` | ✅ Implemented |
| `os.file.exists()` | `path: string` | `bool` | ✅ Implemented |
| `os.file.delete()` | `path: string` | `bool` | ✅ Implemented |
| `os.file.size()` | `path: string` | `int` | ✅ Implemented |

### `os.file.open(path: string, mode: string) -> int`
Open file
- **Parameters**:
  - `path` (string) - File path
  - `mode` (string) - 'r', 'w', or 'a'
- **Returns**: int - File handle (or -1 on failure)
- **Status**: ✅ Implemented (via RAMFS)

### `os.file.read(handle: int, size: int) -> string`
Read from file
- **Parameters**:
  - `handle` (int) - File handle
  - `size` (int) - Bytes to read
- **Returns**: string - Data read
- **Status**: ✅ Implemented (via RAMFS)

### `os.file.write(handle: int, data: string) -> int`
Write to file
- **Parameters**:
  - `handle` (int) - File handle
  - `data` (string) - Data to write
- **Returns**: int - Bytes written
- **Status**: ✅ Implemented (via RAMFS)

### `os.file.close(handle: int) -> null`
Close file
- **Parameters**: `handle` (int) - File handle
- **Returns**: null
- **Status**: ✅ Implemented (via RAMFS)

### `os.file.exists(path: string) -> bool`
Check if file exists
- **Parameters**: `path` (string) - File path
- **Returns**: bool
- **Status**: ✅ Implemented (via RAMFS)

### `os.file.delete(path: string) -> bool`
Delete file
- **Parameters**: `path` (string) - File path
- **Returns**: bool - Success status
- **Status**: ✅ Implemented (via RAMFS)

### `os.file.size(path: string) -> int`
Get file size
- **Parameters**: `path` (string) - File path
- **Returns**: int - File size in bytes
- **Status**: ✅ Implemented (via RAMFS)

---

## 8. Directory APIs (`os.dir.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.dir.list()` | `path: string` | `array` | ✅ Implemented |
| `os.dir.create()` | `path: string` | `bool` | ✅ Implemented |
| `os.dir.delete()` | `path: string` | `bool` | ✅ Implemented |
| `os.dir.exists()` | `path: string` | `bool` | ✅ Implemented |

### `os.dir.list(path: string) -> array`
List directory contents
- **Parameters**: `path` (string) - Directory path
- **Returns**: array - Array of filenames
- **Status**: ✅ Implemented (via RAMFS)

### `os.dir.create(path: string) -> bool`
Create directory
- **Parameters**: `path` (string) - Directory path
- **Returns**: bool - Success status
- **Status**: ✅ Implemented (virtual - RAMFS is flat)

### `os.dir.delete(path: string) -> bool`
Delete directory
- **Parameters**: `path` (string) - Directory path
- **Returns**: bool - Success status
- **Status**: ✅ Implemented (deletes files with path prefix)

### `os.dir.exists(path: string) -> bool`
Check if directory exists
- **Parameters**: `path` (string) - Directory path
- **Returns**: bool
- **Status**: ✅ Implemented (checks for files with path prefix)

---

## 9. GPIO APIs (`os.gpio.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.gpio.pinMode()` | `pin: int, mode: string` | `null` | ✅ Implemented |
| `os.gpio.digitalWrite()` | `pin: int, value: int` | `null` | ✅ Implemented |
| `os.gpio.digitalRead()` | `pin: int` | `int` | ✅ Implemented |
| `os.gpio.analogWrite()` | `pin: int, value: int` | `null` | ✅ Implemented |
| `os.gpio.analogRead()` | `pin: int` | `int` | ✅ Implemented |

### `os.gpio.pinMode(pin: int, mode: string) -> null`
Set pin mode
- **Parameters**:
  - `pin` (int) - GPIO pin number
  - `mode` (string) - "INPUT", "OUTPUT", "INPUT_PULLUP"
- **Returns**: null
- **Status**: ✅ Implemented

### `os.gpio.digitalWrite(pin: int, value: int) -> null`
Write digital value
- **Parameters**:
  - `pin` (int) - GPIO pin number
  - `value` (int) - HIGH (1) or LOW (0)
- **Returns**: null
- **Status**: ✅ Implemented

### `os.gpio.digitalRead(pin: int) -> int`
Read digital value
- **Parameters**: `pin` (int) - GPIO pin number
- **Returns**: int - HIGH (1) or LOW (0)
- **Status**: ✅ Implemented

### `os.gpio.analogWrite(pin: int, value: int) -> null`
PWM output
- **Parameters**:
  - `pin` (int) - GPIO pin number
  - `value` (int) - PWM value (0-255)
- **Returns**: null
- **Status**: ✅ Implemented

### `os.gpio.analogRead(pin: int) -> int`
ADC read
- **Parameters**: `pin` (int) - GPIO pin number
- **Returns**: int - ADC value (0-4095)
- **Status**: ✅ Implemented

---

## 10. I2C APIs (`os.i2c.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.i2c.scan()` | none | `array` | ✅ Implemented |
| `os.i2c.write()` | `address: int, data: string` | `bool` | ✅ Implemented |
| `os.i2c.read()` | `address: int, length: int` | `string` | ✅ Implemented |
| `os.i2c.onDeviceAdded()` | `callback: function` | `null` | 🔒 Blocked |
| `os.i2c.onDeviceRemoved()` | `callback: function` | `null` | 🔒 Blocked |

### `os.i2c.scan() -> array`
Scan for I2C devices
- **Parameters**: none
- **Returns**: array - Array of I2C addresses found
- **Status**: ✅ Implemented

### `os.i2c.write(address: int, data: string) -> bool`
Write to I2C device
- **Parameters**:
  - `address` (int) - I2C device address
  - `data` (string) - Data to write
- **Returns**: bool - Success status
- **Status**: ✅ Implemented

### `os.i2c.read(address: int, length: int) -> string`
Read from I2C device
- **Parameters**:
  - `address` (int) - I2C device address
  - `length` (int) - Bytes to read
- **Returns**: string - Data read
- **Status**: ✅ Implemented

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
| `os.buzzer.beep()` | `frequency: int, duration: int` | `null` | ✅ Implemented |
| `os.buzzer.playMelody()` | `notes: array` | `null` | ✅ Implemented |
| `os.buzzer.stop()` | none | `null` | ✅ Implemented |

### `os.buzzer.beep(frequency: int, duration: int) -> null`
Play tone
- **Parameters**:
  - `frequency` (int) - Frequency in Hz
  - `duration` (int) - Duration in milliseconds
- **Returns**: null
- **Status**: ✅ Implemented

### `os.buzzer.playMelody(notes: array) -> null`
Play note sequence
- **Parameters**: `notes` (array) - Array of {freq, duration} objects
- **Returns**: null
- **Status**: ✅ Implemented

### `os.buzzer.stop() -> null`
Stop buzzing
- **Parameters**: none
- **Returns**: null
- **Status**: ✅ Implemented

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
| `os.rfid.read()` | none | `string` | ✅ Implemented |
| `os.rfid.isPresent()` | none | `bool` | ✅ Implemented |
| `os.rfid.onCardDetected()` | `callback: function` | `null` | 🔒 Blocked |
| `os.rfid.onCardRemoved()` | `callback: function` | `null` | 🔒 Blocked |

### `os.rfid.read() -> string`
Read card UID
- **Parameters**: none
- **Returns**: string - Card UID or empty string
- **Status**: ✅ Implemented (WS1850S via I2C)

### `os.rfid.isPresent() -> bool`
Check if card is present
- **Parameters**: none
- **Returns**: bool
- **Status**: ✅ Implemented (WS1850S via I2C)

### `os.rfid.onCardDetected(callback: function) -> null`
Card detected event
- **Parameters**: `callback` (function) - Called with UID
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

### `os.rfid.onCardRemoved(callback: function) -> null`
Card removed event
- **Parameters**: `callback` (function)
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

---

## 14. Power APIs (`os.power.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.power.sleep()` | none | `null` | ✅ Implemented |
| `os.power.getBatteryLevel()` | none | `int` | ✅ Implemented |
| `os.power.isCharging()` | none | `bool` | ✅ Implemented |
| `os.power.onLowBattery()` | `callback: function` | `null` | 🔒 Blocked |

### `os.power.sleep() -> null`
Enter sleep mode (wake on button/RTC)
- **Parameters**: none
- **Returns**: null
- **Status**: ✅ Implemented (uses M5Dial.Power.deepSleep)

### `os.power.getBatteryLevel() -> int`
Get battery percentage
- **Parameters**: none
- **Returns**: int - Battery level (0-100)
- **Status**: ✅ Implemented

### `os.power.isCharging() -> bool`
Check if charging
- **Parameters**: none
- **Returns**: bool
- **Status**: ✅ Implemented

### `os.power.onLowBattery(callback: function) -> null`
Low battery event
- **Parameters**: `callback` (function) - Called when battery is low
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

---

## 15. App APIs (`os.app.*`)

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.app.exit()` | none | `null` | ✅ Implemented |
| `os.app.getInfo()` | none | `object` | ✅ Implemented |
| `os.app.onLoad()` | `callback: function` | `null` | 🔒 Blocked |
| `os.app.onSuspend()` | `callback: function` | `null` | 🔒 Blocked |
| `os.app.onResume()` | `callback: function` | `null` | 🔒 Blocked |
| `os.app.onUnload()` | `callback: function` | `null` | 🔒 Blocked |

### `os.app.exit() -> null`
Terminate current app
- **Parameters**: none
- **Returns**: null (does not return)
- **Status**: ✅ Implemented

### `os.app.getInfo() -> object`
Get app metadata
- **Parameters**: none
- **Returns**: object with `{name, version, memoryUsage}`
- **Status**: ✅ Implemented

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
| `os.storage.getMounted()` | none | `array` | ✅ Implemented |
| `os.storage.getInfo()` | `device: string` | `object` | ✅ Implemented |
| `os.storage.onMount()` | `callback: function` | `null` | 🔒 Blocked |
| `os.storage.onUnmount()` | `callback: function` | `null` | 🔒 Blocked |

### `os.storage.getMounted() -> array`
List mounted storage devices
- **Parameters**: none
- **Returns**: array - Array of mounted device names
- **Status**: ✅ Implemented

### `os.storage.getInfo(device: string) -> object`
Get device info (size, free space, type)
- **Parameters**: `device` (string) - Device name
- **Returns**: object with `{size, free, type}`
- **Status**: ✅ Implemented

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

> ⚠️ **Status correction (2026-02):** This section previously claimed `attach`/`read` were
> "✅ Implemented". That was inaccurate — **every platform returns a stub**
> (`-1` / `"{}"` with a `NOT IMPLEMENTED` warning). The sensor API is being redesigned
> as part of the **device driver model** — see [`DRIVER_MODEL.md`](./DRIVER_MODEL.md).

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.sensor.attach()` | `driver: string` | `int` | ❌ **Stub** (all platforms return -1) |
| `os.sensor.read()` | `handle: int` | `object` | ❌ **Stub** (all platforms return "{}") |
| `os.sensor.detach()` | `handle: int` | `null` | ❌ **Stub** |
| `os.sensor.onData()` | `handle: int, callback: function` | `null` | 🔒 Blocked (requires callbacks) |

### `os.sensor.attach(driver: string) -> int`
Attach a device by **registered driver name** (e.g. `"bmp280"`, `"mpu6886"`). The old
`PORT.A/PORT.B` parameter was not backed by any port enumeration and is dropped in v2.
- **Parameters**:
  - `driver` (string) - Driver/device name from the device registry
- **Returns**: int - Device handle (or -1 on failure) — **currently always -1**
- **Status**: ❌ Stub — see [DRIVER_MODEL.md](./DRIVER_MODEL.md)

### `os.sensor.read(handle: int) -> object`
Read driver-formatted sensor data
- **Parameters**: `handle` (int) - Device handle from `attach`
- **Returns**: object - Driver-defined data object — **currently always "{}"**
- **Status**: ❌ Stub

### `os.sensor.detach(handle: int) -> null`
Detach sensor and release the device handle
- **Parameters**: `handle` (int) - Sensor handle
- **Returns**: null
- **Status**: ❌ Stub

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
| `os.ipc.send()` | `appId: string, message: any` | `bool` | ✅ Implemented |
| `os.ipc.broadcast()` | `message: any` | `null` | ✅ Implemented |
| `os.ipc.onMessage()` | `callback: function` | `null` | 🔒 Blocked |

### `os.ipc.send(appId: string, message: any) -> bool`
Send message to another app
- **Parameters**:
  - `appId` (string) - Target app ID
  - `message` (any) - Message data
- **Returns**: bool - Send success
- **Status**: ✅ Implemented

### `os.ipc.broadcast(message: any) -> null`
Broadcast to all apps
- **Parameters**: `message` (any) - Message data
- **Returns**: null
- **Status**: ✅ Implemented

### `os.ipc.onMessage(callback: function) -> null`
Receive messages
- **Parameters**: `callback` (function) - Message handler
- **Returns**: null
- **Status**: 🔒 Blocked (requires function support)

---

## 22. SPI APIs (`os.spi.*`)

> New namespace (native IDs `0x15xx`) — part of the [device driver model](./DRIVER_MODEL.md).
> Serial Peripheral Interface for external modules.

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.spi.open()` | `config: string` | `int` | ✅ ESP32 (real) · ✅ SDL (simulated) · ❌ .NET stub |
| `os.spi.transfer()` | `handle: int, tx: string, rxLen: int` | `string` | ✅ ESP32 · ✅ SDL (echo sim) · ❌ .NET stub |
| `os.spi.write()` | `handle: int, tx: string` | `int` | ✅ ESP32 · ✅ SDL · ❌ .NET stub |
| `os.spi.close()` | `handle: int` | `bool` | ✅ ESP32 · ✅ SDL · ❌ .NET stub |

### `os.spi.open(config: string) -> int`
Open the SPI bus. Config JSON fields (all optional, defaults shown):
`{"clockHz":1000000,"mode":0,"csPin":5,"sclk":18,"miso":19,"mosi":23}`
- **Returns**: int - Handle (≥ 0), or -1 on failure. Max 4 concurrent handles on ESP32.
- **Status**: ✅ Implemented (ESP32: Arduino `SPI` host + per-handle CS; SDL: simulated)

### `os.spi.transfer(handle: int, tx: string, rxLen: int) -> string`
Write `tx` bytes, then clock in `rxLen` bytes. Binary payloads follow the `os.i2c`
convention: raw bytes inside a dialScript string.
- **Returns**: string - Raw received bytes (empty on error)
- **Status**: ✅ Implemented (SDL sim returns a deterministic echo pattern)

### `os.spi.write(handle: int, tx: string) -> int`
Write-only transaction. **Returns**: bytes written, or -1. **Status**: ✅ Implemented

### `os.spi.close(handle: int) -> bool`
Release the handle. **Status**: ✅ Implemented

---

## 23. Device APIs (`os.device.*`)

> New namespace (native IDs `0x16xx`) — the front door of the
> [device driver model](./DRIVER_MODEL.md). Discovery + ownership-tracked handles
> for modules on the I2C/SPI buses.

| Function | Parameters | Returns | Status |
|----------|------------|---------|--------|
| `os.device.list()` | none | `string` (JSON array) | ✅ ESP32 · ✅ SDL · ❌ .NET stub |
| `os.device.open()` | `nameOrAddress: string` | `int` | ✅ ESP32 · ✅ SDL · ❌ .NET stub |
| `os.device.close()` | `handle: int` | `bool` | ✅ ESP32 · ✅ SDL · ❌ .NET stub |
| `os.device.getInfo()` | `handle: int` | `string` (JSON object) | ✅ ESP32 · ✅ SDL · ❌ .NET stub |
| `os.device.probe()` | none | `string` (JSON array) | ✅ ESP32 · ✅ SDL · ❌ .NET stub |

### `os.device.list() -> string`
Run discovery and return present devices:
`[{"name":"bmp280","bus":"i2c","address":"0x76","capabilities":{...}}, ...]`
- **Status**: ✅ Implemented (ESP32: real WHO_AM_I probes over Wire; SDL: simulated
  `bmp280` + `w25q32` present, `pn532` absent)

### `os.device.open(nameOrAddress: string) -> int`
Open by driver name (`"bmp280"`) or by address (`"i2c:0x76"`, `"spi:5"`).
One open handle per device; max 16 handles; ownership recorded for the calling task.
- **Returns**: int - Handle (≥ 0), or -1 (unknown/not present/already open/no slots)
- **Status**: ✅ Implemented

### `os.device.close(handle: int) -> bool`
Release a handle. Fails if the handle is not owned by the calling task.
- **Status**: ✅ Implemented

### `os.device.getInfo(handle: int) -> string`
Descriptor JSON for an open handle (`"{}"` if invalid). **Status**: ✅ Implemented

### `os.device.probe() -> string`
Re-run bus discovery (e.g. after physically plugging a module) — same format as
`list()`. Hotplug *events* remain blocked on the callback infrastructure.
- **Status**: ✅ Implemented

---

## Implementation Status Summary

### ✅ Implemented (64 functions)
- **Console**: print, log, warn, error, clear (5)
- **Display**: clear, drawText, drawPixel, drawLine, drawRect, drawCircle, drawImage, setBrightness, getSize, setTitle (10)
- **Encoder**: getButton, getDelta, getPosition, reset (4)
- **Touch**: getX, getY, isPressed (3)
- **System**: getTime, sleep, yield, getRTC, setRTC (5)
- **Memory**: getAvailable, getUsage (2)
- **File**: open, read, write, close, exists, delete, size (7)
- **Directory**: list, create, delete, exists (4)
- **GPIO**: pinMode, digitalWrite, digitalRead, analogWrite, analogRead (5)
- **I2C**: scan, write, read (3)
- **Buzzer**: beep, playMelody, stop (3)
- **RFID**: read, isPresent (2)
- **Power**: sleep, getBatteryLevel, isCharging (3)
- **App**: exit, getInfo (2)
- **Storage**: getMounted, getInfo (2)
- **Sensor**: attach, read (2) *(⚠️ stubs on all platforms — see §17 correction)*
- **IPC**: send, broadcast (2)
- **SPI**: open, transfer, write, close (4) *(ESP32 real, SDL simulated, .NET stub)*
- **Device**: list, open, close, getInfo, probe (5) *(ESP32/SDL registry-backed, .NET stub)*

### 🔜 Planned (4 functions)
- `os.touch.getPosition()` - Composite of getX/getY
- `os.memory.allocate()` - Direct memory allocation
- `os.memory.free()` - Memory deallocation
- `os.wifi.*` (5) - connect, disconnect, getStatus, getIP, scan

### 🔒 Blocked (31 callback-based APIs)
Require function/callback support in VM:
- **App Lifecycle**: onLoad, onSuspend, onResume, onUnload (4)
- **Input Events**: encoder.onTurn, encoder.onButton (2)
- **Touch Events**: onPress, onRelease, onDrag (3)
- **Timers**: setTimeout, setInterval, clearTimeout, clearInterval (4)
- **I2C Events**: onDeviceAdded, onDeviceRemoved (2)
- **RFID Events**: onCardDetected, onCardRemoved (2)
- **Power Events**: onLowBattery (1)
- **Storage Events**: onMount, onUnmount (2)
- **Sensor Events**: onData, detach (2)
- **Event System**: emit, on, off (3)
- **HTTP**: get, post, download (3)
- **IPC**: onMessage (1)
- **WiFi Events**: onConnect, onDisconnect (2)

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
