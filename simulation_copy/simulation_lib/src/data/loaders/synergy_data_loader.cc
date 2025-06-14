#include "base_data_loader.h"
#include "data/combat_synergy_bonus.h"
#include "data/synergy_data.h"
#include "json_keys.h"
#include "utility/json_helper.h"

namespace simulation
{

static constexpr std::string_view key_variables = "Variables";

bool BaseDataLoader::LoadSynergyData(const nlohmann::json& json_object, SynergyData* out_synergy_data)
{
    if (json_object.contains(JSONKeys::kCombatClass) && json_object.contains(JSONKeys::kCombatAffinity))
    {
        LogErr(
            "BaseDataLoader::LoadSynergyDataFromJSON - Synergy Type data had both an 'CombatAffinity' and "
            "'CombatClass' defined");
        return false;
    }

    // Try both Combat Classes and Combat Affinities
    LoadCombatSynergies(
        json_object,
        JSONKeys::kCombatClass,
        "CombatClassComponents",
        true,
        &out_synergy_data->combat_synergy,
        &out_synergy_data->combat_synergy_components);
    LoadCombatSynergies(
        json_object,
        JSONKeys::kCombatAffinity,
        "CombatAffinityComponents",
        false,
        &out_synergy_data->combat_synergy,
        &out_synergy_data->combat_synergy_components);

    // Fail if neither CombatUnitClass nor Combat Affinity is set
    if (out_synergy_data->combat_synergy.IsEmpty())
    {
        LogErr(
            "BaseDataLoader::LoadSynergyDataFromJSON - Synergy Type data had neither a 'Combat Affinity' or 'Combat "
            "Class' defined");
        return false;
    }

    if (!json_helper_.GetIntArray(json_object, "SynergyThresholds", &out_synergy_data->synergy_thresholds))
    {
        return false;
    }
    if (!LoadSynergyThresholdsAbilities(json_object, out_synergy_data))
    {
        return false;
    }
    if (!LoadSynergyIntrinsicAbilities(json_object, out_synergy_data))
    {
        return false;
    }
    if (!LoadSynergyHyperAbilities(json_object, out_synergy_data))
    {
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadSynergyThresholdsAbilities(const nlohmann::json& json_object, SynergyData* out_synergy_data)
{
    static constexpr std::string_view key_synergy_thresholds_abilities = "SynergyThresholdsAbilities";
    static constexpr std::string_view method_name = "BaseDataLoader::LoadSynergyThresholdsAbilitiesFromJSON";

    assert(out_synergy_data);
    if (!json_object.contains(key_synergy_thresholds_abilities))
    {
        LogErr("{} - Key = `{}` does not exist", method_name, key_synergy_thresholds_abilities);
        return false;
    }

    // Walk over all
    const auto& json_object_threshold_abilities = json_object[key_synergy_thresholds_abilities];
    if (!json_object_threshold_abilities.is_object())
    {
        LogErr("{} - Key = `{}` is not an Object type.", method_name, key_synergy_thresholds_abilities);
        return false;
    }

    // TODO(vampy): Don't make this optional anymore in the future
    if (!json_object_threshold_abilities.contains(key_variables))
    {
        return true;
    }

    // Read Variables
    std::vector<std::unordered_map<std::string, int>> thresholds_variables;
    thresholds_variables.resize(out_synergy_data->synergy_thresholds.size());
    if (!LoadSynergyThresholdVariables(json_object_threshold_abilities, &thresholds_variables))
    {
        return false;
    }

    // Check that there are no variables atm.
    // This should not happen unless some sets them from outside (which is not the case now)
    assert(variables_.empty());

    // Clears variables map when object leaves the scope.
    // It's just easier than clearing it every time we do return.
    struct AutoClear
    {
        ~AutoClear()
        {
            target.clear();
        }
        std::unordered_map<std::string, int>& target;
    } auto_clear{variables_};

    // Read abilities
    for (size_t index = 0; index < out_synergy_data->synergy_thresholds.size(); index++)
    {
        const int threshold_value = out_synergy_data->synergy_thresholds[index];

        variables_ = thresholds_variables[index];

        if (!LoadSynergyTeamAbilities(json_object_threshold_abilities, threshold_value, out_synergy_data))
        {
            return false;
        }

        if (!LoadSynergyUnitAbilities(json_object_threshold_abilities, threshold_value, out_synergy_data))
        {
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadSynergyTeamAbilities(
    const nlohmann::json& json_object,
    const int threshold_value,
    SynergyData* out_synergy_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadSynergyTeamAbilities";
    static constexpr std::string_view key_team_abilities = "TeamAbilities";

    // Init context info
    SourceContextData context;
    context.combat_synergy = out_synergy_data->combat_synergy;
    context.Add(SourceContextType::kSynergy);

    // Read Abilities Array
    const bool abilities_ok = json_helper_.WalkArray(
        json_object,
        key_team_abilities,
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
                    key_team_abilities,
                    index);
                return false;
            }

            out_synergy_data->team_threshold_abilities[threshold_value].push_back(std::move(ability));
            return true;
        });

    if (!abilities_ok)
    {
        LogErr("{} - Failed to read Synergy Unit abilities from JSON.", method_name);
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadSynergyUnitAbilities(
    const nlohmann::json& json_object,
    const int threshold_value,
    SynergyData* out_synergy_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadSynergyUnitAbilities";
    static constexpr std::string_view key_unit_abilities = "UnitAbilities";

    // Init context info
    SourceContextData context;
    context.combat_synergy = out_synergy_data->combat_synergy;
    context.Add(SourceContextType::kSynergy);

    // Read Abilities Array
    const bool abilities_ok = json_helper_.WalkArray(
        json_object,
        key_unit_abilities,
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
                    key_unit_abilities,
                    index);
                return false;
            }

            out_synergy_data->unit_threshold_abilities[threshold_value].push_back(std::move(ability));
            return true;
        });

    if (!abilities_ok)
    {
        LogErr("{} - Failed to read Synergy Unit abilities from JSON.", method_name);
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadSynergyIntrinsicAbilities(const nlohmann::json& json_object, SynergyData* out_synergy_data)
    const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadSynergyIntrinsicAbilities";

    // Does not exist
    if (!json_object.contains(JSONKeys::kIntrinsicAbilities))
    {
        // Intrinsic abilities are optional, so return true when not exists
        return true;
    }

    // Has disable key?
    if (json_object.contains(JSONKeys::kDisableIntrinsicAbilitiesOnFirstThreshold))
    {
        json_helper_.GetBoolValue(
            json_object,
            JSONKeys::kDisableIntrinsicAbilitiesOnFirstThreshold,
            &out_synergy_data->disable_intrinsic_abilities_on_first_threshold);
    }

    // Add metadata info
    SourceContextData context;
    context.combat_synergy = out_synergy_data->combat_synergy;
    context.Add(SourceContextType::kSynergy);

    // Read Abilities Array
    const bool abilities_ok = json_helper_.WalkArray(
        json_object,
        JSONKeys::kIntrinsicAbilities,
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
                    JSONKeys::kIntrinsicAbilities,
                    index);
                return false;
            }

            out_synergy_data->intrinsic_abilities.push_back(std::move(ability));
            return true;
        });

    if (!abilities_ok)
    {
        LogErr("{} - Failed to read hyper abilities from JSON.", method_name);
        return false;
    }

    return true;
}

bool BaseDataLoader::LoadSynergyHyperAbilities(const nlohmann::json& json_object, SynergyData* out_synergy_data) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadSynergyHyperAbilities";
    if (json_object.contains(JSONKeys::kHyperAbilities))
    {
        // Add metadata info
        SourceContextData context;
        context.combat_synergy = out_synergy_data->combat_synergy;
        context.Add(SourceContextType::kHyper);

        // Read Abilities Array
        const bool abilities_ok = json_helper_.WalkArray(
            json_object,
            JSONKeys::kHyperAbilities,
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
                        "{} - Failed to extract object from array key =  `{}`, index = {}",
                        method_name,
                        JSONKeys::kHyperAbilities,
                        index);
                    return false;
                }

                out_synergy_data->hyper_abilities.push_back(std::move(ability));
                return true;
            });

        if (!abilities_ok)
        {
            LogErr("{} - Failed to read hyper abilities from JSON.", method_name);
            return false;
        }
    }

    return true;
}

bool BaseDataLoader::LoadSynergyThresholdVariables(
    const nlohmann::json& parent_json_object,
    std::vector<std::unordered_map<std::string, int>>* out_thresholds_variables) const
{
    static constexpr std::string_view method_name = "BaseDataLoader::LoadAllJSONVariablesFromJSON";
    static constexpr std::string_view key_values = "Values";

    if (!parent_json_object.contains(key_variables))
    {
        LogErr("{} - can't find key = `{}`", method_name, key_variables);
        return false;
    }

    // Walk over all variables
    const auto& json_variables_array = parent_json_object[key_variables];
    if (!json_variables_array.is_array())
    {
        LogErr("{} - key = `{}` is not an array type", method_name, key_variables);
        return false;
    }

    for (const auto& variable_json_object : json_variables_array)
    {
        // Read Name
        std::string variable_name;
        if (!json_helper_.GetStringValue(variable_json_object, JSONKeys::kName, &variable_name))
        {
            LogErr("{} - Can't parse Key  =`{}` inside `{}`", method_name, JSONKeys::kName, key_variables);
            continue;
        }

        if (variable_name.empty())
        {
            LogErr("{} - Key `{}` is empty inside `{}`", method_name, JSONKeys::kName, key_variables);
            continue;
        }

        // Ensure that this variable is unique
        bool multiple_times = false;
        for (auto& threshold_variables : *out_thresholds_variables)
        {
            if (threshold_variables.count(variable_name) > 0)
            {
                multiple_times = true;
                break;
            }
        }

        if (multiple_times)
        {
            LogErr("{} - Variable Name = `{}` is defined multiple times!", method_name, variable_name);
            continue;
        }

        // Read values
        std::vector<int> variable_values;
        if (!json_helper_.GetIntArray(variable_json_object, key_values, &variable_values))
        {
            LogErr("{} - Can't parse Key  =`{}` inside `{}`", method_name, key_values, key_variables);
            continue;
        }

        if (variable_values.size() != out_thresholds_variables->size())
        {
            LogErr(
                "{} - Key `{}` contains invalid number of values. {} declared by thresholds array but {} defined here.",
                method_name,
                key_values,
                out_thresholds_variables->size(),
                variable_values.size());
            continue;
        }

        // Add to map
        for (size_t threshold_index = 0; threshold_index != out_thresholds_variables->size(); ++threshold_index)
        {
            (*out_thresholds_variables)[threshold_index][variable_name] = variable_values[threshold_index];
        }
    }

    return true;
}

bool BaseDataLoader::LoadCombatSynergies(
    const nlohmann::json& json_object,
    std::string_view key,
    std::string_view components_key,
    const bool is_combat_class,
    CombatSynergyBonus* out_combat_synergy,
    std::vector<CombatSynergyBonus>* out_combat_synergy_components) const
{
    if (!json_object.contains(key))
    {
        return false;
    }

    if (is_combat_class)
    {
        // Combat class
        CombatClass combat_class = CombatClass::kNone;
        if (json_helper_.GetEnumValue(json_object, key, &combat_class))
        {
            *out_combat_synergy = combat_class;
        }
    }
    else
    {
        // Combat Affinity
        CombatAffinity combat_affinity = CombatAffinity::kNone;
        if (json_helper_.GetEnumValue(json_object, key, &combat_affinity))
        {
            *out_combat_synergy = combat_affinity;
        }
    }

    // Has no data, return
    if (out_combat_synergy->IsEmpty())
    {
        LogErr(
            "BaseDataLoader::LoadCombatSynergiesFromJSON failed to load key = {}, components_key = {}, "
            "is_combat_class = {}",
            key,
            components_key,
            is_combat_class);
        return false;
    }

    // Read components
    return LoadCombatSynergyComponents(json_object, components_key, is_combat_class, out_combat_synergy_components);
}

bool BaseDataLoader::LoadCombatSynergyComponents(
    const nlohmann::json& json_object,
    std::string_view components_key,
    const bool is_combat_class,
    std::vector<CombatSynergyBonus>* out_combat_synergy_components) const
{
    assert(out_combat_synergy_components);

    // Read the to String Values of the array
    std::vector<std::string> combat_string_synergies;
    if (!json_object.contains(components_key))
    {
        LogErr("BaseDataLoader::LoadArrayCombatSynergiesAny - Failed to extract Array of Components");
        return false;
    }
    if (!json_helper_.GetStringArray(json_object, components_key, &combat_string_synergies))
    {
        return false;
    }

    // Iterate through the vector of combat synergies and transform them to enums
    for (const std::string& component_string : combat_string_synergies)
    {
        CombatSynergyBonus combat_synergy;
        if (is_combat_class)
        {
            // Combat Class
            const CombatClass combat_class = Enum::StringToCombatClass(component_string);
            combat_synergy = combat_class;
        }
        else
        {
            // Combat Affinity
            const CombatAffinity combat_affinity = Enum::StringToCombatAffinity(component_string);
            combat_synergy = combat_affinity;
        }

        // Add to the ouput vector
        out_combat_synergy_components->emplace_back(combat_synergy);
    }
    return true;
}

}  // namespace simulation
