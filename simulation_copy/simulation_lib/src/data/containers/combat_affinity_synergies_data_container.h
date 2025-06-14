#pragma once

#include "data/synergy_data.h"
#include "data_container_base.h"

namespace simulation
{

class CombatAffinitySynergiesDataContainer : public DataContainerBase<CombatAffinity, SynergyData>
{
public:
    CombatAffinitySynergiesDataContainer() = default;
    explicit CombatAffinitySynergiesDataContainer(const DataContainerBase& base) : DataContainerBase(base) {}
};

}  // namespace simulation
