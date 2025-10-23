cmake_minimum_required(VERSION 3.10)
project(dialscript_parser VERSION 1.0 LANGUAGES CXX)

# Require C++17 for std::filesystem
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Source files
set(PARSER_SOURCES
    lexer.cpp
    ast.cpp
    parser.cpp
    ast_printer.cpp
    ast_json.cpp
    ../src/vm/bytecode.cpp
    bytecode_compiler.cpp
)

# VM source files
set(VM_SOURCES
    ../src/vm/vm_value.cpp
    ../src/vm/vm_core.cpp
    ../src/vm/platform.cpp
)

# Create a library for the parser (can be linked to other projects)
add_library(dialscript_parser STATIC ${PARSER_SOURCES})

# Create a library for the VM
add_library(dialscript_vm STATIC ${VM_SOURCES})
target_include_directories(dialscript_vm PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_link_libraries(dialscript_vm dialscript_parser)

# Include directory for the library
target_include_directories(dialscript_parser PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Test executable
add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser dialscript_parser)

# File parser executable
add_executable(parse_file parse_file.cpp)
target_link_libraries(parse_file dialscript_parser)

# Bytecode compiler executable
add_executable(compile compile.cpp)
target_link_libraries(compile dialscript_parser)

# VM test executable
add_executable(test_vm test_vm.cpp)
target_link_libraries(test_vm dialscript_vm dialscript_parser)

# SDL Emulator executable (optional, requires SDL2)
# Try vcpkg first (Windows), then pkg-config (Linux/macOS), then find_package
set(SDL2_FOUND FALSE)

# Method 1: vcpkg (Windows with vcpkg)
find_package(SDL2 QUIET)
find_package(SDL2_ttf QUIET)  
find_package(SDL2_mixer QUIET)

if(SDL2_FOUND AND TARGET SDL2::SDL2)
    if(TARGET SDL2_ttf::SDL2_ttf AND TARGET SDL2_mixer::SDL2_mixer)
        add_executable(test_sdl_emulator test_sdl_emulator.cpp sdl_platform.cpp)
        target_link_libraries(test_sdl_emulator 
            dialscript_vm 
            dialscript_parser 
            SDL2::SDL2
            SDL2::SDL2main
            SDL2_ttf::SDL2_ttf 
            SDL2_mixer::SDL2_mixer
        )
        message(STATUS "SDL2 emulator will be built (vcpkg)")
        set(SDL2_FOUND TRUE)
    endif()
endif()

# Method 2: pkg-config (Linux/macOS)
if(NOT SDL2_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2_PC QUIET sdl2)
        pkg_check_modules(SDL2_TTF_PC QUIET SDL2_ttf)
        pkg_check_modules(SDL2_MIXER_PC QUIET SDL2_mixer)
        
        if(SDL2_PC_FOUND AND SDL2_TTF_PC_FOUND AND SDL2_MIXER_PC_FOUND)
            add_executable(test_sdl_emulator test_sdl_emulator.cpp sdl_platform.cpp)
            target_link_libraries(test_sdl_emulator 
                dialscript_vm 
                dialscript_parser 
                ${SDL2_PC_LIBRARIES} 
                ${SDL2_TTF_PC_LIBRARIES} 
                ${SDL2_MIXER_PC_LIBRARIES}
            )
            target_include_directories(test_sdl_emulator PRIVATE 
                ${SDL2_PC_INCLUDE_DIRS} 
                ${SDL2_TTF_PC_INCLUDE_DIRS} 
                ${SDL2_MIXER_PC_INCLUDE_DIRS}
            )
            target_compile_definitions(test_sdl_emulator PRIVATE 
                ${SDL2_PC_CFLAGS_OTHER} 
                ${SDL2_TTF_PC_CFLAGS_OTHER} 
                ${SDL2_MIXER_PC_CFLAGS_OTHER}
            )
            message(STATUS "SDL2 emulator will be built (pkg-config)")
            set(SDL2_FOUND TRUE)
        endif()
    endif()
endif()

# Show help message if SDL2 not found
if(NOT SDL2_FOUND)
    message(STATUS "SDL2 libraries not found, emulator will not be built")
    message(STATUS "To build emulator:")
    message(STATUS "  Windows (vcpkg): vcpkg install sdl2 sdl2-ttf sdl2-mixer")
    message(STATUS "  Ubuntu/Debian:   sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev")
    message(STATUS "  macOS (Homebrew): brew install sdl2 sdl2_ttf sdl2_mixer")
    message(STATUS "  Then run: cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake")
endif()

# Installation rules (optional)
# Create list of targets to install
set(INSTALL_TARGETS dialscript_parser dialscript_vm test_parser parse_file compile test_vm)

# Add emulator to install list if it was built
if(TARGET test_sdl_emulator)
    list(APPEND INSTALL_TARGETS test_sdl_emulator)
endif()

install(TARGETS ${INSTALL_TARGETS}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES
    lexer.h
    ast.h
    parser.h
    ast_printer.h
    ast_json.h
    bytecode.h
    bytecode_compiler.h
    DESTINATION include/dialscript
)

# Enable testing
enable_testing()
add_test(NAME parser_test COMMAND test_parser)

# Print configuration
message(STATUS "dialScript Parser Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
