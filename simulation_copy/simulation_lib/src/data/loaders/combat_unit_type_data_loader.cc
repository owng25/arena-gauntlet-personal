#include "base_data_loader.h"
#include "data/ability_data.h"
#include "data/combat_unit_data.h"
#include "data/effect_expression_custom_functions.h"
#include "data/enums.h"
#include "data/source_context.h"
#include "json_keys.h"
#include "utility/json_helper.h"
#include "utility/stats_helper.h"
#include "utility/string.h"

namespace simulation
{

bool BaseDataLoader::LoadCombatUnitData(const nlohmann::json& json_object, CombatUnitData* out_combat_data) const
{
    // Load The CombatUnitTypeID
    CombatUnitTypeID& type_id = out_combat_data->type_id;
    if (!json_helper_.GetStringValue(json_object, "Line", &type_id.line_name))
    {
        return false;
    }

    // Default to illuvial
    type_id.type = CombatUnitType::kIlluvial;

    // Optional type, read (usually only set by non illuvials)
    if (!json_helper_.GetEnumValue(json_object, "UnitType", &type_id.type))
    {
        return false;
    }

    // Load entity size
    if (!GetIntValue(json_object, "SizeUnits", &out_combat_data->radius_units))
    {
        return false;
    }

    if (!LoadCombatUnitStats(json_object, type_id.type, &out_combat_data->type_data.stats))
    {
        return false;
    }

    if (type_id.type != CombatUnitType::kRanger)
    {
        // Load Stage
        if (!GetIntValue(json_object, JSONKeys::kStage, &type_id.stage))
        {
            return false;
        }

        // Illuvial
        if (!LoadCombatUnitTypeData(json_object, &out_combat_data->type_data))
        {
            return false;
        }

        if (!json_helper_.GetStringValue(json_object, JSONKeys::kPath, &type_id.path))
        {
            return false;
        }

        if (!json_helper_.GetStringValue(json_object, JSONKeys::kVariation, &type_id.variation))
        {
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadEffectTypeIDArray(
    const nlohmann::json& json_object,
    std::string_view key_typeid_array,
    std::vector<EffectTypeID>* out_values) const
{
    return json_helper_.WalkArray(
        json_object,
        key_typeid_array,
        true,
        [&](const size_t index, const nlohmann::json& json_array_element) -> bool
        {
            EffectTypeID effect_type_id;
            if (!LoadEffectTypeID(json_array_element, &effect_type_id))
            {
                LogErr(
                    "BaseDataLoader::LoadEffectTypeIDArrayFromJSON - Failed to load effect type id with "
                    "index = {}",
                    index);
                return false;
            }

            out_values->push_back(effect_type_id);
            return true;
        });
}

bool BaseDataLoader::LoadCombatUnitStats(
    const nlohmann::json& json_object,
    const CombatUnitType unit_type,
    StatsData* out_unit_stats) const
{
    // Load stats (checked that it is present above)
    std::unordered_set<StatType> read_stats_set;

    const bool has_stats_key = json_object.contains(JSONKeys::kStats);
    if (has_stats_key && !LoadStats(json_object[JSONKeys::kStats], true, out_unit_stats, &read_stats_set))
    {
        return false;
    }

    // Ranger has all the stats optional
    if (unit_type != CombatUnitType::kRanger)
    {
        if (!has_stats_key)
        {
            return false;
        }

        // Try to load defaults for stats that not loaded from JSON
        LoadMissingStatsDefaults(out_unit_stats, &read_stats_set);

        if (!EnsureAllStatsWereRead(read_stats_set))
        {
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadCombatUnitTypeData(const nlohmann::json& json_object, CombatUnitTypeData* out_type_data) const
{
    json_helper_.GetEnumValueOptional(json_object, JSONKeys::kCombatClass, &out_type_data->combat_class);
    json_helper_.GetEnumValueOptional(json_object, JSONKeys::kCombatAffinity, &out_type_data->combat_affinity);

    json_helper_.GetEnumValueOptional(
        json_object,
        JSONKeys::kDominantCombatClass,
        &out_type_data->dominant_combat_class);
    json_helper_.GetEnumValueOptional(
        json_object,
        JSONKeys::kDominantCombatAffinity,
        &out_type_data->dominant_combat_affinity);

    json_helper_.GetEnumValueOptional(
        json_object,
        JSONKeys::kPreferredLineDominantCombatClass,
        &out_type_data->preferred_line_dominant_combat_class);
    json_helper_.GetEnumValueOptional(
        json_object,
        JSONKeys::kPreferredLineDominantCombatAffinity,
        &out_type_data->preferred_line_dominant_combat_affinity);

    if (!json_object.contains(JSONKeys::kStats))
    {
        LogErr("BaseDataLoader::LoadCombatUnitTypeDataFromJSON - can't get Stats JSON Object");
        return false;
    }

    // Load Tier
    if (!GetIntValue(json_object, JSONKeys::kTier, &out_type_data->tier))
    {
        return false;
    }

    // Attack abilities
    if (json_object.contains(JSONKeys::kAttackAbilities))
    {
        SourceContextData context;
        context.combat_unit_ability_type = AbilityType::kAttack;
        if (!LoadAbilitiesData(
                json_object,
                AbilityType::kAttack,
                JSONKeys::kAttackAbilities,
                context,
                &out_type_data->attack_abilities))
        {
            return false;
        }
    }

    // Omega abilities
    if (json_object.contains(JSONKeys::kOmegaAbilities))
    {
        SourceContextData context;
        context.combat_unit_ability_type = AbilityType::kOmega;
        if (!LoadAbilitiesData(
                json_object,
                AbilityType::kOmega,
                JSONKeys::kOmegaAbilities,
                context,
                &out_type_data->omega_abilities))
        {
            return false;
        }
    }

    // Innate abilities
    if (json_object.contains(JSONKeys::kInnateAbilities))
    {
        SourceContextData context;
        context.combat_unit_ability_type = AbilityType::kInnate;
        if (!LoadAbilitiesData(
                json_object,
                AbilityType::kInnate,
                JSONKeys::kInnateAbilities,
                context,
                &out_type_data->innate_abilities))
        {
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadAbilitiesData(
    const nlohmann::json& json_object,
    const AbilityType ability_type,
    std::string_view key_abilities_array,
    const SourceContextData& source_context,
    AbilitiesData* out_abilities) const
{
    const AbilityLoadingOptions options{};
    return LoadAbilitiesData(json_object, ability_type, key_abilities_array, source_context, options, out_abilities);
}

bool BaseDataLoader::LoadAbilitiesData(
    const nlohmann::json& json_object,
    const AbilityType ability_type,
    std::string_view key_abilities_array,
    const SourceContextData& source_context,
    const AbilityLoadingOptions& options,
    AbilitiesData* out_abilities) const
{
    // Read abilities selection only for non innates
    if (ability_type == AbilityType::kInnate)
    {
        out_abilities->selection_type = AbilitySelectionType::kCycle;
    }
    else
    {
        // Non Innates
        const std::string key_selection = String::Concat(key_abilities_array, "Selection");
        if (!json_helper_.GetEnumValue(json_object, key_selection, &out_abilities->selection_type))
        {
            return false;
        }
    }

    static constexpr std::string_view suffix_check_value = "ActivationCheckValue";
    // Read attributes depending on the selection type
    switch (out_abilities->selection_type)
    {
    case AbilitySelectionType::kEveryXActivations:
    {
        static constexpr std::string_view suffix_cadence = "ActivationCadence";
        const std::string key_cadence = String::Concat(key_abilities_array, suffix_cadence);
        if (!GetIntValue(json_object, key_cadence, &out_abilities->activation_cadence))
        {
            return false;
        }
        break;
    }
    case AbilitySelectionType::kSelfAttributeCheck:
    {
        const std::string key_check_value = String::Concat(key_abilities_array, suffix_check_value);
        int value = 0;
        if (!GetIntValue(json_object, key_check_value, &value))
        {
            return false;
        }
        out_abilities->activation_check_value = FixedPoint::FromInt(value);

        static constexpr std::string_view suffix_check_stat_type = "ActivationCheckStatType";
        const std::string key_stat_type = String::Concat(key_abilities_array, suffix_check_stat_type);
        if (!json_helper_.GetEnumValue(json_object, key_stat_type, &out_abilities->activation_check_stat_type))
        {
            return false;
        }
        break;
    }
    case AbilitySelectionType::kSelfHealthCheck:
    {
        const std::string key_check_value = String::Concat(key_abilities_array, suffix_check_value);
        int value = 0;
        if (!GetIntValue(json_object, key_check_value, &value))
        {
            return false;
        }
        out_abilities->activation_check_value = FixedPoint::FromInt(value);
        break;
    }
    default:
        break;
    }

    // Read Abilities Array
    const bool can_parse_abilities = json_helper_.WalkArray(
        json_object,
        key_abilities_array,
        true,
        [&](const size_t index, const nlohmann::json& json_array_element) -> bool
        {
            // NOTE: No need to add the ability to the array as it is already there
            AbilityData& ability = out_abilities->AddAbility();
            ability.source_context = source_context;

            if (!LoadAbility(json_array_element, ability_type, options, &ability))
            {
                LogErr(
                    "BaseDataLoader::LoadAbilitiesFromJSON - Failed to extract object from array  key = "
                    "`{}`, index = {}",
                    key_abilities_array,
                    index);
                return false;
            }

            // Change defaults for amplifiers
            if (ability.update_type == AbilityUpdateType::kNone &&
                source_context.from_weapon_type == WeaponType::kAmplifier)
            {
                if (source_context.combat_unit_ability_type == AbilityType::kOmega ||
                    source_context.combat_unit_ability_type == AbilityType::kAttack)
                {
                    ability.update_type = AbilityUpdateType::kReplace;
                }
                else if (source_context.combat_unit_ability_type == AbilityType::kInnate)
                {
                    ability.update_type = AbilityUpdateType::kAdd;
                }
            }

            return true;
        });

    if (!can_parse_abilities)
    {
        LogErr("BaseDataLoader::LoadAbilitiesFromJSON - Failed to extract parse Key = `{}`", key_abilities_array);
        return false;
    }

    // Everything loaded okay
    return true;
}

bool BaseDataLoader::LoadAbilityDuration(const nlohmann::json& json_object, int* out_ability_duration) const
{
    if (!GetIntValue(json_object, JSONKeys::kTotalDurationMs, out_ability_duration))
    {
        return false;
    }

    const int duration = *out_ability_duration;
    if (duration >= 0)
    {
        return true;
    }

    // Want to give detailed explanation for -1 value
    if (duration == -1)
    {
        LogErr(
            "BaseDataLoader::LoadAbilityDuration - key {} has value -1. It is allowed in effects to "
            "denote an infinite duration but is not allowed in abilities.",
            JSONKeys::kTotalDurationMs);
    }
    else
    {
        LogErr(
            "BaseDataLoader::LoadAbilityDuration - key {} has negative value {}.",
            JSONKeys::kTotalDurationMs,
            duration);
    }

    return false;
}

bool BaseDataLoader::LoadAbility(
    const nlohmann::json& json_object,
    const AbilityType ability_type,
    AbilityData* out_ability) const
{
    const AbilityLoadingOptions options{};
    return LoadAbility(json_object, ability_type, options, out_ability);
}

bool BaseDataLoader::LoadAbility(
    const nlohmann::json& json_object,
    const AbilityType ability_type,
    const AbilityLoadingOptions& options,
    AbilityData* out_ability) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadAbility";

    if (!json_object.is_object())
    {
        LogErr("{} - json_object is not of type Object", method_name);
        return false;
    }

    // Ability Name is required
    if (!json_helper_.GetStringValue(json_object, JSONKeys::kName, &out_ability->name))
    {
        return false;
    }

    // MovementLock is optional
    static constexpr std::string_view key_movement_lock = "MovementLock";
    GetBoolOptionalValue(json_object, key_movement_lock, &out_ability->movement_lock);

    json_helper_.GetEnumValueOptional(json_object, JSONKeys::kUpdateType, &out_ability->update_type);

    // Require timings to be present if not a attack ability
    if (ability_type != AbilityType::kAttack)
    {
        if (!LoadAbilityDuration(json_object, &out_ability->total_duration_ms))
        {
            return false;
        }
    }

    // Require innate to have activation trigger if options do not explicitly specify opposite
    const bool require_trigger = ability_type == AbilityType::kInnate && options.require_trigger;
    if (require_trigger)
    {
        if (!LoadAbilityActivationTriggerData(
                json_object,
                JSONKeys::kActivationTrigger,
                &out_ability->activation_trigger_data))
        {
            return false;
        }
    }
    else if (json_object.contains(JSONKeys::kActivationTrigger))
    {
        LogErr("{} - Ability in this context must not have \"{}\"", method_name, JSONKeys::kActivationTrigger);
        return false;
    }

    // Read Skills Array
    static constexpr std::string_view key_skills = "Skills";
    const bool can_parse_skills = json_helper_.WalkArray(
        json_object,
        key_skills,
        true,
        [&](const size_t index, const nlohmann::json& json_array_element) -> bool
        {
            // NOTE: No need to add the skill to the array as it is already there
            auto& skill = out_ability->AddSkill();
            if (!LoadSkill(json_array_element, ability_type, &skill))
            {
                LogErr(
                    "{} - Failed to extract object skills array for key = `{}`, index = {}",
                    method_name,
                    key_skills,
                    index);
                return false;
            }

            if (skill.deployment.type == SkillDeploymentType::kDash)
            {
                const auto skill_duration = FixedPoint::FromInt(skill.percentage_of_ability_duration)
                                                .AsPercentageOf(FixedPoint::FromInt(out_ability->total_duration_ms));
                if (skill_duration <= 0_fp)
                {
                    LogErr(
                        "{} - Dash skill (at index \"{}\") has zero duration, which is not allowed. Skill json: {}",
                        method_name,
                        index,
                        json_array_element.dump(4));
                    return false;
                }
            }

            return true;
        });
    if (!can_parse_skills)
    {
        LogErr("BaseDataLoader::LoadAbility - Failed to extract parse Key = `{}`", key_skills);
        return false;
    }

    // Everything loaded okay
    return true;
}

bool BaseDataLoader::EnsureSpecificStatsWereRead(
    const StatType* check_stats_array,
    const size_t check_stats_count,
    const std::unordered_set<StatType>& read_stats_set) const
{
    // Iterate over all stats, send error if stat did not load
    for (size_t i = 0; i != check_stats_count; ++i)
    {
        const StatType type = check_stats_array[i];
        if (read_stats_set.count(type) == 0)
        {
            LogErr("BaseDataLoader::EnsureSpecificStatsWereRead - failed to load stat_type {}", type);
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadStats(
    const nlohmann::json& stats_json_object,
    const bool all_keys_are_stats,
    StatsData* out_stats_data,
    std::unordered_set<StatType>* out_read_stats_set) const
{
    for (const auto& [key, stat_json_value] : stats_json_object.items())
    {
        const StatType stat_type = Enum::StringToStatType(key);
        if (stat_type == StatType::kNone)
        {
            if (all_keys_are_stats)
            {
                LogErr("BaseDataLoader::LoadStatsFromJSON - key {} doesn't exist as stat type", key);
                return false;
            }

            continue;
        }

        int stat_value = 0;
        if (!json_helper_.GetIntValue(stat_json_value, &stat_value))
        {
            LogErr("BaseDataLoader::LoadStatsFromJSON - failed to parse {} stat value (an integer expected)", key);
            return false;
        }

        out_stats_data->Set(stat_type, FixedPoint::FromInt(stat_value));

        if (out_read_stats_set)
        {
            out_read_stats_set->emplace(stat_type);
        }
    }

    const FixedPoint& omega_range_units = out_stats_data->Get(StatType::kOmegaRangeUnits);
    if (omega_range_units < out_stats_data->Get(StatType::kAttackRangeUnits) && omega_range_units != 0_fp)
    {
        LogErr(
            "BaseDataLoader::LoadCombatUnitTypeDataFromJSON - OmegaRangeUnits = {}, was less than "
            "AttackRangeUnits = {}, "
            "clamped value to AttackRangeUnits",
            omega_range_units,
            out_stats_data->Get(StatType::kAttackRangeUnits));
        return false;
    }

    return true;
}

void BaseDataLoader::LoadMissingStatsDefaults(
    StatsData* out_stats_data,
    std::unordered_set<StatType>* out_read_stats_set) const
{
    // Iterate over all fields, send error if stat fails to load
    for (const StatType stat : EnumSet<StatType>::MakeFull())
    {
        LoadStatDefaultIfMissing(stat, out_stats_data, out_read_stats_set);
    }
}

void BaseDataLoader::LoadStatDefaultIfMissing(
    const StatType stat,
    StatsData* out_stats_data,
    std::unordered_set<StatType>* out_read_stats_set) const
{
    if (StatsHelper::IsStatIgnoredForReading(stat))
    {
        return;
    }

    // Already exists
    if (out_read_stats_set->contains(stat))
    {
        return;
    }

    FixedPoint out_default_value;
    if (!StatsHelper::GetDefaultStatValue(stat, &out_default_value))
    {
        LogWarn("BaseDataLoader::LoadStatDefaultIfMissing - Failed to read default for stat = {}", stat);
        return;
    }

    out_stats_data->Set(stat, out_default_value);
    out_read_stats_set->insert(stat);
}

bool BaseDataLoader::EnsureAllStatsWereRead(const std::unordered_set<StatType>& read_stats_set) const
{
    constexpr int start_index = static_cast<int>(StatType::kNone) + 1;
    constexpr int end_index = static_cast<int>(StatType::kNum);

    // Iterate over all fields, send error if stat fails to load
    for (int stat_index = start_index; stat_index < end_index; stat_index++)
    {
        const StatType stat = static_cast<StatType>(stat_index);
        if (StatsHelper::IsStatIgnoredForReading(stat))
        {
            continue;
        }

        if (!read_stats_set.contains(stat))
        {
            LogErr("BaseDataLoader::LoadAndCheckStatsFromJSON - failed to load stat_type {}", stat);
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadSkillTargetingData(
    const nlohmann::json& json_parent,
    const std::string_view key,
    SkillTargetingData* out_targeting_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadSkillTargetingData";
    return EnsureContainsObjectField(method_name, json_parent, key) &&
           LoadSkillTargetingData(json_parent[key], out_targeting_data);
}

bool BaseDataLoader::LoadSkillTargetingData(const nlohmann::json& json_object, SkillTargetingData* out_targeting) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadSkillTargetingData";

    if (!json_helper_.GetEnumValue(json_object, JSONKeys::kType, &out_targeting->type))
    {
        return false;
    }

    // Optional, defaults to Ground if missing
    if (json_object.contains(JSONKeys::kGuidance))
    {
        if (!LoadGuidancesSet(json_object, JSONKeys::kGuidance, &out_targeting->guidance))
        {
            return false;
        }
    }

    if (SkillTargetingData::UsesTargetingGroup(out_targeting->type))
    {
        if (!json_helper_.GetEnumValue(json_object, JSONKeys::kGroup, &out_targeting->group))
        {
            return false;
        }
    }

    if (SkillTargetingData::UsesTargetingStatType(out_targeting->type))
    {
        if (!json_helper_.GetEnumValue(json_object, JSONKeys::kStat, &out_targeting->stat_type))
        {
            return false;
        }
    }

    if (SkillTargetingData::UsesTargetingLowest(out_targeting->type))
    {
        if (!GetBoolValue(json_object, JSONKeys::kLowest, &out_targeting->lowest))
        {
            return false;
        }
    }

    if (SkillTargetingData::UsesDistanceCheck(out_targeting->type))
    {
        if (!json_helper_.GetBoolValue(json_object, JSONKeys::kLowest, &out_targeting->lowest))
        {
            return false;
        }
    }

    if (SkillTargetingData::UsesTargetingNum(out_targeting->type))
    {
        int targeting_num = 0;
        if (!GetIntValue(json_object, JSONKeys::kNum, &targeting_num))
        {
            return false;
        }
        out_targeting->num = static_cast<size_t>(targeting_num);
    }
    else if (SkillTargetingData::UsesTargetingNumOptional(out_targeting->type))
    {
        int targeting_num = 0;
        if (GetIntOptionalValue(json_object, JSONKeys::kNum, &targeting_num))
        {
            out_targeting->num = static_cast<size_t>(targeting_num);
        }
    }

    if (SkillTargetingData::UsesTargetingRadiusUnits(out_targeting->type))
    {
        if (!GetIntValue(json_object, JSONKeys::kRadiusUnits, &out_targeting->radius_units))
        {
            return false;
        }
    }

    if (SkillTargetingData::UsesTargetingCombatSynergy(out_targeting->type))
    {
        // Combat Class/Affinity and negation fields are optional but at least one of them must be present
        if (json_object.contains(JSONKeys::kCombatClass) || json_object.contains(JSONKeys::kCombatAffinity))
        {
            if (!json_helper_.GetCombatSynergy(json_object, "", &out_targeting->combat_synergy))
            {
                return false;
            }
        }

        if (json_object.contains(JSONKeys::kNotCombatClass) || json_object.contains(JSONKeys::kNotCombatAffinity))
        {
            if (!json_helper_.GetCombatSynergy(json_object, "Not", &out_targeting->not_combat_synergy))
            {
                return false;
            }
        }

        if (out_targeting->combat_synergy.IsEmpty() && out_targeting->not_combat_synergy.IsEmpty())
        {
            LogErr(
                "{} - {} targeting requires synergy filter to be specified, so at least one of these filters must be "
                "used: \"{}\", \"{}\", \"{}\" or \"{}\". JSON object: {}",
                method_name,
                Enum::SkillTargetingTypeToString(out_targeting->type),
                JSONKeys::kCombatClass,
                JSONKeys::kCombatAffinity,
                JSONKeys::kNotCombatClass,
                JSONKeys::kNotCombatAffinity,
                json_object.dump());
            return false;
        }

        auto log_invalid_synergy_filter = [&](const std::string_view key, const std::string_view not_key)
        {
            if (out_targeting->combat_synergy == out_targeting->not_combat_synergy)
            {
                LogErr(
                    "{} - \"{}\" and \"{}\" properties have the same value and thus mutually exclusive. Targeting "
                    "JSON: {}",
                    method_name,
                    key,
                    not_key,
                    json_object.dump());
            }
            else
            {
                LogErr(
                    "{} - \"{}\" and \"{}\" specified at the same time so \"{}\" is excessive. Targeting JSON: {}",
                    method_name,
                    key,
                    not_key,
                    not_key,
                    json_object.dump());
            }
        };

        if (out_targeting->combat_synergy.IsCombatClass() && out_targeting->not_combat_synergy.IsCombatClass())
        {
            log_invalid_synergy_filter(JSONKeys::kCombatClass, JSONKeys::kNotCombatClass);
            return false;
        }

        if (out_targeting->combat_synergy.IsCombatAffinity() && out_targeting->not_combat_synergy.IsCombatAffinity())
        {
            log_invalid_synergy_filter(JSONKeys::kCombatAffinity, JSONKeys::kNotCombatAffinity);
            return false;
        }
    }

    if (SkillTargetingData::UsesTargetingTier(out_targeting->type))
    {
        if (!GetIntValue(json_object, JSONKeys::kTier, &out_targeting->tier))
        {
            return false;
        }
    }

    // targeting expression
    if (json_object.contains(JSONKeys::kExpression))
    {
        if (!LoadEffectExpression(json_object, JSONKeys::kExpression, &out_targeting->expression))
        {
            return false;
        }
    }

    GetBoolOptionalValue(json_object, JSONKeys::kSelf, &out_targeting->self);
    GetBoolOptionalValue(json_object, JSONKeys::kOnlyCurrentFocusers, &out_targeting->only_current_focusers);

    if (out_targeting->only_current_focusers && !SkillTargetingData::SupportsCurrentFocusers(out_targeting->type))
    {
        LogErr(
            "{} - \"{}\" is not compatible with \"{}\" targeting type",
            method_name,
            JSONKeys::kOnlyCurrentFocusers,
            out_targeting->type);
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadSkillDeploymentData(
    const nlohmann::json& json_parent,
    const std::string_view key,
    SkillDeploymentData* out_deployment_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadSkillDeploymentData";
    return EnsureContainsObjectField(method_name, json_parent, key) &&
           LoadSkillDeploymentData(json_parent[key], out_deployment_data);
}

bool BaseDataLoader::LoadSkillDeploymentData(
    const nlohmann::json& json_object,
    SkillDeploymentData* out_deployment_data) const
{
    if (!json_helper_.GetEnumValue(json_object, JSONKeys::kType, &out_deployment_data->type))
    {
        return false;
    }

    // Optional, defaults to Ground if missing
    if (json_object.contains(JSONKeys::kGuidance))
    {
        if (!LoadGuidancesSet(json_object, JSONKeys::kGuidance, &out_deployment_data->guidance))
        {
            return false;
        }
    }

    // Optional, defaults to 0 if missing
    GetIntOptionalValue(
        json_object,
        JSONKeys::kPreDeploymentDelayPercentage,
        &out_deployment_data->pre_deployment_delay_percentage);

    // Optional, defaults to 0 if missing
    GetIntOptionalValue(
        json_object,
        "PreDeploymentRetargetingPercentage",
        &out_deployment_data->pre_deployment_retargeting_percentage);

    return true;
}

bool BaseDataLoader::LoadSkill(const nlohmann::json& json_object, const AbilityType ability_type, SkillData* out_skill)
    const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadSkill";
    out_skill->SetDefaults(ability_type);

    if (!json_object.is_object())
    {
        LogErr("{} - json_object is not of type Object", method_name);
        return false;
    }
    if (!json_helper_.GetStringValue(json_object, JSONKeys::kName, &out_skill->name))
    {
        return false;
    }

    if (!LoadSkillDeploymentData(json_object, "Deployment", &out_skill->deployment))
    {
        LogErr("{} - failed to load deployment data", method_name);
        return false;
    }

    // Only require duration for omegas as attack abilities are by default 100%
    if (ability_type == AbilityType::kOmega)
    {
        if (!GetIntValue(json_object, "PercentageOfAbilityDuration", &out_skill->percentage_of_ability_duration))
        {
            return false;
        }
    }

    GetBoolOptionalValue(json_object, "SpreadEffectPackage", &out_skill->spread_effect_package);

    switch (out_skill->deployment.type)
    {
    case SkillDeploymentType::kProjectile:
    {
        if (!LoadSkillProjectileData(json_object, "Projectile", &out_skill->projectile))
        {
            return false;
        }

        break;
    }
    case SkillDeploymentType::kZone:
    {
        if (!LoadSkillZoneData(json_object, "Zone", &out_skill->zone))
        {
            return false;
        }
        break;
    }
    case SkillDeploymentType::kBeam:
    {
        if (!LoadSkillBeamData(json_object, "Beam", &out_skill->beam))
        {
            return false;
        }

        break;
    }
    case SkillDeploymentType::kDash:
    {
        if (!LoadSkillDashData(json_object, "Dash", &out_skill->dash))
        {
            return false;
        }

        break;
    }
    case SkillDeploymentType::kSpawnedCombatUnit:
    {
        if (!LoadSkillSpawnedCombatUnitData(json_object, "SpawnCombatUnit", &out_skill->spawn))
        {
            return false;
        }
        break;
    }
    default:
        break;
    }

    // Read ChannelTimeMs
    static constexpr std::string_view key_channel_time_ms = "ChannelTimeMs";
    if (!out_skill->IsChannelTimeMsOptional() || json_object.contains(key_channel_time_ms))
    {
        if (!GetIntValue(json_object, key_channel_time_ms, &out_skill->channel_time_ms))
        {
            return false;
        }
    }

    // Read Effect Package
    if (!LoadEffectPackage(json_object, &out_skill->effect_package))
    {
        return false;
    }

    if (!LoadSkillTargetingData(json_object, JSONKeys::kTargeting, &out_skill->targeting))
    {
        return false;
    }

    if (!SkillTargetingData::SupportsDeploymentType(out_skill->targeting.type, out_skill->deployment.type))
    {
        LogErr(
            "{}: \"{}\" deplyment type is not supported by \"{}\" targeting type",
            method_name,
            out_skill->deployment.type,
            out_skill->targeting.type);
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadSkillDashData(
    const nlohmann::json& parent_json_object,
    const std::string_view key,
    SkillDashData* out_dash_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadSkillDashData";
    if (!EnsureContainsObjectField(method_name, parent_json_object, key))
    {
        return false;
    }

    return LoadSkillDashData(parent_json_object[key], out_dash_data);
}

bool BaseDataLoader::LoadSkillDashData(const nlohmann::json& json_object, SkillDashData* out_dash_data) const
{
    // Defaults
    out_dash_data->land_behind = false;
    out_dash_data->apply_to_all = false;

    GetBoolOptionalValue(json_object, "LandBehind", &out_dash_data->land_behind);

    if (!GetBoolValue(json_object, "ApplyToAll", &out_dash_data->apply_to_all))
    {
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadSkillSpawnedCombatUnitData(
    const nlohmann::json& parent_json_object,
    const std::string_view key,
    SkillSpawnedCombatUnitData* out_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadSkillSpawnedCombatUnitData";
    if (!EnsureContainsObjectField(method_name, parent_json_object, key))
    {
        return false;
    }

    return LoadSkillSpawnedCombatUnitData(parent_json_object[key], out_data);
}

bool BaseDataLoader::LoadSkillSpawnedCombatUnitData(
    const nlohmann::json& json_object,
    SkillSpawnedCombatUnitData* out_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadSkillSpawnedCombatUnitData";

    // Defaults
    out_data->linked = false;
    out_data->direction = HexGridCardinalDirection::kNone;
    out_data->on_reserved_position = false;

    static constexpr std::string_view key_combat_unit = "CombatUnit";

    if (!json_object.contains(key_combat_unit))
    {
        LogErr(
            "{} - json object does not contain {} field required for {} deployment type.",
            method_name,
            key_combat_unit,
            SkillDeploymentType::kSpawnedCombatUnit);
        return false;
    }

    const nlohmann::json& combat_unit_json = json_object[key_combat_unit];
    if (!combat_unit_json.is_object())
    {
        LogErr("{} - \"{}\" key is not and object.", method_name, key_combat_unit);
        return false;
    }

    auto combat_unit_ptr = std::make_shared<CombatUnitData>();
    if (!LoadCombatUnitData(json_object[key_combat_unit], combat_unit_ptr.get()))
    {
        LogErr("{} - failed to load spawned combat unit data.", method_name);
        return false;
    }
    out_data->combat_unit = combat_unit_ptr;

    GetBoolOptionalValue(json_object, "Linked", &out_data->linked);

    if (!GetBoolOptionalValue(json_object, "OnReservedPosition", &out_data->on_reserved_position))
    {
        if (!json_helper_.GetEnumValue(json_object, "Direction", &out_data->direction))
        {
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadSkillZoneData(
    const nlohmann::json& parent_json_object,
    const std::string_view key,
    SkillZoneData* out_data) const
{
    static constexpr std::string_view method_name = "LoadSkillZoneData";
    if (!EnsureContainsObjectField(method_name, parent_json_object, key))
    {
        return false;
    }

    return LoadSkillZoneData(parent_json_object[key], out_data);
}

bool BaseDataLoader::LoadSkillZoneData(const nlohmann::json& json_object, SkillZoneData* out_data) const
{
    // Defaults
    out_data->shape = ZoneEffectShape::kHexagon;
    out_data->radius_units = 75;
    out_data->width_units = 150;
    out_data->height_units = 150;
    out_data->duration_ms = 1000;
    out_data->frequency_ms = 500;
    out_data->movement_speed_sub_units_per_time_step = 0;
    out_data->growth_rate_sub_units_per_time_step = 0;

    json_helper_.GetEnumValue(json_object, "Shape", &out_data->shape);
    if (out_data->shape == ZoneEffectShape::kRectangle)
    {
        if (!GetIntValue(json_object, "WidthUnits", &out_data->width_units))
        {
            return false;
        }
        if (!GetIntValue(json_object, "HeightUnits", &out_data->height_units))
        {
            return false;
        }
    }
    else
    {
        if (!GetIntValue(json_object, JSONKeys::kRadiusUnits, &out_data->radius_units))
        {
            return false;
        }
    }
    if (out_data->shape == ZoneEffectShape::kTriangle)
    {
        if (!GetIntValue(json_object, "DirectionDegrees", &out_data->direction_degrees))
        {
            return false;
        }
    }

    static constexpr std::string_view key_zone_duration_ms = "DurationMs";
    if (!GetIntValue(json_object, key_zone_duration_ms, &out_data->duration_ms))
    {
        return false;
    }
    if (!GetIntValue(json_object, "FrequencyMs", &out_data->frequency_ms))
    {
        return false;
    }

    GetBoolOptionalValue(json_object, "Attach", &out_data->attach_to_target);
    GetBoolOptionalValue(json_object, "DestroyWithSender", &out_data->destroy_with_sender);
    GetIntOptionalValue(json_object, "MovementSpeedSubUnits", &out_data->movement_speed_sub_units_per_time_step);
    GetIntOptionalValue(json_object, "GrowthRateSubUnits", &out_data->growth_rate_sub_units_per_time_step);
    GetIntOptionalValue(json_object, "MaxRadiusUnits", &out_data->max_radius_units);
    GetIntOptionalValue(json_object, "GlobalCollisionID", &out_data->global_collision_id);
    GetEnumOptionalValue(json_object, "PredefinedSpawnPosition", &out_data->predefined_spawn_position);
    GetEnumOptionalValue(json_object, "PredefinedTargetPosition", &out_data->predefined_target_position);

    if (json_object.contains(JSONKeys::kSkipActivations))
    {
        if (!json_helper_.GetIntArray(json_object, JSONKeys::kSkipActivations, &out_data->skip_activations))
        {
            return false;
        }
    }

    if (!GetBoolValue(json_object, "ApplyOnce", &out_data->apply_once))
    {
        return false;
    }
    if (out_data->movement_speed_sub_units_per_time_step > 0)
    {
        if (!GetIntValue(json_object, key_zone_duration_ms, &out_data->duration_ms))
        {
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadSkillProjectileData(
    const nlohmann::json& parent_json_object,
    const std::string_view key,
    SkillProjectileData* out_data) const
{
    static constexpr std::string_view method_name = "LoadSkillProjectileData";
    if (!EnsureContainsObjectField(method_name, parent_json_object, key))
    {
        return false;
    }

    return LoadSkillProjectileData(parent_json_object[key], out_data);
}

bool BaseDataLoader::LoadSkillProjectileData(const nlohmann::json& json_object, SkillProjectileData* out_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadSkillProjectileData";

    // Defaults
    out_data->size_units = 0;
    out_data->is_homing = true;
    out_data->speed_sub_units = 2000;
    out_data->is_blockable = false;
    out_data->apply_to_all = false;

    // Read values
    if (!GetIntValue(json_object, "SizeUnits", &out_data->size_units))
    {
        return false;
    }
    if (!GetBoolValue(json_object, JSONKeys::kIsHoming, &out_data->is_homing))
    {
        return false;
    }
    if (!GetIntValue(json_object, "SpeedSubUnits", &out_data->speed_sub_units))
    {
        return false;
    }
    if (!GetBoolValue(json_object, "IsBlockable", &out_data->is_blockable))
    {
        return false;
    }

    if (!GetBoolValue(json_object, JSONKeys::kApplyToAll, &out_data->apply_to_all))
    {
        return false;
    }
    GetBoolOptionalValue(json_object, JSONKeys::kContinueAfterTarget, &out_data->continue_after_target);

    if (out_data->is_homing && out_data->continue_after_target)
    {
        LogErr(
            "{} - \"{}\" and \"{}\" are incompatible. JSON: {}",
            method_name,
            JSONKeys::kIsHoming,
            JSONKeys::kContinueAfterTarget,
            json_object.dump(4));
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadSkillBeamData(
    const nlohmann::json& parent_json_object,
    const std::string_view key,
    SkillBeamData* out_data) const
{
    static constexpr std::string_view method_name = "LoadSkillBeamData";
    if (!EnsureContainsObjectField(method_name, parent_json_object, key))
    {
        return false;
    }

    return LoadSkillBeamData(parent_json_object[key], out_data);
}

bool BaseDataLoader::LoadSkillBeamData(const nlohmann::json& json_object, SkillBeamData* out_data) const
{
    // Defaults
    out_data->width_units = 75;
    out_data->frequency_ms = 500;

    if (!GetIntValue(json_object, "WidthUnits", &out_data->width_units))
    {
        return false;
    }
    if (!GetIntValue(json_object, "FrequencyMs", &out_data->frequency_ms))
    {
        return false;
    }
    if (!GetBoolValue(json_object, "ApplyOnce", &out_data->apply_once))
    {
        return false;
    }
    if (!GetBoolValue(json_object, "IsHoming", &out_data->is_homing))
    {
        return false;
    }
    if (!GetBoolValue(json_object, "IsBlockable", &out_data->is_blockable))
    {
        return false;
    }

    // Optional, defaults to None if missing
    static constexpr std::string_view key_block_allegiance = "BlockAllegiance";
    if (json_object.contains(key_block_allegiance))
    {
        if (!json_helper_.GetEnumValue(json_object, key_block_allegiance, &out_data->block_allegiance))
        {
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadEffectPackage(const nlohmann::json& parent_json_object, EffectPackage* out_effect_package)
    const
{
    static constexpr std::string_view key_attributes = "Attributes";
    if (!parent_json_object.contains(JSONKeys::kEffectPackage))
    {
        LogErr(
            "BaseDataLoader::LoadEffectPackageFromJSON - parent_json_object does not contain key = {}",
            JSONKeys::kEffectPackage);
        return false;
    }
    const nlohmann::json& json_object = parent_json_object[JSONKeys::kEffectPackage];
    if (!json_object.is_object())
    {
        LogErr(
            "BaseDataLoader::LoadEffectPackageFromJSON - json_object is not of type Object, key = {}",
            JSONKeys::kEffectPackage);
        return false;
    }

    // Read optional attributes
    if (json_object.contains(key_attributes))
    {
        const nlohmann::json& json_object_attributes = json_object[key_attributes];
        if (!json_object_attributes.is_object())
        {
            LogErr(
                "BaseDataLoader::LoadEffectPackageFromJSON - json_object_attributes is not of type Object, "
                "key = {}",
                key_attributes);
            return false;
        }

        if (!LoadEffectPackageAttributes(json_object_attributes, &out_effect_package->attributes))
        {
            return false;
        }
    }

    // Require effects
    if (!LoadEffectsArray(json_object, "Effects", {}, &out_effect_package->effects))
    {
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadAbilityTypesArray(
    const nlohmann::json& json_object,
    std::string_view key_abilities_array,
    std::vector<AbilityType>* out_ability_types) const
{
    return json_helper_.WalkArray(
        json_object,
        key_abilities_array,
        true,
        [&](const size_t index, const nlohmann::json& json_array_element) -> bool
        {
            AbilityType ability_type = AbilityType::kNone;
            if (!json_helper_.GetEnumValue(json_array_element, &ability_type))
            {
                LogErr("BaseDataLoader::LoadAbilityTypesArrayFromJSON - Failed to load effect with index = {}", index);
                return false;
            }

            out_ability_types->push_back(ability_type);
            return true;
        });
}

bool BaseDataLoader::LoadEffectsArray(
    const nlohmann::json& json_object,
    std::string_view key_effects_array,
    const EffectLoadingParameters& parameters,
    std::vector<EffectData>* out_effects) const
{
    return json_helper_.WalkArray(
        json_object,
        key_effects_array,
        true,
        [&](const size_t index, const nlohmann::json& json_array_element) -> bool
        {
            EffectData effect_data;
            if (!LoadEffect(json_array_element, parameters, &effect_data))
            {
                LogErr(
                    "BaseDataLoader::LoadEffectChildrenEffectsFromJSON - Failed to load effect with index = "
                    "{}",
                    index);
                return false;
            }

            out_effects->push_back(effect_data);
            return true;
        });
}

bool BaseDataLoader::LoadEffectPackagePropagationAttributes(
    const nlohmann::json& parent_json_object,
    EffectPackageAttributes* out_attributes) const
{
    static constexpr std::string_view key_propagation = "Propagation";
    if (!parent_json_object.contains(key_propagation))
    {
        return true;
    }

    auto& json_object = parent_json_object[key_propagation];

    // Require type
    static constexpr std::string_view key_propagation_type = "PropagationType";
    if (!json_helper_.GetEnumValue(json_object, key_propagation_type, &out_attributes->propagation.type))
    {
        return false;
    }

    // no need to read anything if propagation type is none
    if (out_attributes->propagation.type == EffectPropagationType::kNone)
    {
        LogErr(
            "LoadEffectPackagePropagationAttributesFromJSON: {} has to specify non-empty propagation type",
            key_propagation_type);
        return false;
    }

    // chain-specific parameters
    if (out_attributes->propagation.type == EffectPropagationType::kChain)
    {
        if (!GetIntValue(json_object, "ChainNumber", &out_attributes->propagation.chain_number))
        {
            return false;
        }
        if (!GetIntValue(json_object, "ChainDelayMs", &out_attributes->propagation.chain_delay_ms))
        {
            return false;
        }

        // Optional
        GetIntOptionalValue(
            json_object,
            "ChainBounceMaxDistanceUnits",
            &out_attributes->propagation.chain_bounce_max_distance_units);
        GetBoolOptionalValue(json_object, "OnlyNewTargets", &out_attributes->propagation.only_new_targets);
        GetBoolOptionalValue(json_object, "PrioritizeNewTargets", &out_attributes->propagation.prioritize_new_targets);
        GetIntOptionalValue(
            json_object,
            JSONKeys::kPreDeploymentDelayPercentage,
            &out_attributes->propagation.pre_deployment_delay_percentage);
        GetEnumOptionalValue(json_object, JSONKeys::kTargetingGroup, &out_attributes->propagation.targeting_group);

        if (out_attributes->propagation.only_new_targets && out_attributes->propagation.prioritize_new_targets)
        {
            LogWarn(
                "BaseDataLoader::LoadEffectFromJSON - OnlyNewTargets and PrioritizeNewTargets are true, "
                "BUT ONLY OnlyNewTargets will take effect");
        }
    }
    else if (out_attributes->propagation.type == EffectPropagationType::kSplash)
    {
        if (!GetIntValue(json_object, "SplashRadiusUnits", &out_attributes->propagation.splash_radius_units))
        {
            return false;
        }
    }

    if (json_object.contains(JSONKeys::kEffectPackage))
    {
        out_attributes->propagation.effect_package = std::make_shared<EffectPackage>();
        if (!LoadEffectPackage(json_object, out_attributes->propagation.effect_package.get()))
        {
            return false;
        }
    }

    // Optional propagation attributes
    GetBoolOptionalValue(
        json_object,
        "IgnoreFirstPropagationReceiver",
        &out_attributes->propagation.ignore_first_propagation_receiver);

    GetBoolOptionalValue(
        json_object,
        "AddOriginalEffectPackage",
        &out_attributes->propagation.add_original_effect_package);

    GetBoolOptionalValue(
        json_object,
        "SkipOriginalEffectPackage",
        &out_attributes->propagation.skip_original_effect_package);

    // Optional, defaults to Ground if missing
    static constexpr std::string_view key_deployment_guidance = "DeploymentGuidance";
    if (json_object.contains(key_deployment_guidance))
    {
        if (!LoadGuidancesSet(json_object, key_deployment_guidance, &out_attributes->propagation.deployment_guidance))
        {
            return false;
        }
    }

    if (json_object.contains(JSONKeys::kTargetingGuidance))
    {
        if (!LoadGuidancesSet(
                json_object,
                JSONKeys::kTargetingGuidance,
                &out_attributes->propagation.targeting_guidance))
        {
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadEffectPackageAttributes(
    const nlohmann::json& json_object,
    EffectPackageAttributes* out_optional_attributes) const
{
    // Skill optional attributes
    GetBoolOptionalValue(json_object, "ExcessVampToShield", &out_optional_attributes->excess_vamp_to_shield);
    GetBoolOptionalValue(json_object, "ExploitWeakness", &out_optional_attributes->exploit_weakness);
    GetBoolOptionalValue(json_object, "CanCrit", &out_optional_attributes->can_crit);
    GetBoolOptionalValue(json_object, "AlwaysCrit", &out_optional_attributes->always_crit);
    GetBoolOptionalValue(json_object, "RotateToTarget", &out_optional_attributes->rotate_to_target);
    GetBoolOptionalValue(json_object, "IsTrueshot", &out_optional_attributes->is_trueshot);
    GetBoolOptionalValue(json_object, "CumulativeDamage", &out_optional_attributes->cumulative_damage);
    GetBoolOptionalValue(json_object, "SplitDamage", &out_optional_attributes->split_damage);
    GetBoolOptionalValue(json_object, "UseHitChance", &out_optional_attributes->use_hit_chance);
    GetExprOptionalValue(json_object, "DamageBonus", &out_optional_attributes->damage_bonus);
    GetExprOptionalValue(json_object, "PhysicalDamageBonus", &out_optional_attributes->physical_damage_bonus);
    GetExprOptionalValue(json_object, "EnergyDamageBonus", &out_optional_attributes->energy_damage_bonus);
    GetExprOptionalValue(json_object, "PureDamageBonus", &out_optional_attributes->pure_damage_bonus);
    GetExprOptionalValue(json_object, "DamageAmplification", &out_optional_attributes->damage_amplification);
    GetExprOptionalValue(
        json_object,
        "PhysicalDamageAmplification",
        &out_optional_attributes->physical_damage_amplification);
    GetExprOptionalValue(
        json_object,
        "EnergyDamageAmplification",
        &out_optional_attributes->energy_damage_amplification);
    GetExprOptionalValue(json_object, "PureDamageAmplification", &out_optional_attributes->pure_damage_amplification);
    GetExprOptionalValue(json_object, "RefundHealth", &out_optional_attributes->refund_health);
    GetExprOptionalValue(json_object, "RefundEnergy", &out_optional_attributes->refund_energy);
    GetExprOptionalValue(json_object, "EnergyGainBonus", &out_optional_attributes->energy_gain_bonus);
    GetExprOptionalValue(json_object, "EnergyGainAmplification", &out_optional_attributes->energy_gain_amplification);
    GetExprOptionalValue(json_object, "EnergyBurnBonus", &out_optional_attributes->energy_burn_bonus);
    GetExprOptionalValue(json_object, "EnergyBurnAmplification", &out_optional_attributes->energy_burn_amplification);
    GetExprOptionalValue(json_object, "HealBonus", &out_optional_attributes->heal_bonus);
    GetExprOptionalValue(json_object, "HealAmplification", &out_optional_attributes->heal_amplification);
    GetExprOptionalValue(json_object, "PiercingPercentage", &out_optional_attributes->piercing_percentage);
    GetExprOptionalValue(
        json_object,
        "PhysicalPiercingPercentage",
        &out_optional_attributes->physical_piercing_percentage);
    GetExprOptionalValue(json_object, "EnergyPiercingPercentage", &out_optional_attributes->energy_piercing_percentage);
    GetExprOptionalValue(
        json_object,
        "CritReductionPiercingPercentage",
        &out_optional_attributes->crit_reduction_piercing_percentage);
    GetExprOptionalValue(json_object, "ShieldBonus", &out_optional_attributes->shield_bonus);
    GetExprOptionalValue(json_object, "VampiricPercentage", &out_optional_attributes->vampiric_percentage);
    GetIntOptionalValue(
        json_object,
        "ExcessVampToShieldDurationMs",
        &out_optional_attributes->excess_vamp_to_shield_duration_ms);
    GetExprOptionalValue(json_object, "ShieldAmplification", &out_optional_attributes->shield_amplification);

    if (!LoadEffectPackagePropagationAttributes(json_object, out_optional_attributes))
    {
        return false;
    }

    if (out_optional_attributes->cumulative_damage == true && out_optional_attributes->split_damage == true)
    {
        LogErr(
            "BaseDataLoader::LoadEffectPackageAttributesFromJSON - CumulativeDamage and SplitDamage cannot "
            "both be true");
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadEffectDataAttributes(
    const nlohmann::json& json_object,
    EffectDataAttributes* out_optional_attributes) const
{
    // Load optional attributes
    GetFixedPointIntOptionalValue(json_object, "ExcessHealToShield", &out_optional_attributes->excess_heal_to_shield);
    GetFixedPointIntOptionalValue(
        json_object,
        "MissingHealthPercentageToHealth",
        &out_optional_attributes->missing_health_percentage_to_health);
    GetFixedPointIntOptionalValue(json_object, "ToShieldPercentage", &out_optional_attributes->to_shield_percentage);
    GetIntOptionalValue(json_object, "ToShieldDurationMs", &out_optional_attributes->to_shield_duration_ms);
    GetFixedPointIntOptionalValue(
        json_object,
        "MaxHealthPercentageToHealth",
        &out_optional_attributes->max_health_percentage_to_health);
    GetBoolOptionalValue(json_object, "CleanseNegativeStates", &out_optional_attributes->cleanse_negative_states);
    GetBoolOptionalValue(json_object, "CleanseConditions", &out_optional_attributes->cleanse_conditions);
    GetBoolOptionalValue(json_object, "CleanseDOTs", &out_optional_attributes->cleanse_dots);
    GetBoolOptionalValue(json_object, "CleanseBOTs", &out_optional_attributes->cleanse_bots);
    GetBoolOptionalValue(json_object, "CleanseDebuffs", &out_optional_attributes->cleanse_debuffs);
    GetBoolOptionalValue(json_object, "ShieldBypass", &out_optional_attributes->shield_bypass);

    return true;
}

bool BaseDataLoader::LoadAbilityActivationTriggerData(
    const nlohmann::json& parent_json,
    const std::string_view key_activation_trigger,
    AbilityActivationTriggerData* out_activation_trigger_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadAbilityActivationTriggerData";

    if (!parent_json.contains(key_activation_trigger))
    {
        LogErr(
            "{} - parent json does not contain \"{}\" key. Parent JSON: {}",
            method_name,
            key_activation_trigger,
            parent_json.dump());
        return false;
    }

    return LoadAbilityActivationTriggerData(parent_json[key_activation_trigger], out_activation_trigger_data);
}

bool BaseDataLoader::LoadAbilityActivationTriggerData(
    const nlohmann::json& json_object,
    AbilityActivationTriggerData* out_trigger_attributes) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadAbilityActivationTriggerData";

    if (!json_object.is_object())
    {
        LogErr("{} - parameter is not a JSON OBJECT. JSON: {}", method_name, json_object.dump());
        return false;
    }

    if (!json_helper_.GetEnumValue(json_object, "TriggerType", &out_trigger_attributes->trigger_type))
    {
        return false;
    }

    bool requires_ability_types = false;

    bool ability_type_was_read = false;
    if (json_object.contains(JSONKeys::kAbilityTypes))
    {
        ability_type_was_read = true;
        if (!LoadEnumSet(
                json_object,
                JSONKeys::kAbilityTypes,
                JSONKeys::kAbilityTypes,
                &out_trigger_attributes->ability_types))
        {
            LogErr("{} - failed to read {}", method_name, JSONKeys::kAbilityTypes);
            return false;
        }
    }
    GetIntOptionalValue(json_object, "RangeUnits", &out_trigger_attributes->range_units);
    GetIntOptionalValue(json_object, "NotBeforeMs", &out_trigger_attributes->not_before_ms);
    GetIntOptionalValue(json_object, "NotAfterMs", &out_trigger_attributes->not_after_ms);
    GetBoolOptionalValue(json_object, "OnlyFocus", &out_trigger_attributes->only_focus);
    GetBoolOptionalValue(json_object, "OncePerSpawnedEntity", &out_trigger_attributes->once_per_spawned_entity);
    GetBoolOptionalValue(json_object, "OnlyFromParent", &out_trigger_attributes->only_from_parent);
    GetBoolOptionalValue(json_object, "RequiresHyperActive", &out_trigger_attributes->requires_hyper_active);
    GetIntOptionalValue(json_object, "MaxActivations", &out_trigger_attributes->max_activations);
    GetIntOptionalValue(json_object, "ActivationTimeLimitMs", &out_trigger_attributes->activation_time_limit_ms);
    GetBoolOptionalValue(json_object, "ImmediateActivationOnly", &out_trigger_attributes->immediate_activation_only);

    GetBoolOptionalValue(
        json_object,
        "ForceAddToQueueOnActivation",
        &out_trigger_attributes->force_add_to_queue_on_activation);
    GetEnumOptionalValue(json_object, "ContextRequirement", &out_trigger_attributes->context_requirement);

    static constexpr std::string_view key_activation_cooldown_ms = "ActivationCooldownMs";
    if (json_object.contains(key_activation_cooldown_ms))
    {
        if (!GetIntValue(json_object, key_activation_cooldown_ms, &out_trigger_attributes->activation_cooldown_ms))
        {
            return false;
        }
    }

    auto read_damage_types = [&]()
    {
        if (json_object.contains(JSONKeys::kDamageTypeList) &&
            !LoadEffectDamageTypesSet(json_object, JSONKeys::kDamageTypeList, &out_trigger_attributes->damage_types))
        {
            return false;
        }

        return true;
    };

    if (!LoadTriggerAllegianceFromJSON(json_object, out_trigger_attributes))
    {
        return false;
    }

    switch (out_trigger_attributes->trigger_type)
    {
    case ActivationTriggerType::kInRange:
        if (!GetIntValue(json_object, "ActivationRadiusUnits", &out_trigger_attributes->activation_radius_units))
        {
            return false;
        }
        if (!json_helper_.GetConditionTypes(
                json_object,
                "SenderConditions",
                &out_trigger_attributes->required_sender_conditions))
        {
            return false;
        }

        if (!json_helper_.GetConditionTypes(
                json_object,
                "ReceiverConditions",
                &out_trigger_attributes->required_receiver_conditions))
        {
            return false;
        }
        break;

    case ActivationTriggerType::kEveryXTime:
        if (!GetIntValue(json_object, "ActivateEveryTimeMs", &out_trigger_attributes->activate_every_time_ms))
        {
            return false;
        }
        break;
    case ActivationTriggerType::kHealthInRange:
    {
        int health_lower_limit_percentage = 0;
        if (!GetIntValue(json_object, "HealthLowerLimitPercentage", &health_lower_limit_percentage))
        {
            return false;
        }
        out_trigger_attributes->health_lower_limit_percentage = FixedPoint::FromInt(health_lower_limit_percentage);
        break;
    }

    case ActivationTriggerType::kOnActivateXAbilities:
        requires_ability_types = true;
        if (!GetIntValue(
                json_object,
                JSONKeys::kNumberOfAbilitiesActivated,
                &out_trigger_attributes->number_of_abilities_activated))
        {
            LogErr(
                "{}: {} trigger requires {} to be specified",
                method_name,
                out_trigger_attributes->trigger_type,
                JSONKeys::kNumberOfAbilitiesActivated);
            return false;
        }
        GetBoolOptionalValue(json_object, JSONKeys::kEveryX, &out_trigger_attributes->every_x);
        break;

    case ActivationTriggerType::kOnDeployXSkills:
        requires_ability_types = true;
        if (json_object.contains(JSONKeys::kNumberOfSkillsDeployed) &&
            !GetIntOptionalValue(
                json_object,
                JSONKeys::kNumberOfSkillsDeployed,
                &out_trigger_attributes->number_of_skills_deployed))
        {
            LogErr(
                "{}: {} trigger contains {} key but it is invalid.",
                method_name,
                out_trigger_attributes->trigger_type,
                JSONKeys::kNumberOfSkillsDeployed);
            return false;
        }
        break;

    case ActivationTriggerType::kOnDamage:
        GetFixedPointIntOptionalValue(json_object, "DamageThreshold", &out_trigger_attributes->damage_threshold);
        if (!read_damage_types())
        {
            return false;
        }
        break;

    case ActivationTriggerType::kOnReceiveXEffectPackages:
        requires_ability_types = true;
        GetIntOptionalValue(
            json_object,
            JSONKeys::kModuloValue,
            &out_trigger_attributes->number_of_effect_packages_received_modulo);
        GetEnumOptionalValue(json_object, JSONKeys::kComparisonType, &out_trigger_attributes->comparison_type);
        GetIntOptionalValue(
            json_object,
            JSONKeys::kCompareAgainst,
            &out_trigger_attributes->number_of_effect_packages_received);
        break;

    case ActivationTriggerType::kOnVanquish:
    {
        // Default: activate on every vanquish, by default Modulo operation
        out_trigger_attributes->every_x = true;
        out_trigger_attributes->trigger_value = 1;

        GetIntOptionalValue(json_object, JSONKeys::kTriggerValue, &out_trigger_attributes->trigger_value);

        // "EveryX" can be avoided in JSONs in all cases (it is true by default and it is false if "ComparisonType"
        // was specified) but lets make it optional in JSON if author wants to make it explicit
        const bool read_every_x_key =
            GetBoolOptionalValue(json_object, JSONKeys::kEveryX, &out_trigger_attributes->every_x);

        if (json_object.contains(JSONKeys::kComparisonType))
        {
            if (read_every_x_key)
            {
                LogErr(
                    "\"{}\" and \"{}\" are mutually exclusive keys for \"{}\" trigger.",
                    JSONKeys::kEveryX,
                    JSONKeys::kComparisonType,
                    out_trigger_attributes->trigger_type);
                return false;
            }

            out_trigger_attributes->every_x = false;

            std::string comparison_type_str = json_object[JSONKeys::kComparisonType];
            out_trigger_attributes->comparison_type = Enum::StringToComparisonType(comparison_type_str);
            if (out_trigger_attributes->comparison_type == ComparisonType::kNone)
            {
                LogErr("{} has invalid value \"{}\"", JSONKeys::kComparisonType, comparison_type_str);
                return false;
            }
        }
        break;
    }

    case ActivationTriggerType::kOnBattleStart:
    {
        // Battle start triggers should always added to queue
        out_trigger_attributes->force_add_to_queue_on_activation = true;

        break;
    }

    case ActivationTriggerType::kOnHit:
        requires_ability_types = true;
        break;

    default:
        break;
    }

    if (requires_ability_types && !ability_type_was_read)
    {
        LogErr(
            "{}: {} trigger requires {} key but it is invalid or not specified",
            method_name,
            out_trigger_attributes->trigger_type,
            JSONKeys::kAbilityTypes);
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadTriggerAllegianceFromJSON(
    const nlohmann::json& json_object,
    AbilityActivationTriggerData* out_trigger_attributes) const
{
    // Most trigger have optional field "Allegiance"

    auto read_allegiance = [&](std::string_view key, AllegianceType* out_allegiance)
    {
        if (!json_object.contains(key))
        {
            LogErr("{} trigger requires {} to be specified", out_trigger_attributes->trigger_type, key);
            return false;
        }

        return json_helper_.GetEnumValue(json_object, key, out_allegiance);
    };

    switch (out_trigger_attributes->trigger_type)
    {
    case ActivationTriggerType::kOnDodge:
    case ActivationTriggerType::kOnFaint:
    case ActivationTriggerType::kOnMiss:
    case ActivationTriggerType::kOnVanquish:
    {
        if (!read_allegiance(JSONKeys::kAllegiance, &out_trigger_attributes->sender_allegiance))
        {
            return false;
        }
    }
    break;

    case ActivationTriggerType::kOnHit:
    case ActivationTriggerType::kOnDamage:
    case ActivationTriggerType::kOnShieldHit:
    {
        if (!read_allegiance(JSONKeys::kSenderAllegiance, &out_trigger_attributes->sender_allegiance))
        {
            return false;
        }

        if (!read_allegiance(JSONKeys::kReceiverAllegiance, &out_trigger_attributes->receiver_allegiance))
        {
            return false;
        }
    }
    break;

    default:
        GetEnumOptionalValue(json_object, JSONKeys::kAllegiance, &out_trigger_attributes->sender_allegiance);
        break;
    }

    return true;
}

bool BaseDataLoader::LoadEffectTypeID(const nlohmann::json& json_object, EffectTypeID* out_effect_type_id) const
{
    // Require type
    if (!json_helper_.GetEnumValue(json_object, JSONKeys::kType, &out_effect_type_id->type))
    {
        return false;
    }

    if (out_effect_type_id->type == EffectType::kPositiveState)
    {
        if (!json_helper_.GetEnumValue(json_object, "PositiveState", &out_effect_type_id->positive_state))
        {
            return false;
        }
    }

    if (out_effect_type_id->type == EffectType::kNegativeState)
    {
        if (!json_helper_.GetEnumValue(json_object, "NegativeState", &out_effect_type_id->negative_state))
        {
            return false;
        }
    }

    if (out_effect_type_id->type == EffectType::kPlaneChange)
    {
        if (!json_helper_.GetEnumValue(json_object, "PlaneChange", &out_effect_type_id->plane_change))
        {
            return false;
        }
    }

    if (out_effect_type_id->type == EffectType::kCondition)
    {
        if (!json_helper_.GetEnumValue(json_object, "ConditionType", &out_effect_type_id->condition_type))
        {
            return false;
        }
    }

    if (out_effect_type_id->type == EffectType::kDisplacement)
    {
        if (!json_helper_.GetEnumValue(json_object, "DisplacementType", &out_effect_type_id->displacement_type))
        {
            return false;
        }
    }

    if (out_effect_type_id->type == EffectType::kSpawnMark)
    {
        static constexpr std::string_view key_mark_type = "MarkType";
        if (!json_helper_.GetEnumValue(json_object, key_mark_type, &out_effect_type_id->mark_type))
        {
            LogErr("{} is required key for {} effect", key_mark_type, out_effect_type_id->type);
            return false;
        }
    }

    if (out_effect_type_id->UsesDamageType())
    {
        if (!json_helper_.GetEnumValue(json_object, "DamageType", &out_effect_type_id->damage_type))
        {
            return false;
        }
    }

    if (out_effect_type_id->UsesStatType())
    {
        if (!json_helper_.GetEnumValue(json_object, JSONKeys::kStat, &out_effect_type_id->stat_type))
        {
            return false;
        }
    }

    if (out_effect_type_id->UsesEffectHealType())
    {
        if (!json_helper_.GetEnumValue(json_object, "HealType", &out_effect_type_id->heal_type))
        {
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadValidationList(
    const nlohmann::json& json_object,
    std::string_view key_validation_list,
    EffectValidations* out_effect_validations) const
{
    return json_helper_.WalkArray(
        json_object,
        key_validation_list,
        true,
        [&](const size_t index, const nlohmann::json& json_array_element) -> bool
        {
            ValidationType validation = ValidationType::kNone;
            if (!json_helper_.GetEnumValue(json_array_element, JSONKeys::kType, &validation))
            {
                return false;
            }

            switch (validation)
            {
            case ValidationType::kDistanceCheck:
            {
                ValidationDistanceCheck distance_check;
                distance_check.validation_type = validation;
                if (!LoadValidationDistanceCheck(json_array_element, &distance_check))
                {
                    LogErr(
                        "BaseDataLoader::LoadValidationListFromJSON - Failed to load effect with index = "
                        "{}",
                        index);
                    return false;
                }

                out_effect_validations->distance_checks.push_back(distance_check);
                break;
            }
            case ValidationType::kExpression:
            {
                ValidationEffectExpressionComparison effect_validation;
                effect_validation.validation_type = validation;
                if (!LoadValidationComparisonExpression(json_array_element, &effect_validation))
                {
                    LogErr(
                        "BaseDataLoader::LoadValidationListFromJSON - Failed to load effect with index = "
                        "{}",
                        index);
                    return false;
                }

                out_effect_validations->expression_comparisons.push_back(effect_validation);
                break;
            }
            default:
                return false;
            }

            return true;
        });
}

bool BaseDataLoader::LoadValidationDistanceCheck(
    const nlohmann::json& json_object,
    ValidationDistanceCheck* out_distance_check) const
{
    if (!json_helper_.GetEnumValue(json_object, JSONKeys::kComparisonType, &out_distance_check->comparison_type))
    {
        return false;
    }
    if (!json_helper_.GetEnumValue(json_object, "ComparisonEntities", &out_distance_check->allegiance))
    {
        return false;
    }
    if (!json_helper_.GetIntValue(json_object, "ComparisonEntitiesCount", &out_distance_check->entities_count))
    {
        return false;
    }
    if (!json_helper_.GetIntValue(json_object, "DistanceUnits", &out_distance_check->distance_units))
    {
        return false;
    }
    if (!json_helper_.GetEnumValue(json_object, "FirstEntity", &out_distance_check->first_entity))
    {
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadValidationComparisonExpression(
    const nlohmann::json& json_object,
    ValidationEffectExpressionComparison* out_expression_comparison) const
{
    if (!json_helper_.GetEnumValue(json_object, JSONKeys::kComparisonType, &out_expression_comparison->comparison_type))
    {
        return false;
    }
    if (!LoadEffectExpression(json_object, "Left", &out_expression_comparison->left))
    {
        return false;
    }
    if (!LoadEffectExpression(json_object, "Right", &out_expression_comparison->right))
    {
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadEffect(
    const nlohmann::json& json_object,
    const EffectLoadingParameters& parameters,
    EffectData* out_effect) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadEffect";

    // Load the type id
    if (!LoadEffectTypeID(json_object, &out_effect->type_id))
    {
        return false;
    }

    if (parameters.id_required)
    {
        if (!GetStringValue(json_object, JSONKeys::kID, &out_effect->id))
        {
            return false;
        }
    }
    else
    {
        GetStringOptionalValue(json_object, JSONKeys::kID, &out_effect->id);
    }

    // Read additional fields for displacement type
    if (out_effect->type_id.type == EffectType::kDisplacement)
    {
        // Knock back uses distance
        if (out_effect->type_id.displacement_type == EffectDisplacementType::kKnockBack)
        {
            if (!GetIntValue(json_object, "DisplacementDistanceSubUnits", &out_effect->displacement_distance_sub_units))
            {
                return false;
            }
        }

        // Read optional on finished skill
        SkillData on_finished_skill;
        if (json_object.contains(JSONKeys::kOnFinished))
        {
            if (json_helper_.SetSkillData(
                    json_object,
                    AbilityType::kAttack,
                    JSONKeys::kOnFinished,
                    std::bind(
                        &Self::LoadSkill,
                        this,
                        std::placeholders::_1,
                        std::placeholders::_2,
                        std::placeholders::_3),
                    &on_finished_skill))
            {
                if (on_finished_skill.targeting.type != SkillTargetingType::kSelf)
                {
                    LogErr(
                        "{} - Displacement OnFinished skill should have "
                        "only self targeting, because it's predefined by parent ability",
                        method_name);
                    return false;
                }

                out_effect->event_skills[EventType::kDisplacementStopped] = on_finished_skill.CreateDeepCopy();
            }
            else
            {
                return false;
            }
        }
    }

    // Blink effect additional fields
    if (out_effect->type_id.type == EffectType::kBlink)
    {
        // Require target
        if (!json_helper_.GetEnumValue(json_object, "BlinkTarget", &out_effect->blink_target))
        {
            return false;
        }

        // Require delay
        if (!json_helper_.GetIntValue(json_object, "BlinkDelayMs", &out_effect->blink_delay_ms))
        {
            return false;
        }

        // Optional option for reserving previous position after blink
        GetBoolOptionalValue(json_object, "BlinkReservePreviousPosition", &out_effect->blink_reserve_previous_position);
    }

    // positive state effect additional fields
    if (out_effect->type_id.type == EffectType::kPositiveState)
    {
        if (out_effect->type_id.positive_state == EffectPositiveState::kImmune)
        {
            static constexpr std::string_view key_immuned_effects = "AttachableEffectTypeList";
            if (json_object.contains(key_immuned_effects))
            {
                if (!LoadEffectTypeIDArray(json_object, key_immuned_effects, &out_effect->immuned_effect_types))
                {
                    return false;
                }
            }
        }
    }

    if (out_effect->type_id.type == EffectType::kStatOverride)
    {
        EffectExpression expression;
        int base_value = 0;
        if (!GetIntValue(json_object, JSONKeys::kValue, &base_value))
        {
            LogErr("\"{}\" effect type requires \"{}\" to be specified.", out_effect->type_id.type, JSONKeys::kValue);
            return false;
        }

        expression.base_value.value = FixedPoint::FromInt(base_value);
        out_effect->SetExpression(expression);
    }

    // expression
    if (out_effect->UsesExpression())
    {
        if (!json_object.contains(JSONKeys::kExpression))
        {
            LogErr("{} - Missing EXPECTED Expression Field", method_name);
            return false;
        }

        EffectExpression expression;
        if (!LoadEffectExpression(json_object, JSONKeys::kExpression, &expression))
        {
            return false;
        }

        out_effect->SetExpression(expression);
    }

    // radius_units
    if (parameters.for_aura)
    {
        // Require it
        if (!GetIntValue(json_object, JSONKeys::kRadiusUnits, &out_effect->radius_units))
        {
            return false;
        }
    }

    if (json_object.contains(JSONKeys::kCanCleanse))
    {
        if (!GetBoolValue(json_object, JSONKeys::kCanCleanse, &out_effect->can_cleanse))
        {
            return false;
        }
    }

    // duration_time_ms
    if (out_effect->UsesDurationTime())
    {
        // By default all buffs are static, i.e. they capture the value on application and never update it
        if (out_effect->type_id.type == EffectType::kBuff || out_effect->type_id.type == EffectType::kDebuff)
        {
            out_effect->lifetime.frequency_time_ms = 0;
        }

        if (!LoadEffectLifetime(json_object, parameters, &out_effect->lifetime))
        {
            return false;
        }
    }

    // Load optional attributes
    LoadEffectDataAttributes(json_object, &out_effect->attributes);

    if (json_object.contains(JSONKeys::kValidationList))
    {
        LoadValidationList(json_object, JSONKeys::kValidationList, &out_effect->validations);
    }

    // event_skills
    if (out_effect->type_id.type == EffectType::kSpawnShield)
    {
        SkillData on_hit_skill;
        SkillData on_expire_skill;
        SkillData on_destroy_skill;

        if (json_object.contains(JSONKeys::kOnWasHit))
        {
            if (json_helper_.SetSkillData(
                    json_object,
                    AbilityType::kAttack,
                    JSONKeys::kOnWasHit,
                    std::bind(
                        &Self::LoadSkill,
                        this,
                        std::placeholders::_1,
                        std::placeholders::_2,
                        std::placeholders::_3),
                    &on_hit_skill))
            {
                out_effect->event_skills[EventType::kShieldWasHit] = on_hit_skill.CreateDeepCopy();
            }
            else
            {
                return false;
            }
        }

        if (json_object.contains(JSONKeys::kOnExpire))
        {
            if (json_helper_.SetSkillData(
                    json_object,
                    AbilityType::kAttack,
                    JSONKeys::kOnExpire,
                    std::bind(
                        &Self::LoadSkill,
                        this,
                        std::placeholders::_1,
                        std::placeholders::_2,
                        std::placeholders::_3),
                    &on_expire_skill))
            {
                out_effect->event_skills[EventType::kShieldExpired] = on_expire_skill.CreateDeepCopy();
            }
            else
            {
                return false;
            }
        }

        if (json_object.contains(JSONKeys::kOnDestroy))
        {
            if (json_helper_.SetSkillData(
                    json_object,
                    AbilityType::kAttack,
                    JSONKeys::kOnDestroy,
                    std::bind(
                        &Self::LoadSkill,
                        this,
                        std::placeholders::_1,
                        std::placeholders::_2,
                        std::placeholders::_3),
                    &on_destroy_skill))
            {
                out_effect->event_skills[EventType::kShieldPendingDestroyed] = on_destroy_skill.CreateDeepCopy();
            }
            else
            {
                return false;
            }
        }

        if (json_object.contains(JSONKeys::kAttachedEffects))
        {
            if (!LoadEffectsArray(json_object, JSONKeys::kAttachedEffects, {}, &out_effect->attached_effects))
            {
                return false;
            }
        }
    }

    // Load attached entity effects if any - they are optional for shields
    if (out_effect->type_id.type == EffectType::kAura)
    {
        if (!LoadEffectsArray(
                json_object,
                JSONKeys::kAttachedEffects,
                {.for_aura = true},
                &out_effect->attached_effects))
        {
            return false;
        }

        if (out_effect->attached_effects.empty())
        {
            LogErr("{} - Auras must contain at least one attached effect.", method_name);
            return false;
        }

        bool effects_are_valid = true;
        for (const EffectData& effect : out_effect->attached_effects)
        {
            if (!effect.type_id.IsValidForAura())
            {
                LogErr("{} - {} effect cannot be used in aura", method_name, effect.type_id);
                effects_are_valid = false;
            }
        }

        if (!effects_are_valid)
        {
            return false;
        }
    }

    // Spawn Mark effect
    if (out_effect->type_id.type == EffectType::kSpawnMark)
    {
        AbilitiesData attached_abilities{};

        SourceContextData context{};
        context.combat_unit_ability_type = AbilityType::kInnate;

        // Load attached abilities if there any
        if (!LoadAbilitiesData(
                json_object,
                AbilityType::kInnate,
                JSONKeys::kAttachedAbilities,
                context,
                &attached_abilities))
        {
            return false;
        }

        // Assign attached abilities to mark abilities
        out_effect->attached_abilities = attached_abilities.abilities;
    }

    GetBoolOptionalValue(
        json_object,
        JSONKeys::kShouldDestroyAttachedEntityOnSenderDeath,
        &out_effect->should_destroy_attached_entity_on_sender_death);

    // Empower
    if (out_effect->type_id.type == EffectType::kEmpower || out_effect->type_id.type == EffectType::kDisempower)
    {
        if (!LoadEffectDataForEmpower(json_object, out_effect))
        {
            return false;
        }
    }

    // Execute
    if (out_effect->type_id.type == EffectType::kExecute)
    {
        if (!LoadAbilityTypesArray(json_object, JSONKeys::kAbilityTypes, &out_effect->ability_types))
        {
            return false;
        }
    }

    // Positive State
    if (out_effect->type_id.positive_state == EffectPositiveState::kEffectPackageBlock)
    {
        // Load optional ability types
        if (json_object.contains(JSONKeys::kAbilityTypes))
        {
            if (!LoadAbilityTypesArray(json_object, JSONKeys::kAbilityTypes, &out_effect->ability_types))
            {
                return false;
            }
        }

        if (!GetIntValue(json_object, JSONKeys::kBlocksUntilExpiry, &out_effect->lifetime.blocks_until_expiry))
        {
            return false;
        }

        if (out_effect->lifetime.is_consumable)
        {
            LogErr("{} - effect package block cannot be consumable", method_name);
            return false;
        }
    }

    // Condition effect type
    if (out_effect->type_id.type == EffectType::kCondition)
    {
        if (!LoadEffectDataForCondition(json_object, out_effect))
        {
            LogErr("{} - failed to load condition effect data", method_name);
            return false;
        }
    }

    // conditions
    if (json_object.contains(JSONKeys::kConditions))
    {
        if (!json_helper_.GetConditionTypes(json_object, JSONKeys::kConditions, &out_effect->required_conditions))
        {
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadEffectDataForEmpower(const nlohmann::json& json_object, EffectData* out_effect) const
{
    const bool is_empower = out_effect->type_id.type == EffectType::kEmpower;

    //
    // Shared between empower and disempower
    //
    if (!json_helper_.GetEnumValue(json_object, JSONKeys::kActivatedBy, &out_effect->lifetime.activated_by))
    {
        return false;
    }

    GetBoolOptionalValue(json_object, "ActivateOnCritical", &out_effect->lifetime.activate_on_critical);

    static constexpr std::string_view key_attached_effect_package_attributes = "AttachedEffectPackageAttributes";
    if (json_object.contains(key_attached_effect_package_attributes))
    {
        if (!LoadEffectPackageAttributes(
                json_object[key_attached_effect_package_attributes],
                &out_effect->attached_effect_package_attributes))
        {
            LogErr(
                "BaseDataLoader::LoadEffectExpression - Failed to extract key value = `{}` for "
                "Empower",
                key_attached_effect_package_attributes);
            return false;
        }
    }

    // Everything below this is for empower only
    if (!is_empower)
    {
        return true;
    }

    GetBoolOptionalValue(json_object, "IsUsedAsGlobalEffectAttribute", &out_effect->is_used_as_global_effect_attribute);

    if (out_effect->is_used_as_global_effect_attribute)
    {
        // Used a global effect attribute, to which effect type should this be applied to
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44205000/Empower#Adding-an-Effect-Attribute
        static constexpr std::string_view key_for_effect_type_id = "ForEffectTypeID";
        if (json_object.contains(key_for_effect_type_id))
        {
            // Load the type id
            if (!LoadEffectTypeID(json_object[key_for_effect_type_id], &out_effect->empower_for_effect_type_id))
            {
                return false;
            }
        }
        else
        {
            LogErr(
                "BaseDataLoader::LoadEffectExpression - Failed to extract key value = `{}` for "
                "Empower",
                key_for_effect_type_id);

            return false;
        }
    }

    // Optional
    if (json_object.contains(JSONKeys::kAttachedEffects))
    {
        if (!LoadEffectsArray(json_object, JSONKeys::kAttachedEffects, {}, &out_effect->attached_effects))
        {
            LogErr(
                "BaseDataLoader::LoadEffectExpression - Failed to extract key = `{}` for Empower",
                JSONKeys::kAttachedEffects);
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadEffectDataForCondition(const nlohmann::json& json_object, EffectData* out_effect) const
{
    static constexpr std::string_view key_condition_stacks = "Stacks";

    // Optional stacks expression
    EffectExpression expression;
    if (json_object.contains(key_condition_stacks))
    {
        if (!LoadEffectExpression(json_object[key_condition_stacks], &expression))
        {
            return false;
        }

        out_effect->SetStacksIncrement(expression);
    }

    return true;
}

bool BaseDataLoader::LoadEffectLifetime(
    const nlohmann::json& json_object,
    const EffectLoadingParameters& parameters,
    EffectLifetime* out_effect_lifetime) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadEffectLifetime";
    if (json_object.contains(JSONKeys::kIsConsumable))
    {
        // is_consumable & activations_until_expiry
        if (!GetBoolValue(json_object, JSONKeys::kIsConsumable, &out_effect_lifetime->is_consumable))
        {
            return false;
        }
    }

    if (json_object.contains(JSONKeys::kDeactivateIfValidationListNotValid))
    {
        if (!GetBoolValue(
                json_object,
                JSONKeys::kDeactivateIfValidationListNotValid,
                &out_effect_lifetime->deactivate_if_validation_list_not_valid))
        {
            return false;
        }
    }

    if (json_object.contains(JSONKeys::kMaxStacks))
    {
        if (!GetIntValue(json_object, JSONKeys::kMaxStacks, &out_effect_lifetime->max_stacks))
        {
            return false;
        }
    }

    std::optional<EffectOverlapProcessType> loaded_overlap_process_type;
    if (json_object.contains(JSONKeys::kOverlapProcessType))
    {
        if (!json_helper_
                 .GetEnumValue(json_object, JSONKeys::kOverlapProcessType, &out_effect_lifetime->overlap_process_type))
        {
            return false;
        }
        loaded_overlap_process_type = out_effect_lifetime->overlap_process_type;
    }

    if (parameters.for_aura)
    {
        constexpr auto kOverlapProcessTypeForAuras = EffectOverlapProcessType::kSum;
        if (loaded_overlap_process_type && loaded_overlap_process_type.value() != kOverlapProcessTypeForAuras)
        {
            LogErr(
                "{} - Effects in aura can have only {} value in {} property but {} specified. Set {} there or "
                "do not specify this property at all (recommended)",
                method_name,
                kOverlapProcessTypeForAuras,
                JSONKeys::kOverlapProcessType,
                *loaded_overlap_process_type,
                kOverlapProcessTypeForAuras);
            return false;
        }

        out_effect_lifetime->overlap_process_type = kOverlapProcessTypeForAuras;
    }

    if (out_effect_lifetime->is_consumable)
    {
        if (!GetIntValue(
                json_object,
                JSONKeys::kActivationsUntilExpiry,
                &out_effect_lifetime->activations_until_expiry))
        {
            return false;
        }
        if (!json_helper_.GetEnumValue(json_object, JSONKeys::kActivatedBy, &out_effect_lifetime->activated_by))
        {
            return false;
        }

        GetIntOptionalValue(
            json_object,
            JSONKeys::kConsumableActivationFrequency,
            &out_effect_lifetime->consumable_activation_frequency);
    }
    else
    {
        // duration_time_ms
        if (!GetIntValue(json_object, JSONKeys::kDurationMs, &out_effect_lifetime->duration_time_ms))
        {
            return false;
        }

        // frequency_time_ms
        if (json_object.contains(JSONKeys::kFrequencyMs))
        {
            GetIntValue(json_object, JSONKeys::kFrequencyMs, &out_effect_lifetime->frequency_time_ms);

            if (out_effect_lifetime->frequency_time_ms < 0)
            {
                LogErr("{} - FrequencyMs could not be negative", method_name);
                return false;
            }
        }
    }

    return true;
}

bool BaseDataLoader::LoadEffectExpression(
    const nlohmann::json& json_parent_object,
    const std::string_view key,
    EffectExpression* out_effect_expression) const
{
    if (!json_parent_object.contains(key))
    {
        LogErr("BaseDataLoader::LoadEffectExpression - Can't get Expression for key = {}", key);
        return false;
    }

    const auto& expression_json_value = json_parent_object[key];
    return LoadEffectExpression(expression_json_value, out_effect_expression);
}

bool BaseDataLoader::GetVariableValue(const std::string_view raw_variable_name, int* out_value) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::GetVariableValue";

    // Check if correct variable name
    if (raw_variable_name.size() < 3)
    {
        LogErr(
            "{} - Failed to extract variable, as variable_name = \"{}\" is not in a valid format",
            method_name,
            raw_variable_name);
        return false;
    }

    // Variables are defined in the format: {variable_name}
    // Trim {} from the variable name
    // Note: variable_name could also be string_view but we need C++20 for that
    // (Heterogeneous lookup for unordered containers)
    const auto variable_name = std::string(raw_variable_name.substr(1, raw_variable_name.size() - 2));

    // Check if variable name exists
    auto it = variables_.find(variable_name);
    if (it == variables_.end())
    {
        LogErr("{} - Variable Name = \"{}\" is not defined", method_name, variable_name);
        return false;
    }

    *out_value = it->second;
    return true;
}

bool BaseDataLoader::LoadEffectExpression(
    const nlohmann::json& expression_json_value,
    EffectExpression* out_effect_expression) const
{
    static constexpr std::string_view key_source_type = "StatSource";
    static constexpr std::string_view key_is_used_as_percentage = "IsUsedAsPercentage";

    // Flat value?
    if (expression_json_value.is_number_integer())
    {
        int expression_int_value = 0;
        if (!json_helper_.GetIntValue(expression_json_value, &expression_int_value))
        {
            LogErr(
                "BaseDataLoader::LoadEffectExpression - failed to parse expression value as "
                "integer");
            return false;
        }
        *out_effect_expression = EffectExpression::FromValue(FixedPoint::FromInt(expression_int_value));
        return true;
    }

    if (!variables_.empty())
    {
        // Check if correct variable name
        std::string raw_variable_name;
        if (GetRawVariableName(expression_json_value, &raw_variable_name))
        {
            int variable_value = 0;
            if (!GetVariableValue(raw_variable_name, &variable_value))
            {
                return false;
            }

            // Flat value
            *out_effect_expression = EffectExpression::FromValue(FixedPoint::FromInt(variable_value));

            return true;
        }
    }

    // Must be an object type
    if (!expression_json_value.is_object())
    {
        LogErr("BaseDataLoader::LoadEffectExpression - Expression is not of type Object");
        return false;
    }

    // Is this used as a percentage
    bool is_used_as_percentage = false;
    if (expression_json_value.contains(key_is_used_as_percentage))
    {
        GetBoolValue(expression_json_value, key_is_used_as_percentage, &is_used_as_percentage);
        out_effect_expression->is_used_as_percentage = is_used_as_percentage;
    }
    const auto set_output = [is_used_as_percentage, out_effect_expression](const EffectExpression& expression)
    {
        *out_effect_expression = expression;
        out_effect_expression->is_used_as_percentage = is_used_as_percentage;
    };

    // Is Flat value? But expressed as JSON Object
    if (expression_json_value.contains(JSONKeys::kValue))
    {
        int value = 0;
        if (!GetIntValue(expression_json_value, JSONKeys::kValue, &value))
        {
            LogErr("BaseDataLoader::LoadEffectExpression - Can't parse Value Expression");
            return false;
        }

        set_output(EffectExpression::FromValue(FixedPoint::FromInt(value)));
        return true;
    }

    // Is a SynergyCount
    if (json_helper_.HasCombatSynergyKey(expression_json_value))
    {
        ExpressionDataSourceType source_type = ExpressionDataSourceType::kSender;
        if (expression_json_value.contains(key_source_type) &&
            !json_helper_.GetEnumValue(expression_json_value, key_source_type, &source_type))
        {
            LogErr("BaseDataLoader::LoadEffectExpression - Failed to extract Key = `{}`", key_source_type);
            return false;
        }

        CombatSynergyBonus combat_synergy;
        if (json_helper_.GetCombatSynergy(expression_json_value, "", &combat_synergy))
        {
            set_output(EffectExpression::FromSynergyCount(combat_synergy, source_type));
            return true;
        }

        LogErr("BaseDataLoader::LoadEffectExpression - Failed to extract combat synergies");
        return false;
    }

    // Does not have Operation, simple base type
    static constexpr std::string_view key_operation = "Operation";
    if (!expression_json_value.contains(key_operation))
    {
        StatType stat = StatType::kNone;
        ExpressionDataSourceType source_type = ExpressionDataSourceType::kSender;

        // Any percentage of stat? only used if Percentage field is set
        int percentage = 0;

        const bool is_percentage = expression_json_value.contains(JSONKeys::kPercentage);
        if (is_percentage)
        {
            if (!GetIntValue(expression_json_value, JSONKeys::kPercentage, &percentage))
            {
                return false;
            }
        }

        if (!json_helper_.GetEnumValue(expression_json_value, key_source_type, &source_type))
        {
            return false;
        }

        bool is_metered_stat_percentage = false;
        {
            std::string stat_str;
            if (!json_helper_.GetStringValue(expression_json_value, JSONKeys::kStat, &stat_str))
            {
                return false;
            }

            // Parse stat parameters
            const StatParameters stat_params = ParseStatParameters(stat_str);
            is_metered_stat_percentage = stat_params.is_metered_stat_percentage;

            if (stat_params.is_custom_evaluation_function)
            {
                if (stat_params.stat_str == "ShieldAmount")
                {
                    set_output(EffectExpression::FromCustomEvaluationFunction(
                        EffectExpressionCustomFunctions::GetShieldAmount,
                        source_type));
                    return true;
                }

                LogErr("Unknown virtual stat \"{}\"", stat_params.stat_str);
                return false;
            }

            stat = Enum::StringToStatType(stat_params.stat_str);
            if (stat == StatType::kNone)
            {
                LogErr(
                    "BaseDataLoader::LoadEffectExpression - Failed to parse \"{}\" as stat type.",
                    stat_params.stat_str);
                return false;
            }
        }

        // Get the optional stat group if any
        // For buffs/debuffs the default evaluation type is base. For everything else it is live.
        StatEvaluationType stat_evaluation_type = StatEvaluationType::kLive;
        if (expression_json_value.contains(JSONKeys::kStatEvaluationType))
        {
            if (!json_helper_.GetEnumValue(expression_json_value, JSONKeys::kStatEvaluationType, &stat_evaluation_type))
            {
                return false;
            }
        }

        // Metered stat found
        if (is_metered_stat_percentage)
        {
            set_output(EffectExpression::FromMeteredStatPercentage(stat, stat_evaluation_type, source_type));
            return true;
        }

        // Percentage of stat
        if (is_percentage)
        {
            set_output(EffectExpression::FromStatPercentage(
                FixedPoint::FromInt(percentage),
                stat,
                stat_evaluation_type,
                source_type));
            return true;
        }

        // Just a stat
        set_output(EffectExpression::FromStat(stat, stat_evaluation_type, source_type));
        return true;
    }

    // Read Operation type
    if (!json_helper_.GetEnumValue(expression_json_value, key_operation, &out_effect_expression->operation_type))
    {
        return false;
    }

    // Read Operands
    const bool can_parse_operands = json_helper_.WalkArray(
        expression_json_value,
        "Operands",
        true,
        [&](const size_t index, const nlohmann::json& json_array_element) -> bool
        {
            EffectExpression operand_expression;
            if (!LoadEffectExpression(json_array_element, &operand_expression))
            {
                LogErr(
                    "BaseDataLoader::LoadEffectExpression - Failed to extract Operand Expression "
                    "at "
                    "index = {}",
                    index);
                return false;
            }

            out_effect_expression->operands.emplace_back(operand_expression);
            return true;
        });
    if (!can_parse_operands)
    {
        LogErr("BaseDataLoader::LoadEffectExpression - Failed to extract Operands Array");
        return false;
    }

    return true;
}

BaseDataLoader::StatParameters BaseDataLoader::ParseStatParameters(const std::string_view string)
{
    StatParameters params{};
    std::string_view& str_view = params.stat_str;
    str_view = string;

    // If string starts with $, it is a cusom evaluation function
    // set appropriate flag in params and remove that symbol from string view
    if (String::StartsWith(str_view, "$"))
    {
        params.is_custom_evaluation_function = true;
        str_view = str_view.substr(1);
    }

    // If string ends with %, it is a metered stat percentage
    // set appropriate flag in params and remove that symbol from string view
    if (String::EndsWith(str_view, "%"))
    {
        params.is_metered_stat_percentage = true;
        str_view = str_view.substr(0, str_view.size() - 1);
    }

    return params;
}

bool BaseDataLoader::LoadHexGridPosition(const nlohmann::json& json_object, HexGridPosition* out_position) const
{
    return GetIntValue(json_object, "Q", &out_position->q) && GetIntValue(json_object, "R", &out_position->r);
}

bool BaseDataLoader::LoadHexGridPosition(
    const nlohmann::json& json_object,
    const std::string_view key,
    HexGridPosition* out_position) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadHexGridPosition";
    return EnsureContainsObjectField(method_name, json_object, key) &&
           LoadHexGridPosition(json_object[key], out_position);
}

}  // namespace simulation
