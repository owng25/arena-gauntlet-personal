#include <optional>
#include <unordered_map>

#include "base_data_loader.h"
#include "data/augment/augment_data.h"
#include "data/augment/augment_instance_data.h"
#include "data/enums.h"
#include "data/loaders/json_keys.h"
#include "data/source_context.h"
#include "ecs/world.h"

namespace simulation
{
bool BaseDataLoader::LoadAugmentsConfig(const nlohmann::json& json_object, AugmentsConfig* out_config) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadAugmentsConfig";

    // Default to struct default
    int max_augments = static_cast<int>(AugmentsConfig{}.max_augments);
    if (GetIntValue(json_object, JSONKeys::kMaxAugments, &max_augments))
    {
        if (max_augments < 0)
        {
            LogErr("{} - negative value in property \"{}\"", method_name, max_augments);
            return false;
        }

        out_config->max_augments = static_cast<size_t>(max_augments);
    }
    else
    {
        LogErr("{} - failed to read \"{}\" key. JSON: {}", method_name, JSONKeys::kMaxAugments, json_object.dump(4));
        return false;
    }

    // Default to struct default
    int max_drone_augments = static_cast<int>(AugmentsConfig{}.max_drone_augments);
    if (GetIntValue(json_object, JSONKeys::kMaxDroneAugments, &max_drone_augments))
    {
        if (max_drone_augments < 0)
        {
            LogErr("{} - negative value in property \"{}\"", method_name, max_augments);
            return false;
        }

        out_config->max_drone_augments = static_cast<size_t>(max_drone_augments);
    }
    else
    {
        // Its optional for now
        LogErr(
            "{} - failed to read \"{}\" key. JSON: {}",
            method_name,
            JSONKeys::kMaxDroneAugments,
            json_object.dump(4));
    }

    return true;
}

bool BaseDataLoader::LoadAugmentsConfig(
    const nlohmann::json& json_parent,
    const std::string_view key,
    AugmentsConfig* out_config) const
{
    return EnsureContainsObjectField("BaseDataLoader::LoadAugmentsConfig", json_parent, key) &&
           LoadAugmentsConfig(json_parent[key], out_config);
}

bool BaseDataLoader::LoadAugmentTypeID(
    const nlohmann::json& json_parent,
    const std::string_view key,
    AugmentTypeID* out_augment_type_id) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadAugmentTypeID";
    return EnsureContainsObjectField(method_name, json_parent, key) &&
           LoadAugmentTypeID(json_parent[key], out_augment_type_id);
}

// Loads the object at json_parent[key] into out_augment_type_id
bool BaseDataLoader::LoadAugmentTypeID(const nlohmann::json& json_object, AugmentTypeID* out_augment_type_id) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadAugmentTypeID";
    // Is not an object?
    if (!json_object.is_object())
    {
        LogErr("{} - json_object is not an object", method_name);
        return false;
    }

    if (!json_helper_.GetStringValue(json_object, JSONKeys::kName, &out_augment_type_id->name))
    {
        LogErr("{} - can't parse key = {}", method_name, JSONKeys::kName);
        return false;
    }

    if (!json_helper_.GetIntValue(json_object, JSONKeys::kStage, &out_augment_type_id->stage))
    {
        LogErr("{} - can't parse key = {}", method_name, JSONKeys::kStage);
        return false;
    }

    if (json_object.contains(JSONKeys::kVariation))
    {
        if (!json_helper_.GetStringValue(json_object, JSONKeys::kVariation, &out_augment_type_id->variation))
        {
            LogErr("{} - can't parse key = {}", method_name, JSONKeys::kVariation);
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadAugmentInstanceData(
    const nlohmann::json& json_object,
    AugmentInstanceData* out_augment_instance_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadAugmentInstanceData";

    GetStringOptionalValue(json_object, JSONKeys::kID, &out_augment_instance_data->id);
    if (!LoadAugmentTypeID(json_object, JSONKeys::kTypeID, &out_augment_instance_data->type_id))
    {
        LogErr("{} - Failed to read key = {}", method_name, JSONKeys::kTypeID);
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadCombatAffinityStacks(
    const nlohmann::json& json_object,
    std::unordered_map<CombatAffinity, int>* out_combat_affinity_stacks) const
{
    for (const auto& key_value_json : json_object.items())
    {
        std::string_view combat_affinity_str = key_value_json.key();
        const CombatAffinity combat_affinity = Enum::StringToCombatAffinity(combat_affinity_str);
        if (combat_affinity == CombatAffinity::kNone)
        {
            LogErr("Invalid combat affinity \"{}\".", combat_affinity_str);
            return false;
        }

        int stacks_count = 0;
        if (!json_helper_.GetIntValue(key_value_json.value(), &stacks_count))
        {
            LogErr("Failed to read stacks count for \"{}\" affinity", combat_affinity_str);
            return false;
        }

        (*out_combat_affinity_stacks)[combat_affinity] += stacks_count;
    }

    return true;
}

bool BaseDataLoader::LoadCombatClassStacks(
    const nlohmann::json& json_object,
    std::unordered_map<CombatClass, int>* out_combat_class_stacks) const
{
    for (const auto& key_value_json : json_object.items())
    {
        std::string_view combat_class_str = key_value_json.key();
        const CombatClass combat_class = Enum::StringToCombatClass(combat_class_str);
        if (combat_class == CombatClass::kNone)
        {
            LogErr("Invalid combat class \"{}\".", combat_class_str);
            return false;
        }

        int stacks_count = 0;
        if (!json_helper_.GetIntValue(key_value_json.value(), &stacks_count))
        {
            LogErr("Failed to read stacks count for combat_class = \"{}\"", combat_class_str);
            return false;
        }

        (*out_combat_class_stacks)[combat_class] += stacks_count;
    }

    return true;
}

bool BaseDataLoader::ValidateAugmentData(const AugmentData& augment_data) const
{
    static_assert(
        static_cast<int>(AugmentType::kNum) == 5,
        "Update augment type validation below when AugmentType enum changes");

    const bool has_any_synergy = ((augment_data.combat_classes.size() + augment_data.combat_affinities.size()) > 0);

    static constexpr std::string_view method_name = "BaseDataLoader::ValidateAugmentData";

    if (has_any_synergy && augment_data.augment_type != AugmentType::kSynergyBonus)
    {
        LogErr(
            "{} - An augment with synergies must have \"{}\" augment type (current type is \"{}\")",
            method_name,
            AugmentType::kSynergyBonus,
            augment_data.augment_type);
        return false;
    }

    if (augment_data.augment_type == AugmentType::kSynergyBonus)
    {
        if (!has_any_synergy)
        {
            LogErr("{} - An augment of type {} must add some synergies.", method_name, augment_data.augment_type);
            return false;
        }
    }
    else
    {
        if (augment_data.innate_abilities.empty())
        {
            LogErr("{} - {} augments must have abilities.", method_name, augment_data.augment_type);
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadAugmentData(const nlohmann::json& json_object, AugmentData* out_augment_data) const
{
    static std::string_view method_name = "BaseDataLoader::LoadAugmentData";
    if (!LoadAugmentTypeID(json_object, &out_augment_data->type_id))
    {
        LogErr("{} - Failed to read augment type from {}", method_name, json_object.dump());
        return false;
    }

    if (!json_helper_.GetEnumValue(json_object, JSONKeys::kType, &out_augment_data->augment_type))
    {
        LogErr("Failed to read required field \"{}\".", JSONKeys::kType);
        return false;
    }

    if (!GetIntValue(json_object, "Tier", &out_augment_data->tier))
    {
        return false;
    }

    if (!LoadAugmentAbilitiesData(json_object, out_augment_data))
    {
        return false;
    }

    static constexpr std::string_view key_combat_affinities = "CombatAffinities";
    if (json_object.contains(key_combat_affinities))
    {
        const auto& affinities_json = json_object[key_combat_affinities];
        if (!affinities_json.is_object())
        {
            LogErr(
                "{} expected to be an object where key is a combat affinity and the value is the number of stacks.",
                key_combat_affinities);
            return false;
        }

        if (!LoadCombatAffinityStacks(affinities_json, &out_augment_data->combat_affinities))
        {
            return false;
        }
    }

    static constexpr std::string_view key_combat_classes = "CombatClasses";
    if (json_object.contains(key_combat_classes))
    {
        const auto& combat_classes_json = json_object[key_combat_classes];
        if (!combat_classes_json.is_object())
        {
            LogErr(
                "{} expected to be an object where key is a combat class and the value is the number of stacks.",
                key_combat_classes);
            return false;
        }

        if (!LoadCombatClassStacks(combat_classes_json, &out_augment_data->combat_classes))
        {
            return false;
        }
    }

    return ValidateAugmentData(*out_augment_data);
}

bool BaseDataLoader::LoadAugmentAbilitiesData(const nlohmann::json& json_object, AugmentData* out_augment_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadAugmentAbilitiesDataFromJSON";
    const AugmentType augment_type = out_augment_data->augment_type;

    // Source context for loaded abilities
    SourceContextData context;
    context.combat_unit_ability_type = AbilityType::kInnate;
    context.Add(SourceContextType::kAugment);

    if (json_object.contains(JSONKeys::kAbilities))
    {
        if (json_object.contains(JSONKeys::kActivationTrigger))
        {
            LogErr(
                "{} - Augments of type {} must not specify shared activation trigger. Please, specify activation "
                "trigger in each ability for augments of this type.",
                method_name,
                augment_type);
            return false;
        }

        // Read Abilities Array
        const bool abilities_ok = json_helper_.WalkArray(
            json_object,
            JSONKeys::kAbilities,
            true,
            [&](const size_t index, const nlohmann::json& json_array_element) -> bool
            {
                auto ability = AbilityData::Create();
                ability->source_context = context;

                AbilityLoadingOptions options{};
                options.require_trigger = true;
                if (!LoadAbility(json_array_element, AbilityType::kInnate, options, ability.get()))
                {
                    LogErr(
                        "{} - Failed to extract object from array  key =  `{}`, index = {}",
                        method_name,
                        JSONKeys::kAbilities,
                        index);
                    return false;
                }

                out_augment_data->innate_abilities.push_back(std::move(ability));

                return true;
            });

        if (!abilities_ok)
        {
            LogErr("{} - Failed to read augment abilities from JSON.", method_name);
            return false;
        }

        if (out_augment_data->innate_abilities.empty())
        {
            LogErr("{} - {} augments require at least one ability", method_name, augment_type);
            return false;
        }
    }
    else
    {
        // Ensure that augments that don't have abilities do not specify ability-specific keys

        if (json_object.contains(JSONKeys::kActivationTrigger))
        {
            LogErr("{} - {} augments must not specify activation trigger", method_name, augment_type);
            return false;
        }
    }

    return true;
}

}  // namespace simulation
