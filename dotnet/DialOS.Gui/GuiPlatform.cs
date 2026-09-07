using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using DialOS.Runtime.Platform;
using DialOS.Runtime.Values;
using DialOS.Runtime.VM;

namespace DialOS.Gui;

/// <summary>
/// GUI platform implementation for the dialOS emulator.
/// Renders to a bitmap and handles input from mouse/keyboard.
/// </summary>
public class GuiPlatform : PlatformBase
{
    // Display constants
    public const int DisplayWidth = 240;
    public const int DisplayHeight = 240;
    public const int Scale = 3;

    // Display state
    private Bitmap _displayBuffer;
    private Graphics _displayGraphics;
    private uint _backgroundColor = 0x000000; // Black
    private int _brightness = 255;
    private string _title = "dialOS";

    // Input state
    private bool _touchPressed;
    private int _touchX;
    private int _touchY;
    private int _encoderPosition;
    private int _encoderDelta;
    private bool _encoderButton;
    private bool _encoderButtonPrev;

    // Simulated hardware state
    private bool _rfidPresent;
    private string _rfidData = "";
    private bool _buzzerActive;
    private int _buzzerFrequency;
    private int _batteryLevel = 75;
    private bool _isCharging;

    // Timing
    private readonly Stopwatch _stopwatch;
    private uint _rtcOffset;

    // RAMFS simulation
    private readonly Dictionary<string, byte[]> _ramfs = new();
    private readonly Dictionary<int, (string path, int position)> _fileHandles = new();
    private int _nextFileHandle = 1;

    // GPIO simulation
    private readonly Dictionary<int, int> _gpioModes = new();
    private readonly Dictionary<int, int> _gpioValues = new();

    // Events
    public event Action<Bitmap>? DisplayUpdated;
    public event Action<string>? ConsoleOutput;
    public event Action<string>? BuzzerPlayed;
    public event Action? ExitRequested;

    public GuiPlatform()
    {
        _stopwatch = Stopwatch.StartNew();

        // Initialize display buffer
        _displayBuffer = new Bitmap(DisplayWidth, DisplayHeight);
        _displayGraphics = Graphics.FromImage(_displayBuffer);
        _displayGraphics.SmoothingMode = SmoothingMode.AntiAlias;
        _displayGraphics.TextRenderingHint = TextRenderingHint.AntiAlias;

        // Clear display to black
        ClearDisplay();
    }

    public Bitmap DisplayBuffer => _displayBuffer;

    #region Input Methods (called from GUI)

    public void SetTouchState(bool pressed, int x, int y)
    {
        // Convert from scaled coordinates to display coordinates
        _touchX = Math.Clamp(x / Scale, 0, DisplayWidth - 1);
        _touchY = Math.Clamp(y / Scale, 0, DisplayHeight - 1);
        _touchPressed = pressed;
    }

    public void AddEncoderDelta(int delta)
    {
        _encoderDelta += delta;
        _encoderPosition += delta;
    }

    public void SetEncoderButton(bool pressed)
    {
        _encoderButtonPrev = _encoderButton;
        _encoderButton = pressed;
    }

    public void SimulateRfid(string data)
    {
        _rfidPresent = true;
        _rfidData = data;
    }

    public void ClearRfid()
    {
        _rfidPresent = false;
        _rfidData = "";
    }

    public void SetBatteryLevel(int level)
    {
        _batteryLevel = Math.Clamp(level, 0, 100);
    }

    public void SetCharging(bool charging)
    {
        _isCharging = charging;
    }

    /// <summary>
    /// Consume encoder delta (called by platform to reset after reading).
    /// </summary>
    public int ConsumeEncoderDelta()
    {
        var delta = _encoderDelta;
        _encoderDelta = 0;
        return delta;
    }

    /// <summary>
    /// Check if encoder button was just pressed (edge detection).
    /// </summary>
    public bool WasEncoderButtonPressed()
    {
        return _encoderButton && !_encoderButtonPrev;
    }

    #endregion

    #region Display Operations

    private void ClearDisplay()
    {
        var color = ColorFromRgb565(_backgroundColor);
        _displayGraphics.Clear(color);
    }

    private void InvalidateDisplay()
    {
        DisplayUpdated?.Invoke(_displayBuffer);
    }

    private Color ColorFromRgb565(uint rgb565)
    {
        // Apply brightness
        var brightFactor = _brightness / 255.0f;

        // Extract RGB components from RGB565
        int r = (int)(((rgb565 >> 11) & 0x1F) * 255 / 31 * brightFactor);
        int g = (int)(((rgb565 >> 5) & 0x3F) * 255 / 63 * brightFactor);
        int b = (int)((rgb565 & 0x1F) * 255 / 31 * brightFactor);

        return Color.FromArgb(r, g, b);
    }

    public override void DisplayClear(uint color)
    {
        _backgroundColor = color;
        ClearDisplay();
        InvalidateDisplay();
    }

    public override void DisplayDrawText(int x, int y, string text, uint color, int size)
    {
        var drawColor = ColorFromRgb565(color);
        using var font = new Font("Consolas", size * 8, FontStyle.Regular, GraphicsUnit.Pixel);
        using var brush = new SolidBrush(drawColor);
        _displayGraphics.DrawString(text, font, brush, x, y);
        InvalidateDisplay();
    }

    public override void DisplayDrawRect(int x, int y, int w, int h, uint color, bool filled)
    {
        var drawColor = ColorFromRgb565(color);
        if (filled)
        {
            using var brush = new SolidBrush(drawColor);
            _displayGraphics.FillRectangle(brush, x, y, w, h);
        }
        else
        {
            using var pen = new Pen(drawColor);
            _displayGraphics.DrawRectangle(pen, x, y, w, h);
        }
        InvalidateDisplay();
    }

    public override void DisplayDrawCircle(int x, int y, int r, uint color, bool filled)
    {
        var drawColor = ColorFromRgb565(color);
        if (filled)
        {
            using var brush = new SolidBrush(drawColor);
            _displayGraphics.FillEllipse(brush, x - r, y - r, r * 2, r * 2);
        }
        else
        {
            using var pen = new Pen(drawColor);
            _displayGraphics.DrawEllipse(pen, x - r, y - r, r * 2, r * 2);
        }
        InvalidateDisplay();
    }

    public override void DisplayDrawLine(int x1, int y1, int x2, int y2, uint color)
    {
        var drawColor = ColorFromRgb565(color);
        using var pen = new Pen(drawColor);
        _displayGraphics.DrawLine(pen, x1, y1, x2, y2);
        InvalidateDisplay();
    }

    public override void DisplayDrawPixel(int x, int y, uint color)
    {
        if (x >= 0 && x < DisplayWidth && y >= 0 && y < DisplayHeight)
        {
            _displayBuffer.SetPixel(x, y, ColorFromRgb565(color));
            InvalidateDisplay();
        }
    }

    public override void DisplaySetBrightness(int level)
    {
        _brightness = Math.Clamp(level, 0, 255);
    }

    public override int DisplayWidth => GuiPlatform.DisplayWidth;
    public override int DisplayHeight => GuiPlatform.DisplayHeight;

    public override void DisplaySetTitle(string title)
    {
        _title = title;
    }

    public override void DisplayDrawImage(int x, int y, byte[] imageData)
    {
        try
        {
            using var ms = new MemoryStream(imageData);
            using var image = Image.FromStream(ms);
            _displayGraphics.DrawImage(image, x, y);
            InvalidateDisplay();
        }
        catch
        {
            // Ignore invalid image data
        }
    }

    #endregion

    #region Console Operations

    public override void ConsolePrint(string message)
    {
        ConsoleOutput?.Invoke(message);
    }

    #endregion

    #region Encoder Operations

    public override bool EncoderGetButton()
    {
        return _encoderButton;
    }

    public override int EncoderGetDelta()
    {
        return ConsumeEncoderDelta();
    }

    public override int EncoderGetPosition()
    {
        return _encoderPosition;
    }

    public override void EncoderReset()
    {
        _encoderPosition = 0;
        _encoderDelta = 0;
    }

    #endregion

    #region System Operations

    public override uint SystemGetTime()
    {
        return (uint)_stopwatch.ElapsedMilliseconds;
    }

    public override void SystemSleep(uint ms)
    {
        // Sleep is handled by the VM - we just yield here
        Thread.Yield();
    }

    public override void SystemYield()
    {
        Thread.Yield();
    }

    public override uint SystemGetRTC()
    {
        return (uint)(DateTimeOffset.UtcNow.ToUnixTimeSeconds() + _rtcOffset);
    }

    public override void SystemSetRTC(uint timestamp)
    {
        _rtcOffset = (uint)(timestamp - DateTimeOffset.UtcNow.ToUnixTimeSeconds());
    }

    #endregion

    #region Touch Operations

    public override int TouchGetX()
    {
        return _touchX;
    }

    public override int TouchGetY()
    {
        return _touchY;
    }

    public override bool TouchIsPressed()
    {
        return _touchPressed;
    }

    #endregion

    #region RFID Operations

    public override string RfidRead()
    {
        return _rfidData;
    }

    public override bool RfidIsPresent()
    {
        return _rfidPresent;
    }

    #endregion

    #region File Operations (RAMFS)

    public override int FileOpen(string path, string mode)
    {
        var handle = _nextFileHandle++;

        if (mode == "r" || mode == "rb")
        {
            if (!_ramfs.ContainsKey(path))
                return -1;
            _fileHandles[handle] = (path, 0);
        }
        else if (mode == "w" || mode == "wb")
        {
            _ramfs[path] = Array.Empty<byte>();
            _fileHandles[handle] = (path, 0);
        }
        else if (mode == "a" || mode == "ab")
        {
            if (!_ramfs.ContainsKey(path))
                _ramfs[path] = Array.Empty<byte>();
            _fileHandles[handle] = (path, _ramfs[path].Length);
        }
        else
        {
            return -1;
        }

        return handle;
    }

    public override string FileRead(int handle, int size)
    {
        if (!_fileHandles.TryGetValue(handle, out var file))
            return "";

        if (!_ramfs.TryGetValue(file.path, out var data))
            return "";

        var remaining = Math.Min(size, data.Length - file.position);
        var result = System.Text.Encoding.UTF8.GetString(data, file.position, remaining);
        _fileHandles[handle] = (file.path, file.position + remaining);

        return result;
    }

    public override int FileWrite(int handle, string data)
    {
        if (!_fileHandles.TryGetValue(handle, out var file))
            return -1;

        var bytes = System.Text.Encoding.UTF8.GetBytes(data);
        var existing = _ramfs.TryGetValue(file.path, out var e) ? e : Array.Empty<byte>();

        // Extend or create new array
        var newData = new byte[file.position + bytes.Length];
        Array.Copy(existing, newData, Math.Min(file.position, existing.Length));
        Array.Copy(bytes, 0, newData, file.position, bytes.Length);

        _ramfs[file.path] = newData;
        _fileHandles[handle] = (file.path, file.position + bytes.Length);

        return bytes.Length;
    }

    public override void FileClose(int handle)
    {
        _fileHandles.Remove(handle);
    }

    public override bool FileExists(string path)
    {
        return _ramfs.ContainsKey(path);
    }

    public override bool FileDelete(string path)
    {
        return _ramfs.Remove(path);
    }

    public override int FileSize(string path)
    {
        return _ramfs.TryGetValue(path, out var data) ? data.Length : -1;
    }

    #endregion

    #region Directory Operations

    public override string[] DirList(string path)
    {
        var prefix = path.EndsWith("/") ? path : path + "/";
        return _ramfs.Keys
            .Where(k => k.StartsWith(prefix))
            .Select(k => k.Substring(prefix.Length).Split('/')[0])
            .Distinct()
            .ToArray();
    }

    public override bool DirCreate(string path)
    {
        // RAMFS is flat, directories are implicit
        return true;
    }

    public override bool DirDelete(string path)
    {
        // Remove all files in directory
        var prefix = path.EndsWith("/") ? path : path + "/";
        var keysToRemove = _ramfs.Keys.Where(k => k.StartsWith(prefix)).ToList();
        foreach (var key in keysToRemove)
            _ramfs.Remove(key);
        return true;
    }

    public override bool DirExists(string path)
    {
        var prefix = path.EndsWith("/") ? path : path + "/";
        return _ramfs.Keys.Any(k => k.StartsWith(prefix));
    }

    #endregion

    #region GPIO Operations

    public override void GpioPinMode(int pin, int mode)
    {
        _gpioModes[pin] = mode;
    }

    public override void GpioDigitalWrite(int pin, int value)
    {
        _gpioValues[pin] = value;
    }

    public override int GpioDigitalRead(int pin)
    {
        return _gpioValues.TryGetValue(pin, out var value) ? value : 0;
    }

    public override void GpioAnalogWrite(int pin, int value)
    {
        _gpioValues[pin] = value;
    }

    public override int GpioAnalogRead(int pin)
    {
        // Return a simulated analog value
        return _gpioValues.TryGetValue(pin, out var value) ? value : 512;
    }

    #endregion

    #region I2C Operations

    public override int[] I2cScan()
    {
        // Simulate no I2C devices
        return Array.Empty<int>();
    }

    public override bool I2cWrite(int address, byte[] data)
    {
        return false;
    }

    public override byte[] I2cRead(int address, int length)
    {
        return Array.Empty<byte>();
    }

    #endregion

    #region Buzzer Operations

    public override void BuzzerBeep(int frequency, int duration)
    {
        _buzzerActive = true;
        _buzzerFrequency = frequency;
        BuzzerPlayed?.Invoke($"Beep: {frequency}Hz for {duration}ms");

        // Auto-stop after duration (simplified)
        Task.Run(async () =>
        {
            await Task.Delay(duration);
            _buzzerActive = false;
        });
    }

    public override void BuzzerPlayMelody(int[] notes)
    {
        BuzzerPlayed?.Invoke($"Melody: {notes.Length} notes");
    }

    public override void BuzzerStop()
    {
        _buzzerActive = false;
    }

    #endregion

    #region Timer Operations

    private int _nextTimerId = 1;
    private readonly Dictionary<int, System.Threading.Timer> _timers = new();

    public override int TimerSetTimeout(int ms)
    {
        var id = _nextTimerId++;
        return id;
    }

    public override int TimerSetInterval(Value callback, int ms)
    {
        var id = _nextTimerId++;
        return id;
    }

    public override void TimerClearTimeout(int id)
    {
        if (_timers.TryGetValue(id, out var timer))
        {
            timer.Dispose();
            _timers.Remove(id);
        }
    }

    public override void TimerClearInterval(int id)
    {
        TimerClearTimeout(id);
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

    public override int MemoryAllocate(int size)
    {
        return _nextFileHandle++; // Simple handle
    }

    public override void MemoryFree(int handle)
    {
        // No-op in managed memory
    }

    #endregion

    #region Power Operations

    public override void PowerSleep()
    {
        ExitRequested?.Invoke();
    }

    public override int PowerGetBatteryLevel()
    {
        return _batteryLevel;
    }

    public override bool PowerIsCharging()
    {
        return _isCharging;
    }

    #endregion

    #region App Operations

    public override void AppExit()
    {
        ExitRequested?.Invoke();
    }

    public override string AppGetInfo()
    {
        return "{\"name\":\"dialOS GUI Emulator\",\"version\":\"1.0.0\"}";
    }

    #endregion

    #region Storage Operations

    public override string[] StorageGetMounted()
    {
        return new[] { "ramfs" };
    }

    public override string StorageGetInfo(string device)
    {
        if (device == "ramfs")
        {
            return "{\"type\":\"ram\",\"size\":16384,\"used\":0}";
        }
        return "{}";
    }

    #endregion

    #region Sensor Operations

    public override int SensorAttach(string port, string type)
    {
        return -1;
    }

    public override string SensorRead(int handle)
    {
        return "{}";
    }

    public override void SensorDetach(int handle)
    {
    }

    #endregion

    #region WiFi Operations

    public override bool WifiConnect(string ssid, string password)
    {
        ConsoleOutput?.Invoke($"[WiFi] Simulated connect to {ssid}\n");
        return true;
    }

    public override void WifiDisconnect()
    {
        ConsoleOutput?.Invoke("[WiFi] Disconnected\n");
    }

    public override string WifiGetStatus()
    {
        return "{\"connected\":false}";
    }

    public override string WifiGetIP()
    {
        return "0.0.0.0";
    }

    public override string WifiScan()
    {
        return "[]";
    }

    #endregion

    #region HTTP Operations

    public override string HttpGet(string url)
    {
        ConsoleOutput?.Invoke($"[HTTP] GET {url}\n");
        return "{}";
    }

    public override string HttpPost(string url, string data)
    {
        ConsoleOutput?.Invoke($"[HTTP] POST {url}\n");
        return "{}";
    }

    public override string HttpDownload(string url, string filepath)
    {
        ConsoleOutput?.Invoke($"[HTTP] Download {url} -> {filepath}\n");
        return "{\"status\":\"error\",\"message\":\"Not implemented\"}";
    }

    #endregion

    #region IPC Operations

    public override bool IpcSend(string appId, string message)
    {
        return false;
    }

    public override void IpcBroadcast(string message)
    {
    }

    #endregion

    #region App Management Operations

    public override string AppInstall(string dsbFilePath, string appId)
    {
        return "{\"status\":\"error\",\"message\":\"Not supported\"}";
    }

    public override string AppUninstall(string appId)
    {
        return "{\"status\":\"error\",\"message\":\"Not supported\"}";
    }

    public override string AppList()
    {
        return "[]";
    }

    public override string AppGetMetadata(string dsbFilePath)
    {
        return "{}";
    }

    public override string AppLaunch(string appId)
    {
        return "{\"status\":\"error\",\"message\":\"Not supported\"}";
    }

    public override string AppValidate(string dsbFilePath)
    {
        return "{\"status\":\"ok\"}";
    }

    #endregion

    #region VM Integration

    public override void RegisterCallback(string eventName, Value callback)
    {
        base.RegisterCallback(eventName, callback);
    }

    public override bool InvokeCallback(string eventName, Value[] args)
    {
        // The base implementation just returns true
        // The VM will need to handle actual callback invocation
        return base.InvokeCallback(eventName, args);
    }

    #endregion

    #region Cleanup

    public void Dispose()
    {
        _displayGraphics?.Dispose();
        _displayBuffer?.Dispose();

        foreach (var timer in _timers.Values)
        {
            timer.Dispose();
        }
        _timers.Clear();
    }

    #endregion
}
