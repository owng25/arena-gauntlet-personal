#include "systems/decision_system.h"

#include "components/abilities_component.h"
#include "components/decision_component.h"
#include "components/focus_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/shield_component.h"
#include "components/stats_component.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "systems/focus_system.h"
#include "utility/entity_helper.h"
#include "utility/enum.h"
#include "utility/targeting_helper.h"

namespace simulation
{
void DecisionSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    // Only apply to entities with DecisionComponent
    if (!entity.Has<DecisionComponent>())
    {
        return;
    }
    // LogDebug(entity.GetID(), "DecisionSystem::TimeStep");

    // Get decision component
    auto& decision_component = entity.Get<DecisionComponent>();

    // Evaluate next decision and apply to entity
    const DecisionNextAction next_action = GetNextAction(entity);
    if (next_action != DecisionNextAction::kNone)
    {
        LogDebug(entity.GetID(), "DecisionSystem::TimeStep - action = {}", next_action);

        // If we've switched to finding focus, notify visualisation to stop movement
        if (decision_component.GetNextAction() != DecisionNextAction::kFindFocus &&
            next_action == DecisionNextAction::kFindFocus)
        {
            world_->BuildAndEmitEvent<EventType::kStoppedMovement>(entity.GetID());
        }

        // TODO(shchavinskyi) We are not changing next action to None,
        // So we can have OmegaAbility in DecisionComponent, but DecisionSystem returns None
        decision_component.SetNextAction(next_action);
    }
}

DecisionNextAction DecisionSystem::GetNextAction(const Entity& entity) const
{
    // Not active
    if (!entity.IsActive())
    {
        return DecisionNextAction::kNone;
    }

    // Only apply to entities with DecisionComponent and FocusComponent and PositionComponent
    if (!entity.Has<DecisionComponent, FocusComponent>())
    {
        return DecisionNextAction::kNone;
    }

    // Force move action when have active effect Flee or Charm
    if (EntityHelper::IsFleeing(entity) || EntityHelper::IsCharmed(entity))
    {
        return DecisionNextAction::kMovement;
    }

    if (EntityHelper::IsTaunted(entity))
    {
        return GetActionForTaunted(entity);
    }

    // If entity has abilities and one of them are active, do nothing else
    const auto& decision_component = entity.Get<DecisionComponent>();
    if (entity.Has<AbilitiesComponent>())
    {
        const auto& abilities_component = entity.Get<AbilitiesComponent>();
        if (abilities_component.HasActiveAbility())
        {
            // Special case for innate abilities, do an attack next
            if (decision_component.GetNextAction() == DecisionNextAction::kNone &&
                abilities_component.HasAnyAbility(AbilityType::kAttack) &&
                abilities_component.GetActiveAbilityType() == AbilityType::kInnate)
            {
                return DecisionNextAction::kAttackAbility;
            }

            // Special case for abilities with not locked movement
            // Just don't change decision while active
            if (!abilities_component.IsMovementLocked())
            {
                switch (abilities_component.GetActiveAbility()->ability_type)
                {
                case AbilityType::kOmega:
                    return DecisionNextAction::kOmegaAbility;
                case AbilityType::kAttack:
                    return DecisionNextAction::kAttackAbility;
                default:
                    break;
                }
            }

            // Do nothing else
            return DecisionNextAction::kNone;
        }
    }

    // Can do omega, activate
    // NOTE: that inside IsOmegaReady we also check for DoesOmegaRequireFocus, but if
    // is_omega_ready = true and does_omega_require_focus = true, and if the focus is not ok, we give it a chance to
    // find a new focus
    const bool is_omega_ready = IsOmegaReady(entity);
    const bool does_omega_require_focus = DoesOmegaRequireFocus(entity);
    if (is_omega_ready && !does_omega_require_focus)
    {
        return DecisionNextAction::kOmegaAbility;
    }

    // Get target component
    const auto& focus_component = entity.Get<FocusComponent>();

    // No target set (and active) or target does not have position, must find target
    if (!focus_component.IsFocusActive() || !focus_component.GetFocus()->Has<PositionComponent>())
    {
        // Do not find a new focus if refocus type is never, and we have already set a focus
        if (focus_component.GetRefocusType() != FocusComponent::RefocusType::kNever || !focus_component.IsFocusSet())
        {
            if (EntityHelper::IsAProjectile(entity) &&
                focus_component.GetSelectionType() == FocusComponent::SelectionType::kPredefined &&
                focus_component.GetRefocusType() == FocusComponent::RefocusType::kNever)
            {
                LogDebug(entity.GetID(), "DecisionSystem::GetNextAction - Predefined projectiles should not refocus");
                return DecisionNextAction::kMovement;
            }

            return DecisionNextAction::kFindFocus;
        }
    }

    if (focus_component.GetRefocusType() == FocusComponent::RefocusType::kAlways)
    {
        // Refocus if focus not targettable
        if (!EntityHelper::IsTargetable(*world_, focus_component.GetFocusID()))
        {
            return DecisionNextAction::kFindFocus;
        }

        // Refocus if targeting guidance does not match
        const TargetingHelper& targeting_helper = world_->GetTargetingHelper();
        const EnumSet<GuidanceType> targeting_guidance = targeting_helper.GetTargetingGuidanceForEntity(entity);
        if (!targeting_helper
                 .DoesEntityMatchesGuidance(targeting_guidance, entity.GetID(), focus_component.GetFocusID()))
        {
            return DecisionNextAction::kFindFocus;
        }
    }

    // Activate omega, focus already checked
    // NOTE: We give a change to find a new focus if that is required
    if (is_omega_ready)
    {
        return DecisionNextAction::kOmegaAbility;
    }

    // Shield can do any ability if it has one
    if (entity.Has<AbilitiesComponent, ShieldComponent>())
    {
        const auto& abilities_component = entity.Get<AbilitiesComponent>();
        if (abilities_component.HasAnyAbility(AbilityType::kAttack))
        {
            return DecisionNextAction::kAttackAbility;
        }

        return DecisionNextAction::kNone;
    }

    // Do attack ability? Or Move closer to focus?
    bool move = false;
    if (IsAttackReady(entity, &move))
    {
        if (move)
        {
            return DecisionNextAction::kMovement;
        }
        return DecisionNextAction::kAttackAbility;
    }

    // Refocus if there's a significantly closer target
    if (focus_component.GetRefocusType() == FocusComponent::RefocusType::kAlways && !EntityHelper::IsTaunted(entity) &&
        world_->GetSystem<FocusSystem>().HasOtherTargetSignificantlyCloserThanFocus(entity))
    {
        return DecisionNextAction::kFindFocus;
    }

    // Have to move to do something useful
    if (entity.Has<PositionComponent>())
    {
        // Combat units should not move if they already in attack range, but all previous checks failed
        if (EntityHelper::IsACombatUnit(entity) && HasFocusInRange(entity, false, true))
        {
            return DecisionNextAction::kNone;
        }

        if (entity.Has<MovementComponent>())
        {
            return DecisionNextAction::kMovement;
        }
    }

    return DecisionNextAction::kNone;
}

bool DecisionSystem::DoesOmegaRequireFocus(const Entity& entity) const
{
    if (entity.Has<StatsComponent, AbilitiesComponent>())
    {
        const auto& live_stats = world_->GetLiveStats(entity);
        const auto& abilities_component = entity.Get<AbilitiesComponent>();

        // There is point to check focus for omega if it has some range requirement
        return live_stats.Get(StatType::kOmegaRangeUnits) > 0_fp && abilities_component.DoesOmegaRequireFocusToStart();
    }

    return false;
}

bool DecisionSystem::IsAttackReady(const Entity& entity, bool* out_move) const
{
    *out_move = false;

    // Does not have required components
    if (!entity.Has<AbilitiesComponent, StatsComponent>())
    {
        return false;
    }

    // Get abilities
    const auto& abilities_component = entity.Get<AbilitiesComponent>();

    // Check if we can do attack abilities
    if (!abilities_component.HasAnyAbility(AbilityType::kAttack))
    {
        return false;
    }

    // If it has a attack ability but no position, disregard range
    if (!entity.Has<PositionComponent>())
    {
        return true;
    }

    // Get entity position
    PositionComponent* position_component = nullptr;
    if (entity.Has<PositionComponent>())
    {
        position_component = &(entity.Get<PositionComponent>());
    }

    // Catch this in debug, but return in production
    assert(position_component);
    if (position_component == nullptr)
    {
        return false;
    }

    // Make sure we don't have negative effects preventing attack
    if (!EntityHelper::CanActivateAbility(entity, AbilityType::kAttack))
    {
        return false;
    }

    return HasFocusInRange(entity, false, true);
}

bool DecisionSystem::IsOmegaReady(const Entity& entity) const
{
    // Can only do ability if it has an ability component and stats component
    if (!entity.Has<AbilitiesComponent, StatsComponent, FocusComponent>())
    {
        return false;
    }

    const auto& abilities_component = entity.Get<AbilitiesComponent>();

    // Check if we can do omega abilities
    if (!abilities_component.HasAnyAbility(AbilityType::kOmega))
    {
        return false;
    }
    if (!EntityHelper::HasEnoughEnergyForOmega(entity))
    {
        return false;
    }
    if (!EntityHelper::CanActivateAbility(entity, AbilityType::kOmega))
    {
        return false;
    }

    // Check whether we need any focus checks for omegaCheck
    if (!DoesOmegaRequireFocus(entity))
    {
        // Activate Omega without any focus checks
        return true;
    }

    // Old way, no omega range
    if (world_->IsOmegaRangeEnabled() == false)
    {
        return true;
    }

    return HasFocusInRange(entity, true, false);
}

bool DecisionSystem::HasFocusInRange(const Entity& entity, bool is_omega_range, bool allow_refocus) const
{
    if (FocusSystem::HasFocusInAttackRange(entity, is_omega_range))
    {
        return true;
    }

    if (allow_refocus && world_->GetSystem<FocusSystem>().FindAndUpdateTargetInRange(entity))
    {
        return true;
    }

    return false;
}

DecisionNextAction DecisionSystem::GetActionForTaunted(const Entity& entity) const
{
    if (!entity.Has<FocusComponent>() || !entity.Has<StatsComponent>())
    {
        LogErr(entity.GetID(), "Entities without focus or stats can't be taunted");
        assert(false);
        return DecisionNextAction::kNone;
    }

    const auto& focus_component = entity.Get<FocusComponent>();
    if (!focus_component.GetFocus())
    {
        LogInfo(entity.GetID(), "Focus lost by tauted entity");
        // It happens when focus of taunted entity dies
        // This step taunted effect should be removed by AttachedEffectsSystem
        // And find new focus on the next step
        return DecisionNextAction::kNone;
    }

    if (!entity.Has<PositionComponent>() || !focus_component.GetFocus()->Has<PositionComponent>())
    {
        LogErr(entity.GetID(), "Entities without position can't be taunted or taunt");
        assert(false);
        return DecisionNextAction::kNone;
    }

    if (entity.Has<AbilitiesComponent>())
    {
        const auto& abilities_component = entity.Get<AbilitiesComponent>();

        if (abilities_component.HasAnyAbility(AbilityType::kAttack))
        {
            if (HasFocusInRange(entity, false, false))
            {
                // Can attack current target
                return DecisionNextAction::kAttackAbility;
            }
        }
    }

    return DecisionNextAction::kMovement;
}
}  // namespace simulation
