#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Build and run the dialOS SDL Emulator

.DESCRIPTION
    Builds the SDL-based dialOS emulator with M5 Dial hardware simulation.
    Automatically handles SDL2 dependency installation on Windows via vcpkg.

.PARAMETER Script
    Path to the .ds script file to run in the emulator

.PARAMETER Compile
    Compile the script to bytecode first (default: true if .ds file provided)

.PARAMETER Install
    Install SDL2 dependencies via vcpkg (Windows only)

.PARAMETER Build
    Build the emulator executable only (don't run)

.PARAMETER Clean
    Clean build directory before building

.EXAMPLE
    .\build_emulator.ps1 scripts\counter_applet.ds
    Compile counter_applet.ds and run it in the emulator

.EXAMPLE
    .\build_emulator.ps1 -Install
    Install SDL2 dependencies via vcpkg

.EXAMPLE
    .\build_emulator.ps1 -Build
    Build the emulator executable only
#>

param(
    [string]$Script = "",
    [switch]$Compile,
    [switch]$Install = $false,
    [switch]$Build = $false,
    [switch]$Clean = $false
)

# Colors for output
$Green = "`e[32m"
$Yellow = "`e[33m"
$Red = "`e[31m"
$Blue = "`e[34m"
$Reset = "`e[0m"

function Write-Info($message) {
    Write-Host "${Blue}[INFO]${Reset} $message"
}

function Write-Success($message) {
    Write-Host "${Green}[SUCCESS]${Reset} $message"
}

function Write-Warning($message) {
    Write-Host "${Yellow}[WARNING]${Reset} $message"
}

function Write-Error($message) {
    Write-Host "${Red}[ERROR]${Reset} $message"
}

function Test-Command($command) {
    try {
        Get-Command $command -ErrorAction Stop | Out-Null
        return $true
    } catch {
        return $false
    }
}

Write-Info "dialOS SDL Emulator Build Script"
Write-Info "================================="
Write-Host ""

# Handle SDL installation
if ($Install) {
    Write-Info "Installing SDL2 dependencies..."
    
    if (-not (Test-Command "vcpkg")) {
        Write-Error "vcpkg not found in PATH"
        Write-Info "Please install vcpkg first:"
        Write-Info "1. git clone https://github.com/Microsoft/vcpkg.git"
        Write-Info "2. cd vcpkg && .\bootstrap-vcpkg.bat"
        Write-Info "3. Add vcpkg directory to PATH"
        exit 1
    }
    
    Write-Info "Installing SDL2 packages via vcpkg..."
    & vcpkg install sdl2:x64-windows sdl2-ttf:x64-windows sdl2-mixer:x64-windows
    
    if ($LASTEXITCODE -eq 0) {
        Write-Success "SDL2 dependencies installed successfully"
    } else {
        Write-Error "Failed to install SDL2 dependencies"
        exit 1
    }
    
    if (-not $Build -and -not $Script) {
        Write-Info "SDL2 installation complete. Use -Build to build emulator."
        exit 0
    }
}

# Change to compiler directory
Push-Location "compiler"

try {
    # Clean build if requested
    if ($Clean -and (Test-Path "build")) {
        Write-Info "Cleaning build directory..."
        Remove-Item -Recurse -Force "build"
    }
    
    # Create build directory
    if (-not (Test-Path "build")) {
        Write-Info "Creating build directory..."
        New-Item -ItemType Directory -Path "build" | Out-Null
    }
    
    # Configure CMake
    Push-Location "build"
    
    Write-Info "Configuring CMake build..."
    
    # Try to find vcpkg toolchain
    $vcpkgToolchain = ""
    if (Test-Command "vcpkg") {
        $vcpkgRoot = & vcpkg integrate install 2>&1 | Select-String "CMake projects should use" | ForEach-Object { $_.ToString().Split('"')[1] }
        if ($vcpkgRoot -and (Test-Path $vcpkgRoot)) {
            $vcpkgToolchain = "-DCMAKE_TOOLCHAIN_FILE=$vcpkgRoot"
            Write-Info "Using vcpkg toolchain: $vcpkgRoot"
        }
    }
    
    # Configure with CMake
    $configCmd = "cmake .. $vcpkgToolchain"
    Write-Info "Running: $configCmd"
    Invoke-Expression $configCmd
    
    if ($LASTEXITCODE -ne 0) {
        Write-Error "CMake configuration failed"
        exit 1
    }
    
    # Build
    Write-Info "Building dialOS emulator..."
    cmake --build . --config Debug
    
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed"
        exit 1
    }
    
    Write-Success "Build completed successfully"
    
    # Check if emulator was built
    $emulatorPath = "Debug/test_sdl_emulator.exe"
    if (-not (Test-Path $emulatorPath)) {
        $emulatorPath = "test_sdl_emulator.exe"
        if (-not (Test-Path $emulatorPath)) {
            Write-Warning "SDL emulator executable not found"
            Write-Info "This usually means SDL2 libraries were not found during build"
            Write-Info "Try running with -Install to install SDL2 dependencies"
            exit 1
        }
    }
    
    Write-Success "Emulator built: $emulatorPath"
    
    # If only building, stop here
    if ($Build) {
        Write-Info "Build-only requested, stopping here"
        exit 0
    }
    
    Pop-Location # Back to compiler dir
    
    # Handle script compilation and execution
    if ($Script) {
        $scriptPath = Resolve-Path -Path "../$Script" -ErrorAction SilentlyContinue
        if (-not $scriptPath -or -not (Test-Path $scriptPath)) {
            Write-Error "Script file not found: $Script"
            exit 1
        }
        
        Write-Info "Processing script: $scriptPath"
        
        # Determine bytecode output path
        $bytecodeFile = $scriptPath -replace '\.ds$', '.dsb'
        
        # Compile script to bytecode if requested or if bytecode doesn't exist
        if ($Compile -or -not (Test-Path $bytecodeFile)) {
            Write-Info "Compiling $scriptPath to bytecode..."
            
            $compilerPath = "build/Debug/compile.exe"
            if (-not (Test-Path $compilerPath)) {
                $compilerPath = "build/compile.exe"
                if (-not (Test-Path $compilerPath)) {
                    Write-Error "Compiler executable not found"
                    exit 1
                }
            }
            
            & $compilerPath $scriptPath $bytecodeFile
            
            if ($LASTEXITCODE -ne 0) {
                Write-Error "Script compilation failed"
                exit 1
            }
            
            Write-Success "Compiled to: $bytecodeFile"
        }
        
        # Run in emulator
        Write-Info "Starting dialOS emulator..."
        Write-Info "Controls:"
        Write-Info "  Mouse Wheel    - Rotary encoder"
        Write-Info "  Left Click     - Touch display / Encoder button"  
        Write-Info "  R Key          - Toggle RFID card simulation"
        Write-Info "  B Key          - Test buzzer beep"
        Write-Info "  D Key          - Toggle debug overlay" 
        Write-Info "  ESC Key        - Exit emulator"
        Write-Host ""
        
        # Execute emulator
        Push-Location "build"
        & $emulatorPath $bytecodeFile
        Pop-Location
        
    } else {
        Write-Info "No script provided. Emulator built successfully."
        Write-Info "Usage: $emulatorPath <script.dsb>"
    }
    
} finally {
    Pop-Location # Back to original directory
}

Write-Success "dialOS emulator ready!"