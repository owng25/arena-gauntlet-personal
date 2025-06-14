#pragma once

#include "data/combat_unit_data.h"
#include "data_container_base.h"

namespace simulation
{

class CombatUnitsDataContainer
    : public DataContainerBase<CombatUnitTypeID, CombatUnitData, CombatUnitTypeID::HashFunction>
{
public:
    CombatUnitsDataContainer() = default;
    explicit CombatUnitsDataContainer(const DataContainerBase& base) : DataContainerBase(base) {}
};

}  // namespace simulation
