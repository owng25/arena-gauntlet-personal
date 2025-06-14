using Illuvium.DataModels;

namespace MissionGenerationTool;

public static class CraftingMissions
{
	public static IEnumerable<MissionTemplate> GenerateDailyMissions(List<Item> items)
	{
		foreach (var item in items.Where(x => x is { ItemType: "CuredShard" }))
		{
			yield return new MissionTemplate
			{
				ProductionId = $"Daily_Overworld_Forge_{item.ItemType}_{item.Name}",
				Name = "Forge " + item.GetDisplayName() + "s",
				Description = "Forge {0} " + item.GetDisplayName() + "s",
				Type = "Daily",
				Game = "Overworld",
				MissionTier = item.Tier.GetValueOrDefault(1),
				Objectives = new[]
				{
					new ForgeItemObjectiveTemplate
					{
						Name = item.Name,
						Description = "Forge {0} " + item.GetDisplayName() + "s",
						ItemType = item.ItemType,
						QuantityMinimum = item.Tier.GetValueOrDefault(1) switch
						{
							5 => 1,
							4 => 2,
							3 => 4,
							_ => 5
						},
						QuantityMaximum = item.Tier.GetValueOrDefault(1) switch
						{
							5 => 3,
							4 => 6,
							3 => 12,
							_ => 15
						},
						QuantityStep = item.Tier.GetValueOrDefault(1) switch
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
						PointsStep = item.Tier.GetValueOrDefault(1) switch
						{
							5 => 150,
							4 => 120,
							3 => 90,
							2 => 60,
							1 => 30,
							_ => 10
						},
						PointsBase = item.Tier.GetValueOrDefault(1) switch
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
		foreach (var item in items.Where(x => x is { ItemType: "CuredShard" }))
		{
			yield return new MissionTemplate
			{
				ProductionId = $"Weekly_Overworld_Forge_{item.ItemType}_{item.Name}",
				Name = "Forge " + item.GetDisplayName() + "s",
				Description = "Forge {0} " + item.GetDisplayName() + "s",
				Type = "Weekly",
				Game = "Overworld",
				MissionTier = item.Tier.GetValueOrDefault(1),
				Objectives = new[]
				{
					new ForgeItemObjectiveTemplate
					{
						Name = item.Name,
						Description = "Forge {0} " + item.GetDisplayName() + "s",
						ItemType = item.ItemType,
						QuantityMinimum = item.Tier.GetValueOrDefault(1) switch
						{
							5 => 4,
							4 => 6,
							3 => 12,
							_ => 20
						},
						QuantityMaximum = item.Tier.GetValueOrDefault(1) switch
						{
							5 => 12,
							4 => 18,
							3 => 36,
							_ => 60
						},
						QuantityStep = item.Tier.GetValueOrDefault(1) switch
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
						PointsStep = item.Tier.GetValueOrDefault(1) switch
						{
							5 => 600,
							4 => 480,
							3 => 360,
							2 => 240,
							1 => 120,
							_ => 40
						},
						PointsBase = item.Tier.GetValueOrDefault(1) switch
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

		var itemsToCraft = items.Where(x => x.ItemType != "CuredShard").ToList();
		foreach (var item in itemsToCraft)
		{
			yield return new MissionTemplate
			{
				ProductionId = $"Weekly_Overworld_Forge_{item.ItemType}_{item.Name}",
				Name = "Forge a " + item.GetDisplayName(),
				Description = "Forge a " + item.GetDisplayName(),
				Type = "Weekly",
				Weight = 1,
				Game = "Overworld",
				MissionTier = item.Tier.GetValueOrDefault(),
				Objectives = new[]
				{
					new ForgeItemObjectiveTemplate
					{
						Description = "Forge a " + item.GetDisplayName(),
						Name = item.Name,
						ItemType = item.ItemType,
						Quantity = 1
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						Points = item.Tier.GetValueOrDefault() switch
						{
							5 => item.Stage switch
							{
								3 => 45000,
								2 => 15000,
								_ => 5000
							},
							4 => item.Stage switch
							{
								3 => 36000,
								2 => 12000,
								_ => 4000,
							},
							3 => item.Stage switch
							{
								3 => 27000,
								2 => 9000,
								_ => 3000
							},
							2 => item.Stage switch
							{
								3 => 18000,
								2 => 6000,
								_ => 2000
							},
							1 => item.Stage switch
							{
								3 => 9000,
								2 => 3000,
								_ => 1000
							},
							_ => item.Stage switch
							{
								3 => 1350,
								2 => 450,
								_ => 150
							}
						}
					}
				}
			};
		}
	}

	public static IEnumerable<MissionTemplate> GenerateMilestoneMissions(List<Item> items)
	{
		var weapons = items.Where(x => x.ItemType == "BattleWeapon").ToList();
		var suits = items.Where(x => x.ItemType == "BattleSuit").ToList();

		var weaponPoints = new Dictionary<int, Dictionary<int, int>>
		{
			[0] = new()
			{
				[1] = 7_500,
				[2] = 22_500,
				[3] = 67_500,
			},
			[1] = new()
			{
				[1] = 6_000,
				[2] = 18_000,
				[3] = 54_000,
			},
			[2] = new()
			{
				[1] = 12_000,
				[2] = 36_000,
				[3] = 108_000,
			},
			[3] = new()
			{
				[1] = 18_000,
				[2] = 54_000,
				[3] = 162_000,
			},
			[4] = new()
			{
				[1] = 24_000,
				[2] = 72_000,
				[3] = 216_000,
			},
			[5] = new()
			{
				[1] = 30_000,
				[2] = 90_000,
				[3] = 270_000,
			}
		};

		// all weapons
		yield return new MissionTemplate
		{
			ProductionId = $"Milestone_Overworld_Forge_All_Weapons",
			Name = "Forge All Weapons",
			Description = "Forge All Weapons",
			Type = "Milestone",
			Game = "Overworld",
			MissionTier = 0,
			Objectives = weapons.Select(x => new ForgeItemObjectiveTemplate
			{
				Description = $"Forge a {x.GetDisplayName()}",
				Name = x.Name,
				Quantity = 1
			}).ToList(),
			Rewards = new[]
			{
				new AirdropRewardTemplate
				{
					Points = 300_000
				}
			}
		};

		foreach (var (tier, points) in weaponPoints)
		{
			foreach (var (stage, point) in points)
			{
				yield return new MissionTemplate
				{
					ProductionId = $"Milestone_Overworld_Forge_Weapons_Tier{tier}_Stage{stage}",
					Name = $"Forge Tier {tier} Stage {stage} Weapons",
					Description = $"Forge Tier {tier} Stage {stage} Weapons",
					Type = "Milestone",
					Game = "Overworld",
					MissionTier = 0,
					Objectives = weapons.Where(x => x.Tier == tier && x.Stage == stage).Select(x => new ForgeItemObjectiveTemplate
					{
						Description = $"Forge a {x.GetDisplayName()}",
						Name = x.Name,
						Quantity = 1
					}).ToList(),
					Rewards = new[]
					{
						new AirdropRewardTemplate
						{
							Points = point
						}
					}
				};
			}
		}

		// all suits
		var suitPoints = new Dictionary<int, Dictionary<int, int>>
		{
			[0] = new()
			{
				[1] = 7_500,
				[2] = 22_500,
				[3] = 67_500,
			},
			[1] = new()
			{
				[1] = 6_000,
				[2] = 18_000,
				[3] = 54_000,
			},
			[2] = new()
			{
				[1] = 12_000,
				[2] = 36_000,
				[3] = 108_000,
			},
			[3] = new()
			{
				[1] = 18_000,
				[2] = 54_000,
				[3] = 162_000,
			},
			[4] = new()
			{
				[1] = 24_000,
				[2] = 72_000,
				[3] = 216_000,
			},
			[5] = new()
			{
				[1] = 30_000,
				[2] = 90_000,
				[3] = 270_000,
			}
		};

		yield return new MissionTemplate
		{
			ProductionId = $"Milestone_Overworld_Forge_All_Suits",
			Name = "Forge All Suits",
			Description = "Forge All Suits",
			Type = "Milestone",
			Game = "Overworld",
			MissionTier = 0,
			Objectives = suits.Select(x => new ForgeItemObjectiveTemplate
			{
				Description = $"Forge a {x.GetDisplayName()}",
				Name = x.Name,
				Quantity = 1
			}).ToList(),
			Rewards = new[]
			{
				new AirdropRewardTemplate
				{
					Points = 300_000
				}
			}
		};

		foreach (var (tier, points) in suitPoints)
		{
			foreach (var (stage, point) in points)
			{
				yield return new MissionTemplate
				{
					ProductionId = $"Milestone_Overworld_Forge_Suits_Tier{tier}_Stage{stage}",
					Name = $"Forge Tier {tier} Stage {stage} Suits",
					Description = $"Forge Tier {tier} Stage {stage} Suits",
					Type = "Milestone",
					Game = "Overworld",
					MissionTier = 0,
					Objectives = suits.Where(x => x.Tier == tier && x.Stage == stage).Select(x => new ForgeItemObjectiveTemplate
					{
						Description = $"Forge a {x.GetDisplayName()}",
						Name = x.Name,
						Quantity = 1
					}).ToList(),
					Rewards = new[]
					{
						new AirdropRewardTemplate
						{
							Points = point
						}
					}
				};
			}
		}
	}
}
