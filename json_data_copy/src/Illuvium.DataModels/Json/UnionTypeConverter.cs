using System;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Illuvium.DataModels.Json;

public abstract class UnionTypeConverter<T, TA, TB> : JsonConverter<T>
{
    public override T Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        return IsOfTypeA(ref reader, typeToConvert, options)
            ? Create(JsonSerializer.Deserialize<TA>(ref reader, options))
            : Create(JsonSerializer.Deserialize<TB>(ref reader, options));
    }

    public override void Write(Utf8JsonWriter writer, T value, JsonSerializerOptions options)
    {
        if (IsOfTypeA(value))
            JsonSerializer.Serialize(writer, GetA(value), options);
        else
            JsonSerializer.Serialize(writer, GetB(value), options);
    }

    protected abstract bool IsOfTypeA(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options);
    protected abstract bool IsOfTypeA(T value);

    protected abstract T Create(TA a);
    protected abstract T Create(TB b);

    protected abstract TA GetA(T t);
    protected abstract TB GetB(T t);
}