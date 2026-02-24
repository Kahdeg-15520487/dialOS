using DialOS.Runtime.Bytecode;
using DialOS.Runtime.Platform;
using DialOS.Runtime.Values;

namespace DialOS.Runtime.VM;

/// <summary>
/// VM execution state.
/// Contains the program counter, stack, call stack, and globals.
/// </summary>
public class VMState
{
    /// <summary>
    /// The loaded bytecode module.
    /// </summary>
    public BytecodeModule Module { get; }

    /// <summary>
    /// Memory pool for heap allocations.
    /// </summary>
    public ValuePool Pool { get; }

    /// <summary>
    /// Platform interface for native calls.
    /// </summary>
    public IPlatform Platform { get; }

    /// <summary>
    /// Program counter (current instruction position).
    /// </summary>
    public int Pc { get; set; }

    /// <summary>
    /// Value stack.
    /// </summary>
    public Stack<Value> Stack { get; } = new();

    /// <summary>
    /// Call stack (function frames).
    /// </summary>
    public List<CallFrame> CallStack { get; } = new();

    /// <summary>
    /// Global variables.
    /// </summary>
    public Dictionary<string, Value> Globals { get; } = new();

    /// <summary>
    /// Exception handlers (try/catch blocks).
    /// </summary>
    public Stack<ExceptionHandler> ExceptionHandlers { get; } = new();

    /// <summary>
    /// Whether the VM is running.
    /// </summary>
    public bool Running { get; set; }

    /// <summary>
    /// Whether the VM is sleeping.
    /// </summary>
    public bool Sleeping { get; set; }

    /// <summary>
    /// Time to wake up (when sleeping).
    /// </summary>
    public uint SleepUntil { get; set; }

    /// <summary>
    /// Last error message (if any).
    /// </summary>
    public string? LastError { get; set; }

    /// <summary>
    /// Current exception value (if thrown).
    /// </summary>
    public Value? ExceptionValue { get; set; }

    /// <summary>
    /// Get the current call frame.
    /// </summary>
    public CallFrame CurrentFrame => CallStack.Count > 0 ? CallStack[^1] : null!;

    /// <summary>
    /// Get the stack height.
    /// </summary>
    public int StackHeight => Stack.Count;

    public VMState(BytecodeModule module, ValuePool pool, IPlatform platform)
    {
        Module = module;
        Pool = pool;
        Platform = platform;

        // Initialize PC to main entry point
        Pc = (int)module.MainEntryPoint;

        // Initialize globals with null
        foreach (var name in module.Globals)
        {
            Globals[name] = Value.Null();
        }

        // Create initial call frame for main code
        CallStack.Add(new CallFrame(-1, 0, 256, "__main__"));

        // Set VM reference in platform for callback invocation
        Platform.SetVM(this);
    }

    /// <summary>
    /// Get a global variable by name.
    /// </summary>
    public Value GetGlobal(string name)
    {
        return Globals.TryGetValue(name, out var value) ? value : Value.Null();
    }

    /// <summary>
    /// Set a global variable by name.
    /// </summary>
    public void SetGlobal(string name, Value value)
    {
        Globals[name] = value;
    }

    /// <summary>
    /// Get a global variable by index.
    /// </summary>
    public Value GetGlobal(int index)
    {
        if (index < 0 || index >= Module.Globals.Count)
            return Value.Null();
        return GetGlobal(Module.Globals[index]);
    }

    /// <summary>
    /// Set a global variable by index.
    /// </summary>
    public void SetGlobal(int index, Value value)
    {
        if (index >= 0 && index < Module.Globals.Count)
        {
            Globals[Module.Globals[index]] = value;
        }
    }

    /// <summary>
    /// Push a value onto the stack.
    /// </summary>
    public void Push(Value value)
    {
        Stack.Push(value);
    }

    /// <summary>
    /// Pop a value from the stack.
    /// </summary>
    public Value Pop()
    {
        return Stack.Count > 0 ? Stack.Pop() : Value.Null();
    }

    /// <summary>
    /// Peek at the top of the stack without popping.
    /// </summary>
    public Value Peek()
    {
        return Stack.Count > 0 ? Stack.Peek() : Value.Null();
    }

    /// <summary>
    /// Get current source line (if debug info available).
    /// </summary>
    public uint GetCurrentSourceLine()
    {
        return Module.GetSourceLine(Pc);
    }

    /// <summary>
    /// Reset the VM state for re-execution.
    /// </summary>
    public void Reset()
    {
        Pc = (int)Module.MainEntryPoint;
        Stack.Clear();
        CallStack.Clear();
        ExceptionHandlers.Clear();
        Running = false;
        Sleeping = false;
        SleepUntil = 0;
        LastError = null;
        ExceptionValue = null;

        // Reset globals
        foreach (var name in Module.Globals)
        {
            Globals[name] = Value.Null();
        }

        // Create initial call frame
        CallStack.Add(new CallFrame(-1, 0, 256, "__main__"));
    }
}
