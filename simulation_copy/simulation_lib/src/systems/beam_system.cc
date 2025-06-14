#include "systems/beam_system.h"

#include "components/beam_component.h"
#include "components/deferred_destruction_component.h"
#include "components/filtering_component.h"
#include "components/focus_component.h"
#include "components/position_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "utility/entity_helper.h"
#include "utility/grid_helper.h"
#include "utility/intersection_helper.h"
#include "utility/math.h"
#include "utility/time.h"

namespace simulation
{
void BeamSystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kBeamActivated>(this, &Self::OnBeamActivated);
}

void BeamSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    // Get the entity ID
    const EntityID entity_id = entity.GetID();

    // We are only interested in beams
    if (!EntityHelper::IsABeam(*world_, entity_id))
    {
        return;
    }
    auto& beam_component = entity.Get<BeamComponent>();
    const auto& position_component = entity.Get<PositionComponent>();

    // Make sure it's not pending destruction
    const auto& destruction_component = entity.Get<DeferredDestructionComponent>();
    if (destruction_component.IsPendingDestruction())
    {
        return;
    }

    // Get and increment the time step count
    const int current_time_step_count = beam_component.GetAndIncrementTimeStepCount();

    // Check frequency and activate skill
    const int time_step_frequency = Time::MsToTimeSteps(beam_component.GetFrequencyMs());
    if (time_step_frequency == 0)
    {
        LogWarn(entity_id, "BeamSystem::TimeStep - frequency shouldn't be 0!");
    }

    // Update direction for homing beams
    if (beam_component.IsHoming())
    {
        // Don't update the beam to the desired target if this beam is blocked by something else
        if (!beam_component.IsBlockedByAnEntityID() ||
            beam_component.GetBlockedByEntityID() == beam_component.GetReceiverID())
        {
            SetUpdatedBeamLengthAndDirection(entity, beam_component.GetReceiverID());
        }
    }

    if (time_step_frequency && current_time_step_count % time_step_frequency == 0)
    {
        // Log and emit event
        LogDebug(
            entity_id,
            "BeamSystem::TimeStep - Time for the beam to activate, sender_id = {}, "
            "receiver_id = {}, beam_direction_degrees = {}, beam_world_length_sub_units = {}, "
            "beam_width_sub_units = {}, position = {}",
            beam_component.GetSenderID(),
            beam_component.GetReceiverID(),
            beam_component.GetDirectionDegrees(),
            beam_component.GetWorldLengthSubUnits(),
            beam_component.GetWidthSubUnits(),
            position_component.GetPosition());

        event_data::BeamActivated event_data;
        event_data.entity_id = entity_id;
        event_data.sender_id = beam_component.GetSenderID();
        event_data.receiver_id = beam_component.GetReceiverID();
        event_data.beam_direction_degrees = beam_component.GetDirectionDegrees();
        event_data.beam_world_length_sub_units = beam_component.GetWorldLengthSubUnits();
        event_data.beam_width_sub_units = beam_component.GetWidthSubUnits();
        event_data.world_position = world_->ToWorldPosition(position_component.GetPosition());

        world_->EmitEvent<EventType::kBeamActivated>(event_data);
    }
}

void BeamSystem::OnBeamActivated(const event_data::BeamActivated& data)
{
    const EntityID beam_id = data.entity_id;

    // We are only interested in beams
    if (!EntityHelper::IsABeam(*world_, beam_id))
    {
        return;
    }

    const auto& beam = world_->GetByID(beam_id);
    auto& beam_component = beam.Get<BeamComponent>();
    auto& filtering_component = beam.Get<FilteringComponent>();

    // Make sure it's not pending destruction
    const auto& destruction_component = beam.Get<DeferredDestructionComponent>();
    if (destruction_component.IsPendingDestruction())
    {
        return;
    }

    // Get position of the beam, which is also the sender position because it's not moving
    const auto& position_component = beam.Get<PositionComponent>();

    const BeamIntersectionCache beam_intersection_cache = IntersectionHelper::MakeBeamIntersectionCache(
        *world_,
        position_component.GetPosition(),
        beam_component.GetDirectionDegrees(),
        beam_component.GetWidthSubUnits(),
        beam_component.GetWorldLengthSubUnits());

    LogDebug(
        beam_id,
        "BeamSystem::OnBeamActivated - world_length_sub_units = {}, world_width_sub_units = {}, "
        "direction = {}",
        beam_component.GetWorldLengthSubUnits(),
        beam_component.GetWidthSubUnits(),
        beam_intersection_cache.direction_degrees);

    //
    // Check if we have collision with anything else
    //
    // std::vector<EntityID> collisions;
    std::vector<FoundEntityCollision> collisions;
    const EntityID beam_sender_id = beam_component.GetSenderID();

    for (const auto& other : world_->GetAll())
    {
        // Ignore collision with:
        // - self
        // - sender
        const EntityID other_id = other->GetID();
        if (beam_id == other_id || beam_sender_id == other_id)
        {
            continue;
        }

        // Skip entity if it is no longer active
        if (!EntityHelper::IsCollidable(*other))
        {
            continue;
        }

        // Already collided with this entity, ignore if apply once set
        if (beam_component.IsApplyOnce() && filtering_component.HasCollidedWith(other_id))
        {
            continue;
        }

        // Get position component
        const auto& other_position_component = other->Get<PositionComponent>();

        if (IntersectionHelper::DoesBeamIntersectEntity(
                *world_,
                beam_intersection_cache,
                other_position_component.GetPosition(),
                other_position_component.GetRadius()))
        {
            // If a collision is registered then calculate the distance between the collision and the sender of the beam
            // Consider the size of the entity using distance_from_center_sub_units
            const HexGridPosition vector_to_other =
                other_position_component.GetPosition() - position_component.GetPosition();

            // Consider size of entity
            const int size_of_entities =
                Math::UnitsToSubUnits(other_position_component.GetRadius() + position_component.GetRadius());
            const int distance = Math::UnitsToSubUnits(vector_to_other.Length()) - size_of_entities;
            FoundEntityCollision found_data;
            found_data.id = other_id;
            found_data.distance = distance;

            // Add the data to collisions
            collisions.emplace_back(found_data);
            LogDebug(beam_id, "Added collision, other_id = {},", other_id);
        }
    }

    // Sort the collisions by distance so we can stop the beam if blockable and the beam has hit the closest blocked
    // unit from BlockAllegiance
    std::sort(
        collisions.begin(),
        collisions.end(),
        [](const FoundEntityCollision& a, const FoundEntityCollision& b)
        {
            if (a.distance == b.distance)
            {
                return a.id < b.id;
            }
            return a.distance < b.distance;
        });

    beam_component.SetIsBlockedByEntityID(kInvalidEntityID);
    for (const auto& [collided_with, distance] : collisions)
    {
        const auto& other_entity = world_->GetByID(collided_with);

        // Found a collision
        LogDebug(beam_id, "| Found a collision with entity_id = {}", collided_with);

        // Mark as collided
        filtering_component.AddCollidedWith(collided_with);

        auto& focus_component = beam.Get<FocusComponent>();

        // Set the new focus with our collided with
        focus_component.SetFocus(world_->GetByIDPtr(collided_with));

        // Choose any ability and activate it in the same time step
        constexpr bool try_do_attack = true;
        constexpr bool try_do_omega = true;
        world_->BuildAndEmitEvent<EventType::kActivateAnyAbility>(beam_id, try_do_attack, try_do_omega);

        // Reset the focus
        focus_component.ResetFocus();

        // For blockable beams break from the collision loop once the blocking unit is hit
        if (beam_component.IsBlockable() && IsBlockableByBeam(beam, other_entity))
        {
            beam_component.SetIsBlockedByEntityID(collided_with);
            SetUpdatedBeamLengthAndDirection(beam, collided_with);
            break;
        }
    }
}

bool BeamSystem::IsBlockableByBeam(const Entity& beam, const Entity& entity) const
{
    const auto& beam_component = beam.Get<BeamComponent>();

    const bool are_allies = beam.IsAlliedWith(entity);
    switch (beam_component.GetBlockAllegiance())
    {
    case AllegianceType::kSelf:
    {
        if (entity.GetID() == beam.GetParentID())
        {
            return true;
        }
        break;
    }
    case AllegianceType::kAlly:
    {
        if (are_allies)
        {
            return true;
        }
        break;
    }
    case AllegianceType::kEnemy:
    {
        if (!are_allies)
        {
            return true;
        }
        break;
    }
    case AllegianceType::kAll:
    {
        return true;
    }
    default:
        break;
    }

    return false;
}

void BeamSystem::SetUpdatedBeamLengthAndDirection(const Entity& beam, const EntityID receiver_id) const
{
    auto& beam_component = beam.Get<BeamComponent>();
    const EntityID sender_id = beam_component.GetSenderID();
    const auto& position_component = beam.Get<PositionComponent>();
    const GridHelper& grid_helper = world_->GetGridHelper();

    // Get direction and length to the entity
    const int current_direction_degrees = beam_component.GetDirectionDegrees();
    const int current_length_sub_units = beam_component.GetWorldLengthSubUnits();

    // Calculate new beam direction
    // Get the angle from this position to the receiver position
    // Going counter-clockwise from where x > 0 and y = 0
    // This is the arctangent of the difference between the positions
    const int new_direction_degrees = grid_helper.GetAngle360Between(sender_id, receiver_id);

    // Calculate new length_sub_units
    const HexGridPosition sub_units_vector = grid_helper.GetSubUnitsVectorBetween(sender_id, receiver_id);
    const int new_length_sub_units = world_->ToWorldPosition(sub_units_vector).Length();

    // Update details and emit event if direction is different
    if (current_direction_degrees != new_direction_degrees || current_length_sub_units != new_length_sub_units)
    {
        beam_component.SetDirectionDegrees(new_direction_degrees);
        beam_component.SetWorldLengthSubUnits(new_length_sub_units);

        // Log and emit event
        LogDebug(
            beam.GetID(),
            "BeamSystem::TimeStep - Beam direction updated, direction_degrees = {}, "
            "length_sub_units = {}, sender_id = {}, receiver_id = {}",
            new_direction_degrees,
            new_length_sub_units,
            sender_id,
            receiver_id);

        event_data::BeamUpdated event_data;
        event_data.entity_id = beam.GetID();
        event_data.sender_id = beam_component.GetSenderID();
        event_data.receiver_id = receiver_id;
        event_data.beam_direction_degrees = beam_component.GetDirectionDegrees();
        event_data.beam_world_length_sub_units = beam_component.GetWorldLengthSubUnits();
        event_data.beam_width_sub_units = beam_component.GetWidthSubUnits();
        event_data.world_position = world_->ToWorldPosition(position_component.GetPosition());

        world_->EmitEvent<EventType::kBeamUpdated>(event_data);
    }
}

}  // namespace simulation
