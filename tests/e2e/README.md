# E2E / Cross-Runtime Conformance Tests

Deterministic end-to-end tests: dialScript source → bytecode → VM execution →
console output, verified byte-for-byte against expected files. The **same
scripts and expected files** run on both the C++ VM (reference) and the .NET
runtime — any behavioral divergence between runtimes fails CI.

## Layout

```
tests/e2e/
  scripts/   <name>.ds    dialScript source (language features + os.console only)
  scripts/   <name>.dsb   compiled bytecode (checked in — the .NET side has no compiler)
  expected/  <name>.expected  golden console output
```

## Rules for scripts

- Only **language features** and `os.console.println/print` — no hardware, no
  time, no files. Outputs must be deterministic on every host.
- Avoid printing floats at extremes (e.g. `1e7`) — C++ and .NET float
  formatting diverges for exponent notation.
- Avoid printing objects/arrays directly — their `toString()` differs between
  runtimes; print fields/elements instead.

## Workflow when adding/changing a script

1. Edit or add `scripts/<name>.ds`.
2. Recompile the bytecode (the .NET tests consume the checked-in `.dsb`):
   ```
   build/compile tests/e2e/scripts/<name>.ds tests/e2e/scripts/<name>.dsb
   ```
3. Generate and **review** the golden output:
   ```
   build/e2e_runner tests/e2e/scripts/<name>.ds /tmp/none.expected   # prints actual output
   # save verified output to tests/e2e/expected/<name>.expected
   ```
4. Run both suites:
   ```
   ctest --test-dir build            # C++ (10 e2e_* tests + parser_test)
   dotnet test dotnet/DialOS.sln     # .NET (E2ETests theory per script)
   ```
5. Commit the `.ds`, `.dsb`, and `.expected` together.

## Runners

- **C++:** `compiler/e2e_runner.cpp` → `e2e_runner` binary.
  Single: `e2e_runner <script.ds> <expected>`. All: `e2e_runner --all <scriptsDir> <expectedDir>`.
  Registered in ctest as one test per script (`ctest -R e2e_`).
- **.NET:** `dotnet/DialOS.Tests/E2ETests.cs` — xUnit Theory, one case per script,
  loads the checked-in `.dsb` and compares against the same expected files.
