#include "utility/consumables_helper.h"

#include "components/consumable_component.h"
#include "data/containers/game_data_container.h"
#include "utility/entity_helper.h"

namespace simulation
{
ConsumableHelper::ConsumableHelper(World* world) : world_(world) {}

std::shared_ptr<Logger> ConsumableHelper::GetLogger() const
{
    return world_->GetLogger();
}

void ConsumableHelper::BuildLogPrefixFor(const EntityID id, std::string* out) const
{
    world_->BuildLogPrefixFor(id, out);
}

int ConsumableHelper::GetTimeStepCount() const
{
    return world_->GetTimeStepCount();
}

bool ConsumableHelper::HasConsumable(const EntityID entity_id, const ConsumableInstanceData& instance) const
{
    if (!world_->HasEntity(entity_id))
    {
        return false;
    }

    const auto& entity = world_->GetByID(entity_id);
    return HasConsumable(entity, instance);
}

bool ConsumableHelper::HasConsumable(const Entity& entity, const ConsumableInstanceData& instance) const
{
    if (!entity.Has<ConsumableComponent()>())
    {
        return false;
    }

    const auto& consumable_component = entity.Get<ConsumableComponent>();
    return consumable_component.HasConsumable(instance);
}

bool ConsumableHelper::AddConsumable(const Entity& entity, const ConsumableInstanceData& instance) const
{
    const ConsumableTypeID& consumable_type_id = instance.type_id;
    if (!CanAddConsumable(entity))
    {
        LogErr(entity.GetID(), "ConsumableHelper::AddConsumable - can't apply consumable: {}", consumable_type_id);
        return false;
    }
    if (!world_->GetGameDataContainer().HasConsumableData(consumable_type_id))
    {
        LogErr(
            entity.GetID(),
            "ConsumableHelper::AddConsumable - Consumable data for id = {}, DOES NOT EXIST",
            consumable_type_id);
        return false;
    }

    // Add consumable
    auto& consumable_component = entity.Get<ConsumableComponent>();
    consumable_component.AddConsumable(instance);

    LogDebug(entity.GetID(), "ConsumableHelper::AddConsumable - Added consumable: {}", instance);
    return true;
}

bool ConsumableHelper::RemoveConsumable(const Entity& entity, const ConsumableInstanceData& instance) const
{
    const ConsumableTypeID& consumable_type_id = instance.type_id;
    if (!CanRemoveConsumable(entity))
    {
        LogErr(entity.GetID(), "ConsumableHelper::RemoveConsumable - can't remove consumable: {}", consumable_type_id);
        return false;
    }
    if (!world_->GetGameDataContainer().HasConsumableData(consumable_type_id))
    {
        LogErr(
            entity.GetID(),
            "ConsumableHelper::RemoveConsumable - Consumable data for id = {}, DOES NOT EXIST",
            consumable_type_id);
        return false;
    }

    // Remove consumable
    auto& consumable_component = entity.Get<ConsumableComponent>();
    consumable_component.RemoveConsumable(instance);

    LogDebug(entity.GetID(), "ConsumableHelper::RemoveConsumable - Removed consumable: {}", instance);
    return true;
}

bool ConsumableHelper::CanAddConsumable(const EntityID entity_id) const
{
    if (!world_->HasEntity(entity_id))
    {
        return false;
    }

    const auto& entity = world_->GetByID(entity_id);
    return CanAddConsumable(entity);
}

bool ConsumableHelper::CanAddConsumable(const Entity& entity) const
{
    const bool is_battle_started = world_->IsBattleStarted();
    if (is_battle_started)
    {
        LogErr(entity.GetID(), "ConsumableHelper::CanAddConsumable - failed to apply augment, battle already started");
        return false;
    }

    // Not active
    if (!entity.IsActive())
    {
        return false;
    }

    if (!entity.Has<ConsumableComponent>())
    {
        return false;
    }

    const auto& consumable_component = entity.Get<ConsumableComponent>();
    return consumable_component.GetEquippedConsumablesSize() < world_->GetConsumablesConfig().max_consumables;
}

bool ConsumableHelper::CanRemoveConsumable(const EntityID entity_id) const
{
    if (!world_->HasEntity(entity_id))
    {
        return false;
    }

    const auto& entity = world_->GetByID(entity_id);
    return CanRemoveConsumable(entity);
}

bool ConsumableHelper::CanRemoveConsumable(const Entity& entity) const
{
    const bool is_battle_started = world_->IsBattleStarted();
    if (is_battle_started)
    {
        LogErr(
            entity.GetID(),
            "ConsumableHelper::CanRemoveConsumable - failed to apply consumable, battle already started");
        return false;
    }

    // Not active
    if (!entity.IsActive())
    {
        return false;
    }

    if (!entity.Has<ConsumableComponent>())
    {
        return false;
    }

    const auto& consumable_component = entity.Get<ConsumableComponent>();
    return consumable_component.GetEquippedConsumablesSize() > 0;
}

}  // namespace simulation
