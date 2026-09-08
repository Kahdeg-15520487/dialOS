/**
 * E2E Test Runner - compile + execute + verify console output
 *
 * Usage:
 *   e2e_runner <script.ds> <expected.txt>        Single test: exit 0 if output matches
 *   e2e_runner --all <scriptsDir> <expectedDir>  Run every .ds script, print summary
 *
 * The runner compiles dialScript source in-process (same path as `compile`),
 * executes the bytecode on the real VM, captures os.console output, and diffs
 * it against the expected file. Deterministic by design: scripts only use
 * language features + os.console (no hardware, no time).
 *
 * This is the C++ leg of the cross-runtime conformance suite: the same scripts
 * and expected files are executed by the .NET runtime (DialOS.Tests E2ETests).
 */

#include "lexer.h"
#include "parser.h"
#include "bytecode_compiler.h"
#include "vm/vm_core.h"
#include "vm/platform.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <dirent.h>

using namespace dialos;

// Platform that captures os.console.* output into a string.
// Only the pure virtuals are overridden; everything else uses the defaults.
class CapturingPlatform : public vm::PlatformInterface {
public:
    // Console
    void console_print(const std::string& msg) override { out_ += msg; }
    void console_println(const std::string& msg) override { out_ += msg; out_ += '\n'; }
    void console_log(const std::string&) override {}   // diagnostics: not part of expected output
    void console_warn(const std::string&) override {}
    void console_error(const std::string& msg) override { err_ += msg + "\n"; }

    // Display
    void display_clear(uint32_t) override {}
    void display_drawText(int, int, const std::string&, uint32_t, int) override {}
    void display_drawRect(int, int, int, int, uint32_t, bool) override {}
    void display_drawCircle(int, int, int, uint32_t, bool) override {}
    void display_drawLine(int, int, int, int, uint32_t) override {}
    void display_drawPixel(int, int, uint32_t) override {}
    void display_setBrightness(int) override {}
    int display_getWidth() override { return 240; }
    int display_getHeight() override { return 240; }

    // Encoder / system: fixed neutral state => deterministic
    bool encoder_getButton() override { return false; }
    int encoder_getDelta() override { return 0; }
    uint32_t system_getTime() override { return 0; }
    void system_sleep(uint32_t) override {}

    const std::string& output() const { return out_; }
    const std::string& errors() const { return err_; }

private:
    std::string out_;
    std::string err_;
};

static std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Normalize: CRLF -> LF, strip trailing whitespace, ensure single trailing newline
static std::string normalize(const std::string& s) {
    std::string r = s;
    r.erase(std::remove(r.begin(), r.end(), '\r'), r.end());
    while (!r.empty() && (r.back() == '\n' || r.back() == ' ' || r.back() == '\t')) r.pop_back();
    r += '\n';
    return r;
}

static int runOne(const std::string& scriptPath, const std::string& expectedPath) {
    std::string source = readFile(scriptPath);
    if (source.empty()) {
        std::cerr << "FAIL: cannot read script '" << scriptPath << "'" << std::endl;
        return 1;
    }

    // Compile (same path as the `compile` CLI)
    compiler::Lexer lexer(source);
    compiler::Parser parser(lexer);
    auto program = parser.parse();
    if (parser.hasErrors()) {
        std::cerr << "FAIL: parse errors in " << scriptPath << ":" << std::endl;
        for (const auto& e : parser.getErrors()) std::cerr << "  " << e << std::endl;
        return 1;
    }

    compiler::BytecodeCompiler bc;
    compiler::BytecodeModule module = bc.compile(*program);
    if (bc.hasErrors()) {
        std::cerr << "FAIL: compile errors in " << scriptPath << ":" << std::endl;
        for (const auto& e : bc.getErrors()) std::cerr << "  " << e << std::endl;
        return 1;
    }

    // Execute
    CapturingPlatform platform;
    vm::ValuePool pool(16384);
    vm::VMState vm(module, pool, platform);
    vm.reset();
    vm::VMResult result = vm.execute(1000000);

    if (result != vm::VMResult::FINISHED) {
        std::cerr << "FAIL: VM did not finish (" << scriptPath << "): result="
                  << static_cast<int>(result);
        if (vm.hasError()) std::cerr << " error: " << vm.getError();
        std::cerr << std::endl;
        return 1;
    }

    // Compare
    std::string expected = normalize(readFile(expectedPath));
    std::string actual = normalize(platform.output());
    if (expected != actual) {
        std::cerr << "FAIL: output mismatch for " << scriptPath << std::endl;
        std::cerr << "--- expected ---" << std::endl << expected;
        std::cerr << "--- actual ---" << std::endl << actual;
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc == 4 && std::string(argv[1]) == "--all") {
        std::string scriptsDir = argv[2], expectedDir = argv[3];
        DIR* d = opendir(scriptsDir.c_str());
        if (!d) {
            std::cerr << "Cannot open scripts dir: " << scriptsDir << std::endl;
            return 2;
        }
        int pass = 0, fail = 0;
        std::vector<std::string> names;
        while (dirent* e = readdir(d)) {
            std::string n = e->d_name;
            if (n.size() > 3 && n.substr(n.size() - 3) == ".ds") names.push_back(n);
        }
        closedir(d);
        std::sort(names.begin(), names.end());
        for (const auto& n : names) {
            std::string base = n.substr(0, n.size() - 3);
            std::cout << "[RUN ] " << n << std::flush;
            int rc = runOne(scriptsDir + "/" + n, expectedDir + "/" + base + ".expected");
            if (rc == 0) { std::cout << " PASS" << std::endl; pass++; }
            else { std::cout << std::endl; fail++; }
        }
        std::cout << "=== E2E: " << pass << " passed, " << fail << " failed ===" << std::endl;
        return fail == 0 ? 0 : 1;
    }

    if (argc != 3) {
        std::cerr << "Usage: e2e_runner <script.ds> <expected.txt>" << std::endl
                  << "       e2e_runner --all <scriptsDir> <expectedDir>" << std::endl;
        return 2;
    }
    return runOne(argv[1], argv[2]);
}
