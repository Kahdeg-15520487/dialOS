# Prepare log directory
New-Item -Path C:\temp -ItemType Directory -Force | Out-Null

# Paths (adjust if your build config differs)
$cdb = 'C:\Program Files\WindowsApps\Microsoft.WinDbg_1.2506.12002.0_x64__8wekyb3d8bbwe\amd64\cdb.exe'
$exe = 'C:\Users\mnguyen\Documents\workspace\dialOS\compiler\build\Debug\test_sdl_emulator.exe'
$log = 'C:\Users\mnguyen\Documents\workspace\dialOS\temp\test_sdl_crash_log.txt'
$cmds = 'C:\Users\mnguyen\Documents\workspace\dialOS\temp\cdb_cmds.txt'

# Create a small command script for cdb to run after it breaks
@'
.symfix
.reload /f
// continue until exception or crash
g
!analyze -v
kv
lm
.logclose
.q
'@ | Out-File -FilePath $cmds -Encoding ASCII

# Run cdb: -o (output debug events to console), -G (do not auto-exit on initial load), -cf to run commands from file
# We will redirect stdout/stderr to the log file
& "$cdb" -o -G -cf "$cmds" "$exe" *>&1 | Tee-Object -FilePath $log