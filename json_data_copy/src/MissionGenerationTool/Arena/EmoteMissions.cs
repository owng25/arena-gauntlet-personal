using Illuvium.DataModels;

namespace MissionGenerationTool;

public static class EmoteMissions
{
	public static IEnumerable<MissionTemplate> GenerateDailyMissions()
	{
		yield return CreateMissionTemplate("Daily",null, new EmoteObjectiveTemplate
		{
			QuantityMinimum = 10,
			QuantityMaximum = 60,
			QuantityStep = 10
		}, new AirdropRewardTemplate
		{
			PointsBase = 40,
			PointsStep = 40
		});
	}

	public static IEnumerable<MissionTemplate> GenerateWeeklyMissions()
	{
		yield break;
	}

	public static IEnumerable<MissionTemplate> GenerateMilestoneMissions()
	{
		yield break;
	}

	private static MissionTemplate CreateMissionTemplate(string type, string? emoteType, EmoteObjectiveTemplate objective, AirdropRewardTemplate reward)
	{
		var detail = $"Use {{0}} {emoteType} Emotes".Replace("  ", " ").Replace("  ", " ");
		return new MissionTemplate
		{
			ProductionId = $"{type}_Arena_Emotes_{(emoteType is not null ? emoteType.Replace(" ", "_") : string.Empty)}".Replace("__", "_").Replace("__", "_"),
			Name = detail,
			Description = detail,
			Type = type,
			MissionTier = 0,
			Game = "Arena",
			Objectives = new[]
			{
				objective with
				{
					Description = detail,
					EmoteType = emoteType,
				}
			},
			Rewards = new[]
			{
				reward
			}
		};
	}
}
