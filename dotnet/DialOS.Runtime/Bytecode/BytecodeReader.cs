namespace DialOS.Runtime.Bytecode;

/// <summary>
/// Utility class for reading binary data from bytecode.
/// Uses little-endian byte order (matching the C++ implementation).
/// </summary>
public class BytecodeReader
{
    private readonly byte[] _data;
    private int _position;

    public BytecodeReader(byte[] data)
    {
        _data = data;
        _position = 0;
    }

    public int Position => _position;
    public int Length => _data.Length;
    public int Remaining => _data.Length - _position;
    public bool HasMore => _position < _data.Length;

    /// <summary>
    /// Read a single byte.
    /// </summary>
    public byte ReadByte()
    {
        if (_position >= _data.Length)
            throw new EndOfStreamException("Unexpected end of bytecode");
        return _data[_position++];
    }

    /// <summary>
    /// Read a signed 8-bit integer.
    /// </summary>
    public sbyte ReadSByte()
    {
        return (sbyte)ReadByte();
    }

    /// <summary>
    /// Read an unsigned 16-bit integer (little-endian).
    /// </summary>
    public ushort ReadUInt16()
    {
        if (_position + 2 > _data.Length)
            throw new EndOfStreamException("Unexpected end of bytecode");
        var value = (ushort)(_data[_position] | (_data[_position + 1] << 8));
        _position += 2;
        return value;
    }

    /// <summary>
    /// Read a signed 16-bit integer (little-endian).
    /// </summary>
    public short ReadInt16()
    {
        return (short)ReadUInt16();
    }

    /// <summary>
    /// Read an unsigned 32-bit integer (little-endian).
    /// </summary>
    public uint ReadUInt32()
    {
        if (_position + 4 > _data.Length)
            throw new EndOfStreamException("Unexpected end of bytecode");
        var value = (uint)(_data[_position] |
                          (_data[_position + 1] << 8) |
                          (_data[_position + 2] << 16) |
                          (_data[_position + 3] << 24));
        _position += 4;
        return value;
    }

    /// <summary>
    /// Read a signed 32-bit integer (little-endian).
    /// </summary>
    public int ReadInt32()
    {
        return (int)ReadUInt32();
    }

    /// <summary>
    /// Read a 32-bit float (little-endian).
    /// </summary>
    public float ReadFloat()
    {
        var bytes = ReadBytes(4);
        if (!BitConverter.IsLittleEndian)
            Array.Reverse(bytes);
        return BitConverter.ToSingle(bytes, 0);
    }

    /// <summary>
    /// Read a length-prefixed string (uint16 length + data).
    /// </summary>
    public string ReadString()
    {
        var length = ReadUInt16();
        if (_position + length > _data.Length)
            throw new EndOfStreamException("Unexpected end of bytecode while reading string");
        var str = System.Text.Encoding.UTF8.GetString(_data, _position, length);
        _position += length;
        return str;
    }

    /// <summary>
    /// Read a specified number of bytes.
    /// </summary>
    public byte[] ReadBytes(int count)
    {
        if (_position + count > _data.Length)
            throw new EndOfStreamException("Unexpected end of bytecode");
        var result = new byte[count];
        Array.Copy(_data, _position, result, 0, count);
        _position += count;
        return result;
    }

    /// <summary>
    /// Read an opcode at the current position.
    /// </summary>
    public Opcode ReadOpcode()
    {
        return (Opcode)ReadByte();
    }

    /// <summary>
    /// Peek at the next byte without advancing.
    /// </summary>
    public byte PeekByte()
    {
        if (_position >= _data.Length)
            throw new EndOfStreamException("Unexpected end of bytecode");
        return _data[_position];
    }

    /// <summary>
    /// Set the read position.
    /// </summary>
    public void Seek(int position)
    {
        if (position < 0 || position > _data.Length)
            throw new ArgumentOutOfRangeException(nameof(position));
        _position = position;
    }

    /// <summary>
    /// Verify the magic number "DSBC".
    /// </summary>
    public bool VerifyMagic()
    {
        if (_position + 4 > _data.Length)
            return false;
        return _data[_position] == 'D' &&
               _data[_position + 1] == 'S' &&
               _data[_position + 2] == 'B' &&
               _data[_position + 3] == 'C';
    }

    /// <summary>
    /// Read and verify the magic number, advancing the position.
    /// </summary>
    public void ReadMagic()
    {
        if (!VerifyMagic())
            throw new InvalidDataException("Invalid bytecode file format: missing DSBC magic");
        _position += 4;
    }
}
