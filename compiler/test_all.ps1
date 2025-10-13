# Test all dialScript files
Write-Host "=== Testing dialScript Parser ===" -ForegroundColor Cyan
Write-Host ""

$files = @(
    "j:\workspace2\arduino\dialOS\lsp\timer.ds",
    "j:\workspace2\arduino\dialOS\lsp\test.ds",
    "j:\workspace2\arduino\dialOS\lsp\test_expressions.ds"
)

$totalErrors = 0

foreach ($file in $files) {
    $filename = Split-Path $file -Leaf
    Write-Host "Testing $filename..." -NoNewline
    
    $output = & .\Debug\parse_file.exe $file 2>&1 | Out-String
    
    if ($output -match "Parse errors: (\d+)") {
        $errors = [int]$matches[1]
        $totalErrors += $errors
        
        if ($errors -eq 0) {
            Write-Host " ✓ PASS" -ForegroundColor Green
        } else {
            Write-Host " ✗ FAIL ($errors errors)" -ForegroundColor Red
        }
    } else {
        Write-Host " ? UNKNOWN" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "Total errors: $totalErrors" -ForegroundColor $(if ($totalErrors -eq 0) { "Green" } else { "Red" })

if ($totalErrors -eq 0) {
    Write-Host ""
    Write-Host "🎉 All tests passed!" -ForegroundColor Green
}
