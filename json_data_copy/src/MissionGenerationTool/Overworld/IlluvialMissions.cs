using Illuvium.DataModels;

namespace MissionGenerationTool;

public static class IlluvialMissions
{
	private static Dictionary<string, string> LineDisplayNames = new()
	{
		["Axolotl"] = "Axolotl",
		["Pterodactyl"] = "Pterodactyl",
		["SeaScorpion"] = "Sea Scorpion",
		["Thylacine"] = "Thylacine",
		["Turtle"] = "Turtle",
		["AntEater"] = "Ant Eater",
		["Beetle"] = "Beetle",
		["Dodo"] = "Dodo",
		["Elk"] = "Elk",
		["Pangolin"] = "Pangolin",
		["PistolShrimp"] = "Pistol Shrimp",
		["Shoebill"] = "Shoebill",
		["StarNosedMole"] = "Star Nosed Mole",
		["Taipan"] = "Taipan",
		["Snail"] = "Snail",
		["Mammoth"] = "Mammoth",
		["Squid"] = "Squid",
		["PolarBear"] = "Polar Bear",
		["Tiktaalik"] = "Tiktaalik",
		["Monkier"] = "Monkier",
		["Aapon"] = "Aapon",
		["Grilla"] = "Grilla",
		["Tenrec"] = "Tenrec",
		["KomodoDragon"] = "Komodo Drago",
		["PoodleMoth"] = "Poodle Moth",
		["Sloth"] = "Sloth",
		["TreeKangaroo"] = "Tree Kangaroo",
		["Penguin"] = "Penguin",
		["FennecFox"] = "Fennec Fox",
		["WaterBuffalo"] = "Water Buffalo",
		["TerrorBird"] = "Terror Bird",
		["DropBear"] = "Drop Bear",
		["RedPanda"] = "Red Panda",
		["Lynx"] = "Lynx",
		["DokaWater"] = "Water Doka",
		["DokaEarth"] = "Earth Doka",
		["DokaFire"] = "Fire Doka",
		["DokaNature"] = "Nature Doka",
		["DokaAir"] = "Air Doka",
		["GrokkoWater"] = "Water Grokko",
		["GrokkoEarth"] = "Earth Grokko",
		["GrokkoFire"] = "Fire Grokko",
		["GrokkoNature"] = "Nature Grokko",
		["GrokkoAir"] = "Air Grokko",
		["VolanteWater"] = "Water Volante",
		["VolanteEarth"] = "Earth Volante",
		["VolanteFire"] = "Fire Volante",
		["VolanteNature"] = "Nature Volante",
		["VolanteAir"] = "Air Volante",
		["FliishWater"] = "Water Fliish",
		["FliishEarth"] = "Earth Fliish",
		["FliishFire"] = "Fire Fliish",
		["FliishNature"] = "Nature Fliish",
		["FliishAir"] = "Air Fliish",
		["AtippoWater"] = "Water Atippo",
		["AtippoEarth"] = "Earth Atippo",
		["AtippoFire"] = "Fire Atippo",
		["AtippoNature"] = "Nature Atippo",
		["AtippoAir"] = "Air Atippo"
	};

	private record RegularBountyConfiguration(int Tier, int Stage, int Points);

	private record MegaBountyConfiguration(string DisplayName, string ItemName, int Tier);

	private record FinishMultiplierConfiguration(string Finish, string FinishName, int Multiplier);


	public static IEnumerable<MissionTemplate> GenerateDailyMissions(List<Item> illuvials)
	{
		// Generic daily XP
		yield return new MissionTemplate
		{
			ProductionId = $"Daily_Overworld_Gain_XP",
			Name = "Gain {0} XP with your Illuvials",
			Description = "Gain {0} XP with your Illuvials",
			Type = "Daily",
			Game = "Overworld",
			MissionTier = 0,
			Objectives = new[]
			{
				new GainIlluvialExperienceObjectiveTemplate
				{
					Description = "Gain {0} XP with your Illuvials",
					ExperienceNeededMinimum = 500,
					ExperienceNeededMaximum = 3000,
					ExperienceNeededStep = 500
				}
			},
			Rewards = new[]
			{
				new AirdropRewardTemplate
				{
					PointsStep = 60,
					PointsBase = 60
				}
			}
		};

		// foreach (var (tier, line) in illuvials.Select(x => (x.Tier, x.Line)).Distinct())
		// {
		// 	var lineDisplayName = LineDisplayNames[line];
		// 	// Daily XP
		// 	yield return new MissionTemplate
		// 	{
		// 		ProductionId = $"Daily_Overworld_Gain_XP_{line}_Tier{tier}",
		// 		Name = "Gain {0} XP with your " + lineDisplayName + " Illuvials",
		// 		Description = "Gain {0} XP with your " + lineDisplayName + " Illuvials",
		// 		Type = "Daily",
		// 		Game = "Overworld",
		// 		MissionTier = tier,
		// 		Objectives = new[]
		// 		{
		// 			new GainIlluvialExperienceObjectiveTemplate
		// 			{
		// 				Description = "Gain {0} XP with your " + lineDisplayName + " Illuvials",
		// 				Line = line,
		// 				ExperienceNeededMinimum = 300,
		// 				ExperienceNeededMaximum = 1500,
		// 				ExperienceNeededStep = 100
		// 			}
		// 		},
		// 		Rewards = new[]
		// 		{
		// 			new AirdropRewardTemplate
		// 			{
		// 				PointsStep = 100,
		// 				PointsBase = 60
		// 			}
		// 		}
		// 	};
		// }

		// Capture
		foreach (var tier in new[] { 0, 1, 2, 3, 4, 5 })
		{
			yield return new MissionTemplate
			{
				ProductionId = $"Daily_Overworld_Capture_Tier{tier}",
				Name = $"Capture a Tier {tier} Illuvial",
				Description = $"Capture a Tier {tier} Illuvial",
				Type = "Daily",
				Game = "Overworld",
				MissionTier = tier,
				Objectives = new[]
				{
					new CaptureIlluvialObjectiveTemplate
					{
						Description = $"Capture a Tier {tier} Illuvial",
						Tier = tier,
						Quantity = 1
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						Points = tier switch
						{
							5 => 2000,
							4 => 1600,
							3 => 1200,
							2 => 800,
							1 => 400,
							_ => 50
						}
					}
				}
			};
		}
	}

	public static IEnumerable<MissionTemplate> GenerateWeeklyMissions(List<Item> illuvials)
	{
		// Generic XP
		yield return new MissionTemplate
		{
			ProductionId = $"Weekly_Overworld_Gain_XP",
			Name = "Gain {0} XP with your Illuvials",
			Description = "Gain {0} XP with your Illuvials",
			Type = "Weekly",
			Game = "Overworld",
			MissionTier = 0,
			Objectives = new[]
			{
				new GainIlluvialExperienceObjectiveTemplate
				{
					Description = "Gain {0} XP with your Illuvials",
					ExperienceNeededMinimum = 3500,
					ExperienceNeededMaximum = 10500,
					ExperienceNeededStep = 1750
				}
			},
			Rewards = new[]
			{
				new AirdropRewardTemplate
				{
					PointsStep = 480,
					PointsBase = 480
				}
			}
		};


		// Line XP
		// foreach (var (tier, line) in illuvials.Select(x => (x.Tier, x.Line)).Distinct())
		// {
		// 	var lineDisplayName = LineDisplayNames[line];
		// 	// Daily XP
		// 	yield return new MissionTemplate
		// 	{
		// 		ProductionId = $"Daily_Overworld_Gain_XP_{line}_Tier{tier}",
		// 		Name = "Gain {0} XP with your " + lineDisplayName + " Illuvials",
		// 		Description = "Gain {0} XP with your " + lineDisplayName + " Illuvials",
		// 		Type = "Daily",
		// 		Game = "Overworld",
		// 		MissionTier = tier,
		// 		Objectives = new[]
		// 		{
		// 			new GainIlluvialExperienceObjectiveTemplate
		// 			{
		// 				Description = "Gain {0} XP with your " + lineDisplayName + " Illuvials",
		// 				Line = line,
		// 				ExperienceNeededMinimum = 1800,
		// 				ExperienceNeededMaximum = 5400,
		// 				ExperienceNeededStep = 900
		// 			}
		// 		},
		// 		Rewards = new[]
		// 		{
		// 			new AirdropRewardTemplate
		// 			{
		// 				PointsStep = 960,
		// 				PointsBase = 960
		// 			}
		// 		}
		// 	};
		// }

		// Illuvial fusion
		foreach (var tier in new[] { 0, 1, 2, 3, 4, 5 })
		{
			yield return new MissionTemplate
			{
				ProductionId = $"Weekly_Overworld_Fuse_Tier{tier}",
				Name = $"Fuse a Tier {tier} Illuvial",
				Description = $"Fuse a Tier {tier} Illuvial",
				Type = "Weekly",
				Game = "Overworld",
				MissionTier = tier,
				Objectives = new[]
				{
					new CaptureIlluvialObjectiveTemplate
					{
						Description = $"Fuse a Tier {tier} Illuvial",
						Tier = tier,
						Quantity = 1
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						Points = tier switch
						{
							5 => 10000,
							4 => 8000,
							3 => 6000,
							2 => 4000,
							1 => 2000,
							_ => 150
						}
					}
				}
			};
		}

		// Capture
		foreach (var tier in new[] { 0, 1, 2, 3, 4, 5 })
		{
			yield return new MissionTemplate
			{
				ProductionId = $"Weekly_Overworld_Capture_Tier{tier}",
				Name = $"Capture a Tier {tier} Illuvial",
				Description = $"Capture a Tier {tier} Illuvial",
				Type = "Weekly",
				Game = "Overworld",
				MissionTier = tier,
				Objectives = new[]
				{
					new CaptureIlluvialObjectiveTemplate
					{
						Description = $"Capture a Tier {tier} Illuvial",
						Tier = tier,
						QuantityMinimum = 2,
						QuantityMaximum = 6,
						QuantityStep = 2
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						PointsBase = tier switch
						{
							5 => 4000,
							4 => 3200,
							3 => 2400,
							2 => 1600,
							1 => 800,
							_ => 100
						},
						PointsStep = tier switch
						{
							5 => 4000,
							4 => 3200,
							3 => 2400,
							2 => 1600,
							1 => 800,
							_ => 100
						}
					}
				}
			};
		}

		var startDate = DateTime.Parse("2025-02-04");
		var bountySources = new[] { "Overworld", "ADR" };
		var regularBounties = new[]
		{
			new RegularBountyConfiguration(4, 2, 1),
			new RegularBountyConfiguration(4, 3, 3),
			new RegularBountyConfiguration(5, 2, 6),
			new RegularBountyConfiguration(5, 3, 18),
		};
		var megaBounties = new[]
		{
			new MegaBountyConfiguration("Geyyser", "057_Tiktaalik_Stage3_Default_Original", 4),
			new MegaBountyConfiguration("Adorius", "027_Elk_Stage3_Default_Original", 5),
			new MegaBountyConfiguration("Blazenite", "036_Shoebill_Stage3_Default_Original", 4),
			new MegaBountyConfiguration("Dualeph", "033_PistolShrimp_Stage3_Default_Original", 5),
			new MegaBountyConfiguration("Sear", "030_Pangolin_Stage3_Default_Original", 4),
			new MegaBountyConfiguration("Ophisto", "051_Squid_Stage3_Default_Original", 5),
			new MegaBountyConfiguration("Mah’mut", "048_Mammoth_Stage3_Default_Original", 4),
			new MegaBountyConfiguration("Titanor", "021_Beetle_Stage3_Default_Original", 5),
			new MegaBountyConfiguration("Scoriox", "042_Taipan_Stage3_Default_Original", 4),
			new MegaBountyConfiguration("Phosphorus", "093_TerrorBird_Stage3_Default_Original",  5),
			new MegaBountyConfiguration("Jotun", "054_PolarBear_Stage3_Default_Original", 4),
			new MegaBountyConfiguration("Rhamphyre", "006_Pterodactyl_Stage3_Default_Original", 5),
			new MegaBountyConfiguration("Umbre", "012_Thylacine_Stage3_Default_Original", 4),
			new MegaBountyConfiguration("Adorius", "027_Elk_Stage3_Default_Original", 5),
			new MegaBountyConfiguration("Sear", "030_Pangolin_Stage3_Default_Original", 4),
			new MegaBountyConfiguration("Dualeph", "033_PistolShrimp_Stage3_Default_Original", 5),
			new MegaBountyConfiguration("Geyyser", "057_Tiktaalik_Stage3_Default_Original", 4),
			new MegaBountyConfiguration("Ophisto", "051_Squid_Stage3_Default_Original", 5),
			new MegaBountyConfiguration("Blazenite", "036_Shoebill_Stage3_Default_Original", 4),
			new MegaBountyConfiguration("Titanor", "021_Beetle_Stage3_Default_Original", 5),
			new MegaBountyConfiguration("Scoriox", "042_Taipan_Stage3_Default_Original", 4),
			new MegaBountyConfiguration("Phosphorus", "093_TerrorBird_Stage3_Default_Original", 5),
			new MegaBountyConfiguration("Umbre", "012_Thylacine_Stage3_Default_Original", 4),
			new MegaBountyConfiguration("Rhamphyre", "006_Pterodactyl_Stage3_Default_Original", 5),
		};
		var finishMultipliers = new[]
		{
			new FinishMultiplierConfiguration("Colour", "Colour", 1),
			new FinishMultiplierConfiguration("Holo", "Holo", 5),          // X5 in total with regular/mega bounties
			new FinishMultiplierConfiguration("DarkHolo", "Dark Holo", 25) // X25 in total with regular/mega bounties
		};

		// Regular bounties
		foreach (var finishMultiplier in finishMultipliers)
		foreach (var bounty in regularBounties)
		foreach (var bountySource in bountySources)
		{
			var action = bountySource == "ADR" ? "Collect" : "Capture";
			var points = bounty.Points * finishMultiplier.Multiplier;

			var idPrefix = finishMultiplier.Finish != null ? $"_{finishMultiplier.Finish}" : "";
			var idGame = bountySource == "ADR" ? "ADR" : "Overworld";
			var text =
				$"{action} a Tier {bounty.Tier} Stage {bounty.Stage} Illuvial with a {finishMultiplier.FinishName} Finish";
			yield return new MissionTemplate
			{
				ProductionId = $"Event_{idGame}_Bounty{idPrefix}_Tier{bounty.Tier}_Stage{bounty.Stage}",
				Name = text,
				Description = text,
				Type = "Event",
				Game = "Overworld",
				MissionTier = bounty.Tier,
				Quantity = 100, // whatever amount of times player can capture one of those
				Expiry = startDate.AddDays(7 * 12).ToString("yyyy-MM-dd"), // 12 weeks
				Objectives = new[]
				{
					new CaptureIlluvialObjectiveTemplate()
					{
						Description = text,
						Tier = bounty.Tier,
						Stage = bounty.Stage,
						Source = bountySource,
						Finish = finishMultiplier.Finish,
						Quantity = 1
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						Points = points
					}
				}
			};
		}

		// MegaBounties - 2 per week
		var weeklyStartDate = GetLast(DayOfWeek.Monday, startDate);
		foreach (var finishMultiplier in finishMultipliers)
		foreach (var (megaBounty, week) in megaBounties.Chunk(2)
					 .SelectMany((pair, index) => new[] { (pair.First(), index), (pair.Last(), index) }))
		foreach (var bountySource in bountySources)
		{
			var action = bountySource == "ADR" ? "Collect" : "Capture";

			// add X2 points to the regular mission amount
			var multiplier = 2 * finishMultiplier.Multiplier;
			var extraPoints = regularBounties.First(r => r.Tier == megaBounty.Tier && r.Stage == 3).Points * multiplier;

			var idGame = bountySource == "ADR" ? "ADR" : "Overworld";
			var idPrefix = finishMultiplier.Finish != null ? $"_{finishMultiplier.Finish}" : "";
			var missionStartDate = weeklyStartDate.AddDays(7 * week);
			var text = $"{action} {megaBounty.DisplayName} with a {finishMultiplier.FinishName} Finish";
			yield return new MissionTemplate
			{
				ProductionId = $"Weekly_{idGame}_MegaBounty{idPrefix}_Tier{megaBounty.Tier}_Week{week + 1}",
				Name = text,
				Description = text,
				Type = "Weekly",
				Game = "Overworld",
				MissionTier = megaBounty.Tier,
				Quantity = 100, // whatever amount of times player can capture one of those
				Conditions = new ConditionTemplate[]
				{
					// make mission available on specific week
					new TimeConditionTemplate()
					{
						StartTime = missionStartDate.ToString("yyyy-MM-dd"),
						EndTime = missionStartDate.AddDays(7).ToString("yyyy-MM-dd")
					},
				},
				Objectives = new[]
				{
					new CaptureIlluvialObjectiveTemplate()
					{
						Description = text,
						Name = megaBounty.ItemName,
						Quantity = 1,
						Source = bountySource,
						Finish = finishMultiplier.Finish
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						Points = extraPoints
					}
				}
			};
		}
	}

	public static IEnumerable<MissionTemplate> GenerateMilestoneMissions(List<Item> illuvials)
	{
		// catch all
		yield return new MissionTemplate
		{
			ProductionId = $"Milestone_Overworld_Capture_All",
			Name = "Capture All Illuvials",
			Description = "Capture All Illuvials",
			Type = "Milestone",
			Game = "Overworld",
			MissionTier = 0,
			Objectives = illuvials.Select(x => new CaptureIlluvialObjectiveTemplate
			{
				Description = $"Capture a {x.DisplayName}",
				Tier = x.Tier,
				Stage = x.Stage,
				Quantity = 1
			}).ToList(),
			Rewards = new[]
			{
				new AirdropRewardTemplate
				{
					Points = 660000
				}
			}
		};

		// capture all from stage/tier
		var stageTierPoints = new Dictionary<int, Dictionary<int, int>>
		{
			// Stage - Tier - Points
			[1] = new()
			{
				[0] = 15000,
				[1] = 36000,
				[2] = 2000,
				[3] = 48000,
				[4] = 56000,
				[5] = 60000
			},
			[2] = new()
			{
				[0] = 45000,
				[1] = 108000,
				[2] = 30000,
				[3] = 144000,
				[4] = 168000,
				[5] = 180000
			},
			[3] = new()
			{
				[0] = 150000,
				[1] = 300000,
				[2] = 400000,
				[3] = 450000,
				[4] = 500000,
				[5] = 600000
			}
		};

		foreach (var (stage, tierPoints) in stageTierPoints)
		{
			foreach (var (tier, points) in tierPoints)
			{
				yield return new MissionTemplate
				{
					ProductionId = $"Milestone_Overworld_Capture_Tier{tier}_Stage{stage}",
					Name = $"Capture all Stage {stage} Tier {tier} Illuvials",
					Description = $"Capture all Stage {stage} Tier {tier} Illuvials",
					Type = "Milestone",
					Game = "Overworld",
					MissionTier = 0,
					Objectives = illuvials.Where(x => x.Stage == stage && x.Tier == tier)
						.Select(x => new CaptureIlluvialObjectiveTemplate
							{
								Description = $"Capture a {x.DisplayName}",
								Name = x.Name,
								Tier = x.Tier,
								Stage = x.Stage,
								Quantity = 1
							}
						).ToList(),
					Rewards = new[]
					{
						new AirdropRewardTemplate
						{
							Points = points
						}
					}
				};
			}
		}

		// Gain XP milestones
		var xpPoints = new Dictionary<int, int>
		{
			[100_000] = 10_000,
			[200_000] = 20_000,
			[300_000] = 50_000,
			[400_000] = 40_000,
			[500_000] = 50_000,
			[600_000] = 60_000,
			[700_000] = 70_000,
			[800_000] = 80_000,
			[900_000] = 90_000,
			[1_000_000] = 100_000,
			[1_500_000] = 150_000,
			[2_000_000] = 200_000,
			[2_500_000] = 250_000,
			[3_000_000] = 300_000
		};

		foreach (var (xp, points) in xpPoints)
		{
			yield return new MissionTemplate
			{
				ProductionId = $"Milestone_Overworld_Gain_{xp}_XP",
				Name = $"Gain {xp} Illuvial Experience",
				Description = $"Gain {xp} XP with your Illuvials",
				Type = "Milestone",
				Game = "Overworld",
				Objectives = new[]
				{
					new GainIlluvialExperienceObjectiveTemplate
					{
						Description = $"Gain {xp} XP with your Illuvials",
						ExperienceNeeded = xp
					}
				},
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						Points = points
					}
				}
			};
		}

		// fuse
		var fusePoints = new Dictionary<int, int>
		{
			[2] = 20_000,
			[3] = 80_000
		};

		foreach (var (stage, points) in fusePoints)
		{
			yield return new MissionTemplate
			{
				ProductionId = "Milestone_Overworld_Fuse_Stage" + stage,
				Name = "Fuse all stage " + stage + " Illuvials",
				Description = "Fuse all stage " + stage + " Illuvials",
				Type = "Milestone",
				Game = "Overworld",
				Objectives = illuvials.Where(x => x.Stage == stage).Select(x => new FuseIlluvialObjectiveTemplate
				{
					Description = $"Fuse a {x.DisplayName} Illuvial",
					Name = x.Name,
					Stage = x.Stage,
					Quantity = 1
				}).ToList(),
				Rewards = new[]
				{
					new AirdropRewardTemplate
					{
						Points = points
					}
				}
			};
		}

		yield return new MissionTemplate
		{
			ProductionId = "Milestone_Overworld_Fuse_All",
			Name = "Fuse all Illuvials",
			Description = "Fuse all Illuvials",
			Type = "Milestone",
			Game = "Overworld",
			Objectives = illuvials.Where(x => x.Stage == 2 || x.Stage == 3).Select(x => new FuseIlluvialObjectiveTemplate
			{
				Description = $"Fuse a {x.DisplayName} Illuvial",
				Name = x.Name,
				Stage = x.Stage,
				Quantity = 1
			}).ToList(),
			Rewards = new[]
			{
				new AirdropRewardTemplate
				{
					Points = 100_000
				}
			}
		};
	}

	private static DateTime GetLast(DayOfWeek dayOfWeek, DateTime date)
	{
		// reset to start of the week, and add required day of week
		return date.AddDays(-(int) date.DayOfWeek).AddDays((int)dayOfWeek);
	}
}