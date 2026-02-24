using System.Text;

namespace DialOS.Runtime.Values;

/// <summary>
/// Tagged union value type for VM operations.
/// Represents any value in dialScript.
/// </summary>
public readonly record struct Value
{
    /// <summary>
    /// The type of this value.
    /// </summary>
    public readonly ValueType Type;

    // Storage for different value types
    private readonly object? _ref;
    private readonly int _int;
    private readonly float _float;

    // Private constructor
    private Value(ValueType type, object? refVal = null, int intVal = 0, float floatVal = 0f)
    {
        Type = type;
        _ref = refVal;
        _int = intVal;
        _float = floatVal;
    }

    // Factory methods
    public static Value Null() => new(ValueType.Null);
    public static Value Bool(bool b) => new(ValueType.Bool, null, b ? 1 : 0);
    public static Value Int32(int i) => new(ValueType.Int32, null, i);
    public static Value Float32(float f) => new(ValueType.Float32, floatVal: f);
    public static Value String(string s) => new(ValueType.String, s);
    public static Value Object(DsObject obj) => new(ValueType.Object, obj);
    public static Value Array(DsArray arr) => new(ValueType.Array, arr);
    public static Value Function(DsFunction fn) => new(ValueType.Function, fn);
    public static Value NativeFn(IntPtr fn) => new(ValueType.NativeFn, fn);

    // Type checking
    public bool IsNull => Type == ValueType.Null;
    public bool IsBool => Type == ValueType.Bool;
    public bool IsInt32 => Type == ValueType.Int32;
    public bool IsFloat32 => Type == ValueType.Float32;
    public bool IsString => Type == ValueType.String;
    public bool IsObject => Type == ValueType.Object;
    public bool IsArray => Type == ValueType.Array;
    public bool IsFunction => Type == ValueType.Function;
    public bool IsNativeFn => Type == ValueType.NativeFn;
    public bool IsNumeric => Type == ValueType.Int32 || Type == ValueType.Float32;

    // Value getters
    public bool AsBool() => _int != 0;
    public int AsInt32() => _int;
    public float AsFloat32() => _float;
    public string AsString() => _ref as string ?? "";
    public DsObject AsObject() => _ref as DsObject ?? throw new InvalidOperationException("Value is not an object");
    public DsArray AsArray() => _ref as DsArray ?? throw new InvalidOperationException("Value is not an array");
    public DsFunction AsFunction() => _ref as DsFunction ?? throw new InvalidOperationException("Value is not a function");
    public IntPtr AsNativeFn() => _ref is IntPtr ptr ? ptr : IntPtr.Zero;

    // Try-get methods (no exceptions)
    public bool TryAsObject(out DsObject? obj)
    {
        obj = _ref as DsObject;
        return obj != null;
    }

    public bool TryAsArray(out DsArray? arr)
    {
        arr = _ref as DsArray;
        return arr != null;
    }

    /// <summary>
    /// Get truthiness for conditionals.
    /// </summary>
    public bool IsTruthy()
    {
        return Type switch
        {
            ValueType.Null => false,
            ValueType.Bool => _int != 0,
            ValueType.Int32 => _int != 0,
            ValueType.Float32 => _float != 0.0f,
            ValueType.String => !string.IsNullOrEmpty(_ref as string),
            ValueType.Object => _ref != null,
            ValueType.Array => _ref != null,
            ValueType.Function => _ref != null,
            ValueType.NativeFn => _ref != null,
            _ => false
        };
    }

    /// <summary>
    /// Convert to string for printing/concatenation.
    /// </summary>
    public override string ToString()
    {
        return Type switch
        {
            ValueType.Null => "null",
            ValueType.Bool => _int != 0 ? "true" : "false",
            ValueType.Int32 => _int.ToString(),
            ValueType.Float32 => _float.ToString("G"),
            ValueType.String => _ref as string ?? "",
            ValueType.Object => _ref?.ToString() ?? "[Object]",
            ValueType.Array => _ref?.ToString() ?? "[Array]",
            ValueType.Function => _ref?.ToString() ?? "[Function]",
            ValueType.NativeFn => "[NativeFn]",
            _ => "[Unknown]"
        };
    }

    /// <summary>
    /// Compare two values for equality with type coercion.
    /// </summary>
    public bool EqualsWithCoercion(Value other)
    {
        if (Type != other.Type)
        {
            // Allow int/float comparison
            if (IsNumeric && other.IsNumeric)
            {
                return Math.Abs(ToFloat() - other.ToFloat()) < float.Epsilon;
            }
            return false;
        }

        return Type switch
        {
            ValueType.Null => true,
            ValueType.Bool => _int == other._int,
            ValueType.Int32 => _int == other._int,
            ValueType.Float32 => Math.Abs(_float - other._float) < float.Epsilon,
            ValueType.String => (_ref as string) == (other._ref as string),
            ValueType.Object => ReferenceEquals(_ref, other._ref),
            ValueType.Array => ReferenceEquals(_ref, other._ref),
            ValueType.Function => ReferenceEquals(_ref, other._ref),
            ValueType.NativeFn => _ref?.Equals(other._ref) ?? false,
            _ => false
        };
    }

    /// <summary>
    /// Convert to float (for arithmetic).
    /// </summary>
    public float ToFloat()
    {
        return Type switch
        {
            ValueType.Int32 => _int,
            ValueType.Float32 => _float,
            _ => 0f
        };
    }

    /// <summary>
    /// Convert to int (for array indexing, etc.).
    /// </summary>
    public int ToInt()
    {
        return Type switch
        {
            ValueType.Int32 => _int,
            ValueType.Float32 => (int)_float,
            ValueType.Bool => _int,
            _ => 0
        };
    }

    /// <summary>
    /// Create a deep copy of this value (for objects and arrays).
    /// </summary>
    public Value DeepCopy()
    {
        return Type switch
        {
            ValueType.Object when _ref is DsObject obj => Object(DeepCopyObject(obj)),
            ValueType.Array when _ref is DsArray arr => Array(DeepCopyArray(arr)),
            _ => this
        };
    }

    private static DsObject DeepCopyObject(DsObject obj)
    {
        var clone = new DsObject(obj.ClassName);
        foreach (var (key, value) in obj.Fields)
        {
            clone.Fields[key] = value.DeepCopy();
        }
        return clone;
    }

    private static DsArray DeepCopyArray(DsArray arr)
    {
        var clone = new DsArray();
        foreach (var value in arr.Elements)
        {
            clone.Elements.Add(value.DeepCopy());
        }
        return clone;
    }

    // Arithmetic operators
    public static Value operator +(Value left, Value right)
    {
        if (left.IsString || right.IsString)
        {
            return String(left.ToString() + right.ToString());
        }
        if (left.IsFloat32 || right.IsFloat32)
        {
            return Float32(left.ToFloat() + right.ToFloat());
        }
        return Int32(left._int + right._int);
    }

    public static Value operator -(Value left, Value right)
    {
        if (left.IsFloat32 || right.IsFloat32)
        {
            return Float32(left.ToFloat() - right.ToFloat());
        }
        return Int32(left._int - right._int);
    }

    public static Value operator *(Value left, Value right)
    {
        if (left.IsFloat32 || right.IsFloat32)
        {
            return Float32(left.ToFloat() * right.ToFloat());
        }
        return Int32(left._int * right._int);
    }

    public static Value operator /(Value left, Value right)
    {
        var r = right.ToFloat();
        if (Math.Abs(r) < float.Epsilon)
            return Float32(float.NaN);
        if (left.IsFloat32 || right.IsFloat32)
        {
            return Float32(left.ToFloat() / r);
        }
        return Int32((int)(left._int / r));
    }

    public static Value operator %(Value left, Value right)
    {
        if (left.IsFloat32 || right.IsFloat32)
        {
            return Float32(left.ToFloat() % right.ToFloat());
        }
        return Int32(left._int % right._int);
    }

    public static Value operator -(Value value)
    {
        return value.IsFloat32 ? Float32(-value._float) : Int32(-value._int);
    }

    // Comparison operators
    public static Value operator <(Value left, Value right)
    {
        if (left.IsFloat32 || right.IsFloat32)
            return Bool(left.ToFloat() < right.ToFloat());
        return Bool(left._int < right._int);
    }

    public static Value operator <=(Value left, Value right)
    {
        if (left.IsFloat32 || right.IsFloat32)
            return Bool(left.ToFloat() <= right.ToFloat());
        return Bool(left._int <= right._int);
    }

    public static Value operator >(Value left, Value right)
    {
        if (left.IsFloat32 || right.IsFloat32)
            return Bool(left.ToFloat() > right.ToFloat());
        return Bool(left._int > right._int);
    }

    public static Value operator >=(Value left, Value right)
    {
        if (left.IsFloat32 || right.IsFloat32)
            return Bool(left.ToFloat() >= right.ToFloat());
        return Bool(left._int >= right._int);
    }

    // Override GetHashCode
    public override int GetHashCode()
    {
        return HashCode.Combine(Type, _ref, _int, _float);
    }
}
