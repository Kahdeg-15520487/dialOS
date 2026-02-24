using DialOS.Runtime.Bytecode;
using DialOS.Runtime.Platform;
using DialOS.Runtime.Values;
using DialOS.Runtime.VM;

namespace DialOS.Tests;

public class VMTests
{
    private BytecodeModule CreateTestModule(byte[] code, List<string>? constants = null, List<string>? globals = null)
    {
        var module = new BytecodeModule
        {
            Code = code,
            Metadata = new Metadata { HeapSize = 8192 }
        };

        if (constants != null)
        {
            module.Constants.AddRange(constants);
        }

        if (globals != null)
        {
            module.Globals.AddRange(globals);
        }

        return module;
    }

    [Fact]
    public void VMState_Initialization_ShouldSetMainEntryPoint()
    {
        var module = CreateTestModule(new byte[] { (byte)Opcode.Halt });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);

        Assert.Equal(0, state.Pc);
        Assert.False(state.Running);
    }

    [Fact]
    public void VMState_Stack_PushPop()
    {
        var module = CreateTestModule(new byte[] { (byte)Opcode.Halt });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);

        state.Push(Value.Int32(42));
        state.Push(Value.Int32(100));

        Assert.Equal(2, state.StackHeight);
        Assert.Equal(100, state.Pop().AsInt32());
        Assert.Equal(42, state.Pop().AsInt32());
    }

    [Fact]
    public void VMState_Globals_GetSet()
    {
        var module = CreateTestModule(
            new byte[] { (byte)Opcode.Halt },
            globals: new List<string> { "testVar" }
        );
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);

        state.SetGlobal(0, Value.Int32(123));
        Assert.Equal(123, state.GetGlobal(0).AsInt32());
        Assert.Equal(123, state.GetGlobal("testVar").AsInt32());
    }

    [Fact]
    public void ExecutionEngine_Nop_ShouldDoNothing()
    {
        var module = CreateTestModule(new byte[] { (byte)Opcode.Nop, (byte)Opcode.Halt });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        var result = engine.Execute(100);

        Assert.Equal(VMResult.Finished, result);
        Assert.Equal(2, state.Pc);
    }

    [Fact]
    public void ExecutionEngine_PushNull_ShouldPushNull()
    {
        var module = CreateTestModule(new byte[] { (byte)Opcode.PushNull, (byte)Opcode.Halt });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.True(state.Pop().IsNull);
    }

    [Fact]
    public void ExecutionEngine_PushTrueFalse_ShouldPushBooleans()
    {
        var module = CreateTestModule(new byte[] { (byte)Opcode.PushTrue, (byte)Opcode.PushFalse, (byte)Opcode.Halt });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.False(state.Pop().AsBool()); // false was pushed last
        Assert.True(state.Pop().AsBool());  // true was pushed first
    }

    [Fact]
    public void ExecutionEngine_PushI8_ShouldPush8BitInteger()
    {
        var module = CreateTestModule(new byte[] { (byte)Opcode.PushI8, 0x2A, (byte)Opcode.Halt }); // 42
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.Equal(42, state.Pop().AsInt32());
    }

    [Fact]
    public void ExecutionEngine_PushI32_ShouldPush32BitInteger()
    {
        // 0x12345678 in little-endian
        var module = CreateTestModule(new byte[] {
            (byte)Opcode.PushI32, 0x78, 0x56, 0x34, 0x12, (byte)Opcode.Halt
        });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.Equal(0x12345678, state.Pop().AsInt32());
    }

    [Fact]
    public void ExecutionEngine_PushStr_ShouldPushString()
    {
        var module = CreateTestModule(
            new byte[] { (byte)Opcode.PushStr, 0x00, 0x00, (byte)Opcode.Halt },
            constants: new List<string> { "Hello, World!" }
        );
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.Equal("Hello, World!", state.Pop().AsString());
    }

    [Fact]
    public void ExecutionEngine_Arithmetic_Add()
    {
        var module = CreateTestModule(new byte[] {
            (byte)Opcode.PushI8, 3,
            (byte)Opcode.PushI8, 4,
            (byte)Opcode.Add,
            (byte)Opcode.Halt
        });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.Equal(7, state.Pop().AsInt32());
    }

    [Fact]
    public void ExecutionEngine_Arithmetic_Sub()
    {
        var module = CreateTestModule(new byte[] {
            (byte)Opcode.PushI8, 10,
            (byte)Opcode.PushI8, 3,
            (byte)Opcode.Sub,
            (byte)Opcode.Halt
        });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.Equal(7, state.Pop().AsInt32());
    }

    [Fact]
    public void ExecutionEngine_Arithmetic_Mul()
    {
        var module = CreateTestModule(new byte[] {
            (byte)Opcode.PushI8, 6,
            (byte)Opcode.PushI8, 7,
            (byte)Opcode.Mul,
            (byte)Opcode.Halt
        });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.Equal(42, state.Pop().AsInt32());
    }

    [Fact]
    public void ExecutionEngine_Comparison_Eq()
    {
        var module = CreateTestModule(new byte[] {
            (byte)Opcode.PushI8, 5,
            (byte)Opcode.PushI8, 5,
            (byte)Opcode.Eq,
            (byte)Opcode.Halt
        });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.True(state.Pop().AsBool());
    }

    [Fact]
    public void ExecutionEngine_Comparison_Lt()
    {
        var module = CreateTestModule(new byte[] {
            (byte)Opcode.PushI8, 3,
            (byte)Opcode.PushI8, 5,
            (byte)Opcode.Lt,
            (byte)Opcode.Halt
        });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.True(state.Pop().AsBool());
    }

    [Fact]
    public void ExecutionEngine_LocalVariables_StoreLoad()
    {
        var module = CreateTestModule(new byte[] {
            (byte)Opcode.PushI8, 42,
            (byte)Opcode.StoreLocal, 0,
            (byte)Opcode.LoadLocal, 0,
            (byte)Opcode.Halt
        });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.Equal(42, state.Pop().AsInt32());
    }

    [Fact]
    public void ExecutionEngine_Jump_ShouldJump()
    {
        // Jump over the HALT to reach the PUSH
        // After reading Jump opcode at PC=0, PC=1
        // We want to land at byte 6 (PushI8)
        // PC = 1 + 4 + offset, so offset = 6 - 5 = 1
        var module = CreateTestModule(new byte[] {
            (byte)Opcode.Jump, 0x01, 0x00, 0x00, 0x00, // Jump +1 (to byte 6)
            (byte)Opcode.Halt,                          // byte 5 - should be skipped
            (byte)Opcode.PushI8, 42,                    // bytes 6-7
            (byte)Opcode.Halt                           // byte 8
        });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.Equal(42, state.Pop().AsInt32());
    }

    [Fact]
    public void ExecutionEngine_JumpIf_ShouldJumpWhenTrue()
    {
        // Byte layout:
        // 0: PushTrue
        // 1: JumpIf
        // 2-5: offset
        // 6: PushI8 (1)
        // 7: 1
        // 8: Jump
        // 9-12: offset
        // 13: PushI8 (2)
        // 14: 2
        // 15: Halt
        //
        // After reading JumpIf and offset, PC=6. To jump to byte 13, offset = 13-6 = 7
        var module = CreateTestModule(new byte[] {
            (byte)Opcode.PushTrue,
            (byte)Opcode.JumpIf, 0x07, 0x00, 0x00, 0x00, // Jump +7 if true (to byte 13)
            (byte)Opcode.PushI8, 1,                      // Should be skipped
            (byte)Opcode.Jump, 0x02, 0x00, 0x00, 0x00,   // Jump +2 (never reached)
            (byte)Opcode.PushI8, 2,                      // Should execute
            (byte)Opcode.Halt
        });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.Equal(2, state.Pop().AsInt32());
    }

    [Fact]
    public void ExecutionEngine_NewObject_GetSetField()
    {
        var module = CreateTestModule(
            new byte[] {
                (byte)Opcode.NewObject, 0x00, 0x00,  // class index 0 - creates object, pushes it
                (byte)Opcode.Dup,                     // duplicate object ref
                (byte)Opcode.PushI8, 42,
                (byte)Opcode.SetField, 0x01, 0x00,   // field index 1 - pops value and obj, pushes value
                (byte)Opcode.Pop,                     // pop the result of SetField
                (byte)Opcode.PushI8, 0,               // push dummy (we'll use local var approach)
                (byte)Opcode.Pop,                     // pop it
                (byte)Opcode.LoadLocal, 0,            // load the stored object
                (byte)Opcode.GetField, 0x01, 0x00,   // field index 1
                (byte)Opcode.Halt
            },
            constants: new List<string> { "TestClass", "value" }
        );
        // Simpler approach: store object in local 0, then get field
        var module2 = CreateTestModule(
            new byte[] {
                (byte)Opcode.NewObject, 0x00, 0x00,  // class index 0
                (byte)Opcode.StoreLocal, 0,          // store object in local 0
                (byte)Opcode.PushI8, 42,
                (byte)Opcode.LoadLocal, 0,           // load object
                (byte)Opcode.Swap,                   // swap to get [obj, value]
                (byte)Opcode.SetField, 0x01, 0x00,   // set field
                (byte)Opcode.Pop,                    // pop result
                (byte)Opcode.LoadLocal, 0,           // load object
                (byte)Opcode.GetField, 0x01, 0x00,   // get field
                (byte)Opcode.Halt
            },
            constants: new List<string> { "TestClass", "value" }
        );
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module2, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.Equal(42, state.Pop().AsInt32());
    }

    [Fact]
    public void ExecutionEngine_NewArray_GetSetIndex()
    {
        // SetIndex pops value, index, array - then pushes result
        // Stack before SetIndex: [array, index, value] (value on top)
        // After SetIndex: [result]
        // We need a different approach: store array in local
        var module = CreateTestModule(new byte[] {
            (byte)Opcode.PushI8, 3,           // array size
            (byte)Opcode.NewArray,            // create array - pushes array
            (byte)Opcode.StoreLocal, 0,       // store array in local 0
            (byte)Opcode.LoadLocal, 0,        // load array
            (byte)Opcode.PushI8, 1,           // index
            (byte)Opcode.PushI8, 99,          // value
            (byte)Opcode.SetIndex,            // set array[1] = 99 - pops value, index, array; pushes result
            (byte)Opcode.Pop,                 // pop result
            (byte)Opcode.LoadLocal, 0,        // load array
            (byte)Opcode.PushI8, 1,           // index
            (byte)Opcode.GetIndex,            // get array[1]
            (byte)Opcode.Halt
        });
        var pool = new ValuePool(8192);
        var platform = new ConsolePlatform();
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.Equal(99, state.Pop().AsInt32());
    }

    [Fact]
    public void ExecutionEngine_CallNative_ConsolePrint()
    {
        var output = new System.Text.StringBuilder();
        var platform = new TestPlatform(output);
        var module = CreateTestModule(
            new byte[] {
                (byte)Opcode.PushStr, 0x00, 0x00,   // constant index 0
                (byte)Opcode.CallNative, 0x00, 0x00, 0x01,  // native ID 0x0000 (print), 1 arg
                (byte)Opcode.Halt
            },
            constants: new List<string> { "Hello" }
        );
        var pool = new ValuePool(8192);
        var state = new VMState(module, pool, platform);
        var engine = new ExecutionEngine(state);

        engine.Execute(100);

        Assert.Equal("Hello", output.ToString());
    }

    [Fact]
    public void ValuePool_StringInterning_ShouldReuseStrings()
    {
        var pool = new ValuePool(8192);

        var str1 = pool.InternString("test");
        var str2 = pool.InternString("test");

        Assert.Same(str1, str2);
    }

    [Fact]
    public void ValuePool_AllocateObject_ShouldTrackMemory()
    {
        var pool = new ValuePool(8192);
        var initialAllocated = pool.Allocated;

        pool.AllocateObject("Test");

        Assert.True(pool.Allocated > initialAllocated);
    }
}

/// <summary>
/// Test platform that captures console output.
/// </summary>
internal class TestPlatform : PlatformBase
{
    private readonly System.Text.StringBuilder _output;
    private readonly System.Diagnostics.Stopwatch _stopwatch;

    public TestPlatform(System.Text.StringBuilder output)
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
        Thread.Sleep((int)ms);
    }
}
