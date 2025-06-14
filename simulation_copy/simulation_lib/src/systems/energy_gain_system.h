#pragma once

#include "ecs/system.h"

namespace simulation
{
namespace event_data
{
struct ApplyEnergyGain;
}
/* -------------------------------------------------------------------------------------------------------
 * EnergyGainSystem
 *
 * Handles energy gain events
 * --------------------------------------------------------------------------------------------------------
 */
class EnergyGainSystem : public System
{
    typedef System Super;
    typedef EnergyGainSystem Self;

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kEffect;
    }

private:
    // Apply the skill effects to the target
    void ApplyEnergyGain(const event_data::ApplyEnergyGain& energy_gain_data);
};

}  // namespace simulation
