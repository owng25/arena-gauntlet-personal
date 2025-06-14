#include "base_data_loader.h"
#include "data/hyper_config.h"
#include "utility/json_helper.h"

namespace simulation
{
bool BaseDataLoader::LoadHyperData(const nlohmann::json& json_object, HyperData* out_hyper_data) const
{
    assert(out_hyper_data);
    if (!LoadHyperCombatAffinity(json_object, &out_hyper_data->combat_affinity_opponents))
    {
        LogErr("HyperDataLoader::LoadFromJSON - Can't load combat affinity");
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadHyperCombatAffinity(
    const nlohmann::json& json_object,
    CombatAffinityOpponentsMap* out_combat_affinity_hyper_data) const
{
    static constexpr std::string_view key_combat_affinity_counter = "CombatAffinityCounter";
    if (!json_object.contains(key_combat_affinity_counter))
    {
        LogErr(
            "HyperDataLoader::LoadHyperCombatAffinityFromJSON - JSON doesn't contain {}",
            key_combat_affinity_counter);
        return false;
    }
    const nlohmann::json& json_object_combat_affinity_counter = json_object[key_combat_affinity_counter];

    // Get combat affinity types
    std::vector<std::string> combat_affinity_types;
    if (!json_helper_
             .GetStringArray(json_object_combat_affinity_counter, "CombatAffinityTypes", &combat_affinity_types))
    {
        return false;
    }

    // Validate combat affinity types
    for (const std::string& combat_affinity_type : combat_affinity_types)
    {
        const CombatAffinity combat_affinity = Enum::StringToCombatAffinity(combat_affinity_type);
        if (combat_affinity == CombatAffinity::kNone)
        {
            LogErr("HyperDataLoader::LoadHyperCombatAffinityFromJSON - Invalid combat affinity type");
            return false;
        }
    }

    static constexpr std::string_view key_combat_affinities = "CombatAffinities";
    if (!json_object_combat_affinity_counter.contains(key_combat_affinities))
    {
        LogErr("HyperDataLoader::LoadHyperCombatAffinityFromJSON - JSON doesn't contain CombatAffinity");
        return false;
    }

    // Go through the whole json elements
    for (const nlohmann::json& combat_affinity_element : json_object_combat_affinity_counter[key_combat_affinities])
    {
        std::string current_type = "";
        if (!json_helper_.GetStringValue(combat_affinity_element, "Name", &current_type))
        {
            return false;
        }

        // Get effectiveness values and store them in Hyper Data
        std::unordered_map<CombatAffinity, int> combat_affinity_effectiveness;
        for (const std::string& effectiveness_types : combat_affinity_types)
        {
            static constexpr std::string_view key_effectiveness = "Effectiveness";
            if (!combat_affinity_element.contains(key_effectiveness))
            {
                LogErr("HyperDataLoader::LoadHyperCombatAffinityFromJSON - doesn't contain {}", key_effectiveness);
                return false;
            }

            int value = 0;
            if (!json_helper_.GetIntValue(combat_affinity_element[key_effectiveness], effectiveness_types, &value))
            {
                return false;
            }
            combat_affinity_effectiveness.emplace(Enum::StringToCombatAffinity(effectiveness_types), value);
        }

        // According to GDD sum of effectiveness must be 0
        // If not there is a problem and the JSON needs to be modified
        int combat_affinity_counter_sum = 0;
        for (const auto& values : combat_affinity_effectiveness)
        {
            combat_affinity_counter_sum += values.second;
        }

        if (combat_affinity_counter_sum == 0)
        {
            const CombatAffinity combat_affinity = Enum::StringToCombatAffinity(current_type);
            out_combat_affinity_hyper_data->emplace(combat_affinity, combat_affinity_effectiveness);
        }
        else
        {
            LogErr(
                "HyperDataLoader::LoadHyperCombatAffinityFromJSON - Invalid Sum for {}, Sum:{} "
                "instead of 0",
                current_type,
                combat_affinity_counter_sum);

            return false;
        }
    }

    return true;
}

}  // namespace simulation
