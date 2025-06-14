#include "base_data_loader.h"
#include "data/suit/suit_data.h"
#include "data/suit/suit_instance_data.h"
#include "data/suit/suit_type_id.h"
#include "json_keys.h"

namespace simulation
{

bool BaseDataLoader::LoadCombatSuitTypeID(
    const nlohmann::json& json_parent,
    const std::string_view key,
    CombatSuitTypeID* out_type_id) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadCombatSuitTypeID";
    return EnsureContainsObjectField(method_name, json_parent, key) &&
           LoadCombatSuitTypeID(json_parent[key], out_type_id);
}

bool BaseDataLoader::LoadCombatSuitTypeID(const nlohmann::json& json_object, CombatSuitTypeID* out_type_id) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadCombatSuitTypeID";

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

    return true;
}

// Loads the object at json_parent[key] into out_suit_instance
bool BaseDataLoader::LoadCombatSuitInstanceData(
    const nlohmann::json& json_parent,
    const std::string_view key,
    CombatSuitInstanceData* out_instance) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadCombatSuitInstanceData";
    return EnsureContainsObjectField(method_name, json_parent, key) &&
           LoadCombatSuitInstanceData(json_parent[key], out_instance);
}

bool BaseDataLoader::LoadCombatSuitInstanceData(const nlohmann::json& json_object, CombatSuitInstanceData* out_instance)
    const
{
    GetStringOptionalValue(json_object, JSONKeys::kID, &out_instance->id);
    return LoadCombatSuitTypeID(json_object, JSONKeys::kTypeID, &out_instance->type_id);
}

bool BaseDataLoader::LoadCombatSuitData(const nlohmann::json& json_object, CombatUnitSuitData* out_suit) const
{
    static constexpr std::string_view method_name = "LoadCombatUnitSuit";

    if (!LoadCombatSuitTypeID(json_object, &out_suit->type_id))
    {
        LogErr("{} - failed to load suit type id", method_name);
        return false;
    }

    if (!json_helper_.GetEnumValue(json_object, JSONKeys::kType, &out_suit->suit_type))
    {
        LogErr("Failed to read required field \"{}\".", JSONKeys::kType);
        return false;
    }

    if (!GetIntValue(json_object, JSONKeys::kTier, &out_suit->tier))
    {
        return false;
    }

    if (!json_object.contains(JSONKeys::kStats))
    {
        LogErr("{} - can't get Stats JSON Object", method_name);
        return false;
    }

    // Load suit stats
    std::unordered_set<StatType> read_stats_set;
    if (!LoadStats(json_object[JSONKeys::kStats], false, &out_suit->stats, &read_stats_set))
    {
        return false;
    }

    if (!EnsureSpecificStatsWereRead(kSuitStatTypes.data(), kSuitStatTypes.size(), read_stats_set))
    {
        return false;
    }

    if (json_object.contains(JSONKeys::kAbilities))
    {
        SourceContextData context;
        context.combat_unit_ability_type = AbilityType::kInnate;
        context.Add(SourceContextType::kSuit);

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

                out_suit->innate_abilities.push_back(std::move(ability));

                return true;
            });

        if (!abilities_ok)
        {
            LogErr("{} - Failed to read suit abilities from JSON.", method_name);
            return false;
        }
    }

    return true;
}
}  // namespace simulation
