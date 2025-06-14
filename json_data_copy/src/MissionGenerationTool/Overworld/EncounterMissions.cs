using Illuvium.DataModels;

namespace MissionGenerationTool;

public class EncounterMissions
{
	private static int[] Stages = new[] { 0, 1, 2, 3 };

	public static IEnumerable<MissionTemplate> GenerateDailyMissions()
	{
		foreach (var winRequired in new bool?[] { null, true })
		{
			var prefix = winRequired.GetValueOrDefault() ? "Win" : "Fight";
			foreach (var stage in Stages)
			{
				yield return new MissionTemplate
				{
					ProductionId = $"Daily_Overworld_Encounters_S{stage}_{prefix}",
					Name = $"{prefix} {{0}} encounters in Stage {stage}",
					Description = $"{prefix} {{0}} encounters in Stage {stage}",
					Type = "Daily",
					Weight = 2,
					MissionTier = stage,
					Game = "Overworld",
					Objectives = new[]
					{
						new FightEncounterObjectiveTemplate
						{
							Description = $"{prefix} {{0}} encounters in Stage {stage}",
							Stage = stage,
							WinRequired = winRequired,
							QuantityMinimum = 1,
							QuantityMaximum = 6,
							QuantityStep = 1
						}
					},
					Rewards = new[]
					{
						new AirdropRewardTemplate
						{
							PointsStep = stage switch
							{
								3 => prefix == "Win" ? 60 : 50,
								2 => prefix == "Win" ? 50 : 40,
								1 => prefix == "Win" ? 40 : 30,
								_ => prefix == "Win" ? 10 : 5
							},
							PointsBase = stage switch
							{
								3 => prefix == "Win" ? 60 : 50,
								2 => prefix == "Win" ? 50 : 40,
								1 => prefix == "Win" ? 40 : 30,
								_ => prefix == "Win" ? 10 : 5
							}
						}
					}
				};
			}
		}
	}

	public static IEnumerable<MissionTemplate> GenerateWeeklyMissions()
	{
		foreach (var winRequired in new bool?[] { null, true })
		{
			var prefix = winRequired.GetValueOrDefault() ? "Win" : "Fight";
			foreach (var stage in Stages)
			{
				yield return new MissionTemplate
				{
					ProductionId = $"Weekly_Overworld_Encounters_S{stage}_{prefix}",
					Name = $"{prefix} {{0}} encounters in Stage {stage}",
					Description = $"{prefix} {{0}} encounters in Stage {stage}",
					Type = "Weekly",
					Weight = 2,
					MissionTier = stage,
					Game = "Overworld",
					Objectives = new[]
					{
						new FightEncounterObjectiveTemplate
						{
							Description = $"{prefix} {{0}} encounters in Stage {stage}",
							Stage = stage,
							WinRequired = winRequired,
							QuantityMinimum = 10,
							QuantityMaximum = 30,
							QuantityStep = 5
						}
					},
					Rewards = new[]
					{
						new AirdropRewardTemplate
						{
							PointsStep = stage switch
							{
								3 => prefix == "Win" ? 420 : 350,
								2 => prefix == "Win" ? 350 : 280,
								1 => prefix == "Win" ? 280 : 210,
								_ => prefix == "Win" ? 70 : 35
							},
							PointsBase = stage switch
							{
								3 => prefix == "Win" ? 420 : 350,
								2 => prefix == "Win" ? 350 : 280,
								1 => prefix == "Win" ? 280 : 210,
								_ => prefix == "Win" ? 70 : 35
							}
						}
					}
				};
			}
		}
	}

	public static IEnumerable<MissionTemplate> GenerateMilestoneMissions()
	{
		yield break;
	}
}