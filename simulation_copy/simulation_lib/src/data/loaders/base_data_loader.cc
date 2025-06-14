#include "base_data_loader.h"

#include "data/encounter_mod_type_id.h"
#include "ecs/world.h"
#include "json_keys.h"

namespace simulation
{

bool BaseDataLoader::LoadCombatUnitTypeID(
    const nlohmann::json& json_parent,
    const std::string_view key,
    CombatUnitTypeID* out_combat_unit_type_id) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadCombatUnitTypeID";
    // Check for key
    if (!json_parent.contains(key))
    {
        LogErr("{} - json_parent = {}, does not contain key = {}", method_name, json_parent.dump(), key);
        return true;
    }

    const auto& json_object = json_parent[key];
    if (!json_helper_.GetStringValue(json_object, "LineType", &out_combat_unit_type_id->line_name))
    {
        return false;
    }

    if (!json_helper_.GetEnumValue(json_object, "UnitType", &out_combat_unit_type_id->type))
    {
        return false;
    }

    if (out_combat_unit_type_id->type == CombatUnitType::kRanger)
    {
        // Optional for rangers
        GetIntOptionalValue(json_object, JSONKeys::kStage, &out_combat_unit_type_id->stage);
    }
    else
    {
        if (!GetIntValue(json_object, JSONKeys::kStage, &out_combat_unit_type_id->stage))
        {
            return false;
        }
        if (!json_helper_.GetStringValue(json_object, JSONKeys::kPath, &out_combat_unit_type_id->path))
        {
            LogErr("{} - failed to read {} key from json object: {}", method_name, key, json_object.dump());
            return false;
        }

        if (!json_helper_.GetStringValue(json_object, JSONKeys::kVariation, &out_combat_unit_type_id->variation))
        {
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadAugmentInstancesData(
    const nlohmann::json& json_parent,
    const std::string_view key_array,
    std::vector<AugmentInstanceData>* out_augments_instance_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadAugmentInstancesData";

    // Check for key, it is valid if it does not exist
    if (!json_parent.contains(key_array))
    {
        return true;
    }

    const auto& json_array = json_parent[key_array];
    if (!json_array.is_array())
    {
        LogErr("{} - json_parent[{}] = {} is not an array", method_name, key_array, json_parent.dump(4));
        return false;
    }

    // Read from the array
    for (const auto& json_array_value : json_array)
    {
        AugmentInstanceData instance_data;
        if (!LoadAugmentInstanceData(json_array_value, &instance_data))
        {
            LogErr("{} - Can't parse item = {}, from array", method_name, json_array_value.dump());
            return false;
        }

        out_augments_instance_data->push_back(instance_data);
    }

    return true;
}

bool BaseDataLoader::LoadLevelingConfig(const nlohmann::json& json_object, LevelingConfig* out_config) const
{
    GetBoolOptionalValue(json_object, JSONKeys::kEnableLevelingSystem, &out_config->enable_leveling_system);
    GetFixedPointIntOptionalValue(
        json_object,
        JSONKeys::kLevelingStatGrowthPercentage,
        &out_config->leveling_grow_rate_percentage);
    return true;
}

bool BaseDataLoader::LoadLevelingConfig(
    const nlohmann::json& json_parent,
    const std::string_view key,
    LevelingConfig* out_config) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadLevelingConfig";
    return EnsureContainsObjectField(method_name, json_parent, key) && LoadLevelingConfig(json_parent[key], out_config);
}

bool BaseDataLoader::LoadOverloadConfig(const nlohmann::json& json_object, OverloadConfig* out_config) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadOverloadConfig";

    if (!GetBoolValue(json_object, JSONKeys::kEnableOverloadSystem, &out_config->enable_overload_system))
    {
        LogErr(
            "{} - failed to read \"{}\" key. JSON: {}",
            method_name,
            JSONKeys::kEnableOverloadSystem,
            json_object.dump(4));
        return false;
    }

    // Don't need other fields if system is disabled
    if (!out_config->enable_overload_system)
    {
        return true;
    }

    if (!GetIntValue(
            json_object,
            JSONKeys::kStartSecondsApplyOverloadDamage,
            &out_config->start_seconds_apply_overload_damage))
    {
        LogErr(
            "{} - failed to read \"{}\" key. JSON: {}",
            method_name,
            JSONKeys::kStartSecondsApplyOverloadDamage,
            json_object.dump(4));
        return false;
    }

    if (!GetFixedPointIntValue(
            json_object,
            JSONKeys::kIncreaseOverloadDamagePercentage,
            &out_config->increase_overload_damage_percentage))
    {
        LogErr(
            "{} - failed to read \"{}\" key. JSON: {}",
            method_name,
            JSONKeys::kIncreaseOverloadDamagePercentage,
            json_object.dump(4));
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadOverloadConfig(
    const nlohmann::json& json_parent,
    const std::string_view key,
    OverloadConfig* out_config) const
{
    return EnsureContainsObjectField("BaseDataLoader::LoadOverloadConfig", json_parent, key) &&
           LoadOverloadConfig(json_parent[key], out_config);
}

bool BaseDataLoader::LoadBattleConfig(const nlohmann::json& json_object, BattleConfig* out_battle_config) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadBattleConfig";

    int out_random_seed = 0;
    GetIntOptionalValue(json_object, "RandomSeed", &out_random_seed);

    if (out_random_seed < 0)
    {
        LogErr("{} - Random seed is invalid = {}", method_name, out_random_seed);
        return false;
    }

    out_battle_config->random_seed = static_cast<uint64_t>(out_random_seed);

    GetBoolOptionalValue(json_object, "SortByUniqueID", &out_battle_config->sort_by_unique_id);
    GetBoolOptionalValue(
        json_object,
        JSONKeys::kUseMaxStagePlacementRadius,
        &out_battle_config->use_max_stage_placement_radius);

    if (!LoadTeamsEncounterMods(json_object, "EncounterMods", &out_battle_config->teams_encounter_mods))
    {
        LogErr("{} - Failed to read encounter mods", method_name);
        return false;
    }

    GetIntOptionalValue(json_object, JSONKeys::kGridWidth, &out_battle_config->grid_width);
    GetIntOptionalValue(json_object, JSONKeys::kGridHeight, &out_battle_config->grid_height);
    GetIntOptionalValue(json_object, JSONKeys::kGridScale, &out_battle_config->grid_scale);
    GetIntOptionalValue(json_object, JSONKeys::kMiddleLineWidth, &out_battle_config->middle_line_width);

    int max_weapon_amplifiers = static_cast<int>(out_battle_config->max_weapon_amplifiers);
    GetIntOptionalValue(json_object, JSONKeys::kMaxWeaponAmplifiers, &max_weapon_amplifiers);
    out_battle_config->max_weapon_amplifiers = static_cast<size_t>(max_weapon_amplifiers);

    if (json_object.contains(JSONKeys::kAugmentsConfig) &&
        !LoadAugmentsConfig(json_object, JSONKeys::kAugmentsConfig, &out_battle_config->augments_config))
    {
        return false;
    }

    if (json_object.contains(JSONKeys::kConsumablesConfig) &&
        !LoadConsumablesConfig(json_object, JSONKeys::kConsumablesConfig, &out_battle_config->consumables_config))
    {
        return false;
    }

    if (json_object.contains(JSONKeys::kOverloadConfig) &&
        !LoadOverloadConfig(json_object, JSONKeys::kOverloadConfig, &out_battle_config->overload_config))
    {
        return false;
    }

    if (json_object.contains(JSONKeys::kLevelingConfig) &&
        !LoadLevelingConfig(json_object, JSONKeys::kLevelingConfig, &out_battle_config->leveling_config))
    {
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadWorldEffectsConfig(const nlohmann::json& json_object, WorldEffectsConfig* out_effects_config)
    const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadWorldEffectsConfig";

    // Init with default values
    *out_effects_config = WorldEffectsConfig{};

    if (!json_object.is_object())
    {
        LogErr("{} - parameter is not an object. JSON: {}", method_name, json_object.dump(4));
        return false;
    }

    for (const auto condition_type : EnumSet<EffectConditionType>::MakeFull())
    {
        if (!LoadWorldEffectConditionConfig(
                json_object,
                Enum::EffectConditionTypeToString(condition_type),
                &out_effects_config->GetConditionType(condition_type)))
        {
            LogErr("{} - Failed to read world effects config from key \"{}\"", method_name, condition_type);
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadWorldEffectConditionConfig(
    const nlohmann::json& json_object,
    WorldEffectConditionConfig* out_condition_config) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadWorldEffectConditionConfig";

    if (!GetIntValueWithValidation(
            method_name,
            json_object,
            JSONKeys::kDurationMs,
            kMsPerTimeStep,
            std::nullopt,
            &out_condition_config->duration_ms))
    {
        return false;
    }

    if (!GetIntValueWithValidation(
            method_name,
            json_object,
            JSONKeys::kFrequencyTimeMs,
            kMsPerTimeStep,
            std::nullopt,
            &out_condition_config->frequency_time_ms))
    {
        return false;
    }

    if (!GetIntValueWithValidation(
            method_name,
            json_object,
            JSONKeys::kMaxStacks,
            1,
            std::nullopt,
            &out_condition_config->max_stacks))
    {
        return false;
    }

    if (!GetBoolValue(json_object, JSONKeys::kCleanseOnMaxStacks, &out_condition_config->cleanse_on_max_stacks))
    {
        LogErr(
            "{} - Failed to read property \"{}\" from JSON object: {}",
            method_name,
            JSONKeys::kCleanseOnMaxStacks,
            json_object.dump(4));
        return false;
    }

    // Dot properties
    {
        const bool dot_damage_type_specified = json_object.contains(JSONKeys::kDotDamageType);
        if (dot_damage_type_specified)
        {
            if (!json_helper_
                     .GetEnumValue(json_object, JSONKeys::kDotDamageType, &out_condition_config->dot_damage_type))
            {
                LogErr(
                    "{} - Failed to read property \"{}\" from JSON object: {}",
                    method_name,
                    JSONKeys::kDotDamageType,
                    json_object.dump(4));
                return false;
            }
        }

        const bool dot_percentage_specified = json_object.contains(JSONKeys::kDotHighPrecisionPercentage);
        if (dot_percentage_specified)
        {
            if (!GetFixedPointIntValue(
                    json_object,
                    JSONKeys::kDotHighPrecisionPercentage,
                    &out_condition_config->dot_high_precision_percentage))
            {
                LogErr(
                    "{} - Failed to read property \"{}\" from JSON object: {}",
                    method_name,
                    JSONKeys::kDotHighPrecisionPercentage,
                    json_object.dump(4));
                return false;
            }

            if (out_condition_config->dot_high_precision_percentage <= 0_fp)
            {
                LogErr("{} - \"{}\" must be greater than zero", method_name, JSONKeys::kDotHighPrecisionPercentage);
                return false;
            }
        }

        // Only both fields can be present
        if (dot_damage_type_specified ^ dot_percentage_specified)
        {
            if (dot_damage_type_specified)
            {
                LogErr(
                    "{} - \"{}\" field also requires \"{}\" field. JSON: {}",
                    method_name,
                    JSONKeys::kDotDamageType,
                    JSONKeys::kDotHighPrecisionPercentage,
                    json_object.dump(4));
            }

            if (dot_percentage_specified)
            {
                LogErr(
                    "{} - \"{}\" field also requires \"{}\" field. JSON: {}",
                    method_name,
                    JSONKeys::kDotHighPrecisionPercentage,
                    JSONKeys::kDotDamageType,
                    json_object.dump(4));
            }

            return false;
        }
    }

    // Debuff properties
    {
        const bool debuff_stat_type_specified = json_object.contains(JSONKeys::kDebuffStatType);
        if (debuff_stat_type_specified)
        {
            if (!json_helper_
                     .GetEnumValue(json_object, JSONKeys::kDebuffStatType, &out_condition_config->debuff_stat_type))
            {
                LogErr(
                    "{} - Failed to read property \"{}\" from JSON object: {}",
                    method_name,
                    JSONKeys::kDebuffStatType,
                    json_object.dump(4));
                return false;
            }
        }

        const bool debuff_percentage_specified = json_object.contains(JSONKeys::kDebuffPercentage);
        if (debuff_percentage_specified)
        {
            if (!GetFixedPointIntValue(
                    json_object,
                    JSONKeys::kDebuffPercentage,
                    &out_condition_config->debuff_percentage))
            {
                LogErr(
                    "{} - Failed to read property \"{}\" from JSON object: {}",
                    method_name,
                    JSONKeys::kDebuffPercentage,
                    json_object.dump(4));
                return false;
            }

            if (out_condition_config->debuff_percentage <= 0_fp)
            {
                LogErr("{} - \"{}\" must be greater than zero", method_name, JSONKeys::kDebuffPercentage);
                return false;
            }
        }

        // Only both fields can be present
        if (debuff_stat_type_specified ^ debuff_percentage_specified)
        {
            if (debuff_stat_type_specified)
            {
                LogErr(
                    "{} - \"{}\" field also requires \"{}\" field. JSON: {}",
                    method_name,
                    JSONKeys::kDebuffStatType,
                    JSONKeys::kDebuffPercentage,
                    json_object.dump(4));
            }

            if (debuff_percentage_specified)
            {
                LogErr(
                    "{} - \"{}\" field also requires \"{}\" field. JSON: {}",
                    method_name,
                    JSONKeys::kDebuffPercentage,
                    JSONKeys::kDebuffStatType,
                    json_object.dump(4));
            }

            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadWorldEffectConditionConfig(
    const nlohmann::json& parent_json,
    const std::string_view key,
    WorldEffectConditionConfig* out_condition_config) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadWorldEffectConditionConfig";
    return EnsureContainsObjectField(method_name, parent_json, key) &&
           LoadWorldEffectConditionConfig(parent_json[key], out_condition_config);
}

bool BaseDataLoader::LoadTeamsEncounterMods(
    const nlohmann::json& json_object,
    std::unordered_map<Team, std::vector<EncounterModInstanceData>>* out_teams_encounter_mods) const
{
    // Ensure it is an object
    if (!json_object.is_object())
    {
        LogErr(
            "LoadEncounterModsFromJSON - failed to read encounters from {} (expected an object)",
            json_object.dump());
        return false;
    }

    for (const auto& [team_json, encounters_json] : json_object.items())
    {
        Team team = Team::kNone;
        if (!json_helper_.GetEnumValue(team_json, &team))
        {
            return false;
        }

        // By default it is the "Team": [encounter instances]
        const nlohmann::json* encounters_array_json = &encounters_json;

        // Also support unreal format, see FBattleConfig
        static constexpr std::string_view unreal_key_instances = "Instances";
        if (encounters_json.is_object())
        {
            if (!encounters_json.contains(unreal_key_instances))
            {
                LogErr(
                    "LoadEncounterModsFromJSON - encounters_json = {} is an object but does not have required key = {}",
                    encounters_json.dump(),
                    unreal_key_instances);
                return false;
            }

            encounters_array_json = &encounters_json[unreal_key_instances];
        }

        constexpr bool return_of_first_failure = true;
        const bool instances_ok = json_helper_.WalkArray(
            *encounters_array_json,
            return_of_first_failure,
            [&](const size_t index, const nlohmann::json& encounter_instance_json) -> bool
            {
                EncounterModInstanceData encounter_mod_instance_data{};
                if (!LoadEncounterModInstanceData(encounter_instance_json, &encounter_mod_instance_data))
                {
                    LogErr(
                        "LoadEncounterModsFromJSON - failed to read encounter mod type instance data from json \"{}\". "
                        "Index = {}",
                        encounter_instance_json.dump(),
                        index);
                    return false;
                }

                (*out_teams_encounter_mods)[team].push_back(encounter_mod_instance_data);
                return true;
            });

        if (!instances_ok)
        {
            LogErr(
                "LoadEncounterModsFromJSON - encounters_array_json = {} is not valid",
                encounters_array_json->dump());
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadTeamsEncounterMods(
    const nlohmann::json& parent_json,
    const std::string_view key,
    std::unordered_map<Team, std::vector<EncounterModInstanceData>>* out_teams_encounter_mods) const
{
    static constexpr std::string_view method_name = "LoadTeamsEncounterMods";
    if (!parent_json.is_object())
    {
        LogErr("{} - parent_json is not an object. parent_json = {}", method_name, parent_json.dump());
    }

    if (parent_json.contains(key))
    {
        return LoadTeamsEncounterMods(parent_json[key], out_teams_encounter_mods);
    }

    return true;
}

bool BaseDataLoader::LoadEncounterModTypeID(
    const nlohmann::json& json_object,
    EncounterModTypeID* out_encounter_mod_type_id) const
{
    static constexpr std::string_view method_name = "LoadEncounterModTypeIDFromJSON";
    if (!json_object.is_object())
    {
        LogErr("{} - parameter is not an object: {}", method_name, json_object.dump());
        return false;
    }

    if (!GetStringValue(json_object, JSONKeys::kName, &out_encounter_mod_type_id->name))
    {
        LogErr("{} - failed to parse encounter mod name", method_name);
        return false;
    }

    if (!GetIntValue(json_object, JSONKeys::kStage, &out_encounter_mod_type_id->stage))
    {
        LogErr("{} - failed to parse encounter mod stage", method_name);
        return false;
    }

    if (out_encounter_mod_type_id->stage < 0)
    {
        LogErr(
            "{} - encounter stage has negative value {}, which is invalid",
            method_name,
            out_encounter_mod_type_id->stage);
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadEncounterModInstanceData(
    const nlohmann::json& json_object,
    EncounterModInstanceData* out_encounter_mod_instance_data) const
{
    static constexpr std::string_view method_name = "LoadEncounterModInstanceData";
    if (!json_object.is_object())
    {
        LogErr("{} - parameter is not an object: {}", method_name, json_object.dump());
        return false;
    }

    GetStringOptionalValue(json_object, JSONKeys::kID, &out_encounter_mod_instance_data->id);

    if (!json_object.contains(JSONKeys::kTypeID))
    {
        LogErr(
            "{} - invalid format. Encounter mod instance is expected to have {} field. (json: \"{}\")",
            method_name,
            JSONKeys::kTypeID,
            json_object.dump());
        return false;
    }

    return LoadEncounterModTypeID(json_object[JSONKeys::kTypeID], &out_encounter_mod_instance_data->type_id);
}

// Loads all the stats from
bool BaseDataLoader::LoadCombatUnitStats(
    const nlohmann::json& json_object,
    const bool parse_strings_as_fixed_point,
    std::unordered_map<StatType, FixedPoint>* out_map) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadCombatUnitStats";
    if (!json_object.is_object())
    {
        LogErr("{} - json_object = {}, is not an object", method_name, json_object.dump());
        return false;
    }

    for (const auto& [stat_name, stat_value] : json_object.items())
    {
        StatType stat_type = StatType::kNone;
        if (!json_helper_.GetEnumValue(stat_name, &stat_type))
        {
            LogErr("{} - failed to parse stat name = {}", method_name, stat_name);
            return false;
        }

        FixedPoint stat_value_fp = 0_fp;
        bool succeeded = false;

        if (parse_strings_as_fixed_point && stat_value.is_string())
        {
            const std::string& string_repr = stat_value;
            succeeded = FixedPoint::TryParse(string_repr, &stat_value_fp);
        }
        else
        {
            succeeded = GetFixedPointIntValue(stat_value, &stat_value_fp);
        }

        if (succeeded)
        {
            (*out_map)[stat_type] = stat_value_fp;
        }
        else
        {
            LogErr(
                "{} - failed to parse stat \"{}\" with JSON value \"{}\"",
                method_name,
                stat_type,
                stat_value.dump());
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadCombatUnitStats(
    const nlohmann::json& json_parent,
    const std::string_view key,
    const bool parse_strings_as_fixed_point,
    std::unordered_map<StatType, FixedPoint>* out_map) const
{
    assert(out_map);

    static constexpr std::string_view method_name = "BaseDataLoader::LoadCombatUnitStats";

    // Parent is not valid?
    if (!json_parent.is_object())
    {
        LogErr("{} - json_parent = {}, is not an object", method_name, json_parent.dump());
        return false;
    }

    if (!json_parent.contains(key))
    {
        // Stats are optional
        return true;
    }

    const auto& json_object = json_parent[key];
    return LoadCombatUnitStats(json_object, parse_strings_as_fixed_point, out_map);
}

bool BaseDataLoader::GetRawVariableName(
    const nlohmann::json& json_object,
    std::string_view key,
    std::string* out_raw_variable_name) const
{
    // Key does not exist
    if (!json_object.contains(key))
    {
        return false;
    }

    const auto& json_value = json_object[key];
    return GetRawVariableName(json_value, out_raw_variable_name);
}

bool BaseDataLoader::GetRawVariableName(const nlohmann::json& json_value, std::string* out_raw_variable_name) const
{
    // Not a string type
    if (!json_value.is_string())
    {
        return false;
    }

    // Most likely variable
    *out_raw_variable_name = json_value;
    return true;
}

bool BaseDataLoader::GetIntValue(const nlohmann::json& json_value, int* out_int) const
{
    if (!variables_.empty() && json_value.is_string())
    {
        // Check if correct variable name
        std::string raw_variable_name;
        if (GetRawVariableName(json_value, &raw_variable_name))
        {
            return GetVariableValue(raw_variable_name, out_int);
        }

        LogErr("BaseDataLoader::GetIntValue - Can't get variable = {} for integer", std::string{json_value});
        return false;
    }

    return json_helper_.GetIntValue(json_value, out_int);
}

bool BaseDataLoader::GetIntValue(const nlohmann::json& json_object, const std::string_view key, int* out_int) const
{
    if (!json_object.contains(key))
    {
        LogErr("BaseDataLoader::GetIntValue - Key = `{}` does not exist", key);
        return false;
    }

    const bool return_value = GetIntValue(json_object[key], out_int);
    if (!return_value)
    {
        LogErr("BaseDataLoader::GetIntValue - Key = `{}` failed to load int", key);
    }

    return return_value;
}

bool BaseDataLoader::EnsureContainsField(
    const std::string_view caller_method_name,
    const nlohmann::json& parent_json,
    const std::string_view field_key) const
{
    static constexpr std::string_view method_name = "EnsureContainsField";

    if (!parent_json.is_object())
    {
        LogErr(
            "{} (in {}) - Can't take \"{}\" field from non-object JSON value. JSON value: {}",
            caller_method_name,
            method_name,
            field_key,
            parent_json.dump());
        return false;
    }

    if (!parent_json.contains(field_key))
    {
        LogErr("{} (in {}) - \"{}\" field was not found", caller_method_name, method_name, field_key);
        return false;
    }

    return true;
}

bool BaseDataLoader::GetIntValueWithValidation(
    const std::string_view caller_method_name,
    const nlohmann::json& json_parent,
    const std::string_view key,
    std::optional<int> min_value,
    std::optional<int> max_value,
    int* out_int) const
{
    if (!EnsureContainsField(caller_method_name, json_parent, key))
    {
        return false;
    }

    const auto& json_object = json_parent[key];
    if (!GetIntValue(json_object, out_int))
    {
        LogErr(
            "{} - failed to parse field \"{}\" as integer. Field JSON: {}",
            caller_method_name,
            key,
            json_object.dump());
        return false;
    }

    if (min_value.has_value() && *out_int < *min_value)
    {
        LogErr(
            "{} - field \"{}\" has value \"{}\" but required to be at least \"{}\"",
            caller_method_name,
            key,
            *out_int,
            *min_value);
        return false;
    }

    if (max_value.has_value() && *out_int > *max_value)
    {
        LogErr(
            "{} - field \"{}\" has value \"{}\" but required to be less than or equal to \"{}\"",
            caller_method_name,
            key,
            *out_int,
            *max_value);
        return false;
    }

    return true;
}

bool BaseDataLoader::EnsureContainsObjectField(
    const std::string_view caller_method_name,
    const nlohmann::json& parent_json,
    const std::string_view field_key) const
{
    static constexpr std::string_view method_name = "EnsureContainsObjectField";

    if (!EnsureContainsField(caller_method_name, parent_json, field_key))
    {
        LogErr("{} (in {}) - \"{}\" field was not found", caller_method_name, method_name, field_key);
        return false;
    }

    if (!parent_json[field_key].is_object())
    {
        LogErr("{} (in {}) - \"{}\" field is not an object", caller_method_name, method_name, field_key);
        return false;
    }

    return true;
}

bool BaseDataLoader::EnsureContainsArrayField(
    const std::string_view caller_method_name,
    const nlohmann::json& parent_json,
    const std::string_view field_key) const
{
    static constexpr std::string_view method_name = "EnsureContainsArrayField";

    if (!EnsureContainsField(caller_method_name, parent_json, field_key))
    {
        LogErr("{} (in {}) - \"{}\" field was not found", caller_method_name, method_name, field_key);
        return false;
    }

    if (!parent_json[field_key].is_array())
    {
        LogErr("{} (in {}) - \"{}\" field is not an array", caller_method_name, method_name, field_key);
        return false;
    }

    return true;
}

}  // namespace simulation
