using DialOS.Runtime.Values;
using DialOS.Runtime.VM;

namespace DialOS.Runtime.Platform;

/// <summary>
/// Platform interface for native operations.
/// Implementations provide platform-specific functionality.
/// </summary>
public interface IPlatform
{
    #region Console Operations

    void ConsolePrint(string message);
    void ConsolePrintLn(string message);
    void ConsoleLog(string message);
    void ConsoleWarn(string message);
    void ConsoleError(string message);
    void ConsoleClear();

    #endregion

    #region Display Operations

    void DisplayClear(uint color);
    void DisplayDrawText(int x, int y, string text, uint color, int size);
    void DisplayDrawRect(int x, int y, int w, int h, uint color, bool filled);
    void DisplayDrawCircle(int x, int y, int r, uint color, bool filled);
    void DisplayDrawLine(int x1, int y1, int x2, int y2, uint color);
    void DisplayDrawPixel(int x, int y, uint color);
    void DisplaySetBrightness(int level);
    int DisplayWidth { get; }
    int DisplayHeight { get; }
    void DisplaySetTitle(string title);
    void DisplayDrawImage(int x, int y, byte[] imageData);

    #endregion

    #region Encoder Operations

    bool EncoderGetButton();
    int EncoderGetDelta();
    int EncoderGetPosition();
    void EncoderReset();

    #endregion

    #region System Operations

    uint SystemGetTime();
    void SystemSleep(uint ms);
    void SystemYield();
    uint SystemGetRTC();
    void SystemSetRTC(uint timestamp);

    #endregion

    #region Touch Operations

    int TouchGetX();
    int TouchGetY();
    bool TouchIsPressed();

    #endregion

    #region RFID Operations

    string RfidRead();
    bool RfidIsPresent();

    #endregion

    #region File Operations

    int FileOpen(string path, string mode);
    string FileRead(int handle, int size);
    int FileWrite(int handle, string data);
    void FileClose(int handle);
    bool FileExists(string path);
    bool FileDelete(string path);
    int FileSize(string path);

    #endregion

    #region Directory Operations

    string[] DirList(string path);
    bool DirCreate(string path);
    bool DirDelete(string path);
    bool DirExists(string path);

    #endregion

    #region GPIO Operations

    void GpioPinMode(int pin, int mode);
    void GpioDigitalWrite(int pin, int value);
    int GpioDigitalRead(int pin);
    void GpioAnalogWrite(int pin, int value);
    int GpioAnalogRead(int pin);

    #endregion

    #region I2C Operations

    int[] I2cScan();
    bool I2cWrite(int address, byte[] data);
    byte[] I2cRead(int address, int length);

    #endregion

    #region Buzzer Operations

    void BuzzerBeep(int frequency, int duration);
    void BuzzerPlayMelody(int[] notes);
    void BuzzerStop();

    #endregion

    #region Timer Operations

    int TimerSetTimeout(int ms);
    int TimerSetInterval(Value callback, int ms);
    void TimerClearTimeout(int id);
    void TimerClearInterval(int id);

    #endregion

    #region Memory Operations

    int MemoryGetAvailable();
    int MemoryGetUsage();
    int MemoryAllocate(int size);
    void MemoryFree(int handle);

    #endregion

    #region Power Operations

    void PowerSleep();
    int PowerGetBatteryLevel();
    bool PowerIsCharging();

    #endregion

    #region App Operations

    void AppExit();
    string AppGetInfo();

    #endregion

    #region Storage Operations

    string[] StorageGetMounted();
    string StorageGetInfo(string device);

    #endregion

    #region Sensor Operations

    int SensorAttach(string port, string type);
    string SensorRead(int handle);
    void SensorDetach(int handle);

    #endregion

    #region WiFi Operations

    bool WifiConnect(string ssid, string password);
    void WifiDisconnect();
    string WifiGetStatus();
    string WifiGetIP();
    string WifiScan();

    #endregion

    #region HTTP Operations

    string HttpGet(string url);
    string HttpPost(string url, string data);
    string HttpDownload(string url, string filepath);

    #endregion

    #region IPC Operations

    bool IpcSend(string appId, string message);
    void IpcBroadcast(string message);

    #endregion

    #region App Management Operations

    string AppInstall(string dsbFilePath, string appId);
    string AppUninstall(string appId);
    string AppList();
    string AppGetMetadata(string dsbFilePath);
    string AppLaunch(string appId);
    string AppValidate(string dsbFilePath);

    #endregion

    #region VM Integration

    /// <summary>
    /// Set the VM instance for callback invocation.
    /// </summary>
    void SetVM(VMState vm);

    /// <summary>
    /// Register a callback function for an event.
    /// </summary>
    void RegisterCallback(string eventName, Value callback);

    /// <summary>
    /// Get a registered callback.
    /// </summary>
    Value? GetCallback(string eventName);

    /// <summary>
    /// Invoke a callback with arguments.
    /// </summary>
    bool InvokeCallback(string eventName, Value[] args);

    #endregion
}
