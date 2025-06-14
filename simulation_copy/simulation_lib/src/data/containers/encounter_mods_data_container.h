#pragma once

#include "data/encounter_mod_data.h"
#include "data_container_base.h"

namespace simulation
{

class EncounterModsDataContainer
    : public DataContainerBase<EncounterModTypeID, EncounterModData, EncounterModTypeID::HashFunction>
{
public:
    EncounterModsDataContainer() = default;
    explicit EncounterModsDataContainer(const DataContainerBase& base) : DataContainerBase(base) {}
};

}  // namespace simulation
