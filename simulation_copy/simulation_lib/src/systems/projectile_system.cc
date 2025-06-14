#include "systems/projectile_system.h"

#include "components/filtering_component.h"
#include "components/focus_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/projectile_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "utility/entity_helper.h"

namespace simulation
{
void ProjectileSystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kMovedStep>(this, &Self::OnEntityMovedStep);
    world_->SubscribeMethodToEvent<EventType::kAbilityDeactivated>(this, &Self::OnAbilityDeactivated);
    world_->SubscribeMethodToEvent<EventType::kFocusNeverDeactivated>(this, &Self::OnFocusNeverDeactivated);
    world_->SubscribeMethodToEvent<EventType::kEffectPackageReceived>(this, &Self::OnEffectPackageReceived);
}

void ProjectileSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    if (EntityHelper::IsAProjectile(entity))
    {
        HandleTargetPositionCheck(entity);
    }
}

void ProjectileSystem::OnEntityMovedStep(const event_data::Moved& data)
{
    // We are only interested in projectiles
    if (!EntityHelper::IsAProjectile(*world_, data.entity_id))
    {
        return;
    }

    const EntityID projectile_id = data.entity_id;
    const auto& projectile = world_->GetByID(projectile_id);
    auto& projectile_component = projectile.Get<ProjectileComponent>();

    // Check collision if this projectile can be blocked by something else
    // Or if it is not blocked by apply all is enabled
    if (!projectile_component.CanCheckForCollisions())
    {
        HandleTargetPositionCheck(projectile);
        return;
    }

    //
    // Check if we have collision with anything else
    //
    const auto& position_component = projectile.Get<PositionComponent>();

    // Entities to avoid
    const EntityID sender_id = projectile_component.GetSenderID();

    // Deployment guidance
    const bool can_deploy_airborne = projectile_component.CanDeployToAirborne();
    const bool can_deploy_underground = projectile_component.CanDeployToUnderground();

    auto& filtering_component = projectile.Get<FilteringComponent>();

    collisions_.clear();
    for (const auto& other : world_->GetAll())
    {
        // TODO(vampy): Handle collision with ally
        // TODO(vampy): Check !is_homing
        // Ignore collision with:
        // - self
        // - source entity that shot the projectile
        // - target entity, as that is handled in normal time step anyways
        const EntityID other_id = other->GetID();
        if (sender_id == other_id || projectile_id == other_id)
        {
            continue;
        }

        // Can the entity be collided with
        if (!EntityHelper::IsCollidable(*other))
        {
            continue;
        }

        // Ignore allies
        if (projectile.IsAlliedWith(*other))
        {
            continue;
        }

        // Ignore mismatched guidance
        if (other->Has<AttachedEffectsComponent>())
        {
            const auto& attached_effects_component = other->Get<AttachedEffectsComponent>();
            const bool is_airborne = attached_effects_component.HasPlaneChange(EffectPlaneChange::kAirborne);
            const bool is_underground = attached_effects_component.HasPlaneChange(EffectPlaneChange::kUnderground);

            if (is_airborne && !can_deploy_airborne)
            {
                // Cannot be airborne
                continue;
            }
            if (is_underground && !can_deploy_underground)
            {
                // Cannot be underground
                continue;
            }
        }

        // Already collided with this entity, ignore
        if (filtering_component.HasCollidedWith(other_id))
        {
            continue;
        }

        // Other is not taking up space, ignore
        const auto& other_position_component = other->Get<PositionComponent>();

        // Found collision with other entity
        if (other_position_component.IsPositionsIntersecting(position_component))
        {
            collisions_.push_back(other_id);
            if (!projectile_component.ApplyToAll())
            {
                break;
            }
        }
    }

    // Found a collision
    for (const EntityID collided_with : collisions_)
    {
        LogDebug(
            projectile_id,
            "ProjectileSystem::OnEntityMovedStep - Found a collision with entity = {}",
            collided_with);

        // Mark as collided
        filtering_component.AddCollidedWith(collided_with);

        // Save the previous focus
        auto& focus_component = projectile.Get<FocusComponent>();
        const auto previous_focus = focus_component.GetFocus();

        // Set the new target with our collided with
        const auto& target_entity = world_->GetByIDPtr(collided_with);
        focus_component.SetFocus(target_entity);

        if (projectile_component.IsBlockable() && collided_with != projectile_component.GetReceiverID())
        {
            projectile_component.SetBlockedBy(collided_with);
        }

        // Choose any ability and activate it in the same time step
        constexpr bool can_do_attack = true;
        constexpr bool can_do_omega = true;
        world_->BuildAndEmitEvent<EventType::kActivateAnyAbility>(projectile_id, can_do_attack, can_do_omega);

        // Restore previous focus
        focus_component.SetFocus(previous_focus);
    }

    HandleTargetPositionCheck(projectile);
}

void ProjectileSystem::OnAbilityDeactivated(const event_data::AbilityDeactivated& data)
{
    static constexpr std::string_view method_name = "ProjectileSystem::OnAbilityDeactivated";

    // We are only interested in projectiles
    if (!EntityHelper::IsAProjectile(*world_, data.sender_id))
    {
        return;
    }

    const EntityID projectile_id = data.sender_id;
    const auto& projectile = world_->GetByID(projectile_id);
    const auto& projectile_component = projectile.Get<ProjectileComponent>();
    const auto& filtering_component = projectile.Get<FilteringComponent>();

    // Simple non blocking non apply projectile, simply destroy it
    if (!projectile_component.CanCheckForCollisions())
    {
        if (!projectile_component.ContinueAfterTarget())
        {
            LogDebug(
                projectile_id,
                "{} - Destroying projectile as we hit the target and continue_after_target is false",
                method_name);
            world_->BuildAndEmitEvent<EventType::kMarkProjectileAsDestroyed>(projectile_id);
        }
    }

    // Blockable projectile, after first hit, destroy it even if we did not reach the target
    else if (projectile_component.IsBlockable())
    {
        LogDebug(
            projectile_id,
            "{} - Destroying projectile as the projectile is blockable and it hit something (could be the target)",
            method_name);

        const EntityID blocked_by = projectile_component.GetBlockedBy();
        if (blocked_by != kInvalidEntityID)
        {
            event_data::ProjectileBlocked blocked_event{};
            blocked_event.projectile_entity_id = projectile_id;
            blocked_event.blocked_by_entity_id = blocked_by;
            world_->EmitEvent<EventType::kProjectileBlocked>(blocked_event);
        }

        world_->BuildAndEmitEvent<EventType::kMarkProjectileAsDestroyed>(projectile_id);
    }

    // Apply to all (homing)
    else if (projectile_component.ApplyToAll() && projectile_component.IsHoming())
    {
        // Only destroy this if the projectile is original receiver id
        if (filtering_component.HasCollidedWith(projectile_component.GetReceiverID()))
        {
            LogDebug(
                projectile_id,
                "{} - projectile with apply_all enabled, destroying as it did reach the target",
                method_name);
            world_->BuildAndEmitEvent<EventType::kMarkProjectileAsDestroyed>(projectile_id);
        }
        else
        {
            LogDebug(
                projectile_id,
                "{} - projectile with apply_al enabled, NOT destroying as it did not reach the target",
                method_name);
        }
    }
    // Non-homing projectile
    else if (projectile.IsActive() && !projectile_component.IsHoming())
    {
        if (projectile_component.ApplyToAll())
        {
            if (!projectile_component.ContinueAfterTarget() &&
                filtering_component.HasOldTarget(projectile_component.GetReceiverID()))
            {
                // Non-homing projectile deactivated
                LogDebug(
                    projectile_id,
                    "{} - destroying non-homing projectile with apply_to_all enabled as it reached the target",
                    method_name);
                world_->BuildAndEmitEvent<EventType::kMarkProjectileAsDestroyed>(projectile_id);
            }
        }
        else
        {
            // Non-homing projectile deactivated
            LogDebug(
                projectile_id,
                "{} - projectile with homing disabled, destroying as it was deactivated",
                method_name);
            world_->BuildAndEmitEvent<EventType::kMarkProjectileAsDestroyed>(projectile_id);
        }
    }
}

void ProjectileSystem::OnEffectPackageReceived(const event_data::EffectPackage& data)
{
    const EntityID sender_id = data.sender_id;

    // We are only interested in projectiles
    if (!EntityHelper::IsAProjectile(*world_, sender_id))
    {
        return;
    }

    // Mark collision if projectile hit someone
    const auto& projectile = world_->GetByID(sender_id);
    auto& filtering_component = projectile.Get<FilteringComponent>();
    filtering_component.AddCollidedWith(data.receiver_id);
}

void ProjectileSystem::HandleTargetPositionCheck(const Entity& entity)
{
    assert(EntityHelper::IsAProjectile(entity) && entity.Has<MovementComponent>() && entity.Has<PositionComponent>());

    const auto& projectile_component = entity.Get<ProjectileComponent>();

    // Non-homing projectile reached initial target without hitting something
    if (!projectile_component.IsHoming())
    {
        const auto& movement_component = entity.Get<MovementComponent>();
        const auto& position_component = entity.Get<PositionComponent>();

        // TODO: This check will not work if we have a higher speed and can pass though exact location in one time step
        if (position_component.GetPosition() == movement_component.GetMovementTargetPosition())
        {
            LogDebug(
                entity.GetID(),
                "ProjectileSystem::HandleTargetPositionCheck - projectile with homing disabled, "
                "destroying as it did reach the target");
            world_->BuildAndEmitEvent<EventType::kMarkProjectileAsDestroyed>(entity.GetID());
        }
    }
}

void ProjectileSystem::OnFocusNeverDeactivated(const event_data::Focus& data)
{
    const EntityID projectile_id = data.entity_id;

    // We are only interested in projectiles
    if (!EntityHelper::IsAProjectile(*world_, projectile_id))
    {
        return;
    }

    const auto& projectile_entity = world_->GetByID(projectile_id);
    auto& projectile_component = projectile_entity.Get<ProjectileComponent>();
    if (projectile_component.IsHoming())
    {
        const auto& target_entity = world_->GetByID(projectile_component.GetReceiverID());
        const auto& target_movement_component = target_entity.Get<PositionComponent>();
        auto& projectile_movement_component = projectile_entity.Get<MovementComponent>();

        // Make projectile Non-Homing to handle it's destruction
        projectile_component.SetIsHoming(false);
        projectile_movement_component.SetMovementType(MovementComponent::MovementType::kDirectPosition);

        // Set the target's last known position to destroy the projectile when it reaches it.
        projectile_movement_component.SetMovementTargetPosition(target_movement_component.GetPosition());
    }
}

}  // namespace simulation
