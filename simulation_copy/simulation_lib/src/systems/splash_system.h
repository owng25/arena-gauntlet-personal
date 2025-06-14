#pragma once

#include "ecs/system.h"

namespace simulation
{
namespace event_data
{
struct ZoneCreated;
}
/* -------------------------------------------------------------------------------------------------------
 * SplashSystem
 *
 * Handles the active splash inside the world
 * --------------------------------------------------------------------------------------------------------
 */
class SplashSystem : public System
{
    typedef System Super;
    typedef SplashSystem Self;

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kSpawnable;
    }

private:
    // Listen to world events
    void OnZoneCreated(const event_data::ZoneCreated& data);
};

}  // namespace simulation
