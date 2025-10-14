#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Build script for dialScript (.ds) files

.DESCRIPTION
    Compiles dialScript source files using the dialOS compiler.
    Default behavior: Compile to C byte array and append to vm_applet_data.h
    Supports parsing, AST generation, JSON export, and bytecode compilation.

.PARAMETER Source
    Path to the .ds source file to compile

.PARAMETER Output
    Output file path (optional, used with -Bytecode flag)

.PARAMETER ShowAST
    Display the Abstract Syntax Tree

.PARAMETER ShowDetails
    Show detailed compilation information (default: true)

.PARAMETER Json
    Output AST as JSON format (for programmatic consumption)

.PARAMETER Bytecode
    Compile to standalone bytecode (.dsb binary format) instead of C array

.PARAMETER List
    List all applets currently in vm_applet_data.h

.PARAMETER Remove
    Remove an applet from vm_applet_data.h by name (e.g., "counter_applet")

.PARAMETER Clean
    Clear all applets from vm_applet_data.h and reset to empty header

.EXAMPLE
    .\build.ps1 scripts\counter_applet.ds
    Compile to C byte array and append to vm_applet_data.h (default behavior)

.EXAMPLE
    .\build.ps1 -List
    List all compiled applets in the header

.EXAMPLE
    .\build.ps1 -Remove counter_applet
    Remove the counter_applet from the header

.EXAMPLE
    .\build.ps1 -Clean
    Clear all applets from the header file

.EXAMPLE
    .\build.ps1 lsp\timer.ds -ShowAST
    Compile and display the AST

.EXAMPLE
    .\build.ps1 lsp\timer.ds -Json > output.json
    Export AST as JSON

.EXAMPLE
    .\build.ps1 lsp\timer.ds -Bytecode -Output build\timer.dsb
    Compile to standalone bytecode file

#>

param(
    [Parameter(Mandatory=$false, Position=0)]
    [string]$Source,
    
    [Parameter(Mandatory=$false)]
    [string]$Output = "",
    
    [switch]$ShowAST,
    [switch]$ShowDetails = $true,
    [switch]$Json,
    [switch]$Bytecode,
    [switch]$AppendToHeader,
    [switch]$List,
    [switch]$Clean,
    
    [Parameter(Mandatory=$false)]
    [string]$Remove = ""
)

# Script configuration
$ErrorActionPreference = "Stop"
$CompilerDir = Join-Path $PSScriptRoot "compiler"
$BuildDir = Join-Path $CompilerDir "build"
$ParserExe = Join-Path $BuildDir "Debug\parse_file.exe"
$BytecodeExe = Join-Path $BuildDir "Debug\compile.exe"
$CompilerExe = Join-Path $BuildDir "Debug\compile.exe"
$HeaderFile = Join-Path $PSScriptRoot "src\vm_applet_data.h"

# Banner
function Write-Banner {
    Write-Host "╔═══════════════════════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "║         dialOS - dialScript Compiler v1.0            ║" -ForegroundColor Cyan
    Write-Host "╚═══════════════════════════════════════════════════════╝" -ForegroundColor Cyan
    Write-Host ""
}

# Check if compiler exists
function Test-Compiler {
    $exeToCheck = if ($Bytecode) { $BytecodeExe } else { $ParserExe }
    
    if (-not (Test-Path $exeToCheck)) {
        Write-Host "❌ Compiler not found!" -ForegroundColor Red
        Write-Host ""
        Write-Host "Building compiler..." -ForegroundColor Yellow
        
        # Build the compiler
        Push-Location $BuildDir
        try {
            & cmake --build . 2>&1 | Out-Null
            if ($LASTEXITCODE -ne 0) {
                throw "Compiler build failed"
            }
            Write-Host "✓ Compiler built successfully" -ForegroundColor Green
            Write-Host ""
        }
        catch {
            Write-Host "❌ Failed to build compiler: $_" -ForegroundColor Red
            exit 1
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
        $RelPath = Join-Path $PSScriptRoot $Path
        if (Test-Path $RelPath) {
            return $RelPath
        }
        
        Write-Host "❌ Source file not found: $Path" -ForegroundColor Red
        exit 1
    }
    
    return (Resolve-Path $Path).Path
}

# Parse the source file
function Invoke-Parser {
    param([string]$SourcePath)
    
    if ($ShowDetails -and -not $Json) {
        Write-Host "Parsing: $SourcePath" -ForegroundColor Gray
        Write-Host ""
    }
    
    # Add --json flag if requested
    $args = @($SourcePath)
    if ($Json) {
        $args += "--json"
    }
    
    $output = & $ParserExe @args 2>&1 | Out-String
    
    # Check for parse errors
    if ($output -match "Parse errors: (\d+)") {
        $errorCount = [int]$matches[1]
        
        if ($errorCount -gt 0) {
            Write-Host "❌ Compilation failed with $errorCount error(s):" -ForegroundColor Red
            Write-Host ""
            
            # Extract and display errors
            $output -split "`n" | Where-Object { $_ -match "Line \d+:\d+ -" } | ForEach-Object {
                Write-Host "  $_" -ForegroundColor Red
            }
            
            exit 1
        }
    }
    
    return $output
}

# Display compilation results
function Show-CompilationResults {
    param([string]$Output, [string]$SourcePath)
    
    # Extract summary info
    if ($Output -match "Source length: (\d+)") {
        $sourceLength = $matches[1]
    }
    
    if ($Output -match "Top-level declarations: (\d+)") {
        $declarations = $matches[1]
    }
    
    Write-Host "✓ Compilation successful" -ForegroundColor Green
    Write-Host ""
    
    if ($ShowDetails) {
        Write-Host "Source file:  $(Split-Path $SourcePath -Leaf)" -ForegroundColor Gray
        Write-Host "Size:         $sourceLength bytes" -ForegroundColor Gray
        Write-Host "Declarations: $declarations" -ForegroundColor Gray
        Write-Host ""
    }
}

# Display AST
function Show-AST {
    param([string]$Output)
    
    Write-Host "═══ Abstract Syntax Tree ═══" -ForegroundColor Cyan
    Write-Host ""
    
    # Extract AST section
    $inAST = $false
    $Output -split "`n" | ForEach-Object {
        if ($_ -match "=== Abstract Syntax Tree ===") {
            $inAST = $true
            return
        }
        if ($_ -match "=== Summary ===") {
            $inAST = $false
            return
        }
        if ($inAST -and $_ -match "\S") {
            Write-Host $_
        }
    }
    
    Write-Host ""
}

# Generate bytecode (placeholder for future implementation)
function New-Bytecode {
    param([string]$SourcePath, [string]$OutputPath)
    
    if ($OutputPath -eq "") {
        $OutputPath = [System.IO.Path]::ChangeExtension($SourcePath, ".dsb")
    }
    
    if ($ShowDetails -and -not $Json) {
        Write-Host "Compiling to bytecode..." -ForegroundColor Gray
        Write-Host ""
    }
    
    # Run bytecode compiler
    $output = & $BytecodeExe $SourcePath 2>&1 | Out-String
    
    # Check for errors
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Bytecode compilation failed" -ForegroundColor Red
        Write-Host ""
        Write-Host $output
        exit 1
    }
    
    # Display output
    if ($ShowDetails -and -not $Json) {
        Write-Host $output
    }
    
    return $output
}

# Compile to C array and append to header
function New-CArrayApplet {
    param([string]$SourcePath)
    
    # Generate applet name from source file
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($SourcePath)
    $appletName = $baseName.ToUpper() -replace '[^A-Z0-9_]', '_'
    
    # Create temporary output file for C array
    $tempHeader = [System.IO.Path]::GetTempFileName()
    
    Write-Host "Compiling $baseName to C byte array..." -ForegroundColor Gray
    
    # Run compiler with --c-array flag
    $compilerArgs = @($SourcePath, $tempHeader, "--c-array")
    & $CompilerExe @compilerArgs 2>&1 | Out-Null
    
    # Check for errors
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ C array compilation failed" -ForegroundColor Red
        Remove-Item $tempHeader -ErrorAction SilentlyContinue
        exit 1
    }
    
    # Read the generated C array
    $cArrayContent = Get-Content $tempHeader -Raw
    Remove-Item $tempHeader
    
    # Check if header file exists
    if (-not (Test-Path $HeaderFile)) {
        # Create new header file
        $headerContent = @"
#pragma once

// Generated bytecode arrays from dialScript (.ds) files

$cArrayContent
"@
        Set-Content -Path $HeaderFile -Value $headerContent -NoNewline
        Write-Host "✓ Created new header: $HeaderFile" -ForegroundColor Green
    }
    else {
        # Check if this applet already exists in the header
        $existingContent = Get-Content $HeaderFile -Raw
        
        if ($existingContent -match "const unsigned char ${appletName}\[\]") {
            # Replace existing applet
            Write-Host "⚠ Applet $appletName already exists, replacing..." -ForegroundColor Yellow
            
            # Remove old applet definition (array + size)
            $pattern = "(?s)// Generated bytecode array.*?const unsigned char ${appletName}\[\].*?\};.*?const unsigned int ${appletName}_SIZE = \d+;"
            $existingContent = $existingContent -replace $pattern, ""
            
            # Clean up extra blank lines
            $existingContent = $existingContent -replace "(\r?\n){3,}", "`n`n"
            
            # Append new applet
            $newContent = $existingContent.TrimEnd() + "`n`n" + $cArrayContent
            Set-Content -Path $HeaderFile -Value $newContent -NoNewline
        }
        else {
            # Append new applet
            $newContent = $existingContent.TrimEnd() + "`n`n" + $cArrayContent
            Set-Content -Path $HeaderFile -Value $newContent -NoNewline
            Write-Host "✓ Appended $appletName to header" -ForegroundColor Green
        }
    }
    
    # Extract size from C array content
    if ($cArrayContent -match "const unsigned int ${appletName}_SIZE = (\d+);") {
        $size = $matches[1]
        Write-Host "✓ Compiled: $size bytes" -ForegroundColor Green
    }
    
    # Regenerate applet registry
    Update-AppletRegistry
    
    Write-Host "✓ Updated: $HeaderFile" -ForegroundColor Green
}

# Update the applet registry at the end of the header file
function Update-AppletRegistry {
    $existingContent = Get-Content $HeaderFile -Raw
    
    # Remove old registry if it exists (always do this first)
    $registryPattern = "(?s)// =+ Applet Registry =+.*?// =+ End Registry =+"
    $existingContent = $existingContent -replace $registryPattern, ""
    
    # Find all applet definitions in the header
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
    
    # Always generate registry code (even if empty) because main.cpp depends on it
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
    
    # Add registry array entries
    if ($applets.Count -eq 0) {
        # Empty registry - but still define the struct for main.cpp
        $registryCode += @"
static VMApplet APPLET_REGISTRY[] = {};

static const int APPLET_REGISTRY_SIZE = 0;
"@
    } else {
        # Generate registry with applet entries
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

# List all applets in the header
function Show-AppletList {
    if (-not (Test-Path $HeaderFile)) {
        Write-Host "No applets found. Header file does not exist." -ForegroundColor Yellow
        return
    }
    
    $existingContent = Get-Content $HeaderFile -Raw
    
    # Find all applet definitions
    $pattern = '// Generated bytecode array from .*[\\/](\w+)\.ds\s+// Total size: (\d+) bytes\s+const unsigned char (\w+)\[\]'
    $regexMatches = [regex]::Matches($existingContent, $pattern)
    
    if ($regexMatches.Count -eq 0) {
        Write-Host "No applets found in header." -ForegroundColor Yellow
        return
    }
    
    Write-Host "╔═══════════════════════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "║              Compiled Applets Registry               ║" -ForegroundColor Cyan
    Write-Host "╚═══════════════════════════════════════════════════════╝" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "  Name                Array Name          Size (bytes)" -ForegroundColor Gray
    Write-Host "  ──────────────────  ──────────────────  ────────────" -ForegroundColor Gray
    
    foreach ($match in $regexMatches) {
        $fileName = $match.Groups[1].Value
        $arrayName = $match.Groups[3].Value
        $size = $match.Groups[2].Value
        
        $displayName = $fileName.ToLower()
        Write-Host ("  {0,-18}  {1,-18}  {2,12}" -f $displayName, $arrayName, $size)
    }
    
    Write-Host ""
    Write-Host "Total: $($regexMatches.Count) applet(s)" -ForegroundColor Green
}

# Remove an applet from the header
function Remove-Applet {
    param([string]$AppletName)
    
    if (-not (Test-Path $HeaderFile)) {
        Write-Host "❌ Header file does not exist." -ForegroundColor Red
        exit 1
    }
    
    # Normalize applet name (convert to uppercase for array name)
    $arrayName = $AppletName.ToUpper() -replace '[^A-Z0-9_]', '_'
    
    $existingContent = Get-Content $HeaderFile -Raw
    
    # Check if applet exists
    if ($existingContent -notmatch "const unsigned char ${arrayName}\[\]") {
        Write-Host "❌ Applet '$AppletName' not found in header." -ForegroundColor Red
        Write-Host ""
        Write-Host "Available applets:" -ForegroundColor Yellow
        Show-AppletList
        exit 1
    }
    
    Write-Host "Removing applet: $AppletName ($arrayName)" -ForegroundColor Yellow
    
    # Remove applet definition - handle both CRLF and LF line endings
    # Use \r?\n to match both Windows (CRLF) and Unix (LF) line endings
    # Match the entire applet block: comment + array definition + size constant
    $pattern = "(?s)(\r?\n)?// Generated bytecode array[^\r\n]*\r?\n// Total size:[^\r\n]*\r?\n\r?\nconst unsigned char ${arrayName}\[\][^\}]*\};\r?\n\r?\nconst unsigned int ${arrayName}_SIZE = \d+;(\r?\n)+"
    $newContent = $existingContent -replace $pattern, "`n"
    
    # If nothing was removed, try matching from start of file (first applet case)
    if ($newContent -eq $existingContent) {
        $pattern = "(?s)^// Generated bytecode array[^\r\n]*\r?\n// Total size:[^\r\n]*\r?\n\r?\nconst unsigned char ${arrayName}\[\][^\}]*\};\r?\n\r?\nconst unsigned int ${arrayName}_SIZE = \d+;(\r?\n)+"
        $newContent = $existingContent -replace $pattern, ""
    }
    
    # Clean up multiple consecutive blank lines
    $newContent = $newContent -replace "(\r?\n){3,}", "`r`n`r`n"
    
    Set-Content -Path $HeaderFile -Value $newContent -NoNewline
    
    # Regenerate registry
    Update-AppletRegistry
    
    Write-Host "✓ Applet '$AppletName' removed successfully" -ForegroundColor Green
    Write-Host "✓ Registry updated" -ForegroundColor Green
}

# Clean all applets from the header
function Clear-AppletHeader {
    Write-Host "Clearing all applets from header..." -ForegroundColor Yellow
    
    $headerContent = @"
#pragma once

// Generated bytecode arrays from dialScript (.ds) files
"@
    
    Set-Content -Path $HeaderFile -Value $headerContent -NoNewline -Encoding UTF8
    
    # Generate empty registry structure (main.cpp depends on it)
    Update-AppletRegistry
    
    Write-Host "✓ Header file reset successfully" -ForegroundColor Green
    Write-Host "  File: $HeaderFile" -ForegroundColor Gray
}

# Main execution
try {
    
    # Check compiler
    Test-Compiler
    
    # List mode: show all applets
    if ($List) {
        Show-AppletList
        exit 0
    }
    
    # Clean mode: clear all applets
    if ($Clean) {
        Write-Banner
        Clear-AppletHeader
        exit 0
    }
    
    # Remove mode: remove an applet
    if ($Remove -ne "") {
        Write-Banner
        Remove-Applet -AppletName $Remove
        exit 0
    }
    
    # Validate source parameter for other modes
    if ([string]::IsNullOrEmpty($Source)) {
        Write-Host "❌ Error: Source file is required" -ForegroundColor Red
        Write-Host ""
        Write-Host "Usage:" -ForegroundColor Yellow
        Write-Host "  .\build.ps1 <source.ds>              Compile applet" -ForegroundColor Gray
        Write-Host "  .\build.ps1 -List                    List all applets" -ForegroundColor Gray
        Write-Host "  .\build.ps1 -Remove <name>           Remove applet" -ForegroundColor Gray
        exit 1
    }
    
    # JSON mode: minimal output, just parse and emit JSON
    if ($Json) {
        # Check compiler silently
        if (-not (Test-Path $ParserExe)) {
            Push-Location $BuildDir
            try {
                & cmake --build . 2>&1 | Out-Null
                if ($LASTEXITCODE -ne 0) {
                    throw "Compiler build failed"
                }
            }
            finally {
                Pop-Location
            }
        }
        
        # Resolve and parse
        $SourcePath = Resolve-SourcePath -Path $Source
        $output = Invoke-Parser -SourcePath $SourcePath
        
        # Output JSON directly
        Write-Output $output
        exit 0
    }
    
    # Normal mode: full UI
    Write-Banner
    
    # Resolve source path
    $SourcePath = Resolve-SourcePath -Path $Source
    
    if ($ShowDetails) {
        Write-Host "Source: $SourcePath" -ForegroundColor Gray
        Write-Host ""
    }
    
    # Default behavior: compile to C array and append to header
    if (-not $Bytecode -and -not $ShowAST -and $Output -eq "") {
        New-CArrayApplet -SourcePath $SourcePath
        Write-Host "✓ Done" -ForegroundColor Green
        exit 0
    }
    
    # Bytecode mode: compile to bytecode
    if ($Bytecode) {
        New-Bytecode -SourcePath $SourcePath -OutputPath $Output | Out-Null
        Write-Host "✓ Done" -ForegroundColor Green
        exit 0
    }
    
    # Parse source file
    $parserOutput = Invoke-Parser -SourcePath $SourcePath
    
    # Show results
    Show-CompilationResults -Output $parserOutput -SourcePath $SourcePath
    
    # Show AST if requested
    if ($ShowAST) {
        Show-AST -Output $parserOutput
    }
    
    # Generate bytecode (future)
    if ($Output -ne "") {
        New-Bytecode -SourcePath $SourcePath -OutputPath $Output
    }
    
    Write-Host "✓ Done" -ForegroundColor Green
    
    exit 0
}
catch {
    Write-Host ""
    Write-Host "❌ Compilation error: $_" -ForegroundColor Red
    exit 1
}
