#include "base_data_loader.h"
#include "data/ability_data.h"
#include "data/encounter_mod_data.h"
#include "data/enums.h"
#include "data/loaders/json_keys.h"

namespace simulation
{
bool BaseDataLoader::LoadEncounterModData(const nlohmann::json& json_object, EncounterModData* out_encounter_mod_data)
    const
{
    static constexpr std::string_view method_name = "EncounterModDataLoader::LoadFromJSON";

    if (!LoadEncounterModTypeID(json_object, &out_encounter_mod_data->type_id))
    {
        LogErr("{} - Failed to parse encounter mod type id", method_name);
        return false;
    }

    // Source context for loaded abilities
    SourceContextData context;
    context.combat_unit_ability_type = AbilityType::kInnate;
    context.Add(SourceContextType::kEncounterMod);

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

            out_encounter_mod_data->innate_abilities.emplace_back(std::move(ability));
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
