using DialOS.Runtime.Values;

namespace DialOS.Tests;

public class ValueTests
{
    [Fact]
    public void Value_Null_ShouldBeNull()
    {
        var value = Value.Null();

        Assert.True(value.IsNull);
        Assert.False(value.IsTruthy());
        Assert.Equal("null", value.ToString());
    }

    [Fact]
    public void Value_Bool_ShouldBeBool()
    {
        var trueValue = Value.Bool(true);
        var falseValue = Value.Bool(false);

        Assert.True(trueValue.IsBool);
        Assert.True(trueValue.IsTruthy());
        Assert.True(trueValue.AsBool());

        Assert.True(falseValue.IsBool);
        Assert.False(falseValue.IsTruthy());
        Assert.False(falseValue.AsBool());
    }

    [Fact]
    public void Value_Int32_ShouldBeInt32()
    {
        var value = Value.Int32(42);

        Assert.True(value.IsInt32);
        Assert.Equal(42, value.AsInt32());
        Assert.True(value.IsTruthy());
        Assert.Equal("42", value.ToString());
    }

    [Fact]
    public void Value_Int32_ZeroShouldBeFalsy()
    {
        var value = Value.Int32(0);

        Assert.False(value.IsTruthy());
    }

    [Fact]
    public void Value_Float32_ShouldBeFloat32()
    {
        var value = Value.Float32(3.14f);

        Assert.True(value.IsFloat32);
        Assert.Equal(3.14f, value.AsFloat32(), 0.001);
        Assert.True(value.IsTruthy());
    }

    [Fact]
    public void Value_String_ShouldBeString()
    {
        var value = Value.String("Hello");

        Assert.True(value.IsString);
        Assert.Equal("Hello", value.AsString());
        Assert.True(value.IsTruthy());
    }

    [Fact]
    public void Value_String_EmptyShouldBeFalsy()
    {
        var value = Value.String("");

        Assert.False(value.IsTruthy());
    }

    [Fact]
    public void Value_Object_ShouldBeObject()
    {
        var obj = new DsObject("TestObject");
        var value = Value.Object(obj);

        Assert.True(value.IsObject);
        Assert.True(value.IsTruthy());
        Assert.Equal(obj, value.AsObject());
    }

    [Fact]
    public void Value_Array_ShouldBeArray()
    {
        var arr = new DsArray(5);
        var value = Value.Array(arr);

        Assert.True(value.IsArray);
        Assert.True(value.IsTruthy());
        Assert.Equal(arr, value.AsArray());
    }

    [Fact]
    public void Value_Function_ShouldBeFunction()
    {
        var fn = new DsFunction(0, 2);
        var value = Value.Function(fn);

        Assert.True(value.IsFunction);
        Assert.True(value.IsTruthy());
        Assert.Equal(fn, value.AsFunction());
    }

    [Fact]
    public void Value_Equals_SameType_ShouldCompareCorrectly()
    {
        Assert.Equal(Value.Int32(42), Value.Int32(42));
        Assert.NotEqual(Value.Int32(42), Value.Int32(43));
        Assert.Equal(Value.Bool(true), Value.Bool(true));
        Assert.Equal(Value.String("test"), Value.String("test"));
        Assert.Equal(Value.Null(), Value.Null());
    }

    [Fact]
    public void Value_EqualsWithCoercion_NumericTypes_ShouldCompareAcrossTypes()
    {
        Assert.True(Value.Int32(42).EqualsWithCoercion(Value.Float32(42.0f)));
        Assert.False(Value.Int32(42).EqualsWithCoercion(Value.Float32(42.5f)));
    }

    [Fact]
    public void Value_Arithmetic_Addition()
    {
        Assert.Equal(Value.Int32(7), Value.Int32(3) + Value.Int32(4));
        Assert.Equal(Value.Float32(7.5f), Value.Float32(3.5f) + Value.Int32(4));
        // String concatenation only happens when at least one operand is a string
        Assert.Equal(Value.String("34"), Value.String("3") + Value.Int32(4));
    }

    [Fact]
    public void Value_Arithmetic_Subtraction()
    {
        Assert.Equal(Value.Int32(5), Value.Int32(8) - Value.Int32(3));
        Assert.Equal(Value.Float32(4.5f), Value.Float32(8.5f) - Value.Int32(4));
    }

    [Fact]
    public void Value_Arithmetic_Multiplication()
    {
        Assert.Equal(Value.Int32(12), Value.Int32(3) * Value.Int32(4));
        Assert.Equal(Value.Float32(14.0f), Value.Float32(3.5f) * Value.Int32(4));
    }

    [Fact]
    public void Value_Arithmetic_Division()
    {
        Assert.Equal(Value.Int32(2), Value.Int32(8) / Value.Int32(4));
        Assert.Equal(Value.Float32(2.125f), Value.Float32(8.5f) / Value.Int32(4));
    }

    [Fact]
    public void Value_Arithmetic_Modulo()
    {
        Assert.Equal(Value.Int32(2), Value.Int32(8) % Value.Int32(3));
    }

    [Fact]
    public void Value_Comparison_LessThan()
    {
        Assert.Equal(Value.Bool(true), Value.Int32(3) < Value.Int32(5));
        Assert.Equal(Value.Bool(false), Value.Int32(5) < Value.Int32(3));
    }

    [Fact]
    public void Value_Comparison_GreaterThan()
    {
        Assert.Equal(Value.Bool(true), Value.Int32(5) > Value.Int32(3));
        Assert.Equal(Value.Bool(false), Value.Int32(3) > Value.Int32(5));
    }

    [Fact]
    public void Value_ToFloat_ShouldConvert()
    {
        Assert.Equal(42.0f, Value.Int32(42).ToFloat());
        Assert.Equal(3.14f, Value.Float32(3.14f).ToFloat());
    }

    [Fact]
    public void Value_ToInt_ShouldConvert()
    {
        Assert.Equal(42, Value.Int32(42).ToInt());
        Assert.Equal(3, Value.Float32(3.9f).ToInt()); // Truncation
    }

    [Fact]
    public void DsObject_GetSetField()
    {
        var obj = new DsObject("Test");

        obj.SetField("name", Value.String("TestName"));
        Assert.Equal("TestName", obj.GetField("name").AsString());
        Assert.True(obj.HasField("name"));
        Assert.False(obj.HasField("nonexistent"));
    }

    [Fact]
    public void DsArray_Indexer()
    {
        var arr = new DsArray(3);

        arr[0] = Value.Int32(10);
        arr[1] = Value.Int32(20);
        arr[2] = Value.Int32(30);

        Assert.Equal(10, arr[0].AsInt32());
        Assert.Equal(20, arr[1].AsInt32());
        Assert.Equal(30, arr[2].AsInt32());
        Assert.Equal(3, arr.Length);
    }

    [Fact]
    public void DsArray_PushPop()
    {
        var arr = new DsArray();

        arr.Push(Value.Int32(1));
        arr.Push(Value.Int32(2));
        arr.Push(Value.Int32(3));

        Assert.Equal(3, arr.Length);
        Assert.Equal(Value.Int32(3), arr.Pop());
        Assert.Equal(2, arr.Length);
    }

    [Fact]
    public void DsFunction_Properties()
    {
        var fn = new DsFunction(5, 3);

        Assert.Equal(5, fn.FunctionIndex);
        Assert.Equal(3, fn.ParamCount);
    }
}
