#pragma once

#include "ecs/system.h"
#include "utility/enum_map.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * OverloadSystem
 *
 * Handles Overload System
 * https://illuvium.atlassian.net/wiki/spaces/AB/pages/236783470/Overload
 * --------------------------------------------------------------------------------------------------------
 */
class OverloadSystem : public System
{
    typedef System Super;
    typedef OverloadSystem Self;

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kOverload;
    }

private:
    void ApplyOverloadDamage(const Entity& entity);
    void UpdateTeamUnitsCount();

private:
    // Key: Team
    // Value: Active combat units count at the current time step
    EnumMap<Team, int> team_units_count_;

    // Last time step number when team_units_count_ was updated
    int team_units_count_last_update_ = -1;
};

}  // namespace simulation
