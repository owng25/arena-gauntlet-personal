#pragma once

#include "consumable_type_id.h"
#include "data/ability_data.h"
#include "utility/custom_formatter.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * ConsumableData
 *
 * This class holds consumable data
 * --------------------------------------------------------------------------------------------------------
 */
class ConsumableData
{
public:
    std::shared_ptr<ConsumableData> CreateDeepCopy() const
    {
        // Copy everything
        auto copy = std::make_shared<ConsumableData>(*this);

        // Create a copy of the abilities
        copy->innate_abilities.clear();
        for (const std::shared_ptr<const AbilityData>& ability_data : innate_abilities)
        {
            copy->innate_abilities.emplace_back(ability_data->CreateDeepCopy());
        }

        return copy;
    }

    bool operator==(const ConsumableData& other) const
    {
        return type_id == other.type_id;
    }
    bool operator!=(const ConsumableData& other) const
    {
        return !(*this == other);
    }

    void FormatTo(fmt::format_context& ctx) const;

    // Unique type ID of the consumable
    ConsumableTypeID type_id{};

    // Abilities that extending illuvial
    std::vector<std::shared_ptr<const AbilityData>> innate_abilities{};

    // The tier of the consumable
    int tier = 0;
};
}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(ConsumableData, FormatTo);
