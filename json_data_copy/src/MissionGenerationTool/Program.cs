using Amazon;
using Amazon.DynamoDBv2;
using Amazon.DynamoDBv2.Model;
using Illuvium.DataModels;
using MissionGenerationTool;
using MissionGenerationTool.Overworld;
using MissionGenerationTool.Zero;

if (!ParseArgs(args, out var rootDir, out var outputDir))
{
	return -1;
}

var client = new AmazonDynamoDBClient(new AmazonDynamoDBConfig
{
	Profile = new Profile(Environment.GetEnvironmentVariable("AWS_PROFILE") ?? "illuvium"),
	RegionEndpoint = RegionEndpoint.GetBySystemName("us-east-1")
});

foreach (var table in (await client.ListTablesAsync()).TableNames)
	Console.WriteLine(table);

var items = new List<Item>();

var excludeItems = new[] {
	// General
	"Pistol",
	// Illuvials
	"Rypterus", "Squizz", "Vermilliare",
	// Drones
	"Mozart", "Chopin", "Bach", "Hilde"
};

Dictionary<string, AttributeValue>? cursor = null;
do
{
	var condition = "#pk = :pk";
	var names = new Dictionary<string, string> { ["#pk"] = "PK" };
	var values = new Dictionary<string, AttributeValue> { [":pk"] = new("WorldData#Items") };
	var request = new QueryRequest("int-illuvium-core")
	{
		ExclusiveStartKey = cursor,
		KeyConditionExpression = condition,
		ExpressionAttributeNames = names,
		ExpressionAttributeValues = values
	};

	var response = await client.QueryAsync(request);
	cursor = response.LastEvaluatedKey;

	foreach (var result in response.Items)
	{
		string? displayName = null;
		string? name = null;
		string? itemType = null;
		string? line = null;
		string? finish = null;
		int? stage = null;
		int? tier = null;
		bool isAugment = false;

		foreach (var (key, value) in result)
		{
			switch (key)
			{
				case "DisplayName":
					displayName = value.S;
					break;
				case "Name":
					name = value.S;
					break;
				case "ItemType":
					itemType = value.S;
					break;
				case "Line":
					line = value.S;
					break;
				case "Finish":
					finish = value.S;
					break;
				case "Stage":
					stage = int.Parse(value.N);
					break;
				case "Tier":
					tier = int.Parse(value.N);
					break;
				case "ComponentType":
					isAugment = true;
					break;
			}
		}

		if (isAugment)
		{
			Console.WriteLine("Skipping augment: {0}", displayName);
			continue;
		}

		if (excludeItems.Any(x => name?.StartsWith(x + "_") ?? false) || new [] {"Skin"}.Contains(itemType))
		{
			Console.WriteLine("Discarded item: {0}", name ?? displayName);
			continue;
		}

		var item = new Item(displayName, name, itemType, line, stage, tier, finish);
		items.Add(item);
		//Console.WriteLine("Discovered item: {0}", item);
	}

} while (cursor is { Count: > 0 });

Console.WriteLine("Discovered {0} items", items.Count);

outputDir ??= Path.Combine(rootDir, "data", "MissionTemplateData");

var missions = new List<MissionTemplate>();
missions.AddRange(OverworldMissions.Generate(items));
missions.AddRange(ArenaMissions.Generate());
missions.AddRange(ZeroMissions.Generate());

// Balancing
var finalMissions = new List<MissionTemplate>(missions.Count);
foreach (var game in new[] { "Overworld", "Arena", "Zero" })
{
	var gameMissions = missions.Where(x => x.Game == game).ToList();
	foreach (var type in new[] { "Daily", "Weekly", "Milestone", "Faction", "Event", "Quest", "Collectible" })
	{
		var filteredMissions = gameMissions.Where(x => x.Type == type).ToList();
		if (filteredMissions.Count == 0)
			continue;

		if (!new[] { "Daily", "Weekly" }.Contains(type))
		{
			finalMissions.AddRange(filteredMissions.Select(m => m with { Weight = 1 }));
			continue;
		}

		var count = filteredMissions.SelectMany(x => x.Objectives).GroupBy(x => x.GetType().Name).ToDictionary(x => x.Key, x => x.Count());

		var maxCount = count.Values.Max();
		foreach (var mission in filteredMissions)
		{
			var objective = mission.Objectives.First();
			var weight = (int)(maxCount / count[objective.GetType().Name]);
			finalMissions.Add(mission with { Weight = Math.Min(weight, 200) });
		}
	}
}


if (Directory.Exists(outputDir))
	Directory.Delete(outputDir, true);

Directory.CreateDirectory(outputDir);

foreach (var mission in finalMissions)
	MissionSerializer.SerializeMission(mission, outputDir);

Console.WriteLine("Written: " + missions.Count);

Console.WriteLine("-----");
var ow = missions.Where(x => x.Game == "Overworld");
Console.WriteLine("Overworld Missions");

var owData = string.Join(" ", ow.GroupBy(x => x.Type).Select(x => string.Join(", ", x.GroupBy(y => y.MissionTier.GetValueOrDefault()).OrderBy(x => x.Key).Select(y => $"{x.Key} T{y.Key}: {y.Count()}"))));
Console.WriteLine($"  Count: {ow.Count()} --> {owData}");

var ar = missions.Where(x => x.Game == "Arena");
Console.WriteLine("Arena Missions");

var arData = string.Join(" ", ar.GroupBy(x => x.Type).Select(x => string.Join(", ", x.GroupBy(y => y.MissionTier.GetValueOrDefault()).OrderBy(x => x.Key).Select(y => $"{x.Key} T{y.Key}: {y.Count()}"))));
Console.WriteLine($"  Count: {ar.Count()} --> {arData}");

var zr = missions.Where(x => x.Game == "Zero");
Console.WriteLine("Zero Missions");

var zrData = string.Join(" ", zr.GroupBy(x => x.Type).Select(x => string.Join(", ", x.GroupBy(y => y.MissionTier.GetValueOrDefault()).OrderBy(x => x.Key).Select(y => $"{x.Key} T{y.Key}: {y.Count()}"))));
Console.WriteLine($"  Count: {zr.Count()} --> {zrData}");
Console.WriteLine("-----");

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

return 0;

public record Item(string? DisplayName, string? Name, string? ItemType, string? Line, int? Stage, int? Tier, string? Finish)
{
	public string GetDisplayName()
	{
		return  DisplayName ?? Name ?? "";
	}
}