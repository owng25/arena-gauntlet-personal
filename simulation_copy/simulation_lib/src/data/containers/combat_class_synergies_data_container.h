#pragma once

#include "data/synergy_data.h"
#include "data_container_base.h"

namespace simulation
{

class CombatClassSynergiesDataContainer : public DataContainerBase<CombatClass, SynergyData>
{
public:
    CombatClassSynergiesDataContainer() = default;
    explicit CombatClassSynergiesDataContainer(const DataContainerBase& base) : DataContainerBase(base) {}
};

}  // namespace simulation
