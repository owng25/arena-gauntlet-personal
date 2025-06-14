#pragma once

#include <memory>

#include "data/ability_data.h"
#include "data/enums.h"
#include "drone_augment_id.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * DroneAugmentData
 *
 * This class holds drone augment data
 * --------------------------------------------------------------------------------------------------------
 */
class DroneAugmentData
{
public:
    // Create a new instance
    static std::shared_ptr<DroneAugmentData> Create()
    {
        return std::make_shared<DroneAugmentData>();
    }

    std::shared_ptr<DroneAugmentData> CreateDeepCopy() const
    {
        // Copy everything
        auto copy = std::make_shared<DroneAugmentData>(*this);

        // Create a copy of the abilities
        copy->innate_abilities = innate_abilities.CreateDeepCopy();

        return copy;
    }

    // Unique type ID of the drone augment
    DroneAugmentTypeID type_id{};

    // The type of drone augment
    DroneAugmentType drone_augment_type = DroneAugmentType::kNone;

    // Abilities that apply to board/team
    AbilitiesData innate_abilities;

    bool operator==(const DroneAugmentData& other) const
    {
        return type_id == other.type_id;
    }
    bool operator!=(const DroneAugmentData& other) const
    {
        return !(*this == other);
    }
};

}  // namespace simulation
