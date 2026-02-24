namespace DialOS.Runtime.Bytecode;

/// <summary>
/// Metadata for a bytecode module.
/// Contains app information and execution requirements.
/// </summary>
public class Metadata
{
    /// <summary>
    /// Bytecode format version.
    /// </summary>
    public ushort Version { get; set; } = 1;

    /// <summary>
    /// Required heap size in bytes.
    /// </summary>
    public uint HeapSize { get; set; } = 8192;

    /// <summary>
    /// Application name.
    /// </summary>
    public string AppName { get; set; } = "untitled";

    /// <summary>
    /// Application version string.
    /// </summary>
    public string AppVersion { get; set; } = "1.0.0";

    /// <summary>
    /// Author name.
    /// </summary>
    public string Author { get; set; } = "";

    /// <summary>
    /// Compilation timestamp (Unix epoch).
    /// </summary>
    public uint Timestamp { get; set; }

    /// <summary>
    /// Hash code for metadata integrity.
    /// </summary>
    public uint HashCode { get; set; }

    /// <summary>
    /// Bytecode checksum.
    /// </summary>
    public ushort Checksum { get; set; }

    /// <summary>
    /// Calculate hash code for metadata integrity.
    /// </summary>
    public uint CalculateHash()
    {
        uint hash = 0x811C9DC5; // FNV-1a offset basis
        const uint prime = 0x01000193; // FNV-1a prime

        // Hash version and heap size
        hash ^= Version;
        hash *= prime;
        hash ^= HeapSize;
        hash *= prime;
        hash ^= Timestamp;
        hash *= prime;

        // Hash the bytecode checksum
        hash ^= Checksum;
        hash *= prime;

        // Hash strings
        foreach (char c in AppName)
        {
            hash ^= (byte)c;
            hash *= prime;
        }
        foreach (char c in AppVersion)
        {
            hash ^= (byte)c;
            hash *= prime;
        }
        foreach (char c in Author)
        {
            hash ^= (byte)c;
            hash *= prime;
        }

        return hash;
    }
}
