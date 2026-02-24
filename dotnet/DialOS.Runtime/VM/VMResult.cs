namespace DialOS.Runtime.VM;

/// <summary>
/// Result of VM execution.
/// </summary>
public enum VMResult
{
    /// <summary>
    /// Normal execution, continue running.
    /// </summary>
    Ok,

    /// <summary>
    /// Cooperative yield (sleep/pause).
    /// </summary>
    Yield,

    /// <summary>
    /// Program completed (HALT or end of code).
    /// </summary>
    Finished,

    /// <summary>
    /// Runtime error.
    /// </summary>
    Error,

    /// <summary>
    /// Heap exhausted.
    /// </summary>
    OutOfMemory,

    /// <summary>
    /// Unhandled exception.
    /// </summary>
    Exception,
}
