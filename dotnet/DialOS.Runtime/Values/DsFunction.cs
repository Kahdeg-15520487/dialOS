namespace DialOS.Runtime.Values;

/// <summary>
/// Function reference in dialScript.
/// Points to a function in the bytecode module.
/// </summary>
public class DsFunction
{
    /// <summary>
    /// Index into BytecodeModule.Functions.
    /// </summary>
    public ushort FunctionIndex { get; }

    /// <summary>
    /// Number of parameters the function expects.
    /// </summary>
    public byte ParamCount { get; }

    public DsFunction(ushort functionIndex, byte paramCount)
    {
        FunctionIndex = functionIndex;
        ParamCount = paramCount;
    }

    public override string ToString()
    {
        return $"[Function#{FunctionIndex}({ParamCount} params)]";
    }
}
