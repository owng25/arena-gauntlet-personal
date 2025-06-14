using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;
using Illuvium.DataModels;

namespace MissionGenerationTool;

public static class MissionSerializer
{
	private static JsonSerializerOptions? _options = null;

	public static void SerializeMission(MissionTemplate mission, string outputDir)
	{
		if (_options is null)
		{
			_options = new JsonSerializerOptions
			{
				DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
				WriteIndented = true
			};
			_options.Converters.Add(new JsonStringEnumConverter());
		}

		var tags = new JsonArray();
		foreach (var tag in mission.Tags ?? Enumerable.Empty<string>())
		{
			if (string.IsNullOrWhiteSpace(tag))
				continue;

			tags.Add(tag);
		}

		var json = new JsonObject();
		json["ProductionId"] = mission.ProductionId;
		json["Name"] = mission.Name;
		json["Description"] = mission.Description;
		if (mission.Expiry is not null)
			json["Expiry"] = mission.Expiry;

		json["Quantity"] = mission.Quantity.GetValueOrDefault(1);
		json["Tags"] = tags;
		json["Type"] = mission.Type;
		json["Weight"] = mission.Weight;
		json["Game"] = mission.Game;
		json["MissionTier"] = mission.MissionTier.GetValueOrDefault(0);

		if (mission.Conditions is not null)
		{
			json["Conditions"] = new JsonArray(mission.Conditions.Select(x =>
			{
				var t = x.GetType();
				var obj = JsonSerializer.Serialize(x, t, _options);
				// .net6 doesn't support this so we need to force it back
				var json = JsonSerializer.Deserialize<JsonNode>(obj);
				json["Type"] = t.Name.Replace("Template", "");
				return json;
			}).ToArray());
		}

		json["Objectives"] = new JsonArray(mission.Objectives.Select(x =>
		{
			var t = x.GetType();
			var obj = JsonSerializer.Serialize(x, t, _options);
			// .net6 doesn't support this so we need to force it back
			var json = JsonSerializer.Deserialize<JsonNode>(obj);
			json["Type"] = t.Name.Replace("Template", "");
			return json;
		}).ToArray());

		json["Rewards"] = new JsonArray(mission.Rewards.Select(x =>
		{
			if (x is AirdropRewardTemplate airdrop && airdrop.PointsBase.HasValue)
			{
				if (!airdrop.PointsStep.HasValue)
				{
					Console.WriteLine($"WARNING: {mission.ProductionId} has PointsBase but no PointsStep. This will cause weird behavior in the game.");
				}

				if (mission.Objectives.Count() > 1)
				{
					Console.WriteLine($"WARNING: {mission.ProductionId} has multiple objectives and PointsBase. This will cause weird behavior in the game.");
				}
			}

			var t = x.GetType();
			var obj = JsonSerializer.Serialize(x, t, _options);
			// .net6 doesn't support this so we need to force it back
			var json = JsonSerializer.Deserialize<JsonNode>(obj);
			json["Type"] = t.Name.Replace("Template", "");
			return json;
		}).ToArray());

		var jsonStr = json.ToJsonString(_options);
		var path = Path.Combine(outputDir, $"{mission.ProductionId}.json");
		File.WriteAllText(path, jsonStr);
	}
}