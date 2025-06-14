#pragma once

#include <unordered_map>

#include "data/encounter_mod_instance_data.h"
#include "data/hyper_data.h"
#include "nlohmann/json.hpp"
#include "utility/json_helper.h"
#include "utility/logger.h"

namespace simulation
{

class CombatWeaponAmplifierInstanceData;
class CombatWeaponBaseInstanceData;
class BattleConfig;
class AugmentsConfig;
class ConsumablesConfig;
class LevelingConfig;
class OverloadConfig;
class HyperConfig;
class EncounterModTypeID;
class EncounterModData;
class ConsumableData;
class AugmentTypeID;
class AugmentData;
class DroneAugmentTypeID;
class DroneAugmentData;
class CombatUnitWeaponData;
class CombatUnitSuitData;
class CombatUnitTypeData;
class AugmentInstanceData;
class CombatWeaponTypeID;
class CombatWeaponInstanceData;
class CombatSuitTypeID;
class CombatSuitInstanceData;
class ConsumableTypeID;
class ConsumableInstanceData;
class ConsumableData;
class SynergyData;
class WorldEffectsConfig;
class WorldEffectConditionConfig;

/* -------------------------------------------------------------------------------------------------------
 * BaseDataLoader
 *
 * Base which contains utitlities to load game data from JSON
 * --------------------------------------------------------------------------------------------------------
 */
class BaseDataLoader
{
public:
    using Self = BaseDataLoader;

    explicit BaseDataLoader(std::shared_ptr<Logger> logger) : json_helper_(std::move(logger)) {}

    ///
    /// Augment
    ///

    // Loads augment type id from JSON parent object by key
    bool LoadAugmentTypeID(
        const nlohmann::json& json_parent,
        const std::string_view key,
        AugmentTypeID* out_augment_type_id) const;

    bool LoadAugmentsConfig(const nlohmann::json& json_object, AugmentsConfig* out_config) const;
    bool LoadAugmentsConfig(const nlohmann::json& json_parent, const std::string_view key, AugmentsConfig* out_config)
        const;

    // Loads augment type id from JSON object
    bool LoadAugmentTypeID(const nlohmann::json& json_object, AugmentTypeID* out_augment_type_id) const;

    // Loads array of augment instances from JSON parent object by key
    bool LoadAugmentInstancesData(
        const nlohmann::json& json_parent,
        const std::string_view key_array,
        std::vector<AugmentInstanceData>* out_augments_instance_data) const;

    // Loads augment instance from JSON object
    bool LoadAugmentInstanceData(const nlohmann::json& json_object, AugmentInstanceData* out_augment_instance_data)
        const;

    // Loads augment data from JSON object
    bool LoadAugmentData(const nlohmann::json& json_object, AugmentData* out_augment_data) const;

    ///
    /// Drone Augment
    ///

    // Loads drone augment type id from JSON parent object by key
    bool LoadDroneAugmentTypeID(
        const nlohmann::json& json_parent,
        const std::string_view key,
        DroneAugmentTypeID* out_drone_augment_type_id) const;

    // Loads drone augment type id from JSON object
    bool LoadDroneAugmentTypeID(const nlohmann::json& json_object, DroneAugmentTypeID* out_drone_augment_type_id) const;

    // Loads drone augment data from JSON object
    bool LoadDroneAugmentData(const nlohmann::json& json_object, DroneAugmentData* out_drone_augment_data) const;

    static bool DroneAugmentTypeRequiresAbilities(const DroneAugmentType drone_augment_type);

    bool LoadDroneAugmentAbilitiesData(const nlohmann::json& parent_json, DroneAugmentData* out_drone_augment_data)
        const;

    ///
    /// Consumable
    ///

    bool LoadConsumablesConfig(const nlohmann::json& json_object, ConsumablesConfig* out_config) const;
    bool LoadConsumablesConfig(
        const nlohmann::json& json_parent,
        const std::string_view key,
        ConsumablesConfig* out_config) const;

    // Loads consumalbe type id from JSON parent object by key
    bool LoadConsumableTypeID(
        const nlohmann::json& json_parent,
        const std::string_view key,
        ConsumableTypeID* out_consumable_type_id) const;

    // Loads consumable type id from JSON object
    bool LoadConsumableTypeID(const nlohmann::json& json_object, ConsumableTypeID* out_consumable_type_id) const;

    // Loads consumable instance data from JSON parent object by key
    bool LoadConsumableInstanceData(
        const nlohmann::json& json_parent,
        const std::string_view key,
        ConsumableInstanceData* out_consumable_instance) const;

    // Loads consumable instance from JSON object
    bool LoadConsumableInstanceData(const nlohmann::json& json_object, ConsumableInstanceData* out_consumable_instance)
        const;

    // Loads consumable data from JSON object
    bool LoadConsumableData(const nlohmann::json& json_object, ConsumableData* out_consumable_data) const;

    // Loads an array of consumable instances from JSON array
    bool LoadConsumableInstancesData(
        const nlohmann::json& json_parent,
        const std::string_view key_array,
        std::vector<ConsumableInstanceData>* out_consumables_data) const;

    ///
    /// Suit
    ///

    // Loads suit type id from JSON parent object by key
    bool LoadCombatSuitTypeID(
        const nlohmann::json& json_parent,
        const std::string_view key,
        CombatSuitTypeID* out_suit_type_id) const;

    // Loads suit type id from JSON object
    bool LoadCombatSuitTypeID(const nlohmann::json& json_object, CombatSuitTypeID* out_suit_type_id) const;

    // Loads suit instance data from JSON parent object by key
    bool LoadCombatSuitInstanceData(
        const nlohmann::json& json_parent,
        const std::string_view key,
        CombatSuitInstanceData* out_suit_instance) const;

    // Loads suit instance from JSON object
    bool LoadCombatSuitInstanceData(const nlohmann::json& json_object, CombatSuitInstanceData* out_suit_instance) const;

    // Loads suit data from JSON object
    bool LoadCombatSuitData(const nlohmann::json& json_object, CombatUnitSuitData* out_suit) const;

    ///
    /// Weapon
    ///

    // Loads weapon type id from JSON parent object by key
    bool LoadCombatWeaponTypeID(
        const nlohmann::json& json_parent,
        const std::string_view key,
        CombatWeaponTypeID* out_weapon_type_id) const;

    // Loads weapon type id from JSON object
    bool LoadCombatWeaponTypeID(const nlohmann::json& json_object, CombatWeaponTypeID* out_weapon_type_id) const;

    // Loads weapon instance from JSON parent object by key
    bool LoadCombatWeaponInstanceData(
        const nlohmann::json& json_parent,
        const std::string_view key,
        CombatWeaponInstanceData* out_weapon_instance) const;

    // Loads weapon instance from JSON object
    bool LoadCombatWeaponBaseInstanceData(const nlohmann::json& json_object, CombatWeaponBaseInstanceData* out_instance)
        const;
    bool LoadCombatWeaponAmplifierInstanceData(
        const nlohmann::json& json_object,
        CombatWeaponAmplifierInstanceData* out_instance) const;
    bool LoadCombatWeaponInstanceData(const nlohmann::json& json_object, CombatWeaponInstanceData* out_instance) const;

    // Loads weapon data from JSON object
    bool LoadCombatWeaponData(const nlohmann::json& json_object, CombatUnitWeaponData* out_weapon) const;

    // Update SynergyTypeData with the synergy_type_data from json_object, returns true if the JSON
    // parsing succeeded, false otherwise
    bool LoadSynergyData(const nlohmann::json& json_object, SynergyData* out_synergy_data);

    // Loads combat unit type id from JSON object
    bool LoadCombatUnitTypeID(
        const nlohmann::json& json_parent,
        const std::string_view key,
        CombatUnitTypeID* out_combat_unit_type_id) const;

    // Load data of the battle config from json_object
    bool LoadBattleConfig(const nlohmann::json& json_object, BattleConfig* out_battle_config) const;

    // Load data of the world effects config from json object
    bool LoadWorldEffectsConfig(const nlohmann::json& json_object, WorldEffectsConfig* out_effects_config) const;

    bool LoadWorldEffectConditionConfig(
        const nlohmann::json& json_object,
        WorldEffectConditionConfig* out_condition_config) const;

    bool LoadWorldEffectConditionConfig(
        const nlohmann::json& parent_json,
        const std::string_view key,
        WorldEffectConditionConfig* out_condition_config) const;

    // Loads all the stats from
    bool LoadCombatUnitStats(
        const nlohmann::json& json_parent,
        const std::string_view key,
        const bool parse_strings_as_fixed_point,
        std::unordered_map<StatType, FixedPoint>* out_map) const;

    // Loads all the stats from
    bool LoadCombatUnitStats(
        const nlohmann::json& json_object,
        const bool parse_strings_as_fixed_point,
        std::unordered_map<StatType, FixedPoint>* out_map) const;

    // Helper functions
    const JSONHelper& GetJSONHelper() const
    {
        return json_helper_;
    }

    // Wrapper for JSONHelper::GetIntValue that is extendable in child classes
    bool GetIntValue(const nlohmann::json& json_value, int* out_int) const;
    bool GetIntValue(const nlohmann::json& json_object, const std::string_view key, int* out_int) const;

    bool GetIntValueWithValidation(
        const std::string_view caller_method_name,
        const nlohmann::json& json_parent,
        const std::string_view key,
        std::optional<int> min_value,
        std::optional<int> max_value,
        int* out_int) const;

    // Wrapper for JSONHelper::GetBoolValue
    bool GetBoolValue(const nlohmann::json& json_object, const std::string_view key, bool* out_bool) const
    {
        return json_helper_.GetBoolValue(json_object, key, out_bool);
    }

    // Wrapper for JSONHelper::GetStringValue
    bool GetStringValue(const nlohmann::json& json_object, const std::string_view key, std::string* out_string) const
    {
        return json_helper_.GetStringValue(json_object, key, out_string);
    }

    // Reads regular integer and converts to FixedPoint type
    // Does not support fractional part
    bool GetFixedPointIntValue(const nlohmann::json& json_object, FixedPoint* out_value) const
    {
        int value = 0;
        if (GetIntValue(json_object, &value))
        {
            *out_value = FixedPoint::FromInt(value);
            return true;
        }

        return false;
    }

    // Reads regular integer and converts to FixedPoint type
    // Does not support fractional part
    bool GetFixedPointIntValue(const nlohmann::json& json_object, const std::string_view key, FixedPoint* out_value)
        const
    {
        int value = 0;
        if (GetIntValue(json_object, key, &value))
        {
            *out_value = FixedPoint::FromInt(value);
            return true;
        }

        return false;
    }

    // Reads regular integer and converts to FixedPoint type
    // Does not support fractional part
    bool GetFixedPointIntOptionalValue(
        const nlohmann::json& json_object,
        const std::string_view key,
        FixedPoint* out_value) const
    {
        int value = 0;
        if (GetIntOptionalValue(json_object, key, &value))
        {
            *out_value = FixedPoint::FromInt(value);
            return true;
        }

        return false;
    }

    bool GetIntOptionalValue(const nlohmann::json& json_object, const std::string_view key, int* out_value) const
    {
        if (json_object.contains(key))
        {
            return GetIntValue(json_object, key, out_value);
        }

        return false;
    }

    bool GetBoolOptionalValue(const nlohmann::json& json_object, const std::string_view key, bool* out_value) const
    {
        if (json_object.contains(key))
        {
            return GetBoolValue(json_object, key, out_value);
        }

        return false;
    }

    bool GetStringOptionalValue(const nlohmann::json& json_object, const std::string_view key, std::string* out_string)
        const
    {
        if (json_object.contains(key))
        {
            return GetStringValue(json_object, key, out_string);
        }

        return false;
    }

    // Reads array of encounter mod instances data from JSON object for each team into `out_teams_encounter_mods`.
    // Returns false if `json_object` does not match expected format.
    bool LoadTeamsEncounterMods(
        const nlohmann::json& json_object,
        std::unordered_map<Team, std::vector<EncounterModInstanceData>>* out_teams_encounter_mods) const;

    // Reads array of encounter mod instance data from parent_json[key] into `out_teams_encounter_mods`.
    // Returns false if `parent_json` does not have specified key or parent_json[key] has invalid format.
    bool LoadTeamsEncounterMods(
        const nlohmann::json& parent_json,
        const std::string_view key,
        std::unordered_map<Team, std::vector<EncounterModInstanceData>>* out_teams_encounter_mods) const;

    // Reads encounter mod type id from specified json object to `out_encounter_mod_type_id` pointer.
    // Returns false if `json_object` does not match expected format.
    bool LoadEncounterModTypeID(const nlohmann::json& json_object, EncounterModTypeID* out_encounter_mod_type_id) const;

    // Reads encounter mod instance data from specified json object.
    // Returns false if `json_object` does not match expected format.
    bool LoadEncounterModInstanceData(
        const nlohmann::json& json_object,
        EncounterModInstanceData* out_encounter_mod_instance_data) const;

    // Load hyper combat affinities from JSON
    bool LoadHyperCombatAffinity(
        const nlohmann::json& json_object,
        CombatAffinityOpponentsMap* out_combat_affinity_hyper_data) const;

    // Update HyperData with the combat class counters from json_object, returns true if the JSON
    // parsing succeeded, false otherwise.
    bool LoadHyperData(const nlohmann::json& json_object, HyperData* out_hyper_data) const;

    // Update CombatUnitData with the out_combat_data from json_object, returns true
    // if the JSON parsing succeeded, false otherwise
    bool LoadCombatUnitData(const nlohmann::json& json_object, CombatUnitData* out_combat_data) const;

    // Update HyperBonusData with the bonus json_object, returns true if the JSON
    // parsing succeeded, false otherwise.
    bool LoadHyperConfig(const nlohmann::json& json_object, HyperConfig* out_hyper_config) const;

    // Update EncounterModData with the bonus json_object, returns true if the JSON
    // parsing succeeded, false otherwise.
    bool LoadEncounterModData(const nlohmann::json& json_object, EncounterModData* out_encounter_mod_data) const;

    bool LoadLevelingConfig(const nlohmann::json& json_object, LevelingConfig* out_config) const;
    bool LoadLevelingConfig(const nlohmann::json& json_parent, const std::string_view key, LevelingConfig* out_config)
        const;

    bool LoadOverloadConfig(const nlohmann::json& json_object, OverloadConfig* out_config) const;
    bool LoadOverloadConfig(const nlohmann::json& json_parent, const std::string_view key, OverloadConfig* out_config)
        const;

    bool LoadAugmentAbilitiesData(const nlohmann::json& parent_json, AugmentData* out_augment_data) const;

    bool LoadCombatAffinityStacks(
        const nlohmann::json& json_object,
        std::unordered_map<CombatAffinity, int>* out_combat_affinity_stacks) const;

    bool LoadCombatClassStacks(
        const nlohmann::json& json_object,
        std::unordered_map<CombatClass, int>* out_combat_class_stacks) const;

    bool ValidateAugmentData(const AugmentData& augment_data) const;

    // Loads all the synergy_threshold_abilities into the SynergyData struct
    bool LoadSynergyThresholdsAbilities(const nlohmann::json& json_object, SynergyData* out_synergy_data);

    bool LoadSynergyTeamAbilities(
        const nlohmann::json& json_object,
        const int threshold_value,
        SynergyData* out_synergy_data) const;

    bool LoadSynergyUnitAbilities(
        const nlohmann::json& json_object,
        const int threshold_value,
        SynergyData* out_synergy_data) const;

    // Loads all the intrinsic_abilities into the SynergyData struct
    bool LoadSynergyIntrinsicAbilities(const nlohmann::json& json_object, SynergyData* out_synergy_data) const;
    bool LoadSynergyHyperAbilities(const nlohmann::json& json_object, SynergyData* out_synergy_data) const;

    // Loads all json variables from the json_object
    bool LoadSynergyThresholdVariables(
        const nlohmann::json& parent_json_object,
        std::vector<std::unordered_map<std::string, int>>* out_threshold_variables) const;

    // Load the CombatClass or Combat Affinity from the json_object[key] and
    // json_object[components_key]
    bool LoadCombatSynergies(
        const nlohmann::json& json_object,
        std::string_view key,
        std::string_view components_key,
        const bool is_combat_class,
        CombatSynergyBonus* out_combat_synergy,
        std::vector<CombatSynergyBonus>* out_combat_synergy_components) const;

    // Loads the json array at json_object[array_key] into the out_combat_synergies_any
    bool LoadCombatSynergyComponents(
        const nlohmann::json& json_object,
        std::string_view components_key,
        const bool is_combat_class,
        std::vector<CombatSynergyBonus>* out_combat_synergy_components) const;

    bool LoadSkillDashData(
        const nlohmann::json& parent_json_object,
        const std::string_view key,
        SkillDashData* out_dash_data) const;

    bool LoadSkillDashData(const nlohmann::json& json_object, SkillDashData* out_dash_data) const;

    bool LoadSkillSpawnedCombatUnitData(
        const nlohmann::json& parent_json_object,
        const std::string_view key,
        SkillSpawnedCombatUnitData* out_data) const;

    bool LoadSkillSpawnedCombatUnitData(const nlohmann::json& json_object, SkillSpawnedCombatUnitData* out_data) const;

    bool LoadSkillZoneData(
        const nlohmann::json& parent_json_object,
        const std::string_view key,
        SkillZoneData* out_data) const;

    bool LoadSkillZoneData(const nlohmann::json& json_object, SkillZoneData* out_data) const;

    bool LoadSkillProjectileData(
        const nlohmann::json& parent_json_object,
        const std::string_view key,
        SkillProjectileData* out_data) const;

    bool LoadSkillProjectileData(const nlohmann::json& json_object, SkillProjectileData* out_data) const;

    bool LoadSkillBeamData(
        const nlohmann::json& parent_json_object,
        const std::string_view key,
        SkillBeamData* out_data) const;

    bool LoadSkillBeamData(const nlohmann::json& json_object, SkillBeamData* out_data) const;

    bool LoadHexGridPosition(const nlohmann::json& json_object, HexGridPosition* out_position) const;
    bool LoadHexGridPosition(
        const nlohmann::json& json_object,
        const std::string_view key,
        HexGridPosition* out_position) const;

    // Returns true if parent_json is an object and contains `key` field (of any type).
    // Uses caller method name for logging
    bool EnsureContainsField(
        const std::string_view caller_method_name,
        const nlohmann::json& parent_json,
        const std::string_view field_key) const;

    // Returns true if parent_json is an object and contains `key` field and this field is an object.
    // Uses caller method name for logging
    bool EnsureContainsObjectField(
        const std::string_view caller_method_name,
        const nlohmann::json& parent_json,
        const std::string_view field_key) const;

    // Returns true if parent_json is an object and contains `key` field and this field is an array.
    // Uses caller method name for logging
    bool EnsureContainsArrayField(
        const std::string_view caller_method_name,
        const nlohmann::json& parent_json,
        const std::string_view field_key) const;

    // JSON Loading function for an SkillData
    bool LoadSkill(const nlohmann::json& json_object, const AbilityType ability_Type, SkillData* out_skill) const;

    struct AbilityLoadingOptions
    {
        bool require_trigger = true;
    };

    // JSON Loading function for an AbilityData (with options)
    bool LoadAbility(
        const nlohmann::json& json_object,
        const AbilityType ability_type,
        const AbilityLoadingOptions& options,
        AbilityData* out_ability) const;

    // JSON Loading function for an AbilityData (default options)
    bool LoadAbility(const nlohmann::json& json_object, const AbilityType ability_type, AbilityData* out_ability) const;

protected:
    // Helper to get the raw variable name from the jsonb_object[key]
    // Returns false if we can't extract it
    bool GetRawVariableName(const nlohmann::json& json_object, std::string_view key, std::string* out_raw_variable_name)
        const;
    bool GetRawVariableName(const nlohmann::json& json_value, std::string* out_raw_variable_name) const;

    // JSON Loading function for array of EffectTypeID
    bool LoadEffectTypeIDArray(
        const nlohmann::json& json_object,
        std::string_view key_typeid_array,
        std::vector<EffectTypeID>* out_values) const;

    // Loads a combat unit type data from the JSON object
    bool LoadCombatUnitTypeData(const nlohmann::json& json_object, CombatUnitTypeData* out_type_data) const;

    bool LoadCombatUnitStats(
        const nlohmann::json& json_object,
        const CombatUnitType unit_type,
        StatsData* out_unit_stats) const;

    // JSON Loading function for all AbilitiesData of a type (with options)
    bool LoadAbilitiesData(
        const nlohmann::json& json_object,
        const AbilityType ability_type,
        std::string_view key_abilities_array,
        const SourceContextData& source_context,
        const AbilityLoadingOptions& options,
        AbilitiesData* out_abilities) const;

    // JSON Loading function for all AbilitiesData of a type (default options)
    bool LoadAbilitiesData(
        const nlohmann::json& json_object,
        const AbilityType ability_type,
        std::string_view key_abilities_array,
        const SourceContextData& source_context,
        AbilitiesData* out_abilities) const;

    bool LoadAbilityDuration(const nlohmann::json& json_object, int* out_ability_duration) const;

    // JSON Loading function for an StatsData
    // out_read_stats_set - an optional set of read stats
    bool LoadStats(
        const nlohmann::json& stats_json_object,
        const bool all_keys_are_stats,
        StatsData* out_stats_data,
        std::unordered_set<StatType>* out_read_stats_set) const;

    void LoadMissingStatsDefaults(StatsData* out_stats_data, std::unordered_set<StatType>* out_read_stats_set) const;

    void LoadStatDefaultIfMissing(
        const StatType stat,
        StatsData* out_stats_data,
        std::unordered_set<StatType>* out_read_stats_set) const;

    // Checks that all StatType enum elements, except StatsHelper::IsStatIgnoredForReading, are present in
    // read_stats_set
    bool EnsureAllStatsWereRead(const std::unordered_set<StatType>& read_stats_set) const;

    bool EnsureSpecificStatsWereRead(
        const StatType* check_stats_array,
        const size_t check_stats_count,
        const std::unordered_set<StatType>& read_stats_set) const;

    bool LoadSkillTargetingData(
        const nlohmann::json& json_parent,
        const std::string_view key,
        SkillTargetingData* out_targeting_data) const;
    bool LoadSkillTargetingData(const nlohmann::json& json_object, SkillTargetingData* out_targeting_data) const;

    bool LoadSkillDeploymentData(
        const nlohmann::json& json_parent,
        const std::string_view key,
        SkillDeploymentData* out_deployment_data) const;
    bool LoadSkillDeploymentData(const nlohmann::json& json_object, SkillDeploymentData* out_deployment_data) const;

    // JSON loading function for EffectTypeID
    bool LoadEffectTypeID(const nlohmann::json& json_object, EffectTypeID* out_effect_type_id) const;

    // JSON loading function for out_effect_validations
    bool LoadValidationList(
        const nlohmann::json& json_object,
        std::string_view key_validation_list,
        EffectValidations* out_effect_validations) const;

    // JSON loading function for out_distance_check
    bool LoadValidationDistanceCheck(const nlohmann::json& json_object, ValidationDistanceCheck* out_distance_check)
        const;

    // JSON loading function for out_expression_comparison
    bool LoadValidationComparisonExpression(
        const nlohmann::json& json_object,
        ValidationEffectExpressionComparison* out_expression_comparison) const;

    // JSON Loading function for an out_effect
    struct EffectLoadingParameters
    {
        bool for_aura = false;
        bool id_required = false;
    };
    bool LoadEffect(
        const nlohmann::json& json_object,
        const EffectLoadingParameters& parameters,
        EffectData* out_effect) const;
    bool LoadEffectDataForEmpower(const nlohmann::json& json_object, EffectData* out_effect) const;

    // JSON Loading function for condition data of effect
    bool LoadEffectDataForCondition(const nlohmann::json& json_object, EffectData* out_effect) const;

    // JSON Loading function for an out_effect_package
    bool LoadEffectPackage(const nlohmann::json& parent_json_object, EffectPackage* out_effect_package) const;

    // JSON Loading function for an out_effects
    bool LoadAbilityTypesArray(
        const nlohmann::json& json_object,
        std::string_view key_abilities_array,
        std::vector<AbilityType>* out_ability_types) const;

    // JSON Loading function for an out_effects
    bool LoadEffectsArray(
        const nlohmann::json& json_object,
        std::string_view key_effects_array,
        const EffectLoadingParameters& parameters,
        std::vector<EffectData>* out_effects) const;

    // JSON loading function for propagation related attributes in EffectPackageAttributes
    bool LoadEffectPackagePropagationAttributes(
        const nlohmann::json& json_object,
        EffectPackageAttributes* out_attributes) const;

    // JSON loading function for EffectPackageAttributes
    bool LoadEffectPackageAttributes(
        const nlohmann::json& json_object,
        EffectPackageAttributes* out_optional_attributes) const;

    // JSON loading function for EffectDataAttributes
    bool LoadEffectDataAttributes(const nlohmann::json& json_object, EffectDataAttributes* out_optional_attributes)
        const;

    // JSON loading function for AbilityActivationTriggerData
    bool LoadAbilityActivationTriggerData(
        const nlohmann::json& json_object,
        AbilityActivationTriggerData* out_trigger_attributes) const;

    // JSON loading function for AbilityActivationTriggerData from parent json
    bool LoadAbilityActivationTriggerData(
        const nlohmann::json& parent_json,
        const std::string_view key_activation_trigger,
        AbilityActivationTriggerData* out_trigger_attributes) const;

    bool LoadTriggerAllegianceFromJSON(
        const nlohmann::json& json_object,
        AbilityActivationTriggerData* out_trigger_attributes) const;

    // Helper to get the curent variable value
    bool GetVariableValue(const std::string_view raw_variable_name, int* out_value) const;

    // JSON Loading function for an out_effect_expression
    bool LoadEffectExpression(
        const nlohmann::json& json_parent_object,
        const std::string_view key,
        EffectExpression* out_effect_expression) const;
    bool LoadEffectExpression(const nlohmann::json& expression_json_value, EffectExpression* out_effect_expression)
        const;

    struct StatParameters
    {
        std::string_view stat_str;
        bool is_metered_stat_percentage = false;
        bool is_custom_evaluation_function = false;
    };

    static StatParameters ParseStatParameters(const std::string_view string);

    // JSON Loading function for an out_effect_lifetime
    bool LoadEffectLifetime(
        const nlohmann::json& json_object,
        const EffectLoadingParameters& parameters,
        EffectLifetime* out_effect_lifetime) const;

    // JSON loading function for the set of enumeration objects
    template <auto enum_to_str_function, typename Collection>
    bool LoadEnumSetFromJSON(
        const nlohmann::json& json_object,
        std::string_view value_type_name,
        std::string_view key,
        Collection* out_enum_set) const
    {
        using EnumType = typename Collection::value_type;
        static_assert(std::is_enum_v<EnumType>);

        out_enum_set->clear();

        auto element_callback = [&](const size_t index, const nlohmann::json& json_array_element) -> bool
        {
            // Get the enum value
            EnumType value = EnumType::kNone;
            if (!json_helper_.GetEnumValue(json_array_element, &value))
            {
                LogErr(
                    "BaseDataLoader::LoadEnumSetFromJSON - Failed to extract {} at index = {}",
                    value_type_name,
                    index);
                return false;
            }

            // Add to the set
            const bool added = out_enum_set->insert(value).second;
            if (!added)
            {
                // Not an error but worth notifying
                LogWarn(
                    "BaseDataLoader::LoadEnumSetFromJSON - found duplicate item \"{}\" in JSON array at "
                    "index {}",
                    enum_to_str_function(value),
                    index);
            }

            return true;
        };

        // Iterate all entries
        return json_helper_.WalkArray(json_object, key, true, element_callback);
    }

    // JSON loading function for the set of enumeration objects
    template <typename Enum>
    bool LoadEnumSet(
        const nlohmann::json& json_object,
        std::string_view value_type_name,
        std::string_view key,
        EnumSet<Enum>* out_enum_set) const
    {
        static_assert(std::is_enum_v<Enum>);

        out_enum_set->Clear();

        auto element_callback = [&](const size_t index, const nlohmann::json& json_array_element) -> bool
        {
            // Get the enum value
            Enum value = Enum::kNone;
            if (!json_helper_.GetEnumValue(json_array_element, &value))
            {
                LogErr(
                    "BaseDataLoader::LoadEnumSetFromJSON - Failed to extract {} at index = {}",
                    value_type_name,
                    index);
                return false;
            }

            // Check if already there
            if (out_enum_set->Contains(value))
            {
                // Not an error but worth notifying
                LogWarn(
                    "BaseDataLoader::LoadEnumSetFromJSON - found duplicate item \"{}\" in JSON array at "
                    "index {}",
                    value,
                    index);
            }

            // Add to the set
            out_enum_set->Add(value);

            return true;
        };

        // Iterate all guidance enums
        return json_helper_.WalkArray(json_object, key, true, element_callback);
    }

    // JSON loading function for guidances
    bool LoadGuidancesSet(const nlohmann::json& json_object, std::string_view key, EnumSet<GuidanceType>* out_guidance)
        const
    {
        return LoadEnumSet(json_object, "GuidanceType", key, out_guidance);
    }

    // JSON loading function for damage type
    bool LoadEffectDamageTypesSet(
        const nlohmann::json& json_object,
        std::string_view key,
        EnumSet<EffectDamageType>* out_damage_types) const
    {
        return LoadEnumSet(json_object, "EffectDamageType", key, out_damage_types);
    }

    bool GetExprOptionalValue(const nlohmann::json& json_object, std::string_view key, EffectExpression* out_value)
        const
    {
        if (json_object.contains(key))
        {
            return LoadEffectExpression(json_object, key, out_value);
        }

        return false;
    }

    template <typename Enum>
    bool GetEnumOptionalValue(const nlohmann::json& json_object, std::string_view key, Enum* out_value) const
    {
        static_assert(std::is_enum_v<Enum>);
        if (json_object.contains(key))
        {
            return json_helper_.GetEnumValue(json_object, key, out_value);
        }

        return false;
    }

    std::shared_ptr<Logger> GetLogger() const
    {
        return json_helper_.GetLogger();
    }

    // Helper logger functions
    template <typename... Args>
    void LogInfo(const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        GetLogger()->LogInfo(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void LogErr(const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        GetLogger()->LogErr(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void LogWarn(const fmt::format_string<Args...>& fmt, Args&&... args) const
    {
        GetLogger()->LogWarn(fmt, std::forward<Args>(args)...);
    }

    JSONHelper json_helper_;
    std::unordered_map<std::string, int> variables_{};
};  // class BaseDataLoader

}  // namespace simulation
