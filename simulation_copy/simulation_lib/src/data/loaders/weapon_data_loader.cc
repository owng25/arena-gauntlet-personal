#include "base_data_loader.h"
#include "data/loaders/json_keys.h"
#include "data/weapon/weapon_data.h"
#include "data/weapon/weapon_instance_data.h"
#include "data/weapon/weapon_type_id.h"

namespace simulation
{

bool BaseDataLoader::LoadCombatWeaponTypeID(
    const nlohmann::json& json_parent,
    const std::string_view key,
    CombatWeaponTypeID* out_type_id) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadCombatWeaponTypeID";
    return EnsureContainsObjectField(method_name, json_parent, key) &&
           LoadCombatWeaponTypeID(json_parent[key], out_type_id);
}

bool BaseDataLoader::LoadCombatWeaponTypeID(const nlohmann::json& json_object, CombatWeaponTypeID* out_type_id) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadCombatWeaponTypeID";

    // Is not an object?
    if (!json_object.is_object())
    {
        LogErr("{} - json_object is not an object", method_name);
        return false;
    }

    if (!json_helper_.GetStringValue(json_object, JSONKeys::kName, &out_type_id->name))
    {
        LogErr("{} - can't parse key = {}", method_name, JSONKeys::kName);
        return false;
    }

    if (!json_helper_.GetIntValue(json_object, JSONKeys::kStage, &out_type_id->stage))
    {
        LogErr("{} - can't parse key = {}", method_name, JSONKeys::kStage);
        return false;
    }

    // Optional variation field
    out_type_id->variation.clear();
    if (json_object.contains(JSONKeys::kVariation))
    {
        if (!json_helper_.GetStringValue(json_object, JSONKeys::kVariation, &out_type_id->variation))
        {
            LogErr("{} - can't parse key = {}", method_name, JSONKeys::kVariation);
            return false;
        }
    }

    // Load Optional combat affinity
    json_helper_.GetEnumValueOptional(json_object, JSONKeys::kCombatAffinity, &out_type_id->combat_affinity);

    return true;
}

bool BaseDataLoader::LoadCombatWeaponData(const nlohmann::json& json_object, CombatUnitWeaponData* out_weapon) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadCombatUnitWeapon";
    if (!LoadCombatWeaponTypeID(json_object, &out_weapon->type_id))
    {
        LogErr("{} - Failed to load weapon type id.", method_name);
        return false;
    }

    if (!json_helper_.GetEnumValue(json_object, JSONKeys::kType, &out_weapon->weapon_type))
    {
        LogErr("Failed to read required field \"{}\".", JSONKeys::kType);
        return false;
    }

    // Required AmplifierForWeapon field which tells us for which weapon is this amplifier for
    if (out_weapon->weapon_type == WeaponType::kAmplifier)
    {
        if (!json_object.contains(JSONKeys::kAmplifierForWeapon))
        {
            LogErr("{} - Key = {} is required for amplifier weapons", method_name, JSONKeys::kAmplifierForWeapon);
            return false;
        }

        if (!LoadCombatWeaponTypeID(
                json_object[JSONKeys::kAmplifierForWeapon],
                &out_weapon->amplifier_for_weapon_type_id))
        {
            LogErr("{} - Key = {}, Failed to load weapon.", method_name, JSONKeys::kAmplifierForWeapon);
            return false;
        }
    }

    if (!GetIntValue(json_object, JSONKeys::kTier, &out_weapon->tier))
    {
        return false;
    }

    json_helper_.GetEnumValueOptional(json_object, JSONKeys::kCombatClass, &out_weapon->combat_class);

    if (out_weapon->combat_class != CombatClass::kNone)
    {
        json_helper_.GetEnumValueOptional(
            json_object,
            JSONKeys::kDominantCombatClass,
            &out_weapon->dominant_combat_class);

        if (!json_helper_.GetEnumValue(json_object, JSONKeys::kDominantCombatClass, &out_weapon->dominant_combat_class))
        {
            LogErr(
                "{} - {} field is required when {} is not {}",
                method_name,
                JSONKeys::kDominantCombatClass,
                JSONKeys::kCombatClass,
                CombatClass::kNone);
            return false;
        }
    }

    // Copy the affinity from type_id
    out_weapon->combat_affinity = out_weapon->type_id.combat_affinity;

    if (out_weapon->combat_affinity != CombatAffinity::kNone)
    {
        if (!json_helper_
                 .GetEnumValue(json_object, JSONKeys::kDominantCombatAffinity, &out_weapon->dominant_combat_affinity))
        {
            LogErr(
                "{} - {} field is required when {} is not {}",
                method_name,
                JSONKeys::kDominantCombatAffinity,
                JSONKeys::kCombatAffinity,
                CombatAffinity::kNone);
            return false;
        }
    }

    // Load weapon Attack abilities
    if (json_object.contains(JSONKeys::kAttackAbilities))
    {
        SourceContextData context;
        context.combat_unit_ability_type = AbilityType::kAttack;
        context.from_weapon_type = out_weapon->weapon_type;
        if (!LoadAbilitiesData(
                json_object,
                AbilityType::kAttack,
                JSONKeys::kAttackAbilities,
                context,
                &out_weapon->attack_abilities))
        {
            return false;
        }
    }
    else
    {
        LogErr("{} - can't get AttackAbilities JSON Object", method_name);
        return false;
    }

    // Load weapon Omega abilities
    if (json_object.contains(JSONKeys::kOmegaAbilities))
    {
        SourceContextData context;
        context.combat_unit_ability_type = AbilityType::kOmega;
        context.from_weapon_type = out_weapon->weapon_type;
        if (!LoadAbilitiesData(
                json_object,
                AbilityType::kOmega,
                JSONKeys::kOmegaAbilities,
                context,
                &out_weapon->omega_abilities))
        {
            return false;
        };
    }
    else
    {
        LogErr("{} - can't get OmegaAbilities JSON Object", method_name);
        return false;
    }

    // Load weapon innate abilities
    if (json_object.contains(JSONKeys::kInnateAbilities))
    {
        SourceContextData context;
        context.combat_unit_ability_type = AbilityType::kInnate;
        context.from_weapon_type = out_weapon->weapon_type;
        if (!LoadAbilitiesData(
                json_object,
                AbilityType::kInnate,
                JSONKeys::kInnateAbilities,
                context,
                &out_weapon->innate_abilities))
        {
            return false;
        }
    }

    // Load Weapon Stats
    if (!json_object.contains(JSONKeys::kStats))
    {
        LogErr("{} - can't get Stats JSON Object", method_name);
        return false;
    }

    std::unordered_set<StatType> read_stats_set;
    if (!LoadStats(json_object[JSONKeys::kStats], false, &out_weapon->stats, &read_stats_set))
    {
        return false;
    }

    const bool bIsAmplifier = out_weapon->weapon_type == WeaponType::kAmplifier;
    if (!bIsAmplifier)
    {
        // Normal weapons act like illuvial combat unit data stats, see BaseDataLoader::LoadCombatUnitStats

        // Try to load defaults for stats that not loaded from JSON
        LoadMissingStatsDefaults(&out_weapon->stats, &read_stats_set);
    }

    // Weapon check
    if (!EnsureSpecificStatsWereRead(kWeaponStatTypes.data(), kWeaponStatTypes.size(), read_stats_set))
    {
        return false;
    }

    if (json_object.contains(JSONKeys::kEffectsReplacements))
    {
        out_weapon->effect_data_replacements = EffectDataReplacements::Create();

        // Require effects
        EffectLoadingParameters loading_parameters;
        loading_parameters.id_required = true;
        if (!LoadEffectsArray(
                json_object,
                JSONKeys::kEffectsReplacements,
                loading_parameters,
                &out_weapon->effect_data_replacements->replacements))
        {
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadCombatWeaponInstanceData(
    const nlohmann::json& json_parent,
    const std::string_view key,
    CombatWeaponInstanceData* out_instance) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadCombatWeaponInstanceData";
    return EnsureContainsObjectField(method_name, json_parent, key) &&
           LoadCombatWeaponInstanceData(json_parent[key], out_instance);
}

bool BaseDataLoader::LoadCombatWeaponBaseInstanceData(
    const nlohmann::json& json_object,
    CombatWeaponBaseInstanceData* out_instance) const
{
    GetStringOptionalValue(json_object, JSONKeys::kID, &out_instance->id);
    return LoadCombatWeaponTypeID(json_object, JSONKeys::kTypeID, &out_instance->type_id);
}

bool BaseDataLoader::LoadCombatWeaponAmplifierInstanceData(
    const nlohmann::json& json_object,
    CombatWeaponAmplifierInstanceData* out_instance) const
{
    return LoadCombatWeaponBaseInstanceData(json_object, out_instance);
}

bool BaseDataLoader::LoadCombatWeaponInstanceData(
    const nlohmann::json& json_object,
    CombatWeaponInstanceData* out_instance) const
{
    // Optional amplifiers
    if (json_object.contains(JSONKeys::kEquippedAmplifiers))
    {
        json_helper_.WalkArray(
            json_object,
            JSONKeys::kEquippedAmplifiers,
            true,
            [&](const size_t index, const nlohmann::json& json_array_element) -> bool
            {
                CombatWeaponAmplifierInstanceData amplifier_instance;
                if (!LoadCombatWeaponAmplifierInstanceData(json_array_element, &amplifier_instance))
                {
                    LogErr(
                        "BaseDataLoader::LoadCombatWeaponInstanceData - Failed to load amplifier instance at index = "
                        "{}",
                        index);
                    return false;
                }

                out_instance->equipped_amplifiers.push_back(amplifier_instance);
                return true;
            });
    }

    return LoadCombatWeaponBaseInstanceData(json_object, out_instance);
}
}  // namespace simulation
