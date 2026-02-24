using DialOS.Runtime.Values;
using DialOS.Runtime.VM;

namespace DialOS.Runtime.Platform;

/// <summary>
/// Base implementation of IPlatform with default/stub implementations.
/// Override specific methods in derived classes.
/// </summary>
public abstract class PlatformBase : IPlatform
{
    protected VMState? VM;
    protected readonly Dictionary<string, Value> Callbacks = new();

    #region Console Operations

    public virtual void ConsolePrint(string message) { }
    public virtual void ConsolePrintLn(string message) => ConsolePrint(message + "\n");
    public virtual void ConsoleLog(string message) => ConsolePrint("[INFO] " + message + "\n");
    public virtual void ConsoleWarn(string message) => ConsolePrint("[WARN] " + message + "\n");
    public virtual void ConsoleError(string message) => ConsolePrint("[ERROR] " + message + "\n");
    public virtual void ConsoleClear() { }

    #endregion

    #region Display Operations

    public virtual void DisplayClear(uint color) { }
    public virtual void DisplayDrawText(int x, int y, string text, uint color, int size) { }
    public virtual void DisplayDrawRect(int x, int y, int w, int h, uint color, bool filled) { }
    public virtual void DisplayDrawCircle(int x, int y, int r, uint color, bool filled) { }
    public virtual void DisplayDrawLine(int x1, int y1, int x2, int y2, uint color) { }
    public virtual void DisplayDrawPixel(int x, int y, uint color) { }
    public virtual void DisplaySetBrightness(int level) { }

    public virtual int DisplayWidth => 240;
    public virtual int DisplayHeight => 240;
    public virtual void DisplaySetTitle(string title) { }
    public virtual void DisplayDrawImage(int x, int y, byte[] imageData) { }

    #endregion

    #region Encoder Operations

    public virtual bool EncoderGetButton() => false;
    public virtual int EncoderGetDelta() => 0;
    public virtual int EncoderGetPosition() => 0;
    public virtual void EncoderReset() { }

    #endregion

    #region System Operations

    public abstract uint SystemGetTime();
    public abstract void SystemSleep(uint ms);
    public virtual void SystemYield() { }
    public virtual uint SystemGetRTC() => 0;
    public virtual void SystemSetRTC(uint timestamp) { }

    #endregion

    #region Touch Operations

    public virtual int TouchGetX() => 0;
    public virtual int TouchGetY() => 0;
    public virtual bool TouchIsPressed() => false;

    #endregion

    #region RFID Operations

    public virtual string RfidRead() => "";
    public virtual bool RfidIsPresent() => false;

    #endregion

    #region File Operations

    public virtual int FileOpen(string path, string mode) => -1;
    public virtual string FileRead(int handle, int size) => "";
    public virtual int FileWrite(int handle, string data) => -1;
    public virtual void FileClose(int handle) { }
    public virtual bool FileExists(string path) => false;
    public virtual bool FileDelete(string path) => false;
    public virtual int FileSize(string path) => -1;

    #endregion

    #region Directory Operations

    public virtual string[] DirList(string path) => Array.Empty<string>();
    public virtual bool DirCreate(string path) => false;
    public virtual bool DirDelete(string path) => false;
    public virtual bool DirExists(string path) => false;

    #endregion

    #region GPIO Operations

    public virtual void GpioPinMode(int pin, int mode) { }
    public virtual void GpioDigitalWrite(int pin, int value) { }
    public virtual int GpioDigitalRead(int pin) => 0;
    public virtual void GpioAnalogWrite(int pin, int value) { }
    public virtual int GpioAnalogRead(int pin) => 0;

    #endregion

    #region I2C Operations

    public virtual int[] I2cScan() => Array.Empty<int>();
    public virtual bool I2cWrite(int address, byte[] data) => false;
    public virtual byte[] I2cRead(int address, int length) => Array.Empty<byte>();

    #endregion

    #region Buzzer Operations

    public virtual void BuzzerBeep(int frequency, int duration) { }
    public virtual void BuzzerPlayMelody(int[] notes) { }
    public virtual void BuzzerStop() { }

    #endregion

    #region Timer Operations

    public virtual int TimerSetTimeout(int ms) => -1;
    public virtual int TimerSetInterval(Value callback, int ms) => -1;
    public virtual void TimerClearTimeout(int id) { }
    public virtual void TimerClearInterval(int id) { }

    #endregion

    #region Memory Operations

    public virtual int MemoryGetAvailable() => 0;
    public virtual int MemoryGetUsage() => 0;
    public virtual int MemoryAllocate(int size) => -1;
    public virtual void MemoryFree(int handle) { }

    #endregion

    #region Power Operations

    public virtual void PowerSleep() { }
    public virtual int PowerGetBatteryLevel() => 100;
    public virtual bool PowerIsCharging() => false;

    #endregion

    #region App Operations

    public virtual void AppExit() { }
    public virtual string AppGetInfo() => "{}";

    #endregion

    #region Storage Operations

    public virtual string[] StorageGetMounted() => Array.Empty<string>();
    public virtual string StorageGetInfo(string device) => "{}";

    #endregion

    #region Sensor Operations

    public virtual int SensorAttach(string port, string type) => -1;
    public virtual string SensorRead(int handle) => "{}";
    public virtual void SensorDetach(int handle) { }

    #endregion

    #region WiFi Operations

    public virtual bool WifiConnect(string ssid, string password) => false;
    public virtual void WifiDisconnect() { }
    public virtual string WifiGetStatus() => "{}";
    public virtual string WifiGetIP() => "";
    public virtual string WifiScan() => "[]";

    #endregion

    #region HTTP Operations

    public virtual string HttpGet(string url) => "{}";
    public virtual string HttpPost(string url, string data) => "{}";
    public virtual string HttpDownload(string url, string filepath) => "{}";

    #endregion

    #region IPC Operations

    public virtual bool IpcSend(string appId, string message) => false;
    public virtual void IpcBroadcast(string message) { }

    #endregion

    #region App Management Operations

    public virtual string AppInstall(string dsbFilePath, string appId)
        => "{\"status\":\"error\",\"message\":\"Not supported\"}";
    public virtual string AppUninstall(string appId)
        => "{\"status\":\"error\",\"message\":\"Not supported\"}";
    public virtual string AppList() => "[]";
    public virtual string AppGetMetadata(string dsbFilePath) => "{}";
    public virtual string AppLaunch(string appId)
        => "{\"status\":\"error\",\"message\":\"Not supported\"}";
    public virtual string AppValidate(string dsbFilePath)
        => "{\"status\":\"error\",\"message\":\"Not supported\"}";

    #endregion

    #region VM Integration

    public virtual void SetVM(VMState vm) => VM = vm;

    public virtual void RegisterCallback(string eventName, Value callback)
    {
        Callbacks[eventName] = callback;
    }

    public virtual Value? GetCallback(string eventName)
    {
        return Callbacks.TryGetValue(eventName, out var callback) ? callback : null;
    }

    public virtual bool InvokeCallback(string eventName, Value[] args)
    {
        var callback = GetCallback(eventName);
        if (callback == null || !callback.Value.IsFunction)
            return false;

        // Callback invocation would be handled by the execution engine
        // This is a simplified implementation
        return true;
    }

    #endregion
}
