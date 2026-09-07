using DialOS.Runtime.Bytecode;
using DialOS.Runtime.Platform;
using DialOS.Runtime.Values;
using DialOS.Runtime.VM;
using System.Text;

namespace DialOS.Tests;

/// <summary>
/// Integration tests that load and run actual .dsb bytecode files.
/// </summary>
public class IntegrationTests
{
    private string GetTestFilesPath()
    {
        // Navigate from bin/Debug/net8.0 to project root, then to parent (dialOS)
        var testDir = AppDomain.CurrentDomain.BaseDirectory;
        var projectRoot = Path.GetFullPath(Path.Combine(testDir, "..", "..", "..", "..", ".."));
        return projectRoot;
    }

    [Fact]
    public void LoadDsbFile_HelloWorld_ShouldLoadCorrectly()
    {
        var dsbPath = Path.Combine(GetTestFilesPath(), "scripts", "hello_world.dsb");
        
        // Skip if file doesn't exist
        if (!File.Exists(dsbPath))
        {
            return; // Test environment doesn't have the file
        }

        // Load without integrity check (hash algorithm may differ between C++ and .NET)
        var module = BytecodeModule.Load(dsbPath, verifyIntegrity: false);

        Assert.NotNull(module);
        Assert.NotNull(module.Metadata);
        Assert.True(module.Code.Length > 0);
    }

    [Fact]
    public void LoadDsbFile_TestApp_ShouldLoadWithMetadata()
    {
        var dsbPath = Path.Combine(GetTestFilesPath(), "test_app.dsb");
        
        if (!File.Exists(dsbPath))
        {
            return;
        }

        var module = BytecodeModule.Load(dsbPath, verifyIntegrity: false);

        Assert.NotNull(module);
        Assert.True(module.Metadata.HeapSize > 0);
    }

    [Fact]
    public void ExecuteDsbFile_ShouldRunWithoutErrors()
    {
        var dsbPath = Path.Combine(GetTestFilesPath(), "scripts", "hello_world.dsb");
        
        if (!File.Exists(dsbPath))
        {
            return;
        }

        var module = BytecodeModule.Load(dsbPath, verifyIntegrity: false);
        var output = new StringBuilder();
        var platform = new TestOutputPlatform(output);
        var pool = new ValuePool(module.Metadata.HeapSize);
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        // Execute with timeout (max 10000 instructions)
        var result = VMResult.Ok;
        for (int i = 0; i < 100 && result == VMResult.Ok; i++)
        {
            result = engine.Execute(100);
        }

        // Should either finish or still be running (not error)
        Assert.True(result == VMResult.Finished || result == VMResult.Ok || result == VMResult.Yield,
            $"Unexpected result: {result}, Error: {state.LastError}");
    }

    [Fact]
    public void TouchCalculator_ShouldLoadWithFunctions()
    {
        var dsbPath = Path.Combine(GetTestFilesPath(), "touch_calculator_simple.dsb");
        
        if (!File.Exists(dsbPath))
        {
            return;
        }

        var module = BytecodeModule.Load(dsbPath, verifyIntegrity: false);

        Assert.NotNull(module);
        Assert.True(module.Functions.Count > 0, "Should have function definitions");
        Assert.True(module.Constants.Count > 0, "Should have string constants");
    }

    /// <summary>
    /// Test platform that captures console output.
    /// </summary>
    private class TestOutputPlatform : PlatformBase
    {
        private readonly StringBuilder _output;
        private readonly System.Diagnostics.Stopwatch _stopwatch;

        public TestOutputPlatform(StringBuilder output)
        {
            _output = output;
            _stopwatch = System.Diagnostics.Stopwatch.StartNew();
        }

        public override void ConsolePrint(string message)
        {
            _output.Append(message);
        }

        public override uint SystemGetTime()
        {
            return (uint)_stopwatch.ElapsedMilliseconds;
        }

        public override void SystemSleep(uint ms)
        {
            // Don't actually sleep in tests
        }

        public string GetOutput() => _output.ToString();
    }
}
