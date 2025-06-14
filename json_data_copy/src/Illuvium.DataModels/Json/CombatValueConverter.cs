using System;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Illuvium.DataModels.Json;

public class CombatValueConverter : JsonConverter<CombatValue>
{
    public override CombatValue Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        return reader.TokenType switch
        {
            JsonTokenType.Number => new CombatValue(reader.GetInt32()),
            JsonTokenType.String => new CombatValue(reader.GetString()),
            _ => throw new JsonException($"Encountered unexpected token of type {reader.TokenType}"),
        };
    }

    public override void Write(Utf8JsonWriter writer, CombatValue value, JsonSerializerOptions options)
    {
        if (value.TryGetInt(out var intVal))
            writer.WriteNumberValue(intVal);
        else
            writer.WriteStringValue(value.GetString());
    }
}