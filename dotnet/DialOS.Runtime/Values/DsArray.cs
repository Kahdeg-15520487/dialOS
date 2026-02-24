namespace DialOS.Runtime.Values;

/// <summary>
/// Array type (dynamic array) in dialScript.
/// </summary>
public class DsArray
{
    /// <summary>
    /// Array elements.
    /// </summary>
    public List<Value> Elements { get; } = new();

    public DsArray() { }

    public DsArray(int size)
    {
        Elements.Capacity = size;
        for (int i = 0; i < size; i++)
        {
            Elements.Add(Value.Null());
        }
    }

    /// <summary>
    /// Get the length of the array.
    /// </summary>
    public int Length => Elements.Count;

    /// <summary>
    /// Get or set an element by index.
    /// </summary>
    public Value this[int index]
    {
        get
        {
            if (index < 0 || index >= Elements.Count)
                return Value.Null();
            return Elements[index];
        }
        set
        {
            if (index >= 0 && index < Elements.Count)
            {
                Elements[index] = value;
            }
        }
    }

    /// <summary>
    /// Add an element to the end of the array.
    /// </summary>
    public void Add(Value value)
    {
        Elements.Add(value);
    }

    /// <summary>
    /// Push an element onto the end (alias for Add).
    /// </summary>
    public void Push(Value value) => Add(value);

    /// <summary>
    /// Remove and return the last element.
    /// </summary>
    public Value Pop()
    {
        if (Elements.Count == 0)
            return Value.Null();
        var value = Elements[^1];
        Elements.RemoveAt(Elements.Count - 1);
        return value;
    }

    public override string ToString()
    {
        return $"[Array({Length})]";
    }
}
