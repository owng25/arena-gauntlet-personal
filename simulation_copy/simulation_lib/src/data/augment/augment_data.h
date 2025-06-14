#pragma once

#include <memory>
#include <unordered_map>

#include "augment_type_id.h"
#include "data/ability_data.h"
#include "data/enums.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * AugmentData
 *
 * This class holds augment data
 * --------------------------------------------------------------------------------------------------------
 */
class AugmentData
{
public:
    // Create a new instance
    static std::shared_ptr<AugmentData> Create()
    {
        return std::make_shared<AugmentData>();
    }
    std::shared_ptr<AugmentData> CreateDeepCopy() const
    {
        // Copy everything
        auto copy = std::make_shared<AugmentData>(*this);

        // Create a copy of the abilities
        copy->innate_abilities.clear();
        for (const std::shared_ptr<const AbilityData>& ability_data : innate_abilities)
        {
            copy->innate_abilities.emplace_back(ability_data->CreateDeepCopy());
        }

        return copy;
    }

    // Unique type ID of the augment
    AugmentTypeID type_id{};

    // The type of augment
    AugmentType augment_type = AugmentType::kNone;

    // The tier of an augment
    int tier = 0;

    // Abilities that extending illuvial
    std::vector<std::shared_ptr<const AbilityData>> innate_abilities;

    // Affinities granted by this augment
    // Key: combat affinity
    // Value: number of stacks
    std::unordered_map<CombatAffinity, int> combat_affinities{};

    // Combat classes granted by this augment
    // Key: combat class
    // Value: number of stacks
    std::unordered_map<CombatClass, int> combat_classes{};

    bool operator==(const AugmentData& other) const
    {
        return type_id == other.type_id;
    }
    bool operator!=(const AugmentData& other) const
    {
        return !(*this == other);
    }
};

}  // namespace simulation
