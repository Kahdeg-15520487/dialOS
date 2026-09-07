using System.Diagnostics;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using DialOS.Runtime.Platform;
using DialOS.Runtime.Values;
using DialOS.Runtime.VM;

namespace DialOS.Gui;

/// <summary>
/// WinForms implementation of IPlatform with GDI+ rendering.
/// Simulates M5Dial display (240x240), encoder, and touch input.
/// </summary>
public class WinFormsPlatform : PlatformBase, IDisposable
{
    // Display
    private readonly Bitmap _displayBuffer;
    private readonly Graphics _displayGraphics;
    private readonly object _displayLock = new();
    private readonly Action _invalidateCallback;
    private bool _displayDirty = true;

    // Timing
    private readonly Stopwatch _stopwatch;

    // Encoder simulation (mouse wheel)
    private int _encoderPosition = 0;
    private int _encoderDelta = 0;
    private bool _encoderButton = false;

    // Touch simulation (mouse)
    private int _touchX = 0;
    private int _touchY = 0;
    private bool _touchPressed = false;

    // Console output
    private readonly Action<string>? _consoleCallback;

    // File handles
    private readonly Dictionary<int, FileStream> _fileHandles = new();
    private int _nextFileHandle = 1;

    // Working directory for file operations
    public string WorkingDirectory { get; set; } = Environment.CurrentDirectory;

    public WinFormsPlatform(Action invalidateCallback, Action<string>? consoleCallback = null)
    {
        _invalidateCallback = invalidateCallback;
        _consoleCallback = consoleCallback;

        // Create 240x240 display buffer (M5Dial resolution)
        _displayBuffer = new Bitmap(240, 240, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
        _displayGraphics = Graphics.FromImage(_displayBuffer);
        _displayGraphics.SmoothingMode = SmoothingMode.AntiAlias;
        _displayGraphics.TextRenderingHint = TextRenderingHint.ClearTypeGridFit;

        // Clear to black initially
        _displayGraphics.Clear(Color.Black);

        _stopwatch = Stopwatch.StartNew();
    }

    #region Display Operations

    public override int DisplayWidth => 240;
    public override int DisplayHeight => 240;

    /// <summary>
    /// Get the display buffer for rendering to the form.
    /// </summary>
    public Bitmap GetDisplayBuffer()
    {
        lock (_displayLock)
        {
            return (Bitmap)_displayBuffer.Clone();
        }
    }

    /// <summary>
    /// Check if display needs redraw.
    /// </summary>
    public bool IsDisplayDirty
    {
        get
        {
            lock (_displayLock) return _displayDirty;
        }
    }

    /// <summary>
    /// Clear dirty flag after redraw.
    /// </summary>
    public void ClearDirtyFlag()
    {
        lock (_displayLock) _displayDirty = false;
    }

    private void MarkDirty()
    {
        lock (_displayLock) _displayDirty = true;
        _invalidateCallback?.Invoke();
    }

    /// <summary>
    /// Convert RGB565 color to System.Drawing.Color
    /// </summary>
    private static Color FromRgb565(uint rgb565)
    {
        int r = (int)((rgb565 >> 11) & 0x1F) * 255 / 31;
        int g = (int)((rgb565 >> 5) & 0x3F) * 255 / 63;
        int b = (int)(rgb565 & 0x1F) * 255 / 31;
        return Color.FromArgb(255, r, g, b);
    }

    public override void DisplayClear(uint color)
    {
        lock (_displayLock)
        {
            _displayGraphics.Clear(FromRgb565(color));
        }
        MarkDirty();
    }

    public override void DisplayDrawText(int x, int y, string text, uint color, int size)
    {
        lock (_displayLock)
        {
            // Map size to font size (approximate M5Dial font scaling)
            float fontSize = size switch
            {
                1 => 8f,
                2 => 12f,
                3 => 16f,
                4 => 20f,
                _ => size * 4f
            };

            using var font = new Font("Consolas", fontSize, FontStyle.Regular, GraphicsUnit.Pixel);
            using var brush = new SolidBrush(FromRgb565(color));
            _displayGraphics.DrawString(text, font, brush, x, y);
        }
        MarkDirty();
    }

    public override void DisplayDrawRect(int x, int y, int w, int h, uint color, bool filled)
    {
        lock (_displayLock)
        {
            var drawColor = FromRgb565(color);
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
        }
        MarkDirty();
    }

    public override void DisplayDrawCircle(int x, int y, int r, uint color, bool filled)
    {
        lock (_displayLock)
        {
            var drawColor = FromRgb565(color);
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
        }
        MarkDirty();
    }

    public override void DisplayDrawLine(int x1, int y1, int x2, int y2, uint color)
    {
        lock (_displayLock)
        {
            using var pen = new Pen(FromRgb565(color));
            _displayGraphics.DrawLine(pen, x1, y1, x2, y2);
        }
        MarkDirty();
    }

    public override void DisplayDrawPixel(int x, int y, uint color)
    {
        lock (_displayLock)
        {
            if (x >= 0 && x < 240 && y >= 0 && y < 240)
            {
                _displayBuffer.SetPixel(x, y, FromRgb565(color));
            }
        }
        MarkDirty();
    }

    public override void DisplaySetTitle(string title)
    {
        // This could be used to update the form title
        _consoleCallback?.Invoke($"[Title] {title}");
    }

    #endregion

    #region Encoder Operations (Mouse Wheel Simulation)

    /// <summary>
    /// Call this from Form.MouseWheel event.
    /// </summary>
    public void HandleMouseWheel(int delta)
    {
        int ticks = delta / 120; // Standard wheel delta is 120 per tick
        _encoderDelta += ticks;
        _encoderPosition += ticks;
    }

    /// <summary>
    /// Call this from Form.MouseDown/MouseUp for middle button.
    /// </summary>
    public void HandleEncoderButton(bool pressed)
    {
        _encoderButton = pressed;
    }

    public override bool EncoderGetButton() => _encoderButton;

    public override int EncoderGetDelta()
    {
        int delta = _encoderDelta;
        _encoderDelta = 0; // Reset after reading
        return delta;
    }

    public override int EncoderGetPosition() => _encoderPosition;

    public override void EncoderReset()
    {
        _encoderPosition = 0;
        _encoderDelta = 0;
    }

    #endregion

    #region Touch Operations (Mouse Simulation)

    /// <summary>
    /// Call this from Form.MouseMove event.
    /// </summary>
    public void HandleMouseMove(int x, int y)
    {
        _touchX = Math.Clamp(x, 0, 239);
        _touchY = Math.Clamp(y, 0, 239);
    }

    /// <summary>
    /// Call this from Form.MouseDown/MouseUp for left button.
    /// </summary>
    public void HandleMouseButton(bool pressed)
    {
        _touchPressed = pressed;
    }

    public override int TouchGetX() => _touchX;
    public override int TouchGetY() => _touchY;
    public override bool TouchIsPressed() => _touchPressed;

    #endregion

    #region System Operations

    public override uint SystemGetTime() => (uint)_stopwatch.ElapsedMilliseconds;

    public override void SystemSleep(uint ms)
    {
        // In WinForms, we don't actually block - the VM handles sleep state
    }

    public override void SystemYield()
    {
        // Allow UI to process events
        Application.DoEvents();
    }

    public override uint SystemGetRTC()
    {
        return (uint)DateTimeOffset.UtcNow.ToUnixTimeSeconds();
    }

    #endregion

    #region Console Operations

    public override void ConsolePrint(string message)
    {
        _consoleCallback?.Invoke(message);
    }

    public override void ConsolePrintLn(string message)
    {
        _consoleCallback?.Invoke(message + Environment.NewLine);
    }

    public override void ConsoleLog(string message)
    {
        _consoleCallback?.Invoke($"[INFO] {message}{Environment.NewLine}");
    }

    public override void ConsoleWarn(string message)
    {
        _consoleCallback?.Invoke($"[WARN] {message}{Environment.NewLine}");
    }

    public override void ConsoleError(string message)
    {
        _consoleCallback?.Invoke($"[ERROR] {message}{Environment.NewLine}");
    }

    #endregion

    #region File Operations

    private string ResolvePath(string path)
    {
        if (Path.IsPathRooted(path))
            return path;
        return Path.Combine(WorkingDirectory, path);
    }

    public override int FileOpen(string path, string mode)
    {
        try
        {
            var fullPath = ResolvePath(path);
            FileMode fileMode;
            FileAccess fileAccess;

            switch (mode.ToLower())
            {
                case "r":
                    fileMode = FileMode.Open;
                    fileAccess = FileAccess.Read;
                    break;
                case "w":
                    fileMode = FileMode.Create;
                    fileAccess = FileAccess.Write;
                    break;
                case "a":
                    fileMode = FileMode.Append;
                    fileAccess = FileAccess.Write;
                    break;
                case "rw":
                case "r+":
                    fileMode = FileMode.OpenOrCreate;
                    fileAccess = FileAccess.ReadWrite;
                    break;
                default:
                    return -1;
            }

            var stream = new FileStream(fullPath, fileMode, fileAccess);
            int handle = _nextFileHandle++;
            _fileHandles[handle] = stream;
            return handle;
        }
        catch
        {
            return -1;
        }
    }

    public override string FileRead(int handle, int size)
    {
        try
        {
            if (!_fileHandles.TryGetValue(handle, out var stream))
                return "";

            var buffer = new byte[size];
            int bytesRead = stream.Read(buffer, 0, size);
            return System.Text.Encoding.UTF8.GetString(buffer, 0, bytesRead);
        }
        catch
        {
            return "";
        }
    }

    public override int FileWrite(int handle, string data)
    {
        try
        {
            if (!_fileHandles.TryGetValue(handle, out var stream))
                return -1;

            var bytes = System.Text.Encoding.UTF8.GetBytes(data);
            stream.Write(bytes, 0, bytes.Length);
            return bytes.Length;
        }
        catch
        {
            return -1;
        }
    }

    public override void FileClose(int handle)
    {
        if (_fileHandles.TryGetValue(handle, out var stream))
        {
            stream.Dispose();
            _fileHandles.Remove(handle);
        }
    }

    public override bool FileExists(string path)
    {
        return File.Exists(ResolvePath(path));
    }

    public override bool FileDelete(string path)
    {
        try
        {
            File.Delete(ResolvePath(path));
            return true;
        }
        catch
        {
            return false;
        }
    }

    public override int FileSize(string path)
    {
        try
        {
            var info = new FileInfo(ResolvePath(path));
            return (int)info.Length;
        }
        catch
        {
            return -1;
        }
    }

    #endregion

    #region Directory Operations

    public override string[] DirList(string path)
    {
        try
        {
            var fullPath = ResolvePath(path);
            var entries = new List<string>();

            foreach (var dir in Directory.GetDirectories(fullPath))
                entries.Add(Path.GetFileName(dir) + "/");

            foreach (var file in Directory.GetFiles(fullPath))
                entries.Add(Path.GetFileName(file));

            return entries.ToArray();
        }
        catch
        {
            return Array.Empty<string>();
        }
    }

    public override bool DirCreate(string path)
    {
        try
        {
            Directory.CreateDirectory(ResolvePath(path));
            return true;
        }
        catch
        {
            return false;
        }
    }

    public override bool DirDelete(string path)
    {
        try
        {
            Directory.Delete(ResolvePath(path), recursive: true);
            return true;
        }
        catch
        {
            return false;
        }
    }

    public override bool DirExists(string path)
    {
        return Directory.Exists(ResolvePath(path));
    }

    #endregion

    #region Memory Operations

    public override int MemoryGetAvailable()
    {
        // Return approximate available memory
        return (int)(GC.GetTotalMemory(false) / 1024);
    }

    public override int MemoryGetUsage()
    {
        return (int)(GC.GetTotalMemory(false) / 1024);
    }

    #endregion

    #region HTTP Operations

    private static readonly HttpClient _httpClient = new();

    public override string HttpGet(string url)
    {
        try
        {
            return _httpClient.GetStringAsync(url).GetAwaiter().GetResult();
        }
        catch (Exception ex)
        {
            return $"{{\"error\":\"{ex.Message}\"}}";
        }
    }

    public override string HttpPost(string url, string data)
    {
        try
        {
            var content = new StringContent(data, System.Text.Encoding.UTF8, "application/json");
            var response = _httpClient.PostAsync(url, content).GetAwaiter().GetResult();
            return response.Content.ReadAsStringAsync().GetAwaiter().GetResult();
        }
        catch (Exception ex)
        {
            return $"{{\"error\":\"{ex.Message}\"}}";
        }
    }

    public override string HttpDownload(string url, string filepath)
    {
        try
        {
            var fullPath = ResolvePath(filepath);
            var data = _httpClient.GetByteArrayAsync(url).GetAwaiter().GetResult();
            File.WriteAllBytes(fullPath, data);
            return $"{{\"status\":\"success\",\"size\":{data.Length}}}";
        }
        catch (Exception ex)
        {
            return $"{{\"status\":\"error\",\"message\":\"{ex.Message}\"}}";
        }
    }

    #endregion

    #region IDisposable

    private bool _disposed = false;

    public void Dispose()
    {
        if (!_disposed)
        {
            _displayGraphics.Dispose();
            _displayBuffer.Dispose();

            foreach (var handle in _fileHandles.Values)
            {
                handle.Dispose();
            }
            _fileHandles.Clear();

            _disposed = true;
        }
        GC.SuppressFinalize(this);
    }

    #endregion
}
