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
    Main command: registry, compile, run, setup

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

.EXAMPLE
    .\dscli.ps1 compile scripts\counter_applet.ds -DebugInfo
    Compile applet with debug line information

.EXAMPLE
    .\dscli.ps1 run scripts\counter_applet.ds -DebugInfo -ShowDebug
    Compile with debug info and run with debug overlay

.EXAMPLE
    .\dscli.ps1 disasm temp\counter_applet.dsb
    Disassemble a compiled bytecode file

.EXAMPLE
    .\dscli.ps1 disasm scripts\counter_applet.ds
    Compile and disassemble a source file

#>

param(
    [Parameter(Mandatory = $false, Position = 0)]
    [ValidateSet("registry", "compile", "run", "setup", "disasm")]
    [string]$Command,
    
    [Parameter(Mandatory = $false, Position = 1)]
    [string]$Action,
    
    [Parameter(Mandatory = $false, Position = 2)]
    [string]$Name,
    
    [string]$Source,
    [switch]$ShowDebug,
    [switch]$DebugInfo,
    [string]$Out
)

# Script configuration
$ErrorActionPreference = "Stop"
$ScriptRoot = $PSScriptRoot
$CompilerDir = Join-Path $ScriptRoot "compiler"
$BuildDir = Join-Path $CompilerDir "build"
$CompilerExe = Join-Path $BuildDir "Debug\compile.exe"
$SimulatorExe = Join-Path $BuildDir "Debug\test_sdl_emulator.exe"
$HeaderFile = Join-Path $ScriptRoot "src\vm_builtin_applets.h"
$BuiltinAppletsList = Join-Path $ScriptRoot "scripts\builtin_applets.txt"
$BuiltinAppletsDir = Join-Path $ScriptRoot "scripts"
$TempDir = Join-Path $ScriptRoot "temp"

# Colors for output
$Colors = @{
    Banner    = "Cyan"
    Success   = "Green"
    Warning   = "Yellow"
    Error     = "Red"
    Info      = "Gray"
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
    param(
        [string]$Message,
        [System.Management.Automation.ErrorRecord]$ErrorRecord = $null
    )

    Write-Host "❌ Error: $Message" -ForegroundColor $Colors.Error

    if ($ErrorRecord) {
        # Print a compact, wrapped exception block (type and message only)
        $ex = $ErrorRecord.Exception
        $typeName = $ex.GetType().FullName
        $msg = $ex.Message
        Write-Host "--- Exception (${typeName}) ---" -ForegroundColor $Colors.Error
        Write-Host "  $msg" -ForegroundColor $Colors.Error
        Write-Host "-------------------------------" -ForegroundColor $Colors.Error
    }

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
                Write-Host "❌ Compiler build failed" -ForegroundColor Red
                exit 1
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


# Build compiler
function Setup-Compiler {
    Write-Info "Building compiler..."
        
    Push-Location $BuildDir
    try {
        # Show build output to see compilation progress and any warnings/errors
        & cmake --build . --config Debug
        if ($LASTEXITCODE -ne 0) {
            Write-Host "❌ Compiler build failed" -ForegroundColor Red
            exit 1
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

# Core compilation function - handles running the compiler and displaying output
function Invoke-Compiler {
    param(
        [string]$SourcePath,
        [string]$OutputPath,
        [string[]]$AdditionalArgs = @(),
        [bool]$ShowOutput = $true
    )
    
    # Build compiler arguments
    $compilerArgs = @($SourcePath, $OutputPath) + $AdditionalArgs
    
    # Run compiler
    $compilerOutput = & $CompilerExe @compilerArgs 2>&1
    
    # Check if compilation failed
    $compilationFailed = ($LASTEXITCODE -ne 0)
    
    # Parse compiler output for errors
    $hasErrors = $false
    $inErrorSection = $false
    $errorLines = @()
    $normalOutput = @()
    
    foreach ($line in $compilerOutput) {
        $lineStr = $line.ToString().Trim()
        if ($lineStr -match "Compilation errors:|Parse errors:") {
            $hasErrors = $true
            $inErrorSection = $true
            continue
        }
        if ($inErrorSection -and $lineStr -ne "") {
            if ($lineStr -match "Note:|===|Bytecode|^Source:|^Parsing\.\.\.|^$") {
                $inErrorSection = $false
            }
            elseif ($lineStr -notmatch "System\.Management\.Automation\.") {
                # Filter out PowerShell exception type lines
                $errorLines += $lineStr
            }
        }
        elseif (-not $inErrorSection -and $lineStr -notmatch "System\.Management\.Automation\." -and $lineStr -ne "") {
            # Collect normal output
            $normalOutput += $lineStr
        }
    }
    
    # Display output based on compilation result
    if ($ShowOutput) {
        Write-Host ""
    }
    
    if ($compilationFailed -or ($hasErrors -and $errorLines.Count -gt 0)) {
        if ($ShowOutput) {
            Write-Host "❌ Compilation Errors:" -ForegroundColor $Colors.Error
            Write-Host "----------------------------------------" -ForegroundColor $Colors.Error

            if ($errorLines.Count -gt 0) {
                foreach ($errorLine in $errorLines) {
                    Write-Host "  $errorLine" -ForegroundColor $Colors.Error
                }
            }
            else {
                # Fall back to printing raw compiler output (filtered) so failures are visible
                $raw = $compilerOutput | ForEach-Object { $_.ToString() } | Where-Object { $_.Trim() -ne "" -and ($_ -notmatch "System\.Management\.Automation\.") }
                foreach ($line in $raw) {
                    Write-Host "  $line" -ForegroundColor $Colors.Error
                }
            }

            Write-Host "----------------------------------------" -ForegroundColor $Colors.Error
            Write-Host ""
            Write-Host "❌ Compilation failed" -ForegroundColor Red
        }
        return $false
    }
    else {
        # Show successful compilation output
        if ($ShowOutput -and $normalOutput.Count -gt 0) {
            foreach ($line in $normalOutput) {
                Write-Host "$line" -ForegroundColor $Colors.Info
            }
            Write-Host ""
        }
        return $true
    }
}

# Compile source to bytecode file
function Compile-ToBytecode {
    param([string]$SourcePath, [string]$OutputPath, [bool]$IncludeDebugInfo = $false)
    
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($SourcePath)

    if ($IncludeDebugInfo) {
        Write-Info "Compiling applet $baseName from $SourcePath to bytecode (with debug info)..."
    }
    else {
        Write-Info "Compiling applet $baseName from $SourcePath to bytecode..."
    }

    # Ensure output directory exists
    $outputDir = Split-Path $OutputPath -Parent
    if ($outputDir -and -not (Test-Path $outputDir)) {
        New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
    }
    
    try {
        # Prepare compiler arguments
        $additionalArgs = @()
        if ($IncludeDebugInfo) {
            $additionalArgs += "--debug"
        }
        
        # Run compiler
        $success = Invoke-Compiler -SourcePath $SourcePath -OutputPath $OutputPath -AdditionalArgs $additionalArgs -ShowOutput $true
        
        if (-not $success) {
            exit 1
        }
        
        if (-not (Test-Path $OutputPath)) {
            Write-Host "❌ Bytecode file not generated" -ForegroundColor Red
            exit 1
        }
        
        # Get file size
        $fileInfo = Get-Item $OutputPath
        $size = $fileInfo.Length
        
        Write-Success "Compiled applet $baseName to $OutputPath ($size bytes)"
        
        return $baseName
    }
    catch {
        Write-ErrorAndExit "Failed to compile $baseName : $($_.Exception.Message)" $_
    }
}

# Compile source to C array and optionally add to registry
function Compile-Applet {
    param([string]$SourcePath, [bool]$AddToRegistry = $false, [bool]$IncludeDebugInfo = $false)
    
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($SourcePath)
    $appletName = $baseName.ToUpper() -replace '[^A-Z0-9_]', '_'
    
    $debugText = if ($IncludeDebugInfo) { " (with debug info)" } else { "" }
    
    if ($AddToRegistry) {
        Write-Info "Compiling applet $baseName from $SourcePath and adding to registry$debugText..."
    }
    else {
        Write-Info "Compiling applet $baseName from $SourcePath$debugText..."
    }
    
    # Create temporary output file for C array
    $tempHeader = [System.IO.Path]::GetTempFileName()
    
    try {
        # Prepare compiler arguments
        $additionalArgs = @("--c-array")
        if ($IncludeDebugInfo) {
            $additionalArgs += "--debug"
        }
        
        # Run compiler
        $success = Invoke-Compiler -SourcePath $SourcePath -OutputPath $tempHeader -AdditionalArgs $additionalArgs -ShowOutput $true
        
        if (-not $success) {
            exit 1
        }
        
        # Read the generated C array
        $cArrayContent = Get-Content $tempHeader -Raw
        
        # Extract size for reporting
        $size = "unknown"
        if ($cArrayContent -match "const unsigned int ${appletName}_SIZE = (\d+);") {
            $size = $matches[1]
        }
        
        if ($AddToRegistry) {
            Write-Info "Compiled $baseName (dsb $size, c-array $($cArrayContent.Length) bytes)"
            return @{ bytecode = $cArrayContent; name = $baseName; size = $size }
        }
        else {
            Write-Success "Compiled $baseName ($size bytes)"
        }
        
        return $baseName
    }
    catch {
        Write-ErrorAndExit "Failed to compile applet $baseName : $($_.Exception.Message)" $_
    }
    finally {
        Remove-Item $tempHeader -ErrorAction SilentlyContinue
    }
}

# Add an applet to the registry
function Add-AppletToRegistry {
    if ([string]::IsNullOrEmpty($Name)) {
        Write-ErrorAndExit "Applet name required for add operation"
    }
    $applet = Compile-Applet -SourcePath $Name -AddToRegistry $true -IncludeDebugInfo $DebugInfo
    Write-Info "Adding $($applet.name): $($applet.bytecode.Length) bytes to registry..."
    # Normalize to base name (strip path and extension)
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($Name)

    if (Test-Path $BuiltinAppletsList) {
        # Read existing entries, ignore blank lines and comments
        $lines = Get-Content $BuiltinAppletsList | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" -and -not ($_ -like "#*") }

        # Check for existing entry (allow full name or path entries); case-insensitive
        $exists = $false
        foreach ($l in $lines) {
            if ([string]::Equals($l, $baseName, [System.StringComparison]::InvariantCultureIgnoreCase)) { $exists = $true; break }
            $lnBase = [System.IO.Path]::GetFileNameWithoutExtension($l)
            if ([string]::Equals($lnBase, $baseName, [System.StringComparison]::InvariantCultureIgnoreCase)) { $exists = $true; break }
        }

        if (-not $exists) {
            Set-Content -Path $BuiltinAppletsList -Value "$(Get-Content $BuiltinAppletsList -Raw)`n$baseName" -NoNewline
            Write-Success "Added $baseName to builtin applet list"
            # Regenerate header from built-in list
            Update-AppletRegistry
        }
        else {
            Write-Info "$baseName already present in $BuiltinAppletsList"
        }
    }
    else {
        # Create file and add this applet
        Set-Content -Path $BuiltinAppletsList -Value $baseName -NoNewline
        Write-Success "Created $BuiltinAppletsList and added $baseName"
        Update-AppletRegistry
    }
}

# Update the applet registry
function Update-AppletRegistry {
    if (-not (Test-Path $HeaderFile)) {
        return
    }
    
    # Generate registry code
    $registryCode = @"
#pragma once
// ===================== Applet Registry =====================
// This section is auto-generated - do not edit manually
#include <stdint.h>
#include <stddef.h>
// Generated bytecode arrays from dialScript (.ds) files


"@

    # Build registry strictly from the builtin applet list instead of parsing arrays
    $applets = [System.Collections.ArrayList]::new()
    if (Test-Path $BuiltinAppletsList) {
        $lines = Get-Content $BuiltinAppletsList | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" -and -not ($_ -like "#*") }
        foreach ($line in $lines) {
            # Normalize to base name (handles paths and extensions)
            $appletSourceFile = "$(Join-Path $BuiltinAppletsDir $line).ds"
            Write-Info "Processing builtin applet entry: $appletSourceFile"
            if ([string]::IsNullOrEmpty($appletSourceFile)) { Write-ErrorAndExit "Invalid applet source file: $appletSourceFile" }

            $applet = Compile-Applet -SourcePath $appletSourceFile -AddToRegistry $true -IncludeDebugInfo $false

            $arrayName = $applet.name.ToUpper() -replace '[^A-Z0-9_]', '_'
            $applets.Add(@{
                    FileName  = $applet.name
                    ArrayName = $arrayName
                    Size      = $applet.bytecode.Length
                }) | Out-Null

            # Create new header file
            $registryCode += @"
$($applet.bytecode)
"@
        }
    }

    $registryCode += @"
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
static VMApplet BUILTIN_APPLET_REGISTRY[] = {};

static const int BUILTIN_APPLET_REGISTRY_SIZE = 0;
"@
    }
    else {
        $registryCode += "static VMApplet BUILTIN_APPLET_REGISTRY[] = {`n"

        foreach ($applet in $applets) {
            $name = $applet.FileName.ToLower()
            $arrayName = $applet.ArrayName
            # Generate entry using the expected array and size identifiers generated by the compiler
            $registryCode += "    {`"$name`", $arrayName, ${arrayName}_SIZE, 0, false},`n"
        }

        $registryCode += @"
};

static const int BUILTIN_APPLET_REGISTRY_SIZE = sizeof(BUILTIN_APPLET_REGISTRY) / sizeof(VMApplet);
"@
    }
    
    $registryCode += @"

// ===================== End Registry =====================
"@

    Set-Content -Path $HeaderFile -Value $registryCode -NoNewline
    Write-Success "$(Get-Content -Path $HeaderFile -Raw)"
}

# List all applets in registry
function Show-Registry {
    if (-not (Test-Path $HeaderFile)) {
        Write-Warning "No applets found. Registry is empty."
        return
    }
    
    $existingContent = Get-Content $HeaderFile -Raw
    
    # Find the APPLET_REGISTRY array
    $registryPattern = '(?s)static VMApplet APPLET_REGISTRY\[\] = \{(.*?)\};'
    $registryMatch = [regex]::Match($existingContent, $registryPattern)
    
    if (-not $registryMatch.Success) {
        Write-Warning "Registry is empty."
        return
    }
    
    $registryContent = $registryMatch.Groups[1].Value
    
    # Parse individual applet entries
    $appletPattern = '\{\s*"([^"]+)"\s*,\s*[^,]+\s*,\s*[^,]+\s*,\s*(\d+)\s*,\s*(true|false)\s*\}'
    $appletMatches = [regex]::Matches($registryContent, $appletPattern)
    
    if ($appletMatches.Count -eq 0) {
        Write-Warning "Registry is empty."
        return
    }
    
    Write-Host "Registry contains $($appletMatches.Count) applet(s):" -ForegroundColor $Colors.Info
    Write-Host ""
    Write-Host "  Name                Size (bytes)  Interval (ms)  Repeat" -ForegroundColor $Colors.Info
    Write-Host "  ──────────────────  ────────────  ─────────────  ──────" -ForegroundColor $Colors.Info
    
    foreach ($match in $appletMatches) {
        $name = $match.Groups[1].Value
        $interval = $match.Groups[2].Value
        $repeat = $match.Groups[3].Value
        
        # Get size from the bytecode array definition
        $upperName = $name.ToUpper()
        $sizePattern = "const unsigned int ${upperName}_SIZE = (\d+);"
        $sizeMatch = [regex]::Match($existingContent, $sizePattern)
        $size = if ($sizeMatch.Success) { $sizeMatch.Groups[1].Value } else { "unknown" }
        
        $repeatDisplay = if ($repeat -eq "true") { "Yes" } else { "No" }
        $intervalDisplay = if ($interval -eq "0") { "Once" } else { "$interval" }
        
        Write-Host ("  {0,-18}  {1,12}  {2,12}  {3,6}" -f $name, $size, $intervalDisplay, $repeatDisplay) -ForegroundColor $Colors.Highlight
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
    param([string]$SourcePath, [bool]$Debug = $false, [bool]$IncludeDebugInfo = $false)
    
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($SourcePath)
    $extension = [System.IO.Path]::GetExtension($SourcePath).ToLower()
    $tempDsb = $null
    
    try {
        # Determine if we need to compile or use existing bytecode
        if ($extension -eq ".dsb") {
            # Already compiled bytecode file
            $dsbFile = $SourcePath
            Write-Info "Running bytecode file $baseName directly..."
        }
        else {
            # Source file - need to compile
            $tempDsb = Join-Path $TempDir "$baseName.dsb"
            
            if ($IncludeDebugInfo) {
                Write-Info "Compiling applet $baseName from $SourcePath for simulation (with debug info)..."
            }
            else {
                Write-Info "Compiling applet $baseName from $SourcePath for simulation..."
            }
            
            # Prepare compiler arguments
            $additionalArgs = @()
            if ($IncludeDebugInfo) {
                $additionalArgs += "--debug"
            }
            
            # Run compiler
            $success = Invoke-Compiler -SourcePath $SourcePath -OutputPath $tempDsb -AdditionalArgs $additionalArgs -ShowOutput $true
            
            if (-not $success) {
                exit 1
            }

            if (-not (Test-Path $tempDsb)) {
                Write-Host "❌ Bytecode file not generated" -ForegroundColor Red
                exit 1
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
        Write-ErrorAndExit "Failed to run simulation: $($_.Exception.Message)" $_
    }
    finally {
        # Clean up temporary file if we created one
        if ($tempDsb -and (Test-Path $tempDsb)) {
            Remove-Item $tempDsb -ErrorAction SilentlyContinue
        }
    }
}

# Disassemble bytecode file or compile and disassemble source
function Show-Disassembly {
    param([string]$SourcePath, [bool]$IncludeDebugInfo = $false)
    
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($SourcePath)
    $extension = [System.IO.Path]::GetExtension($SourcePath).ToLower()
    $tempDsb = $null
    
    try {
        Write-Host ""
        Write-Host "╔═══════════════════════════════════════════════════════╗" -ForegroundColor $Colors.Banner
        Write-Host "║                Bytecode Disassembly                   ║" -ForegroundColor $Colors.Banner
        Write-Host "╚═══════════════════════════════════════════════════════╝" -ForegroundColor $Colors.Banner
        Write-Host ""
        
        if ($extension -eq ".dsb") {
            # Disassemble existing bytecode file
            Write-Info "Disassembling bytecode file: $baseName.dsb"
            Write-Host ""
            
            # Run compiler in disassemble mode (it detects .dsb extension)
            $compilerOutput = & $CompilerExe $SourcePath 2>&1
            
            if ($LASTEXITCODE -ne 0) {
                Write-Host "❌ Failed to disassemble bytecode file" -ForegroundColor Red
                exit 1
            }
            
            # Display disassembly output with color coding
            foreach ($line in $compilerOutput) {
                $lineStr = $line.ToString()
                if ($lineStr -notmatch "System\.Management\.Automation\." -and $lineStr.Trim() -ne "") {
                    if ($lineStr -match "^===") {
                        Write-Host $lineStr -ForegroundColor $Colors.Banner
                    }
                    elseif ($lineStr -match "^\s*\[\d+\]|^Functions|^Constants|^Globals|^Main Entry|^Metadata:|^Debug Info:|^Code \(") {
                        Write-Host $lineStr -ForegroundColor $Colors.Highlight
                    }
                    elseif ($lineStr -match "^\d{6}") {
                        Write-Host $lineStr -ForegroundColor $Colors.Info
                    }
                    else {
                        Write-Host $lineStr -ForegroundColor $Colors.Info
                    }
                }
            }
        }
        else {
            # Source file - compile first (compilation automatically shows disassembly)
            $tempDsb = Join-Path $TempDir "$baseName.dsb"
            
            if ($IncludeDebugInfo) {
                Write-Info "Compiling and disassembling $baseName (with debug info)..."
            }
            else {
                Write-Info "Compiling and disassembling $baseName..."
            }
            Write-Host ""
            
            # Prepare compiler arguments
            $additionalArgs = @()
            if ($IncludeDebugInfo) {
                $additionalArgs += "--debug"
            }
            
            # Run compiler - it automatically shows disassembly
            $compilerArgs = @($SourcePath, $tempDsb) + $additionalArgs
            $compilerOutput = & $CompilerExe @compilerArgs 2>&1
            
            # Check if compilation failed
            $compilationFailed = ($LASTEXITCODE -ne 0)
            
            if ($compilationFailed) {
                # Parse and show errors
                $hasErrors = $false
                $inErrorSection = $false
                $errorLines = @()
                
                foreach ($line in $compilerOutput) {
                    $lineStr = $line.ToString().Trim()
                    if ($lineStr -match "Compilation errors:|Parse errors:") {
                        $hasErrors = $true
                        $inErrorSection = $true
                        continue
                    }
                    if ($inErrorSection -and $lineStr -ne "") {
                        if ($lineStr -notmatch "System\.Management\.Automation\.") {
                            $errorLines += $lineStr
                        }
                    }
                }
                
                if ($hasErrors -and $errorLines.Count -gt 0) {
                    Write-Host "❌ Compilation Errors:" -ForegroundColor $Colors.Error
                    Write-Host "----------------------------------------" -ForegroundColor $Colors.Error
                    foreach ($errorLine in $errorLines) {
                        Write-Host "  $errorLine" -ForegroundColor $Colors.Error
                    }
                    Write-Host "----------------------------------------" -ForegroundColor $Colors.Error
                }
                Write-Host ""
                exit 1
            }
            
            # Display the compiler output (includes disassembly)
            foreach ($line in $compilerOutput) {
                $lineStr = $line.ToString()
                if ($lineStr -notmatch "System\.Management\.Automation\." -and $lineStr.Trim() -ne "") {
                    if ($lineStr -match "^===") {
                        Write-Host $lineStr -ForegroundColor $Colors.Banner
                    }
                    elseif ($lineStr -match "^\s*\[\d+\]|^Functions|^Constants|^Globals|^Main Entry|^Metadata:|^Debug Info:|^Code \(") {
                        Write-Host $lineStr -ForegroundColor $Colors.Highlight
                    }
                    elseif ($lineStr -match "^\d{6}") {
                        Write-Host $lineStr -ForegroundColor $Colors.Info
                    }
                    else {
                        Write-Host $lineStr -ForegroundColor $Colors.Info
                    }
                }
            }
        }
        
        Write-Host ""
        Write-Success "Disassembly complete"
    }
    catch {
        Write-ErrorAndExit "Failed to disassemble: $($_.Exception.Message)" $_
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
    Write-Host "    dscli compile <source.ds> -DebugInfo   Include debug line information" -ForegroundColor $Colors.Info
    Write-Host "    dscli compile <source.ds> -Out <.dsb> Compile to bytecode file" -ForegroundColor $Colors.Info
    Write-Host "    dscli run <source.ds|.dsb>            Run simulator (compile if needed)" -ForegroundColor $Colors.Info
    Write-Host "    dscli run <source.ds|.dsb> -ShowDebug Run with debug overlay" -ForegroundColor $Colors.Info
    Write-Host "    dscli run <source.ds> -DebugInfo      Compile with debug info and run" -ForegroundColor $Colors.Info
    Write-Host "    dscli disasm <source.ds|.dsb>         Show bytecode disassembly" -ForegroundColor $Colors.Info
    Write-Host "    dscli disasm <source.ds> -DebugInfo   Disassemble with debug info" -ForegroundColor $Colors.Info
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor $Colors.Highlight
    Write-Host "  .\dscli.ps1 registry list" -ForegroundColor $Colors.Info
    Write-Host "  .\dscli.ps1 compile scripts\counter_applet.ds" -ForegroundColor $Colors.Info
    Write-Host "  .\dscli.ps1 compile scripts\counter_applet.ds -Register" -ForegroundColor $Colors.Info
    Write-Host "  .\dscli.ps1 compile scripts\test.ds -Out build\test.dsb" -ForegroundColor $Colors.Info
    Write-Host "  .\dscli.ps1 compile scripts\test.ds -DebugInfo" -ForegroundColor $Colors.Info
    Write-Host "  .\dscli.ps1 run scripts\timer.ds -ShowDebug" -ForegroundColor $Colors.Info
    Write-Host "  .\dscli.ps1 run scripts\timer.ds -DebugInfo -ShowDebug" -ForegroundColor $Colors.Info
    Write-Host "  .\dscli.ps1 run temp\hello_world.dsb" -ForegroundColor $Colors.Info
    Write-Host "  .\dscli.ps1 disasm temp\counter.dsb" -ForegroundColor $Colors.Info
    Write-Host "  .\dscli.ps1 disasm scripts\timer.ds -DebugInfo" -ForegroundColor $Colors.Info
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
                "refresh" {
                    Update-AppletRegistry
                    Write-Success "Applet registry refreshed"
                }
                "add" {
                    Add-AppletToRegistry -AppletName $Name -SourcePath $Source
                    Write-Success "Applet $Name added to registry"
                }
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
                # Compile to bytecode file
                Compile-ToBytecode -SourcePath $SourcePath -OutputPath $Out -IncludeDebugInfo $DebugInfo
            }
            else {
                # Just compile applet
                Compile-Applet -SourcePath $SourcePath -IncludeDebugInfo $DebugInfo
            }
        }
        
        "run" {
            if ([string]::IsNullOrEmpty($Action)) {
                Write-ErrorAndExit "Source file required for run"
            }
            
            $SourcePath = Resolve-SourcePath -Path $Action
            Run-Simulator -SourcePath $SourcePath -Debug $ShowDebug -IncludeDebugInfo $DebugInfo
        }

        "disasm" {
            if ([string]::IsNullOrEmpty($Action)) {
                Write-ErrorAndExit "Source or bytecode file required for disasm"
            }
            
            $SourcePath = Resolve-SourcePath -Path $Action
            Show-Disassembly -SourcePath $SourcePath -IncludeDebugInfo $DebugInfo
        }

        "setup" {
            Setup-Compiler
        }
        
        default {
            Write-ErrorAndExit "Unknown command: $Command"
            Show-Usage
        }
    }
    
}
catch {
    Write-Host ""
    Write-ErrorAndExit $_.Exception.Message
}