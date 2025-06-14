#include "systems/destruction_system.h"

#include "components/abilities_component.h"
#include "components/beam_component.h"
#include "components/deferred_destruction_component.h"
#include "components/duration_component.h"
#include "components/mark_component.h"
#include "components/shield_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "utility/entity_helper.h"

namespace simulation
{
void DestructionSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    // Get the entity ID
    const EntityID entity_id = entity.GetID();

    // The rest of this code is not valid for entities without destruction components
    if (!entity.Has<DeferredDestructionComponent>())
    {
        return;
    }

    // Check pending destruction
    auto& destruction_component = entity.Get<DeferredDestructionComponent>();
    if (destruction_component.IsPendingDestruction())
    {
        bool safe_to_destroy = true;

        // Make sure this attached entity doesn't have an active ability
        if (entity.Has<AbilitiesComponent>())
        {
            const auto& abilities = entity.Get<AbilitiesComponent>();
            if (abilities.HasActiveAbility())
            {
                safe_to_destroy = false;
            }
        }

        if (safe_to_destroy)
        {
            if (EntityHelper::IsAShield(*world_, entity_id))
            {
                // Destroy the shield
                LogDebug(entity_id, "DestructionSystem::TimeStep - Destroying shield as it is pending destruction");
                world_->BuildAndEmitEvent<EventType::kMarkAttachedEntityAsDestroyed>(entity_id);
            }
            else if (EntityHelper::IsAMark(*world_, entity_id))
            {
                // Destroy the shield
                LogDebug(entity_id, "DestructionSystem::TimeStep - Destroying mark as it is pending destruction");
                world_->BuildAndEmitEvent<EventType::kMarkAttachedEntityAsDestroyed>(entity_id);
            }
            else if (EntityHelper::IsAZone(*world_, entity_id))
            {
                LogDebug(entity_id, "DestructionSystem::TimeStep - Destroying zone as it is pending destruction");
                world_->BuildAndEmitEvent<EventType::kMarkZoneAsDestroyed>(entity_id);
            }
            else if (EntityHelper::IsABeam(*world_, entity_id))
            {
                LogDebug(entity_id, "DestructionSystem::TimeStep - Destroying beam as it is pending destruction");
                world_->BuildAndEmitEvent<EventType::kMarkBeamAsDestroyed>(entity_id);
            }
        }

        // No need to continue checking
        return;
    }

    // Handle expiry for entities with duration
    if (entity.Has<DurationComponent>())
    {
        // Get and decrement the remaining time steps
        auto& duration_component = entity.Get<DurationComponent>();
        const int remaining_time_step_count = duration_component.GetAndDecrementRemainingTimeSteps();

        // Check for expiration
        if (remaining_time_step_count == 0)
        {
            // Let it be destroyed
            destruction_component.SetPendingDestruction(DestructionReason::kExpired);

            LogDebug(entity_id, "DestructionSystem::TimeStep - Entity expired");
            if (EntityHelper::IsAShield(*world_, entity_id))
            {
                const auto& shield_component = entity.Get<ShieldComponent>();

                // Send expired event
                world_->BuildAndEmitEvent<EventType::kShieldExpired>(
                    entity_id,
                    shield_component.GetSenderID(),
                    shield_component.GetReceiverID());

                // also send pending destroyed even
                world_->BuildAndEmitEvent<EventType::kShieldPendingDestroyed>(
                    entity_id,
                    shield_component.GetSenderID(),
                    shield_component.GetReceiverID());
            }
            else if (EntityHelper::IsAMark(*world_, entity_id))
            {
                const auto& mark_component = entity.Get<MarkComponent>();

                // Emit event to destroy mark
                world_->BuildAndEmitEvent<EventType::kMarkDestroyed>(
                    entity.GetID(),
                    mark_component.GetSenderID(),
                    mark_component.GetReceiverID());
            }

            // Done processing
            return;
        }
    }

    // Handle interruptions for beams
    if (entity.Has<BeamComponent>())
    {
        // Get component and retrieve sender
        const auto& beam_component = entity.Get<BeamComponent>();
        const EntityID beam_sender_id = beam_component.GetSenderID();
        const auto& beam_sender = world_->GetByID(beam_sender_id);

        // Sender no longer active
        if (!beam_sender.IsActive())
        {
            LogDebug(
                entity_id,
                "DestructionSystem::TimeStep - Beam expired because the sender_id = {} is no "
                "longer active",
                beam_sender_id);
            destruction_component.SetPendingDestruction(DestructionReason::kExpired);
            return;
        }

        // Get the active skill
        const auto& abilities_component = beam_sender.Get<AbilitiesComponent>();
        const auto* skill_state = abilities_component.GetActiveSkill();

        // Destroy if skill is no longer deploying, or if this beam is orphaned
        if (skill_state == nullptr || skill_state->state != SkillStateType::kChanneling)
        {
            LogDebug(
                entity_id,
                "DestructionSystem::TimeStep - Beam expired. Either skill is orphaned or skill is "
                "no "
                "longer deploying");
            destruction_component.SetPendingDestruction(DestructionReason::kExpired);
        }
    }
}

}  // namespace simulation
