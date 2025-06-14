#include "consumable_system.h"

#include "components/abilities_component.h"
#include "components/consumable_component.h"
#include "components/stats_component.h"
#include "data/containers/game_data_container.h"
#include "ecs/world.h"
#include "utility/augment_helper.h"

namespace simulation
{
void ConsumableSystem::Init(World* world)
{
    System::Init(world);
}

void ConsumableSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    AddConsumablesDataToEntity(entity);
}

void ConsumableSystem::PreBattleStarted(const Entity& entity)
{
    AddConsumablesDataToEntity(entity);
}

void ConsumableSystem::AddConsumablesDataToEntity(const Entity& entity)
{
    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    // Add innate abilities from augments
    if (!entity.Has<StatsComponent, ConsumableComponent>())
    {
        return;
    }

    auto& abilities_component = entity.Get<AbilitiesComponent>();
    auto& consumable_component = entity.Get<ConsumableComponent>();

    // Has to have a valid consumable
    if (consumable_component.GetEquippedConsumablesSize() == 0)
    {
        return;
    }

    // Once per entity
    const EntityID entity_id = entity.GetID();
    if (entities_activated_.count(entity_id) > 0)
    {
        return;
    }
    entities_activated_.insert(entity_id);

    static constexpr std::string_view method_name = "ConsumableSystem::AddConsumablesDataToEntity";

    LogDebug(entity_id, "{}", method_name);

    const auto& game_data_container = world_->GetGameDataContainer();
    const auto& equipped_consumables = consumable_component.GetEquippedConsumables();
    for (const auto& equipped_consumable : equipped_consumables)
    {
        const auto consumable_data = game_data_container.GetConsumableData(equipped_consumable.type_id);

        if (!consumable_data)
        {
            LogErr(
                entity_id,
                "{} - Can't find consumbale data. type_id = {}",
                method_name,
                equipped_consumable.type_id);
            continue;
        }

        // Add the innate
        LogDebug(entity_id, "{} - Adding consumable: {}", method_name, equipped_consumable);

        abilities_component.AddDataInnateAbilities(consumable_data->innate_abilities);
    }
}

}  // namespace simulation
