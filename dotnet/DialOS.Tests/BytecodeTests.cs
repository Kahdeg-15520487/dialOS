using DialOS.Runtime.Bytecode;

namespace DialOS.Tests;

public class BytecodeTests
{
    [Fact]
    public void Opcode_ShouldHaveCorrectValues()
    {
        // Stack operations
        Assert.Equal(0x00, (byte)Opcode.Nop);
        Assert.Equal(0x01, (byte)Opcode.Pop);
        Assert.Equal(0x02, (byte)Opcode.Dup);
        Assert.Equal(0x03, (byte)Opcode.Swap);

        // Constants
        Assert.Equal(0x10, (byte)Opcode.PushNull);
        Assert.Equal(0x11, (byte)Opcode.PushTrue);
        Assert.Equal(0x12, (byte)Opcode.PushFalse);
        Assert.Equal(0x13, (byte)Opcode.PushI8);
        Assert.Equal(0x14, (byte)Opcode.PushI16);
        Assert.Equal(0x15, (byte)Opcode.PushI32);
        Assert.Equal(0x16, (byte)Opcode.PushF32);
        Assert.Equal(0x17, (byte)Opcode.PushStr);

        // Arithmetic
        Assert.Equal(0x40, (byte)Opcode.Add);
        Assert.Equal(0x41, (byte)Opcode.Sub);
        Assert.Equal(0x42, (byte)Opcode.Mul);
        Assert.Equal(0x43, (byte)Opcode.Div);
        Assert.Equal(0x44, (byte)Opcode.Mod);
        Assert.Equal(0x45, (byte)Opcode.Neg);

        // Control flow
        Assert.Equal(0x70, (byte)Opcode.Jump);
        Assert.Equal(0x71, (byte)Opcode.JumpIf);
        Assert.Equal(0x72, (byte)Opcode.JumpIfNot);

        // Functions
        Assert.Equal(0x80, (byte)Opcode.Call);
        Assert.Equal(0x81, (byte)Opcode.CallNative);
        Assert.Equal(0x82, (byte)Opcode.Return);

        // Special
        Assert.Equal(0xF0, (byte)Opcode.Print);
        Assert.Equal(0xFF, (byte)Opcode.Halt);
    }

    [Fact]
    public void BytecodeReader_ReadByte_ShouldWork()
    {
        var data = new byte[] { 0x10, 0x20, 0x30 };
        var reader = new BytecodeReader(data);

        Assert.Equal(0x10, reader.ReadByte());
        Assert.Equal(0x20, reader.ReadByte());
        Assert.Equal(0x30, reader.ReadByte());
        Assert.Equal(3, reader.Position);
    }

    [Fact]
    public void BytecodeReader_ReadUInt16_ShouldUseLittleEndian()
    {
        var data = new byte[] { 0x34, 0x12 }; // 0x1234 in little-endian
        var reader = new BytecodeReader(data);

        Assert.Equal(0x1234, reader.ReadUInt16());
    }

    [Fact]
    public void BytecodeReader_ReadInt32_ShouldUseLittleEndian()
    {
        var data = new byte[] { 0x78, 0x56, 0x34, 0x12 }; // 0x12345678 in little-endian
        var reader = new BytecodeReader(data);

        Assert.Equal(0x12345678, reader.ReadInt32());
    }

    [Fact]
    public void BytecodeReader_ReadString_ShouldReadLengthPrefixedString()
    {
        var data = new byte[] { 5, 0, (byte)'H', (byte)'e', (byte)'l', (byte)'l', (byte)'o' };
        var reader = new BytecodeReader(data);

        Assert.Equal("Hello", reader.ReadString());
    }

    [Fact]
    public void BytecodeReader_VerifyMagic_ShouldDetectValidMagic()
    {
        var data = new byte[] { (byte)'D', (byte)'S', (byte)'B', (byte)'C' };
        var reader = new BytecodeReader(data);

        Assert.True(reader.VerifyMagic());
    }

    [Fact]
    public void BytecodeReader_VerifyMagic_ShouldDetectInvalidMagic()
    {
        var data = new byte[] { (byte)'X', (byte)'Y', (byte)'Z', (byte)'W' };
        var reader = new BytecodeReader(data);

        Assert.False(reader.VerifyMagic());
    }

    [Fact]
    public void Metadata_CalculateHash_ShouldBeConsistent()
    {
        var metadata = new Metadata
        {
            Version = 1,
            HeapSize = 8192,
            AppName = "TestApp",
            AppVersion = "1.0.0",
            Author = "Test",
            Timestamp = 1234567890,
            Checksum = 0x1234
        };

        var hash1 = metadata.CalculateHash();
        var hash2 = metadata.CalculateHash();

        Assert.Equal(hash1, hash2);
    }

    [Fact]
    public void Metadata_CalculateHash_ShouldChangeWithDifferentValues()
    {
        var metadata1 = new Metadata { AppName = "App1" };
        var metadata2 = new Metadata { AppName = "App2" };

        Assert.NotEqual(metadata1.CalculateHash(), metadata2.CalculateHash());
    }
}
