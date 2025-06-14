#include "systems/focus_system.h"

#include "components/abilities_component.h"
#include "components/attached_effects_component.h"
#include "components/dash_component.h"
#include "components/decision_component.h"
#include "components/focus_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/projectile_component.h"
#include "components/stats_component.h"
#include "data/constants.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "systems/decision_system.h"
#include "utility/entity_helper.h"
#include "utility/enum.h"
#include "utility/grid_helper.h"
#include "utility/targeting_helper.h"

namespace simulation
{
void FocusSystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kFocusUnreachable>(this, &Self::OnFocusUnreachable);
    world_->SubscribeMethodToEvent<EventType::kAbilityDeactivated>(this, &Self::OnAbilityDeactivated);
    world_->SubscribeMethodToEvent<EventType::kOnEffectApplied>(this, &Self::OnEffectApplied);
    world_->SubscribeMethodToEvent<EventType::kOnAttachedEffectRemoved>(this, &Self::OnAttachedEffectRemoved);
}

void FocusSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    static constexpr std::string_view method_name = "FocusSystem::TimeStep";

    // Only apply to entities with FocusComponent
    if (!entity.Has<FocusComponent>())
    {
        return;
    }

    // Component has never refocus property and focus is already set
    auto& focus_component = entity.Get<FocusComponent>();
    if (focus_component.GetRefocusType() == FocusComponent::RefocusType::kNever && focus_component.IsFocusSet())
    {
        // Focus set, but it is no longer focusable
        // Announce the world as otherwise this entity gets to live forever.
        const EntityID focus_id = focus_component.GetFocusID();
        if (!EntityHelper::IsTargetable(*world_, focus_id))
        {
            LogDebug(
                entity.GetID(),
                "{} - set focus = {} is no longer targetable, but we can't change focus type as we are kNever",
                method_name,
                focus_id);
            world_->BuildAndEmitEvent<EventType::kFocusNeverDeactivated>(entity.GetID());
        }

        return;
    }

    // if entity taunted, wait until effect expires to find new focus or when focused target becomes not active
    if (CheckForTauntedEffect(entity))
    {
        // Don't continue
        return;
    }

    // if entity charmed, wait until effect expires to find new focus
    if (EntityHelper::IsCharmed(entity))
    {
        // Don't continue
        return;
    }

    // If the entity is not active we can't do anything else for it
    if (!entity.IsActive())
    {
        bool has_innate_waiting = false;
        if (entity.Has<AbilitiesComponent>())
        {
            has_innate_waiting = entity.Get<AbilitiesComponent>().HasAnyInnateWaitingActivation();
        }

        // Also reset its focus
        if (has_innate_waiting)
        {
            LogDebug(entity.GetID(), "{} - Not Active but has an Innate Waiting Activation.", method_name);
        }
        else
        {
            if (focus_component.IsFocusSet())
            {
                LogDebug(entity.GetID(), "{} - we are no longer active, resetting target", method_name);
                focus_component.ResetFocus();
            }
            return;
        }
    }

    // We have a focus but it is not active, remove it
    // NOTE: will also reset the values for inactive
    if (focus_component.IsFocusSet() && !focus_component.IsFocusActive())
    {
        LogDebug(
            entity.GetID(),
            "{} - focus = {} is set but it is not longer active, resetting focus",
            method_name,
            focus_component.GetFocusID());
        focus_component.ResetFocus();
    }

    // Filter to skip if an ability is active - if the ability uses the focus, it will bail out, so we can refocus
    bool active_ability = false;
    bool ability_lock_movement = true;
    if (entity.Has<AbilitiesComponent>())
    {
        const auto& abilities_component = entity.Get<AbilitiesComponent>();
        active_ability = abilities_component.HasActiveAbility();
        ability_lock_movement = abilities_component.IsMovementLocked();
    }

    // We have a focus, but it is not targetable, so remove it
    if (!active_ability && focus_component.IsFocusSet() && focus_component.GetFocus()->Has<AttachedEffectsComponent>())
    {
        const TargetingHelper& targeting_helper = world_->GetTargetingHelper();
        const EnumSet<GuidanceType> targeting_guidance = targeting_helper.GetTargetingGuidanceForEntity(entity);
        const EntityID focus_id = focus_component.GetFocusID();

        if (!EntityHelper::IsTargetable(*world_, focus_component.GetFocusID()))
        {
            // Cannot target the unit
            LogDebug(
                entity.GetID(),
                "{} - focus = {} is set but it is untargetable, resetting focus",
                method_name,
                focus_component.GetFocusID());

            focus_component.ResetFocus();
        }
        else if (!targeting_helper.DoesEntityMatchesGuidance(targeting_guidance, entity.GetID(), focus_id))
        {
            // Cannot target the unit
            LogDebug(
                entity.GetID(),
                "{} - focus = {} is set but it does not match targeting guidance, resetting focus",
                method_name,
                focus_id);

            focus_component.ResetFocus();
        }
    }

    // Get entity position
    const GridHelper& grid_helper = world_->GetGridHelper();
    const PositionComponent* position_component = grid_helper.GetSourcePositionComponent(entity);

    // Bail out if not valid
    if (position_component == nullptr)
    {
        return;
    }

    // Check if there is an active focused effect affecting this entity
    if (CheckForFocusedEffect(entity))
    {
        // Don't continue
        return;
    }

    // If entity has AI decisions, check decision
    if (entity.Has<DecisionComponent>())
    {
        if (entity.Get<DecisionComponent>().GetNextAction() != DecisionNextAction::kFindFocus)
        {
            // Handling case for abilities with not locked movement
            const bool should_refocus_for_ability =
                active_ability && !ability_lock_movement && !focus_component.IsFocusSet();
            if (!should_refocus_for_ability)
            {
                // AI is not seeking a focus
                return;
            }

            LogDebug(entity.GetID(), "{} - Find a new focus during ability with not locked movement.", method_name);
        }
    }

    // Reset the current focus
    focus_component.ResetFocus();

    auto found_focus = ChooseFocus(entity, std::unordered_set<EntityID>{});

    // Try once more after clearing unreachable
    if (!found_focus)
    {
        // Couldn't find focus, clear unreachable and try again
        focus_component.ClearUnreachable();
        found_focus = ChooseFocus(entity, std::unordered_set<EntityID>{});
    }

    // Update the focus
    if (found_focus)
    {
        LogDebug(entity.GetID(), "{} - set focus = {}", method_name, found_focus->GetID());
        focus_component.SetFocus(found_focus);
        world_->BuildAndEmitEvent<EventType::kFocusFound>(entity.GetID(), found_focus->GetID());
    }

    // Focus but in the same time step do another action
    if (found_focus && entity.Has<DecisionComponent>())
    {
        auto& decision_component = entity.Get<DecisionComponent>();
        const DecisionNextAction next_action = world_->GetSystem<DecisionSystem>().GetNextAction(entity);
        LogDebug(entity.GetID(), "{} - Decision action = {}", method_name, next_action);
        decision_component.SetNextAction(next_action);
    }
}

void FocusSystem::InitialTimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();
    static constexpr std::string_view method_name = "FocusSystem::InitialTimeStep";

    // Only apply to entities with FocusComponent
    if (!entity.Has<FocusComponent>())
    {
        return;
    }

    auto& focus_component = entity.Get<FocusComponent>();
    if (focus_component.IsFocusSet())
    {
        // Entity has a preset focus
        return;
    }

    // Get entity position
    const GridHelper& grid_helper = world_->GetGridHelper();
    const PositionComponent* position_component = grid_helper.GetSourcePositionComponent(entity);
    if (position_component == nullptr)
    {
        return;
    }

    // Handle Rogue/Composite from Rogue movement in a special way
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/261423382/Rogue+Movement

    // Is this a rogue?
    const SynergiesHelper& synergies_helper = world_->GetSynergiesHelper();
    if (synergies_helper.HasCombatClassOrCompositeFrom(entity, CombatClass::kRogue))
    {
        return;
    }

    // Find all the rogues or composite from rogue
    std::unordered_set<EntityID> rogue_set;
    for (const auto& other_entity : world_->GetAll())
    {
        if (synergies_helper.HasCombatClassOrCompositeFrom(*other_entity, CombatClass::kRogue))
        {
            rogue_set.insert(other_entity->GetID());
        }
    }

    // Find focus that is not a rogue
    const auto found_focus = ChooseFocus(entity, rogue_set);

    // Don't continue if we have nothing to work with
    if (!found_focus)
    {
        return;
    }

    // Check range
    const StatsData& stats_data = world_->GetLiveStats(entity);
    const int range_units = stats_data.Get(StatType::kAttackRangeUnits).AsInt();
    const auto& other_position_component = found_focus->Get<PositionComponent>();
    if (!position_component->IsInRange(other_position_component, range_units))
    {
        // Can't reach, don't focus
        LogDebug(entity.GetID(), "{} - possible focus {} not in range", method_name, found_focus->GetID());
        return;
    }

    LogDebug(entity.GetID(), "{} - set initial focus = {}", method_name, found_focus->GetID());
    focus_component.SetFocus(found_focus);
    world_->BuildAndEmitEvent<EventType::kFocusFound>(entity.GetID(), found_focus->GetID());
}

bool FocusSystem::CheckForFocusedEffect(const Entity& entity)
{
    static constexpr std::string_view method_name = "FocusSystem::CheckForFocusedEffect";

    // Check for focused effect on opponents
    const auto focused_effect_entity = world_->GetActiveFocusedEffectEntity(entity);
    if (focused_effect_entity == nullptr)
    {
        return false;
    }

    // Get range of entity
    const StatsData& stats_data = world_->GetLiveStats(entity);
    const int range_units = stats_data.Get(StatType::kAttackRangeUnits).AsInt();

    // Get entity position
    const GridHelper& grid_helper = world_->GetGridHelper();
    const PositionComponent* position_component = grid_helper.GetSourcePositionComponent(entity);

    // Bail out if not valid
    if (position_component == nullptr)
    {
        return false;
    }

    // Get other entity position
    const auto& other_position_component = focused_effect_entity->Get<PositionComponent>();

    // Only switch focus if in range
    if (position_component->IsInRange(other_position_component, range_units))
    {
        LogDebug(
            entity.GetID(),
            "{} - focus on {} due to Focused effect attached and in range",
            method_name,
            focused_effect_entity->GetID());

        // Get focus component to update
        auto& focus_component = entity.Get<FocusComponent>();

        // Set focus to focused entity
        focus_component.SetFocus(focused_effect_entity);

        // Emit event for focus found
        world_->BuildAndEmitEvent<EventType::kFocusFound>(entity.GetID(), focused_effect_entity->GetID());

        // Found a new focus
        return true;
    }

    // Continue normal processing
    return false;
}

bool FocusSystem::CheckForTauntedEffect(const Entity& entity)
{
    static constexpr std::string_view method_name = "FocusSystem::CheckForTauntedEffect";

    // Check for focused effect on entity
    if (!EntityHelper::IsTaunted(entity))
    {
        return false;
    }

    // Get focus component to check
    auto& focus_component = entity.Get<FocusComponent>();
    if (focus_component.IsFocusActive())
    {
        LogDebug(entity.GetID(), "{} - New focus rejected due to taunted state", method_name);
    }
    else
    {
        focus_component.ResetFocus();

        // Remove taunted effect due to inactive forced focus
        auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
        attached_effects_component.RemoveNegativeStateNextTimeStep(EffectNegativeState::kTaunted);

        LogDebug(
            entity.GetID(),
            "{} - Schedule removing of taunt effect in next tick due to inactive focus",
            method_name);
    }

    return true;
}

bool FocusSystem::HasOtherPotentialFocus(const Entity& entity) const
{
    // Check if another potential focus is available
    const auto& focus_component = entity.Get<FocusComponent>();
    std::unordered_set<EntityID> excluded_entity_set;
    excluded_entity_set.insert(focus_component.GetFocusID());
    return ChooseFocus(entity, excluded_entity_set) != nullptr;
}

void FocusSystem::OnEffectApplied(const event_data::Effect& data)
{
    static constexpr std::string_view method_name = "FocusSystem::OnEffectApplied";

    const auto& receiver_entity = world_->GetByID(data.receiver_id);

    if (!EntityHelper::IsACombatUnit(*world_, data.receiver_id))
    {
        return;
    }

    // Listen for negative state that changes focus
    const auto& effect_type_id = data.data.type_id;
    if (effect_type_id.negative_state == EffectNegativeState::kTaunted ||
        effect_type_id.negative_state == EffectNegativeState::kCharm)
    {
        const EntityID new_focus_id = world_->GetCombatUnitParentID(data.sender_id);

        // Set Focus on from entity
        auto& receiver_focus_component = receiver_entity.Get<FocusComponent>();
        receiver_focus_component.SetFocus(world_->GetByIDPtr(new_focus_id));
        world_->BuildAndEmitEvent<EventType::kFocusFound>(receiver_entity.GetID(), new_focus_id);

        LogDebug(
            data.receiver_id,
            "{} - Applying focus on taunted/charmed entity receiver_id = {}, forced focus = {}",
            method_name,
            data.receiver_id,
            new_focus_id);
    }
}

void FocusSystem::OnAttachedEffectRemoved(const event_data::Effect& data)
{
    static constexpr std::string_view method_name = "FocusSystem::OnAttachedEffectRemoved";

    const auto& receiver_entity = world_->GetByID(data.receiver_id);

    if (!receiver_entity.Has<FocusComponent>())
    {
        return;
    }

    // Listen for negative states affecting movement and attacks, or displacement
    const auto& effect_type_id = data.data.type_id;
    if (effect_type_id.negative_state == EffectNegativeState::kStun ||
        effect_type_id.negative_state == EffectNegativeState::kFrozen ||
        effect_type_id.negative_state == EffectNegativeState::kTaunted ||
        effect_type_id.negative_state == EffectNegativeState::kFlee ||
        effect_type_id.negative_state == EffectNegativeState::kCharm ||
        effect_type_id.negative_state == EffectNegativeState::kDisarm ||
        effect_type_id.displacement_type != EffectDisplacementType::kNone)
    {
        if (ConsiderRefocus(receiver_entity))
        {
            LogDebug(data.receiver_id, "{} - reset focus due to negative state/displacement/blink", method_name);
            return;
        }
    }
}

void FocusSystem::OnFocusUnreachable(const event_data::Focus& data)
{
    static constexpr std::string_view method_name = "FocusSystem::OnFocusUnreachable";

    // Get the focus component from the data
    const auto entity_id = data.entity_id;
    const auto& entity = world_->GetByID(entity_id);
    auto& focus_component = entity.Get<FocusComponent>();

    if (focus_component.GetRefocusType() == FocusComponent::RefocusType::kNever)
    {
        // Not allowed to change the focus
        return;
    }

    // Don't make decisions without decision system
    if (!entity.Has<DecisionComponent>())
    {
        LogDebug(entity.GetID(), "{} - entity does not have DecisionComponent", method_name);
        return;
    }

    // Mark current focus as unreachable
    focus_component.AddUnreachable(focus_component.GetFocusID());

    // Clear focus
    focus_component.ResetFocus();

    // Forget the movement history
    if (entity.Has<MovementComponent>())
    {
        auto& movement_component = entity.Get<MovementComponent>();
        movement_component.ClearMovementHistory();
    }
}

std::shared_ptr<Entity> FocusSystem::ChooseFocus(
    const Entity& entity,
    const std::unordered_set<EntityID>& exclude_entities) const
{
    static constexpr std::string_view method_name = "FocusSystem::ChooseFocus";

    const auto& focus_component = entity.Get<FocusComponent>();

    // Choose our focus depending on our selection type
    std::shared_ptr<Entity> found_focus = nullptr;
    switch (focus_component.GetSelectionType())
    {
    case FocusComponent::SelectionType::kClosestEnemy:
    {
        constexpr bool include_unreachable = false;
        const EntityID closest_id = world_->GetGridHelper().FindClosest(entity, exclude_entities, include_unreachable);
        if (closest_id != kInvalidEntityID)
        {
            found_focus = world_->GetByIDPtr(closest_id);
        }

        break;
    }
    case FocusComponent::SelectionType::kPredefined:
    {
        // Only projectiles and dash for now
        if (EntityHelper::IsAProjectile(*world_, entity.GetID()))
        {
            const auto& projectile_component = entity.Get<ProjectileComponent>();
            const EntityID receiver_id = projectile_component.GetReceiverID();
            if (!world_->HasEntity(receiver_id))
            {
                LogDebug(entity.GetID(), "{} - Predefined target is not active", method_name);
                return nullptr;
            }
            found_focus = world_->GetByIDPtr(receiver_id);
        }
        else if (EntityHelper::IsADash(*world_, entity.GetID()))
        {
            const auto& dash_component = entity.Get<DashComponent>();
            const EntityID receiver_id = dash_component.GetReceiverID();
            if (!world_->HasEntity(receiver_id))
            {
                return nullptr;
            }

            found_focus = world_->GetByIDPtr(receiver_id);
        }
        else
        {
            // Other predefined focus is set directly and needs no action
            return nullptr;
        }

        break;
    }
    default:
        break;
    }

    return found_focus;
}

bool FocusSystem::HasFocusInAttackRange(const Entity& entity, bool is_omega_range)
{
    if (!entity.Has<FocusComponent>() || !entity.Has<StatsComponent>())
    {
        return false;
    }

    const auto& focus_component = entity.Get<FocusComponent>();
    if (!focus_component.IsFocusActive())
    {
        return false;
    }

    if (!entity.Has<PositionComponent>() || !focus_component.GetFocus()->Has<PositionComponent>())
    {
        return false;
    }

    // Get entity position
    const auto& position_component = entity.Get<PositionComponent>();
    // Get receiver position
    const auto& receiver_position_component = focus_component.GetFocus()->Get<PositionComponent>();

    // Get range of entity
    const StatsData& stats_data = entity.GetOwnerWorld()->GetLiveStats(entity);
    const int range_units = is_omega_range ? stats_data.Get(StatType::kOmegaRangeUnits).AsInt()
                                           : stats_data.Get(StatType::kAttackRangeUnits).AsInt();

    return position_component.IsInRange(receiver_position_component, range_units);
}

void FocusSystem::OnAbilityDeactivated(const event_data::AbilityDeactivated& data)
{
    static constexpr std::string_view method_name = "FocusSystem::OnAbilityDeactivated";

    // Get the details from the data
    const EntityID sender_id = data.sender_id;

    // Only omegas
    if (data.ability_type != AbilityType::kOmega)
    {
        return;
    }

    // Only with movement skills or exceeding minimum duration for refocus
    if (!data.has_skill_with_movement && data.total_duration_ms < kRefocusAfterOmegaMs)
    {
        return;
    }

    for (const auto& entity : world_->GetAll())
    {
        // Ignore entities without focus or position component
        if (!entity->Has<FocusComponent>() || !entity->Has<PositionComponent>())
        {
            continue;
        }

        // Get the focus
        auto& focus_component = entity->Get<FocusComponent>();

        if (!focus_component.IsFocusActive())
        {
            // Doesn't have an active focus anyway
            continue;
        }

        // Check for sender or anything focused on sender
        if (entity->GetID() == sender_id)
        {
            if (data.has_skill_with_movement)
            {
                // Movement omega consider refocus
                ConsiderRefocus(*entity);
            }
            else
            {
                // Always refocus sender
                LogDebug(entity->GetID(), "{} - reset focus due to omega", method_name);
                focus_component.ResetFocus();
            }
        }
        else if (data.has_skill_with_movement && focus_component.GetFocusID() == sender_id)
        {
            // Movement omega consider refocus
            ConsiderRefocus(*entity);
        }
    }
}

bool FocusSystem::HasOtherTargetSignificantlyCloserThanFocus(const Entity& entity) const
{
    static constexpr std::string_view method_name = "FocusSystem::HasOtherTargetSignificantlyCloserThanFocus";

    const EntityID entity_id = entity.GetID();

    // Get focus component
    auto& focus_component = entity.Get<FocusComponent>();

    // Get current time step and scan frequency
    const int current_time_step = world_->GetTimeStepCount();
    if (current_time_step < focus_component.GetDistanceScanLastTimeStep() + world_->GetDistanceScanFrequencyTimeSteps())
    {
        // Not time to do it
        LogDebug(entity_id, "{} - skipping this time step", method_name);
        return false;
    }

    // Update last time step
    focus_component.SetDistanceScanLastTimeStep(current_time_step);

    // Check if another potential focus is available
    std::unordered_set<EntityID> excluded_entity_set;
    excluded_entity_set.insert(focus_component.GetFocusID());
    const auto other_target = ChooseFocus(entity, excluded_entity_set);

    if (other_target == nullptr)
    {
        // Nothing else
        LogDebug(entity_id, "{} - no other targets", method_name);
        return false;
    }

    // Calculate target distance
    const auto& movement_component = entity.Get<MovementComponent>();
    const int target_distance = world_->GetFocusSwitchMovementMultiplier() *
                                movement_component.GetMovementSpeedSubUnits().AsInt() / kSubUnitsPerUnit;

    // Compare distances
    const HexGridPosition& current_position = entity.Get<PositionComponent>().GetPosition();
    const int distance_to_current_focus =
        (focus_component.GetFocus()->Get<PositionComponent>().GetPosition() - current_position).Length();
    const int distance_to_alternate_focus =
        (other_target->Get<PositionComponent>().GetPosition() - current_position).Length();
    if (distance_to_alternate_focus < distance_to_current_focus - target_distance)
    {
        // Alternate is much closer
        LogDebug(entity_id, "{} - other target close enough", method_name);
        return true;
    }

    // Not close enough to switch
    LogDebug(entity_id, "{} - other target not close enough", method_name);
    return false;
}

bool FocusSystem::ConsiderRefocus(const Entity& entity)
{
    // Get the focus component
    auto& focus_component = entity.Get<FocusComponent>();

    // Don't need to move to get to focus
    const DecisionNextAction next_action = world_->GetSystem<DecisionSystem>().GetNextAction(entity);
    if (next_action != DecisionNextAction::kMovement && next_action != DecisionNextAction::kFindFocus)
    {
        return false;
    }

    // Not allowed to change the focus
    if (focus_component.GetRefocusType() == FocusComponent::RefocusType::kNever)
    {
        return false;
    }

    // Can't change focus to a valid target
    if (!HasOtherPotentialFocus(entity))
    {
        return false;
    }

    // Reset unreachable
    focus_component.ResetUnreachableFocusTimeSteps();

    // Clear focus
    focus_component.ResetFocus();

    // Forget the movement history
    if (entity.Has<MovementComponent>())
    {
        auto& movement_component = entity.Get<MovementComponent>();
        movement_component.ClearMovementHistory();
    }

    return true;
}

bool FocusSystem::FindAndUpdateTargetInRange(const Entity& entity)
{
    // Get focus component
    auto& focus_component = entity.Get<FocusComponent>();

    // Update focus when in range, if possible
    if (focus_component.GetRefocusType() == FocusComponent::RefocusType::kAlways)
    {
        // Get position component
        const auto& position_component = entity.Get<PositionComponent>();

        // Get range of entity
        const StatsData& stats_data = world_->GetLiveStats(entity);
        const int range_units = stats_data.Get(StatType::kAttackRangeUnits).AsInt();

        // Get the targeting guidance
        const TargetingHelper& targeting_helper = world_->GetTargetingHelper();
        const EnumSet<GuidanceType> targeting_guidance = targeting_helper.GetTargetingGuidanceForEntity(entity);

        std::shared_ptr<Entity> closest_enemy;
        int closest_enemy_distance = 0;

        // Consider other targets
        for (const auto& other_entity : world_->GetAll())
        {
            const EntityID other_entity_id = other_entity->GetID();

            // Skip self
            if (other_entity_id == entity.GetID())
            {
                continue;
            }

            // Current focus, already checked
            if (other_entity_id == focus_component.GetFocusID())
            {
                continue;
            }

            // Not targettable, ignore
            if (!EntityHelper::IsTargetable(*world_, other_entity->GetID()))
            {
                continue;
            }

            // No friendly fire
            if (other_entity->IsAlliedWith(entity))
            {
                continue;
            }

            // Only focus entities that have required components
            if (!other_entity->Has<StatsComponent, PositionComponent>())
            {
                continue;
            }

            // Targeting guidance must match
            if (!targeting_helper.DoesEntityMatchesGuidance(targeting_guidance, entity.GetID(), other_entity_id))
            {
                continue;
            }

            // Get other entity position
            const auto& other_position_component = other_entity->Get<PositionComponent>();

            if (!position_component.IsInRange(other_position_component, range_units))
            {
                continue;
            }

            const int distance = (other_position_component.GetPosition() - position_component.GetPosition()).Length();
            if (closest_enemy && closest_enemy_distance < distance)
            {
                continue;
            }

            closest_enemy = other_entity;
            closest_enemy_distance = distance;
        }

        // Update focus
        if (closest_enemy)
        {
            focus_component.SetFocus(closest_enemy);
            world_->BuildAndEmitEvent<EventType::kFocusFound>(entity.GetID(), closest_enemy->GetID());

            // Can attack new target
            return true;
        }
    }

    // No new target
    return false;
}

}  // namespace simulation
