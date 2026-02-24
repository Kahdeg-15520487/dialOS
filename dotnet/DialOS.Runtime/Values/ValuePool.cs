namespace DialOS.Runtime.Values;

/// <summary>
/// Memory pool for heap-allocated values with string interning.
/// Manages memory for strings, objects, arrays, and functions.
/// </summary>
public class ValuePool : IDisposable
{
    private readonly long _heapSize;
    private long _allocated;

    // String interning: maps string content to the interned string
    private readonly Dictionary<string, string> _internedStrings = new();

    // Track all allocated objects for cleanup
    private readonly List<DsObject> _objects = new();
    private readonly List<DsArray> _arrays = new();
    private readonly List<DsFunction> _functions = new();

    private bool _disposed;

    public ValuePool(uint heapSize)
    {
        _heapSize = heapSize;
        _allocated = 0;
    }

    /// <summary>
    /// Get the total heap size.
    /// </summary>
    public long HeapSize => _heapSize;

    /// <summary>
    /// Get the currently allocated bytes.
    /// </summary>
    public long Allocated => _allocated;

    /// <summary>
    /// Get the available heap space.
    /// </summary>
    public long Available => _heapSize - _allocated;

    /// <summary>
    /// Check if there's enough space for allocation.
    /// </summary>
    public bool CanAllocate(long size) => _allocated + size <= _heapSize;

    /// <summary>
    /// Intern a string (reuse if already exists).
    /// </summary>
    public string InternString(string str)
    {
        if (_internedStrings.TryGetValue(str, out var interned))
        {
            return interned;
        }

        // Estimate memory: ~50 bytes overhead + 2 bytes per char
        var size = 50 + str.Length * 2;
        if (!CanAllocate(size))
        {
            GarbageCollectStrings();
            if (!CanAllocate(size))
            {
                throw new OutOfMemoryException($"Out of memory: cannot allocate string of length {str.Length}");
            }
        }

        _internedStrings[str] = str;
        _allocated += size;
        return str;
    }

    /// <summary>
    /// Allocate a new object.
    /// </summary>
    public DsObject AllocateObject(string className = "Object")
    {
        // Estimate: ~100 bytes overhead for object + dictionary
        const int size = 100;
        if (!CanAllocate(size))
        {
            throw new OutOfMemoryException("Out of memory: cannot allocate object");
        }

        var obj = new DsObject(className);
        _objects.Add(obj);
        _allocated += size;
        return obj;
    }

    /// <summary>
    /// Allocate a new array with specified size.
    /// </summary>
    public DsArray AllocateArray(int size = 0)
    {
        // Estimate: ~50 bytes overhead + 24 bytes per element (Value struct)
        var allocSize = 50 + size * 24;
        if (!CanAllocate(allocSize))
        {
            throw new OutOfMemoryException($"Out of memory: cannot allocate array of size {size}");
        }

        var arr = new DsArray(size);
        _arrays.Add(arr);
        _allocated += allocSize;
        return arr;
    }

    /// <summary>
    /// Allocate a new function reference.
    /// </summary>
    public DsFunction AllocateFunction(ushort functionIndex, byte paramCount)
    {
        const int size = 32;
        if (!CanAllocate(size))
        {
            throw new OutOfMemoryException("Out of memory: cannot allocate function");
        }

        var fn = new DsFunction(functionIndex, paramCount);
        _functions.Add(fn);
        _allocated += size;
        return fn;
    }

    /// <summary>
    /// Create a Value from an interned string.
    /// </summary>
    public Value CreateString(string str)
    {
        return Value.String(InternString(str));
    }

    /// <summary>
    /// Garbage collect interned strings (simple implementation).
    /// In a real implementation, this would track references.
    /// </summary>
    public void GarbageCollectStrings()
    {
        // Simple heuristic: clear all interned strings
        // This works because we're called when temporaries should be gone
        foreach (var str in _internedStrings.Values)
        {
            var size = 50 + str.Length * 2;
            _allocated -= size;
        }
        _internedStrings.Clear();
    }

    /// <summary>
    /// Reset the pool (clear all allocations).
    /// </summary>
    public void Reset()
    {
        _internedStrings.Clear();
        _objects.Clear();
        _arrays.Clear();
        _functions.Clear();
        _allocated = 0;
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            Reset();
            _disposed = true;
        }
        GC.SuppressFinalize(this);
    }

    ~ValuePool()
    {
        Dispose();
    }
}
