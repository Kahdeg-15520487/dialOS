using System.Text;
using DialOS.Runtime.Bytecode;
using DialOS.Runtime.Platform;
using DialOS.Runtime.Values;

namespace DialOS.Runtime.VM;

/// <summary>
/// Main execution engine for the dialScript VM.
/// Implements the instruction dispatch loop and all opcodes.
/// </summary>
public class ExecutionEngine
{
    private readonly VMState _state;
    private readonly BytecodeModule _module;
    private readonly ValuePool _pool;
    private readonly IPlatform _platform;

    public ExecutionEngine(VMState state)
    {
        _state = state;
        _module = state.Module;
        _pool = state.Pool;
        _platform = state.Platform;
    }

    /// <summary>
    /// Execute a batch of instructions.
    /// </summary>
    public VMResult Execute(int maxInstructions)
    {
        _state.Running = true;

        for (int i = 0; i < maxInstructions && _state.Running; i++)
        {
            // Check for sleep
            if (_state.Sleeping)
            {
                if (_platform.SystemGetTime() >= _state.SleepUntil)
                {
                    _state.Sleeping = false;
                }
                else
                {
                    return VMResult.Yield;
                }
            }

            var result = ExecuteInstruction();
            if (result != VMResult.Ok)
            {
                return result;
            }
        }

        return _state.Running ? VMResult.Ok : VMResult.Finished;
    }

    /// <summary>
    /// Execute a single instruction.
    /// </summary>
    private VMResult ExecuteInstruction()
    {
        if (_state.Pc < 0 || _state.Pc >= _module.Code.Length)
        {
            _state.Running = false;
            return VMResult.Finished;
        }

        var opcode = (Opcode)_module.Code[_state.Pc++];

        try
        {
            return opcode switch
            {
                // Stack operations
                Opcode.Nop => VMResult.Ok,
                Opcode.Pop => ExecutePop(),
                Opcode.Dup => ExecuteDup(),
                Opcode.Swap => ExecuteSwap(),

                // Constants
                Opcode.PushNull => ExecutePushNull(),
                Opcode.PushTrue => ExecutePushTrue(),
                Opcode.PushFalse => ExecutePushFalse(),
                Opcode.PushI8 => ExecutePushI8(),
                Opcode.PushI16 => ExecutePushI16(),
                Opcode.PushI32 => ExecutePushI32(),
                Opcode.PushF32 => ExecutePushF32(),
                Opcode.PushStr => ExecutePushStr(),

                // Local variables
                Opcode.LoadLocal => ExecuteLoadLocal(),
                Opcode.StoreLocal => ExecuteStoreLocal(),

                // Global variables
                Opcode.LoadGlobal => ExecuteLoadGlobal(),
                Opcode.StoreGlobal => ExecuteStoreGlobal(),

                // Arithmetic
                Opcode.Add => ExecuteAdd(),
                Opcode.Sub => ExecuteSub(),
                Opcode.Mul => ExecuteMul(),
                Opcode.Div => ExecuteDiv(),
                Opcode.Mod => ExecuteMod(),
                Opcode.Neg => ExecuteNeg(),

                // String operations
                Opcode.StrConcat => ExecuteStrConcat(),
                Opcode.TemplateFormat => ExecuteTemplateFormat(),

                // Comparison
                Opcode.Eq => ExecuteEq(),
                Opcode.Ne => ExecuteNe(),
                Opcode.Lt => ExecuteLt(),
                Opcode.Le => ExecuteLe(),
                Opcode.Gt => ExecuteGt(),
                Opcode.Ge => ExecuteGe(),

                // Logical
                Opcode.Not => ExecuteNot(),
                Opcode.And => ExecuteAnd(),
                Opcode.Or => ExecuteOr(),

                // Control flow
                Opcode.Jump => ExecuteJump(),
                Opcode.JumpIf => ExecuteJumpIf(),
                Opcode.JumpIfNot => ExecuteJumpIfNot(),

                // Functions
                Opcode.Call => ExecuteCall(),
                Opcode.CallNative => ExecuteCallNative(),
                Opcode.Return => ExecuteReturn(),
                Opcode.LoadFunction => ExecuteLoadFunction(),
                Opcode.CallIndirect => ExecuteCallIndirect(),
                Opcode.CallMethod => ExecuteCallMethod(),

                // Object/Member access
                Opcode.GetField => ExecuteGetField(),
                Opcode.SetField => ExecuteSetField(),
                Opcode.GetIndex => ExecuteGetIndex(),
                Opcode.SetIndex => ExecuteSetIndex(),

                // Object creation
                Opcode.NewObject => ExecuteNewObject(),
                Opcode.NewArray => ExecuteNewArray(),

                // Exception handling
                Opcode.Try => ExecuteTry(),
                Opcode.EndTry => ExecuteEndTry(),
                Opcode.Throw => ExecuteThrow(),

                // Special
                Opcode.Print => ExecutePrint(),
                Opcode.Halt => ExecuteHalt(),

                _ => VMResult.Ok
            };
        }
        catch (OutOfMemoryException)
        {
            _state.LastError = "Out of memory";
            return VMResult.OutOfMemory;
        }
        catch (Exception ex)
        {
            _state.LastError = ex.Message;

            // Check for exception handler
            if (_state.ExceptionHandlers.Count > 0)
            {
                var handler = _state.ExceptionHandlers.Pop();
                _state.Pc = handler.CatchPc;

                // Restore stack height
                while (_state.StackHeight > handler.StackHeight)
                {
                    _state.Pop();
                }

                // Restore call stack
                while (_state.CallStack.Count > handler.FrameIndex + 1)
                {
                    _state.CallStack.RemoveAt(_state.CallStack.Count - 1);
                }

                // Push exception value
                _state.Push(Value.String(ex.Message));
                return VMResult.Ok;
            }

            return VMResult.Error;
        }
    }

    #region Stack Operations

    private VMResult ExecutePop()
    {
        _state.Pop();
        return VMResult.Ok;
    }

    private VMResult ExecuteDup()
    {
        var value = _state.Peek();
        _state.Push(value);
        return VMResult.Ok;
    }

    private VMResult ExecuteSwap()
    {
        var a = _state.Pop();
        var b = _state.Pop();
        _state.Push(a);
        _state.Push(b);
        return VMResult.Ok;
    }

    #endregion

    #region Constants

    private VMResult ExecutePushNull()
    {
        _state.Push(Value.Null());
        return VMResult.Ok;
    }

    private VMResult ExecutePushTrue()
    {
        _state.Push(Value.Bool(true));
        return VMResult.Ok;
    }

    private VMResult ExecutePushFalse()
    {
        _state.Push(Value.Bool(false));
        return VMResult.Ok;
    }

    private VMResult ExecutePushI8()
    {
        var value = (sbyte)_module.Code[_state.Pc++];
        _state.Push(Value.Int32(value));
        return VMResult.Ok;
    }

    private VMResult ExecutePushI16()
    {
        var value = (short)(_module.Code[_state.Pc] | (_module.Code[_state.Pc + 1] << 8));
        _state.Pc += 2;
        _state.Push(Value.Int32(value));
        return VMResult.Ok;
    }

    private VMResult ExecutePushI32()
    {
        var value = _module.Code[_state.Pc] |
                   (_module.Code[_state.Pc + 1] << 8) |
                   (_module.Code[_state.Pc + 2] << 16) |
                   (_module.Code[_state.Pc + 3] << 24);
        _state.Pc += 4;
        _state.Push(Value.Int32(value));
        return VMResult.Ok;
    }

    private VMResult ExecutePushF32()
    {
        var bytes = new byte[] {
            _module.Code[_state.Pc],
            _module.Code[_state.Pc + 1],
            _module.Code[_state.Pc + 2],
            _module.Code[_state.Pc + 3]
        };
        _state.Pc += 4;
        var value = BitConverter.ToSingle(bytes, 0);
        _state.Push(Value.Float32(value));
        return VMResult.Ok;
    }

    private VMResult ExecutePushStr()
    {
        var index = _module.Code[_state.Pc] | (_module.Code[_state.Pc + 1] << 8);
        _state.Pc += 2;
        var str = _module.GetConstant(index);
        var interned = _pool.InternString(str);
        _state.Push(Value.String(interned));
        return VMResult.Ok;
    }

    #endregion

    #region Variables

    private VMResult ExecuteLoadLocal()
    {
        var index = _module.Code[_state.Pc++];
        var value = _state.CurrentFrame.GetLocal(index);
        _state.Push(value);
        return VMResult.Ok;
    }

    private VMResult ExecuteStoreLocal()
    {
        var index = _module.Code[_state.Pc++];
        var value = _state.Pop();
        _state.CurrentFrame.SetLocal(index, value);
        return VMResult.Ok;
    }

    private VMResult ExecuteLoadGlobal()
    {
        var index = _module.Code[_state.Pc] | (_module.Code[_state.Pc + 1] << 8);
        _state.Pc += 2;
        var value = _state.GetGlobal(index);
        _state.Push(value);
        return VMResult.Ok;
    }

    private VMResult ExecuteStoreGlobal()
    {
        var index = _module.Code[_state.Pc] | (_module.Code[_state.Pc + 1] << 8);
        _state.Pc += 2;
        var value = _state.Pop();
        _state.SetGlobal(index, value);
        return VMResult.Ok;
    }

    #endregion

    #region Arithmetic

    private VMResult ExecuteAdd()
    {
        var b = _state.Pop();
        var a = _state.Pop();

        if (a.IsString || b.IsString)
        {
            _state.Push(Value.String(a.ToString() + b.ToString()));
        }
        else if (a.IsFloat32 || b.IsFloat32)
        {
            _state.Push(Value.Float32(a.ToFloat() + b.ToFloat()));
        }
        else
        {
            _state.Push(Value.Int32(a.AsInt32() + b.AsInt32()));
        }
        return VMResult.Ok;
    }

    private VMResult ExecuteSub()
    {
        var b = _state.Pop();
        var a = _state.Pop();

        if (a.IsFloat32 || b.IsFloat32)
        {
            _state.Push(Value.Float32(a.ToFloat() - b.ToFloat()));
        }
        else
        {
            _state.Push(Value.Int32(a.AsInt32() - b.AsInt32()));
        }
        return VMResult.Ok;
    }

    private VMResult ExecuteMul()
    {
        var b = _state.Pop();
        var a = _state.Pop();

        if (a.IsFloat32 || b.IsFloat32)
        {
            _state.Push(Value.Float32(a.ToFloat() * b.ToFloat()));
        }
        else
        {
            _state.Push(Value.Int32(a.AsInt32() * b.AsInt32()));
        }
        return VMResult.Ok;
    }

    private VMResult ExecuteDiv()
    {
        var b = _state.Pop();
        var a = _state.Pop();

        if (a.IsFloat32 || b.IsFloat32)
        {
            var bf = b.ToFloat();
            _state.Push(Math.Abs(bf) < float.Epsilon
                ? Value.Float32(float.NaN)
                : Value.Float32(a.ToFloat() / bf));
        }
        else
        {
            var bi = b.AsInt32();
            _state.Push(bi == 0
                ? Value.Int32(0)
                : Value.Int32(a.AsInt32() / bi));
        }
        return VMResult.Ok;
    }

    private VMResult ExecuteMod()
    {
        var b = _state.Pop();
        var a = _state.Pop();

        if (a.IsFloat32 || b.IsFloat32)
        {
            _state.Push(Value.Float32(a.ToFloat() % b.ToFloat()));
        }
        else
        {
            _state.Push(Value.Int32(a.AsInt32() % b.AsInt32()));
        }
        return VMResult.Ok;
    }

    private VMResult ExecuteNeg()
    {
        var a = _state.Pop();
        _state.Push(a.IsFloat32 ? Value.Float32(-a.AsFloat32()) : Value.Int32(-a.AsInt32()));
        return VMResult.Ok;
    }

    #endregion

    #region String Operations

    private VMResult ExecuteStrConcat()
    {
        var b = _state.Pop();
        var a = _state.Pop();
        var result = _pool.InternString(a.ToString() + b.ToString());
        _state.Push(Value.String(result));
        return VMResult.Ok;
    }

    private VMResult ExecuteTemplateFormat()
    {
        var argCount = _module.Code[_state.Pc++];
        var args = new Value[argCount];
        for (int i = argCount - 1; i >= 0; i--)
        {
            args[i] = _state.Pop();
        }
        var template = _state.Pop().ToString();

        // Simple template formatting: replace {0}, {1}, etc.
        var result = template;
        for (int i = 0; i < argCount; i++)
        {
            result = result.Replace($"{{{i}}}", args[i].ToString());
        }

        _state.Push(Value.String(_pool.InternString(result)));
        return VMResult.Ok;
    }

    #endregion

    #region Comparison

    private VMResult ExecuteEq()
    {
        var b = _state.Pop();
        var a = _state.Pop();
        _state.Push(Value.Bool(a.Equals(b)));
        return VMResult.Ok;
    }

    private VMResult ExecuteNe()
    {
        var b = _state.Pop();
        var a = _state.Pop();
        _state.Push(Value.Bool(!a.Equals(b)));
        return VMResult.Ok;
    }

    private VMResult ExecuteLt()
    {
        var b = _state.Pop();
        var a = _state.Pop();
        if (a.IsFloat32 || b.IsFloat32)
            _state.Push(Value.Bool(a.ToFloat() < b.ToFloat()));
        else
            _state.Push(Value.Bool(a.AsInt32() < b.AsInt32()));
        return VMResult.Ok;
    }

    private VMResult ExecuteLe()
    {
        var b = _state.Pop();
        var a = _state.Pop();
        if (a.IsFloat32 || b.IsFloat32)
            _state.Push(Value.Bool(a.ToFloat() <= b.ToFloat()));
        else
            _state.Push(Value.Bool(a.AsInt32() <= b.AsInt32()));
        return VMResult.Ok;
    }

    private VMResult ExecuteGt()
    {
        var b = _state.Pop();
        var a = _state.Pop();
        if (a.IsFloat32 || b.IsFloat32)
            _state.Push(Value.Bool(a.ToFloat() > b.ToFloat()));
        else
            _state.Push(Value.Bool(a.AsInt32() > b.AsInt32()));
        return VMResult.Ok;
    }

    private VMResult ExecuteGe()
    {
        var b = _state.Pop();
        var a = _state.Pop();
        if (a.IsFloat32 || b.IsFloat32)
            _state.Push(Value.Bool(a.ToFloat() >= b.ToFloat()));
        else
            _state.Push(Value.Bool(a.AsInt32() >= b.AsInt32()));
        return VMResult.Ok;
    }

    #endregion

    #region Logical

    private VMResult ExecuteNot()
    {
        var a = _state.Pop();
        _state.Push(Value.Bool(!a.IsTruthy()));
        return VMResult.Ok;
    }

    private VMResult ExecuteAnd()
    {
        var b = _state.Pop();
        var a = _state.Pop();
        _state.Push(Value.Bool(a.IsTruthy() && b.IsTruthy()));
        return VMResult.Ok;
    }

    private VMResult ExecuteOr()
    {
        var b = _state.Pop();
        var a = _state.Pop();
        _state.Push(Value.Bool(a.IsTruthy() || b.IsTruthy()));
        return VMResult.Ok;
    }

    #endregion

    #region Control Flow

    private VMResult ExecuteJump()
    {
        var offsetBytes = new byte[] {
            _module.Code[_state.Pc],
            _module.Code[_state.Pc + 1],
            _module.Code[_state.Pc + 2],
            _module.Code[_state.Pc + 3]
        };
        var offset = BitConverter.ToInt32(offsetBytes, 0);
        _state.Pc += 4 + offset;
        return VMResult.Ok;
    }

    private VMResult ExecuteJumpIf()
    {
        var offsetBytes = new byte[] {
            _module.Code[_state.Pc],
            _module.Code[_state.Pc + 1],
            _module.Code[_state.Pc + 2],
            _module.Code[_state.Pc + 3]
        };
        var offset = BitConverter.ToInt32(offsetBytes, 0);
        _state.Pc += 4;

        var condition = _state.Pop();
        if (condition.IsTruthy())
        {
            _state.Pc += offset;
        }
        return VMResult.Ok;
    }

    private VMResult ExecuteJumpIfNot()
    {
        var offsetBytes = new byte[] {
            _module.Code[_state.Pc],
            _module.Code[_state.Pc + 1],
            _module.Code[_state.Pc + 2],
            _module.Code[_state.Pc + 3]
        };
        var offset = BitConverter.ToInt32(offsetBytes, 0);
        _state.Pc += 4;

        var condition = _state.Pop();
        if (!condition.IsTruthy())
        {
            _state.Pc += offset;
        }
        return VMResult.Ok;
    }

    #endregion

    #region Functions

    private VMResult ExecuteCall()
    {
        var funcIndex = _module.Code[_state.Pc] | (_module.Code[_state.Pc + 1] << 8);
        var argCount = _module.Code[_state.Pc + 2];
        _state.Pc += 3;

        var func = _module.GetFunction(funcIndex);

        // Create new call frame
        var frame = new CallFrame(_state.Pc, _state.StackHeight, 256, func.Name);
        frame.ArgCount = argCount;

        // Pop arguments and store as locals (in reverse order)
        for (int i = argCount - 1; i >= 0; i--)
        {
            frame.SetLocal(i, _state.Pop());
        }

        _state.CallStack.Add(frame);
        _state.Pc = (int)func.EntryPoint;

        return VMResult.Ok;
    }

    private VMResult ExecuteCallNative()
    {
        var nativeId = (ushort)(_module.Code[_state.Pc] | (_module.Code[_state.Pc + 1] << 8));
        var argCount = _module.Code[_state.Pc + 2];
        _state.Pc += 3;

        // Pop arguments (in reverse order)
        var args = new Value[argCount];
        for (int i = argCount - 1; i >= 0; i--)
        {
            args[i] = _state.Pop();
        }

        // Execute native function
        var result = ExecuteNativeFunction(nativeId, args);
        _state.Push(result);

        return VMResult.Ok;
    }

    private VMResult ExecuteReturn()
    {
        if (_state.CallStack.Count <= 1)
        {
            // Return from main - halt
            _state.Running = false;
            return VMResult.Finished;
        }

        var returnValue = _state.Pop();
        var frame = _state.CallStack[^1];
        _state.CallStack.RemoveAt(_state.CallStack.Count - 1);

        // Restore PC
        _state.Pc = frame.ReturnPc;

        // Push return value
        _state.Push(returnValue);

        return VMResult.Ok;
    }

    private VMResult ExecuteLoadFunction()
    {
        var funcIndex = _module.Code[_state.Pc] | (_module.Code[_state.Pc + 1] << 8);
        _state.Pc += 2;

        var func = _module.GetFunction(funcIndex);
        var fn = _pool.AllocateFunction((ushort)funcIndex, func.ParamCount);
        _state.Push(Value.Function(fn));

        return VMResult.Ok;
    }

    private VMResult ExecuteCallIndirect()
    {
        var argCount = _module.Code[_state.Pc++];

        var fnValue = _state.Pop();
        if (!fnValue.IsFunction)
        {
            throw new InvalidOperationException("Cannot call non-function value");
        }

        var fn = fnValue.AsFunction();
        var func = _module.GetFunction(fn.FunctionIndex);

        // Create new call frame
        var frame = new CallFrame(_state.Pc, _state.StackHeight, 256, func.Name);
        frame.ArgCount = argCount;

        // Pop arguments and store as locals (in reverse order)
        for (int i = argCount - 1; i >= 0; i--)
        {
            frame.SetLocal(i, _state.Pop());
        }

        _state.CallStack.Add(frame);
        _state.Pc = (int)func.EntryPoint;

        return VMResult.Ok;
    }

    private VMResult ExecuteCallMethod()
    {
        var argCount = _module.Code[_state.Pc++];
        var nameIdx = _module.Code[_state.Pc] | (_module.Code[_state.Pc + 1] << 8);
        _state.Pc += 2;

        var methodName = _module.GetConstant(nameIdx);

        // Pop arguments (in reverse order)
        var args = new Value[argCount];
        for (int i = argCount - 1; i >= 0; i--)
        {
            args[i] = _state.Pop();
        }

        // Get receiver (object)
        var receiver = _state.Pop();
        if (!receiver.IsObject)
        {
            throw new InvalidOperationException("Cannot call method on non-object");
        }

        // Get method from object
        var obj = receiver.AsObject();
        var methodValue = obj.GetField(methodName);

        if (!methodValue.IsFunction)
        {
            throw new InvalidOperationException($"Method '{methodName}' not found or not a function");
        }

        var fn = methodValue.AsFunction();
        var func = _module.GetFunction(fn.FunctionIndex);

        // Create new call frame with receiver as first argument
        var frame = new CallFrame(_state.Pc, _state.StackHeight, 256, func.Name);
        frame.ArgCount = argCount + 1;

        // Store receiver as local 0
        frame.SetLocal(0, receiver);

        // Store other arguments
        for (int i = 0; i < argCount; i++)
        {
            frame.SetLocal(i + 1, args[i]);
        }

        _state.CallStack.Add(frame);
        _state.Pc = (int)func.EntryPoint;

        return VMResult.Ok;
    }

    #endregion

    #region Object/Member Access

    private VMResult ExecuteGetField()
    {
        var fieldIdx = _module.Code[_state.Pc] | (_module.Code[_state.Pc + 1] << 8);
        _state.Pc += 2;

        var fieldName = _module.GetConstant(fieldIdx);
        var objValue = _state.Pop();

        if (!objValue.IsObject)
        {
            _state.Push(Value.Null());
            return VMResult.Ok;
        }

        var obj = objValue.AsObject();
        _state.Push(obj.GetField(fieldName));
        return VMResult.Ok;
    }

    private VMResult ExecuteSetField()
    {
        var fieldIdx = _module.Code[_state.Pc] | (_module.Code[_state.Pc + 1] << 8);
        _state.Pc += 2;

        var fieldName = _module.GetConstant(fieldIdx);
        var value = _state.Pop();
        var objValue = _state.Pop();

        if (objValue.IsObject)
        {
            var obj = objValue.AsObject();
            obj.SetField(fieldName, value);
        }

        _state.Push(value);
        return VMResult.Ok;
    }

    private VMResult ExecuteGetIndex()
    {
        var index = _state.Pop().ToInt();
        var arrValue = _state.Pop();

        if (!arrValue.IsArray)
        {
            _state.Push(Value.Null());
            return VMResult.Ok;
        }

        var arr = arrValue.AsArray();
        _state.Push(arr[index]);
        return VMResult.Ok;
    }

    private VMResult ExecuteSetIndex()
    {
        var value = _state.Pop();
        var index = _state.Pop().ToInt();
        var arrValue = _state.Pop();

        if (arrValue.IsArray)
        {
            var arr = arrValue.AsArray();
            arr[index] = value;
        }

        _state.Push(value);
        return VMResult.Ok;
    }

    #endregion

    #region Object Creation

    private VMResult ExecuteNewObject()
    {
        var classIdx = _module.Code[_state.Pc] | (_module.Code[_state.Pc + 1] << 8);
        _state.Pc += 2;

        var className = _module.GetConstant(classIdx);
        var obj = _pool.AllocateObject(className);
        _state.Push(Value.Object(obj));
        return VMResult.Ok;
    }

    private VMResult ExecuteNewArray()
    {
        var size = _state.Pop().ToInt();
        var arr = _pool.AllocateArray(size);
        _state.Push(Value.Array(arr));
        return VMResult.Ok;
    }

    #endregion

    #region Exception Handling

    private VMResult ExecuteTry()
    {
        var offsetBytes = new byte[] {
            _module.Code[_state.Pc],
            _module.Code[_state.Pc + 1],
            _module.Code[_state.Pc + 2],
            _module.Code[_state.Pc + 3]
        };
        var offset = BitConverter.ToInt32(offsetBytes, 0);
        _state.Pc += 4;

        var handler = new ExceptionHandler(
            _state.Pc + offset,
            _state.StackHeight,
            _state.CallStack.Count - 1
        );
        _state.ExceptionHandlers.Push(handler);

        return VMResult.Ok;
    }

    private VMResult ExecuteEndTry()
    {
        if (_state.ExceptionHandlers.Count > 0)
        {
            _state.ExceptionHandlers.Pop();
        }
        return VMResult.Ok;
    }

    private VMResult ExecuteThrow()
    {
        var exceptionValue = _state.Pop();
        _state.ExceptionValue = exceptionValue;

        if (_state.ExceptionHandlers.Count > 0)
        {
            var handler = _state.ExceptionHandlers.Pop();
            _state.Pc = handler.CatchPc;

            // Restore stack height
            while (_state.StackHeight > handler.StackHeight)
            {
                _state.Pop();
            }

            // Push exception value
            _state.Push(exceptionValue);
            return VMResult.Ok;
        }

        _state.LastError = exceptionValue.ToString();
        return VMResult.Exception;
    }

    #endregion

    #region Special

    private VMResult ExecutePrint()
    {
        var value = _state.Pop();
        _platform.ConsolePrintLn(value.ToString());
        return VMResult.Ok;
    }

    private VMResult ExecuteHalt()
    {
        _state.Running = false;
        return VMResult.Finished;
    }

    #endregion

    #region Native Functions

    private Value ExecuteNativeFunction(ushort nativeId, Value[] args)
    {
        // Console namespace (0x00xx)
        if (nativeId >= 0x0000 && nativeId < 0x0100)
        {
            return ExecuteConsoleNative(nativeId, args);
        }

        // Display namespace (0x01xx)
        if (nativeId >= 0x0100 && nativeId < 0x0200)
        {
            return ExecuteDisplayNative(nativeId, args);
        }

        // Encoder namespace (0x02xx)
        if (nativeId >= 0x0200 && nativeId < 0x0300)
        {
            return ExecuteEncoderNative(nativeId, args);
        }

        // System namespace (0x03xx)
        if (nativeId >= 0x0300 && nativeId < 0x0400)
        {
            return ExecuteSystemNative(nativeId, args);
        }

        // Touch namespace (0x04xx)
        if (nativeId >= 0x0400 && nativeId < 0x0500)
        {
            return ExecuteTouchNative(nativeId, args);
        }

        // RFID namespace (0x05xx)
        if (nativeId >= 0x0500 && nativeId < 0x0600)
        {
            return ExecuteRfidNative(nativeId, args);
        }

        // File namespace (0x06xx)
        if (nativeId >= 0x0600 && nativeId < 0x0700)
        {
            return ExecuteFileNative(nativeId, args);
        }

        // Directory namespace (0x07xx)
        if (nativeId >= 0x0700 && nativeId < 0x0800)
        {
            return ExecuteDirectoryNative(nativeId, args);
        }

        // GPIO namespace (0x08xx)
        if (nativeId >= 0x0800 && nativeId < 0x0900)
        {
            return ExecuteGpioNative(nativeId, args);
        }

        // I2C namespace (0x09xx)
        if (nativeId >= 0x0900 && nativeId < 0x0A00)
        {
            return ExecuteI2cNative(nativeId, args);
        }

        // Buzzer namespace (0x0Axx)
        if (nativeId >= 0x0A00 && nativeId < 0x0B00)
        {
            return ExecuteBuzzerNative(nativeId, args);
        }

        // Memory namespace (0x0Cxx)
        if (nativeId >= 0x0C00 && nativeId < 0x0D00)
        {
            return ExecuteMemoryNative(nativeId, args);
        }

        // Power namespace (0x0Dxx)
        if (nativeId >= 0x0D00 && nativeId < 0x0E00)
        {
            return ExecutePowerNative(nativeId, args);
        }

        // App namespace (0x0Exx)
        if (nativeId >= 0x0E00 && nativeId < 0x0F00)
        {
            return ExecuteAppNative(nativeId, args);
        }

        // WiFi namespace (0x11xx)
        if (nativeId >= 0x1100 && nativeId < 0x1200)
        {
            return ExecuteWifiNative(nativeId, args);
        }

        // HTTP namespace (0x12xx)
        if (nativeId >= 0x1200 && nativeId < 0x1300)
        {
            return ExecuteHttpNative(nativeId, args);
        }

        // IPC namespace (0x13xx)
        if (nativeId >= 0x1300 && nativeId < 0x1400)
        {
            return ExecuteIpcNative(nativeId, args);
        }

        // SPI namespace (0x15xx)
        if (nativeId >= 0x1500 && nativeId < 0x1600)
        {
            return ExecuteSpiNative(nativeId, args);
        }

        // Device namespace (0x16xx)
        if (nativeId >= 0x1600 && nativeId < 0x1700)
        {
            return ExecuteDeviceNative(nativeId, args);
        }

        return Value.Null();
    }

    private Value ExecuteConsoleNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x0000: // CONSOLE_PRINT
                if (args.Length > 0)
                    _platform.ConsolePrint(args[0].ToString());
                break;
            case 0x0001: // CONSOLE_PRINTLN
                if (args.Length > 0)
                    _platform.ConsolePrintLn(args[0].ToString());
                break;
            case 0x0002: // CONSOLE_LOG
                if (args.Length > 0)
                    _platform.ConsoleLog(args[0].ToString());
                break;
            case 0x0003: // CONSOLE_WARN
                if (args.Length > 0)
                    _platform.ConsoleWarn(args[0].ToString());
                break;
            case 0x0004: // CONSOLE_ERROR
                if (args.Length > 0)
                    _platform.ConsoleError(args[0].ToString());
                break;
            case 0x0005: // CONSOLE_CLEAR
                _platform.ConsoleClear();
                break;
        }
        return Value.Null();
    }

    private Value ExecuteDisplayNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x0100: // DISPLAY_CLEAR
                _platform.DisplayClear(args.Length > 0 ? (uint)args[0].ToInt() : 0);
                break;
            case 0x0101: // DISPLAY_DRAW_TEXT
                if (args.Length >= 5)
                    _platform.DisplayDrawText(
                        args[0].ToInt(), args[1].ToInt(),
                        args[2].ToString(), (uint)args[3].ToInt(), args[4].ToInt());
                break;
            case 0x0102: // DISPLAY_DRAW_RECT
                if (args.Length >= 6)
                    _platform.DisplayDrawRect(
                        args[0].ToInt(), args[1].ToInt(),
                        args[2].ToInt(), args[3].ToInt(),
                        (uint)args[4].ToInt(), args[5].IsTruthy());
                break;
            case 0x0103: // DISPLAY_DRAW_CIRCLE
                if (args.Length >= 5)
                    _platform.DisplayDrawCircle(
                        args[0].ToInt(), args[1].ToInt(),
                        args[2].ToInt(), (uint)args[3].ToInt(), args[4].IsTruthy());
                break;
            case 0x0104: // DISPLAY_DRAW_LINE
                if (args.Length >= 5)
                    _platform.DisplayDrawLine(
                        args[0].ToInt(), args[1].ToInt(),
                        args[2].ToInt(), args[3].ToInt(), (uint)args[4].ToInt());
                break;
            case 0x0105: // DISPLAY_DRAW_PIXEL
                if (args.Length >= 3)
                    _platform.DisplayDrawPixel(args[0].ToInt(), args[1].ToInt(), (uint)args[2].ToInt());
                break;
            case 0x0106: // DISPLAY_SET_BRIGHTNESS
                if (args.Length > 0)
                    _platform.DisplaySetBrightness(args[0].ToInt());
                break;
            case 0x0107: // DISPLAY_GET_WIDTH
                return Value.Int32(_platform.DisplayWidth);
            case 0x0108: // DISPLAY_GET_HEIGHT
                return Value.Int32(_platform.DisplayHeight);
        }
        return Value.Null();
    }

    private Value ExecuteEncoderNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x0200: // ENCODER_GET_BUTTON
                return Value.Bool(_platform.EncoderGetButton());
            case 0x0201: // ENCODER_GET_DELTA
                return Value.Int32(_platform.EncoderGetDelta());
            case 0x0202: // ENCODER_GET_POSITION
                return Value.Int32(_platform.EncoderGetPosition());
            case 0x0203: // ENCODER_RESET
                _platform.EncoderReset();
                break;
        }
        return Value.Null();
    }

    private Value ExecuteSystemNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x0300: // SYSTEM_GET_TIME
                return Value.Int32((int)_platform.SystemGetTime());
            case 0x0301: // SYSTEM_SLEEP
                if (args.Length > 0)
                {
                    _platform.SystemSleep((uint)args[0].ToInt());
                    _state.Sleeping = true;
                    _state.SleepUntil = _platform.SystemGetTime() + (uint)args[0].ToInt();
                }
                break;
            case 0x0302: // SYSTEM_YIELD
                _platform.SystemYield();
                break;
            case 0x0303: // SYSTEM_GET_RTC
                return Value.Int32((int)_platform.SystemGetRTC());
        }
        return Value.Null();
    }

    private Value ExecuteTouchNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x0400: // TOUCH_GET_X
                return Value.Int32(_platform.TouchGetX());
            case 0x0401: // TOUCH_GET_Y
                return Value.Int32(_platform.TouchGetY());
            case 0x0402: // TOUCH_IS_PRESSED
                return Value.Bool(_platform.TouchIsPressed());
        }
        return Value.Null();
    }

    private Value ExecuteMemoryNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x0C00: // MEMORY_GET_AVAILABLE
                return Value.Int32(_platform.MemoryGetAvailable());
            case 0x0C01: // MEMORY_GET_USAGE
                return Value.Int32(_platform.MemoryGetUsage());
        }
        return Value.Null();
    }

    private Value ExecutePowerNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x0D00: // POWER_SLEEP
                _platform.PowerSleep();
                break;
            case 0x0D01: // POWER_GET_BATTERY_LEVEL
                return Value.Int32(_platform.PowerGetBatteryLevel());
            case 0x0D02: // POWER_IS_CHARGING
                return Value.Bool(_platform.PowerIsCharging());
        }
        return Value.Null();
    }

    private Value ExecuteAppNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x0E00: // APP_EXIT
                _platform.AppExit();
                _state.Running = false;
                break;
            case 0x0E01: // APP_GET_INFO
                return Value.String(_platform.AppGetInfo());
        }
        return Value.Null();
    }

    private Value ExecuteRfidNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x0500: // RFID_READ
                return Value.String(_platform.RfidRead());
            case 0x0501: // RFID_IS_PRESENT
                return Value.Bool(_platform.RfidIsPresent());
        }
        return Value.Null();
    }

    private Value ExecuteFileNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x0600: // FILE_OPEN
                if (args.Length >= 2)
                    return Value.Int32(_platform.FileOpen(args[0].ToString(), args[1].ToString()));
                break;
            case 0x0601: // FILE_READ
                if (args.Length >= 2)
                    return Value.String(_platform.FileRead(args[0].ToInt(), args[1].ToInt()));
                break;
            case 0x0602: // FILE_WRITE
                if (args.Length >= 2)
                    return Value.Int32(_platform.FileWrite(args[0].ToInt(), args[1].ToString()));
                break;
            case 0x0603: // FILE_CLOSE
                if (args.Length >= 1)
                    _platform.FileClose(args[0].ToInt());
                break;
            case 0x0604: // FILE_EXISTS
                if (args.Length >= 1)
                    return Value.Bool(_platform.FileExists(args[0].ToString()));
                break;
            case 0x0605: // FILE_DELETE
                if (args.Length >= 1)
                    return Value.Bool(_platform.FileDelete(args[0].ToString()));
                break;
            case 0x0606: // FILE_SIZE
                if (args.Length >= 1)
                    return Value.Int32(_platform.FileSize(args[0].ToString()));
                break;
        }
        return Value.Null();
    }

    private Value ExecuteDirectoryNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x0700: // DIR_LIST
                if (args.Length >= 1)
                {
                    var entries = _platform.DirList(args[0].ToString());
                    var arr = _pool.AllocateArray(entries.Length);
                    for (int i = 0; i < entries.Length; i++)
                    {
                        arr[i] = Value.String(_pool.InternString(entries[i]));
                    }
                    return Value.Array(arr);
                }
                break;
            case 0x0701: // DIR_CREATE
                if (args.Length >= 1)
                    return Value.Bool(_platform.DirCreate(args[0].ToString()));
                break;
            case 0x0702: // DIR_DELETE
                if (args.Length >= 1)
                    return Value.Bool(_platform.DirDelete(args[0].ToString()));
                break;
            case 0x0703: // DIR_EXISTS
                if (args.Length >= 1)
                    return Value.Bool(_platform.DirExists(args[0].ToString()));
                break;
        }
        return Value.Null();
    }

    private Value ExecuteGpioNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x0800: // GPIO_PIN_MODE
                if (args.Length >= 2)
                    _platform.GpioPinMode(args[0].ToInt(), args[1].ToInt());
                break;
            case 0x0801: // GPIO_DIGITAL_WRITE
                if (args.Length >= 2)
                    _platform.GpioDigitalWrite(args[0].ToInt(), args[1].ToInt());
                break;
            case 0x0802: // GPIO_DIGITAL_READ
                if (args.Length >= 1)
                    return Value.Int32(_platform.GpioDigitalRead(args[0].ToInt()));
                break;
            case 0x0803: // GPIO_ANALOG_WRITE
                if (args.Length >= 2)
                    _platform.GpioAnalogWrite(args[0].ToInt(), args[1].ToInt());
                break;
            case 0x0804: // GPIO_ANALOG_READ
                if (args.Length >= 1)
                    return Value.Int32(_platform.GpioAnalogRead(args[0].ToInt()));
                break;
        }
        return Value.Null();
    }

    private Value ExecuteI2cNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x0900: // I2C_SCAN
                {
                    var addresses = _platform.I2cScan();
                    var arr = _pool.AllocateArray(addresses.Length);
                    for (int i = 0; i < addresses.Length; i++)
                    {
                        arr[i] = Value.Int32(addresses[i]);
                    }
                    return Value.Array(arr);
                }
            case 0x0901: // I2C_WRITE
                // TODO: Convert Value array to byte[]
                break;
            case 0x0902: // I2C_READ
                // TODO: Convert byte[] to Value array
                break;
        }
        return Value.Null();
    }

    private Value ExecuteBuzzerNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x0A00: // BUZZER_BEEP
                if (args.Length >= 2)
                    _platform.BuzzerBeep(args[0].ToInt(), args[1].ToInt());
                break;
            case 0x0A01: // BUZZER_PLAY_MELODY
                // TODO: Convert Value array to int[]
                break;
            case 0x0A02: // BUZZER_STOP
                _platform.BuzzerStop();
                break;
        }
        return Value.Null();
    }

    private Value ExecuteWifiNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x1100: // WIFI_CONNECT
                if (args.Length >= 2)
                    return Value.Bool(_platform.WifiConnect(args[0].ToString(), args[1].ToString()));
                break;
            case 0x1101: // WIFI_DISCONNECT
                _platform.WifiDisconnect();
                break;
            case 0x1102: // WIFI_GET_STATUS
                return Value.String(_platform.WifiGetStatus());
            case 0x1103: // WIFI_GET_IP
                return Value.String(_platform.WifiGetIP());
            case 0x1104: // WIFI_SCAN
                return Value.String(_platform.WifiScan());
        }
        return Value.Null();
    }

    private Value ExecuteHttpNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x1200: // HTTP_GET
                if (args.Length >= 1)
                    return Value.String(_platform.HttpGet(args[0].ToString()));
                break;
            case 0x1201: // HTTP_POST
                if (args.Length >= 2)
                    return Value.String(_platform.HttpPost(args[0].ToString(), args[1].ToString()));
                break;
            case 0x1202: // HTTP_DOWNLOAD
                if (args.Length >= 2)
                    return Value.String(_platform.HttpDownload(args[0].ToString(), args[1].ToString()));
                break;
        }
        return Value.Null();
    }

    private Value ExecuteIpcNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x1300: // IPC_SEND
                if (args.Length >= 2)
                    return Value.Bool(_platform.IpcSend(args[0].ToString(), args[1].ToString()));
                break;
            case 0x1301: // IPC_BROADCAST
                if (args.Length >= 1)
                    _platform.IpcBroadcast(args[0].ToString());
                break;
        }
        return Value.Null();
    }

    private Value ExecuteSpiNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x1500: // SPI_OPEN
                if (args.Length >= 1)
                    return Value.Int32(_platform.SpiOpen(args[0].ToString()));
                break;
            case 0x1501: // SPI_TRANSFER
                if (args.Length >= 3)
                {
                    var tx = System.Text.Encoding.UTF8.GetBytes(args[1].ToString());
                    return Value.String(_platform.SpiTransfer(args[0].ToInt(), tx, args[2].ToInt()));
                }
                break;
            case 0x1502: // SPI_WRITE
                if (args.Length >= 2)
                {
                    var tx = System.Text.Encoding.UTF8.GetBytes(args[1].ToString());
                    return Value.Int32(_platform.SpiWrite(args[0].ToInt(), tx));
                }
                break;
            case 0x1503: // SPI_CLOSE
                if (args.Length >= 1)
                    return Value.Bool(_platform.SpiClose(args[0].ToInt()));
                break;
        }
        return Value.Null();
    }

    private Value ExecuteDeviceNative(ushort nativeId, Value[] args)
    {
        switch (nativeId)
        {
            case 0x1600: // DEVICE_LIST
                return Value.String(_platform.DeviceList());
            case 0x1601: // DEVICE_OPEN
                if (args.Length >= 1)
                    return Value.Int32(_platform.DeviceOpen(args[0].ToString(), 0));
                break;
            case 0x1602: // DEVICE_CLOSE
                if (args.Length >= 1)
                    return Value.Bool(_platform.DeviceClose(args[0].ToInt(), 0));
                break;
            case 0x1603: // DEVICE_GETINFO
                if (args.Length >= 1)
                    return Value.String(_platform.DeviceGetInfo(args[0].ToInt()));
                break;
            case 0x1604: // DEVICE_PROBE
                return Value.String(_platform.DeviceProbe());
        }
        return Value.Null();
    }

    #endregion
}
