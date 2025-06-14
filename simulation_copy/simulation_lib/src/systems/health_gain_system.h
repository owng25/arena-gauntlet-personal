#pragma once

#include "ecs/system.h"

namespace simulation
{
namespace event_data
{
struct ApplyHealthGain;
}
/* -------------------------------------------------------------------------------------------------------
 * HealthGainSystem
 *
 * Handles health gain events
 * --------------------------------------------------------------------------------------------------------
 */
class HealthGainSystem : public System
{
    typedef System Super;
    typedef HealthGainSystem Self;

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kEffect;
    }

private:
    // Apply the skill effects to the target
    void ApplyHealthGain(const event_data::ApplyHealthGain& health_gain_data);
};

}  // namespace simulation
