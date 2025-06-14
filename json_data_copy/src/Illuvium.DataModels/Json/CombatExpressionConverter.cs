using System;
using System.Text.Json;

namespace Illuvium.DataModels.Json;

public class CombatExpressionConverter : UnionTypeConverter<CombatExpression, CombatExpressionObject, CombatValue>
{
    protected override bool IsOfTypeA(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        return reader.TokenType == JsonTokenType.StartObject;
    }

    protected override bool IsOfTypeA(CombatExpression value) => value.IsCombatExpressionObject();

    protected override CombatExpression Create(CombatExpressionObject a) => new(a);

    protected override CombatExpression Create(CombatValue b) => new(b);

    protected override CombatExpressionObject GetA(CombatExpression t) => t.GetCombatExpressionObject();

    protected override CombatValue GetB(CombatExpression t) => t.GetCombatValue();
}