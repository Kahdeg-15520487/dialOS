namespace DialOS.Runtime.Values;

/// <summary>
/// Types of values in the dialScript VM.
/// </summary>
public enum ValueType : byte
{
    /// <summary>
    /// Null value.
    /// </summary>
    Null,

    /// <summary>
    /// Boolean value.
    /// </summary>
    Bool,

    /// <summary>
    /// 32-bit signed integer.
    /// </summary>
    Int32,

    /// <summary>
    /// 32-bit floating point.
    /// </summary>
    Float32,

    /// <summary>
    /// String (reference to pooled string).
    /// </summary>
    String,

    /// <summary>
    /// Object (key-value map).
    /// </summary>
    Object,

    /// <summary>
    /// Array (dynamic array of values).
    /// </summary>
    Array,

    /// <summary>
    /// Function reference.
    /// </summary>
    Function,

    /// <summary>
    /// Native function pointer.
    /// </summary>
    NativeFn,
}
