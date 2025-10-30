// Minimal C API wrapper for WebAssembly builds.
// Exposes compile_source(const char* src, int* out_len) -> unsigned char* (malloced)
// and free_compiled_buffer(unsigned char* p).

#include "lexer.h"
#include "parser.h"
#include "bytecode_compiler.h"
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>

using namespace dialos::compiler;

extern "C" {

// Compile a source string and return a malloc'd buffer and length via out_len.
// Returns nullptr and sets *out_len = 0 on error.
unsigned char* compile_source(const char* src, int* out_len) {
    if (!out_len) return nullptr;
    *out_len = 0;
    std::string source = src ? src : std::string();

    try {
        Lexer lexer(source);
        Parser parser(lexer);
        auto program = parser.parse();
        if (parser.hasErrors()) {
            return nullptr;
        }

        BytecodeCompiler compiler;
        BytecodeModule module = compiler.compile(*program);
        if (compiler.hasErrors()) {
            return nullptr;
        }

        std::vector<uint8_t> bytes = module.serialize();
        if (bytes.empty()) {
            return nullptr;
        }

        unsigned char* buf = (unsigned char*)malloc(bytes.size());
        if (!buf) {
            *out_len = 0;
            return nullptr;
        }
        memcpy(buf, bytes.data(), bytes.size());
        *out_len = static_cast<int>(bytes.size());
        return buf;
    } catch (...) {
        *out_len = 0;
        return nullptr;
    }
}

void free_compiled_buffer(unsigned char* p) {
    if (p) free(p);
}

} // extern "C"
