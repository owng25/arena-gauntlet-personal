using System;

namespace Illuvium.DataModels;

// To aid with handling "oneOf" elements in a schema, we'll simulate a union
// by allowing initialization to any type/value, but only one.
public abstract record UnionType(object Value, Type Type)
{
    public bool IsType<T>()
    {
        return Type == typeof(T);
    }

    public T GetValue<T>()
    {
        return (T)Value;
    }

    public bool TryGetValue<T>(out T value)
    {
        if (IsType<T>())
        {
            value = GetValue<T>();
            return true;
        }
        value = default;
        return false;
    }
}
