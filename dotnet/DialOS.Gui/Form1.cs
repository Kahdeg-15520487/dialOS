using System.Diagnostics;
using DialOS.Runtime.Bytecode;
using DialOS.Runtime.Values;
using DialOS.Runtime.VM;

namespace DialOS.Gui;

public partial class Form1 : Form
{
    // VM components
    private WinFormsPlatform? _platform;
    private VMState? _vmState;
    private ExecutionEngine? _engine;
    private BytecodeModule? _module;

    // UI components
    private PictureBox _displayPanel = null!;
    private TextBox _consoleOutput = null!;
    private MenuStrip _menuStrip = null!;
    private StatusStrip _statusStrip = null!;
    private ToolStripStatusLabel _statusLabel = null!;
    private System.Windows.Forms.Timer _vmTimer = null!;

    // State
    private bool _vmRunning = false;
    private string? _loadedFilePath;
    private readonly Stopwatch _fpsStopwatch = new();
    private int _frameCount = 0;
    private double _currentFps = 0;

    public Form1()
    {
        InitializeComponent();
        SetupUI();
        SetupVMTimer();
    }

    private void SetupUI()
    {
        // Form settings
        Text = "dialOS Simulator";
        ClientSize = new Size(600, 400);
        MinimumSize = new Size(500, 350);
        StartPosition = FormStartPosition.CenterScreen;

        // Menu strip
        _menuStrip = new MenuStrip();
        var fileMenu = new ToolStripMenuItem("&File");
        fileMenu.DropDownItems.Add("&Open .dsb...", null, OnOpenFile);
        fileMenu.DropDownItems.Add(new ToolStripSeparator());
        fileMenu.DropDownItems.Add("&Reload", null, OnReload);
        fileMenu.DropDownItems.Add(new ToolStripSeparator());
        fileMenu.DropDownItems.Add("E&xit", null, (s, e) => Close());

        var vmMenu = new ToolStripMenuItem("&VM");
        vmMenu.DropDownItems.Add("&Start", null, OnVMStart);
        vmMenu.DropDownItems.Add("&Stop", null, OnVMStop);
        vmMenu.DropDownItems.Add("&Reset", null, OnVMReset);

        _menuStrip.Items.Add(fileMenu);
        _menuStrip.Items.Add(vmMenu);
        Controls.Add(_menuStrip);

        // Status strip
        _statusStrip = new StatusStrip();
        _statusLabel = new ToolStripStatusLabel("Ready");
        _statusStrip.Items.Add(_statusLabel);
        Controls.Add(_statusStrip);

        // Main layout
        var mainPanel = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = 1,
            Padding = new Padding(5)
        };
        mainPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 260));
        mainPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));

        // Display panel (240x240 with border)
        var displayContainer = new Panel
        {
            Width = 250,
            Height = 250,
            BorderStyle = BorderStyle.FixedSingle,
            BackColor = Color.DarkGray,
            Margin = new Padding(5)
        };

        _displayPanel = new PictureBox
        {
            Width = 240,
            Height = 240,
            Location = new Point(4, 4),
            BackColor = Color.Black,
            SizeMode = PictureBoxSizeMode.Normal
        };

        // Wire up input events
        _displayPanel.MouseMove += OnDisplayMouseMove;
        _displayPanel.MouseDown += OnDisplayMouseDown;
        _displayPanel.MouseUp += OnDisplayMouseUp;
        _displayPanel.MouseWheel += OnDisplayMouseWheel;

        displayContainer.Controls.Add(_displayPanel);
        mainPanel.Controls.Add(displayContainer, 0, 0);

        // Console output
        var consoleContainer = new Panel
        {
            Dock = DockStyle.Fill,
            Padding = new Padding(5)
        };

        var consoleLabel = new Label
        {
            Text = "Console Output:",
            Dock = DockStyle.Top,
            Height = 20
        };

        _consoleOutput = new TextBox
        {
            Multiline = true,
            ReadOnly = true,
            ScrollBars = ScrollBars.Vertical,
            Dock = DockStyle.Fill,
            Font = new Font("Consolas", 9),
            BackColor = Color.FromArgb(30, 30, 30),
            ForeColor = Color.LightGreen
        };

        consoleContainer.Controls.Add(_consoleOutput);
        consoleContainer.Controls.Add(consoleLabel);
        mainPanel.Controls.Add(consoleContainer, 1, 0);

        Controls.Add(mainPanel);

        // Make display panel focusable for mouse wheel events
        _displayPanel.TabStop = true;
    }

    private void SetupVMTimer()
    {
        _vmTimer = new System.Windows.Forms.Timer
        {
            Interval = 16 // ~60 FPS
        };
        _vmTimer.Tick += OnVMTick;
    }

    #region File Operations

    private void OnOpenFile(object? sender, EventArgs e)
    {
        using var dialog = new OpenFileDialog
        {
            Filter = "dialScript Bytecode (*.dsb)|*.dsb|All Files (*.*)|*.*",
            Title = "Open dialOS Bytecode"
        };

        if (dialog.ShowDialog() == DialogResult.OK)
        {
            LoadBytecodeFile(dialog.FileName);
        }
    }

    private void OnReload(object? sender, EventArgs e)
    {
        if (_loadedFilePath != null)
        {
            LoadBytecodeFile(_loadedFilePath);
        }
    }

    private void LoadBytecodeFile(string filePath)
    {
        try
        {
            // Stop current VM
            StopVM();

            // Load bytecode
            var data = File.ReadAllBytes(filePath);
            _module = BytecodeModule.Deserialize(data);

            _loadedFilePath = filePath;

            // Update UI
            Text = $"dialOS Simulator - {Path.GetFileName(filePath)}";
            _consoleOutput.Clear();
            AppendConsole($"Loaded: {Path.GetFileName(filePath)}\r\n");
            AppendConsole($"App: {_module.Metadata.AppName} v{_module.Metadata.AppVersion}\r\n");
            AppendConsole($"Author: {_module.Metadata.Author}\r\n");
            AppendConsole($"Heap Size: {_module.Metadata.HeapSize} bytes\r\n");
            AppendConsole($"Code Size: {_module.Code.Length} bytes\r\n");
            AppendConsole("---\r\n");

            // Initialize VM
            InitializeVM();

            // Auto-start
            StartVM();

            _statusLabel.Text = $"Loaded: {Path.GetFileName(filePath)}";
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Failed to load bytecode: {ex.Message}", "Error",
                MessageBoxButtons.OK, MessageBoxIcon.Error);
            _statusLabel.Text = "Load failed";
        }
    }

    #endregion

    #region VM Operations

    private void InitializeVM()
    {
        if (_module == null) return;

        // Dispose old platform
        _platform?.Dispose();

        // Create new platform
        _platform = new WinFormsPlatform(
            invalidateCallback: () => BeginInvoke(InvalidateDisplay),
            consoleCallback: msg => BeginInvoke(() => AppendConsole(msg))
        );

        // Set working directory to the bytecode file's directory
        if (_loadedFilePath != null)
        {
            _platform.WorkingDirectory = Path.GetDirectoryName(_loadedFilePath) ?? Environment.CurrentDirectory;
        }

        // Create VM state and engine
        var pool = new ValuePool((uint)Math.Max(8192, (int)_module.Metadata.HeapSize));
        _vmState = new VMState(_module, pool, _platform);
        _engine = new ExecutionEngine(_vmState);
    }

    private void StartVM()
    {
        if (_engine == null) return;

        _vmRunning = true;
        _fpsStopwatch.Restart();
        _frameCount = 0;
        _vmTimer.Start();

        AppendConsole("VM Started\r\n");
        _statusLabel.Text = "Running";
    }

    private void StopVM()
    {
        _vmRunning = false;
        _vmTimer.Stop();

        if (_vmState != null)
        {
            AppendConsole("VM Stopped\r\n");
            _statusLabel.Text = "Stopped";
        }
    }

    private void OnVMStart(object? sender, EventArgs e) => StartVM();
    private void OnVMStop(object? sender, EventArgs e) => StopVM();

    private void OnVMReset(object? sender, EventArgs e)
    {
        StopVM();
        InitializeVM();
        StartVM();
    }

    private void OnVMTick(object? sender, EventArgs e)
    {
        if (!_vmRunning || _engine == null || _vmState == null) return;

        try
        {
            // Execute a batch of instructions
            var result = _engine.Execute(1000); // 1000 instructions per tick

            // Update FPS counter
            _frameCount++;
            if (_fpsStopwatch.ElapsedMilliseconds >= 1000)
            {
                _currentFps = _frameCount * 1000.0 / _fpsStopwatch.ElapsedMilliseconds;
                _fpsStopwatch.Restart();
                _frameCount = 0;
            }

            // Check result
            switch (result)
            {
                case VMResult.Finished:
                    StopVM();
                    AppendConsole("Program finished\r\n");
                    break;

                case VMResult.Error:
                    StopVM();
                    AppendConsole($"VM Error: {_vmState.LastError}\r\n");
                    break;

                case VMResult.OutOfMemory:
                    StopVM();
                    AppendConsole("Out of memory!\r\n");
                    break;

                case VMResult.Exception:
                    StopVM();
                    AppendConsole($"Unhandled exception: {_vmState.ExceptionValue}\r\n");
                    break;
            }

            // Update display if dirty
            if (_platform?.IsDisplayDirty == true)
            {
                InvalidateDisplay();
            }

            // Update status
            _statusLabel.Text = $"Running | PC: {_vmState.Pc} | Stack: {_vmState.StackHeight} | FPS: {_currentFps:F0}";
        }
        catch (Exception ex)
        {
            StopVM();
            AppendConsole($"Exception: {ex.Message}\r\n");
        }
    }

    private void InvalidateDisplay()
    {
        if (_platform == null) return;

        var buffer = _platform.GetDisplayBuffer();
        _displayPanel.Image?.Dispose();
        _displayPanel.Image = buffer;
        _platform.ClearDirtyFlag();
    }

    #endregion

    #region Input Handling

    private void OnDisplayMouseMove(object? sender, MouseEventArgs e)
    {
        _platform?.HandleMouseMove(e.X, e.Y);
    }

    private void OnDisplayMouseDown(object? sender, MouseEventArgs e)
    {
        _displayPanel.Focus(); // For mouse wheel events

        if (e.Button == MouseButtons.Left)
        {
            _platform?.HandleMouseButton(true);
        }
        else if (e.Button == MouseButtons.Middle)
        {
            _platform?.HandleEncoderButton(true);
        }
    }

    private void OnDisplayMouseUp(object? sender, MouseEventArgs e)
    {
        if (e.Button == MouseButtons.Left)
        {
            _platform?.HandleMouseButton(false);
        }
        else if (e.Button == MouseButtons.Middle)
        {
            _platform?.HandleEncoderButton(false);
        }
    }

    private void OnDisplayMouseWheel(object? sender, MouseEventArgs e)
    {
        _platform?.HandleMouseWheel(e.Delta);
    }

    #endregion

    #region Console

    private void AppendConsole(string text)
    {
        if (InvokeRequired)
        {
            BeginInvoke(() => AppendConsole(text));
            return;
        }

        _consoleOutput.AppendText(text);
    }

    #endregion

    #region Cleanup

    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        StopVM();
        _platform?.Dispose();
        base.OnFormClosing(e);
    }

    #endregion
}
