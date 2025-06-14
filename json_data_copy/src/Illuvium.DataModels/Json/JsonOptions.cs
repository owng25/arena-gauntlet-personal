using System.Text.Encodings.Web;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Illuvium.DataModels.Json;

/// <summary>
/// Contains common System.Text.Json serializer options. It is performance essential to reuse these rather than creating a new
/// option on every json serialisation/deserialisation. JsonSerializerOptions are also threadsafe and immutable after first use.
/// </summary>
/// <remarks>
/// Changes to these options can have small, potentially unpredictable changes
/// over a large amount of code. Test throughly!
/// </remarks>
public static class JsonOptions
{
    public static readonly JsonSerializerOptions GameData = new()
    {
        DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
        Converters =
        {
            new CombatValueConverter(),
            new CombatExpressionConverter(),
            new DerivedTypeConverter<CombatSynergyBonus>(),
        },
    };

    public static readonly JsonSerializerOptions CombatUnitTypes = new()
    {
        DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        PropertyNameCaseInsensitive = true,
        Converters =
        {
            new JsonStringEnumConverter(),
            new CombatExpressionConverter(),
            new CombatValueConverter(),
        },
        Encoder = JavaScriptEncoder.UnsafeRelaxedJsonEscaping,
    };
}
