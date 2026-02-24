namespace DialOS.Runtime.VM;

/// <summary>
/// Represents an exception handler (try/catch block).
/// </summary>
public class ExceptionHandler
{
    /// <summary>
    /// PC where the catch block starts.
    /// </summary>
    public int CatchPc { get; }

    /// <summary>
    /// Stack height when the try block was entered.
    /// </summary>
    public int StackHeight { get; }

    /// <summary>
    /// Call frame index when the try block was entered.
    /// </summary>
    public int FrameIndex { get; }

    public ExceptionHandler(int catchPc, int stackHeight, int frameIndex)
    {
        CatchPc = catchPc;
        StackHeight = stackHeight;
        FrameIndex = frameIndex;
    }
}
