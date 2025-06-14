#pragma once

#include <unordered_set>

#include "ecs/system.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * AugmentSystem
 *
 * Handles the augments attached to an entity
 * --------------------------------------------------------------------------------------------------------
 */
class AugmentSystem : public System
{
    typedef System Super;
    typedef AugmentSystem Self;

public:
    void Init(World* world) override;
    void PreBattleStarted(const Entity& entity) override;
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kAugment;
    }

private:
    // Helper function that adds the augment data to the entity
    void AddAugmentDataToEntity(const Entity& entity);

    // Keep track of entities that had called AddAugmentDataToEntity
    std::unordered_set<EntityID> entities_activated_;
};

}  // namespace simulation
