#include "base_data_loader.h"
#include "data/drone_augment/drone_augment_data.h"
#include "data/drone_augment/drone_augment_id.h"
#include "data/enums.h"
#include "data/loaders/json_keys.h"
#include "ecs/world.h"

namespace simulation
{
bool BaseDataLoader::LoadDroneAugmentTypeID(
    const nlohmann::json& json_parent,
    const std::string_view key,
    DroneAugmentTypeID* out_drone_augment_type_id) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadDroneAugmentTypeID";
    return EnsureContainsObjectField(method_name, json_parent, key) &&
           LoadDroneAugmentTypeID(json_parent[key], out_drone_augment_type_id);
}

bool BaseDataLoader::LoadDroneAugmentTypeID(
    const nlohmann::json& json_object,
    DroneAugmentTypeID* out_drone_augment_type_id) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadDroneAugmentTypeID";
    // Isn't an object?
    if (!json_object.is_object())
    {
        LogErr("{} - json_object is not an object", method_name);
        return false;
    }

    if (!json_helper_.GetStringValue(json_object, JSONKeys::kName, &out_drone_augment_type_id->name))
    {
        LogErr("{} - can't parse key = {}", method_name, JSONKeys::kName);
        return false;
    }

    if (!json_helper_.GetIntValue(json_object, JSONKeys::kStage, &out_drone_augment_type_id->stage))
    {
        LogErr("{} - can't parse key = {}", method_name, JSONKeys::kStage);
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadDroneAugmentData(const nlohmann::json& json_object, DroneAugmentData* out_drone_augment_data)
    const
{
    static std::string_view method_name = "BaseDataLoader::LoadDroneAugmentData";
    if (!LoadDroneAugmentTypeID(json_object, &out_drone_augment_data->type_id))
    {
        LogErr("{} - Failed to read drone augment type ID from {}", method_name, json_object.dump());
        return false;
    }

    if (!json_helper_.GetEnumValue(json_object, JSONKeys::kType, &out_drone_augment_data->drone_augment_type))
    {
        LogErr("Failed to read required field \"{}\".", JSONKeys::kType);
        return false;
    }

    return LoadDroneAugmentAbilitiesData(json_object, out_drone_augment_data);
}

bool BaseDataLoader::DroneAugmentTypeRequiresAbilities(const DroneAugmentType drone_augment_type)
{
    return drone_augment_type == DroneAugmentType::kSimulation;
}

bool BaseDataLoader::LoadDroneAugmentAbilitiesData(
    const nlohmann::json& json_object,
    DroneAugmentData* out_drone_augment_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadDroneAugmentAbilitiesData";
    const DroneAugmentType drone_augment_type = out_drone_augment_data->drone_augment_type;
    const bool require_abilities = DroneAugmentTypeRequiresAbilities(drone_augment_type);

    // Source context for loaded abilities
    SourceContextData context;
    context.combat_unit_ability_type = AbilityType::kInnate;
    context.Add(SourceContextType::kDroneAugment);

    if (require_abilities)
    {
        if (!LoadAbilitiesData(
                json_object,
                AbilityType::kInnate,
                JSONKeys::kAbilities,
                context,
                &out_drone_augment_data->innate_abilities))
        {
            LogErr("{} - Failed to read drone augment abilities from JSON.", method_name);
            return false;
        }

        if (out_drone_augment_data->innate_abilities.abilities.empty())
        {
            LogErr("{} - {} drone augments require at least one ability", method_name, drone_augment_type);
            return false;
        }
    }
    else
    {
        // Ensure that augments that don't have abilities do not specify ability-specific keys
        if (json_object.contains(JSONKeys::kAbilities))
        {
            LogErr("{} - {} drone augments must not specify abilities", method_name, drone_augment_type);
            return false;
        }
    }

    return true;
}
}  // namespace simulation