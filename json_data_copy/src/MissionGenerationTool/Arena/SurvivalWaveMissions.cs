using Illuvium.DataModels;

namespace MissionGenerationTool;

public static class SurvivalWaveMissions
{
	private static int[] Tiers = Enumerable.Range(0, 5).ToArray();

	public static IEnumerable<MissionTemplate> GenerateDailyMissions()
	{
		var type = "Daily";
		yield return CreateMissionTemplate(type, new SurvivalWaveObjectiveTemplate
		{
			Challenge = ArenaGameMissions.SurvivalTraining,
			QuantityMinimum = 5,
			QuantityMaximum = 20,
			QuantityStep = 5
		}, new AirdropRewardTemplate
		{
			PointsBase = 130,
			PointsStep = 130
		});
		yield return CreateMissionTemplate(type, new SurvivalWaveObjectiveTemplate
		{
			Challenge = ArenaGameMissions.SurvivalCompetitive,
			QuantityMinimum = 5,
			QuantityMaximum = 20,
			QuantityStep = 5
		}, new AirdropRewardTemplate
		{
			PointsBase = 170,
			PointsStep = 170
		});
	}

	public static IEnumerable<MissionTemplate> GenerateWeeklyMissions()
	{
		var type = "Weekly";
		yield return CreateMissionTemplate(type, new SurvivalWaveObjectiveTemplate
		{
			Challenge = ArenaGameMissions.SurvivalTraining,
			QuantityMinimum = 9,
			QuantityMaximum = 27,
			QuantityStep = 3
		}, new AirdropRewardTemplate
		{
			PointsBase = 750,
			PointsStep = 750
		});
		yield return CreateMissionTemplate(type, new SurvivalWaveObjectiveTemplate
		{
			Challenge = ArenaGameMissions.SurvivalCompetitive,
			QuantityMinimum = 9,
			QuantityMaximum = 27,
			QuantityStep = 3
		}, new AirdropRewardTemplate
		{
			PointsBase = 1000,
			PointsStep = 1000
		});
	}

	public static IEnumerable<MissionTemplate> GenerateMilestoneMissions()
	{
		yield break;
	}


	private static MissionTemplate CreateMissionTemplate(string type, SurvivalWaveObjectiveTemplate objective, AirdropRewardTemplate reward)
	{
		return new MissionTemplate
		{
			ProductionId = $"{type}_Arena_SurvivalWave_{objective.Challenge}",
			Name = $"Reach {objective.Challenge} Survival Wave {{0}}",
			Description = $"Reach {objective.Challenge} Survival Wave {{0}}",
			Type = type,
			MissionTier = 0,
			Game = "Arena",
			Objectives = new[]
			{
				objective with
				{
					Description = $"Reach {objective.Challenge} Survival Wave {{0}}"
				}
			},
			Rewards = new[]
			{
				reward
			}
		};
	}
}
