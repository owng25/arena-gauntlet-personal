#pragma once

#include "ecs/system.h"

namespace simulation
{

namespace event_data
{
struct Hyperactive;
}
/* -------------------------------------------------------------------------------------------------------
 * HyperSystem
 *
 * Handles Hyper System
 * --------------------------------------------------------------------------------------------------------
 */
class HyperSystem : public System
{
    using Super = System;
    using Self = HyperSystem;

public:
    void PreBattleStarted(const Entity& entity) override;
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kHyper;
    }
};

}  // namespace simulation
