#include "systems/displacement_system.h"

#include "ability_system.h"
#include "components/attached_effects_component.h"
#include "components/displacement_component.h"
#include "components/position_component.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "utility/grid_helper.h"

namespace simulation
{
void DisplacementSystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kDisplacementStopped>(this, &Self::OnDisplacementStopped);
}

void DisplacementSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    // Only apply to entities with these components
    if (!entity.Has<DisplacementComponent, PositionComponent, AttachedEffectsComponent>())
    {
        return;
    }

    // Get components
    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();

    // Check for active displacement
    if (!attached_effects_component.HasDisplacement())
    {
        return;
    }

    // Get the type and make sure it's not kNone
    const auto& displacement_component = entity.Get<DisplacementComponent>();
    const auto displacement_type = displacement_component.GetDisplacementType();
    if (displacement_type == EffectDisplacementType::kNone)
    {
        return;
    }

    // Nothing further to do for knock up
    if (displacement_type == EffectDisplacementType::kKnockUp)
    {
        return;
    }

    const auto effect_state = displacement_component.GetAttachedEffectState();
    if (!effect_state)
    {
        LogErr("Attempt to move displaced entity but attached effect state is not set or expired");
        return;
    }

    HexGridPosition target_position = displacement_component.GetDisplacementTargetPosition();
    if (displacement_type == EffectDisplacementType::kPull)
    {
        auto& sender = world_->GetByID(displacement_component.GetSenderID());
        target_position = sender.Get<PositionComponent>().GetPosition();
    }

    const auto& entity_position_component = entity.Get<PositionComponent>();
    const auto& distance_vector = (target_position - entity_position_component.GetPosition()).ToSubUnits();
    const int distance_sub_units = distance_vector.Length() + 1;  // Add 1 for potential rounding error
    const int remaining_time_steps = std::max(effect_state->duration_time_steps - effect_state->current_time_steps, 0);
    const int speed_sub_units =
        Time::MsToSubUnitsPerTimeStep(distance_sub_units, Time::TimeStepsToMs(remaining_time_steps));

    MoveToward(entity, target_position, speed_sub_units);
}

void DisplacementSystem::OnDisplacementStopped(const event_data::Displacement& event_data)
{
    const auto& receiver_entity = world_->GetByID(event_data.receiver_id);
    if (!receiver_entity.IsActive())
    {
        return;
    }

    // Get the type and make sure it's not kNone
    auto& position_component = receiver_entity.Get<PositionComponent>();

    // Clear reserved position
    if (position_component.HasReservedPosition())
    {
        position_component.ClearReservedPosition();
    }

    // Disable overlapable state
    if (position_component.IsOverlapable())
    {
        position_component.SetOverlapable(false);
    }

    // Execute displacement end skill if we have
    const auto& displacement_component = receiver_entity.Get<DisplacementComponent>();
    if (const auto& displacement_end_skill = displacement_component.GetDisplacementEndSkill())
    {
        auto& ability_system = world_->GetSystem<AbilitySystem>();
        ability_system.DeployInstantTargetedSkill(
            event_data.sender_id,
            event_data.receiver_id,
            displacement_end_skill,
            SourceContextType::kDisplacement);
    }
}

void DisplacementSystem::MoveToward(const Entity& entity, const HexGridPosition& position, const int sub_units_to_move)
{
    if (sub_units_to_move <= 0)
    {
        // assert(false);
        return;
    }

    // Get position component
    const auto& position_component = entity.Get<PositionComponent>();

    // Calculate vectors
    const HexGridPosition start = position_component.ToSubUnits();
    const HexGridPosition end = position.ToSubUnits();
    const HexGridPosition destination_vector = end - start;

    // Calculate direction
    const HexGridPosition destination_vector_normal = destination_vector.ToNormalizedAndScaled();

    // Calculate destination
    HexGridPosition movement_vector = (destination_vector_normal * sub_units_to_move) / kPrecisionFactor;

    // Don't move past the destination location
    if (movement_vector.Length() > destination_vector.Length())
    {
        movement_vector = destination_vector;
    }

    // Apply movement vector to entity
    MoveEntity(entity, movement_vector);
}

void DisplacementSystem::MoveEntity(const Entity& entity, const HexGridPosition& sub_unit_delta)
{
    if (sub_unit_delta.IsNull())
    {
        return;
    }

    // Get position component
    auto& position_component = entity.Get<PositionComponent>();

    // Create vectors
    const HexGridPosition unit_move_vector = sub_unit_delta.ToUnits();
    const HexGridPosition subunit_move_vector = sub_unit_delta.ToSubUnitsRemainder();
    HexGridPosition unit_position = position_component.GetPosition();
    HexGridPosition subunit_position = position_component.GetSubUnitPosition();

    // Update position
    unit_position += unit_move_vector;
    subunit_position += subunit_move_vector;

    // Shift excessive subunits to units
    GridHelper::ApplyExcessiveSubUnitsToUnits(&unit_position, &subunit_position);

    // Check collision if not overlapable
    if (!position_component.IsOverlapable())
    {
        // TODO(shchavinskyi): We should have a proper collision here, currently it's only end position check
        // We should finish displacement movement just right before collision

        // Can't move here if blocked
        const GridHelper& grid_helper = world_->GetGridHelper();
        grid_helper.BuildObstacles(&entity);
        if (grid_helper.HasObstacleAt(unit_position))
        {
            LogDebug(entity.GetID(), "- can't move further");
            return;
        }
    }

    // Update position component
    position_component.SetPosition(unit_position, subunit_position);

    // Log and emit Moved event
    LogDebug(
        entity.GetID(),
        "- move to position = {}, subunit = {}",
        position_component.GetPosition(),
        position_component.GetSubUnitPosition());

    world_->BuildAndEmitEvent<EventType::kMoved>(
        entity.GetID(),
        position_component.GetPosition(),
        std::list<HexGridPosition>{unit_position});
}

}  // namespace simulation
