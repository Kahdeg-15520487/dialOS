namespace DialOS.Runtime.VM;

/// <summary>
/// Represents a function call frame on the call stack.
/// </summary>
public class CallFrame
{
    /// <summary>
    /// Return address (PC to resume after function returns).
    /// </summary>
    public int ReturnPc { get; set; }

    /// <summary>
    /// Stack base index (where locals start).
    /// </summary>
    public int StackBase { get; set; }

    /// <summary>
    /// Local variables (indexed by byte).
    /// </summary>
    public Values.Value[] Locals { get; }

    /// <summary>
    /// Function name (for debugging).
    /// </summary>
    public string FunctionName { get; set; }

    /// <summary>
    /// Number of arguments passed to this function.
    /// </summary>
    public int ArgCount { get; set; }

    public CallFrame(int returnPc, int stackBase, int localCount = 256, string functionName = "main")
    {
        ReturnPc = returnPc;
        StackBase = stackBase;
        Locals = new Values.Value[localCount];
        FunctionName = functionName;
        ArgCount = 0;

        // Initialize locals to null
        for (int i = 0; i < localCount; i++)
        {
            Locals[i] = Values.Value.Null();
        }
    }

    /// <summary>
    /// Get a local variable by index.
    /// </summary>
    public Values.Value GetLocal(int index)
    {
        if (index < 0 || index >= Locals.Length)
            return Values.Value.Null();
        return Locals[index];
    }

    /// <summary>
    /// Set a local variable by index.
    /// </summary>
    public void SetLocal(int index, Values.Value value)
    {
        if (index >= 0 && index < Locals.Length)
        {
            Locals[index] = value;
        }
    }
}
