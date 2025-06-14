using Illuvium.DataModels;

namespace MissionGenerationTool.Zero;

public class ZeroMissions
{
	public static IEnumerable<MissionTemplate> Generate()
	{
		var missions = new List<MissionTemplate>();

		missions.AddRange(GenerateDailyMissions());
		missions.AddRange(GenerateWeeklyMissions());
		missions.AddRange(GenerateMilestoneMissions());

		return missions;
	}

	private static IEnumerable<MissionTemplate> GenerateDailyMissions()
	{
		yield return new MissionTemplate
		{
			ProductionId = "Daily_Zero_Complete_Daily_Goals",
			Name = "Complete {0} Daily Goals",
			Description = "Complete {0} Daily Goals",
			Quantity = 10,
			Type = "Daily",
			Game = "Zero",
			Objectives = new []
			{
				new FinishPlotGoalsObjectiveTemplate
				{
					Quantity = 3,
					GoalType = "Daily",
					Description = "Complete {0} Daily Goals"
				}
			},
			Rewards = new []
			{
				new AirdropRewardTemplate
				{
					Points = 10
				}
			}
		};

		foreach (var fuel in new[] { "Crypton", "Hyperion", "Solon" })
		{
			yield return new MissionTemplate
			{
				ProductionId = $"Daily_Zero_Convert_{fuel}",
				Name = $"Convert {fuel}",
				Description = $"Convert {{0}} {fuel} into a different resource",
				Quantity = 100,
				Type = "Daily",
				Game = "Zero",
				Objectives = new []
				{
					new ConvertFuelObjectiveTemplate
					{
						Quantity = 99,
						FuelType = fuel,
						Description = $"Convert {{0}} {fuel} into a different resource"
					}
				},
				Rewards = new []
				{
					new AirdropRewardTemplate
					{
						Points = 10
					}
				}
			};

			yield return new MissionTemplate
			{
				ProductionId = $"Daily_Zero_Extract_{fuel}",
				Name = $"Extract {fuel}",
				Description = $"Extract {{0}} {fuel} from a {fuel} site",
				Quantity = 100,
				Type = "Daily",
				Game = "Zero",
				Objectives = new []
				{
					new ExtractResourceObjectiveTemplate
					{
						Quantity = 99,
						ResourceType = fuel,
						Description = $"Extract {{0}} {fuel} from a {fuel} site"
					}
				},
				Rewards = new []
				{
					new AirdropRewardTemplate
					{
						Points = 10
					}
				}
			};
		}

		foreach (var resource in new[] { "Carbon", "Hydrogen", "Silicon" })
		{
			yield return new MissionTemplate
			{
				ProductionId = $"Daily_Zero_Convert_{resource}",
				Name = $"Convert {resource}",
				Description = $"Convert {{0}} {resource} into a different resource",
				Quantity = 100,
				Type = "Daily",
				Game = "Zero",
				Objectives = new []
				{
					new ConvertResourceObjectiveTemplate
					{
						Quantity = 9999,
						ResourceType = resource,
						Description = $"Convert {{0}} {resource} into a different resource"
					}
				},
				Rewards = new []
				{
					new AirdropRewardTemplate
					{
						Points = 10
					}
				}
			};

			yield return new MissionTemplate
			{
				ProductionId = $"Daily_Zero_Extract_{resource}",
				Name = $"Extract {resource}",
				Description = $"Extract {{0}} {resource} from a {resource} site",
				Quantity = 100,
				Type = "Daily",
				Game = "Zero",
				Objectives = new []
				{
					new ExtractResourceObjectiveTemplate
					{
						Quantity = 9999,
						ResourceType = resource,
						Description = $"Extract {{0}} {resource} from a {resource} site"
					}
				},
				Rewards = new []
				{
					new AirdropRewardTemplate
					{
						Points = 10
					}
				}
			};
		}

		yield return new MissionTemplate
		{
			ProductionId = "Daily_Zero_Research",
			Name = "Research Biodata",
			Description = "Complete a research operation",
			Quantity = 100,
			Type = "Daily",
			Game = "Zero",
			Objectives = new []
			{
				new ResearchCountObjectiveTemplate
				{
					Quantity = 1,
					Description = "Complete a research operation"
				}
			},
			Rewards = new []
			{
				new AirdropRewardTemplate
				{
					Points = 10
				}
			}
		};

		yield return new MissionTemplate
		{
			ProductionId = "Daily_Zero_Scan",
			Name = "Scan for Illuvials",
			Description = "Complete a Scan operation",
			Quantity = 100,
			Type = "Daily",
			Game = "Zero",
			Objectives = new []
			{
				new ScanCountObjectiveTemplate
				{
					Quantity = 1,
					Description = "Complete a Scan operation"
				}
			},
			Rewards = new []
			{
				new AirdropRewardTemplate
				{
					Points = 10
				}
			}
		};
	}

	private static IEnumerable<MissionTemplate> GenerateWeeklyMissions()
	{
		yield return new MissionTemplate
		{
			ProductionId = "Weekly_Zero_Complete_Daily_Goals",
			Name = "Complete {0} Daily Goals",
			Description = "Complete {0} Daily Goals",
			Quantity = 100,
			Type = "Weekly",
			Game = "Zero",
			Objectives = new []
			{
				new FinishPlotGoalsObjectiveTemplate
				{
					Quantity = 9,
					GoalType = "Daily",
					Description = "Complete {0} Daily Goals"
				}
			},
			Rewards = new []
			{
				new AirdropRewardTemplate
				{
					Points = 10
				}
			}
		};

		foreach (var fuel in new[] { "Crypton", "Hyperion", "Solon" })
		{
			yield return new MissionTemplate
			{
				ProductionId = $"Weekly_Zero_Convert_{fuel}",
				Name = $"Convert {fuel}",
				Description = $"Convert {{0}} {fuel} into a different resource",
				Quantity = 100,
				Type = "Weekly",
				Game = "Zero",
				Objectives = new []
				{
					new ConvertFuelObjectiveTemplate
					{
						Quantity = 99,
						FuelType = fuel,
						Description = $"Convert {{0}} {fuel} into a different resource"
					}
				},
				Rewards = new []
				{
					new AirdropRewardTemplate
					{
						Points = 10
					}
				}
			};

			yield return new MissionTemplate
			{
				ProductionId = $"Weekly_Zero_Extract_{fuel}",
				Name = $"Extract {fuel}",
				Description = $"Extract {{0}} {fuel} from a {fuel} site",
				Quantity = 100,
				Type = "Weekly",
				Game = "Zero",
				Objectives = new []
				{
					new ExtractResourceObjectiveTemplate
					{
						Quantity = 99,
						ResourceType = fuel,
						Description = $"Extract {{0}} {fuel} from a {fuel} site"
					}
				},
				Rewards = new []
				{
					new AirdropRewardTemplate
					{
						Points = 10
					}
				}
			};
		}

		foreach (var resource in new[] { "Carbon", "Hydrogen", "Silicon" })
		{
			yield return new MissionTemplate
			{
				ProductionId = $"Weekly_Zero_Convert_{resource}",
				Name = $"Convert {resource}",
				Description = $"Convert {{0}} {resource} into a different resource",
				Quantity = 100,
				Type = "Weekly",
				Game = "Zero",
				Objectives = new []
				{
					new ConvertResourceObjectiveTemplate
					{
						Quantity = 9999,
						ResourceType = resource,
						Description = $"Convert {{0}} {resource} into a different resource"
					}
				},
				Rewards = new []
				{
					new AirdropRewardTemplate
					{
						Points = 10
					}
				}
			};

			yield return new MissionTemplate
			{
				ProductionId = $"Weekly_Zero_Extract_{resource}",
				Name = $"Extract {resource}",
				Description = $"Extract {{0}} {resource} from a {resource} site",
				Quantity = 100,
				Type = "Weekly",
				Game = "Zero",
				Objectives = new []
				{
					new ExtractResourceObjectiveTemplate
					{
						Quantity = 9999,
						ResourceType = resource,
						Description = $"Extract {{0}} {resource} from a {resource} site"
					}
				},
				Rewards = new []
				{
					new AirdropRewardTemplate
					{
						Points = 10
					}
				}
			};
		}

		yield return new MissionTemplate
		{
			ProductionId = "Weekly_Zero_Research",
			Name = "Research Biodata",
			Description = "Complete a research operation",
			Quantity = 100,
			Type = "Weekly",
			Game = "Zero",
			Objectives = new []
			{
				new ResearchCountObjectiveTemplate
				{
					Quantity = 1,
					Description = "Complete a research operation"
				}
			},
			Rewards = new []
			{
				new AirdropRewardTemplate
				{
					Points = 10
				}
			}
		};

		yield return new MissionTemplate
		{
			ProductionId = "Weekly_Zero_Scan",
			Name = "Scan for Illuvials",
			Description = "Complete a Scan operation",
			Quantity = 100,
			Type = "Weekly",
			Game = "Zero",
			Objectives = new []
			{
				new ScanCountObjectiveTemplate
				{
					Quantity = 1,
					Description = "Complete a Scan operation"
				}
			},
			Rewards = new []
			{
				new AirdropRewardTemplate
				{
					Points = 10
				}
			}
		};

		yield return new MissionTemplate
		{
			ProductionId = "Weekly_Zero_Upgrade_Buildings",
			Name = "Upgrade {0} Buildings (of any kind) on a Land Plot",
			Description = "Upgrade {0} Buildings of any kind",
			Quantity = 100,
			Type = "Weekly",
			Game = "Zero",
			Objectives = new []
			{
				new UpgradeBuildingObjectiveTemplate
				{
					Quantity = 1,
					BuildingId = "ANY",
					Description = "Upgrade {0} Buildings of any kind"
				}
			},
			Rewards = new []
			{
				new AirdropRewardTemplate
				{
					Points = 10
				}
			}
		};

		yield return new MissionTemplate
		{
			ProductionId = "Weekly_Zero_Build_Buildings",
			Name = "Build {0} Buildings (of any kind) on a Land Plot",
			Description = "Build {0} Buildings of any kind",
			Quantity = 100,
			Type = "Weekly",
			Game = "Zero",
			Objectives = new []
			{
				new ConstructBuildingObjectiveTemplate
				{
					Quantity = 1,
					BuildingId = "ANY",
					Description = "Build {0} Buildings of any kind"
				}
			},
			Rewards = new []
			{
				new AirdropRewardTemplate
				{
					Points = 10
				}
			}
		};
	}

	private static IEnumerable<MissionTemplate> GenerateMilestoneMissions()
	{
		yield break;
	}
}