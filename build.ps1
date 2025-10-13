#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Build script for dialScript (.ds) files

.DESCRIPTION
    Compiles dialScript source files using the dialOS compiler.
    Supports parsing, AST generation, JSON export, and bytecode compilation.

.PARAMETER Source
    Path to the .ds source file to compile

.PARAMETER Output
    Output file path (optional, defaults to output.dsb for bytecode)

.PARAMETER ShowAST
    Display the Abstract Syntax Tree

.PARAMETER ShowDetails
    Show detailed compilation information (default: true)

.PARAMETER Json
    Output AST as JSON format (for programmatic consumption)

.PARAMETER Bytecode
    Compile to bytecode (.dsb binary format) for dialOS VM

.EXAMPLE
    .\build.ps1 lsp\timer.ds
    Compile timer.ds and show results

.EXAMPLE
    .\build.ps1 lsp\timer.ds -ShowAST
    Compile and display the AST

.EXAMPLE
    .\build.ps1 lsp\timer.ds -Json > output.json
    Export AST as JSON

.EXAMPLE
    .\build.ps1 lsp\timer.ds -Output build\timer.dsb -ShowDetails
    Compile to specific output file with details

.EXAMPLE
    .\build.ps1 lsp\timer.ds -Bytecode
    Compile to bytecode (.dsb file)
#>

param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$Source,
    
    [Parameter(Mandatory=$false)]
    [string]$Output = "",
    
    [switch]$ShowAST,
    [switch]$ShowDetails = $true,
    [switch]$Json,
    [switch]$Bytecode
)

# Script configuration
$ErrorActionPreference = "Stop"
$CompilerDir = Join-Path $PSScriptRoot "compiler"
$BuildDir = Join-Path $CompilerDir "build"
$ParserExe = Join-Path $BuildDir "Debug\parse_file.exe"
$BytecodeExe = Join-Path $BuildDir "Debug\compile.exe"

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

# Main execution
try {
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
    
    # Check compiler
    Test-Compiler
    
    # Resolve source path
    $SourcePath = Resolve-SourcePath -Path $Source
    
    if ($ShowDetails) {
        Write-Host "Source: $SourcePath" -ForegroundColor Gray
        Write-Host ""
    }
    
    # Bytecode mode: compile to bytecode
    if ($Bytecode) {
        $bytecodeOutput = New-Bytecode -SourcePath $SourcePath -OutputPath $Output
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
