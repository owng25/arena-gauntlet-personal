using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace SchemaTool;

internal class Dto
{
    public string Name { get; set; }
    public List<DtoProperty>? Properties { get; set; }
    public List<string>? EnumValues { get; set; }
    public List<string>? Attributes { get; set; }
    public string? Parent { get; set; }
    public string? Summary { get; set; }
    public bool IsUnion { get; set; }

    public Dto(string name)
    {
        Name = name;
    }
}

internal class DtoProperty
{
    public string Name { get; set; }
    public string Type { get; set; }
    public string? Summary { get; set; }
    public List<string>? Attributes { get; set; }

    public DtoProperty(string name, string type)
    {
        Name = name;
        Type = type;
    }
}

internal class DtoFileBuilder
{
    // We disable enum generation as consuming code (GameServer) would need a few changes to support it
    public const bool DisableEnumGeneration = true;

    private readonly StringBuilder _builder = new();
    private readonly List<Dto> _dtos = new();
    private readonly HashSet<string> _usings = new();
    private JsonDocument? _schema;
    private string _namespace = "Illuvium.DataModels";

    #region initializers

    public DtoFileBuilder WithSchema(JsonDocument schema)
    {
        _schema = schema;
        return this;
    }

    public DtoFileBuilder WithNamespace(string ns)
    {
        _namespace = ns;
        return this;
    }

    #endregion

    public string Build()
    {
        if (_schema == null) throw new Exception("No schema!");
        // To make the code generation easier, we'll only generate classes from the defs.  The top-level schema should have only a ref or oneOf a group of refs.
        // It's a little extra work for simple schemas, but it solves the problem of naming the classes.

        if (_schema.RootElement.TryGetProperty("$defs", out var defs))
        {
            foreach (var def in defs.EnumerateObject())
                ParseDto(def.Value, def.Name);
        }

        _builder.Clear();
        WriteHeader();
        WriteDtos();
        return _builder.ToString();
    }

    #region parsing

    private void ParseDto(JsonElement element, string name)
    {
        var summary = element.TryGetProperty("description", out var description) ? description.GetString() : null;
        var required = element.TryGetProperty("required", out var requiredElement) ? requiredElement.EnumerateArray().Select(x => x.GetString() ?? "") : null;
        var properties = element.TryGetProperty("properties", out var propsElement) ? ParseProperties(propsElement, required) : null;
        var enumValues = element.TryGetProperty("enum", out var enumElement)
            ? enumElement.EnumerateArray().Select(x => x.GetString() ?? throw new Exception($"invalid enum value {x}"))
            : null;

        var isUnion = false;
        if (properties == null && element.TryGetProperty("oneOf", out var oneOfElement))
        {
            isUnion = true;
            properties = oneOfElement.EnumerateArray().Select(e =>
            {
                var type = ParsePropertyType(e, false, out _) ?? throw new Exception($"Cannot oneOf type for object {name}");
                return new DtoProperty(type[..1].ToUpper() + type[1..], type);
            });
        }

        var dto = new Dto(name)
        {
            Summary = summary,
            Properties = properties?.ToList(),
            EnumValues = enumValues?.ToList(),
            IsUnion = isUnion
        };
		if (element.TryGetProperty("allOf", out var allOf))
        {
            // We're using allOf to signify inheritance here, assuming there's only one parent
            if (allOf.ValueKind == JsonValueKind.Array && allOf.GetArrayLength() == 1 && allOf[0].TryGetProperty("$ref", out var refElement))
            {
                dto.Parent = GetRefType(refElement);
            }
            else
            {
                throw new Exception($"Failed to parse type {name}, unrecognized allOf");
            }
        }

		if (element.TryGetProperty("$ref", out var dtoRefElement))
		{
			dto.Parent = GetRefType(dtoRefElement);
		}

        // not a standard property, but we're using "versioned" to signify that the DTO uses versioned data
        if (element.TryGetProperty("x-versioned", out var versionedProp) && versionedProp.ValueKind == JsonValueKind.True)
        {
			if (dto.Parent is not null)
			{
				dto.Parent = $"{dto.Parent}, IVersionedData";
			}
			else
			{
				dto.Parent = "IVersionedData";
			}
            dto.Properties ??= new List<DtoProperty>();
            dto.Properties.Add(new DtoProperty("Version", "string"));
        }

        _dtos.Add(dto);
    }

    private IEnumerable<DtoProperty> ParseProperties(JsonElement propertiesElement, IEnumerable<string>? required)
    {
        return propertiesElement.EnumerateObject().Select(jsonProp =>
        {
            var type = ParsePropertyType(jsonProp.Value, true, out var constraints) ?? throw new Exception($"Cannot parse type for property {jsonProp.Name}");
            var summary = jsonProp.Value.TryGetProperty("description", out var description) ? description.GetString() : null;
            var attributes = new List<string>();
            if (required?.Contains(jsonProp.Name) ?? false)
            {
                attributes.Add("Required");
                _usings.Add("System.ComponentModel.DataAnnotations");
            }

            if (constraints != null)
            {
                attributes.AddRange(constraints);
                _usings.Add("System.ComponentModel.DataAnnotations");
            }

            return new DtoProperty(jsonProp.Name, type)
            {
                Summary = summary,
                Attributes = attributes.Any() ? attributes : null
            };
        });
    }

    private string? ParsePropertyType(JsonElement element, bool nullable, out List<string>? constraints)
    {
        constraints = null;
        if (element.TryGetProperty("type", out var typeElement))
        {
            switch (typeElement.GetString())
            {
                case "string":
                    if (element.TryGetProperty("minLength", out var minLength))
                    {
                        constraints ??= new List<string>();
                        constraints.Add($"MinLength({minLength.GetInt32()})");
                    }
                    if (element.TryGetProperty("maxLength", out var maxLength))
                    {
                        constraints ??= new List<string>();
                        constraints.Add($"MaxLength({maxLength.GetInt32()})");
                    }
                    return "string";
                case "integer":
                {
                    var min = element.TryGetProperty("minimum", out var minElement) ? (int?) minElement.GetInt32() : null;
                    var max = element.TryGetProperty("maximum", out var maxElement) ? (int?) maxElement.GetInt32() : null;
                    if (min != null || max != null)
                    {
                        constraints = new List<string>
                            { $"Range({min?.ToString() ?? "int.MinValue"}, {max?.ToString() ?? "int.MaxValue"})" };
                    }

                    return nullable ? "int?" : "int";
                }
                case "number":
                {
                    var min = element.TryGetProperty("minimum", out var minElement) ? (decimal?) minElement.GetDecimal() : null;
                    var max = element.TryGetProperty("maximum", out var maxElement) ? (decimal?) maxElement.GetDecimal() : null;
                    if (min != null || max != null)
                    {
                        constraints = new List<string> { $"Range({min?.ToString() ?? "double.MinValue"}, {max?.ToString() ?? "double.MaxValue"})" };
                    }

                    return nullable ? "decimal?" : "decimal";
                }
                case "boolean":
                    return nullable ? "bool?" : "bool";
                case "array":
                    var itemsElement = element.GetProperty("items");
                    var itemType = ParsePropertyType(itemsElement, false, out _); // no constraints on array elements
                    _usings.Add("System.Collections.Generic");
                    return $"IEnumerable<{itemType}>";
				case "object":
					throw new Exception($"Unexpected object type. This can be fixed if you move your definition of the type to be inside the $defs and just reference it.");
				default:
                    throw new Exception($"Unexpected property type: {typeElement.GetString()}");
            }
        }

        if (element.TryGetProperty("$ref", out var refElement))
        {
            return GetRefType(refElement);
        }

        if (element.TryGetProperty("additionalProperties", out var addProps) && addProps.ValueKind == JsonValueKind.Object)
        {
            var itemsType = ParsePropertyType(addProps, false, out _);
            _usings.Add("System.Collections.Generic");
            return $"IReadOnlyDictionary<string, {itemsType}>";
        }

        return null;
    }

    private string GetRefType(JsonElement refElement)
    {
        var refPath = refElement.GetString() ?? "";
        var match = Regex.Match(refPath, "^(?<schema>.*)#/\\$defs/(?<def>\\w+)$");
        if (!match.Success) throw new Exception($"Invalid reference element: {refElement}");
        var refType = match.Groups["def"].Value;

        if (DisableEnumGeneration && refType.EndsWith("Enum"))
        {
            return "string";
        }

        return refType;
    }

    #endregion

    #region output

    private void WriteHeader()
    {
        _builder.AppendLine("// Generated code. Do not edit.");
        _builder.AppendLine();
        foreach (var u in _usings.OrderBy(x => x))
        {
            _builder.AppendLine($"using {u};");
        }
        _builder.AppendLine();
        _builder.AppendLine("// ReSharper disable once CheckNamespace"); // R# doesn't like when the namespace doesn't match the folder name
        _builder.AppendLine($"namespace {_namespace};");
        _builder.AppendLine();
    }

    private void WriteDtos()
    {
        foreach (var dto in _dtos.OrderBy(x => x.Name))
        {
            WriteSummary(dto.Summary);
            WriteAttributes(dto.Attributes);
            if (dto.IsUnion)
            {
                WriteUnion(dto);
            }
            else if (dto.EnumValues?.Any() ?? false)
            {
                WriteEnum(dto);
            }
            else
            {
                WriteDto(dto);
            }
        }
    }

    private void WriteDto(Dto dto)
    {
        _builder.Append($"public record {dto.Name}");
        if (!string.IsNullOrEmpty(dto.Parent))
            _builder.Append($" : {dto.Parent}");
        _builder.AppendLine();
        _builder.AppendLine("{");
        WriteProperties(dto);
        _builder.AppendLine("}");
        _builder.AppendLine();
    }

    private void WriteEnum(Dto dto)
    {
        if (DisableEnumGeneration)
            return;

        if (dto.EnumValues == null) throw new InvalidOperationException("Expected enum values to be non-null");
        _builder.AppendLine($"public enum {dto.Name}");
        _builder.AppendLine("{");
        var values = string.Join(",\n", dto.EnumValues.Select(v => $"  {v}"));
        _builder.AppendLine(values);
        _builder.AppendLine("}");
        _builder.AppendLine();
    }

    private void WriteProperties(Dto dto)
    {
        if (dto.Properties == null) return;
        foreach (var property in dto.Properties.OrderBy(x => x.Name))
        {
            WriteSummary(property.Summary, "  ");
            WriteAttributes(property.Attributes, "  ");
            _builder.AppendLine($"  public {property.Type} {property.Name} {{ get; init; }}");
        }
    }

    private void WriteUnion(Dto dto)
    {
        _builder.AppendLine($"public record {dto.Name} : UnionType");
        _builder.AppendLine("{");
        foreach (var property in dto.Properties ?? new List<DtoProperty>())
        {
            _builder.AppendLine($"  public {dto.Name} ({property.Type} value) : base(value, typeof({property.Type})) {{ }}");
            _builder.AppendLine($"  public bool Is{property.Name}() => IsType<{property.Type}>();");
            _builder.AppendLine($"  public {property.Type} Get{property.Name}() => GetValue<{property.Type}>();");
            _builder.AppendLine($"  public bool TryGet{property.Name}(out {property.Type} value) => TryGetValue(out value);");
            _builder.AppendLine($"  public static explicit operator {property.Type}({dto.Name} x) => x.Get{property.Name}();");
            _builder.AppendLine();
        }
        _builder.AppendLine("}");
    }

    private void WriteSummary(string? summary, string indent = "")
    {
        if (string.IsNullOrEmpty(summary)) return;

        _builder.AppendLine($"{indent}/// <summary>");
        _builder.AppendLine($"{indent}/// {summary}");
        _builder.AppendLine($"{indent}/// </summary>");
    }

    private void WriteAttributes(IEnumerable<string>? attributes, string indent = "")
    {
        if (attributes == null) return;
        foreach (var attribute in attributes)
            _builder.AppendLine($"{indent}[{attribute}]");
    }

    #endregion
}
