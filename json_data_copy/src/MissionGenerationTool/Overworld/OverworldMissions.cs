using Illuvium.DataModels;

namespace MissionGenerationTool.Overworld;

public class OverworldMissions
{
	public static IEnumerable<MissionTemplate> Generate(List<Item> items)
	{
		var gather = new[] { "Consumable", "Essence", "OverworldOre", "OverworldGemstone", "Shard" };
		var crafting = new[] { "BattleSuit", "BattleWeapon", "CuredShard", "InfusedGemstone", "OverworldIngot", "OverworldDroneEquipment", "Skin" };
		var restrictedCrafting = new[] { "OverworldDroneEquipment", "OverworldEquipment" };
		var illuvials = items.Where(x => x.ItemType == "Illuvial").ToList();
		var morphs = items.Where(x => x.ItemType == "OverworldMorphopod").ToList();
		var gear = items.Where(x => crafting.Contains(x.ItemType))
			.Where(x => !restrictedCrafting.Contains(x.ItemType) || x.Stage.GetValueOrDefault() > 1).ToList();
		var harvest = items.Where(x => gather.Contains(x.ItemType)).ToList();
		var missions = new List<MissionTemplate>();

		missions.AddRange(IlluvialMissions.GenerateDailyMissions(illuvials));
		missions.AddRange(IlluvialMissions.GenerateWeeklyMissions(illuvials));
		missions.AddRange(IlluvialMissions.GenerateMilestoneMissions(illuvials));

		missions.AddRange(MorphopodMissions.GenerateDailyMissions(morphs));
		missions.AddRange(MorphopodMissions.GenerateWeeklyMissions(morphs));

		missions.AddRange(CraftingMissions.GenerateDailyMissions(gear));
		missions.AddRange(CraftingMissions.GenerateWeeklyMissions(gear));
		missions.AddRange(CraftingMissions.GenerateMilestoneMissions(gear));

		missions.AddRange(GatherMissions.GenerateDailyMissions(harvest));
		missions.AddRange(GatherMissions.GenerateWeeklyMissions(harvest));
		missions.AddRange(GatherMissions.GenerateMilestoneMissions(harvest));

		missions.AddRange(EncounterMissions.GenerateDailyMissions());
		missions.AddRange(EncounterMissions.GenerateWeeklyMissions());
		missions.AddRange(EncounterMissions.GenerateMilestoneMissions());

		return missions;
	}
}