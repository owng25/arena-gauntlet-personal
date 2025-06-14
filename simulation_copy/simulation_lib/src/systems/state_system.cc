#include "systems/state_system.h"

#include "components/state_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "utility/entity_helper.h"

namespace simulation
{
void StateSystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kFainted>(this, &Self::OnFainted);
}

void StateSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // We are only interested in Combat Units
    if (!EntityHelper::IsACombatUnit(entity))
    {
        return;
    }

    // Get the state component
    auto& state_component = entity.Get<StateComponent>();
    if (!state_component.IsFainted())
    {
        // Only Fainted state requires further processing
        return;
    }

    const int disappear_time_steps = state_component.GetAndDecrementDisappearTimeSteps();

    // If time passed for fainted, disappear
    if (disappear_time_steps == 0)
    {
        state_component.SetState(CombatUnitState::kDisappeared);

        // Emit event
        world_->BuildAndEmitEvent<EventType::kCombatUnitStateChanged>(entity.GetID(), CombatUnitState::kDisappeared);
    }
}

void StateSystem::OnFainted(const event_data::Fainted& fainted_data)
{
    // We are only interested in Combat Units
    if (!EntityHelper::IsACombatUnit(*world_, fainted_data.entity_id))
    {
        return;
    }

    // Update fainted state and time step
    const auto& entity = world_->GetByID(fainted_data.entity_id);
    auto& state_component = entity.Get<StateComponent>();
    state_component.SetState(CombatUnitState::kFainted);

    // Emit event
    world_->BuildAndEmitEvent<EventType::kCombatUnitStateChanged>(entity.GetID(), CombatUnitState::kFainted);
}

}  // namespace simulation
