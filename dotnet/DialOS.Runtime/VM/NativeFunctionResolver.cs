using System.Collections.Generic;

namespace DialOS.Runtime.VM;

/// <summary>
/// Maps full native function names (as stored in the bytecode function table,
/// e.g. "console.println") to native function IDs.
///
/// IMPORTANT: This mirrors the C++ reference implementation
/// (include/vm/platform.h getNativeFunctionID). CALL_NATIVE instructions store a
/// function-table INDEX whose entry names the native function; the VM must resolve
/// name → ID at runtime. Dispatching on the raw operand as an ID is incorrect.
/// Keep this table in lockstep with the C++ map.
/// </summary>
public static class NativeFunctionResolver
{
    private static readonly Dictionary<string, ushort> Map = new()
    {
        // Console
        ["console.print"] = 0x0000,
        ["console.println"] = 0x0001,
        ["console.log"] = 0x0002,
        ["console.warn"] = 0x0003,
        ["console.error"] = 0x0004,
        ["console.clear"] = 0x0005,
        // Display
        ["display.clear"] = 0x0100,
        ["display.drawText"] = 0x0101,
        ["display.drawRect"] = 0x0102,
        ["display.drawCircle"] = 0x0103,
        ["display.drawLine"] = 0x0104,
        ["display.drawPixel"] = 0x0105,
        ["display.setBrightness"] = 0x0106,
        ["display.getWidth"] = 0x0107,
        ["display.getHeight"] = 0x0108,
        ["display.setTitle"] = 0x0109,
        ["display.getSize"] = 0x010A,
        ["display.drawImage"] = 0x010B,
        // Encoder
        ["encoder.getButton"] = 0x0200,
        ["encoder.getDelta"] = 0x0201,
        ["encoder.getPosition"] = 0x0202,
        ["encoder.reset"] = 0x0203,
        ["encoder.onTurn"] = 0x0204,
        ["encoder.onButton"] = 0x0205,
        // System
        ["system.getTime"] = 0x0300,
        ["system.sleep"] = 0x0301,
        ["system.yield"] = 0x0302,
        ["system.getRTC"] = 0x0303,
        ["system.setRTC"] = 0x0304,
        // Touch
        ["touch.getX"] = 0x0400,
        ["touch.getY"] = 0x0401,
        ["touch.isPressed"] = 0x0402,
        ["touch.getPosition"] = 0x0403,
        ["touch.onPress"] = 0x0404,
        ["touch.onRelease"] = 0x0405,
        ["touch.onDrag"] = 0x0406,
        // RFID
        ["rfid.read"] = 0x0500,
        ["rfid.isPresent"] = 0x0501,
        // File
        ["file.open"] = 0x0600,
        ["file.read"] = 0x0601,
        ["file.write"] = 0x0602,
        ["file.close"] = 0x0603,
        ["file.exists"] = 0x0604,
        ["file.delete"] = 0x0605,
        ["file.size"] = 0x0606,
        // Directory
        ["dir.list"] = 0x0700,
        ["dir.create"] = 0x0701,
        ["dir.delete"] = 0x0702,
        ["dir.exists"] = 0x0703,
        // GPIO
        ["gpio.pinMode"] = 0x0800,
        ["gpio.digitalWrite"] = 0x0801,
        ["gpio.digitalRead"] = 0x0802,
        ["gpio.analogWrite"] = 0x0803,
        ["gpio.analogRead"] = 0x0804,
        // I2C
        ["i2c.scan"] = 0x0900,
        ["i2c.write"] = 0x0901,
        ["i2c.read"] = 0x0902,
        // Buzzer
        ["buzzer.beep"] = 0x0A00,
        ["buzzer.playMelody"] = 0x0A01,
        ["buzzer.stop"] = 0x0A02,
        // Timer
        ["timer.setTimeout"] = 0x0B00,
        ["timer.setInterval"] = 0x0B01,
        ["timer.clearTimeout"] = 0x0B02,
        ["timer.clearInterval"] = 0x0B03,
        // Memory
        ["memory.getAvailable"] = 0x0C00,
        ["os.memory.getAvailable"] = 0x0C00,
        ["memory.getUsage"] = 0x0C01,
        ["os.memory.getUsage"] = 0x0C01,
        ["memory.allocate"] = 0x0C02,
        ["os.memory.allocate"] = 0x0C02,
        ["memory.free"] = 0x0C03,
        ["os.memory.free"] = 0x0C03,
        // Power
        ["power.sleep"] = 0x0D00,
        ["power.getBatteryLevel"] = 0x0D01,
        ["power.isCharging"] = 0x0D02,
        // App
        ["app.exit"] = 0x0E00,
        ["app.getInfo"] = 0x0E01,
        ["app.onLoad"] = 0x0E02,
        ["app.onSuspend"] = 0x0E03,
        ["app.onResume"] = 0x0E04,
        ["app.onUnload"] = 0x0E05,
        // Storage
        ["storage.getMounted"] = 0x0F00,
        ["storage.getInfo"] = 0x0F01,
        // Sensor
        ["sensor.attach"] = 0x1000,
        ["sensor.read"] = 0x1001,
        ["sensor.detach"] = 0x1002,
        // WiFi
        ["wifi.connect"] = 0x1100,
        ["wifi.disconnect"] = 0x1101,
        ["wifi.getStatus"] = 0x1102,
        ["wifi.getIP"] = 0x1103,
        ["wifi.scan"] = 0x1104,
        // HTTP
        ["http.get"] = 0x1200,
        ["http.post"] = 0x1201,
        ["http.download"] = 0x1202,
        // IPC
        ["ipc.send"] = 0x1300,
        ["ipc.broadcast"] = 0x1301,
        // App management
        ["app.install"] = 0x1400,
        ["app.uninstall"] = 0x1401,
        ["app.list"] = 0x1402,
        ["app.getMetadata"] = 0x1403,
        ["app.launch"] = 0x1404,
        ["app.validate"] = 0x1405,
        // SPI
        ["spi.open"] = 0x1500,
        ["spi.transfer"] = 0x1501,
        ["spi.write"] = 0x1502,
        ["spi.close"] = 0x1503,
        // Device
        ["device.list"] = 0x1600,
        ["device.open"] = 0x1601,
        ["device.close"] = 0x1602,
        ["device.getInfo"] = 0x1603,
        ["device.probe"] = 0x1604,
    };

    /// <summary>
    /// Resolve a native function name to its ID. Returns null if unknown.
    /// </summary>
    public static ushort? Resolve(string name)
        => Map.TryGetValue(name, out var id) ? id : null;
}
