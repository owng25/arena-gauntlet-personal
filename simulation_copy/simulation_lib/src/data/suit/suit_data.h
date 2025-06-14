#pragma once

#include "data/ability_data.h"
#include "data/stats_data.h"
#include "data/suit/suit_type_id.h"
#include "utility/custom_formatter.h"

namespace simulation
{

// Holds the data for a combat unit suit
class CombatUnitSuitData
{
public:
    std::shared_ptr<CombatUnitSuitData> CreateDeepCopy() const
    {
        // Copy everything
        auto copy = std::make_shared<CombatUnitSuitData>(*this);

        // Create a copy of the abilities
        copy->innate_abilities.clear();
        for (const std::shared_ptr<const AbilityData>& ability_data : innate_abilities)
        {
            copy->innate_abilities.emplace_back(ability_data->CreateDeepCopy());
        }

        return copy;
    }

    // Is this item considered valid
    bool IsValid() const
    {
        return type_id.IsValid();
    }

    void FormatTo(fmt::format_context& ctx) const;

    // Unique identifier for this combat suit
    CombatSuitTypeID type_id{};

    // The type of suit
    SuitType suit_type = SuitType::kNone;

    // Stats for equipped suit
    StatsData stats{};

    // The tier of the item
    int tier = 0;

    // Abilities that extending illuvial
    std::vector<std::shared_ptr<const AbilityData>> innate_abilities;
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(CombatUnitSuitData, FormatTo);
