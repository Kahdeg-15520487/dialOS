# dialOS - M5 Dial Operating System

## Project Overview
dialOS is a multi-tasking operating system for the M5 Dial development board (ESP32-based) featuring a rotary encoder, button, and round LCD touchscreen. The OS supports app execution through a custom scripting language with userland API access.

## Architecture Vision

### Core Components (To Be Implemented)
- **Kernel**: Task scheduler with freeze/sleep capabilities for non-active apps
- **App Framework**: Scripting language interpreter for user applications
- **Hardware Abstraction Layer (HAL)**: Interface for M5 Dial peripherals
- **Graphics System**: UI rendering and touch input handling
- **File System**: Hierarchical storage with I2C expansion capabilities

### Hardware Platform
- **Target**: M5Stack Stamps3 (ESP32-S3) with M5 Dial peripherals
- **Display**: 1.28" circular TFT touch screen (240x240px, GC9A01 driver, FT3267 touch)
- **Input**: Rotary encoder (16 positions, 64 pulses/revolution) + button + RFID (13.56MHz)
- **Expansion Ports**: 
  - **PORT.A**: GPIO13, GPIO15 (HY2.0-4P Grove connector, 5V power)
  - **PORT.B**: GPIO2, GPIO1 (HY2.0-4P Grove connector, 5V power)
- **Power**: Wide voltage input (6-36V DC), lithium battery support (1.25mm-2P)
- **Additional**: RTC (BM8563), buzzer (80dB), RFID reader (WS1850S)
- **Library**: M5Dial@^1.0.3 (includes M5Unified and M5GFX)

## Development Environment

### Build System
- **Platform**: PlatformIO with Arduino framework
- **Board**: `m5stack-stamps3`
- **Key Dependencies**: `m5stack/M5Dial@^1.0.3`
- **Build Commands**:
  ```bash
  # Build project
  pio run -e m5stack-stamps3
  
  # Upload to device
  pio run -e m5stack-stamps3 -t upload
  
  # Monitor serial output
  pio device monitor
  ```

### Project Structure
```
src/                 # Main application code
├── main.cpp         # Entry point and main loop
├── kernel/          # OS kernel and task management (planned)
├── hal/             # Hardware abstraction layer (planned)
├── graphics/        # UI and rendering system (planned)
└── scripting/       # Script interpreter and userland API (planned)

include/             # Project header files
lib/                 # Project-specific libraries
test/               # Unit tests
```

## Development Patterns

### Initialization Sequence
1. Hardware initialization via M5Dial.begin()
2. Kernel task scheduler setup
3. Graphics system initialization
4. App loading and management system startup

### Multi-tasking Architecture
- **Active App**: Currently running application (full resources)
- **Frozen Apps**: Suspended applications (state preserved, no CPU time)
- **Task Scheduler**: Handles app switching and resource allocation

### Hardware Integration
- Use M5Unified library for consistent hardware access patterns
- M5GFX for all display rendering and touch input
- Abstract hardware interfaces through HAL for portability
- Expansion ports enable modular peripheral ecosystem via Grove connectors
- GPIO capabilities: Digital I/O, PWM, ADC, I2C, SPI, UART protocols

## Kernel API Design

### Core System APIs
**App Lifecycle**
- `app.onLoad(callback)` - Called when app starts
- `app.onSuspend(callback)` - Called when app is frozen
- `app.onResume(callback)` - Called when app becomes active again
- `app.onUnload(callback)` - Called when app exits
- `app.exit()` - Terminate current app
- `app.getInfo()` - Get app metadata (name, version, memory usage)

**Task Management**
- `system.sleep(ms)` - Yield CPU for specified time
- `system.yield()` - Cooperative yield to scheduler
- `system.getTime()` - Get system uptime in milliseconds
- `system.getRTC()` - Get real-time clock value
- `system.setRTC(datetime)` - Set RTC

**Memory Management**
- `memory.allocate(size)` - Request memory allocation
- `memory.free(handle)` - Release memory
- `memory.getAvailable()` - Query available heap
- `memory.getUsage()` - Get current app memory usage

### Display & Graphics APIs
**Screen Control**
- `display.clear(color)` - Clear screen with color
- `display.setTitle(text)` - Set app title bar
- `display.setBrightness(level)` - Adjust backlight (0-255)
- `display.getSize()` - Returns {width: 240, height: 240}

**Drawing Primitives**
- `display.drawPixel(x, y, color)` - Draw single pixel
- `display.drawLine(x1, y1, x2, y2, color)` - Draw line
- `display.drawRect(x, y, w, h, color, filled)` - Draw rectangle
- `display.drawCircle(x, y, r, color, filled)` - Draw circle
- `display.drawText(x, y, text, color, size)` - Render text
- `display.drawImage(x, y, imageData)` - Draw bitmap image

**Touch Input**
- `touch.onPress(callback)` - Touch press event
- `touch.onRelease(callback)` - Touch release event
- `touch.onDrag(callback)` - Touch drag event
- `touch.getPosition()` - Get current touch coordinates

### Input Device APIs
**Rotary Encoder**
- `encoder.onTurn(callback)` - Encoder rotation event (delta: +/-)
- `encoder.onButton(callback)` - Encoder button press
- `encoder.getPosition()` - Get absolute encoder position
- `encoder.reset()` - Reset encoder position to 0

**RFID Reader**
- `rfid.onCardDetected(callback)` - Card detected event
- `rfid.onCardRemoved(callback)` - Card removed event
- `rfid.read()` - Read card UID
- `rfid.isPresent()` - Check if card is present

### Storage & File System APIs
**File Operations**
- `file.open(path, mode)` - Open file (modes: 'r', 'w', 'a')
- `file.read(handle, size)` - Read from file
- `file.write(handle, data)` - Write to file
- `file.close(handle)` - Close file
- `file.exists(path)` - Check if file exists
- `file.delete(path)` - Delete file
- `file.size(path)` - Get file size

**Directory Operations**
- `dir.list(path)` - List directory contents
- `dir.create(path)` - Create directory
- `dir.delete(path)` - Delete directory
- `dir.exists(path)` - Check if directory exists

**Storage Management**
- `storage.getMounted()` - List mounted storage devices
- `storage.getInfo(device)` - Get device info (size, free space, type)
- `storage.onMount(callback)` - Storage device mounted event
- `storage.onUnmount(callback)` - Storage device removed event

### Hardware Expansion APIs
**GPIO Control**
- `gpio.pinMode(pin, mode)` - Set pin mode (INPUT, OUTPUT, INPUT_PULLUP)
- `gpio.digitalWrite(pin, value)` - Write digital value (HIGH/LOW)
- `gpio.digitalRead(pin)` - Read digital value
- `gpio.analogWrite(pin, value)` - PWM output (0-255)
- `gpio.analogRead(pin)` - ADC read (0-4095)

**I2C Communication**
- `i2c.scan()` - Scan for I2C devices
- `i2c.write(address, data)` - Write to I2C device
- `i2c.read(address, length)` - Read from I2C device
- `i2c.onDeviceAdded(callback)` - New device detected event
- `i2c.onDeviceRemoved(callback)` - Device removed event

**Hardware Sensors**
- `sensor.attach(port, type)` - Attach sensor to PORT.A/B
- `sensor.read(handle)` - Read sensor value
- `sensor.detach(handle)` - Detach sensor
- `sensor.onData(handle, callback)` - Sensor data event

### Network & Communication APIs
**WiFi (Future)**
- `wifi.connect(ssid, password)` - Connect to WiFi
- `wifi.disconnect()` - Disconnect WiFi
- `wifi.getStatus()` - Get connection status
- `wifi.getIP()` - Get IP address

**HTTP (Future)**
- `http.get(url, callback)` - HTTP GET request
- `http.post(url, data, callback)` - HTTP POST request

### Audio & Feedback APIs
**Buzzer Control**
- `buzzer.beep(frequency, duration)` - Play tone
- `buzzer.playMelody(notes)` - Play note sequence
- `buzzer.stop()` - Stop buzzing

### Timer & Events APIs
**Timers**
- `timer.setTimeout(callback, ms)` - One-shot timer
- `timer.setInterval(callback, ms)` - Recurring timer
- `timer.clearTimeout(id)` - Cancel timeout
- `timer.clearInterval(id)` - Cancel interval

**Events**
- `events.emit(eventName, data)` - Emit custom event
- `events.on(eventName, callback)` - Listen for event
- `events.off(eventName, callback)` - Remove event listener

### Power Management APIs
**Power Control**
- `power.sleep()` - Enter sleep mode (wake on button/RTC)
- `power.getBatteryLevel()` - Get battery percentage
- `power.isCharging()` - Check if charging
- `power.onLowBattery(callback)` - Low battery event

### Inter-App Communication (IPC)
**Message Passing**
- `ipc.send(appId, message)` - Send message to another app
- `ipc.onMessage(callback)` - Receive messages
- `ipc.broadcast(message)` - Broadcast to all apps

### Debugging & Logging APIs
**Console Output**
- `console.log(message)` - Log informational message
- `console.warn(message)` - Log warning
- `console.error(message)` - Log error
- `console.clear()` - Clear console buffer

## Key Files to Understand
- `platformio.ini`: Build configuration and dependencies
- `src/main.cpp`: Current minimal Arduino setup (to be expanded)
- Future core files: `kernel/scheduler.h`, `hal/m5dial_hal.h`, `scripting/interpreter.h`

## Current Status
🚧 **Early Development Phase**: Basic PlatformIO setup complete, M5Dial library integrated. Core OS components and architecture need implementation.

## File System Architecture

### Storage Hierarchy
- **Internal Flash**: Core OS, system apps, bootloader (8MB ESP32-S3)
- **I2C Storage Expansion**: Modular storage via Grove ports
  - EEPROM modules (24C256, 24C512, etc.) for applet data and distribution
  - FRAM modules for high-endurance logging
  - SD card modules via I2C interface for bulk storage
- **Virtual File System (VFS)**: Custom lightweight format optimized for I2C block access
- **Applet Format**: Custom bytecode with bounded memory execution model

### Scripting Language Design
- **Syntax**: JavaScript-like for familiarity
- **Execution**: Custom lightweight bytecode interpreter (WASM consideration for future)
- **Security Model**: Bounded memory allocation and restricted OS API calls
- **Applet Packaging**: Compiled bytecode files with embedded resources
- **Distribution**: Download via network or physical EEPROM modules ("USB-like" chips)

### I2C Expansion Ecosystem
- **Auto-discovery**: Periodic I2C address space scanning for new devices
- **Hot-plug Support**: Dynamic mounting/unmounting with runtime driver loading
- **Device Recognition**: 
  - Well-known devices: Built-in kernel drivers auto-loaded
  - Custom devices: Query device identifier, load matching drivers (kernel or dialOS script)
- **Address Management**: Dynamic enumeration and conflict resolution
- **Physical Distribution**: EEPROM chips as portable app/data carriers
- **Standard Protocol**: Custom I2C protocol layer for device capability advertisement

## Implementation Plan

### Phase 1: Hardware Foundation (Week 1-2)
**Step 1.1: M5Dial Hardware Initialization**
- Initialize M5Dial.begin() with all peripherals
- Test display output, touch input, rotary encoder
- Verify RFID reader functionality
- **Verification**: Basic "Hello dialOS" display with encoder interaction

**Step 1.2: Hardware Abstraction Layer (HAL)**
- Create HAL interface for display, touch, encoder, RFID
- Abstract M5Unified/M5GFX calls behind OS interface
- Implement GPIO abstraction for expansion ports
- **Verification**: HAL test suite passes, clean hardware interface

**Step 1.3: I2C Bus Management**
- Implement I2C scanner for address enumeration
- Create device detection and identification system
- Basic hot-plug detection (periodic scanning)
- **Verification**: Connect/disconnect I2C EEPROM, OS detects changes

### Phase 2: Basic OS Kernel (Week 3-4)
**Step 2.1: Task Scheduler Foundation**
- Implement cooperative task scheduler
- Basic task states: running, frozen, sleeping
- Task switching mechanism with state preservation
- **Verification**: Multiple tasks can be created, paused, resumed

**Step 2.2: Memory Management**
- Bounded memory allocator for applets
- Heap management with isolation between tasks
- Stack overflow protection
- **Verification**: Memory stress tests, isolation verified

**Step 2.3: System Services**
- Basic logging system
- Error handling and recovery
- System state management (sleep/wake)
- **Verification**: System can recover from applet crashes

### Phase 3: File System & Storage (Week 5-6)
**Step 3.1: Custom VFS Design**
- Design file system layout for I2C EEPROM
- Implement basic file operations (open, read, write, close)
- Directory structure and metadata management
- **Verification**: Create, read, write files on EEPROM module

**Step 3.2: I2C Storage Drivers**
- EEPROM driver (24Cxx series support)
- FRAM driver for logging
- Storage device abstraction layer
- **Verification**: Multiple storage types work interchangeably

**Step 3.3: Hot-Plug Storage System**
- Dynamic mount/unmount of I2C storage
- Storage device discovery and capability detection
- Graceful handling of device removal
- **Verification**: Add/remove storage devices during runtime

### Phase 4: Bytecode Engine (Week 7-8)
**Step 4.1: Bytecode VM Design**
- Define instruction set architecture
- Implement stack-based virtual machine
- Basic opcodes (math, logic, control flow)
- **Verification**: Simple bytecode programs execute correctly

**Step 4.2: OS API Bridge**
- Define applet API for hardware access
- Implement security boundaries and API restrictions
- HAL integration for controlled hardware access
- **Verification**: Applet can control display, read sensors safely

**Step 4.3: Applet Loader System**
- Bytecode file format with headers
- Applet loading, verification, and execution
- Memory bounds enforcement
- **Verification**: Load and run applet from storage device

### Phase 5: Graphics & UI Framework (Week 9-10)
**Step 5.1: Graphics Abstraction**
- Screen management and rendering pipeline
- UI primitive library (text, shapes, widgets)
- Touch input handling and event system
- **Verification**: Rich UI elements render and respond to touch

**Step 5.2: App Sandboxing**
- Graphics context isolation between applets
- Screen region management
- Input event routing to active applet
- **Verification**: Multiple applets can't interfere with each other

**Step 5.3: System UI**
- App launcher and task manager
- Settings and system configuration
- File manager for storage devices
- **Verification**: Complete OS user interface functional

### Phase 6: Scripting Language (Week 11-12)
**Step 6.1: Language Parser**
- JavaScript-like syntax parser
- Abstract syntax tree generation
- Bytecode compilation pipeline
- **Verification**: Parse and compile simple scripts

**Step 6.2: Runtime Integration**
- Script-to-bytecode compiler
- Runtime library for common operations
- Event system for UI and hardware events
- **Verification**: Write, compile, and run JavaScript-like applets

**Step 6.3: Development Tools**
- Simple on-device code editor
- Applet packaging and distribution tools
- Debug output and error reporting
- **Verification**: Create applet entirely on-device

### Phase 7: I2C Ecosystem (Week 13-14)
**Step 7.1: Device Discovery Protocol**
- Standard I2C capability advertisement protocol
- Device driver loading system (kernel and script)
- Automatic driver matching and loading
- **Verification**: Custom I2C device auto-detected and functional

**Step 7.2: Applet Distribution System**
- EEPROM-based applet distribution ("USB-like" functionality)
- Network download and installation
- Applet metadata and dependency management
- **Verification**: Install applet from physical EEPROM chip

### Phase 8: Polish & Testing (Week 15-16)
**Step 8.1: Performance Optimization**
- Memory usage optimization
- Bytecode execution performance tuning
- I2C communication efficiency improvements
- **Verification**: Smooth UI performance, responsive system

**Step 8.2: Comprehensive Testing**
- Stress testing with multiple applets
- Hot-plug reliability testing
- Power management and sleep/wake cycles
- **Verification**: System stable under all test conditions

**Step 8.3: Documentation & Examples**
- API documentation for applet developers
- Example applets demonstrating features
- Hardware integration guides
- **Verification**: Third party can create functional applets

## Verification Checkpoints
Each phase must pass verification before proceeding to the next. This ensures a solid foundation and catches issues early in development.