namespace DialOS.Runtime.Platform;

/// <summary>
/// Console-only platform implementation for testing.
/// Only console output and time functions are implemented.
/// </summary>
public class ConsolePlatform : PlatformBase
{
    private readonly System.Diagnostics.Stopwatch _stopwatch;

    public ConsolePlatform()
    {
        _stopwatch = System.Diagnostics.Stopwatch.StartNew();
    }

    #region Console Operations

    public override void ConsolePrint(string message)
    {
        Console.Write(message);
    }

    #endregion

    #region System Operations

    public override uint SystemGetTime()
    {
        return (uint)_stopwatch.ElapsedMilliseconds;
    }

    public override void SystemSleep(uint ms)
    {
        Thread.Sleep((int)ms);
    }

    public override void SystemYield()
    {
        Thread.Yield();
    }

    #endregion

    #region Memory Operations

    public override int MemoryGetAvailable()
    {
        return (int)(GC.GetGCMemoryInfo().TotalAvailableMemoryBytes / 1024);
    }

    public override int MemoryGetUsage()
    {
        return (int)(GC.GetTotalMemory(false) / 1024);
    }

    #endregion
}
