using System.ComponentModel.DataAnnotations;
using System.Text.Json;
using Json.More;
using Json.Schema;
using SchemaTool;

if (!ParseArgs(args, out var rootDir, out var outputDir))
{
    return -1;
}

var generateFiles = outputDir != null;

// Tell JsonSchema how to resolve our references. (assuming that all referenced schemas are local files)
SchemaRegistry.Global.Fetch = uri => JsonSchema.FromFile(uri.LocalPath);

// assuming a specific folder structure here: for a given root '$', schemas will be in $/schemas,
// and each schema will correspond by name to another subfolder with the instances to validate.
// E.g. $/schemas/Foo.schema.json -> $/data/Foo/*.json
var schemaFiles = Directory.GetFiles(Path.Combine(rootDir, "schemas"), "*.schema.json");
var outputFiles = new Dictionary<string, string>();

Result result = new Result();

foreach (var schemaPath in schemaFiles)
{
    var fileName = Path.GetFileName(schemaPath);
    Console.WriteLine($"Validating {fileName}...");
    JsonSchema schema;
    try
    {
        schema = JsonSchema.FromFile(schemaPath);
    }
    catch (Exception ex)
    {
        Console.WriteLine($"Invalid schema: {ex.Message}");
        continue;
    }

    var baseName = fileName[..fileName.IndexOf(".schema.json", StringComparison.Ordinal)];
    var schemaValidationResult = ValidateSchema(schema, Path.Combine(rootDir, "data", baseName));
    if (schemaValidationResult.IsSuccess && generateFiles)
    {
        using var doc = schema.ToJsonDocument();
        var dtoFile = GenerateDtoFile(doc);
        outputFiles.Add($"{baseName}.cs", dtoFile);
    }
    Console.WriteLine();

    result.Increment(schemaValidationResult);
}

if (result.IsSuccess && generateFiles && outputDir != null)
{
    Console.WriteLine("Writing files...");
    Directory.CreateDirectory(outputDir);
    foreach (var (fileName, fileText) in outputFiles)
    {
        File.WriteAllText(Path.Combine(outputDir, fileName), fileText);
    }
}

PrintSummary();

return result.IsSuccess ? 0 : -1;


void PrintSummary()
{
    var message = result.IsSuccess
        ? $"Success! All {result.TotalValidFiles} file(s) are valid."
        : $"Validation failed! {result.TotalFiles - result.TotalValidFiles} files out of {result.TotalFiles} are NOT valid.";

    Console.WriteLine(message);
}

bool ParseArgs(string[] args, out string jsonBaseDir, out string? codeGenDir)
{
    string? jsonDir = null;
    codeGenDir = null;
    // call should be "SchemaTool <JSON base dir> [-g <C# output dir>]"
    for (var i = 0; i < args.Length; i++)
    {
        switch (args[i])
        {
            case "-g":
                codeGenDir = args[i + 1];
                i++;
                break;
            default:
                jsonDir = args[i];
                break;
        }
    }

    if (jsonDir == null || !Directory.Exists(jsonDir))
    {
        Console.WriteLine("Invalid JSON base directory");
        jsonBaseDir = string.Empty;
        return false;
    }
    jsonBaseDir = jsonDir;
    return true;
}

Result ValidateSchema(JsonSchema schema, string instanceDir)
{
    var options = new ValidationOptions { OutputFormat = OutputFormat.Basic };
    var validationResult = new Result();

	// Iterate over all the directories recursively
    foreach (var jsonFile in Directory.EnumerateFiles(instanceDir, "*.json", SearchOption.AllDirectories))
    {
        validationResult.TotalFiles++;
        ValidationResults results;
        Console.Write($"\t{Path.GetRelativePath(instanceDir, jsonFile)}... ");
        try
        {
            var fs = new FileStream(jsonFile, FileMode.Open, FileAccess.Read);
            using var instance = JsonDocument.Parse(fs);
            results = schema.Validate(instance, options);
        }
        catch (JsonException ex)
        {
            Console.WriteLine("failed!");
            Console.WriteLine();
            Console.WriteLine($"Parse error: {ex.Message}");
            Console.WriteLine();
            continue;
        }

        Console.WriteLine($"{(results.IsValid ? "OK" : "failed!")}");

        if (results.IsValid)
            validationResult.TotalValidFiles++;

        if (!results.IsValid)
        {
            Console.WriteLine();
            PrintResults(results);
            Console.WriteLine();
        }
    }

    return validationResult;
}

string GenerateDtoFile(JsonDocument schema)
{
    Console.WriteLine("\tGenerating classes...");
    return new DtoFileBuilder()
        .WithSchema(schema)
        .WithNamespace("Illuvium.DataModels")
        .Build();
}

void PrintResults(ValidationResults results)
{
    if (!results.HasNestedResults)
        Console.WriteLine($"{results.InstanceLocation.Source}: {results.Message}");
    else
        foreach (var result in results.NestedResults)
            PrintResults(result);
}
