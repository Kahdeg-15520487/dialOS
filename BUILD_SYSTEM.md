# dialOS Build System

## Quick Start

Compile any dialScript (.ds) file:

```powershell
# Simple compilation
.\build.ps1 lsp\timer.ds

# Show detailed information
.\build.ps1 lsp\timer.ds -ShowDetails

# Display the Abstract Syntax Tree
.\build.ps1 lsp\timer.ds -ShowAST

# Specify output file (future: bytecode)
.\build.ps1 lsp\timer.ds -Output build\timer.dsb
```

## Usage

```
.\build.ps1 [-Source] <file.ds> [options]
```

### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `Source` | String | Path to the .ds source file (required) |
| `Output` | String | Output file path (optional, future feature) |
| `ShowAST` | Switch | Display the Abstract Syntax Tree |
| `ShowDetails` | Switch | Show detailed compilation information |

### Examples

**Basic Compilation**
```powershell
.\build.ps1 lsp\timer.ds
```
Output:
```
╔═══════════════════════════════════════════════════════╗
║         dialOS - dialScript Compiler v1.0            ║
╚═══════════════════════════════════════════════════════╝

✓ Compilation successful

✓ Done
```

**Detailed Output**
```powershell
.\build.ps1 lsp\timer.ds -ShowDetails
```
Output:
```
Source: J:\workspace2\arduino\dialOS\lsp\timer.ds
Parsing: J:\workspace2\arduino\dialOS\lsp\timer.ds

✓ Compilation successful

Source file:  timer.ds
Size:         3011 bytes
Declarations: 10

✓ Done
```

**View AST**
```powershell
.\build.ps1 lsp\test_expressions.ds -ShowAST
```

**Combine Options**
```powershell
.\build.ps1 lsp\timer.ds -ShowDetails -ShowAST
```

## Build Process

### Current (v1.0)
1. ✅ **Lexical Analysis** - Tokenize source code
2. ✅ **Syntax Analysis** - Parse into AST
3. ✅ **Error Checking** - Report parse errors
4. ⏳ **Bytecode Generation** - Coming soon

### Future Phases
- **Type Checking** - Semantic analysis with type inference
- **Optimization** - AST transformations
- **Bytecode Emission** - Generate .dsb files for dialOS VM
- **Linking** - Resolve imports and dependencies

## Error Handling

### Parse Errors
```powershell
PS> .\build.ps1 broken.ds

❌ Compilation failed with 3 error(s):

  Line 5:12 - Expected ';' after statement
  Line 8:20 - Unexpected token 'while'
  Line 15:3 - Expected '}' to close block
```

### File Not Found
```powershell
PS> .\build.ps1 missing.ds

❌ Source file not found: missing.ds
```

## Directory Structure

```
dialOS/
├── build.ps1              # Main build script (this file)
├── lsp/                   # dialScript source files
│   ├── timer.ds           # Timer app example
│   ├── test.ds            # Language test file
│   └── test_expressions.ds # Expression tests
├── compiler/              # Compiler implementation
│   ├── lexer.cpp/h        # Tokenizer
│   ├── parser.cpp/h       # Parser
│   ├── ast.cpp/h          # AST definitions
│   └── build/             # Build artifacts
│       └── Debug/
│           └── parse_file.exe  # Parser executable
└── src/                   # dialOS kernel
```

## Building the Compiler

If the compiler isn't built yet, the script will automatically build it:

```powershell
PS> .\build.ps1 lsp\timer.ds

❌ Compiler not found!

Building compiler...
✓ Compiler built successfully

✓ Compilation successful
...
```

**Manual Compiler Build:**
```powershell
cd compiler/build
cmake --build .
```

## Platform Support

- ✅ **Windows** (PowerShell 5.1+, PowerShell 7+)
- ✅ **Linux** (PowerShell Core 7+)
- ✅ **macOS** (PowerShell Core 7+)

**Note**: On non-Windows platforms, you may need to adjust the executable path:
```powershell
# Linux/macOS might use:
$ParserExe = Join-Path $BuildDir "parse_file"
```

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Compilation error or file not found |

## Integration with dialOS

Once bytecode generation is implemented, compiled `.dsb` files can be:

1. **Stored in RAMFS** - Loaded into dialOS virtual filesystem
2. **Burned to EEPROM** - Distributed as I2C modules
3. **Downloaded via Network** - Installed over WiFi
4. **Executed by VM** - Run in dialOS bytecode interpreter

## Development Workflow

```powershell
# 1. Write your dialScript app
notepad lsp\myapp.ds

# 2. Compile and check syntax
.\build.ps1 lsp\myapp.ds -ShowDetails

# 3. View AST if needed
.\build.ps1 lsp\myapp.ds -ShowAST

# 4. Generate bytecode (when ready)
.\build.ps1 lsp\myapp.ds -Output build\myapp.dsb

# 5. Test on M5Dial
# (Upload myapp.dsb to device)
```

## Troubleshooting

### "Compiler not found" persists
```powershell
# Manually build the compiler
cd compiler
mkdir build
cd build
cmake ..
cmake --build .
cd ..\..
```

### "Parse errors" on valid code
- Check grammar alignment: See `compiler/GRAMMAR_ALIGNMENT.md`
- File encoding: Ensure UTF-8 without BOM
- Line endings: LF or CRLF both supported

### Performance issues
- Large files may take a moment to parse
- Current implementation is single-threaded
- Future: Incremental parsing and caching

## See Also

- `compiler/README.md` - Compiler documentation
- `compiler/GRAMMAR_ALIGNMENT.md` - Grammar specification
- `compiler/QUICKSTART.md` - Compiler quick reference
- `.github/copilot-instructions.md` - dialOS architecture

## Version History

- **v1.0** (Oct 2025) - Initial release with parsing and AST generation
- **v1.1** (Planned) - Bytecode generation
- **v2.0** (Planned) - Type checking and optimization
