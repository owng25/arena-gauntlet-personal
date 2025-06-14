#pragma once

#include "data/constants.h"
#include "ecs/component.h"
#include "utility/time.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * DurationComponent
 *
 * This class holds details for entities with a limited duration
 * --------------------------------------------------------------------------------------------------------
 */
class DurationComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<DurationComponent>(*this);
    }

    // How long should the entity remain in play
    void SetDurationMs(const int duration_ms)
    {
        duration_ms_ = duration_ms;
        duration_time_steps_ = Time::MsToTimeSteps(duration_ms);
    }
    int GetDurationMs() const
    {
        return duration_ms_;
    }

    // Remaining time steps
    int GetAndDecrementRemainingTimeSteps()
    {
        return duration_time_steps_--;
    }

private:
    // Keep track of remaining time steps
    int duration_time_steps_ = kTimeInfinite;
    int duration_ms_ = kTimeInfinite;
};

}  // namespace simulation
