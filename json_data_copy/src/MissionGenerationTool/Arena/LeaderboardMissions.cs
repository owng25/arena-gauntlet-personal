using Illuvium.DataModels;

namespace MissionGenerationTool;

public static class LeaderboardMissions
{
	public static IEnumerable<MissionTemplate> GenerateDailyMissions()
	{
		yield break;
	}

	public static IEnumerable<MissionTemplate> GenerateWeeklyMissions()
	{
		yield break;
	}

	public static IEnumerable<MissionTemplate> GenerateMilestoneMissions()
	{
		var type = "Milestone";
		yield return CreateMissionTemplate(type, ArenaGameMissions.Survival, 3, 100, 100_000);
		yield return CreateMissionTemplate(type, ArenaGameMissions.Ascendant, 3, 100, 100_000);

		yield return CreateMissionTemplate(type, ArenaGameMissions.Survival, 4, 10, 200_000);
		yield return CreateMissionTemplate(type, ArenaGameMissions.Ascendant, 4, 10, 200_000);

		yield return CreateMissionTemplate(type, ArenaGameMissions.Survival, 5, 1, 250_000);
		yield return CreateMissionTemplate(type, ArenaGameMissions.Ascendant, 5, 1, 300_000);
	}

	private static MissionTemplate CreateMissionTemplate(string type, string? gameMode, int tier, int rank, int reward)
	{
		return new MissionTemplate
		{
			ProductionId = $"{type}_Arena_Leaderboard_{gameMode}_{rank}",
			Name = $"Reach Rank {{0}} On Arena {gameMode} Leaderboard",
			Description = $"Reach Rank {{0}} On Arena {gameMode} Leaderboard",
			Type = type,
			MissionTier = tier,
			Game = "Arena",
			Objectives = new[]
			{
				new LeaderboardObjectiveTemplate
				{
					Description = $"Reach Rank {{0}} On Arena {gameMode} Leaderboard",
					GameMode= gameMode,
					Rank = rank
				}
			},
			Rewards = new[]
			{
				new AirdropRewardTemplate
				{
					Points = reward
				}
			}
		};
	}
}
