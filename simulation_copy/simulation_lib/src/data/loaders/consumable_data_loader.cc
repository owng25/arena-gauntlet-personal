
#include "base_data_loader.h"
#include "data/consumable/consumable_data.h"
#include "data/consumable/consumable_instance_data.h"
#include "data/consumable/consumable_type_id.h"
#include "data/enums.h"
#include "data/loaders/json_keys.h"
#include "data/source_context.h"
#include "ecs/world.h"

namespace simulation
{
bool BaseDataLoader::LoadConsumablesConfig(const nlohmann::json& json_object, ConsumablesConfig* out_config) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadConsumablesConfig";
    int max_consumables = 0;
    if (!GetIntValue(json_object, JSONKeys::kMaxConsumables, &max_consumables))
    {
        LogErr("{} - failed to read \"{}\" key. JSON: {}", method_name, JSONKeys::kMaxConsumables, json_object.dump(4));
        return false;
    }

    if (max_consumables < 0)
    {
        LogErr("{} - negtive value in property \"{}\"", method_name, max_consumables);
        return false;
    }

    out_config->max_consumables = static_cast<size_t>(max_consumables);
    return true;
}

bool BaseDataLoader::LoadConsumablesConfig(
    const nlohmann::json& json_parent,
    const std::string_view key,
    ConsumablesConfig* out_config) const
{
    return EnsureContainsObjectField("BaseDataLoader::LoadConsumablesConfig", json_parent, key) &&
           LoadConsumablesConfig(json_parent[key], out_config);
}

bool BaseDataLoader::LoadConsumableTypeID(
    const nlohmann::json& json_parent,
    const std::string_view key,
    ConsumableTypeID* out_type_id) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadConsumableTypeID";
    return EnsureContainsObjectField(method_name, json_parent, key) &&
           LoadConsumableTypeID(json_parent[key], out_type_id);
}

bool BaseDataLoader::LoadConsumableTypeID(const nlohmann::json& json_object, ConsumableTypeID* out_type_id) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadConsumableTypeID";

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

    return true;
}

bool BaseDataLoader::LoadConsumableInstanceData(
    const nlohmann::json& json_parent,
    const std::string_view key,
    ConsumableInstanceData* out_instance) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadConsumableInstanceData";
    return EnsureContainsObjectField(method_name, json_parent, key) &&
           LoadConsumableInstanceData(json_parent[key], out_instance);
}

bool BaseDataLoader::LoadConsumableInstanceData(const nlohmann::json& json_object, ConsumableInstanceData* out_instance)
    const
{
    GetStringOptionalValue(json_object, JSONKeys::kID, &out_instance->id);
    return LoadConsumableTypeID(json_object, JSONKeys::kTypeID, &out_instance->type_id);
}

bool BaseDataLoader::LoadConsumableInstancesData(
    const nlohmann::json& json_parent,
    const std::string_view key_array,
    std::vector<ConsumableInstanceData>* out_consumables_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadConsumablesInstanceData";

    // Check for key, it is valid if it does not exist
    if (!json_parent.contains(key_array))
    {
        return true;
    }

    const auto& json_array = json_parent[key_array];
    if (!json_array.is_array())
    {
        LogErr("{} - json_parent[{}] = {} is not an array", method_name, key_array, json_parent.dump());
        return false;
    }

    // Read from the array
    for (const auto& json_array_value : json_array)
    {
        ConsumableInstanceData instance_data;
        if (!LoadConsumableInstanceData(json_array_value, &instance_data))
        {
            LogErr("{} - Can't parse item = {}, from array", method_name, json_array_value.dump());
            return false;
        }

        out_consumables_data->push_back(instance_data);
    }

    return true;
}

bool BaseDataLoader::LoadConsumableData(const nlohmann::json& json_object, ConsumableData* out_consumable_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadConsumableData";

    auto& type_id = out_consumable_data->type_id;
    if (!json_helper_.GetStringValue(json_object, "Name", &type_id.name))
    {
        return false;
    }
    if (!GetIntValue(json_object, "Stage", &type_id.stage))
    {
        return false;
    }
    if (!GetIntValue(json_object, "Tier", &out_consumable_data->tier))
    {
        return false;
    }

    SourceContextData context;
    context.combat_unit_ability_type = AbilityType::kInnate;
    context.Add(SourceContextType::kConsumable);

    // Read Abilities Array
    const bool can_parse_abilities = json_helper_.WalkArray(
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
                    "{} - Failed to extract object from array  key = \"{}\", index = {}",
                    method_name,
                    JSONKeys::kAbilities,
                    index);
                return false;
            }

            out_consumable_data->innate_abilities.emplace_back(std::move(ability));
            return true;
        });

    if (!can_parse_abilities)
    {
        LogErr("{} - Failed to extract parse Key = {} array", method_name, JSONKeys::kAbilities);
        return false;
    }

    return true;
}

}  // namespace simulation
