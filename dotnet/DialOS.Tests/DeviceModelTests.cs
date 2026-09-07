using System.Diagnostics;
using DialOS.Runtime.Bytecode;
using DialOS.Runtime.Platform;
using DialOS.Runtime.Values;
using DialOS.Runtime.VM;

namespace DialOS.Tests;

/// <summary>
/// Tests for SPI (0x15xx) and Device (0x16xx) native function dispatch.
/// Note: CALL_NATIVE encodes nativeId as little-endian u16: [lo, hi, argCount].
/// </summary>
public class DeviceModelTests
{
    private static (VMState state, ExecutionEngine engine) CreateVM(
        byte[] code, List<string>? constants = null, IPlatform? platform = null)
    {
        var module = new BytecodeModule
        {
            Code = code,
            Metadata = new Metadata { HeapSize = 8192 }
        };
        if (constants != null) module.Constants.AddRange(constants);

        var p = platform ?? new ConsolePlatform();
        var pool = new ValuePool(8192);
        var state = new VMState(module, pool, p);
        return (state, new ExecutionEngine(state));
    }

    private static Value RunAndPop(byte[] code, List<string>? constants = null, IPlatform? platform = null)
    {
        var (state, engine) = CreateVM(code, constants, platform);
        var result = engine.Execute(1000);
        Assert.True(result == VMResult.Finished, $"VM did not finish: {result}, error: {state.LastError}");
        return state.Pop();
    }

    [Fact]
    public void DeviceList_DefaultPlatform_ReturnsEmptyArray()
    {
        // DEVICE_LIST = 0x1600 -> lo=0x00, hi=0x16, argc=0
        var value = RunAndPop(new byte[] {
            (byte)Opcode.CallNative, 0x00, 0x16, 0x00,
            (byte)Opcode.Halt
        });
        Assert.Equal("[]", value.AsString());
    }

    [Fact]
    public void DeviceProbe_DefaultPlatform_ReturnsEmptyArray()
    {
        // DEVICE_PROBE = 0x1604 -> lo=0x04, hi=0x16, argc=0
        var value = RunAndPop(new byte[] {
            (byte)Opcode.CallNative, 0x04, 0x16, 0x00,
            (byte)Opcode.Halt
        });
        Assert.Equal("[]", value.AsString());
    }

    [Fact]
    public void SpiOpen_DefaultPlatform_ReturnsMinusOne()
    {
        // SPI_OPEN = 0x1500 -> lo=0x00, hi=0x15, argc=1
        var value = RunAndPop(new byte[] {
            (byte)Opcode.PushStr, 0x00, 0x00,
            (byte)Opcode.CallNative, 0x00, 0x15, 0x01,
            (byte)Opcode.Halt
        }, constants: new List<string> { "{\"clockHz\":1000000,\"mode\":0,\"csPin\":5}" });
        Assert.Equal(-1, value.AsInt32());
    }

    [Fact]
    public void DeviceOpen_UnknownDevice_ReturnsMinusOne()
    {
        // DEVICE_OPEN = 0x1601 -> lo=0x01, hi=0x16, argc=1
        var value = RunAndPop(new byte[] {
            (byte)Opcode.PushStr, 0x00, 0x00,
            (byte)Opcode.CallNative, 0x01, 0x16, 0x01,
            (byte)Opcode.Halt
        }, constants: new List<string> { "bmp280" });
        Assert.Equal(-1, value.AsInt32());
    }

    [Fact]
    public void DeviceList_CustomPlatform_ReturnsRegisteredDevice()
    {
        var platform = new FakeDevicePlatform();
        var value = RunAndPop(new byte[] {
            (byte)Opcode.CallNative, 0x00, 0x16, 0x00,
            (byte)Opcode.Halt
        }, platform: platform);
        Assert.Contains("bmp280", value.AsString());
        Assert.Contains("0x76", value.AsString());
    }

    [Fact]
    public void SpiTransfer_CustomPlatform_PassesTxAndRxLen()
    {
        var platform = new FakeDevicePlatform();
        // Stack order (bottom->top): handle, tx, rxLen
        // SPI_TRANSFER = 0x1501 -> lo=0x01, hi=0x15, argc=3
        var value = RunAndPop(new byte[] {
            (byte)Opcode.PushI8, 7,              // handle
            (byte)Opcode.PushStr, 0x00, 0x00,    // tx = "AB"
            (byte)Opcode.PushI8, 4,              // rxLen = 4
            (byte)Opcode.CallNative, 0x01, 0x15, 0x03,
            (byte)Opcode.Halt
        }, constants: new List<string> { "AB" }, platform: platform);
        Assert.Equal("rx:4", value.AsString());
        Assert.Equal(1, platform.SpiTransferCalls);
    }

    /// <summary>Platform with a registered fake device and SPI capture behavior.</summary>
    private class FakeDevicePlatform : PlatformBase
    {
        private readonly Stopwatch _sw = Stopwatch.StartNew();

        public int SpiTransferCalls { get; private set; }

        public override string DeviceList()
            => "[{\"name\":\"bmp280\",\"bus\":\"i2c\",\"address\":\"0x76\",\"capabilities\":{\"type\":\"sensor\"}}]";

        public override string DeviceProbe()
            => "[{\"name\":\"bmp280\",\"bus\":\"i2c\",\"address\":\"0x76\",\"capabilities\":{\"type\":\"sensor\"}}]";

        public override string SpiTransfer(int handle, byte[] tx, int rxLen)
        {
            SpiTransferCalls++;
            Assert.Equal(7, handle);
            Assert.Equal(2, tx.Length);
            Assert.Equal(4, rxLen);
            return "rx:4";
        }

        public override uint SystemGetTime() => (uint)_sw.ElapsedMilliseconds;
        public override void SystemSleep(uint ms) { }
    }
}
