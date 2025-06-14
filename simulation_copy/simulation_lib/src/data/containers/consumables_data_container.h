#pragma once

#include "data/consumable/consumable_data.h"
#include "data/consumable/consumable_instance_data.h"
#include "data/consumable/consumable_type_id.h"
#include "data_container_base.h"

namespace simulation
{

class ConsumablesDataContainer
    : public DataContainerBase<ConsumableTypeID, ConsumableData, ConsumableTypeID::HashFunction>
{
public:
    ConsumablesDataContainer() = default;
    explicit ConsumablesDataContainer(const DataContainerBase& base) : DataContainerBase(base) {}
};

}  // namespace simulation
