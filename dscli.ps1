#!/usr/bin/env pwsh
<#
.SYNOPSIS
    dialOS Command Line Interface - Unified development tool

.DESCRIPTION
    A unified tool for developing dialOS applets:
    - Compile dialScript (.ds) files
    - Manage the built-in applet registry
    - Run the SDL simulator
    - Automatic cleanup of temporary files

.PARAMETER Command
    Main command: registry, compile, run

.PARAMETER Action
    Sub-command for registry: add, list, remove

.PARAMETER Name
    Name of the applet (for registry operations)

.PARAMETER Source
    Path to the .ds source file

.PARAMETER ShowDebug
    Start simulator with debug overlay enabled

.EXAMPLE
    .\dscli.ps1 registry list
    List all applets in the registry

.EXAMPLE
    .\dscli.ps1 registry add counter_applet
    Add counter_applet to the registry (must be compiled first)

.EXAMPLE
    .\dscli.ps1 registry remove counter_applet
    Remove counter_applet from the registry

.EXAMPLE
    .\dscli.ps1 compile scripts\counter_applet.ds
    Compile applet (without adding to registry)

.EXAMPLE
    .\dscli.ps1 compile scripts\counter_applet.ds -Register
    Compile applet and add to registry

.EXAMPLE
    .\dscli.ps1 compile scripts\counter_applet.ds -Out build\counter.dsb
    Compile applet to specific bytecode file

.EXAMPLE
    .\dscli.ps1 run scripts\counter_applet.ds
    Compile applet and run in simulator

.EXAMPLE
    .\dscli.ps1 run scripts\counter_applet.ds -ShowDebug
    Run with debug overlay enabled

#>

param(
    [Parameter(Mandatory=$false, Position=0)]
    [ValidateSet("registry", "compile", "run")]
    [string]$Command,
    
    [Parameter(Mandatory=$false, Position=1)]
    [string]$Action,
    
    [Parameter(Mandatory=$false, Position=2)]
    [string]$Name,
    
    [string]$Source,
    [switch]$ShowDebug,
    [switch]$Register,
    [string]$Out
)

# Script configuration
$ErrorActionPreference = "Stop"
$ScriptRoot = $PSScriptRoot
$CompilerDir = Join-Path $ScriptRoot "compiler"
$BuildDir = Join-Path $CompilerDir "build"
$CompilerExe = Join-Path $BuildDir "Debug\compile.exe"
$SimulatorExe = Join-Path $BuildDir "Debug\test_sdl_emulator.exe"
$HeaderFile = Join-Path $ScriptRoot "src\vm_applet_data.h"
$TempDir = Join-Path $ScriptRoot "temp"

# Colors for output
$Colors = @{
    Banner = "Cyan"
    Success = "Green"
    Warning = "Yellow"
    Error = "Red"
    Info = "Gray"
    Highlight = "White"
}

# Ensure temp directory exists
if (-not (Test-Path $TempDir)) {
    New-Item -ItemType Directory -Path $TempDir -Force | Out-Null
}

# Clean up temp files on exit
Register-EngineEvent PowerShell.Exiting -Action {
    if (Test-Path $TempDir) {
        Remove-Item "$TempDir\*.dsb" -ErrorAction SilentlyContinue
    }
} | Out-Null

# Banner
function Write-Banner {
    Write-Host "╔═══════════════════════════════════════════════════════╗" -ForegroundColor $Colors.Banner
    Write-Host "║              dialOS CLI - Development Tool            ║" -ForegroundColor $Colors.Banner
    Write-Host "╚═══════════════════════════════════════════════════════╝" -ForegroundColor $Colors.Banner
    Write-Host ""
}

# Error handling
function Write-ErrorAndExit {
    param([string]$Message)
    Write-Host "❌ Error: $Message" -ForegroundColor $Colors.Error
    exit 1
}

# Success message
function Write-Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor $Colors.Success
}

# Warning message
function Write-Warning {
    param([string]$Message)
    Write-Host "⚠ $Message" -ForegroundColor $Colors.Warning
}

# Info message
function Write-Info {
    param([string]$Message)
    Write-Host "$Message" -ForegroundColor $Colors.Info
}

# Build compiler if needed
function Ensure-Compiler {
    if (-not (Test-Path $CompilerExe)) {
        Write-Info "Building compiler..."
        
        Push-Location $BuildDir
        try {
            & cmake --build . --config Debug 2>&1 | Out-Null
            if ($LASTEXITCODE -ne 0) {
                throw "Compiler build failed"
            }
            Write-Success "Compiler built successfully"
        }
        catch {
            Write-ErrorAndExit "Failed to build compiler: $($_.Exception.Message)"
        }
        finally {
            Pop-Location
        }
    }
}

# Resolve source file path
function Resolve-SourcePath {
    param([string]$Path)
    
    if (-not (Test-Path $Path)) {
        # Try relative to script root
        $RelPath = Join-Path $ScriptRoot $Path
        if (Test-Path $RelPath) {
            return (Resolve-Path $RelPath).Path
        }
        
        Write-ErrorAndExit "Source file not found: $Path"
    }
    
    return (Resolve-Path $Path).Path
}

# Compile source to bytecode file
function Compile-ToBytecode {
    param([string]$SourcePath, [string]$OutputPath)
    
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($SourcePath)

    Write-Info "Compiling applet $baseName from $SourcePath to bytecode..."

    # Ensure output directory exists
    $outputDir = Split-Path $OutputPath -Parent
    if ($outputDir -and -not (Test-Path $outputDir)) {
        New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
    }
    
    try {
        # Run compiler to generate bytecode
        & $CompilerExe $SourcePath $OutputPath 2>&1 | Out-Null
        
        if ($LASTEXITCODE -ne 0) {
            throw "Compilation failed"
        }
        
        if (-not (Test-Path $OutputPath)) {
            throw "Bytecode file not generated"
        }
        
        # Get file size
        $fileInfo = Get-Item $OutputPath
        $size = $fileInfo.Length
        
        Write-Success "Compiled applet $baseName to $OutputPath ($size bytes)"
        
        return $baseName
    }
    catch {
        Write-ErrorAndExit "Failed to compile $baseName : $($_.Exception.Message)"
    }
}

# Compile source to C array and optionally add to registry
function Compile-Applet {
    param([string]$SourcePath, [bool]$AddToRegistry = $false)
    
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($SourcePath)
    $appletName = $baseName.ToUpper() -replace '[^A-Z0-9_]', '_'
    
    if ($AddToRegistry) {
        Write-Info "Compiling applet $baseName from $SourcePath and adding to registry..."
    } else {
        Write-Info "Compiling applet $baseName from $SourcePath..."
    }
    
    # Create temporary output file for C array
    $tempHeader = [System.IO.Path]::GetTempFileName()
    
    try {
        # Run compiler with --c-array flag
        $compilerArgs = @($SourcePath, $tempHeader, "--c-array")
        & $CompilerExe @compilerArgs 2>&1 | Out-Null
        
        if ($LASTEXITCODE -ne 0) {
            throw "Compilation failed"
        }
        
        # Read the generated C array
        $cArrayContent = Get-Content $tempHeader -Raw
        
        # Extract size for reporting
        $size = "unknown"
        if ($cArrayContent -match "const unsigned int ${appletName}_SIZE = (\d+);") {
            $size = $matches[1]
        }
        
        if ($AddToRegistry) {
            # Update header file
            if (-not (Test-Path $HeaderFile)) {
                # Create new header file
                $headerContent = @"
#pragma once

// Generated bytecode arrays from dialScript (.ds) files

$cArrayContent
"@
                Set-Content -Path $HeaderFile -Value $headerContent -NoNewline
                Write-Success "Created new header file"
            }
            else {
                # Check if this applet already exists in the header
                $existingContent = Get-Content $HeaderFile -Raw
                
                if ($existingContent -match "const unsigned char ${appletName}\[\]") {
                    Write-Warning "Applet $baseName already exists, replacing..."
                    
                    # Remove old applet definition
                    $pattern = "(?s)// Generated bytecode array.*?const unsigned char ${appletName}\[\].*?\};.*?const unsigned int ${appletName}_SIZE = \d+;"
                    $existingContent = $existingContent -replace $pattern, ""
                    $existingContent = $existingContent -replace "(\r?\n){3,}", "`n`n"
                }
                
                # Append new applet
                $newContent = $existingContent.TrimEnd() + "`n`n" + $cArrayContent
                Set-Content -Path $HeaderFile -Value $newContent -NoNewline
            }
            
            # Update registry
            Update-AppletRegistry
            
            Write-Success "Compiled $baseName ($size bytes) and added to registry"
        } else {
            Write-Success "Compiled $baseName ($size bytes)"
        }
        
        return $baseName
    }
    catch {
        Write-ErrorAndExit "Failed to compile applet $baseName : $($_.Exception.Message)"
    }
    finally {
        Remove-Item $tempHeader -ErrorAction SilentlyContinue
    }
}

# Update the applet registry
function Update-AppletRegistry {
    if (-not (Test-Path $HeaderFile)) {
        return
    }
    
    $existingContent = Get-Content $HeaderFile -Raw
    
    # Remove old registry
    $registryPattern = "(?s)// =+ Applet Registry =+.*?// =+ End Registry =+"
    $existingContent = $existingContent -replace $registryPattern, ""
    
    # Find all applet definitions
    $applets = [System.Collections.ArrayList]@()
    $pattern = '// Generated bytecode array from .*[\\/](\w+)\.ds\s+// Total size: (\d+) bytes\s+const unsigned char (\w+)\[\]'
    $regexMatches = [regex]::Matches($existingContent, $pattern)
    
    foreach ($match in $regexMatches) {
        $fileName = $match.Groups[1].Value
        $size = $match.Groups[2].Value
        $arrayName = $match.Groups[3].Value
        
        $applets.Add(@{
            FileName = $fileName
            ArrayName = $arrayName
            Size = $size
        }) | Out-Null
    }
    
    # Generate registry code
    $registryCode = @"

// ===================== Applet Registry =====================
// This section is auto-generated - do not edit manually

struct VMApplet {
  const char *name;
  const unsigned char *bytecode;
  size_t bytecodeSize;
  uint32_t executeInterval;  // ms between executions (0 = run once)
  bool repeat;               // true = repeat indefinitely, false = run once
};

"@
    
    if ($applets.Count -eq 0) {
        $registryCode += @"
static VMApplet APPLET_REGISTRY[] = {};

static const int APPLET_REGISTRY_SIZE = 0;
"@
    } else {
        $registryCode += "static VMApplet APPLET_REGISTRY[] = {`n"
        
        foreach ($applet in $applets) {
            $name = $applet.FileName.ToLower()
            $arrayName = $applet.ArrayName
            $registryCode += "    {`"$name`", $arrayName, ${arrayName}_SIZE, 2000, true},`n"
        }
        
        $registryCode += @"
};

static const int APPLET_REGISTRY_SIZE = sizeof(APPLET_REGISTRY) / sizeof(VMApplet);
"@
    }
    
    $registryCode += @"

// ===================== End Registry =====================
"@
    
    # Append registry to content
    $newContent = $existingContent.TrimEnd() + "`n" + $registryCode
    Set-Content -Path $HeaderFile -Value $newContent -NoNewline
}

# List all applets in registry
function Show-Registry {
    if (-not (Test-Path $HeaderFile)) {
        Write-Warning "No applets found. Registry is empty."
        return
    }
    
    $existingContent = Get-Content $HeaderFile -Raw
    
    # Find all applet definitions
    $pattern = '// Generated bytecode array from .*[\\/](\w+)\.ds\s+// Total size: (\d+) bytes\s+const unsigned char (\w+)\[\]'
    $regexMatches = [regex]::Matches($existingContent, $pattern)
    
    if ($regexMatches.Count -eq 0) {
        Write-Warning "Registry is empty."
        return
    }
    
    Write-Host "Registry contains $($regexMatches.Count) applet(s):" -ForegroundColor $Colors.Info
    Write-Host ""
    Write-Host "  Name                Size (bytes)" -ForegroundColor $Colors.Info
    Write-Host "  ──────────────────  ────────────" -ForegroundColor $Colors.Info
    
    foreach ($match in $regexMatches) {
        $fileName = $match.Groups[1].Value
        $size = $match.Groups[2].Value
        
        Write-Host ("  {0,-18}  {1,12}" -f $fileName, $size) -ForegroundColor $Colors.Highlight
    }
    
    Write-Host ""
}

# Remove applet from registry
function Remove-FromRegistry {
    param([string]$AppletName)
    
    if (-not (Test-Path $HeaderFile)) {
        Write-ErrorAndExit "Registry is empty"
    }
    
    # Normalize applet name
    $arrayName = $AppletName.ToUpper() -replace '[^A-Z0-9_]', '_'
    
    $existingContent = Get-Content $HeaderFile -Raw
    
    # Check if applet exists
    if ($existingContent -notmatch "const unsigned char ${arrayName}\[\]") {
        Write-ErrorAndExit "Applet '$AppletName' not found in registry"
    }
    
    Write-Info "Removing $AppletName from registry..."
    
    # Remove applet definition
    $pattern = "(?s)(\r?\n)?// Generated bytecode array[^\r\n]*\r?\n// Total size:[^\r\n]*\r?\n\r?\nconst unsigned char ${arrayName}\[\][^\}]*\};\r?\n\r?\nconst unsigned int ${arrayName}_SIZE = \d+;(\r?\n)+"
    $newContent = $existingContent -replace $pattern, "`n"
    
    # Clean up multiple blank lines
    $newContent = $newContent -replace "(\r?\n){3,}", "`r`n`r`n"
    
    Set-Content -Path $HeaderFile -Value $newContent -NoNewline
    
    # Update registry
    Update-AppletRegistry
    
    Write-Success "Removed $AppletName from registry"
}

# Run simulator with applet
function Run-Simulator {
    param([string]$SourcePath, [bool]$Debug = $false)
    
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($SourcePath)
    $extension = [System.IO.Path]::GetExtension($SourcePath).ToLower()
    $tempDsb = $null
    
    try {
        # Determine if we need to compile or use existing bytecode
        if ($extension -eq ".dsb") {
            # Already compiled bytecode file
            $dsbFile = $SourcePath
            Write-Info "Running bytecode file $baseName directly..."
        } else {
            # Source file - need to compile
            $tempDsb = Join-Path $TempDir "$baseName.dsb"
            
            Write-Info "Compiling applet $baseName from $SourcePath for simulation..."
            
            # Compile to bytecode
            & $CompilerExe $SourcePath $tempDsb 2>&1 | Out-Null
            
            if ($LASTEXITCODE -ne 0) {
                throw "Compilation failed"
            }
            
            if (-not (Test-Path $tempDsb)) {
                throw "Bytecode file not generated"
            }
            
            Write-Success "Compiled successfully"
            $dsbFile = $tempDsb
        }
        
        Write-Info "Starting SDL simulator..."
        
        if ($Debug) {
            Write-Info "Debug overlay will be enabled (press 'D' to toggle)"
        }
        
        Write-Host ""
        Write-Host "╔═══════════════════════════════════════════════════════╗" -ForegroundColor $Colors.Banner
        Write-Host "║                   SDL Simulator                       ║" -ForegroundColor $Colors.Banner
        Write-Host "║  Controls: D=Debug, R=RFID Card, ESC=Exit             ║" -ForegroundColor $Colors.Banner
        Write-Host "╚═══════════════════════════════════════════════════════╝" -ForegroundColor $Colors.Banner
        Write-Host ""
        
        # Run simulator
        & $SimulatorExe $dsbFile
        
        Write-Host ""
        Write-Success "Simulation completed"
    }
    catch {
        Write-ErrorAndExit "Failed to run simulation: $($_.Exception.Message)"
    }
    finally {
        # Clean up temporary file if we created one
        if ($tempDsb -and (Test-Path $tempDsb)) {
            Remove-Item $tempDsb -ErrorAction SilentlyContinue
        }
    }
}

# Show usage information
function Show-Usage {
    Write-Host "Usage:" -ForegroundColor $Colors.Info
    Write-Host ""
    Write-Host "  Registry Management:" -ForegroundColor $Colors.Highlight
    Write-Host "    dscli registry list                   List all applets" -ForegroundColor $Colors.Info
    Write-Host "    dscli registry remove <name>          Remove applet from registry" -ForegroundColor $Colors.Info
    Write-Host ""
    Write-Host "  Development:" -ForegroundColor $Colors.Highlight
    Write-Host "    dscli compile <source.ds>             Compile applet only" -ForegroundColor $Colors.Info
    Write-Host "    dscli compile <source.ds> -Register   Compile and add to registry" -ForegroundColor $Colors.Info
    Write-Host "    dscli compile <source.ds> -Out <.dsb> Compile to bytecode file" -ForegroundColor $Colors.Info
    Write-Host "    dscli run <source.ds|.dsb>            Run simulator (compile if needed)" -ForegroundColor $Colors.Info
    Write-Host "    dscli run <source.ds|.dsb> -ShowDebug Run with debug overlay" -ForegroundColor $Colors.Info
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor $Colors.Highlight
    Write-Host "  .\dscli.ps1 registry list" -ForegroundColor $Colors.Info
    Write-Host "  .\dscli.ps1 compile scripts\counter_applet.ds" -ForegroundColor $Colors.Info
    Write-Host "  .\dscli.ps1 compile scripts\counter_applet.ds -Register" -ForegroundColor $Colors.Info
    Write-Host "  .\dscli.ps1 compile scripts\test.ds -Out build\test.dsb" -ForegroundColor $Colors.Info
    Write-Host "  .\dscli.ps1 run scripts\timer.ds -ShowDebug" -ForegroundColor $Colors.Info
    Write-Host "  .\dscli.ps1 run temp\hello_world.dsb" -ForegroundColor $Colors.Info
}

# Main execution
try {
    Write-Banner
    
    # Show help if no command provided
    if ([string]::IsNullOrEmpty($Command)) {
        Show-Usage
        exit 0
    }
    
    # Ensure compiler is built
    Ensure-Compiler
    
    switch ($Command.ToLower()) {
        "registry" {
            if ([string]::IsNullOrEmpty($Action)) {
                Write-ErrorAndExit "Registry action required: list, remove"
            }
            
            switch ($Action.ToLower()) {
                "list" {
                    Show-Registry
                }
                "remove" {
                    if ([string]::IsNullOrEmpty($Name)) {
                        Write-ErrorAndExit "Applet name required for remove operation"
                    }
                    Remove-FromRegistry -AppletName $Name
                }
                default {
                    Write-ErrorAndExit "Unknown registry action: $Action (valid: list, remove)"
                }
            }
        }
        
        "compile" {
            if ([string]::IsNullOrEmpty($Action)) {
                Write-ErrorAndExit "Source file required for compile"
            }
            
            $SourcePath = Resolve-SourcePath -Path $Action
            
            # Check if user wants to output to a specific .dsb file
            if (-not [string]::IsNullOrEmpty($Out)) {
                # Warn if both -Out and -Register are specified
                if ($Register) {
                    Write-Warning "Both -Out and -Register specified. Compiling to bytecode file only (ignoring -Register)"
                }
                # Compile to bytecode file
                Compile-ToBytecode -SourcePath $SourcePath -OutputPath $Out
            } else {
                # Compile to C array (with optional registry addition)
                Compile-Applet -SourcePath $SourcePath -AddToRegistry $Register
            }
        }
        
        "run" {
            if ([string]::IsNullOrEmpty($Action)) {
                Write-ErrorAndExit "Source file required for run"
            }
            
            $SourcePath = Resolve-SourcePath -Path $Action
            Run-Simulator -SourcePath $SourcePath -Debug $ShowDebug
        }
        
        default {
            Write-ErrorAndExit "Unknown command: $Command"
            Show-Usage
        }
    }
    
} catch {
    Write-Host ""
    Write-ErrorAndExit $_.Exception.Message
}