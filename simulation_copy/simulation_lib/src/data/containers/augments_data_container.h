#pragma once

#include "data/augment/augment_data.h"
#include "data_container_base.h"

namespace simulation
{

class AugmentsDataContainer : public DataContainerBase<AugmentTypeID, AugmentData, AugmentTypeID::HashFunction>
{
public:
    AugmentsDataContainer() = default;
    explicit AugmentsDataContainer(const DataContainerBase& base) : DataContainerBase(base) {}
};

}  // namespace simulation
