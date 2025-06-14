#pragma once

#include "data/drone_augment/drone_augment_data.h"
#include "data/drone_augment/drone_augment_id.h"
#include "data_container_base.h"

namespace simulation
{

class DroneAugmentsDataContainer
    : public DataContainerBase<DroneAugmentTypeID, DroneAugmentData, DroneAugmentTypeID::HashFunction>
{
public:
    DroneAugmentsDataContainer() = default;
    explicit DroneAugmentsDataContainer(const DataContainerBase& base) : DataContainerBase(base) {}
};

}  // namespace simulation
