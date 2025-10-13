cmake_minimum_required(VERSION 3.10)
project(dialscript_parser VERSION 1.0 LANGUAGES CXX)

# Require C++11
set(CMAKE_CXX_STANDARD 11)
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
    bytecode.cpp
    bytecode_compiler.cpp
)

# VM source files
set(VM_SOURCES
    ../src/vm/vm_value.cpp
    ../src/vm/vm_core.cpp
)

# Create a library for the parser (can be linked to other projects)
add_library(dialscript_parser STATIC ${PARSER_SOURCES})

# Create a library for the VM
add_library(dialscript_vm STATIC ${VM_SOURCES})
target_include_directories(dialscript_vm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dialscript_vm dialscript_parser)

# Include directory for the library
target_include_directories(dialscript_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

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
add_executable(test_vm ../src/vm/test_vm.cpp)
target_link_libraries(test_vm dialscript_vm dialscript_parser)

# Installation rules (optional)
install(TARGETS dialscript_parser dialscript_vm test_parser parse_file compile test_vm
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
