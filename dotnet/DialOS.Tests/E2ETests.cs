using DialOS.Runtime.Bytecode;
using DialOS.Runtime.Platform;
using DialOS.Runtime.Values;
using DialOS.Runtime.VM;
using System.Text;

namespace DialOS.Tests;

/// <summary>
/// Cross-runtime E2E conformance suite (C++ leg: compiler/e2e_runner.cpp + ctest).
///
/// Each tests/e2e/scripts/&lt;name&gt;.ds is compiled to &lt;name&gt;.dsb with the C++ compiler
/// (checked in; regenerate with: compile tests/e2e/scripts/&lt;name&gt;.ds &lt;name&gt;.dsb),
/// executed on the .NET VM, and its console output is compared byte-for-byte
/// (modulo CRLF/trailing whitespace) with tests/e2e/expected/&lt;name&gt;.expected.
///
/// If any script's output ever diverges between the C++ VM and this runtime,
/// these tests fail — this is the guard against cross-runtime drift.
/// </summary>
public class E2ETests
{
    private static string GetRepoRoot()
    {
        // Navigate from bin/Debug/net8.0 to repo root
        var testDir = AppDomain.CurrentDomain.BaseDirectory;
        return Path.GetFullPath(Path.Combine(testDir, "..", "..", "..", "..", ".."));
    }

    private static string GetScriptsDir() => Path.Combine(GetRepoRoot(), "tests", "e2e", "scripts");
    private static string GetExpectedDir() => Path.Combine(GetRepoRoot(), "tests", "e2e", "expected");

    public static IEnumerable<object[]> ScriptNames()
    {
        var dir = GetScriptsDir();
        if (!Directory.Exists(dir)) return Enumerable.Empty<object[]>();
        return Directory.GetFiles(dir, "*.ds")
            .Select(Path.GetFileNameWithoutExtension)
            .OrderBy(n => n)
            .Select(n => new object[] { n });
    }

    private static string Normalize(string s)
    {
        var sb = new StringBuilder(s.Length);
        foreach (var c in s)
        {
            if (c != '\r') sb.Append(c);
        }
        return sb.ToString().TrimEnd() + "\n";
    }

    [Theory]
    [MemberData(nameof(ScriptNames))]
    public void E2E_Script_OutputMatchesExpected(string name)
    {
        var dsbPath = Path.Combine(GetScriptsDir(), name + ".dsb");
        var expectedPath = Path.Combine(GetExpectedDir(), name + ".expected");

        Assert.True(File.Exists(dsbPath),
            $"Missing compiled bytecode '{name}.dsb' — regenerate with: compile {name}.ds {name}.dsb");
        Assert.True(File.Exists(expectedPath), $"Missing expected output for '{name}'");

        var module = BytecodeModule.Load(dsbPath, verifyIntegrity: false);
        var output = new StringBuilder();
        var platform = new E2ECapturePlatform(output);
        var pool = new ValuePool(module.Metadata.HeapSize);
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        // Run until finished (bounded)
        var result = VMResult.Ok;
        for (int i = 0; i < 1000 && result == VMResult.Ok; i++)
        {
            result = engine.Execute(100);
        }

        Assert.True(result == VMResult.Finished,
            $"VM did not finish for '{name}': {result}, error: {state.LastError}");

        var expected = Normalize(File.ReadAllText(expectedPath));
        var actual = Normalize(output.ToString());
        Assert.True(expected == actual,
            $"Output mismatch for '{name}'.\n--- expected ---\n{expected}\n--- actual ---\n{actual}");
    }

    /// <summary>Captures os.console output; everything else inert.</summary>
    private class E2ECapturePlatform : PlatformBase
    {
        private readonly StringBuilder _output;
        public E2ECapturePlatform(StringBuilder output) { _output = output; }
        public override void ConsolePrint(string message) { _output.Append(message); }
        public override void ConsoleLog(string message) { } // diagnostics: not part of expected output
        public override void ConsoleWarn(string message) { }
        public override uint SystemGetTime() => 0;
        public override void SystemSleep(uint ms) { }
    }
}
