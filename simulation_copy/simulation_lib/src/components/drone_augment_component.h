#pragma once

#include "data/drone_augment/drone_augment_id.h"
#include "ecs/component.h"

namespace simulation
{

/* -------------------------------------------------------------------------------------------------------
 * DroneAugmentEntityComponent
 *
 * Used to identify drone augment entity that is used for holding and executing drone augments abilities.
 * --------------------------------------------------------------------------------------------------------
 */
class DroneAugmentEntityComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<DroneAugmentEntityComponent>(*this);
    }

    void SetDroneAugmentTypeID(const DroneAugmentTypeID& type_id)
    {
        drone_augment_type_id_ = type_id;
    }

    const DroneAugmentTypeID& GetDroneAugmentTypeID() const
    {
        return drone_augment_type_id_;
    }

private:
    DroneAugmentTypeID drone_augment_type_id_;
};

}  // namespace simulation