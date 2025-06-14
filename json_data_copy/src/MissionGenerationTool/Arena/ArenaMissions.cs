using Illuvium.DataModels;

namespace MissionGenerationTool;

public class ArenaMissions
{
	public static IEnumerable<MissionTemplate> Generate()
	{
		var missions = new List<MissionTemplate>();

		missions.AddRange(ArenaGameMissions.GenerateDailyMissions());
		missions.AddRange(ArenaGameMissions.GenerateWeeklyMissions());
		missions.AddRange(ArenaGameMissions.GenerateMilestoneMissions());

		missions.AddRange(EmoteMissions.GenerateDailyMissions());
		missions.AddRange(EmoteMissions.GenerateWeeklyMissions());
		missions.AddRange(EmoteMissions.GenerateMilestoneMissions());

		missions.AddRange(SurvivalWaveMissions.GenerateDailyMissions());
		missions.AddRange(SurvivalWaveMissions.GenerateWeeklyMissions());
		missions.AddRange(SurvivalWaveMissions.GenerateMilestoneMissions());

		missions.AddRange(LeaderboardMissions.GenerateDailyMissions());
		missions.AddRange(LeaderboardMissions.GenerateWeeklyMissions());
		missions.AddRange(LeaderboardMissions.GenerateMilestoneMissions());

		return missions;
	}
}