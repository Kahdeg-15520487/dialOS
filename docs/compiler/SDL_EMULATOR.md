# dialOS SDL Emulator

A comprehensive M5 Dial hardware emulator using SDL2 that allows you to test dialScript applications without physical hardware.

## Features

### Hardware Simulation
- **240x240 Circular Display**: Simulated as a circular region in a scalable window
- **Rotary Encoder**: Mouse wheel for rotation, clicking outside display for button
- **Touch Interface**: Mouse clicks within the circular display area
- **RFID Reader**: Keyboard simulation (R key to toggle card presence)
- **Buzzer**: Audio feedback simulation with console logging
- **RTC & Timing**: Real-time clock simulation with system timing
- **GPIO Ports**: Expansion port simulation for PORT.A and PORT.B
- **I2C Bus**: Storage expansion simulation
- **Power Management**: Battery level and charging simulation

### Emulator Features
- **Debug Overlay**: Real-time system information (toggle with D key)
- **Console Logging**: All platform operations logged to console
- **60 FPS Rendering**: Smooth visual feedback
- **Frame Rate Control**: Maintains consistent timing
- **Error Handling**: Comprehensive error reporting and recovery

## Installation

### Windows (using vcpkg)

1. Install vcpkg if not already installed:
   ```cmd
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   ```

2. Add vcpkg to your PATH or note the installation directory.

3. Install SDL2 dependencies:
   ```cmd
   .\build_emulator.ps1 -Install
   ```
   
   Or manually:
   ```cmd
   vcpkg install sdl2:x64-windows sdl2-ttf:x64-windows sdl2-mixer:x64-windows
   ```

### Ubuntu/Debian

```bash
sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev
```

### macOS

```bash
brew install sdl2 sdl2_ttf sdl2_mixer
```

## Building

### Quick Build (Windows)

Use the provided PowerShell script:

```powershell
# Install dependencies and build
.\build_emulator.ps1 -Install

# Build emulator only
.\build_emulator.ps1 -Build

# Compile and run a script
.\build_emulator.ps1 scripts\counter_applet.ds
```

### Manual Build

1. Configure CMake with SDL2 support:
   ```bash
   cd compiler
   mkdir build && cd build
   
   # Windows with vcpkg
   cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake
   
   # Linux/macOS (SDL2 installed via package manager)
   cmake ..
   ```

2. Build the project:
   ```bash
   cmake --build . --config Debug
   ```

3. The emulator will be built as `test_sdl_emulator` (or `test_sdl_emulator.exe` on Windows).

## Usage

### Running Scripts

```bash
# Run a compiled bytecode file
./test_sdl_emulator script.dsb

# Or use the PowerShell helper
.\build_emulator.ps1 scripts\my_app.ds
```

### Controls

| Control | Function |
|---------|----------|
| **Mouse Wheel** | Rotary encoder rotation |
| **Left Click (in display)** | Touch screen input |
| **Left Click (outside display)** | Encoder button press |
| **R Key** | Toggle RFID card simulation |
| **B Key** | Test buzzer beep |
| **D Key** | Toggle debug overlay |
| **ESC Key** | Exit emulator |

### Debug Overlay

Press **D** to toggle the debug overlay, which shows:
- System uptime
- Encoder position and button state
- Touch coordinates and state
- RFID card presence and UID
- Battery level and charging status
- Buzzer activity and frequency

## Hardware Mapping

### M5 Dial → SDL Emulator

| M5 Dial Hardware | SDL Emulation |
|------------------|---------------|
| 240x240 TFT Display | Circular rendering area (scaled 3x) |
| Rotary Encoder | Mouse wheel |
| Encoder Button | Mouse click outside display |
| Touch Screen | Mouse click in display area |
| RFID Reader | R key toggle |
| Buzzer | Console log + audio simulation |
| RTC | System clock |
| PORT.A GPIO | Simulated GPIO pins |
| PORT.B GPIO | Simulated GPIO pins |
| I2C Bus | Virtual I2C device simulation |
| Battery | Simulated power management |

## API Support

The SDL platform implements the full dialOS PlatformInterface:

### Display Operations
- `display_clear(color)` - Clear screen
- `display_drawText(x, y, text, color, size)` - Render text
- `display_drawPixel(x, y, color)` - Draw pixel
- `display_drawLine(x1, y1, x2, y2, color)` - Draw line
- `display_drawRect(x, y, w, h, color, filled)` - Draw rectangle
- `display_drawCircle(x, y, radius, color, filled)` - Draw circle
- `display_setBrightness(level)` - Adjust brightness

### Input Operations
- `encoder_getButton()` - Get encoder button state
- `encoder_getDelta()` - Get encoder rotation delta
- `touch_isPressed()` - Check touch state
- `touch_getPosition(x, y)` - Get touch coordinates
- `rfid_isCardPresent()` - Check RFID card
- `rfid_getCardUID()` - Get card UID

### System Operations
- `system_getTime()` - Get uptime in milliseconds
- `system_sleep(ms)` - Sleep/yield
- `system_getRTC()` - Get real-time clock
- `buzzer_beep(freq, duration)` - Play tone
- `console_log(msg)` - Debug output

## Example Script

Here's a simple dialScript that works with the emulator:

```javascript
// counter_applet.ds
let count = 0;

function onLoad() {
    display.clear(0x000000FF);
    display.drawText(10, 10, "Counter App", 0xFFFFFFFF, 16);
    updateDisplay();
}

function onEncoderTurn(delta) {
    count += delta;
    updateDisplay();
}

function onTouch(x, y) {
    count = 0;
    updateDisplay();
}

function updateDisplay() {
    display.drawRect(10, 50, 220, 30, 0x000000FF, true);
    display.drawText(10, 60, "Count: " + count, 0x00FF00FF, 14);
}
```

## Troubleshooting

### Build Issues

**"SDL2 libraries not found"**
- Install SDL2 dependencies using your package manager
- On Windows, use vcpkg: `.\build_emulator.ps1 -Install`

**"vcpkg not found"**
- Install vcpkg and add it to your PATH
- Or specify toolchain manually in CMake

**"Font rendering not working"**
- The emulator tries to load arial.ttf
- Install system fonts or provide a font file
- Text rendering will fall back to basic rendering if font fails

### Runtime Issues

**"Could not initialize SDL"**
- Check SDL2 installation
- Verify graphics drivers are up to date
- Try running in a different display mode

**"Audio initialization failed"**
- Audio is optional, emulator continues without it
- Check audio drivers and permissions

## Development

### Adding New Platform APIs

1. Add method to `PlatformInterface` in `platform.h`
2. Implement in `SDLPlatform` class in `sdl_platform.h/cpp`
3. Update VM core to call new platform methods
4. Test with emulator

### Extending Hardware Simulation

The SDL platform is designed to be easily extensible:
- Add new input methods in `pollEvents()`
- Extend GPIO simulation in gpio methods
- Add I2C device simulation in i2c methods
- Enhance debug overlay in `debug_drawOverlay()`

## License

Part of the dialOS project. See main project README for licensing information.