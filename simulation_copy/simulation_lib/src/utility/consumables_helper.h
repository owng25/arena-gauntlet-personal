#pragma once

#include "ecs/entity.h"
#include "utility/logger_consumer.h"

namespace simulation
{
class World;
class ConsumableInstanceData;

/* -------------------------------------------------------------------------------------------------------
 * ConsumableHelper
 *
 * This defines the helper functions for consumables
 * --------------------------------------------------------------------------------------------------------
 */
class ConsumableHelper final : public LoggerConsumer
{
public:
    ConsumableHelper() = default;
    explicit ConsumableHelper(World* world);

    //
    // LoggerConsumer interface
    //

    std::shared_ptr<Logger> GetLogger() const override;
    void BuildLogPrefixFor(const EntityID id, std::string* out_string) const override;

    int GetTimeStepCount() const override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kConsumable;
    }

    //
    // Own functions
    //

    // Can add any consumable to this entity?
    bool CanAddConsumable(const EntityID entity_id) const;
    bool CanAddConsumable(const Entity& entity) const;

    // Can remove any consumable from this entity?
    bool CanRemoveConsumable(const EntityID entity_id) const;
    bool CanRemoveConsumable(const Entity& entity) const;

    // Does the entity has this augment instance?
    bool HasConsumable(const EntityID entity_id, const ConsumableInstanceData& instance) const;
    bool HasConsumable(const Entity& entity, const ConsumableInstanceData& instance) const;

    // Apply consumable to an entity
    bool AddConsumable(const Entity& entity, const ConsumableInstanceData& instance) const;

    // Remove consumable from an entity
    bool RemoveConsumable(const Entity& entity, const ConsumableInstanceData& instance) const;

private:
    // Owner world of this helper
    World* world_ = nullptr;
};

}  // namespace simulation
