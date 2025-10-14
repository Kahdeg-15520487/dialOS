@echo off
echo === dialOS SDL Emulator Setup (Windows) ===
echo.

REM Check if vcpkg is available
where vcpkg >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: vcpkg not found in PATH
    echo.
    echo Please install vcpkg first:
    echo 1. git clone https://github.com/Microsoft/vcpkg.git
    echo 2. cd vcpkg
    echo 3. .\bootstrap-vcpkg.bat
    echo 4. Add vcpkg directory to your PATH
    echo.
    pause
    exit /b 1
)

echo Installing SDL2 dependencies via vcpkg...
echo.

REM Install SDL2 packages
vcpkg install sdl2:x64-windows
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to install SDL2
    pause
    exit /b 1
)

vcpkg install sdl2-ttf:x64-windows
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to install SDL2-TTF
    pause
    exit /b 1
)

vcpkg install sdl2-mixer:x64-windows
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to install SDL2-Mixer
    pause
    exit /b 1
)

echo.
echo === SDL2 Installation Complete ===
echo.
echo Now you can build the emulator:
echo   cd build
echo   cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake
echo   cmake --build . --config Debug
echo.
echo Or use the provided build script:
echo   build.ps1
echo.
pause