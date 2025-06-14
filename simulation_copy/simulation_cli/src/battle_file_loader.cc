#include "battle_file_loader.h"

#include "battle_file_data.h"
#include "data/drone_augment/drone_augment_id.h"
#include "data/loaders/json_keys.h"
#include "utility/string.h"

namespace simulation::tool
{

static constexpr std::string_view key_instance = "Instance";

BattleFileLoader::BattleFileLoader(std::shared_ptr<Logger> logger, fs::path battle_files_path)
    : Super(std::move(logger)),
      battle_files_path_(std::move(battle_files_path))
{
    file_helper_ = std::make_unique<FileHelper>(GetLogger());
}

bool BattleFileLoader::LoadBattleBoardState(const fs::path& file_name, BattleBoardState* out_battle_board_state) const
{
    // Try find battle file
    const std::string file_content = FindBattleFile(file_name);
    if (file_content.empty())
    {
        LogErr("Can't find battle file with give name: {}", file_name);
        return false;
    }

    const auto json = JSONHelper::FromString(file_content);
    if (json.is_null() || !json.is_object())
    {
        LogErr("JSON is invalid! file_content = {}", file_content);
        return false;
    }

    if (!LoadBattleBoardStateFromJSON(json, out_battle_board_state))
    {
        LogErr("Failed to load board state from battle file.");
        return false;
    }

    return true;
}

std::string BattleFileLoader::FindBattleFile(const fs::path& file_name) const
{
    // 1. Search in CWD - just use file name
    if (FileHelper::DoesFileExist(file_name))
    {
        return file_helper_->ReadAllContentFromFile(file_name);
    }

    // 1.5 Binary path absolute path name
    {
        const fs::path& full_path = FileHelper::GetExecutableDirectory() / file_name;
        if (FileHelper::DoesFileExist(full_path))
        {
            return file_helper_->ReadAllContentFromFile(full_path);
        }
    }

    // 2. Search in CWD but with .json extension
    fs::path file_name_with_json(file_name);
    file_name_with_json += ".json";
    if (FileHelper::DoesFileExist(file_name_with_json))
    {
        return file_helper_->ReadAllContentFromFile(file_name_with_json);
    }

    // 2.5 Binary Path, absolute path name with .json extension
    {
        const fs::path& full_path_with_json = FileHelper::GetExecutableDirectory() / file_name_with_json;
        if (FileHelper::DoesFileExist(full_path_with_json))
        {
            return file_helper_->ReadAllContentFromFile(full_path_with_json);
        }
    }

    // 3. Search in battle file folder folder from settings + file name
    const fs::path alternative_path = battle_files_path_ / file_name;
    if (FileHelper::DoesFileExist(alternative_path))
    {
        return file_helper_->ReadAllContentFromFile(alternative_path);
    }

    // 4. Folder from settings + file name + .json
    const fs::path alternative_path_with_json = battle_files_path_ / file_name_with_json;
    if (FileHelper::DoesFileExist(alternative_path_with_json))
    {
        return file_helper_->ReadAllContentFromFile(alternative_path_with_json);
    }

    return {};
}

bool BattleFileLoader::LoadBattleBoardStateFromJSON(
    const nlohmann::json& json_object,
    BattleBoardState* out_battle_board_state) const
{
    if (!GetIntValue(json_object, "Version", &out_battle_board_state->version))
    {
        LogErr("LoadBattleBoardStateFromJSON - Failed to extract version from battle file");
        return false;
    }

    // Optional battle config
    static constexpr std::string_view key_battle_config = "BattleConfig";
    if (json_object.contains(key_battle_config))
    {
        if (!LoadBattleConfig(json_object[key_battle_config], &out_battle_board_state->battle_config))
        {
            LogErr("Failed to parse config data from battle file");
            return false;
        }
    }

    const bool units_parsed = json_helper_.WalkArray(
        json_object,
        "CombatUnits",
        true,
        [&](const size_t, const nlohmann::json& object) -> bool
        {
            BattleCombatUnitState unit_state;
            if (LoadBattleCombatUnitStateFromJSON(object, &unit_state))
            {
                out_battle_board_state->combat_units.emplace_back(unit_state);
                return true;
            }

            return false;
        });

    if (!units_parsed)
    {
        LogErr("Failed to parse units from battle file");
        return false;
    }

    // Optional drone augments
    static constexpr std::string_view key_drone_augments = "DroneAugments";
    if (json_object.contains(key_drone_augments))
    {
        if (!LoadBattleDroneAugmentStateFromJSON(
                json_object[key_drone_augments],
                &out_battle_board_state->drone_augments))
        {
            LogErr("Failed to drone augments data from battle file");
            return false;
        }
    }

    return true;
}

bool BattleFileLoader::LoadBattleCombatUnitStateFromJSON(
    const nlohmann::json& json_object,
    BattleCombatUnitState* out_combat_unit_state) const
{
    static constexpr std::string_view method_name = "BattleFileLoader::LoadBattleCombatUnitStateFromJSON";
    // Type ID
    if (!LoadCombatUnitTypeID(json_object, JSONKeys::kTypeID, &out_combat_unit_state->type_id))
    {
        LogErr("{} - Failed to parse unit id", method_name);
        return false;
    }

    // Instance
    if (!json_object.contains(key_instance))
    {
        LogErr("{} - json_object does not contain key = {}", method_name, key_instance);
        return false;
    }
    if (!LoadBattleCombatUnitInstanceDataFromJSON(
            json_object[key_instance],
            out_combat_unit_state->type_id,
            &out_combat_unit_state->instance))
    {
        LogErr("{} - Failed to parse instance data", method_name);
        return false;
    }

    // Position
    if (!LoadHexGridPosition(json_object, JSONKeys::kPosition, &out_combat_unit_state->position))
    {
        // Also try lower case because unreal is stupid sometimes
        if (!LoadHexGridPosition(
                json_object,
                String::ToLowerCopy(JSONKeys::kPosition),
                &out_combat_unit_state->position))
        {
            return false;
        }
    }

    return true;
}

bool BattleFileLoader::LoadBattleCombatUnitInstanceDataFromJSON(
    const nlohmann::json& json_object,
    const CombatUnitTypeID& type_id,
    CombatUnitInstanceData* out_combat_unit_instance_data) const
{
    // Used an Optional here because regular one prints warning when trying to read "None" string
    json_helper_.GetEnumValueOptional(
        json_object,
        JSONKeys::kDominantCombatAffinity,
        &out_combat_unit_instance_data->dominant_combat_affinity);
    json_helper_.GetEnumValueOptional(
        json_object,
        JSONKeys::kDominantCombatClass,
        &out_combat_unit_instance_data->dominant_combat_class);

    // Load Optional level
    GetIntOptionalValue(json_object, JSONKeys::kLevel, &out_combat_unit_instance_data->level);

    {
        std::unordered_map<StatType, FixedPoint> stat_type_to_percentage;
        const bool parse_strings_as_fixed_point = true;
        if (!LoadCombatUnitStats(
                json_object,
                "RandomPercentageModifierStats",
                parse_strings_as_fixed_point,
                &stat_type_to_percentage))
        {
            return false;
        }

        for (const auto& [stat_type, percentage] : stat_type_to_percentage)
        {
            out_combat_unit_instance_data->random_percentage_modifier_stats.Set(stat_type, percentage);
        }
    }

    if (!json_helper_.GetStringValue(json_object, JSONKeys::kID, &out_combat_unit_instance_data->id))
    {
        return false;
    }

    if (type_id.type == CombatUnitType::kRanger)
    {
        // Can be a missing field
        GetStringOptionalValue(json_object, JSONKeys::kBondedID, &out_combat_unit_instance_data->bonded_id);

        // Weapon
        if (!LoadCombatWeaponInstanceData(
                json_object,
                JSONKeys::kEquippedWeapon,
                &out_combat_unit_instance_data->equipped_weapon))
        {
            return false;
        }

        // Suit
        if (!LoadCombatSuitInstanceData(
                json_object,
                JSONKeys::kEquippedSuit,
                &out_combat_unit_instance_data->equipped_suit))
        {
            return false;
        }
    }
    else
    {
        // Non Ranger
        if (!LoadAugmentInstancesData(
                json_object,
                JSONKeys::kEquippedAugments,
                &out_combat_unit_instance_data->equipped_augments))
        {
            LogErr("LoadBattleCombatUnitInstanceDataFromJSON - Failed to load equipped augments");
            return false;
        }

        if (!LoadConsumableInstancesData(
                json_object,
                JSONKeys::kEquippedConsumables,
                &out_combat_unit_instance_data->equipped_consumables))
        {
            LogErr("LoadBattleCombatUnitInstanceDataFromJSON - Failed to load equipped consumables");
            return false;
        }
    }

    const bool parse_strings_as_fixed_point = false;
    if (!LoadCombatUnitStats(
            json_object,
            JSONKeys::kStatsOverrides,
            parse_strings_as_fixed_point,
            &out_combat_unit_instance_data->stats_overrides))
    {
        LogErr("LoadBattleCombatUnitInstanceDataFromJSON - Failed to load stats overrides");
        return false;
    }

    return true;
}

bool BattleFileLoader::LoadBattleDroneAugmentStateFromJSON(
    const nlohmann::json& json_object,
    std::vector<DroneAugmentState>* out_drone_augments) const
{
    return json_helper_.WalkArray(
        json_object,
        true,
        [&](const size_t, const nlohmann::json& object) -> bool
        {
            DroneAugmentState drone_augment_state;
            if (!LoadDroneAugmentTypeID(object[JSONKeys::kTypeID], &drone_augment_state.type_id))
            {
                LogErr("Failed to read drone augment id \"{}\".", JSONKeys::kTypeID);
                return false;
            }

            if (!json_helper_.GetEnumValue(object, JSONKeys::kTeam, &drone_augment_state.team))
            {
                LogErr("Failed to read team field \"{}\".", JSONKeys::kTeam);
                return false;
            }

            out_drone_augments->emplace_back(std::move(drone_augment_state));
            return true;
        });
}
}  // namespace simulation::tool
