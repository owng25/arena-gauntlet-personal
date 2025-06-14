#pragma once

#include <vector>

#include "data/ability_data.h"
#include "data/encounter_mod_type_id.h"

namespace simulation
{

// Contains encounter mod data
class EncounterModData
{
public:
    std::shared_ptr<EncounterModData> CreateDeepCopy() const
    {
        // Copy everything
        auto copy = std::make_shared<EncounterModData>(*this);

        // Create a copy of the abilities
        copy->innate_abilities.clear();
        for (const std::shared_ptr<const AbilityData>& ability_data : innate_abilities)
        {
            copy->innate_abilities.emplace_back(ability_data->CreateDeepCopy());
        }

        return copy;
    }

    bool operator==(const EncounterModData& other) const
    {
        return type_id == other.type_id;
    }

    bool operator!=(const EncounterModData& other) const
    {
        return !(*this == other);
    }

    // Unique id of this encounter
    EncounterModTypeID type_id{};

    // Innate abilities for entire team
    std::vector<std::shared_ptr<const AbilityData>> innate_abilities{};
};
}  // namespace simulation
