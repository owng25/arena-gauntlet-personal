using Illuvium.DataModels;

namespace MissionGenerationTool;

public static class GatherMissions
{
	public static IEnumerable<MissionTemplate> GenerateDailyMissions(List<Item> items)
	{
		foreach (var item in items.Where(x => x.ItemType == "Essence"))
		{
			yield return new MissionTemplate
			{
				ProductionId = "Daily_Overworld_Gather_Essence_" + item.Name,
				Name = "Gather {0} " + item.GetDisplayName(),
				Description = "Gather {0} " + item.GetDisplayName() + " by harvesting plants",
				Type = "Daily",
				Game = "Overworld",
				MissionTier = item.Tier.GetValueOrDefault(),
				Objectives = new[]
				{
					new GatherResourceObjectiveTemplate
					{
						Description = "Gather {0} " + item.GetDisplayName() + " by harvesting plants",
						Name = item.Name,
						ItemType = item.ItemType,
						QuantityMinimum = item.Tier.GetValueOrDefault() switch
						{
							5 => 2,
							4 => 3,
							3 => 4,
							_ => 5
						},
						QuantityMaximum = item.Tier.GetValueOrDefault() switch
						{
							5 => 6,
							4 => 9,
							3 => 12,
							_ => 15
						},
						QuantityStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 2,
							4 => 3,
							3 => 4,
							_ => 5
						}
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						PointsStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 120,
							4 => 100,
							3 => 80,
							2 => 60,
							1 => 40,
							_ => 10
						},
						PointsBase = item.Tier.GetValueOrDefault() switch
						{
							5 => 120,
							4 => 100,
							3 => 80,
							2 => 60,
							1 => 40,
							_ => 10
						}
					}
				}
			};
		}

		foreach (var item in items.Where(x => x.ItemType == "Consumable" && !x.Name.StartsWith("NaniticDiscord")))
		{
			yield return new MissionTemplate
			{
				ProductionId = "Daily_Overworld_Gather_Consumable_" + item.Name,
				Name = "Gather {0} " + item.GetDisplayName(),
				Description = "Gather {0} " + item.GetDisplayName() + " by harvesting plants",
				Type = "Daily",
				Game = "Overworld",
				MissionTier = item.Tier.GetValueOrDefault(),
				Objectives = new[]
				{
					new GatherResourceObjectiveTemplate
					{
						Description = "Gather {0} " + item.GetDisplayName() + " by harvesting plants",
						Name = item.Name,
						ItemType = item.ItemType,
						QuantityMinimum = item.Tier.GetValueOrDefault() switch
						{
							3 => 2,
							_ => 3
						},
						QuantityMaximum = item.Tier.GetValueOrDefault() switch
						{
							3 => 10,
							_ => 15
						},
						QuantityStep = item.Tier.GetValueOrDefault() switch
						{
							3 => 2,
							_ => 3
						}
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						PointsStep = item.Tier.GetValueOrDefault() switch
						{
							3 => 40,
							2 => 30,
							1 => 20,
							_ => 2
						},
						PointsBase = item.Tier.GetValueOrDefault() switch
						{
							3 => 40,
							2 => 30,
							1 => 20,
							_ => 2
						}
					}
				}
			};
		}

		foreach (var item in items.Where(x => x.ItemType == "OverworldOre"))
		{
			yield return new MissionTemplate
			{
				ProductionId = "Daily_Overworld_Gather_Ore_" + item.Name,
				Name = "Extract {0} " + item.GetDisplayName(),
				Description = "Extract {0} " + item.GetDisplayName() + " by mining deposits",
				Type = "Daily",
				Game = "Overworld",
				MissionTier = item.Tier.GetValueOrDefault(),
				Objectives = new[]
				{
					new GatherResourceObjectiveTemplate
					{
						Description = "Extract {0} " + item.GetDisplayName() + " by mining deposits",
						Name = item.Name,
						ItemType = item.ItemType,
						QuantityMinimum = item.Tier.GetValueOrDefault() switch
						{
							5 => 2,
							4 => 4,
							3 => 4,
							_ => 5
						},
						QuantityMaximum = item.Tier.GetValueOrDefault() switch
						{
							5 => 10,
							4 => 20,
							3 => 20,
							_ => 25
						},
						QuantityStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 2,
							4 => 4,
							3 => 4,
							_ => 5
						}
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						PointsStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 60,
							4 => 50,
							3 => 40,
							2 => 30,
							1 => 20,
							_ => 5
						},
						PointsBase = item.Tier.GetValueOrDefault() switch
						{
							5 => 60,
							4 => 50,
							3 => 40,
							2 => 30,
							1 => 20,
							_ => 5
						}
					}
				}
			};
		}

		foreach (var item in items.Where(x => x.ItemType == "OverworldGemstone"))
		{
			yield return new MissionTemplate
			{
				ProductionId = "Daily_Overworld_Gather_Gem_" + item.Name,
				Name = "Extract {0} " + item.GetDisplayName(),
				Description = "Extract {0} " + item.GetDisplayName() + " by mining deposits",
				Type = "Daily",
				Game = "Overworld",
				MissionTier = item.Tier.GetValueOrDefault(),
				Objectives = new[]
				{
					new GatherResourceObjectiveTemplate
					{
						Description = "Extract {0} " + item.GetDisplayName() + " by mining deposits",
						Name = item.Name,
						ItemType = item.ItemType,
						QuantityMinimum = item.Tier.GetValueOrDefault() switch
						{
							5 => 1,
							4 => 2,
							3 => 4,
							_ => 5
						},
						QuantityMaximum = item.Tier.GetValueOrDefault() switch
						{
							5 => 3,
							4 => 6,
							3 => 12,
							_ => 15
						},
						QuantityStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 1,
							4 => 2,
							3 => 4,
							_ => 5
						}
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						PointsStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 150,
							4 => 120,
							3 => 90,
							2 => 60,
							1 => 30,
							_ => 10
						},
						PointsBase = item.Tier.GetValueOrDefault() switch
						{
							5 => 150,
							4 => 120,
							3 => 90,
							2 => 60,
							1 => 30,
							_ => 10
						}
					}
				}
			};
		}

		foreach (var item in items.Where(x => x.ItemType == "Shard"))
		{
			yield return new MissionTemplate
			{
				ProductionId = "Daily_Overworld_Gather_Shard_" + item.Name,
				Name = "Extract {0} " + item.GetDisplayName(),
				Description = "Extract {0} " + item.GetDisplayName() + " by mining deposits",
				Type = "Daily",
				Game = "Overworld",
				MissionTier = item.Tier.GetValueOrDefault(),
				Objectives = new[]
				{
					new GatherResourceObjectiveTemplate
					{
						Description = "Extract {0} " + item.GetDisplayName() + " by mining deposits",
						Name = item.Name,
						ItemType = item.ItemType,
						QuantityMinimum = item.Tier.GetValueOrDefault() switch
						{
							5 => 1,
							4 => 2,
							3 => 4,
							_ => 5
						},
						QuantityMaximum = item.Tier.GetValueOrDefault() switch
						{
							5 => 3,
							4 => 6,
							3 => 12,
							_ => 15
						},
						QuantityStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 1,
							4 => 2,
							3 => 4,
							_ => 5
						}
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						PointsStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 150,
							4 => 120,
							3 => 90,
							2 => 60,
							1 => 30,
							_ => 10
						},
						PointsBase = item.Tier.GetValueOrDefault() switch
						{
							5 => 150,
							4 => 120,
							3 => 90,
							2 => 60,
							1 => 30,
							_ => 10
						}
					}
				}
			};
		}
	}

	public static IEnumerable<MissionTemplate> GenerateWeeklyMissions(List<Item> items)
	{
		foreach (var item in items.Where(x => x.ItemType == "Essence"))
		{
			yield return new MissionTemplate
			{
				ProductionId = "Weekly_Overworld_Gather_Essence_" + item.Name,
				Name = "Gather {0} " + item.GetDisplayName(),
				Description = "Gather {0} " + item.GetDisplayName() + " by harvesting plants",
				Type = "Weekly",
				Game = "Overworld",
				MissionTier = item.Tier.GetValueOrDefault(),
				Objectives = new[]
				{
					new GatherResourceObjectiveTemplate
					{
						Description = "Gather {0} " + item.GetDisplayName() + " by harvesting plants",
						Name = item.Name,
						ItemType = item.ItemType,
						QuantityMinimum = item.Tier.GetValueOrDefault() switch
						{
							5 => 8,
							4 => 12,
							3 => 16,
							_ => 20
						},
						QuantityMaximum = item.Tier.GetValueOrDefault() switch
						{
							5 => 25,
							4 => 36,
							3 => 48,
							_ => 60
						},
						QuantityStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 4,
							4 => 6,
							3 => 8,
							_ => 10
						}
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						PointsStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 480,
							4 => 400,
							3 => 320,
							2 => 240,
							1 => 160,
							_ => 40
						},
						PointsBase = item.Tier.GetValueOrDefault() switch
						{
							5 => 480,
							4 => 400,
							3 => 320,
							2 => 240,
							1 => 160,
							_ => 40
						}
					}
				}
			};
		}

		foreach (var item in items.Where(x => x.ItemType == "Consumable" && !x.Name.StartsWith("NaniticDiscord")))
		{
			yield return new MissionTemplate
			{
				ProductionId = "Weekly_Overworld_Gather_Consumable_" + item.Name,
				Name = "Gather {0} " + item.GetDisplayName(),
				Description = "Gather {0} " + item.GetDisplayName() + " by harvesting plants",
				Type = "Weekly",
				Game = "Overworld",
				MissionTier = item.Tier.GetValueOrDefault(),
				Objectives = new[]
				{
					new GatherResourceObjectiveTemplate
					{
						Description = "Gather {0} " + item.GetDisplayName() + " by harvesting plants",
						Name = item.Name,
						ItemType = item.ItemType,
						QuantityMinimum = item.Tier.GetValueOrDefault() switch
						{
							3 => 12,
							_ => 18
						},
						QuantityMaximum = item.Tier.GetValueOrDefault() switch
						{
							3 => 36,
							_ => 45
						},
						QuantityStep = item.Tier.GetValueOrDefault() switch
						{
							3 => 6,
							_ => 9
						}
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						PointsStep = item.Tier.GetValueOrDefault() switch
						{
							3 => 240,
							2 => 180,
							1 => 120,
							_ => 12
						},
						PointsBase = item.Tier.GetValueOrDefault() switch
						{
							3 => 240,
							2 => 180,
							1 => 120,
							_ => 12
						}
					}
				}
			};
		}

		foreach (var item in items.Where(x => x.ItemType == "OverworldOre"))
		{
			yield return new MissionTemplate
			{
				ProductionId = "Weekly_Overworld_Gather_Ore_" + item.Name,
				Name = "Extract {0} " + item.GetDisplayName(),
				Description = "Extract {0} " + item.GetDisplayName() + " by mining deposits",
				Type = "Weekly",
				Game = "Overworld",
				MissionTier = item.Tier.GetValueOrDefault(),
				Objectives = new[]
				{
					new GatherResourceObjectiveTemplate
					{
						Description = "Extract {0} " + item.GetDisplayName() + " by mining deposits",
						Name = item.Name,
						ItemType = item.ItemType,
						QuantityMinimum = item.Tier.GetValueOrDefault() switch
						{
							5 => 12,
							4 => 24,
							3 => 24,
							_ => 30
						},
						QuantityMaximum = item.Tier.GetValueOrDefault() switch
						{
							5 => 36,
							4 => 72,
							3 => 72,
							_ => 90
						},
						QuantityStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 6,
							4 => 12,
							3 => 12,
							_ => 15
						}
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						PointsStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 360,
							4 => 300,
							3 => 240,
							2 => 180,
							1 => 120,
							_ => 30
						},
						PointsBase = item.Tier.GetValueOrDefault() switch
						{
							5 => 360,
							4 => 300,
							3 => 240,
							2 => 180,
							1 => 120,
							_ => 30
						}
					}
				}
			};
		}

		foreach (var item in items.Where(x => x.ItemType == "OverworldGemstone"))
		{
			yield return new MissionTemplate
			{
				ProductionId = "Weekly_Overworld_Gather_Gem_" + item.Name,
				Name = "Extract {0} " + item.GetDisplayName(),
				Description = "Extract {0} " + item.GetDisplayName() + " by mining deposits",
				Type = "Weekly",
				Game = "Overworld",
				MissionTier = item.Tier.GetValueOrDefault(),
				Objectives = new[]
				{
					new GatherResourceObjectiveTemplate
					{
						Description = "Extract {0} " + item.GetDisplayName() + " by mining deposits",
						Name = item.Name,
						ItemType = item.ItemType,
						QuantityMinimum = item.Tier.GetValueOrDefault() switch
						{
							5 => 4,
							4 => 6,
							3 => 12,
							_ => 20
						},
						QuantityMaximum = item.Tier.GetValueOrDefault() switch
						{
							5 => 12,
							4 => 18,
							3 => 36,
							_ => 60
						},
						QuantityStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 2,
							4 => 3,
							3 => 6,
							_ => 10
						}
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						PointsStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 600,
							4 => 480,
							3 => 360,
							2 => 240,
							1 => 120,
							_ => 40
						},
						PointsBase = item.Tier.GetValueOrDefault() switch
						{
							5 => 600,
							4 => 480,
							3 => 360,
							2 => 240,
							1 => 120,
							_ => 40
						}
					}
				}
			};
		}

		foreach (var item in items.Where(x => x.ItemType == "Shard"))
		{
			yield return new MissionTemplate
			{
				ProductionId = "Weekly_Overworld_Gather_Shard_" + item.Name,
				Name = "Extract {0} " + item.GetDisplayName(),
				Description = "Extract {0} " + item.GetDisplayName() + " by mining deposits",
				Type = "Weekly",
				Game = "Overworld",
				MissionTier = item.Tier.GetValueOrDefault(),
				Objectives = new[]
				{
					new GatherResourceObjectiveTemplate
					{
						Description = "Extract {0} " + item.GetDisplayName() + " by mining deposits",
						Name = item.Name,
						ItemType = item.ItemType,
						QuantityMinimum = item.Tier.GetValueOrDefault() switch
						{
							5 => 4,
							4 => 6,
							3 => 12,
							_ => 20
						},
						QuantityMaximum = item.Tier.GetValueOrDefault() switch
						{
							5 => 12,
							4 => 18,
							3 => 36,
							_ => 60
						},
						QuantityStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 2,
							4 => 3,
							3 => 6,
							_ => 10
						}
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						PointsStep = item.Tier.GetValueOrDefault() switch
						{
							5 => 600,
							4 => 480,
							3 => 360,
							2 => 240,
							1 => 120,
							_ => 40
						},
						PointsBase = item.Tier.GetValueOrDefault() switch
						{
							5 => 600,
							4 => 480,
							3 => 360,
							2 => 240,
							1 => 120,
							_ => 40
						}
					}
				}
			};
		}
	}

	public static IEnumerable<MissionTemplate> GenerateMilestoneMissions(List<Item> items)
	{
		yield break;
	}
}
