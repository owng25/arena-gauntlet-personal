#pragma once

#include "data/suit/suit_data.h"
#include "data_container_base.h"

namespace simulation
{

class SuitsDataContainer
    : public DataContainerBase<CombatSuitTypeID, CombatUnitSuitData, CombatSuitTypeID::HashFunction>
{
public:
    SuitsDataContainer() = default;
    explicit SuitsDataContainer(const DataContainerBase& base) : DataContainerBase(base) {}
};

}  // namespace simulation
