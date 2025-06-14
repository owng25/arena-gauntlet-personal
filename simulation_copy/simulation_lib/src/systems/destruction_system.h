#pragma once

#include "ecs/system.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * DestructionSystem
 *
 * Handles the destruction of entities with a DeferredDestructionComponent
 * --------------------------------------------------------------------------------------------------------
 */
class DestructionSystem : public System
{
public:
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kSpawnable;
    }
};

}  // namespace simulation
