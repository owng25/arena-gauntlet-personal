#pragma once

#include <unordered_set>

#include "ecs/system.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * ConsumableSystem
 *
 * Handles the consumables attached to an entity
 * --------------------------------------------------------------------------------------------------------
 */
class ConsumableSystem : public System
{
    typedef System Super;
    typedef ConsumableSystem Self;

public:
    void Init(World* world) override;
    void PreBattleStarted(const Entity& entity) override;
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kConsumable;
    }

private:
    // Helper function that adds the consumables data to the entity
    void AddConsumablesDataToEntity(const Entity& entity);

    // Keep track of entities that had called AddConsumablesDataToEntity
    std::unordered_set<EntityID> entities_activated_;
};

}  // namespace simulation
