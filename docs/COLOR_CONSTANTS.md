# Color Constants Reference

DialScript provides built-in color constants via the `_color` namespace. These are compile-time constants that are replaced with their RGB565 hex values during compilation.

## Usage

```javascript
os.display.clear(_color.black);
os.display.drawCircle(120, 120, 50, _color.blue, true);
os.display.drawRect(10, 10, 100, 50, _color.red, false);
```

## Available Colors

### Basic Colors
- `_color.black` - 0x0000 - Pure black
- `_color.white` - 0xFFFF - Pure white
- `_color.red` - 0xF800 - Pure red
- `_color.green` - 0x07E0 - Pure green
- `_color.blue` - 0x001F - Pure blue
- `_color.yellow` - 0xFFE0 - Yellow (red + green)
- `_color.cyan` - 0x07FF - Cyan (green + blue)
- `_color.magenta` - 0xF81F - Magenta (red + blue)

### Grayscale
- `_color.darkgray` / `_color.darkgrey` - 0x39E7 - Dark gray
- `_color.gray` / `_color.grey` - 0x7BEF - Medium gray
- `_color.lightgray` / `_color.lightgrey` - 0xBDF7 - Light gray

### Extended Colors
- `_color.orange` - 0xFD20 - Orange
- `_color.purple` - 0x780F - Purple
- `_color.pink` - 0xFE19 - Pink
- `_color.brown` - 0x9A60 - Brown
- `_color.lime` - 0x07E0 - Lime green
- `_color.navy` - 0x000F - Navy blue
- `_color.teal` - 0x0410 - Teal
- `_color.maroon` - 0x7800 - Maroon
- `_color.olive` - 0x7BE0 - Olive

### Light Variations
- `_color.lightred` - 0xFBAE - Light red
- `_color.lightgreen` - 0x9772 - Light green
- `_color.lightblue` - 0xAEDC - Light blue
- `_color.lightyellow` - 0xFFFC - Light yellow
- `_color.lightcyan` - 0xE7FF - Light cyan
- `_color.lightmagenta` - 0xFBDF - Light magenta

### Dark Variations
- `_color.darkred` - 0x8800 - Dark red
- `_color.darkgreen` - 0x0400 - Dark green
- `_color.darkblue` - 0x0010 - Dark blue
- `_color.darkyellow` - 0x8C00 - Dark yellow
- `_color.darkcyan` - 0x0410 - Dark cyan
- `_color.darkmagenta` - 0x8010 - Dark magenta

## RGB565 Format

All colors use the RGB565 format, which is a 16-bit color encoding:
- **R** (Red): 5 bits (0-31)
- **G** (Green): 6 bits (0-63)
- **B** (Blue): 5 bits (0-31)

Format: `RRRRRGGGGGGBBBBB`

## Compile-Time Optimization

Color constants are resolved at compile time, meaning:
- Zero runtime overhead
- No memory allocation needed
- Direct bytecode generation with numeric values
- Type-safe color handling

## Example

```javascript
// These two are equivalent:
os.display.drawCircle(50, 50, 30, _color.red, true);
os.display.drawCircle(50, 50, 30, 0xF800, true);

// But the constant version is more readable and maintainable
```

## Custom Colors

For custom colors not in the predefined list, you can still use hex literals:

```javascript
var customPurple: 0x8010;
var customOrange: 0xFD20;

os.display.drawRect(10, 10, 50, 50, customPurple, true);
```

## Notes

- Color names are case-sensitive (use lowercase)
- Both American (`gray`) and British (`grey`) spellings are supported
- Unknown color names will produce a compile-time error
- Colors are optimized for the M5 Dial's circular TFT display
