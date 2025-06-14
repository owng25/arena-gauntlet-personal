using Illuvium.DataModels;

namespace MissionGenerationTool;

public static class MorphopodMissions
{
	public static IEnumerable<MissionTemplate> GenerateDailyMissions(List<Item> items)
	{
		yield return new MissionTemplate
		{
			ProductionId = "Daily_Overworld_Morphopod_Catch",
			Name = "Catch {0} Morphopods",
			Description = "Catch {0} Morphopods",
			Type = "Daily",
			Game = "Overworld",
			MissionTier = 0,
			Objectives = new[]
			{
				new CatchMorphopodObjectiveTemplate()
				{
					Description = "Catch {0} Morphopods",
					QuantityMinimum = 5,
					QuantityMaximum = 30,
					QuantityStep = 5
				}
			},
			Rewards = new[]
			{
				new AirdropRewardTemplate
				{
					PointsStep = 10,
					PointsBase = 10
				}
			}
		};

		foreach (var line in items.Select(x => x.Line).Distinct().OrderBy(x => x))
		{
			yield return new MissionTemplate
			{
				ProductionId = "Daily_Overworld_Morphopod_Catch_" + line,
				Name = "Catch {0} " + line + " Morphopods",
				Description = "Catch {0} " + line + " Morphopods",
				Type = "Daily",
				Game = "Overworld",
				MissionTier = 0,
				Objectives = new[]
				{
					new CatchMorphopodObjectiveTemplate()
					{
						Description = "Catch {0} " + line + " Morphopods",
						Line = line,
						QuantityMinimum = 5,
						QuantityMaximum = 20,
						QuantityStep = 5
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						PointsStep = 25,
						PointsBase = 25
					}
				}
			};
		}
	}

	public static IEnumerable<MissionTemplate> GenerateWeeklyMissions(List<Item> items)
	{
		yield return new MissionTemplate
		{
			ProductionId = "Weekly_Overworld_Morphopod_Catch",
			Name = "Catch {0} Morphopods",
			Description = "Catch {0} Morphopods",
			Type = "Weekly",
			Game = "Overworld",
			MissionTier = 0,
			Objectives = new[]
			{
				new CatchMorphopodObjectiveTemplate()
				{
					Description = "Catch {0} Morphopods",
					QuantityMinimum = 30,
					QuantityMaximum = 90,
					QuantityStep = 15
				}
			},
			Rewards = new[]
			{
				new AirdropRewardTemplate
				{
					PointsStep = 70,
					PointsBase = 70
				}
			}
		};

		foreach (var line in items.Select(x => x.Line).Distinct().OrderBy(x => x))
		{
			yield return new MissionTemplate
			{
				ProductionId = "Weekly_Overworld_Morphopod_Catch_" + line,
				Name = "Catch {0} " + line + " Morphopods",
				Description = "Catch {0} " + line + " Morphopods",
				Type = "Weekly",
				Game = "Overworld",
				MissionTier = 0,
				Objectives = new[]
				{
					new CatchMorphopodObjectiveTemplate()
					{
						Description = "Catch {0} " + line + " Morphopods",
						Line = line,
						QuantityMinimum = 20,
						QuantityMaximum = 60,
						QuantityStep = 10
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						PointsStep = 125,
						PointsBase = 125
					}
				}
			};
		}
	}
}