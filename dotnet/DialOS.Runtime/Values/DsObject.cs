namespace DialOS.Runtime.Values;

/// <summary>
/// Object type (key-value map) in dialScript.
/// </summary>
public class DsObject
{
    /// <summary>
    /// Object fields (property storage).
    /// </summary>
    public Dictionary<string, Value> Fields { get; } = new();

    /// <summary>
    /// Class name for this object.
    /// </summary>
    public string ClassName { get; set; } = "Object";

    public DsObject() { }

    public DsObject(string className)
    {
        ClassName = className;
    }

    /// <summary>
    /// Get a field value by name.
    /// </summary>
    public Value GetField(string name)
    {
        return Fields.TryGetValue(name, out var value) ? value : Value.Null();
    }

    /// <summary>
    /// Set a field value by name.
    /// </summary>
    public void SetField(string name, Value value)
    {
        Fields[name] = value;
    }

    /// <summary>
    /// Check if a field exists.
    /// </summary>
    public bool HasField(string name)
    {
        return Fields.ContainsKey(name);
    }

    public override string ToString()
    {
        return $"[{ClassName}]";
    }
}
