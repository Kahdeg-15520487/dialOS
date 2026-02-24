namespace DialOS.Runtime.Bytecode;

/// <summary>
/// Represents a function definition in the bytecode module.
/// </summary>
public class FunctionDef
{
    public string Name { get; set; } = "";
    public uint EntryPoint { get; set; }
    public byte ParamCount { get; set; }
}

/// <summary>
/// Represents a parsed dialScript bytecode module (.dsb file).
/// Contains metadata, constants, globals, functions, and bytecode.
/// </summary>
public class BytecodeModule
{
    /// <summary>
    /// Module metadata (app info, heap size, etc.)
    /// </summary>
    public Metadata Metadata { get; set; } = new();

    /// <summary>
    /// String constant pool.
    /// </summary>
    public List<string> Constants { get; } = new();

    /// <summary>
    /// Global variable names.
    /// </summary>
    public List<string> Globals { get; } = new();

    /// <summary>
    /// Function definitions.
    /// </summary>
    public List<FunctionDef> Functions { get; } = new();

    /// <summary>
    /// Bytecode instructions.
    /// </summary>
    public byte[] Code { get; set; } = Array.Empty<byte>();

    /// <summary>
    /// Source line numbers for each bytecode byte (optional debug info).
    /// </summary>
    public uint[]? DebugLines { get; set; }

    /// <summary>
    /// Main entry point (PC where execution starts).
    /// </summary>
    public uint MainEntryPoint { get; set; }

    /// <summary>
    /// Whether debug info is available.
    /// </summary>
    public bool HasDebugInfo => DebugLines != null && DebugLines.Length > 0;

    /// <summary>
    /// Load a bytecode module from a .dsb file.
    /// </summary>
    public static BytecodeModule Load(string path)
    {
        var data = File.ReadAllBytes(path);
        return Deserialize(data);
    }

    /// <summary>
    /// Load a bytecode module from a stream.
    /// </summary>
    public static BytecodeModule Load(Stream stream)
    {
        using var ms = new MemoryStream();
        stream.CopyTo(ms);
        return Deserialize(ms.ToArray());
    }

    /// <summary>
    /// Deserialize bytecode module from binary format.
    /// </summary>
    public static BytecodeModule Deserialize(byte[] data)
    {
        var reader = new BytecodeReader(data);
        var module = new BytecodeModule();

        // Read and verify magic number
        reader.ReadMagic();

        // Read format version
        module.Metadata.Version = reader.ReadUInt16();

        // Read flags
        var flags = reader.ReadUInt16();
        var hasDebugInfo = (flags & 0x0001) != 0;

        // Metadata section
        module.Metadata.HeapSize = reader.ReadUInt32();
        module.Metadata.AppName = reader.ReadString();
        module.Metadata.AppVersion = reader.ReadString();
        module.Metadata.Author = reader.ReadString();
        module.Metadata.Timestamp = reader.ReadUInt32();
        module.Metadata.HashCode = reader.ReadUInt32();
        module.Metadata.Checksum = reader.ReadUInt16();

        // Constants section
        var constantCount = reader.ReadUInt32();
        module.Constants.EnsureCapacity((int)constantCount);
        for (uint i = 0; i < constantCount; i++)
        {
            module.Constants.Add(reader.ReadString());
        }

        // Globals section
        var globalCount = reader.ReadUInt32();
        module.Globals.EnsureCapacity((int)globalCount);
        for (uint i = 0; i < globalCount; i++)
        {
            module.Globals.Add(reader.ReadString());
        }

        // Functions section
        var functionCount = reader.ReadUInt32();
        module.Functions.EnsureCapacity((int)functionCount);
        for (uint i = 0; i < functionCount; i++)
        {
            var func = new FunctionDef
            {
                Name = reader.ReadString(),
                EntryPoint = reader.ReadUInt32(),
                ParamCount = reader.ReadByte()
            };
            module.Functions.Add(func);
        }

        // Main entry point
        module.MainEntryPoint = reader.ReadUInt32();

        // Code section
        var codeSize = reader.ReadUInt32();
        module.Code = reader.ReadBytes((int)codeSize);

        // Debug section (optional)
        if (hasDebugInfo && reader.HasMore)
        {
            var debugSize = reader.ReadUInt32();
            module.DebugLines = new uint[debugSize];
            for (uint i = 0; i < debugSize; i++)
            {
                module.DebugLines[i] = reader.ReadUInt32();
            }
        }

        // Verify bytecode integrity
        if (!module.VerifyIntegrity())
        {
            throw new InvalidDataException("Bytecode integrity check failed - file may be corrupted");
        }

        return module;
    }

    /// <summary>
    /// Calculate checksum of bytecode content.
    /// </summary>
    public ushort CalculateBytecodeChecksum()
    {
        ushort sum = 0;

        // Checksum all bytecode bytes
        foreach (var b in Code)
        {
            sum += b;
        }

        // Include debug info in checksum if present
        if (DebugLines != null)
        {
            foreach (var line in DebugLines)
            {
                sum += (byte)(line & 0xFF);
                sum += (byte)((line >> 8) & 0xFF);
                sum += (byte)((line >> 16) & 0xFF);
                sum += (byte)((line >> 24) & 0xFF);
            }
        }

        return sum;
    }

    /// <summary>
    /// Verify bytecode and metadata integrity.
    /// </summary>
    public bool VerifyIntegrity()
    {
        // Verify bytecode checksum
        if (Metadata.Checksum != CalculateBytecodeChecksum())
            return false;

        // Verify metadata hash
        if (Metadata.HashCode != Metadata.CalculateHash())
            return false;

        return true;
    }

    /// <summary>
    /// Get a constant string by index.
    /// </summary>
    public string GetConstant(int index)
    {
        if (index < 0 || index >= Constants.Count)
            throw new ArgumentOutOfRangeException(nameof(index));
        return Constants[index];
    }

    /// <summary>
    /// Get a global variable name by index.
    /// </summary>
    public string GetGlobal(int index)
    {
        if (index < 0 || index >= Globals.Count)
            throw new ArgumentOutOfRangeException(nameof(index));
        return Globals[index];
    }

    /// <summary>
    /// Get a function by index.
    /// </summary>
    public FunctionDef GetFunction(int index)
    {
        if (index < 0 || index >= Functions.Count)
            throw new ArgumentOutOfRangeException(nameof(index));
        return Functions[index];
    }

    /// <summary>
    /// Get source line for a bytecode position (if debug info available).
    /// </summary>
    public uint GetSourceLine(int pc)
    {
        if (DebugLines == null || pc < 0 || pc >= DebugLines.Length)
            return 0;
        return DebugLines[pc];
    }

    /// <summary>
    /// Disassemble bytecode to string (for debugging).
    /// </summary>
    public string Disassemble()
    {
        var sb = new System.Text.StringBuilder();

        sb.AppendLine("=== Bytecode Disassembly ===");
        sb.AppendLine();

        // Metadata
        sb.AppendLine("Metadata:");
        sb.AppendLine($"  App Name:    {Metadata.AppName}");
        sb.AppendLine($"  Version:     {Metadata.AppVersion}");
        sb.AppendLine($"  Author:      {(string.IsNullOrEmpty(Metadata.Author) ? "(none)" : Metadata.Author)}");
        sb.AppendLine($"  Heap Size:   {Metadata.HeapSize} bytes");
        sb.AppendLine($"  Format Ver:  {Metadata.Version}");
        sb.AppendLine($"  Hash Code:   0x{Metadata.HashCode:X8} (metadata)");
        sb.AppendLine($"  Checksum:    0x{Metadata.Checksum:X4} (bytecode)");
        sb.AppendLine($"  Integrity:   {(VerifyIntegrity() ? "VALID" : "CORRUPTED")}");
        sb.AppendLine();

        // Constants
        sb.AppendLine($"Constants ({Constants.Count}):");
        for (int i = 0; i < Constants.Count; i++)
        {
            sb.AppendLine($"  [{i}] \"{Constants[i]}\"");
        }
        sb.AppendLine();

        // Globals
        sb.AppendLine($"Globals ({Globals.Count}):");
        for (int i = 0; i < Globals.Count; i++)
        {
            sb.AppendLine($"  [{i}] {Globals[i]}");
        }
        sb.AppendLine();

        // Functions
        sb.AppendLine($"Functions ({Functions.Count}):");
        for (int i = 0; i < Functions.Count; i++)
        {
            var func = Functions[i];
            sb.AppendLine($"  [{i}] {func.Name} @ PC:{func.EntryPoint} (params: {func.ParamCount})");
        }
        sb.AppendLine();

        sb.AppendLine($"Main Entry Point: PC:{MainEntryPoint}");
        sb.AppendLine();

        // Debug info status
        sb.AppendLine($"Debug Info: {(HasDebugInfo ? $"Enabled ({DebugLines!.Length} entries)" : "Disabled")}");
        sb.AppendLine();

        // Code disassembly
        sb.AppendLine($"Code ({Code.Length} bytes):");
        DisassembleCode(sb);

        return sb.ToString();
    }

    private void DisassembleCode(System.Text.StringBuilder sb)
    {
        int pos = 0;
        while (pos < Code.Length)
        {
            sb.Append($"{pos,6:D6}  ");

            // Show source line if debug info available
            if (HasDebugInfo && pos < DebugLines!.Length && DebugLines[pos] > 0)
            {
                sb.Append($"[L{DebugLines[pos],3:D3}] ");
            }
            else
            {
                sb.Append("      ");
            }

            var op = (Opcode)Code[pos++];

            switch (op)
            {
                case Opcode.Nop:
                    sb.AppendLine("NOP");
                    break;
                case Opcode.Pop:
                    sb.AppendLine("POP");
                    break;
                case Opcode.Dup:
                    sb.AppendLine("DUP");
                    break;
                case Opcode.Swap:
                    sb.AppendLine("SWAP");
                    break;

                case Opcode.PushNull:
                    sb.AppendLine("PUSH_NULL");
                    break;
                case Opcode.PushTrue:
                    sb.AppendLine("PUSH_TRUE");
                    break;
                case Opcode.PushFalse:
                    sb.AppendLine("PUSH_FALSE");
                    break;
                case Opcode.PushI8:
                    if (pos < Code.Length)
                    {
                        var value = (sbyte)Code[pos++];
                        sb.AppendLine($"PUSH_I8 {value}");
                    }
                    break;
                case Opcode.PushI16:
                    if (pos + 1 < Code.Length)
                    {
                        var value = (short)(Code[pos] | (Code[pos + 1] << 8));
                        pos += 2;
                        sb.AppendLine($"PUSH_I16 {value}");
                    }
                    break;
                case Opcode.PushI32:
                    if (pos + 3 < Code.Length)
                    {
                        var value = Code[pos] | (Code[pos + 1] << 8) |
                                   (Code[pos + 2] << 16) | (Code[pos + 3] << 24);
                        pos += 4;
                        sb.AppendLine($"PUSH_I32 {value}");
                    }
                    break;
                case Opcode.PushF32:
                    if (pos + 3 < Code.Length)
                    {
                        var bytes = new byte[] { Code[pos], Code[pos + 1], Code[pos + 2], Code[pos + 3] };
                        pos += 4;
                        var value = BitConverter.ToSingle(bytes, 0);
                        sb.AppendLine($"PUSH_F32 {value}");
                    }
                    break;
                case Opcode.PushStr:
                    if (pos + 1 < Code.Length)
                    {
                        var idx = Code[pos] | (Code[pos + 1] << 8);
                        pos += 2;
                        var str = idx < Constants.Count ? Constants[idx] : "?";
                        sb.AppendLine($"PUSH_STR [{idx}] \"{str}\"");
                    }
                    break;

                case Opcode.LoadLocal:
                    if (pos < Code.Length)
                    {
                        sb.AppendLine($"LOAD_LOCAL {Code[pos++]}");
                    }
                    break;
                case Opcode.StoreLocal:
                    if (pos < Code.Length)
                    {
                        sb.AppendLine($"STORE_LOCAL {Code[pos++]}");
                    }
                    break;

                case Opcode.LoadGlobal:
                    if (pos + 1 < Code.Length)
                    {
                        var idx = Code[pos] | (Code[pos + 1] << 8);
                        pos += 2;
                        var name = idx < Globals.Count ? Globals[idx] : "?";
                        sb.AppendLine($"LOAD_GLOBAL [{idx}] {name}");
                    }
                    break;
                case Opcode.StoreGlobal:
                    if (pos + 1 < Code.Length)
                    {
                        var idx = Code[pos] | (Code[pos + 1] << 8);
                        pos += 2;
                        var name = idx < Globals.Count ? Globals[idx] : "?";
                        sb.AppendLine($"STORE_GLOBAL [{idx}] {name}");
                    }
                    break;

                case Opcode.Add:
                    sb.AppendLine("ADD");
                    break;
                case Opcode.Sub:
                    sb.AppendLine("SUB");
                    break;
                case Opcode.Mul:
                    sb.AppendLine("MUL");
                    break;
                case Opcode.Div:
                    sb.AppendLine("DIV");
                    break;
                case Opcode.Mod:
                    sb.AppendLine("MOD");
                    break;
                case Opcode.Neg:
                    sb.AppendLine("NEG");
                    break;

                case Opcode.StrConcat:
                    sb.AppendLine("STR_CONCAT");
                    break;

                case Opcode.Eq:
                    sb.AppendLine("EQ");
                    break;
                case Opcode.Ne:
                    sb.AppendLine("NE");
                    break;
                case Opcode.Lt:
                    sb.AppendLine("LT");
                    break;
                case Opcode.Le:
                    sb.AppendLine("LE");
                    break;
                case Opcode.Gt:
                    sb.AppendLine("GT");
                    break;
                case Opcode.Ge:
                    sb.AppendLine("GE");
                    break;

                case Opcode.Not:
                    sb.AppendLine("NOT");
                    break;
                case Opcode.And:
                    sb.AppendLine("AND");
                    break;
                case Opcode.Or:
                    sb.AppendLine("OR");
                    break;

                case Opcode.Jump:
                case Opcode.JumpIf:
                case Opcode.JumpIfNot:
                    if (pos + 3 < Code.Length)
                    {
                        var offset = Code[pos] | (Code[pos + 1] << 8) |
                                    (Code[pos + 2] << 16) | (Code[pos + 3] << 24);
                        // Handle signed offset
                        var signedOffset = BitConverter.ToInt32(BitConverter.GetBytes(offset), 0);
                        pos += 4;
                        var target = pos + signedOffset;
                        var name = op switch
                        {
                            Opcode.Jump => "JUMP",
                            Opcode.JumpIf => "JUMP_IF",
                            Opcode.JumpIfNot => "JUMP_IF_NOT",
                            _ => "?"
                        };
                        sb.AppendLine($"{name} {signedOffset} (to {target})");
                    }
                    break;

                case Opcode.Call:
                    if (pos + 2 < Code.Length)
                    {
                        var funcIdx = Code[pos] | (Code[pos + 1] << 8);
                        var argCount = Code[pos + 2];
                        pos += 3;
                        var funcName = funcIdx < Functions.Count ? Functions[funcIdx].Name : "?";
                        sb.AppendLine($"CALL [{funcIdx}] {funcName} argc={argCount}");
                    }
                    break;
                case Opcode.CallNative:
                    if (pos + 2 < Code.Length)
                    {
                        var nativeIdx = Code[pos] | (Code[pos + 1] << 8);
                        var argCount = Code[pos + 2];
                        pos += 3;
                        sb.AppendLine($"CALL_NATIVE [0x{nativeIdx:X4}] argc={argCount}");
                    }
                    break;
                case Opcode.Return:
                    sb.AppendLine("RETURN");
                    break;
                case Opcode.LoadFunction:
                    if (pos + 1 < Code.Length)
                    {
                        var funcIdx = Code[pos] | (Code[pos + 1] << 8);
                        pos += 2;
                        var funcName = funcIdx < Functions.Count ? Functions[funcIdx].Name : "?";
                        sb.AppendLine($"LOAD_FUNCTION [{funcIdx}] {funcName}");
                    }
                    break;
                case Opcode.CallIndirect:
                    if (pos < Code.Length)
                    {
                        var argCount = Code[pos++];
                        sb.AppendLine($"CALL_INDIRECT argc={argCount}");
                    }
                    break;
                case Opcode.CallMethod:
                    if (pos + 2 < Code.Length)
                    {
                        var argCount = Code[pos++];
                        var nameIdx = Code[pos] | (Code[pos + 1] << 8);
                        pos += 2;
                        var methodName = nameIdx < Constants.Count ? Constants[nameIdx] : "?";
                        sb.AppendLine($"CALL_METHOD argc={argCount} {methodName}");
                    }
                    break;

                case Opcode.GetField:
                    if (pos + 1 < Code.Length)
                    {
                        var idx = Code[pos] | (Code[pos + 1] << 8);
                        pos += 2;
                        var fieldName = idx < Constants.Count ? Constants[idx] : "?";
                        sb.AppendLine($"GET_FIELD [{idx}] {fieldName}");
                    }
                    break;
                case Opcode.SetField:
                    if (pos + 1 < Code.Length)
                    {
                        var idx = Code[pos] | (Code[pos + 1] << 8);
                        pos += 2;
                        var fieldName = idx < Constants.Count ? Constants[idx] : "?";
                        sb.AppendLine($"SET_FIELD [{idx}] {fieldName}");
                    }
                    break;
                case Opcode.GetIndex:
                    sb.AppendLine("GET_INDEX");
                    break;
                case Opcode.SetIndex:
                    sb.AppendLine("SET_INDEX");
                    break;

                case Opcode.NewObject:
                    if (pos + 1 < Code.Length)
                    {
                        var classIdx = Code[pos] | (Code[pos + 1] << 8);
                        pos += 2;
                        sb.AppendLine($"NEW_OBJECT [{classIdx}]");
                    }
                    break;
                case Opcode.NewArray:
                    sb.AppendLine("NEW_ARRAY");
                    break;

                case Opcode.Try:
                    if (pos + 3 < Code.Length)
                    {
                        var offsetBytes = new byte[] { Code[pos], Code[pos + 1], Code[pos + 2], Code[pos + 3] };
                        var offset = BitConverter.ToInt32(offsetBytes, 0);
                        pos += 4;
                        sb.AppendLine($"TRY +{offset}");
                    }
                    break;
                case Opcode.EndTry:
                    sb.AppendLine("END_TRY");
                    break;
                case Opcode.Throw:
                    sb.AppendLine("THROW");
                    break;

                case Opcode.Print:
                    sb.AppendLine("PRINT");
                    break;
                case Opcode.Halt:
                    sb.AppendLine("HALT");
                    break;

                case Opcode.TemplateFormat:
                    if (pos < Code.Length)
                    {
                        var argCount = Code[pos++];
                        sb.AppendLine($"TEMPLATE_FORMAT argc={argCount}");
                    }
                    break;

                default:
                    sb.AppendLine($"UNKNOWN(0x{(byte)op:X2})");
                    break;
            }
        }
    }
}
