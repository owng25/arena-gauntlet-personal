using Illuvium.DataModels;

namespace MissionGenerationTool;

public static class ArenaGameMissions
{
	private readonly static int[] Tiers = Enumerable.Range(0, 5).ToArray();

	public const string Ascendant = "Ascendant";
	public const string Survival = "Survival";
	public const string Gauntlet = "Gauntlet";

	public const string AscendantLeviathan = "Leviathan";
	public const string AscendantCasual = "Casual";
	public const string AscendantRanked = "Ranked";
	public const string SurvivalTraining = "Training";
	public const string SurvivalCompetitive = "Competitive";

	private readonly static string[] Queues = new[] { AscendantCasual, AscendantRanked, AscendantLeviathan };
	private readonly static string[] GameModes = new[] { Ascendant, Survival }; // Leviathan, Gauntlet to be readded post june release

	public static IEnumerable<MissionTemplate> GenerateDailyMissions()
	{
		var type = "Daily";
		yield return CreateMissionTemplate(type, null, 0, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 1,
			QuantityMaximum = 5,
			QuantityStep = 1,
			WinRequired = false
		}, new AirdropRewardTemplate
		{
			PointsBase = 40,
			PointsStep = 40
		});

		yield return CreateMissionTemplate(type, Ascendant, 1, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 1,
			QuantityMaximum = 3,
			QuantityStep = 1,
			WinRequired = false,
			Queue = AscendantCasual
		}, new AirdropRewardTemplate
		{
			PointsBase = 100,
			PointsStep = 100
		});
		yield return CreateMissionTemplate(type, Survival, 2, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 1,
			QuantityMaximum = 3,
			QuantityStep = 1,
			WinRequired = false,
			Queue = SurvivalTraining
		}, new AirdropRewardTemplate
		{
			PointsBase = 150,
			PointsStep = 150
		});
		yield return CreateMissionTemplate(type, Survival, 3, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 1,
			QuantityMaximum = 3,
			QuantityStep = 1,
			WinRequired = false,
			Queue = SurvivalCompetitive
		}, new AirdropRewardTemplate
		{
			PointsBase = 200,
			PointsStep = 200
		});
		yield return CreateMissionTemplate(type, Ascendant, 4, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 1,
			QuantityMaximum = 3,
			QuantityStep = 1,
			WinRequired = false,
			Queue = AscendantRanked
		}, new AirdropRewardTemplate
		{
			PointsBase = 250,
			PointsStep = 250
		});
		// yield return CreateMissionTemplate(type, Gauntlet, 5, new ArenaGameObjectiveTemplate
		// {
		// 	QuantityMinimum = 1,
		// 	QuantityMaximum = 3,
		// 	QuantityStep = 1,
		// 	WinRequired = false,
		// }, new AirdropRewardTemplate
		// {
		// 	PointsBase = 300,
		// 	PointsStep = 300
		// });

		// win
		yield return CreateMissionTemplate(type, null, 0, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 1,
			QuantityMaximum = 3,
			QuantityStep = 1,
			WinRequired = true
		}, new AirdropRewardTemplate
		{
			PointsBase = 60,
			PointsStep = 60
		});
		yield return CreateMissionTemplate(type, Ascendant, 1, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 1,
			QuantityMaximum = 2,
			QuantityStep = 1,
			WinRequired = true,
			Queue = AscendantCasual
		}, new AirdropRewardTemplate
		{
			PointsBase = 150,
			PointsStep = 150
		});
		// yield return CreateMissionTemplate(type, Ascendant, 4, new ArenaGameObjectiveTemplate
		// {
		// 	QuantityMinimum = 1,
		// 	QuantityMaximum = 2,
		// 	QuantityStep = 1,
		// 	WinRequired = true,
		//	Queue = AscendantLeviathan
		// }, new AirdropRewardTemplate
		// {
		// 	PointsBase = 300,
		// 	PointsStep = 300
		// });
		yield return CreateMissionTemplate(type, Ascendant, 5, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 1,
			QuantityMaximum = 2,
			QuantityStep = 1,
			WinRequired = true,
			Queue = AscendantRanked
		}, new AirdropRewardTemplate
		{
			PointsBase = 450,
			PointsStep = 450
		});
	}

	public static IEnumerable<MissionTemplate> GenerateWeeklyMissions()
	{
		var type = "Weekly";
		yield return CreateMissionTemplate(type, null, 0, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 10,
			QuantityMaximum = 30,
			QuantityStep = 5,
			WinRequired = false
		}, new AirdropRewardTemplate
		{
			PointsBase = 250,
			PointsStep = 250
		});

		yield return CreateMissionTemplate(type, Ascendant, 1, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 9,
			QuantityMaximum = 24,
			QuantityStep = 3,
			WinRequired = false,
			Queue = AscendantCasual
		}, new AirdropRewardTemplate
		{
			PointsBase = 500,
			PointsStep = 500
		});
		yield return CreateMissionTemplate(type, Survival, 2, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 9,
			QuantityMaximum = 24,
			QuantityStep = 3,
			WinRequired = false,
			Queue = SurvivalTraining
		}, new AirdropRewardTemplate
		{
			PointsBase = 750,
			PointsStep = 750,
		});
		yield return CreateMissionTemplate(type, Survival, 3, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 9,
			QuantityMaximum = 24,
			QuantityStep = 3,
			WinRequired = false,
			Queue = SurvivalCompetitive
		}, new AirdropRewardTemplate
		{
			PointsBase = 1000,
			PointsStep = 1000
		});
		yield return CreateMissionTemplate(type, Ascendant, 4, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 9,
			QuantityMaximum = 24,
			QuantityStep = 3,
			WinRequired = false,
			Queue = AscendantRanked
		}, new AirdropRewardTemplate
		{
			PointsBase = 1200,
			PointsStep = 1200
		});
		// yield return CreateMissionTemplate(type, Gauntlet, 5, new ArenaGameObjectiveTemplate
		// {
		// 	QuantityMinimum = 9,
		// 	QuantityMaximum = 24,
		// 	QuantityStep = 3,
		// 	WinRequired = false,
		// }, new AirdropRewardTemplate
		// {
		// 	PointsBase = 1500,
		// 	PointsStep = 1500
		// });

		// win
		yield return CreateMissionTemplate(type, null, 0, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 10,
			QuantityMaximum = 30,
			QuantityStep = 5,
			WinRequired = true
		}, new AirdropRewardTemplate
		{
			PointsBase = 300,
			PointsStep = 300
		});
		yield return CreateMissionTemplate(type, Ascendant, 1, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 6,
			QuantityMaximum = 18,
			QuantityStep = 3,
			WinRequired = true,
			Queue = AscendantCasual
		}, new AirdropRewardTemplate
		{
			PointsBase = 650,
			PointsStep = 650
		});
		// yield return CreateMissionTemplate(type, Ascendant, 4, new ArenaGameObjectiveTemplate
		// {
		// 	QuantityMinimum = 6,
		// 	QuantityMaximum = 18,
		// 	QuantityStep = 3,
		// 	WinRequired = true,
		//	Queue = AscendantLeviathan
		// }, new AirdropRewardTemplate
		// {
		// 	PointsBase = 1500,
		// 	PointsStep = 1500
		// });
		yield return CreateMissionTemplate(type, Ascendant, 5, new ArenaGameObjectiveTemplate
		{
			QuantityMinimum = 6,
			QuantityMaximum = 18,
			QuantityStep = 3,
			WinRequired = true,
			Queue = AscendantRanked
		}, new AirdropRewardTemplate
		{
			PointsBase = 2000,
			PointsStep = 2000
		});
	}

	public static IEnumerable<MissionTemplate> GenerateMilestoneMissions()
	{
		yield return CreateMilestoneTemplate(Ascendant, null, 0, false, 10, 1_000);
		// yield return CreateMilestoneTemplate(Gauntlet, null, 0, false, 10, 1_000);
		yield return CreateMilestoneTemplate(Survival, null, 0, false, 10, 1_000);
		yield return CreateMilestoneTemplate(Ascendant, null, 0, true, 10, 1_000);

		yield return CreateMilestoneTemplate(Ascendant, null, 1, false, 100, 10_000);
		// yield return CreateMilestoneTemplate(Gauntlet, null, 1, false, 100, 10_000);
		yield return CreateMilestoneTemplate(Survival, null, 1, false, 100, 10_000);
		yield return CreateMilestoneTemplate(Ascendant, null, 1, true, 100, 10_000);

		yield return CreateMilestoneTemplate(Ascendant, null, 2, false, 1_000, 50_000);
		// yield return CreateMilestoneTemplate(Gauntlet, null, 2, false, 1_000, 50_000);
		yield return CreateMilestoneTemplate(Survival, null, 2, false, 1_000, 50_000);
		yield return CreateMilestoneTemplate(Ascendant, null, 2, true, 1_000, 50_000);

		yield return CreateMilestoneTemplate(Ascendant, null, 4, false, 10_000, 100_000);
		// yield return CreateMilestoneTemplate(Gauntlet, null, 4, false, 10_000, 100_000);
		yield return CreateMilestoneTemplate(Survival, null, 4, false, 10_000, 100_000);

		yield return CreateMilestoneTemplate(Ascendant, null, 5, true, 10_000, 200_000);
	}

	private static MissionTemplate CreateMilestoneTemplate(string? gameMode, string? queue, int tier, bool winRequred, int quantity, int reward)
	{
		return CreateMissionTemplate("Milestone", gameMode, tier, new ArenaGameObjectiveTemplate
		{
			Quantity = quantity,
			WinRequired = winRequred,
			Queue = queue
		}, new AirdropRewardTemplate
		{
			Points = reward
		});
	}

	private static MissionTemplate CreateMissionTemplate(string type, string? gameMode, int tier, ArenaGameObjectiveTemplate objective, AirdropRewardTemplate reward)
	{
		var prefix = objective.WinRequired.GetValueOrDefault() ? "Win" : "Play";
		var detail = $"{prefix} {{0}} Arena {gameMode} {objective.Queue} Games".Replace("  ", " ").Replace("  ", " ");
		return new MissionTemplate
		{
			ProductionId = $"{type}_Arena_{gameMode}_{objective.Queue}_Tier{tier}_{prefix}".Replace("__", "_").Replace("__", "_"),
			Name = detail,
			Description = detail,
			Type = type,
			MissionTier = tier,
			Game = "Arena",
			Objectives = new[]
			{
				objective with
				{
					Description = detail,
					GameMode= gameMode,
				}
			},
			Rewards = new[]
			{
				reward
			}
		};
	}
}
