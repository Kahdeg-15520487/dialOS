Short answer: yes — the SDL platform already implements the app-management APIs; the ESP32 platform does not (it still uses the default "Not supported" stubs).

Details I found

Where the VM calls the platform API:
vm_core.cpp — calls platform_.app_install(...), platform_.app_uninstall(...), platform_.app_list(), platform_.app_getMetadata(...), platform_.app_launch(...), platform_.app_validate(...) (around lines ~2134–2206).
Default interface (fallback stubs):
platform.h — declares the NativeFunctionID enum for APP_INSTALL/APP_UNINSTALL/APP_LIST/APP_GETMETADATA/APP_LAUNCH/APP_VALIDATE and provides default virtual methods that return "Not supported" or empty JSON (lines ~796–801).
SDL implementation (complete):
sdl_platform.h — declares the overrides for all app-management methods (app_install, app_uninstall, app_list, app_getMetadata, app_launch, app_validate).
sdl_platform.cpp — implements all of them:
app_install: copies .dsb into apps/installed, updates apps/registry.json and returns JSON status (around lines ~2332+).
app_uninstall: removes installed .dsb and updates registry.json (around ~2429+).
app_list: returns contents of registry.json (around ~2505+).
app_getMetadata: reads .dsb and uses compiler::BytecodeModule::deserialize(...) to extract metadata (around ~2533+).
app_launch: verifies the app exists and returns success+message (VM restart required) (around ~2583+).
app_validate: deserializes/validates the DSB file and returns JSON result (around ~2612+).
ESP32 implementation: not present
esp32_platform.h / esp32_platform.cpp — many platform methods are implemented (display, I/O, file via RAMFS, etc.) but there are no overrides for app_install / app_uninstall / app_list / app_getMetadata / app_launch / app_validate in esp32_platform.*. So the code running on the real device will fall back to the default "Not supported" implementations in include/vm/platform.h.
Recommended next steps (pick what you want me to do)

If you want app-management on the device: I can implement the ESP32 equivalents (use RAMFS / kernel APIs already present in esp32_platform.cpp — e.g., use ramfs and kernel services to install/validate/list apps) and add tests.
If you want the web UI to exercise app-management via the emulator: the SDL implementation is ready; we can wire the appstore UI to call the VM native functions (or the platform API via your emulator harness).
If you want, I can:
Add a short README snippet describing where SDL stores apps (sdl_filesystem/apps/installed + registry.json) and how to test install/list/validate with the emulator.
Implement ESP32 versions (I can propose the minimal safe behavior to match SDL and avoid going too deep into kernel changes).
Which of the next steps would you like me to take? I can open the SDL implementation file and paste the small sample JSON responses or start implementing the ESP32 methods (I'll follow the project's RAMFS/Kernel patterns).