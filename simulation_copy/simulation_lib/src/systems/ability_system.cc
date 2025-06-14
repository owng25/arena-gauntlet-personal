#include "systems/ability_system.h"

#include "components/abilities_component.h"
#include "components/combat_unit_component.h"
#include "components/dash_component.h"
#include "components/decision_component.h"
#include "components/filtering_component.h"
#include "components/focus_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/stats_component.h"
#include "data/combat_unit_data.h"
#include "data/constants.h"
#include "data/dash_data.h"
#include "data/effect_enums.h"
#include "data/enums.h"
#include "data/projectile_data.h"
#include "data/skill_data.h"
#include "data/source_context.h"
#include "data/zone_data.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "factories/entity_factory.h"
#include "systems/decision_system.h"
#include "utility/attached_effects_helper.h"
#include "utility/effect_package_helper.h"
#include "utility/entity_helper.h"
#include "utility/enum.h"
#include "utility/grid_helper.h"
#include "utility/hex_grid_position.h"
#include "utility/math.h"
#include "utility/stats_helper.h"
#include "utility/targeting_helper.h"
#include "utility/time.h"
#include "utility/vector_helper.h"

namespace simulation
{

void AbilitySystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kActivateAnyAbility>(this, &Self::OnActivateAnyAbility);
    world_->SubscribeMethodToEvent<EventType::kEffectPackageMissed>(this, &Self::OnEffectPackageMissed);
    world_->SubscribeMethodToEvent<EventType::kEffectPackageReceived>(this, &Self::OnEffectPackageReceived);
    world_->SubscribeMethodToEvent<EventType::kEffectPackageDodged>(this, &Self::OnEffectPackageDodged);
    world_->SubscribeMethodToEvent<EventType::kAbilityActivated>(this, &Self::OnAbilityActivated);
    world_->SubscribeMethodToEvent<EventType::kOnDamage>(this, &Self::OnDamage);
    world_->SubscribeMethodToEvent<EventType::kFainted>(this, &Self::OnCombatUnitFainted);
    world_->SubscribeMethodToEvent<EventType::kHealthChanged>(this, &Self::OnHealthChanged);
    world_->SubscribeMethodToEvent<EventType::kOnEffectApplied>(this, &Self::OnEffectApplied);
    world_->SubscribeMethodToEvent<EventType::kOnAttachedEffectApplied>(this, &Self::OnAttachedEffectApplied);
    world_->SubscribeMethodToEvent<EventType::kOnAttachedEffectRemoved>(this, &Self::OnAttachedEffectRemoved);
    world_->SubscribeMethodToEvent<EventType::kApplyEnergyGain>(this, &Self::OnEneryGain);
    world_->SubscribeMethodToEvent<EventType::kSkillFinished>(this, &Self::OnSkillFinished);
    world_->SubscribeMethodToEvent<EventType::kGoneHyperactive>(this, &Self::OnGoneHyperactive);
    world_->SubscribeMethodToEvent<EventType::kShieldWasHit>(this, &Self::OnShieldWasHit);
}

void AbilitySystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Only relevant to entities with AbilitiesComponent
    if (!entity.Has<AbilitiesComponent>())
    {
        return;
    }

    // Not active and
    const auto& abilities_component = entity.Get<AbilitiesComponent>();
    if (!entity.IsActive())
    {
        if (abilities_component.HasAnyInnateWaitingActivation())
        {
            LogDebug(entity.GetID(), "AbilitySystem::TimeStep - Not Active but has an Innate Waiting Activation.");
        }
        else
        {
            // Has no innate waiting activation
            return;
        }
    }

    // LogDebug(entity.GetID(), "AbilitySystem::TimeStep");
    if (entity.Has<StatsComponent>())
    {
        RegenerateHealthAndEnergy(entity);
    }

    CancelAttackOnFullEnergy(entity);

    // Activate innates
    {
        AbilityStateActivatorContext activator_context;
        activator_context.sender_entity_id = entity.GetID();
        activator_context.sender_combat_unit_entity_id = entity.GetID();
        activator_context.ability_type = AbilityType::kNone;

        // Try to activate time innates
        ChooseAndTryToActivateInnateAbilities(activator_context, entity, ActivationTriggerType::kEveryXTime);

        // Activate kInRange innate ability
        ChooseAndTryToActivateInnateAbilities(activator_context, entity, ActivationTriggerType::kInRange);
    }

    // Try to activate all the instant innates
    ActivateAllInstantInnateAbilities(entity);

    // Check Decision Component if any
    bool decision_system_do_omega = false;
    bool decision_system_do_attack = false;
    const bool has_decision_component = entity.Has<DecisionComponent>();
    if (has_decision_component)
    {
        const auto& decision_component = entity.Get<DecisionComponent>();
        const auto next_action = decision_component.GetNextAction();
        decision_system_do_attack = next_action == DecisionNextAction::kAttackAbility;
        decision_system_do_omega = next_action == DecisionNextAction::kOmegaAbility;
        if (!decision_system_do_attack && !decision_system_do_omega)
        {
            // Decided not do any ability
            if (auto active_ability = abilities_component.GetActiveAbility())
            {
                LogDebug(
                    entity.GetID(),
                    "AbilitySystem::TimeStep - Decision component said we can't do an attack ability and omega "
                    "ability");
                force_next_time_step_ = true;
                ForceFinishAllSkills(entity, active_ability);
                force_next_time_step_ = false;
            }
            return;
        }
    }

    // Omega can be activated without focus, so it should not be blocked by a focus check
    // Decision system also has a focus check on whether it's required for activating omega
    if (!decision_system_do_omega && entity.Has<FocusComponent>())
    {
        // Perform focus check only if decision system does not what to activate omega
        if (!TimeStepCheckFocus(entity))
        {
            return;
        }
    }

    //
    // TimeStep the ability/skill timers
    //
    // Enforce that activate and deactivate always happen in a different time step.
    // The TimeStep timers can deactivate an omega ability but the decision system did not have
    // the time to step, so we can activate the omega ability again (false as we depleted it).
    const bool has_omega_active_ability = abilities_component.HasOmegaActiveAbility();
    TimeStepTimers(entity);
    if (has_omega_active_ability != abilities_component.HasOmegaActiveAbility())
    {
        LogDebug(entity.GetID(), "AbilitySystem::TimeStep - Can't Activate and Deactivate Omega in the same TimeStep.");
        return;
    }

    // TimeStep through active ability
    auto active_ability = abilities_component.GetActiveAbility();
    if (active_ability)
    {
        TryToStepAbility(entity, active_ability);
        return;
    }

    // Choose and Activate an ability
    bool try_to_activate_attack = false;
    bool try_to_activate_omega = false;
    if (has_decision_component)
    {
        // Respect the decision system
        try_to_activate_attack = decision_system_do_attack;
        try_to_activate_omega = decision_system_do_omega;
    }
    else
    {
        // Try just attack abilities for all entities except Dashes
        try_to_activate_attack = !entity.Has<DashComponent>();
        try_to_activate_omega = false;
    }

    // Activate attack or omega
    ChooseAndActivateAnyAbility(entity, try_to_activate_attack, try_to_activate_omega);
}

void AbilitySystem::PostTimeStep(const Entity& entity)
{
    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    // Does not have the right components
    if (!entity.Has<StatsComponent>())
    {
        return;
    }

    auto& stats_component = entity.Get<StatsComponent>();
    const StatsData& live_stats = world_->GetLiveStats(entity);

    const FixedPoint live_attack_speed = world_->GetClampedAttackSpeed(live_stats.Get(StatType::kAttackSpeed));
    stats_component.SetPreviousAttackSpeed(live_attack_speed);
    LogDebug(entity.GetID(), "AbilitySystem::PostTimeStep - Setting previous attack speed to {}.", live_attack_speed);
}

bool AbilitySystem::TimeStepCheckFocus(const Entity& entity)
{
    const auto& abilities_component = entity.Get<AbilitiesComponent>();
    auto& focus_component = entity.Get<FocusComponent>();

    if (focus_component.IsFocusActive() && EntityHelper::IsTargetable(*world_, focus_component.GetFocusID()))
    {
        // Has an active focus that is targetable
        return true;
    }

    // Handle direct position movement type in a special way as we don't need a focus
    if (entity.Has<MovementComponent>())
    {
        const auto& movement_component = entity.Get<MovementComponent>();
        if (movement_component.GetMovementType() == MovementComponent::MovementType::kDirectPosition &&
            !movement_component.GetMovementTargetPosition().IsNull())
        {
            return true;
        }
    }

    // No target yet
    LogDebug(entity.GetID(), "AbilitySystem::TimeStepCheckFocus - Focus no longer active");

    if (!abilities_component.HasActiveAbility())
    {
        LogDebug(entity.GetID(), "- does NOT have an active ability");
        return false;
    }

    // Special case for innates, we don't care we don't have any focus
    if (abilities_component.GetActiveAbilityType() == AbilityType::kInnate)
    {
        LogDebug(entity.GetID(), "- Innate ability, continuing.");
        return true;
    }

    // We still have an active ability, this can happen while we started an ability
    // but the target gets deactivated in the meantime (by another entity for example)

    // Set the active ability into the finished state
    auto active_ability = abilities_component.GetActiveAbility();
    SkillState& skill_state = active_ability->GetCurrentSkillState();

    // Focus id is not a target for our ability, let it continue.
    const EntityID focus_id =
        focus_component.IsFocusSet() ? focus_component.GetFocusID() : focus_component.GetPreviousFocusID();
    if (skill_state.targeting_state.available_targets.count(focus_id) > 0)
    {
        LogDebug(
            entity.GetID(),
            "- focus_id = {} is no longer active and in the current skill predicted_receiver_ids, aborting skill",
            focus_id);

        // Mark the active skill as finished
        SetSkillState(entity, SkillStateType::kFinished, active_ability, skill_state);

        return false;
    }

    focus_component.ResetUnreachableFocusTimeSteps();

    // Clear unreachable state of other entities as they may become reachable
    focus_component.ClearUnreachable();

    LogDebug(
        entity.GetID(),
        "- focus_id = {} is not active, but skill_index = {} does not target it. Continuing",
        focus_id,
        skill_state.index);
    return true;
}

void AbilitySystem::RegenerateHealthAndEnergy(const Entity& receiver_entity) const
{
    // Only for combat units
    if (!EntityHelper::IsACombatUnit(receiver_entity))
    {
        return;
    }

    const EntityID receiver_id = receiver_entity.GetID();
    const StatsData receiver_stats = world_->GetLiveStats(receiver_entity);

    // Regen health and energy

    // Energy gain event to be calculated by energy gain system and updated
    if (receiver_stats.Get(StatType::kEnergyRegeneration) > 0_fp)
    {
        event_data::ApplyEnergyGain event_data;
        event_data.caused_by_id = receiver_id;
        event_data.receiver_id = receiver_id;
        event_data.energy_gain_type = EnergyGainType::kRegeneration;

        world_->EmitEvent<EventType::kApplyEnergyGain>(event_data);
    }

    if (receiver_stats.Get(StatType::kHealthRegeneration) > 0_fp)
    {
        event_data::ApplyHealthGain event_data;
        event_data.caused_by_id = receiver_id;
        event_data.receiver_id = receiver_id;
        event_data.health_gain_type = HealthGainType::kRegeneration;

        world_->EmitEvent<EventType::kApplyHealthGain>(event_data);
    }
}

void AbilitySystem::OnActivateAnyAbility(const event_data::ActivateAnyAbility& data)
{
    ILLUVIUM_PROFILE_FUNCTION();

    ChooseAndActivateAnyAbility(world_->GetByID(data.entity_id), data.can_do_attack, data.can_do_omega);
}

void AbilitySystem::OnEffectPackageReceived(const event_data::EffectPackage& data)
{
    if (data.ability_type == AbilityType::kInnate)
    {
        return;
    }

    const EntityID receiver_combat_unit_id = world_->GetCombatUnitParentID(data.receiver_id);

    // TODO(vampy): Implement the full on hit https://illuvium.atlassian.net/wiki/spaces/AB/pages/256802893/OnHit
    AbilityStateActivatorContext activator_context;
    activator_context.sender_entity_id = data.sender_id;
    activator_context.sender_combat_unit_entity_id = data.combat_unit_sender_id;
    activator_context.receiver_entity_id = data.receiver_id;
    activator_context.receiver_combat_unit_entity_id = receiver_combat_unit_id;
    activator_context.ability_type = data.source_context.combat_unit_ability_type;

    world_->SafeWalkAll(
        [&](const Entity& entity)
        {
            if (EntityHelper::IsACombatUnit(entity))
            {
                ChooseAndTryToActivateInnateAbilities(activator_context, entity, ActivationTriggerType::kOnHit);
            }
        });

    if (data.is_critical)
    {
        ChooseAndTryToActivateInnateAbilities(
            activator_context,
            data.combat_unit_sender_id,
            ActivationTriggerType::kOnDealCrit);
    }

    ChooseAndTryToActivateInnateAbilities(
        activator_context,
        receiver_combat_unit_id,
        ActivationTriggerType::kOnReceiveXEffectPackages);
}

void AbilitySystem::OnEffectPackageMissed(const event_data::EffectPackage& data)
{
    if (data.ability_type == AbilityType::kInnate)
    {
        return;
    }

    AbilityStateActivatorContext activator_context;
    activator_context.sender_entity_id = data.sender_id;
    activator_context.sender_combat_unit_entity_id = data.combat_unit_sender_id;
    activator_context.ability_type = data.ability_type;

    world_->SafeWalkAll(
        [&](const Entity& entity)
        {
            if (EntityHelper::IsACombatUnit(entity))
            {
                ChooseAndTryToActivateInnateAbilities(activator_context, entity, ActivationTriggerType::kOnMiss);
            }
        });
}

void AbilitySystem::OnEffectPackageDodged(const event_data::EffectPackage& data)
{
    if (data.ability_type == AbilityType::kInnate)
    {
        return;
    }

    AbilityStateActivatorContext activator_context;
    activator_context.sender_entity_id = data.receiver_id;
    activator_context.sender_combat_unit_entity_id = world_->GetCombatUnitParentID(activator_context.sender_entity_id);
    activator_context.ability_type = data.ability_type;
    activator_context.receiver_entity_id = data.sender_id;
    activator_context.receiver_combat_unit_entity_id = data.combat_unit_sender_id;

    world_->SafeWalkAll(
        [&](const Entity& entity)
        {
            if (EntityHelper::IsACombatUnit(entity))
            {
                ChooseAndTryToActivateInnateAbilities(activator_context, entity, ActivationTriggerType::kOnDodge);
            }
        });
}

void AbilitySystem::OnDamage(const event_data::AppliedDamage& event_data)
{
    // Record physical damage for wound application, if requested
    if (!currently_applying_wound_)
    {
        if (event_data.damage_type == EffectDamageType::kPhysical && !wound_recording_frames_.empty())
        {
            WoundRecordingFrame& frame = wound_recording_frames_.front();
            if (frame.sender == event_data.sender_id && frame.receiver == event_data.receiver_id &&
                frame.ability_type == event_data.source_context.combat_unit_ability_type)
            {
                frame.accumulated_damage += event_data.damage_value;
            }
        }
    }

    // Track damage sent/received
    if (const std::shared_ptr<Entity>& entity_sender = world_->GetByIDPtr(event_data.combat_unit_sender_id))
    {
        // Assumes its already a combat unit
        StatsHistoryData& sender_stats_history_data = entity_sender->Get<StatsComponent>().GetMutableHistoryData();
        sender_stats_history_data.AddDamageSent(event_data.damage_type, event_data.damage_value);

        const std::shared_ptr<Entity>& entity_receiver = world_->GetByIDPtr(event_data.combat_unit_sender_id);
        if (entity_receiver && entity_receiver->Has<StatsComponent>())
        {
            StatsHistoryData& receiver_stats_history_data =
                entity_receiver->Get<StatsComponent>().GetMutableHistoryData();
            receiver_stats_history_data.AddDamageReceived(event_data.damage_type, event_data.damage_value);
        }
    }

    // Ignore damage events from innates in the context of activating abilities
    if (event_data.source_context.combat_unit_ability_type == AbilityType::kInnate)
    {
        return;
    }

    AbilityStateActivatorContext activator_context;
    activator_context.sender_entity_id = event_data.sender_id;
    activator_context.sender_combat_unit_entity_id = event_data.combat_unit_sender_id;
    activator_context.receiver_entity_id = event_data.receiver_id;
    activator_context.receiver_combat_unit_entity_id =
        world_->GetCombatUnitParentID(activator_context.receiver_entity_id);
    activator_context.ability_type = event_data.source_context.combat_unit_ability_type;
    activator_context.event = Event(EventType::kOnDamage);
    activator_context.event.Set(event_data);

    world_->SafeWalkAll(
        [&](const Entity& entity)
        {
            if (EntityHelper::IsACombatUnit(entity))
            {
                ChooseAndTryToActivateInnateAbilities(activator_context, entity, ActivationTriggerType::kOnDamage);
            }
        });
}

void AbilitySystem::OnCombatUnitFainted(const event_data::Fainted& fainted_data)
{
    // Force finish all remaining abilities
    const EntityID combat_unit_fainted_id = fainted_data.entity_id;

    // We are only interested in Combat Units
    if (!EntityHelper::IsACombatUnit(*world_, combat_unit_fainted_id))
    {
        return;
    }

    const auto& entity_fainted = world_->GetByID(combat_unit_fainted_id);
    const AbilityType activator_ability_type = fainted_data.source_context.combat_unit_ability_type;
    {
        const auto& abilities_component = entity_fainted.Get<AbilitiesComponent>();
        if (auto active_ability = abilities_component.GetActiveAbility())
        {
            if (active_ability->ability_type != AbilityType::kInnate)
            {
                ForceFinishAllSkills(entity_fainted, active_ability);
            }
        }
    }

    // Get parent_id if created entity triggers kOnVanquish instead of the illuvial with the innate
    const EntityID combat_unit_vanquisher_id = world_->GetCombatUnitParentID(fainted_data.vanquisher_id);

    // Track count vanquishes
    if (const std::shared_ptr<Entity>& combat_unit_vanquisher_entity = world_->GetByIDPtr(combat_unit_vanquisher_id))
    {
        StatsHistoryData& stats_history_data =
            combat_unit_vanquisher_entity->Get<StatsComponent>().GetMutableHistoryData();
        stats_history_data.count_faint_vanquishes++;
    }

    // Check allies or enemies, check all allies of the entity_id
    world_->SafeWalkAll(
        [&](const Entity& entity)
        {
            AbilityStateActivatorContext activator_context;
            activator_context.sender_entity_id = fainted_data.entity_id;
            activator_context.sender_combat_unit_entity_id =
                world_->GetCombatUnitParentID(activator_context.sender_entity_id);
            activator_context.ability_type = activator_ability_type;

            ChooseAndTryToActivateInnateAbilities(activator_context, entity, ActivationTriggerType::kOnFaint);

            // Special case for vanquish trigger - need to keep count of events that
            // satisfied all trigger conditions except the counter condition itself
            //
            // For example. We have this trigger.
            //
            // {
            //    "TriggerType": "OnVanquish",
            //    "ComparisonType": ">",
            //    "TriggerValue": 3,
            //    "AllegianceType": "Ally",
            //    "Range": 20
            // }
            //
            // In this case we should increment the trigger counter every time when
            // an ally kills someone in range 20
            // 1. A faint event happens.
            // 2. We check the allegiance of the vanquisher.
            // 3. If it is an Ally then
            // 4. if it is in range.
            // 5. Then it is a valid event for us. We increment the counter and then check if > 3.
            //
            // In order to implement that we increment counter if
            // CanActivateAbilityForTriggerData(with_trigger_counter=false) is true and then use regular
            // ChooseAndTryToActivateInnateAbilities which will internally call the
            // CanActivateAbilityForTriggerData(with_trigger_counter=true)
            bool try_to_activate = false;
            if (entity.Has<AbilitiesComponent>())
            {
                auto& abilities_component = entity.Get<AbilitiesComponent>();
                const int time_step = world_->GetTimeStepCount();
                abilities_component.ChooseInnateAbility(ActivationTriggerType::kOnVanquish);
                for (auto& ability_state : abilities_component.GetStateInnateAbilities())
                {
                    const bool test_trigger_counter = false;
                    if (CanActivateAbilityForTriggerData(entity, *ability_state, time_step, test_trigger_counter))
                    {
                        ability_state->trigger_counter++;
                        try_to_activate = true;
                    }
                }
            }

            // There is some innate ability with kOnVanquish trigger and it can be activate
            // Try to activate it but with internal counter test this time.
            if (try_to_activate)
            {
                activator_context.sender_entity_id = fainted_data.vanquisher_id;
                activator_context.sender_combat_unit_entity_id = combat_unit_vanquisher_id;
                ChooseAndTryToActivateInnateAbilities(activator_context, entity, ActivationTriggerType::kOnVanquish);
            }

            InvalidateTargetIfNeeded(entity, entity_fainted);
        });

    const std::vector<EntityID>& combat_unit_assists = world_->GetCombatUnitsFaintAssists(combat_unit_fainted_id);

    LogDebug(
        combat_unit_fainted_id,
        "AbilitySystem::OnCombatUnitFainted - combat_unit_assists = {}, context: {}",
        combat_unit_assists,
        fainted_data.source_context);

    AbilityStateActivatorContext activator_context;
    activator_context.sender_entity_id = combat_unit_vanquisher_id;
    activator_context.sender_combat_unit_entity_id = combat_unit_vanquisher_id;
    activator_context.ability_type = activator_ability_type;

    // Find out all the assists triggers
    for (const EntityID assist_id : combat_unit_assists)
    {
        // Only for active combat units
        if (!world_->IsCombatUnitAlive(assist_id))
        {
            continue;
        }

        // Track count faints
        if (const std::shared_ptr<Entity>& assist_entity = world_->GetByIDPtr(assist_id))
        {
            StatsHistoryData& stats_history_data = assist_entity->Get<StatsComponent>().GetMutableHistoryData();
            stats_history_data.count_faint_assists++;
        }

        ChooseAndTryToActivateInnateAbilities(activator_context, assist_id, ActivationTriggerType::kOnAssist);
    }
}

void AbilitySystem::OnHealthChanged(const event_data::StatChanged& data)
{
    AbilityStateActivatorContext activator_context;
    activator_context.sender_entity_id = data.caused_by_id;
    activator_context.sender_combat_unit_entity_id = world_->GetCombatUnitParentID(data.caused_by_id);
    activator_context.ability_type = AbilityType::kNone;

    ChooseAndTryToActivateInnateAbilities(activator_context, data.entity_id, ActivationTriggerType::kHealthInRange);
}

void AbilitySystem::OnEneryGain(const event_data::ApplyEnergyGain& event)
{
    AbilityStateActivatorContext activator_context;
    activator_context.sender_entity_id = event.caused_by_id;
    activator_context.sender_combat_unit_entity_id = world_->GetCombatUnitParentID(event.caused_by_id);
    activator_context.ability_type = AbilityType::kNone;

    ChooseAndTryToActivateInnateAbilities(activator_context, event.receiver_id, ActivationTriggerType::kOnEnergyFull);
}

void AbilitySystem::OnEffectApplied(const event_data::Effect& event_data)
{
    const auto& sender_entity_id = event_data.sender_id;
    const auto& receiver_entity_id = event_data.receiver_id;
    const auto& receiver_entity = world_->GetByID(receiver_entity_id);
    const auto& effect_data = event_data.data;

    // Check for Untargetable effect
    const bool is_untargetable_effect = effect_data.type_id.type == EffectType::kPositiveState &&
                                        effect_data.type_id.positive_state == EffectPositiveState::kUntargetable;
    const bool is_untargetable = is_untargetable_effect && !EntityHelper::IsTargetable(receiver_entity);

    // Check for PlaneChange effect
    const bool is_plane_change = effect_data.type_id.type == EffectType::kPlaneChange;

    if (is_untargetable || is_plane_change)
    {
        // Invalidate new entity in all targeting states of other entities
        world_->SafeWalkAll(
            [&](const Entity& entity)
            {
                // Not Active
                if (!entity.IsActive())
                {
                    return;
                }

                if (!EntityHelper::IsACombatUnit(entity))
                {
                    return;
                }

                if (is_plane_change)
                {
                    const TargetingHelper& targeting_helper = world_->GetTargetingHelper();
                    const EnumSet<GuidanceType> targeting_guidance =
                        targeting_helper.GetTargetingGuidanceForEntity(entity);
                    if (targeting_helper
                            .DoesEntityMatchesGuidance(targeting_guidance, sender_entity_id, receiver_entity_id))
                    {
                        // Guidance still matches
                        return;
                    }
                }

                InvalidateTargetIfNeeded(entity, receiver_entity);
            });
    }
}

void AbilitySystem::OnAttachedEffectApplied(const event_data::Effect& event_data)
{
    const auto& receiver_entity_id = event_data.receiver_id;
    const auto& receiver_entity = world_->GetByID(receiver_entity_id);

    // Reset attack sequence when it is interrupted
    if (EntityHelper::CanInterruptAbility(receiver_entity, AbilityType::kAttack))
    {
        auto& abilities_component = receiver_entity.Get<AbilitiesComponent>();
        auto& attack_abilities = abilities_component.GetAbilities(AbilityType::kAttack);
        attack_abilities.state.current_ability_index = kInvalidIndex;
    }
}

void AbilitySystem::OnAttachedEffectRemoved(const event_data::Effect& event_data)
{
    const auto& sender_entity_id = event_data.sender_id;
    const auto& receiver_entity_id = event_data.receiver_id;
    const auto& receiver_entity = world_->GetByID(receiver_entity_id);
    const auto& effect_data = event_data.data;

    // Check for Untargetable effect
    const bool is_untargetable_effect = effect_data.type_id.type == EffectType::kPositiveState &&
                                        effect_data.type_id.positive_state == EffectPositiveState::kUntargetable;
    const bool is_untargetable = is_untargetable_effect && !EntityHelper::IsTargetable(receiver_entity);

    // Check for PlaneChange effect
    const bool is_plane_change = effect_data.type_id.type == EffectType::kPlaneChange;

    // Proceed further only for these effects
    if (!is_untargetable && !is_plane_change)
    {
        return;
    }

    // Restore old entity in all targeting states of other entities
    world_->SafeWalkAll(
        [&](const Entity& entity)
        {
            // Not Active
            if (!entity.IsActive())
            {
                return;
            }

            if (!EntityHelper::IsACombatUnit(entity))
            {
                return;
            }

            const auto& abilities_component = entity.Get<AbilitiesComponent>();

            const auto active_ability_state = abilities_component.GetActiveAbility();
            if (!active_ability_state)
            {
                return;
            }

            const SkillState& skill_state = active_ability_state->GetCurrentSkillState();
            if (skill_state.state != SkillStateType::kWaiting)
            {
                return;
            }

            if (is_plane_change)
            {
                const TargetingHelper& targeting_helper = world_->GetTargetingHelper();
                const EnumSet<GuidanceType> targeting_guidance = targeting_helper.GetTargetingGuidanceForEntity(entity);
                if (!targeting_helper
                         .DoesEntityMatchesGuidance(targeting_guidance, sender_entity_id, receiver_entity_id))
                {
                    // Guidance does not match
                    return;
                }
            }

            RestoreTargetIfNeeded(entity, receiver_entity);
        });
}

void AbilitySystem::OnSkillFinished(const event_data::Skill& data)
{
    AbilityStateActivatorContext activator_context;
    activator_context.sender_entity_id = data.sender_id;
    activator_context.sender_combat_unit_entity_id = world_->GetCombatUnitParentID(activator_context.sender_entity_id);
    activator_context.ability_type = data.ability_type;
    ChooseAndTryToActivateInnateAbilities(
        activator_context,
        activator_context.sender_combat_unit_entity_id,
        ActivationTriggerType::kOnDeployXSkills);
}

void AbilitySystem::OnGoneHyperactive(const event_data::Hyperactive& data)
{
    AbilityStateActivatorContext activator_context;
    activator_context.sender_entity_id = data.entity_id;
    activator_context.sender_combat_unit_entity_id = world_->GetCombatUnitParentID(activator_context.sender_entity_id);
    activator_context.ability_type = AbilityType::kInnate;
    ChooseAndTryToActivateInnateAbilities(
        activator_context,
        activator_context.sender_combat_unit_entity_id,
        ActivationTriggerType::kOnHyperactive);
}

void AbilitySystem::OnShieldWasHit(const event_data::ShieldWasHit& data)
{
    const EntityID sender_unit = world_->GetCombatUnitParentID(data.hit_by);
    const EntityID receiver_unit = world_->GetCombatUnitParentID(data.receiver_id);
    if (sender_unit != kInvalidEntityID && receiver_unit != kInvalidEntityID)
    {
        AbilityStateActivatorContext activator_context;
        activator_context.sender_entity_id = data.hit_by;
        activator_context.sender_combat_unit_entity_id = sender_unit;
        activator_context.receiver_entity_id = data.receiver_id;
        activator_context.receiver_combat_unit_entity_id = receiver_unit;

        world_->SafeWalkAll(
            [&](const Entity& entity)
            {
                if (EntityHelper::IsACombatUnit(entity))
                {
                    ChooseAndTryToActivateInnateAbilities(
                        activator_context,
                        entity,
                        ActivationTriggerType::kOnShieldHit);
                }
            });
    }
}

bool AbilitySystem::CanActivateAbility(const Entity& sender_entity, const AbilityState& state, const int time_step)
    const
{
    ILLUVIUM_PROFILE_FUNCTION();

    const AbilityData& data = *state.data;

    // Shared between omega and attack
    if (data.is_use_once && state.activation_count > 0)
    {
        return false;
    }

    // Check the attributes of the ability activation trigger
    if (!CanActivateAbilityForTriggerData(sender_entity, state, time_step))
    {
        return false;
    }

    // Attack ability or omega
    return !state.is_active;
}

void AbilitySystem::OnAbilityActivated(const event_data::AbilityActivated& data)
{
    AbilityStateActivatorContext activator_context;
    activator_context.sender_entity_id = data.sender_id;
    activator_context.sender_combat_unit_entity_id = data.combat_unit_sender_id;
    activator_context.ability_type = data.ability_type;

    ChooseAndTryToActivateInnateAbilities(
        activator_context,
        data.sender_id,
        ActivationTriggerType::kOnActivateXAbilities);
}

void AbilitySystem::ChooseAndActivateAnyAbility(
    const Entity& sender_entity,
    const bool can_do_attack,
    const bool can_do_omega)
{
    ILLUVIUM_PROFILE_FUNCTION();

    const EntityID sender_id = sender_entity.GetID();
    if (!sender_entity.Has<AbilitiesComponent>())
    {
        return;
    }
    auto& abilities_component = sender_entity.Get<AbilitiesComponent>();

    if (abilities_component.HasActiveAbility())
    {
        LogDebug(
            sender_id,
            "AbilitySystem::ChooseAndActivateAnyAbility - Has an active ability already, "
            "IGNORING.");
        return;
    }

    // Try to consume innates abilities that are not instant
    if (AbilityStatePtr innate_ability = abilities_component.PopInnateWaitingActivation())
    {
        LogDebug(sender_id, "| Activating innate from Queue.");
        bool out_activated = false;
        bool out_added_to_queue = false;
        TryToActivateInnateOrAddToQueue(sender_entity, innate_ability, &out_activated, &out_added_to_queue);
        if (out_activated)
        {
            return;
        }
    }

    // Try to activate omega ability
    bool did_activate_omega = false;
    if (can_do_omega)
    {
        // Choose omega ability
        if (auto ability = abilities_component.ChooseOmegaAbility())
        {
            did_activate_omega = TryToActivateOmega(sender_entity, ability);
        }
    }

    // Try to activate basis
    if (can_do_attack && !did_activate_omega)
    {
        // Choose attack ability
        if (auto ability = abilities_component.ChooseAttackAbility())
        {
            TryToActivateAttack(sender_entity, ability);
        }
    }
}

void AbilitySystem::TimeStepTimers(const Entity& sender_entity)
{
    const EntityID sender_id = sender_entity.GetID();
    LogDebug(sender_id, "AbilitySystem::TimeStepTimers");
    auto& abilities_component = sender_entity.Get<AbilitiesComponent>();

    FixedPoint attack_speed_ratio = 1_fp;
    if (sender_entity.Has<StatsComponent>())
    {
        // In order to keep the passage of time calculations as simple with the overflow,
        // we always have the duration of abilities calculated using the base attack speed.
        // If the attack speed changes, then we just modify how much time has passed for
        // the skill rather than changing the duration. So, if the duration is 1000ms
        // and the illuvial attack speed doubles, then every time step, we will increment
        // the current time by 200ms, so 0ms -> 200ms -> 400ms -> 600ms, etc...
        // instead of the usual 100ms.
        // For more details, please check the pseudocode at:
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/62128206/Attack+Ability
        // To get a feel for how the numbers actually change in detail, please take a look
        // at the AbilitySystemTestWithStartAttackEvery0Point53Seconds.AttackEvery0Point53Seconds test
        // and the AbilitySystemTestWithComplexSpeedChanges.ComplexSpeedChanges test.
        const StatsData& base_stats = world_->GetBaseStats(sender_entity);

        FixedPoint base_attack_speed = world_->GetClampedAttackSpeed(base_stats.Get(StatType::kAttackSpeed));
        if (base_attack_speed <= 0_fp)
        {
            base_attack_speed = 1_fp;
        }

        const auto& stats_component = sender_entity.Get<StatsComponent>();
        const FixedPoint previous_attack_speed = stats_component.GetPreviousAttackSpeed();
        attack_speed_ratio = (previous_attack_speed * kPrecisionFactorFP) / base_attack_speed;
        LogDebug(
            sender_id,
            "AbilitySystem::TimeStepTimers - previous_attack_speed = {}, base_attack_speed = {}, attack_speed_ratio = "
            "{}",
            previous_attack_speed,
            base_attack_speed,
            attack_speed_ratio);
    }

    // Attack Abilities
    for (auto& state_ability : abilities_component.GetStateAttackAbilities())
    {
        TimeStepTimerState(sender_entity, state_ability, attack_speed_ratio.AsInt());
    }

    // Omega abilities
    for (auto& state_ability : abilities_component.GetStateOmegaAbilities())
    {
        TimeStepTimerState(sender_entity, state_ability, kPrecisionFactor);
    }

    // Innate abilities
    for (auto& state_ability : abilities_component.GetStateInnateAbilities())
    {
        TimeStepTimerState(sender_entity, state_ability, kPrecisionFactor);
    }
}

void AbilitySystem::TimeStepTimerState(
    const Entity& sender_entity,
    const AbilityStatePtr& ability,
    const int attack_speed_ratio)
{
    const EntityID sender_id = sender_entity.GetID();
    // Only active abilities next
    if (!ability->is_active)
    {
        // LogDebug(sender_id, "| ability is NOT active, ignoring");
        return;
    }

    LogDebug(
        sender_id,
        "AbilitySystem::TimeStepTimerState - ability = {} [is_active = {}, attack_speed_ratio = {}, {}]",
        ability->index,
        ability->is_active,
        attack_speed_ratio,
        AbilityTypeContextFormat{*ability});

    // Ability has finished
    if (ability->IsFinished())
    {
        // LogDebug(sender_id, "| ability is FINISHED, ignoring");
        return;
    }

    // Advance ability timer
    ability->IncreaseCurrentTimeMs((kMsPerTimeStep * attack_speed_ratio) / kPrecisionFactor);

    auto& current_skill_state = ability->GetCurrentSkillState();
    LogDebug(
        sender_id,
        "| skill = {}, current_time_ms = {}, ability_current_time_ms = {}, pre_deployment_delay_ms = {}, duration_ms = "
        "{}",
        current_skill_state.index,
        ability->GetCurrentSkillTimeMs(),
        ability->GetCurrentAbilityTimeMs(),
        current_skill_state.pre_deployment_delay_ms,
        current_skill_state.duration_ms);
}

void AbilitySystem::SetSkillState(
    const Entity& sender_entity,
    const SkillStateType new_state_type,
    AbilityStatePtr& ability,
    SkillState& skill)
{
    const EntityID sender_id = sender_entity.GetID();

    skill.state = new_state_type;

    switch (new_state_type)
    {
    case SkillStateType::kWaiting:
    {
        LogDebug(
            sender_id,
            "AbilitySystem::SetSkillState - ability = {}, skill = {} [{}, state = "
            "kWaiting]",
            ability->index,
            skill.index,
            AbilityTypeContextFormat{*ability});

        event_data::Skill event_data;
        event_data.sender_id = sender_id;
        event_data.ability_type = ability->ability_type;
        event_data.ability_index = ability->index;
        event_data.skill_index = skill.index;

        world_->EmitEvent<EventType::kSkillWaiting>(event_data);
        break;
    }
    case SkillStateType::kDeploying:
    {
        LogDebug(
            sender_id,
            "AbilitySystem::SetSkillState - ability = {}, skill = {} [{}, state = "
            "kDeploying]",
            ability->index,
            skill.index,
            AbilityTypeContextFormat{*ability});

        // Target and deploy skill
        TargetAndDeploySkill(sender_entity, ability, skill);

        // Advance in the same time step
        if (ability->IsInstant())
        {
            LogDebug(sender_id, "| ability is INSTANT, time stepping again");
            TimeStepAbility(sender_entity, ability);
        }
        break;
    }
    case SkillStateType::kChanneling:
    {
        LogDebug(
            sender_id,
            "AbilitySystem::SetSkillState - ability = {}, skill = {} [{}, state = "
            "kChanneling]",
            ability->index,
            skill.index,
            AbilityTypeContextFormat{*ability});

        // Emit event
        {
            event_data::Skill event_data;
            event_data.sender_id = sender_id;
            event_data.ability_type = ability->ability_type;
            event_data.ability_index = ability->index;
            event_data.skill_index = skill.index;

            world_->EmitEvent<EventType::kSkillChanneling>(event_data);
        }

        // Advance in the same time step
        if (ability->IsInstant())
        {
            LogDebug(sender_id, "| ability is INSTANT, time stepping again");
            TimeStepAbility(sender_entity, ability);
        }
        break;
    }
    case SkillStateType::kFinished:
    {
        LogDebug(
            sender_id,
            "AbilitySystem::SetSkillState - ability = {}, skill = {} [{}, state = "
            "kFinished]",
            ability->index,
            skill.index,
            AbilityTypeContextFormat{*ability});

        if (world_->HasEntity(sender_id))
        {
            Entity& original_sender = world_->GetByID(sender_id);
            if (original_sender.Has<AbilitiesComponent>())
            {
                auto& abilities_component = original_sender.Get<AbilitiesComponent>();
                abilities_component.IncrementDeployedSkillsCount(ability->ability_type);
            }
        }

        // Emit event
        {
            event_data::Skill event_data;
            event_data.sender_id = sender_id;
            event_data.ability_type = ability->ability_type;
            event_data.ability_index = ability->index;
            event_data.skill_index = skill.index;

            world_->EmitEvent<EventType::kSkillFinished>(event_data);
        }

        // Advance to the next skill in the list
        ability->IncrementCurrentSkillIndex();

        // Ability finished
        if (ability->IsFinished())
        {
            Deactivate(sender_entity, ability);
        }
        else
        {
            if (!TryInitSkillTargeting(sender_entity, ability, false))
            {
                // We are not able to start targeting of the next skill, so fail
                HandleTargetingFailure(sender_entity, ability);
                return;
            }

            // Can we apply the next skill in the same time step?
            if (ability->CanDeployCurrentSkillInstantly())
            {
                LogDebug(
                    sender_id,
                    "| current skill = {} has INSTANT deployment, stepping ability again",
                    ability->GetCurrentSkillIndex());
                TryToStepAbility(sender_entity, ability);
            }
        }

        break;
    }

    // Nothing to do, happens when we Activate/Deactivate an ability
    case SkillStateType::kNone:
    {
        LogDebug(
            sender_id,
            "AbilitySystem::SetSkillState - ability = {}, skill = {}  [{}, state "
            "= "
            "kNone]",
            ability->index,
            skill.index,
            AbilityTypeContextFormat{*ability});
        break;
    }
    }
}

void AbilitySystem::UpdateSkillTargeting(
    const Entity& sender_entity,
    const AbilityStatePtr& ability,
    SkillState& skill,
    const bool merge)
{
    SkillTargetingState& targeting_state = skill.targeting_state;
    const TargetingHelper& targeting_helper = world_->GetTargetingHelper();
    TargetingHelper::SkillTargetFindResult find_result;
    targeting_helper.GetEntitiesOfSkillTarget(sender_entity.GetID(), ability.get(), *skill.data, {}, &find_result);

    if (merge)
    {
        if (skill.data->targeting.num != 0 && find_result.receiver_ids.size() > skill.data->targeting.num)
        {
            LogErr(
                sender_entity.GetID(),
                "AbilitySystem::UpdateSkillTargeting - find returned more targets than allowed");
            assert(false);
            return;
        }

        // Merge new targets with a current state
        targeting_state.MergeValidTargets(*world_, find_result, skill.data->targeting.num);

        // NOTE: targeting.num == 0 means infinite
        if (skill.data->targeting.num && targeting_state.GetTotalTargetsSize() > skill.data->targeting.num)
        {
            LogErr(
                sender_entity.GetID(),
                "AbilitySystem::UpdateSkillTargeting - After merge we have more targets than allowed");
        }
    }
    else
    {
        targeting_state.CreateFromFindResult(*world_, find_result);
    }

    // Send target update events
    if (EntityHelper::IsACombatUnit(sender_entity) && !targeting_state.available_targets.empty())
    {
        if (targeting_state.available_targets.size() == 1 &&
            targeting_state.available_targets.count(targeting_state.true_sender_id) > 0)
        {
            // We don't need to sent targeting updates for self targeted skills
            return;
        }

        const bool instant_innate = ability->IsInstant() && ability->ability_type == AbilityType::kInnate;
        const bool rotate_to_target = skill.data->effect_package.attributes.rotate_to_target || !instant_innate;

        event_data::SkillTargets event_data;
        event_data.sender_id = sender_entity.GetID();
        event_data.ability_type = ability->ability_type;
        event_data.ability_index = ability->index;
        event_data.skill_index = skill.index;
        event_data.rotate_to_target = rotate_to_target;
        event_data.receiver_ids = targeting_state.GetAvailableTargetsVector();
        event_data.deployment_locations = targeting_state.GetLostTargetsLastKnownPosition();

        world_->EmitEvent<EventType::kSkillTargetsUpdated>(event_data);
    }
}

void AbilitySystem::TryToStepAbility(const Entity& sender_entity, AbilityStatePtr& ability)
{
    // Can't execute this ability, try again later
    if (!ability->is_active)
    {
        return;
    }

    TimeStepAbility(sender_entity, ability);
}

void AbilitySystem::TimeStepAbility(const Entity& sender_entity, AbilityStatePtr& ability)
{
    ILLUVIUM_PROFILE_FUNCTION();

    const EntityID sender_id = sender_entity.GetID();
    assert(ability);
    LogDebug(
        sender_id,
        "AbilitySystem::TimeStepAbility - ability = {} [total_duration_ms = {}, "
        "current_time_ms = {}, {}]",
        ability->index,
        ability->total_duration_ms,
        ability->GetCurrentAbilityTimeMs(),
        AbilityTypeContextFormat{*ability});

    // Execute the payload
    if (ability->IsFinished())
    {
        LogDebug(sender_id, "| Ability is already FINISHED. IGNORING");
        return;
    }

    // Interrupt channeling on negative state
    if (EntityHelper::CanInterruptAbility(sender_entity, ability->ability_type))
    {
        ForceFinishAllSkills(sender_entity, ability);
        return;
    }

    // Execute current skill
    auto& current_skill_state = ability->GetCurrentSkillState();
    LogDebug(
        sender_id,
        "| skill = {}, pre_deployment_delay_ms = {}, duration_ms = {}",
        current_skill_state.index,
        current_skill_state.pre_deployment_delay_ms,
        current_skill_state.duration_ms);

    const int current_skill_time_ms = ability->GetCurrentSkillTimeMs();
    switch (current_skill_state.state)
    {
    case SkillStateType::kWaiting:
    {
        LogDebug(sender_id, "| state = kWaiting");

        // Can we deploy?
        if (current_skill_time_ms >= current_skill_state.pre_deployment_delay_ms)
        {
            SetSkillState(sender_entity, SkillStateType::kDeploying, ability, current_skill_state);
        }
        break;
    }

    case SkillStateType::kDeploying:
    {
        LogDebug(sender_id, "| state = kDeploying");

        // Needs to channel
        if (ability->CanChannelCurrentSkill())
        {
            LogDebug(sender_id, "| channel_time_ms = {}", current_skill_state.channel_time_ms);
            SetSkillState(sender_entity, SkillStateType::kChanneling, ability, current_skill_state);
            break;
        }

        // Is finished
        if (!ability->CanDeployCurrentSkill())
        {
            SetSkillState(sender_entity, SkillStateType::kFinished, ability, current_skill_state);
        }
        break;
    }

    case SkillStateType::kChanneling:
    {
        LogDebug(sender_id, "| state = kChanneling");

        // Interrupt channeling on negative state
        if (!EntityHelper::CanActivateAbility(sender_entity, ability->ability_type))
        {
            SetSkillState(sender_entity, SkillStateType::kFinished, ability, current_skill_state);
            break;
        }

        // Is finished
        if (!ability->CanChannelCurrentSkill())
        {
            // Can finish?
            if (ability->CanDeployCurrentSkill())
            {
                // Still deploying
                SetSkillState(sender_entity, SkillStateType::kDeploying, ability, current_skill_state);
            }
            else
            {
                // Finished
                SetSkillState(sender_entity, SkillStateType::kFinished, ability, current_skill_state);
            }
        }
        break;
    }

    case SkillStateType::kFinished:
    {
        LogDebug(sender_id, "| state = kFinished");
        assert(false);  // Should never happen
        break;
    }

    case SkillStateType::kNone:
    {
        LogDebug(sender_id, "| state = kNone");
        if (ability->CanDeployCurrentSkillInstantly())
        {
            LogDebug(sender_id, "| skill has INSTANT deployment, Deploying right now ");

            // No wait time, deploy in this step
            SetSkillState(sender_entity, SkillStateType::kDeploying, ability, current_skill_state);
        }
        else
        {
            // Has wait time, wait for the pre deployment delay
            SetSkillState(sender_entity, SkillStateType::kWaiting, ability, current_skill_state);
        }
        break;
    }
    }
}

void AbilitySystem::ForceFinishAllSkills(const Entity& sender_entity, AbilityStatePtr& ability)
{
    ILLUVIUM_PROFILE_FUNCTION();

    const EntityID sender_id = sender_entity.GetID();
    for (SkillState& skill_state : ability->skills)
    {
        LogDebug(
            sender_id,
            "| Ability interrupted - ability = {}, skill = {} [{}]",
            ability->index,
            skill_state.index,
            AbilityTypeContextFormat{*ability});

        if (skill_state.state != SkillStateType::kFinished)
        {
            SetSkillState(sender_entity, SkillStateType::kFinished, ability, skill_state);
        }
    }

    if (sender_entity.Has<DecisionComponent>())
    {
        auto& decision_component = sender_entity.Get<DecisionComponent>();
        decision_component.SetPreviousAction(DecisionNextAction::kNone);
    }

    // Emit event
    event_data::AbilityInterrupted event_data;
    event_data.sender_id = sender_entity.GetID();
    event_data.ability_type = ability->ability_type;
    event_data.ability_index = ability->index;
    world_->EmitEvent<EventType::kAbilityInterrupted>(event_data);
}

bool AbilitySystem::HandleTargetingFailure(const Entity& sender_entity, AbilityStatePtr& ability)
{
    ILLUVIUM_PROFILE_FUNCTION();

    const EntityID sender_entity_id = sender_entity.GetID();
    auto& current_skill_state = ability->GetCurrentSkillState();

    // Check for propagation type
    if (current_skill_state.data->effect_package.attributes.propagation.type != EffectPropagationType::kNone)
    {
        // Use self for propagation as a dummy entity
        // Propagations ignore agency, but their skills don't
        LogDebug(sender_entity_id, "| No targets for propagation, using self");

        TargetingHelper::SkillTargetFindResult find_result{};
        find_result.receiver_ids.push_back(sender_entity_id);
        find_result.true_sender_id = sender_entity_id;
        current_skill_state.targeting_state.CreateFromFindResult(*world_, find_result);

        return false;
    }

    // Force finish all skills
    ForceFinishAllSkills(sender_entity, ability);
    return true;
}

void AbilitySystem::CancelAttackOnFullEnergy(const Entity& entity)
{
    if (!entity.IsActive()) return;
    if (!entity.Has<AbilitiesComponent, StatsComponent>()) return;

    // No need to interrupt attack if there is no omega
    const auto& abilities_component = entity.Get<AbilitiesComponent>();
    if (abilities_component.GetAbilities(AbilityType::kOmega).data.abilities.empty()) return;

    // Only if there is active attack ability
    auto active_ability = abilities_component.GetActiveAbility();
    if (active_ability == nullptr || active_ability->ability_type != AbilityType::kAttack) return;

    // Only when energy is full
    if (!EntityHelper::HasEnoughEnergyForOmega(entity)) return;

    // Stop attack ability only if it is possible to activate omega
    if (!EntityHelper::CanActivateAbility(entity, AbilityType::kOmega)) return;

    // Have to set next action for decision component otherwise attack ability will be activated again
    if (auto* decision_component = entity.GetPtr<DecisionComponent>())
    {
        decision_component->SetNextAction(DecisionNextAction::kOmegaAbility);
        decision_component->SetPreviousAction(DecisionNextAction::kAttackAbility);
    }

    ForceFinishAllSkills(entity, active_ability);
}

void AbilitySystem::TargetAndDeploySkill(const Entity& sender_entity, AbilityStatePtr& ability, SkillState& skill)
{
    const EntityID sender_id = sender_entity.GetID();
    LogDebug(
        sender_id,
        "AbilitySystem::TargetAndDeploySkill - ability = {} [{}, skill = {}, "
        "targeting = {}, guidances = {}]",
        ability->index,
        AbilityTypeContextFormat{*ability},
        skill.index,
        skill.data->targeting.type,
        skill.data->targeting.guidance);

    // Skill was already deployed, ignore
    if (skill.is_deployed)
    {
        LogDebug(sender_id, "| Skill is already deployed. IGNORING");
        return;
    }

    // Emit event
    {
        event_data::Skill event_data;
        event_data.sender_id = sender_id;
        event_data.ability_type = ability->ability_type;
        event_data.ability_index = ability->index;
        event_data.skill_index = skill.index;

        world_->EmitEvent<EventType::kSkillDeploying>(event_data);
    }

    // Update targets before deployment if allowed
    if (ability->CanAlwaysRetarget())
    {
        UpdateSkillTargeting(sender_entity, ability, skill, true);
    }

    const auto& true_sender_entity_id = skill.targeting_state.true_sender_id;
    if (!world_->HasEntity(true_sender_entity_id))
    {
        LogErr(
            sender_id,
            "| true_sender_entity_id = {} does not exist. TryInitSkillTargeting was not called?",
            true_sender_entity_id);
        assert(false);
        return;
    }

    const auto& true_sender_entity = world_->GetByID(true_sender_entity_id);
    if (true_sender_entity.Has<FocusComponent>())
    {
        auto& focus_component = true_sender_entity.Get<FocusComponent>();
        if (focus_component.IsFocusActive())
        {
            // Check if focus is a receiver
            if (skill.targeting_state.available_targets.count(focus_component.GetFocusID()) > 0)
            {
                // Reset unreachable only once we can reach something
                focus_component.ResetUnreachableFocusTimeSteps();

                // Clear unreachable state of other entities as they may become reachable
                focus_component.ClearUnreachable();
            }
        }
    }

    // Deploy to targets
    DeploySkill(true_sender_entity, sender_id, ability, skill);
}

void AbilitySystem::GetFinalReceiverIDs(
    const std::shared_ptr<SkillData>& skill_data,
    const std::vector<EntityID>& receiver_ids,
    std::vector<EntityID>* out_final_receiver_ids)
{
    // Nothing to do
    if (receiver_ids.empty())
    {
        return;
    }

    assert(out_final_receiver_ids);
    *out_final_receiver_ids = receiver_ids;

    // No optional attributes set, nothing to do
    const EffectPackageAttributes& attributes = skill_data->effect_package.attributes;
    if (!attributes.cumulative_damage && !attributes.split_damage)
    {
        return;
    }

    // If the number of existing targets is less than the number of targets we want to target
    // We have the right amount of targets, nothing to do
    if (out_final_receiver_ids->size() >= skill_data->targeting.num)
    {
        return;
    }

    // The total number of targets we need to add
    const size_t num_targets_to_add = skill_data->targeting.num - out_final_receiver_ids->size();

    if (attributes.cumulative_damage)
    {
        // Target the first receiver multiple times
        for (size_t i = 0; i < num_targets_to_add; i++)
        {
            out_final_receiver_ids->push_back(receiver_ids.at(0));
        }
    }
    else if (attributes.split_damage)
    {
        // Split the same damage between targets and wrap around if we don't have enough receiver_ids
        const size_t receiver_ids_size = receiver_ids.size();
        size_t receiver_index = 0;
        size_t current_targets_to_add = num_targets_to_add;
        while (current_targets_to_add)
        {
            out_final_receiver_ids->push_back(receiver_ids.at(receiver_index));

            // Wrap around the receiver_ids so that we don't get an invalid index
            receiver_index = (receiver_index + 1) % receiver_ids_size;

            // Move next
            current_targets_to_add--;
        }
    }
}

void AbilitySystem::DeployDirectSkill(
    const Entity& sender_entity,
    AbilityStatePtr& ability,
    SkillState& skill,
    const std::vector<EntityID>& filtered_receiver_ids,
    const std::shared_ptr<SkillData>& skill_data_with_empowers)
{
    if (skill.data->spread_effect_package)
    {
        skill_data_with_empowers->effect_package.SpreadEvenlyForInstantMeteredTypes(filtered_receiver_ids.size());
    }

    const SkillTargetingState& targeting_state = skill.targeting_state;
    // Intentionally copy this map - gonna modify it later
    auto saved_targets_info = targeting_state.targets_saved_data;

    for (const EntityID receiver_id : filtered_receiver_ids)
    {
        // Add empower, save original pointer to the skill data
        const std::shared_ptr<SkillData> original_skill_data = skill.data;
        skill.data = skill_data_with_empowers;

        ApplyEffectPackage(
            sender_entity,
            ability,
            skill.data->effect_package,
            skill.CheckAllIfIsCritical(),
            receiver_id);

        // Restore original without empowers
        skill.data = original_skill_data;
    }

    // Check if there is new entities at previous locations
    std::vector<EntityID> new_targets_at_saved_locations;

    {
        const TargetingHelper& targeting_helper = world_->GetTargetingHelper();
        TargetingHelper::SkillTargetFindResult find_results;
        targeting_helper.GetEntitiesOfSkillTarget(sender_entity.GetID(), ability.get(), *skill.data, {}, &find_results);
        new_targets_at_saved_locations = targeting_helper.FilterEntitiesForGuidance(
            skill.data->deployment.guidance,
            sender_entity.GetID(),
            find_results.receiver_ids);
    }

    // Filter out entities that does not intersect any fallback location
    VectorHelper::EraseValuesForCondition(
        new_targets_at_saved_locations,
        [&](const EntityID receiver_id)
        {
            if (saved_targets_info.count(receiver_id) != 0)
            {
                return true;
            }

            const Entity& receiver = world_->GetByID(receiver_id);
            if (receiver.Has<PositionComponent>())
            {
                const PositionComponent& receiver_position_component = receiver.Get<PositionComponent>();
                const HexGridPosition receiver_position = receiver_position_component.GetPosition();
                for (auto& [another_id, fallback_location] : saved_targets_info)
                {
                    const auto distance = (receiver_position - fallback_location).Length();
                    if (distance < receiver_position_component.GetRadius())
                    {
                        return false;
                    }
                }
            }

            return true;
        });

    // Apply effect package to newly discovered targets at location of previous targets
    for (const EntityID receiver_id : new_targets_at_saved_locations)
    {
        // Add empower, save original pointer to the skill data
        const std::shared_ptr<SkillData> original_skill_data = skill.data;
        skill.data = skill_data_with_empowers;

        ApplyEffectPackage(
            sender_entity,
            ability,
            skill.data->effect_package,
            skill.CheckAllIfIsCritical(),
            receiver_id);

        // Restore original without empowers
        skill.data = original_skill_data;

        // To not send at same place twice
        saved_targets_info.erase(receiver_id);
    }

    // Remove entities that already received this effect package
    for (const EntityID receiver_id : filtered_receiver_ids)
    {
        saved_targets_info.erase(receiver_id);
    }

    // Apply effect package to fallback locations.
    // These event will be ignored by simulation but the game still needs them
    // (to finish skill gracefully / spawn visual effects at right place).
    for (const auto& [dead_receiver_id, fallback_location] : saved_targets_info)
    {
        // Add empower, save original pointer to the skill data
        const std::shared_ptr<SkillData> original_skill_data = skill.data;
        skill.data = skill_data_with_empowers;

        ApplyEffectPackage(
            sender_entity,
            ability,
            skill.data->effect_package,
            skill.CheckAllIfIsCritical(),
            fallback_location);

        // Restore original without empowers
        skill.data = original_skill_data;
    }
}

void AbilitySystem::DeploySkill(
    const Entity& sender_entity,
    const EntityID original_sender_id,
    AbilityStatePtr& ability,
    SkillState& skill)
{
    const EntityID sender_id = sender_entity.GetID();
    skill.is_deployed = true;

    SkillTargetingState& targeting_state = skill.targeting_state;
    const std::vector<EntityID> receiver_ids = targeting_state.GetAvailableTargetsVector();
    const std::vector<HexGridPosition> fallback_locations = targeting_state.GetLostTargetsLastKnownPosition();

    LogDebug(
        sender_id,
        "| DeploySkill - deployment = {}, guidances = {}, targets = {}, fallback_locations = {}",
        skill.data->deployment.type,
        skill.data->deployment.guidance,
        receiver_ids,
        fallback_locations);

    if (!targeting_state.IsValidFor(skill.data->deployment.type))
    {
        if (HandleTargetingFailure(sender_entity, ability))
        {
            return;
        }
    }

    // Energy gain for attack abilities
    const bool is_sender_a_combat_unit = EntityHelper::IsACombatUnit(sender_entity);
    if (is_sender_a_combat_unit && ability->ability_type == AbilityType::kAttack &&
        !EntityHelper::IsAShield(*world_, original_sender_id))
    {
        event_data::ApplyEnergyGain event_data;
        event_data.caused_by_id = sender_id;
        event_data.receiver_id = sender_id;
        event_data.energy_gain_type = EnergyGainType::kAttack;

        world_->EmitEvent<EventType::kApplyEnergyGain>(event_data);
    }

    // Find out the true ability type origin
    const AbilityType combat_unit_sender_ability_type =
        is_sender_a_combat_unit ? ability->ability_type : ability->data->source_context.combat_unit_ability_type;
    if (sender_entity.Has<FilteringComponent>())
    {
        auto& filter = sender_entity.Get<FilteringComponent>();
        filter.AddOldTargets(receiver_ids);
    }

    // Filter entities for deployment guidance
    const TargetingHelper& targeting_helper = world_->GetTargetingHelper();
    const auto filtered_receiver_ids =
        targeting_helper.FilterEntitiesForGuidance(skill.data->deployment.guidance, sender_id, receiver_ids);

    // Create new skill with empowers
    const EffectPackageHelper& effect_package_helper = world_->GetEffectPackageHelper();
    EffectPackage empower_effect_package;
    effect_package_helper.GetEmpowerEffectPackageForAbility(
        sender_entity,
        ability->ability_type,
        skill.CheckAllIfIsCritical(),
        &empower_effect_package,
        &ability->consumable_empowers_used);
    std::shared_ptr<SkillData> skill_data_with_empowers =
        effect_package_helper.CreateSkillFromEmpowerEffectPackage(skill.data, empower_effect_package);

    const bool skill_is_critical = skill.CheckAllIfIsCritical();

    // Update the context
    SourceContextData context = ability->data->source_context;
    context.combat_unit_ability_type = combat_unit_sender_ability_type;
    context.ability_name = ability->data->name;
    context.skill_name = skill.data->name;

    // Different depending on the SkillDeploymentType
    switch (skill.data->deployment.type)
    {
    case SkillDeploymentType::kDirect:
    {
        DeployDirectSkill(sender_entity, ability, skill, filtered_receiver_ids, skill_data_with_empowers);
        break;
    }
    case SkillDeploymentType::kProjectile:
    {
        std::vector<EntityID> final_receiver_ids;

        if (skill.data->spread_effect_package)
        {
            skill_data_with_empowers->effect_package.SpreadEvenlyForInstantMeteredTypes(filtered_receiver_ids.size());
        }
        else
        {
            GetFinalReceiverIDs(skill.data, filtered_receiver_ids, &final_receiver_ids);
        }

        LogDebug(sender_id, "| final_receiver_ids = {}", final_receiver_ids);

        ProjectileData projectile_data;
        projectile_data.source_context = context;

        const auto& sender_entity_position_component = sender_entity.Get<PositionComponent>();
        const HexGridPosition& from_position = sender_entity_position_component.GetPosition();

        // Spawn projectile
        projectile_data.sender_id = sender_id;
        projectile_data.receiver_id = kInvalidEntityID;

        // Sets the data from the skill
        projectile_data.radius_units = skill.data->projectile.size_units;
        projectile_data.move_speed_sub_units = skill.data->projectile.speed_sub_units;
        projectile_data.is_blockable = skill.data->projectile.is_blockable;
        projectile_data.is_homing = skill.data->projectile.is_homing;
        projectile_data.apply_to_all = skill.data->projectile.apply_to_all;
        projectile_data.continue_after_target = skill.data->projectile.continue_after_target;
        projectile_data.is_critical = skill_is_critical;
        projectile_data.deployment_guidance = skill.data->deployment.guidance;

        // Copy the pointer to the skill
        projectile_data.skill_data = skill_data_with_empowers;

        for (const EntityID receiver_id : final_receiver_ids)
        {
            projectile_data.receiver_id = receiver_id;

            // Set movement to be direct
            HexGridPosition target = kInvalidHexHexGridPosition;
            if (world_->HasEntity(projectile_data.receiver_id))
            {
                const auto& receiver_entity = world_->GetByID(projectile_data.receiver_id);
                target = receiver_entity.Get<PositionComponent>().GetPosition();
            }

            // Spawn the projectile at the entity position
            EntityFactory::SpawnProjectile(*world_, sender_entity.GetTeam(), from_position, target, projectile_data);
        }

        // No receiver for location projectiles
        projectile_data.receiver_id = kInvalidEntityID;
        // No homing for location projectiles
        projectile_data.is_homing = false;

        // Spawn projectiles to fallback locations
        for (const HexGridPosition& location : fallback_locations)
        {
            // Spawn the projectile at the entity position
            EntityFactory::SpawnProjectile(*world_, sender_entity.GetTeam(), from_position, location, projectile_data);
        }
        break;
    }

    case SkillDeploymentType::kZone:
    {
        ZoneData zone_data;
        zone_data.source_context = context;

        // Spawn zone
        zone_data.sender_id = original_sender_id;
        zone_data.original_sender_id = original_sender_id;

        // Sets the data from the skill
        zone_data.shape = skill.data->zone.shape;
        zone_data.radius_sub_units = Math::UnitsToSubUnits(skill.data->zone.radius_units);
        zone_data.max_radius_sub_units = Math::UnitsToSubUnits(skill.data->zone.max_radius_units);
        zone_data.width_sub_units = Math::UnitsToSubUnits(skill.data->zone.width_units);
        zone_data.height_sub_units = Math::UnitsToSubUnits(skill.data->zone.height_units);
        zone_data.duration_ms = skill.data->zone.duration_ms;
        zone_data.frequency_ms = skill.data->zone.frequency_ms;
        zone_data.direction_degrees = skill.data->zone.direction_degrees;
        zone_data.movement_speed_sub_units_per_time_step = skill.data->zone.movement_speed_sub_units_per_time_step;
        zone_data.growth_rate_sub_units_per_time_step = skill.data->zone.growth_rate_sub_units_per_time_step;
        zone_data.apply_once = skill.data->zone.apply_once;
        zone_data.global_collision_id = skill.data->zone.global_collision_id;
        zone_data.is_critical = skill_is_critical;
        zone_data.destroy_with_sender = skill.data->zone.destroy_with_sender;
        zone_data.skip_activations = skill.data->zone.skip_activations;

        // Determine whether the zone is channeled based on skill channel_time_ms
        zone_data.is_channeled = skill.channel_time_ms > 0;

        // Copy the pointer to the skill
        zone_data.skill_data = skill_data_with_empowers;

        for (const EntityID receiver_id : receiver_ids)
        {
            const auto& receiver_entity = world_->GetByID(receiver_id);
            const auto& receiver_entity_position_component = receiver_entity.Get<PositionComponent>();
            const HexGridPosition& receiver_position = receiver_entity_position_component.GetPosition();

            if (skill.data->zone.attach_to_target)
            {
                zone_data.attach_to_entity = receiver_id;
            }

            // Spawn the zone at the target position
            EntityFactory::SpawnZone(*world_, sender_entity.GetTeam(), receiver_position, zone_data);
        }

        if (!skill.data->zone.attach_to_target)
        {
            // Spawn Zone to fallback locations
            for (const HexGridPosition& location : fallback_locations)
            {
                // Spawn the zone at the target position
                EntityFactory::SpawnZone(*world_, sender_entity.GetTeam(), location, zone_data);
            }
        }
        break;
    }

    case SkillDeploymentType::kBeam:
    {
        BeamData beam_data;
        beam_data.source_context = context;

        // Get sender position
        const auto& sender_position_component = sender_entity.Get<PositionComponent>();
        const auto& sender_position = sender_position_component.GetPosition();

        // Spawn beam
        beam_data.sender_id = sender_id;

        // Sets the data from the skill
        beam_data.width_sub_units = Math::UnitsToSubUnits(skill.data->beam.width_units);
        beam_data.frequency_ms = skill.data->beam.frequency_ms;
        beam_data.apply_once = skill.data->beam.apply_once;
        beam_data.is_blockable = skill.data->beam.is_blockable;
        beam_data.is_homing = skill.data->beam.is_homing;
        beam_data.block_allegiance = skill.data->beam.block_allegiance;

        // Copy the pointer to the skill
        beam_data.skill_data = skill_data_with_empowers;
        beam_data.is_critical = skill_is_critical;

        for (const EntityID receiver_id : receiver_ids)
        {
            beam_data.receiver_id = receiver_id;

            const auto& receiver_entity = world_->GetByID(receiver_id);
            const auto& receiver_entity_position_component = receiver_entity.Get<PositionComponent>();
            const HexGridPosition& receiver_position = receiver_entity_position_component.GetPosition();

            // Spawn the beam at the target position
            EntityFactory::SpawnBeam(*world_, sender_entity.GetTeam(), sender_position, receiver_position, beam_data);
        }

        // Invalid receiver for location beams
        beam_data.receiver_id = kInvalidEntityID;

        // Spawn Beam to fallback locations
        for (const HexGridPosition& location : fallback_locations)
        {
            // Spawn the beam at the target position
            EntityFactory::SpawnBeam(*world_, sender_entity.GetTeam(), sender_position, location, beam_data);
        }
        break;
    }
    // TODO Implement deployment using locations
    case SkillDeploymentType::kDash:
    {
        // A dash needs to have exactly one receiver, otherwise it doesn't make sense
        if (receiver_ids.size() == 1)
        {
            const EntityID receiver_id = receiver_ids.empty() ? kInvalidEntityID : receiver_ids[0];

            DashData dash_data;
            dash_data.source_context = context;

            // Spawn dash
            dash_data.sender_id = sender_id;
            dash_data.receiver_id = receiver_id;

            // Sets the data from the skill
            dash_data.apply_to_all = skill.data->dash.apply_to_all;
            dash_data.land_behind = skill.data->dash.land_behind;

            // Copy the pointer to the skill
            dash_data.skill_data = skill_data_with_empowers;

            // Spawn the dash
            EntityFactory::SpawnDash(*world_, sender_entity.GetTeam(), dash_data, skill.duration_ms);
        }
        else
        {
            LogErr(sender_id, "| Dash with receiver count != 1 AND !dash_jump_across");
            break;
        }
        break;
    }

    case SkillDeploymentType::kSpawnedCombatUnit:
    {
        const GridHelper& grid_helper = world_->GetGridHelper();

        std::vector<HexGridPosition> spawn_locations;

        for (const EntityID receiver_id : receiver_ids)
        {
            HexGridPosition spawn_location = kInvalidHexHexGridPosition;

            // Get a valid location
            const auto& receiver_entity = world_->GetByID(receiver_id);
            if (skill.data->spawn.on_reserved_position)
            {
                auto& receiver_position_component = receiver_entity.Get<PositionComponent>();
                spawn_location = receiver_position_component.GetReservedPosition();
                receiver_position_component.ClearReservedPosition();
            }
            else
            {
                spawn_location = grid_helper.GetOpenPositionNear(
                    receiver_entity,
                    skill.data->spawn.direction,
                    skill.data->spawn.combat_unit->radius_units);
            }

            // Could not find a space to spawn
            if (spawn_location == kInvalidHexHexGridPosition)
            {
                // TODO(Lionel): Handle this unlikely case of the grid being full
                LogErr("| No space to spawn entity");
                break;
            }

            const auto& receiver_position_comp = receiver_entity.Get<PositionComponent>();
            const auto& sender_position_comp = sender_entity.Get<PositionComponent>();

            LogDebug(
                "| Added pet spawn location {}, parent {}, target {}",
                spawn_location,
                sender_position_comp.GetPosition(),
                receiver_position_comp.GetPosition());

            spawn_locations.push_back(spawn_location);
        }

        for (const auto& location : fallback_locations)
        {
            // Get a valid location
            const HexGridPosition& spawn_location = grid_helper.GetOpenPositionNearLocation(
                location,
                skill.data->spawn.direction,
                0,
                skill.data->spawn.combat_unit->radius_units);

            // Could not find a space to spawn
            if (spawn_location == kInvalidHexHexGridPosition)
            {
                LogErr("| No space to spawn entity");
                break;
            }

            spawn_locations.push_back(spawn_location);
        }

        for (const auto& spawn_location : spawn_locations)
        {
            // Spawn new creature
            FullCombatUnitData full_data;

            // Set the unique id of the child to be the <parent_id>_<parent_spawn_count>
            const EntityID combat_unit_parent_id = world_->GetCombatUnitParentID(sender_entity);
            if (world_->HasEntity(combat_unit_parent_id))
            {
                const Entity& combat_unit_parent_entity = world_->GetByID(combat_unit_parent_id);
                assert(EntityHelper::IsACombatUnit(combat_unit_parent_entity));

                full_data.instance.id = fmt::format(
                    "{}_pet_{}",
                    EntityHelper::GetUniqueID(combat_unit_parent_entity),
                    combat_unit_parent_entity.Get<CombatUnitComponent>().GetChildrenSpawnedCount());
            }

            full_data.instance.team = sender_entity.GetTeam();
            full_data.instance.position = spawn_location;
            full_data.data = *skill.data->spawn.combat_unit;

            auto* new_entity = EntityFactory::SpawnCombatUnit(*world_, full_data, sender_id);

            if (!new_entity)
            {
                // Give it another try
                HexGridPosition another_spawn_location = world_->GetGridHelper().GetOpenPositionNearLocation(
                    spawn_location,
                    HexGridCardinalDirection::kRight,
                    1,
                    full_data.data.radius_units);
                full_data.instance.position = another_spawn_location;
                new_entity = EntityFactory::SpawnCombatUnit(*world_, full_data, sender_id);
                if (!new_entity)
                {
                    LogErr(
                        "| Failed to spawn CombatUnit at {} (also retried at {})",
                        spawn_location,
                        another_spawn_location);
                }

                break;
            }
        }
        break;
    }
    default:
        break;
    }

    // Special case when we have multiple skills, ignore the first one
    // TODO(vampy): Be more smart about empowers with multiple skills
    // https://discord.com/channels/760344898200666112/972756694595686430/977799204799909918
    if (skill.index == 0 && ability->skills.size() > 1 && skill.data->targeting.type == SkillTargetingType::kSelf)
    {
        return;
    }
    if (ability->applied_effect_package_attributes)
    {
        return;
    }

    // Apply the effect package from the skill with empowers
    ApplyEffectPackageAttributes(sender_entity, skill_data_with_empowers->effect_package.attributes, context);
    ability->applied_effect_package_attributes = true;
}

void AbilitySystem::ApplyEffectPackage(
    const Entity& sender_entity,
    const AbilityStatePtr& ability,
    const EffectPackage& effect_package,
    const bool is_critical,
    const EntityID receiver_id)
{
    const EntityID sender_id = sender_entity.GetID();
    const auto& receiver_entity = world_->GetByID(receiver_id);
    const auto& receiver_stats_component = receiver_entity.Get<StatsComponent>();

    const StatsData sender_live_stats = world_->GetLiveStats(sender_entity);
    const StatsData receiver_live_stats = world_->GetLiveStats(receiver_entity);

    // Did we hit the target
    bool is_hit = true;
    bool is_dodged = false;

    const EffectPackageAttributes& attributes = effect_package.attributes;

    // Is combat unit?
    const bool is_sender_a_combat_unit = EntityHelper::IsACombatUnit(sender_entity);
    EntityID combat_unit_sender_id = world_->GetCombatUnitParentID(sender_entity);

    // Handle invalid parent
    if (!EntityHelper::CanApplyEffect(sender_entity))
    {
        LogErr(sender_id, "AbilitySystem::ApplyEffectPackage() entity {} cant apply effects", sender_entity.GetID());
        assert(false);
    }

    const AbilityType combat_unit_sender_ability_type =
        is_sender_a_combat_unit ? ability->ability_type : ability->data->source_context.combat_unit_ability_type;

    // Abilities that comes from the shield triggers should not use hit chance
    const bool source_is_shield = ability->data->source_context.Has(SourceContextType::kShield);

    if (attributes.use_hit_chance && !source_is_shield)
    {
        const Entity& combat_unit_sender = world_->GetByID(combat_unit_sender_id);
        const bool sender_has_truesight = EntityHelper::HasTruesight(combat_unit_sender);
        const bool sender_is_blinded = EntityHelper::IsBlinded(combat_unit_sender);

        if (sender_has_truesight)
        {
            is_hit = true;
        }
        // Check for Blind effect
        else if (sender_is_blinded)
        {
            is_hit = false;
        }
        else
        {
            // Check hit chance
            // If hit_chance_overflow is >= 100 then the attack should hit
            is_hit =
                StatsHelper::TimeStepOverflowStat(sender_entity, sender_live_stats, StatType::kHitChancePercentage);
        }

        // Only attack ability can be dodged
        // Trueshot cannot be dodged
        // TODO(vampy): Check here if the receiver is an enemy or not because we don't want to dodge effect packages
        // that contain a heal
        if (ability->ability_type != AbilityType::kAttack || sender_has_truesight || attributes.is_trueshot ||
            !EntityHelper::CanDodge(receiver_entity))
        {
            is_dodged = false;
        }
        else
        {
            // Check dodge chance
            is_dodged = StatsHelper::TimeStepOverflowStat(
                receiver_entity,
                receiver_live_stats,
                StatType::kAttackDodgeChancePercentage);
        }
    }

    SourceContextData context = ability->data->source_context;
    context.combat_unit_ability_type = combat_unit_sender_ability_type;
    context.ability_name = ability->data->name;
    context.skill_name =
        ability->IsFinished() ? "Skills Are Already Finished" : ability->GetCurrentSkillState().data->name;

    // Construct the event data
    event_data::EffectPackage event_data;
    event_data.sender_id = sender_id;
    event_data.combat_unit_sender_id = combat_unit_sender_id;
    event_data.receiver_id = receiver_id;
    event_data.ability_type = ability->ability_type;
    event_data.is_critical = is_critical;
    event_data.attributes = attributes;
    event_data.source_context = context;

    // Handle missed
    if (!is_hit)
    {
        LogDebug(
            sender_id,
            "| ApplyEffectPackage - MISSED receiver = {}, sender hit_chance_percentage = {}",
            receiver_id,
            sender_live_stats.Get(StatType::kHitChancePercentage));
        if (!attributes.IsEmpty())
        {
            LogDebug(sender_id, "- effect package attributes: {}", attributes);
        }

        world_->EmitEvent<EventType::kEffectPackageMissed>(event_data);

        // Return as we can't apply any effect anyways
        return;
    }

    // Handle dodged
    if (is_dodged)
    {
        LogDebug(
            sender_id,
            "| ApplyEffectPackage - DODGED receiver = {}, receiver attack_dodge_chance_percentage = {}, "
            "attack_dodge_chance_overflow_percentage = {}",
            receiver_id,
            receiver_live_stats.Get(StatType::kAttackDodgeChancePercentage),
            receiver_stats_component.GetAttackDodgeChanceOverflowPercentage());
        if (!attributes.IsEmpty())
        {
            LogDebug(sender_id, "- effect package attributes: {}", attributes);
        }

        world_->EmitEvent<EventType::kEffectPackageDodged>(event_data);

        // Return as we can't apply any effect anyways
        return;
    }

    // Blocked?
    // Has EffectPackageBlock, and the effect package comes from enemies
    if (ReceiverBlockEffectPackage(sender_entity, receiver_entity, combat_unit_sender_ability_type))
    {
        LogDebug(sender_id, "| ApplyEffectPackage - BLOCKED receiver = {}", receiver_id);
        if (!attributes.IsEmpty())
        {
            LogDebug(sender_id, "- effect package attributes: {}", attributes);
        }

        world_->EmitEvent<EventType::kEffectPackageBlocked>(event_data);

        // Return as we can't apply any effect anyways
        return;
    }

    //
    // Apply effects package if Hit & Not dodged
    //

    // Send the stats of the from entity inside the package
    EffectState effect_state;
    effect_state.sender_stats = FullStatsData{world_->GetBaseStats(sender_entity), sender_live_stats};
    effect_state.source_context = context;
    effect_state.is_critical = is_critical;
    effect_state.effect_package_attributes = attributes;
    effect_state.attached_from_entity_id = ability->data->attached_from_entity_id;

    LogDebug(
        sender_id,
        "| ApplyEffectPackage - HIT receiver = {}, is_critical = {}, sender [crit_amplification_percentage = {},  "
        "hit_chance_percentage = {}], receiver [attack_dodge_chance_percentage = {}, "
        "attack_dodge_chance_overflow_percentage = {}]",
        receiver_id,
        effect_state.is_critical,
        sender_live_stats.Get(StatType::kCritAmplificationPercentage),
        sender_live_stats.Get(StatType::kHitChancePercentage),
        receiver_live_stats.Get(StatType::kAttackDodgeChancePercentage),
        receiver_stats_component.GetAttackDodgeChanceOverflowPercentage());
    if (!attributes.IsEmpty())
    {
        LogDebug(sender_id, "- effect package attributes: {}", attributes);
    }

    if (receiver_entity.Has<AbilitiesComponent>())
    {
        auto& abilities_component = receiver_entity.Get<AbilitiesComponent>();
        abilities_component.IncrementReceivedEffectPackagesCount(ability->ability_type);
    }

    // Send event
    world_->EmitEvent<EventType::kEffectPackageReceived>(event_data);

    ApplyEffectPackagePropagation(sender_entity, effect_package, context, is_critical, receiver_entity);
    const bool should_skip_effect_package = ShouldSkipOriginalEffectPackage(effect_package);

    // Start recording damage if there is any wound effect
    size_t wound_frame_index = kInvalidIndex;
    for (auto& effect : effect_package.effects)
    {
        if (effect.IsWound())
        {
            wound_frame_index = AddWoundRecordingFrame(sender_id, receiver_id, combat_unit_sender_ability_type);
            break;
        }
    }

    // Send skill effects
    bool has_any_instant_damage_effect = false;
    for (const auto& effect_data : effect_package.effects)
    {
        if (effect_data.IsWound())
        {
            // apply wounds after this loop so that we have total damage dealt by this package
            continue;
        }

        if (effect_data.type_id.type == EffectType::kInstantDamage)
        {
            has_any_instant_damage_effect = true;
        }

        // Capture stats of combat unit currently focused by the sender if so required by effect
        if (effect_data.GetRequiredDataSourceTypes().Contains(ExpressionDataSourceType::kSenderFocus))
        {
            if (sender_entity.Has<FocusComponent>())
            {
                const auto& focus_component = sender_entity.Get<FocusComponent>();
                const EntityID focused_entity_id = focus_component.GetFocusID();
                effect_state.sender_focus_stats = world_->GetFullStats(focused_entity_id);
            }
            else
            {
                LogWarn(
                    "InitEffectStateData: Failed to get stats of sender current focus since sender ({}) does not have "
                    "focus component.",
                    combat_unit_sender_id);
            }
        }

        if (!should_skip_effect_package)
        {
            world_->BuildAndEmitEvent<EventType::kTryToApplyEffect>(sender_id, receiver_id, effect_data, effect_state);
        }
    }

    if (wound_frame_index != kInvalidIndex)
    {
        assert(wound_frame_index + 1 == wound_recording_frames_.size());
        const auto frame = wound_recording_frames_.back();
        wound_recording_frames_.pop_back();
        ApplyWounds(frame, effect_package, effect_state);
    }

    // Apply thorns?
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/154894374/Thorns
    if (combat_unit_sender_ability_type == AbilityType::kAttack && has_any_instant_damage_effect)
    {
        ApplyThorns(sender_entity, ability, receiver_id, is_critical);
    }
}

size_t AbilitySystem::AddWoundRecordingFrame(
    const EntityID sender_id,
    const EntityID receiver_id,
    const AbilityType ability_type)
{
    size_t wound_frame_index = kInvalidIndex;

    // no need to add a new frame if we already have one with the same filters
    // this way double application of wounds is avoided
    if (!wound_recording_frames_.empty())
    {
        auto& frame = wound_recording_frames_.back();
        if (frame.ability_type == ability_type && frame.sender == sender_id && frame.receiver == receiver_id)
        {
            wound_frame_index = wound_recording_frames_.size() - 1;
        }
    }

    if (wound_frame_index == kInvalidIndex)
    {
        wound_frame_index = wound_recording_frames_.size();
        wound_recording_frames_.emplace_back();
        auto& wound_frame = wound_recording_frames_.back();
        wound_frame.ability_type = ability_type;
        wound_frame.sender = sender_id;
        wound_frame.receiver = receiver_id;
    }

    return wound_frame_index;
}

void AbilitySystem::ApplyWounds(
    const WoundRecordingFrame& frame,
    const EffectPackage& effect_package,
    const EffectState& effect_state)
{
    assert(!currently_applying_wound_);
    currently_applying_wound_ = true;

    for (const EffectData& effect_data : effect_package.effects)
    {
        if (effect_data.IsWound())
        {
            auto wound_effect = effect_data;
            wound_effect.SetExpression(EffectExpression::FromValue(frame.accumulated_damage));
            world_->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
                frame.sender,
                frame.receiver,
                std::move(wound_effect),
                effect_state);
        }
    }

    currently_applying_wound_ = false;
}

void AbilitySystem::ApplyEffectPackage(
    const Entity& sender_entity,
    const AbilityStatePtr& ability,
    const EffectPackage& effect_package,
    const bool is_critical,
    const HexGridPosition receiver_position)
{
    const EntityID sender_id = sender_entity.GetID();
    const EntityID combat_unit_sender_id = world_->GetCombatUnitParentID(sender_entity);

    // Handle invalid parent
    if (!EntityHelper::CanApplyEffect(sender_entity))
    {
        LogErr(sender_id, "AbilitySystem::ApplyEffectPackage() entity {} cant apply effects", sender_entity.GetID());
        assert(false);
    }

    const AbilityType combat_unit_sender_ability_type = EntityHelper::IsACombatUnit(sender_entity)
                                                            ? ability->ability_type
                                                            : ability->data->source_context.combat_unit_ability_type;

    // Send event
    {
        event_data::EffectPackage event_data;
        event_data.sender_id = sender_id;
        event_data.combat_unit_sender_id = combat_unit_sender_id;
        event_data.receiver_id = kInvalidEntityID;
        event_data.ability_type = ability->ability_type;
        event_data.is_critical = is_critical;
        event_data.attributes = effect_package.attributes;
        event_data.source_context = ability->data->source_context;
        event_data.receiver_position = receiver_position;
        world_->EmitEvent<EventType::kEffectPackageReceived>(event_data);
    }

    if (ShouldSkipOriginalEffectPackage(effect_package))
    {
        return;
    }

    SourceContextData context = ability->data->source_context;
    context.combat_unit_ability_type = combat_unit_sender_ability_type;
    context.ability_name = ability->data->name;
    context.skill_name =
        ability->IsFinished() ? "Skills Are Already Finished" : ability->GetCurrentSkillState().data->name;

    // Send the stats of the from entity inside the package
    EffectState effect_state;
    effect_state.sender_stats = world_->GetFullStats(sender_id);
    effect_state.source_context = context;
    effect_state.is_critical = is_critical;
    effect_state.effect_package_attributes = effect_package.attributes;
    effect_state.attached_from_entity_id = ability->data->attached_from_entity_id;

    // TODO (kostiantyn) Should we have to handle propagation when sending package to location? Commented it for now
    // ApplyEffectPackagePropagation(sender_entity, effect_package, context, is_critical, receiver_entity);

    // Send skill effects
    for (const auto& effect_data : effect_package.effects)
    {
        // TODO (kostiantyn): Do we even need sender stats when emiting and event which will be ignored by simulation?

        // Capture stats of combat unit currently focused by the sender if so required by effect
        if (effect_data.GetRequiredDataSourceTypes().Contains(ExpressionDataSourceType::kSenderFocus))
        {
            if (sender_entity.Has<FocusComponent>())
            {
                const auto& focus_component = sender_entity.Get<FocusComponent>();
                const EntityID focused_entity_id = focus_component.GetFocusID();
                effect_state.sender_focus_stats = world_->GetFullStats(focused_entity_id);
            }
            else
            {
                LogWarn(
                    "InitEffectStateData: Failed to get stats of sender current focus since sender ({}) does not have "
                    "focus component.",
                    combat_unit_sender_id);
            }
        }

        event_data::Effect event{};
        event.sender_id = sender_id;
        event.receiver_id = kInvalidEntityID;
        event.data = effect_data;
        event.state = effect_state;
        event.receiver_position = receiver_position;
        world_->EmitEvent<EventType::kTryToApplyEffect>(event);
    }
}

bool AbilitySystem::ShouldSkipOriginalEffectPackage(const EffectPackage& effect_package)
{
    // We skip application of the original effect package if we spawn propagation with
    // replace_original_effect_package is set to true
    return effect_package.attributes.propagation.type != EffectPropagationType::kNone &&
           effect_package.attributes.propagation.skip_original_effect_package;
}

void AbilitySystem::AddOriginalEffectPackage(EffectPackage& destination, const EffectPackage& original)
{
    // The Effect Package of the original effect package will be added to
    // the one specified in the propagation.
    if (!original.attributes.propagation.add_original_effect_package)
    {
        return;
    }

    if (destination.attributes.propagation.type == EffectPropagationType::kNone)
    {
        // Do not add propagation from original effect package to avoid recursion
        destination.attributes.EmpowerIgnorePropagation(original.attributes);
    }
    else
    {
        destination.attributes += original.attributes;
    }

    destination.effects.insert(destination.effects.end(), original.effects.begin(), original.effects.end());
}

void AbilitySystem::ApplyEffectPackagePropagation(
    const Entity& sender_entity,
    const EffectPackage& effect_package,
    const SourceContextData& context,
    const bool is_critical,
    const Entity& receiver_entity)
{
    SourceContextData propagation_context(context);
    // Use source context from propagation data if set
    if (!effect_package.attributes.propagation.source_context.IsEmpty())
    {
        propagation_context = effect_package.attributes.propagation.source_context;
    }

    switch (effect_package.attributes.propagation.type)
    {
    case EffectPropagationType::kChain:
    {
        auto& propagation = effect_package.attributes.propagation;

        ChainData chain_data;
        chain_data.source_context = propagation_context;
        chain_data.sender_id = sender_entity.GetID();
        chain_data.first_propagation_receiver_id = receiver_entity.GetID();
        chain_data.chain_delay_ms = propagation.chain_delay_ms;
        chain_data.chain_number = propagation.chain_number;
        chain_data.prioritize_new_targets = propagation.prioritize_new_targets;
        chain_data.only_new_targets = propagation.only_new_targets;
        chain_data.ignore_first_propagation_receiver = propagation.ignore_first_propagation_receiver;
        chain_data.chain_bounce_max_distance_units = propagation.chain_bounce_max_distance_units;
        chain_data.pre_deployment_delay_percentage = propagation.pre_deployment_delay_percentage;
        chain_data.is_critical = is_critical;
        chain_data.propagation_effect_package = *propagation.effect_package;
        chain_data.targeting_group = propagation.targeting_group;
        chain_data.targeting_guidance = propagation.targeting_guidance;
        chain_data.deployment_guidance = propagation.deployment_guidance;

        AddOriginalEffectPackage(chain_data.propagation_effect_package, effect_package);
        EntityFactory::SpawnChainPropagation(*world_, sender_entity.GetTeam(), chain_data);
    }
    break;

    case EffectPropagationType::kSplash:
    {
        auto& propagation = effect_package.attributes.propagation;

        SplashData splash_data;
        splash_data.source_context = propagation_context;
        splash_data.sender_id = sender_entity.GetID();
        splash_data.is_critical = is_critical;
        splash_data.ignore_first_propagation_receiver = propagation.ignore_first_propagation_receiver;
        splash_data.splash_radius_units = propagation.splash_radius_units;
        splash_data.targeting_guidance = propagation.targeting_guidance;
        splash_data.deployment_guidance = propagation.deployment_guidance;

        // Copy the pointer to the skill
        splash_data.propagation_effect_package = *propagation.effect_package;
        AddOriginalEffectPackage(splash_data.propagation_effect_package, effect_package);
        const auto& receiver_entity_position_component = receiver_entity.Get<PositionComponent>();
        const HexGridPosition& receiver_position = receiver_entity_position_component.GetPosition();

        // Spawn the zone at the target position
        EntityFactory::SpawnSplash(
            *world_,
            sender_entity.GetTeam(),
            receiver_entity.GetID(),
            receiver_position,
            splash_data);
    }
    break;

    case EffectPropagationType::kNone:
        break;

    default:
        assert(false);
        break;
    }
}

void AbilitySystem::ApplyThorns(
    const Entity& sender_entity,
    const AbilityStatePtr& ability,
    const EntityID receiver_id,
    const bool is_critical) const
{
    // Nothing to do thorns is zero
    const StatsData receiver_live_stats = world_->GetLiveStats(receiver_id);
    if (receiver_live_stats.Get(StatType::kThorns) == 0_fp)
    {
        return;
    }

    // As sender_entity can be a projectile we want to know the true combat unit
    const EntityID sender_combat_unit_parent_id = world_->GetCombatUnitParentID(sender_entity);
    if (!world_->HasEntity(sender_combat_unit_parent_id))
    {
        LogWarn(
            sender_entity.GetID(),
            "| AbilitySystem::ApplyThorns - sender_combat_unit_parent_id = {} NOT FOUND",
            receiver_id,
            sender_combat_unit_parent_id);
        return;
    }

    LogDebug(
        sender_entity.GetID(),
        "| AbilitySystem::ApplyThorns - receiver_id = {} returns damage back to sender_combat_unit_parent_id = {}, "
        "receiver_live_stats.thorns = {}",
        receiver_id,
        sender_combat_unit_parent_id,
        receiver_live_stats.Get(StatType::kThorns));

    const auto thorns_effect_data = EffectData::CreateDamage(
        EffectDamageType::kPure,
        EffectExpression::FromValue(receiver_live_stats.Get(StatType::kThorns)));

    EffectState thorns_effect_state;
    thorns_effect_state.is_critical = is_critical;
    thorns_effect_state.source_context = ability->data->source_context;
    thorns_effect_state.sender_stats = world_->GetFullStats(sender_combat_unit_parent_id);

    world_->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        receiver_id,
        sender_combat_unit_parent_id,
        thorns_effect_data,
        thorns_effect_state);
}

bool AbilitySystem::ReceiverBlockEffectPackage(
    const Entity& sender_entity,
    const Entity& receiver_entity,
    const AbilityType ability_type)
{
    // Receiver must have EffectPackageBlock
    if (!EntityHelper::HasPositiveState(receiver_entity, EffectPositiveState::kEffectPackageBlock))
    {
        return false;
    }

    // Must be enemies
    if (receiver_entity.IsAlliedWith(sender_entity))
    {
        return false;
    }

    const auto& receiver_attached_effects_component = receiver_entity.Get<AttachedEffectsComponent>();
    const auto& receiver_map_positive_states = receiver_attached_effects_component.GetPositiveStates();

    // NOTE: this should always succeed because we checked EntityHelper::HasEffectPackageBlock above
    const auto& effect_package_block_positive_states =
        receiver_map_positive_states.at(EffectPositiveState::kEffectPackageBlock);

    // Increase the consumption of the EffectPackageBlock effects on the receiver
    for (const auto& effect_package_block_effect : effect_package_block_positive_states)
    {
        if (effect_package_block_effect->IsExpired())
        {
            continue;
        }

        // check that we can block an ability of this type
        const EffectPackageHelper& effect_package_helper = world_->GetEffectPackageHelper();
        if (!effect_package_helper.DoesAbilityTypesMatchActivatedAbility(
                effect_package_block_effect->effect_data.ability_types,
                ability_type))
        {
            continue;
        }

        effect_package_block_effect->current_blocks++;
        return true;
    }

    return false;
}

void AbilitySystem::ApplyEffectPackageAttributes(
    const Entity& sender_entity,
    const EffectPackageAttributes& attributes,
    const SourceContextData& context) const
{
    if (attributes.IsEmpty())
    {
        return;
    }

    const EntityID sender_id = sender_entity.GetID();
    LogDebug(sender_id, "| ApplyEffectPackageAttributes - {}", attributes);

    const EntityID combat_unit_sender_id = world_->GetCombatUnitParentID(sender_entity);

    // If the effect has any health to refund send an event
    if (!attributes.refund_health.IsEmpty())
    {
        // Energy gain event to calculate how much energy we refund
        event_data::ApplyHealthGain event_data;
        event_data.caused_by_id = sender_id;
        event_data.receiver_id = combat_unit_sender_id;
        event_data.health_gain_type = HealthGainType::kInstantHeal;
        event_data.value = world_->EvaluateExpression(attributes.refund_health, sender_entity.GetID());
        event_data.source_context = context;

        world_->EmitEvent<EventType::kApplyHealthGain>(event_data);
    }

    // If the effect has any energy to refund send an event
    if (!attributes.refund_energy.IsEmpty())
    {
        // Energy gain event to calculate how much energy we refund
        event_data::ApplyEnergyGain event_data;
        event_data.caused_by_id = sender_id;
        event_data.receiver_id = combat_unit_sender_id;
        event_data.energy_gain_type = EnergyGainType::kRefund;
        event_data.value = world_->EvaluateExpression(attributes.refund_energy, sender_entity.GetID());

        world_->EmitEvent<EventType::kApplyEnergyGain>(event_data);
    }
}

bool AbilitySystem::TryToActivateOmega(const Entity& sender_entity, AbilityStatePtr& ability)
{
    ILLUVIUM_PROFILE_FUNCTION();

    const EntityID sender_id = sender_entity.GetID();
    LogDebug(sender_id, "AbilitySystem::TryToActivateOmega - {}", *ability);

    if (ability->ability_type != AbilityType::kOmega)
    {
        assert(false);
        LogErr(sender_id, "| Not an Omega. You have a BUG IN YOUR CODE.");
        return false;
    }

    // Stop ability if there is another active ability running.
    // Most likeley an innate.
    const auto& abilities_component = sender_entity.Get<AbilitiesComponent>();
    if (abilities_component.HasActiveAbility())
    {
        LogDebug(sender_id, "| Can't activate, there is an active ability already running.");
        return false;
    }

    // Make sure we don't have negative effects preventing attack
    if (!EntityHelper::CanActivateAbility(sender_entity, AbilityType::kOmega))
    {
        LogDebug(sender_id, "| Canceled due to Negative State");
        return false;
    }

    if (!CanActivateAbility(sender_entity, *ability, world_->GetTimeStepCount()))
    {
        LogDebug(sender_id, "| Can't activate, denied by ability");
        return false;
    }

    if (!EntityHelper::HasEnoughEnergyForOmega(sender_entity))
    {
        LogDebug(sender_id, "| Can't activate, denied by ability, not enoughE energy");
        return false;
    }

    // Try to perform targeting
    if (!TryInitSkillTargeting(sender_entity, ability, true))
    {
        LogDebug(sender_id, "| Can't activate omega, denied by ability skill targeting");
        return false;
    }

    // Deplete energy
    auto& stats_component = sender_entity.Get<StatsComponent>();

    // Obtain energy before it is depleted for delta
    const FixedPoint delta = stats_component.GetCurrentEnergy();

    // Use energy for the omega
    stats_component.SetCurrentEnergy(0_fp);
    LogDebug(sender_id, "| kEnergyChanged - depleted {} energy to activate omega", delta);
    world_->BuildAndEmitEvent<EventType::kEnergyChanged>(sender_id, sender_id, -delta);

    // Activate
    Activate(sender_entity, ability);

    return true;
}

bool AbilitySystem::TryToActivateAttack(const Entity& sender_entity, AbilityStatePtr& ability)
{
    ILLUVIUM_PROFILE_FUNCTION();

    const EntityID sender_id = sender_entity.GetID();
    LogDebug(sender_id, "AbilitySystem::TryToActivateAttack - {}", *ability);

    if (ability->ability_type != AbilityType::kAttack)
    {
        assert(false);
        LogErr(sender_id, "| Not an Attack. You have a BUG IN YOUR CODE.");
        return false;
    }

    // Stop ability if there is another active ability running.
    // Most likeley an innate.
    const auto& abilities_component = sender_entity.Get<AbilitiesComponent>();
    if (abilities_component.HasActiveAbility())
    {
        LogDebug(sender_id, "| Can't activate, there is an active ability already running.");
        return false;
    }

    // Make sure we don't have negative effects preventing attack
    if (!EntityHelper::CanActivateAbility(sender_entity, AbilityType::kAttack))
    {
        LogDebug(sender_id, "| Interrupted by Negative State");
        return false;
    }

    if (!CanActivateAbility(sender_entity, *ability, world_->GetTimeStepCount()))
    {
        LogDebug(sender_id, " | Can't activate, denied by ability");
        return false;
    }

    // Try to perform targeting
    if (!TryInitSkillTargeting(sender_entity, ability, true))
    {
        LogDebug(sender_id, "| Can't activate attack, denied by ability skill targeting");
        return false;
    }

    // Activate
    Activate(sender_entity, ability);

    return true;
}

void AbilitySystem::TryToClearReservedPosition(const Entity& sender_entity)
{
    // Get the source position component to work on
    const GridHelper& grid_helper = world_->GetGridHelper();
    PositionComponent* position_component = grid_helper.GetSourcePositionComponent(sender_entity);
    if (position_component != nullptr)
    {
        // Clear the reservation
        position_component->ClearReservedPosition();
    }
}

void AbilitySystem::CheckAndSetCriticalSkills(
    const Entity& sender_entity,
    const StatsData& sender_live_stats,
    const AbilityStatePtr& ability,
    const bool always_roll,
    bool* out_ability_is_critical,
    bool* out_random_is_critical) const
{
    *out_ability_is_critical = false;
    *out_random_is_critical = false;

    // Roll for each skill that says it can crit
    const EffectPackageHelper& effect_package_helper = world_->GetEffectPackageHelper();
    for (SkillState& skill : ability->skills)
    {
        SkillState empowered_skill = skill;

        // Empower can change can_crit or always_crit to true so we have to get the empowers and check critical
        // Based on the empower attributes merged in with the original attributes
        // Create new skill with empowers
        EffectPackage empower_effect_package;
        effect_package_helper.GetEmpowerEffectPackageForAbility(
            sender_entity,
            ability->ability_type,
            skill.CheckAllIfIsCritical(),
            &empower_effect_package,
            nullptr);
        effect_package_helper.MergeEmpowerEffectPackage(skill.data->effect_package, &empower_effect_package);

        // Empower_effect_package has the attributes from the effect_package and the empower effect_package
        const EffectPackageAttributes& attributes = empower_effect_package.attributes;

        // TODO(vampy): Remove always roll and set can_crit to defaults if ability_type == AbilityType::kAttack
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/252575899/EffectPackage.CanCrit
        if (always_roll || attributes.can_crit)
        {
            // If crit_chance_percentage is >= 100 then we set the attack to a critical
            const bool random_is_critical =
                StatsHelper::TimeStepOverflowStat(sender_entity, sender_live_stats, StatType::kCritChancePercentage);

            skill.is_critical = random_is_critical;
            *out_random_is_critical = *out_random_is_critical || random_is_critical;
        }

        // Skill has a flag that says to always to crit, respect that
        if (attributes.always_crit)
        {
            skill.is_critical = true;
        }

        if (skill.is_critical)
        {
            *out_ability_is_critical = true;
        }
    }
}

void AbilitySystem::Activate(const Entity& sender_entity, AbilityStatePtr& ability)
{
    ILLUVIUM_PROFILE_FUNCTION();

    const EntityID sender_id = sender_entity.GetID();
    const bool is_sender_a_combat_unit = EntityHelper::IsACombatUnit(sender_entity);
    LogDebug(sender_id, "AbilitySystem::Activate - {}", *ability);

    auto& abilities_component = sender_entity.Get<AbilitiesComponent>();
    abilities_component.OnAbilityActivated(ability, world_->GetTimeStepCount());

    // Cache the sender stats
    const StatsData sender_live_stats = world_->GetLiveStats(sender_entity);
    const FixedPoint current_attack_speed =
        world_->GetClampedAttackSpeed(sender_live_stats.Get(StatType::kAttackSpeed));
    int previous_overflow_ms = 0;

    // Attack ability
    bool ability_is_critical = false;
    if (ability->ability_type == AbilityType::kAttack)
    {
        // Attack ability duration gets its total duration from the attack speed
        if (!ability->data->ignore_attack_speed)
        {
            if (sender_entity.Has<DecisionComponent>())
            {
                auto& decision_component = sender_entity.Get<DecisionComponent>();
                if (decision_component.GetPreviousAction() != DecisionNextAction::kAttackAbility)
                {
                    // Overflow is only used for improving the timing of several attacks in a row
                    // If the previous action was anything else, reset the overflow
                    ability->previous_overflow_ms = 0;
                }
            }

            // Attack duration is determined by base attack speed of the illuvial
            // We change how time elapses in TimeStepTimerState() based on the ratio
            // between the current attack speed and the base attack speed
            // For more details, please check the pseudocode at:
            // https://illuvium.atlassian.net/wiki/spaces/AB/pages/62128206/Attack+Ability
            const StatsData sender_base_stats = world_->GetBaseStats(sender_entity.GetID());
            const FixedPoint base_attack_speed =
                world_->GetClampedAttackSpeed(sender_base_stats.Get(StatType::kAttackSpeed));
            const int duration_ms = Time::AttackSpeedToMsNoRounding(base_attack_speed.AsInt());
            ability->UpdateTimers(duration_ms);

            // Can be negative if the previous ability was interrupted in the middle of attacking.
            previous_overflow_ms = std::max(0, ability->previous_overflow_ms);

            // Overflow makes the current time start at a higher number than before.
            // We cap it at the total duration of the attack because things can get crazy
            // if the attack speed is faster than one time step
            // To get a feel for how the numbers actually change in detail, please take a look
            // at the AbilitySystemTestWithStartAttackEvery0Point53Seconds.AttackEvery0Point53Seconds test
            // and the AbilitySystemTestWithComplexSpeedChanges.ComplexSpeedChanges test.
            ability->SetCurrentAbilityTimeMs(std::clamp(previous_overflow_ms, 0, duration_ms));

            const FixedPoint attack_speed =
                world_->GetClampedAttackSpeed(sender_base_stats.Get(StatType::kAttackSpeed));
            LogDebug(
                sender_id,
                "| current_attack_speed = {}, base_attack_speed = {}, previous_overflow_ms = {}, duration_ms = {}, "
                "ability->current_time_ms = {}",
                current_attack_speed,
                attack_speed,
                previous_overflow_ms,
                duration_ms,
                ability->GetCurrentAbilityTimeMs());
        }

        // Only roll for crit if the sender has any stats
        if (sender_entity.Has<StatsComponent>())
        {
            bool random_is_critical = false;

            // Always roll random for attack abilities coming from combat units
            CheckAndSetCriticalSkills(
                sender_entity,
                sender_live_stats,
                ability,
                is_sender_a_combat_unit,
                &ability_is_critical,
                &random_is_critical);

            const auto& stats_component = sender_entity.Get<StatsComponent>();
            LogDebug(
                sender_id,
                "| crit_chance_overflow_percentage = {}, random_is_critical = {}, ability_is_critical = {}",
                stats_component.GetCritChanceOverflowPercentage(),
                random_is_critical,
                ability_is_critical);
        }

        // If the entity is a shield we need to check for critical from the parent since shield damage effects can crit
        if (EntityHelper::IsAShield(sender_entity))
        {
            bool random_is_critical = false;
            const Entity& parent_entity = world_->GetByID(sender_entity.GetParentID());
            CheckAndSetCriticalSkills(
                parent_entity,
                sender_live_stats,
                ability,
                false,
                &ability_is_critical,
                &random_is_critical);

            const auto& stats_component = parent_entity.Get<StatsComponent>();
            LogDebug(
                parent_entity.GetID(),
                "| crit_chance_overflow_percentage = {}, random_is_critical = {}, ability_is_critical = {}, critical "
                "check queried parent",
                stats_component.GetCritChanceOverflowPercentage(),
                random_is_critical,
                ability_is_critical);
        }
    }

    // Omega ability
    if (ability->ability_type == AbilityType::kOmega && is_sender_a_combat_unit)
    {
        bool random_is_critical = false;
        CheckAndSetCriticalSkills(
            sender_entity,
            sender_live_stats,
            ability,
            false,
            &ability_is_critical,
            &random_is_critical);

        // On Activation Energy gain for omegas
        event_data::ApplyEnergyGain event_data;
        event_data.caused_by_id = sender_id;
        event_data.receiver_id = sender_id;
        event_data.energy_gain_type = EnergyGainType::kOnActivation;

        world_->EmitEvent<EventType::kApplyEnergyGain>(event_data);
    }

    SkillState& first_skill_state = ability->skills.front();
    if (!first_skill_state.HasValidTargeting())
    {
        // We should not have invalid targeting here, all necessary targeting checks should be made before
        // activation
        LogErr(sender_id, "| First skill don't have a valid target during activation.");
        if (HandleTargetingFailure(sender_entity, ability))
        {
            assert(false);
            return;
        }
    }

    HandleReservingPositionIfNeeded(sender_entity, ability);

    // Send event
    {
        event_data::AbilityActivated event_data;
        event_data.sender_id = sender_id;
        event_data.combat_unit_sender_id = world_->GetCombatUnitParentID(sender_id);
        event_data.ability_type = ability->ability_type;
        event_data.ability_index = ability->index;
        event_data.ability_name = ability->data->name;
        event_data.attack_speed = current_attack_speed;
        event_data.is_critical = ability_is_critical;
        event_data.trigger_type = ability->data->activation_trigger_data.trigger_type;
        event_data.previous_overflow_ms = previous_overflow_ms;
        event_data.total_duration_ms = ability->total_duration_ms;
        event_data.source_context = ability->data->source_context;

        // TODO Why do we need predicted targets at ability level?
        // In case of first skill is self-targeted in will be just only self
        event_data.predicted_receiver_ids = first_skill_state.targeting_state.GetAvailableTargetsVector();

        world_->EmitEvent<EventType::kAbilityActivated>(event_data);
    }

    // Keep track of the previous action
    if (sender_entity.Has<DecisionComponent>())
    {
        auto& decision_component = sender_entity.Get<DecisionComponent>();
        if (ability->ability_type == AbilityType::kOmega)
        {
            decision_component.SetPreviousAction(DecisionNextAction::kOmegaAbility);
        }
        else if (ability->ability_type == AbilityType::kAttack)
        {
            decision_component.SetPreviousAction(DecisionNextAction::kAttackAbility);
        }
    }

    // TimeStep through ability in the same time step
    TryToStepAbility(sender_entity, ability);
}

void AbilitySystem::Deactivate(const Entity& sender_entity, AbilityStatePtr& ability)
{
    const EntityID sender_id = sender_entity.GetID();
    LogDebug(sender_id, "AbilitySystem::Deactivate - {}", *ability);

    auto& abilities_component = sender_entity.Get<AbilitiesComponent>();
    ability->Deactivate();
    abilities_component.OnAbilityDeactivated();

    // Dashes must not clear the reserved position of parent entity on ability deactivation
    // (which may happen on dash attack ability deactivation during passthrough).
    // This way it would open a way for other dashes to choose the same point and cause entities overlap.
    // The same applies to instant abilities.
    // Example here: https://illuvium.atlassian.net/browse/ARENA-9111
    if (!EntityHelper::IsADash(sender_entity) && !ability->IsInstant())
    {
        // This check fixes this bug: https://illuvium.atlassian.net/browse/ARENA-11033
        // Do not clear reserved position if there is any displacement effect on entity
        // because it may be caused by other entity's skill.
        // In general, we need better way to handle position reservation rather than simple clear on ability
        // deactivation
        if (!sender_entity.Has<AttachedEffectsComponent>() ||
            !sender_entity.Get<AttachedEffectsComponent>().HasDisplacement())
        {
            TryToClearReservedPosition(sender_entity);
        }
    }

    // Check if ability is critical
    bool ability_is_critical = false;
    for (const auto& skill : ability->skills)
    {
        if (skill.CheckAllIfIsCritical())
        {
            ability_is_critical = true;
            break;
        }
    }

    // Send event
    event_data::AbilityDeactivated event_data;
    event_data.sender_id = sender_id;
    event_data.combat_unit_sender_id = world_->GetCombatUnitParentID(sender_id);
    event_data.ability_type = ability->ability_type;
    event_data.ability_index = ability->index;
    event_data.ability_name = ability->data->name;
    event_data.trigger_type = ability->data->activation_trigger_data.trigger_type;
    event_data.source_context = ability->data->source_context;
    event_data.has_skill_with_movement = ability->HasSkillWithMovement();
    event_data.total_duration_ms = ability->total_duration_ms;
    event_data.is_critical = ability_is_critical;

    // Normal deactivated event
    world_->EmitEvent<EventType::kAbilityDeactivated>(event_data);

    ability->consumable_empowers_used.clear();

    // After all the deactivated events are fired
    world_->EmitEvent<EventType::kAfterAbilityDeactivated>(event_data);

    // Workaround for some edge cases
    if (force_next_time_step_)
    {
        LogDebug(sender_id, "| force_next_time_step = true, returning early!");
        return;
    }

    // Continue attack ability in the same time step
    const auto& decision_system = world_->GetSystem<DecisionSystem>();
    const DecisionNextAction next_action = decision_system.GetNextAction(sender_entity);
    const bool next_action_attack_ability = next_action == DecisionNextAction::kAttackAbility;
    if (ability->ability_type == AbilityType::kAttack && !ability->IsInstant() && next_action_attack_ability)
    {
        LogDebug(sender_id, "| Activating next attack ability in the same time step");
        constexpr bool try_to_activate_attack = true;
        constexpr bool try_to_activate_omega = false;
        ChooseAndActivateAnyAbility(sender_entity, try_to_activate_attack, try_to_activate_omega);
    }
    else
    {
        // Try to consume innates abilities that are not instant
        if (AbilityStatePtr innate_ability = abilities_component.PopInnateWaitingActivation())
        {
            LogDebug(sender_id, "| Activating innate from Queue.");
            bool out_activated = false;
            bool out_added_to_queue = false;
            TryToActivateInnateOrAddToQueue(sender_entity, innate_ability, &out_activated, &out_added_to_queue);
        }
    }
}

bool AbilitySystem::CanActivateAbilityForTriggerData(
    const Entity& sender_entity,
    const AbilityState& state,
    const int time_step,
    bool with_trigger_counter) const
{
    const AbilityData& data = *state.data;
    const auto& activation_trigger_data = data.activation_trigger_data;

    // Sender has required conditions
    if (!world_->DoesPassConditions(sender_entity, activation_trigger_data.required_sender_conditions))
    {
        return false;
    }

    // Has activation time limit
    if (state.activation_time_limit_time_steps > 0 &&
        Time::MsToTimeSteps(state.total_current_time_ms) >= state.activation_time_limit_time_steps)
    {
        return false;
    }

    // Activate at time interval
    if (state.activate_every_time_steps > 0 &&
        (time_step - state.last_activation_time_step) < state.activate_every_time_steps)
    {
        return false;
    }

    // Early return if the ability hasn't been activated yet
    if (state.last_activation_time_step > 0 && state.activation_cooldown_time_steps > 0)
    {
        if (time_step - state.last_activation_time_step < state.activation_cooldown_time_steps)
        {
            return false;
        }
    }

    // Has max activations set
    if (activation_trigger_data.max_activations > 0 &&
        state.activation_count >= activation_trigger_data.max_activations)
    {
        return false;
    }

    // Trigger requires hyper to be active
    if (activation_trigger_data.requires_hyper_active)
    {
        if (!EntityHelper::IsACombatUnit(sender_entity) || !sender_entity.Get<StatsComponent>().IsHyperactive())
        {
            return false;
        }
    }

    // Activate every X abilities
    const auto& abilities_component = sender_entity.Get<AbilitiesComponent>();
    if (activation_trigger_data.trigger_type == ActivationTriggerType::kOnActivateXAbilities)
    {
        // Get the count of abilities activations
        const int total_abilities_activated =
            abilities_component.GetTotalActivatedAbilitiesCount(activation_trigger_data.ability_types);
        if (activation_trigger_data.every_x)
        {
            if (total_abilities_activated % activation_trigger_data.number_of_abilities_activated != 0)
            {
                return false;
            }
        }
        else
        {
            if (total_abilities_activated != activation_trigger_data.number_of_abilities_activated)
            {
                return false;
            }
        }
    }

    // Activate every x deployed skills
    if (activation_trigger_data.trigger_type == ActivationTriggerType::kOnDeployXSkills)
    {
        // Get the count of abilities activations
        const int total_skills_deployed =
            abilities_component.GetDeployedSkillsCount(activation_trigger_data.ability_types);
        if (total_skills_deployed == 0 ||
            total_skills_deployed % activation_trigger_data.number_of_skills_deployed != 0)
        {
            return false;
        }
    }

    // Check health range
    if (activation_trigger_data.health_lower_limit_percentage > 0_fp)
    {
        // Get the trigger level of health
        const StatsData& sender_live_stats = world_->GetLiveStats(sender_entity);
        const FixedPoint health_lower_limit = activation_trigger_data.health_lower_limit_percentage.AsPercentageOf(
            sender_live_stats.Get(StatType::kMaxHealth));

        // If health is lower from trigger level activate innate
        if (sender_live_stats.Get(StatType::kCurrentHealth) >= health_lower_limit)
        {
            return false;
        }
    }

    // Check the ability activation before/after certain time
    const int current_time_ms = Time::TimeStepsToMs(time_step);
    const bool can_activate_before_certain_time =
        activation_trigger_data.not_before_ms == 0 || current_time_ms >= activation_trigger_data.not_before_ms;
    if (!can_activate_before_certain_time)
    {
        return false;
    }
    const bool can_activate_after_certain_time =
        activation_trigger_data.not_after_ms == 0 || current_time_ms <= activation_trigger_data.not_after_ms;
    if (!can_activate_after_certain_time)
    {
        return false;
    }

    // Check if activated this once
    if (activation_trigger_data.once_per_spawned_entity)
    {
        // If we have any duplicates in this then we don't allow
        std::unordered_set<EntityID> spawned_entities_ids;
        for (const AbilityStateActivatorContext& context : state.activator_contexts)
        {
            // Only spawned entities
            if (!EntityHelper::IsSpawned(*world_, context.sender_entity_id))
            {
                continue;
            }

            // Already exists, we don't allow that
            if (spawned_entities_ids.count(context.sender_entity_id) > 0)
            {
                return false;
            }

            spawned_entities_ids.insert(context.sender_entity_id);
        }
    }

    // Special cases for some activation trigger types
    switch (activation_trigger_data.trigger_type)
    {
    case ActivationTriggerType::kInRange:
    {
        const auto& entities = world_->GetAll();

        // Looking for entities in range that meet our criteria
        bool found_any_target = false;
        for (const auto& entity : entities)
        {
            assert(entity);

            // If entity is the sender, going to next entity
            if (entity->GetID() == sender_entity.GetID())
            {
                continue;
            }

            // Receiver does not pass conditions
            if (!world_->DoesPassConditions(*entity, activation_trigger_data.required_receiver_conditions))
            {
                continue;
            }

            // Get distance between entities
            const HexGridPosition units_vector_between = GridHelper::GetUnitsVectorBetween(sender_entity, *entity);
            const int distance = units_vector_between.Length();

            if (distance <= activation_trigger_data.activation_radius_units)
            {
                found_any_target = true;
                break;
            }
        }

        if (!found_any_target)
        {
            return false;
        }
        break;
    }
    case ActivationTriggerType::kOnEnergyFull:
    {
        const auto& stats_component = sender_entity.Get<StatsComponent>();
        if (stats_component.GetCurrentEnergy() < stats_component.GetEnergyCost())
        {
            return false;
        }
        break;
    }
    default:
        break;
    }

    // Has no context, just assume yes
    if (state.activator_contexts.empty())
    {
        return true;
    }

    return CanActivateAbilityByActivatorContext(
        sender_entity,
        state,
        state.activator_contexts.back(),
        with_trigger_counter);
}

bool AbilitySystem::CanActivateAbilityByActivatorContext(
    const Entity& sender_entity,
    const AbilityState& state,
    const AbilityStateActivatorContext& ability_state_activator_context,
    const bool with_trigger_counter) const
{
    const AbilityData& data = *state.data;
    const auto& activation_trigger_data = data.activation_trigger_data;

    if (with_trigger_counter && !state.CanActivateAbilityByTriggerCounter())
    {
        return false;
    }

    if (activation_trigger_data.trigger_type == ActivationTriggerType::kOnDamage)
    {
        // Empty set means "any type of damage"
        if (!activation_trigger_data.damage_types.IsEmpty())
        {
            const auto& event_data = ability_state_activator_context.event.Get<event_data::AppliedDamage>();
            if (!activation_trigger_data.damage_types.Contains(event_data.damage_type))
            {
                return false;
            }

            if (event_data.damage_value < activation_trigger_data.damage_threshold)
            {
                return false;
            }
        }
    }

    // Filter by ability type
    const auto filter_abilities_types = activation_trigger_data.ability_types;
    if (ability_state_activator_context.ability_type != AbilityType::kNone &&
        !filter_abilities_types.Contains(ability_state_activator_context.ability_type))
    {
        return false;
    }

    if (activation_trigger_data.trigger_type == ActivationTriggerType::kOnReceiveXEffectPackages)
    {
        const auto& abilities_component = sender_entity.Get<AbilitiesComponent>();
        int num_received = abilities_component.GetReceivedEffectPackagesCount(activation_trigger_data.ability_types);
        if (activation_trigger_data.number_of_effect_packages_received_modulo != 0)
        {
            num_received %= activation_trigger_data.number_of_effect_packages_received_modulo;
        }

        if (!EvaluateComparison(
                activation_trigger_data.comparison_type,
                FixedPoint::FromInt(num_received),
                FixedPoint::FromInt(activation_trigger_data.number_of_effect_packages_received)))
        {
            return false;
        }
    }

    if (activation_trigger_data.only_from_parent)
    {
        const EntityID parent_id = sender_entity.GetParentID();
        if (parent_id != ability_state_activator_context.sender_combat_unit_entity_id)
        {
            return false;
        }
    }

    if (!DoesActivatorContextMatchAllegianceType(
            sender_entity,
            activation_trigger_data,
            ability_state_activator_context))
    {
        return false;
    }

    // Check the range
    if (activation_trigger_data.range_units != 0)
    {
        const auto& activator_entity = world_->GetByID(ability_state_activator_context.sender_combat_unit_entity_id);

        const HexGridPosition units_vector_between = GridHelper::GetUnitsVectorBetween(sender_entity, activator_entity);
        const int distance = units_vector_between.Length();

        if (distance >= activation_trigger_data.range_units)
        {
            return false;
        }
    }

    // Check the focus
    if (activation_trigger_data.only_focus)
    {
        const auto& activator_entity = world_->GetByID(ability_state_activator_context.sender_combat_unit_entity_id);
        const auto& focus_component = activator_entity.Get<FocusComponent>();

        if (sender_entity.GetID() != focus_component.GetFocusID())
        {
            return false;
        }
    }

    return true;
}

bool AbilitySystem::DoEntitiesMatchAllegianceType(
    const Entity& entity,
    const EntityID other_entity_id,
    AllegianceType allegiance_type) const
{
    if (allegiance_type == AllegianceType::kAll)
    {
        return true;
    }

    if (!world_->HasEntity(other_entity_id))
    {
        return false;
    }

    const auto& other_entity = world_->GetByID(other_entity_id);
    const bool is_ally = entity.IsAlliedWith(other_entity);

    // Check that entities are allies but not the same entity because
    // 'Ally' allegiance type is not superset of 'Self' allegiance type
    if (allegiance_type == AllegianceType::kAlly && (!is_ally || other_entity_id == entity.GetID()))
    {
        return false;
    }

    if (allegiance_type == AllegianceType::kEnemy && is_ally)
    {
        return false;
    }

    if (allegiance_type == AllegianceType::kSelf && entity.GetID() != other_entity.GetID())
    {
        return false;
    }

    return true;
}

bool AbilitySystem::DoesActivatorContextMatchAllegianceType(
    const Entity& sender_entity,
    const AbilityActivationTriggerData& activation_trigger_data,
    const AbilityStateActivatorContext& ability_state_activator_context) const
{
    bool check_sender_allegiance = true;
    bool check_receiver_allegiance = false;

    ILLUVIUM_ENSURE_ENUM_SIZE(ActivationTriggerType, 19);
    switch (activation_trigger_data.trigger_type)
    {
    case ActivationTriggerType::kOnDamage:
    case ActivationTriggerType::kOnHit:
    case ActivationTriggerType::kOnShieldHit:
        check_receiver_allegiance = true;
        break;

    default:
        break;
    }

    if (check_sender_allegiance && !DoEntitiesMatchAllegianceType(
                                       sender_entity,
                                       ability_state_activator_context.sender_combat_unit_entity_id,
                                       activation_trigger_data.sender_allegiance))
    {
        return false;
    }

    if (check_receiver_allegiance && !DoEntitiesMatchAllegianceType(
                                         sender_entity,
                                         ability_state_activator_context.receiver_combat_unit_entity_id,
                                         activation_trigger_data.receiver_allegiance))
    {
        return false;
    }

    return true;
}

void AbilitySystem::DeployInstantTargetedSkill(
    EntityID sender_id,
    EntityID receiver_id,
    const std::shared_ptr<SkillData>& skill_data,
    SourceContextType source_context)
{
    const auto& sender_entity = world_->GetByID(sender_id);
    if (!sender_entity.IsActive())
    {
        return;
    }
    auto& abilities_component = sender_entity.Get<AbilitiesComponent>();

    // Keep track of previous active ability so that we can restore it
    const auto active_ability = abilities_component.GetActiveAbility();
    abilities_component.SetActiveAbility(nullptr);

    // Init skill state with predefined targeting
    SkillState skill_state;
    skill_state.data = skill_data;
    skill_state.targeting_state.CreateFromTarget(*world_, receiver_id, sender_id);

    // Init innate ability with source context
    auto ability_data = AbilityData::Create();
    ability_data->source_context.Add(source_context);

    auto ability_state = AbilityState::Create();
    ability_state->data = ability_data;
    ability_state->ability_type = AbilityType::kInnate;
    ability_state->skills.emplace_back(std::move(skill_state));

    DeploySkill(sender_entity, sender_id, ability_state, ability_state->GetCurrentSkillState());

    // Restore previous ability
    abilities_component.SetActiveAbility(active_ability);
}

bool AbilitySystem::DoesPassContextRequirement(
    const Entity& receiver_entity,
    const AbilityTriggerContextRequirement context_requirement)
{
    if (context_requirement == AbilityTriggerContextRequirement::kDuringOmega)
    {
        const auto& abilities_component = receiver_entity.Get<AbilitiesComponent>();
        const AbilityStatePtr active_ability = abilities_component.GetActiveAbility();
        if (!active_ability)
        {
            return false;
        }

        if (active_ability->ability_type == AbilityType::kOmega)
        {
            return true;
        }

        return false;
    }

    return true;
}

void AbilitySystem::InvalidateTargetIfNeeded(const Entity& entity, const Entity& target_entity)
{
    if (!entity.Has<AbilitiesComponent>())
    {
        return;
    }

    const auto& abilities_component = entity.Get<AbilitiesComponent>();
    AbilityStatePtr active_ability_state = abilities_component.GetActiveAbility();
    if (!active_ability_state)
    {
        return;
    }

    SkillState& skill_state = active_ability_state->GetCurrentSkillState();
    if (skill_state.state != SkillStateType::kWaiting)
    {
        return;
    }

    SkillTargetingState& skill_targeting_state = skill_state.targeting_state;

    const EntityID target_entity_id = target_entity.GetID();
    if (skill_targeting_state.available_targets.erase(target_entity_id) == 0)
    {
        // No target in list, not needed to remove
        return;
    }

    // TODO (konstantin): which state should we save - the one which was saved during targeting or the current one?
    const auto& target_position_component = target_entity.Get<PositionComponent>();
    const auto& target_position = target_position_component.GetPosition();
    skill_targeting_state.targets_saved_data[target_entity_id] = target_position;

    if (active_ability_state->CanRetargetCurrentSkill())
    {
        UpdateSkillTargeting(entity, active_ability_state, skill_state, true);
    }

    if (!skill_state.HasValidTargeting())
    {
        HandleTargetingFailure(entity, active_ability_state);
    }
}

void AbilitySystem::RestoreTargetIfNeeded(const Entity& entity, const Entity& target_entity)
{
    const auto& abilities_component = entity.Get<AbilitiesComponent>();
    const auto active_ability_state = abilities_component.GetActiveAbility();

    if (!active_ability_state)
    {
        return;
    }

    SkillState& skill_state = active_ability_state->GetCurrentSkillState();
    SkillTargetingState& skill_targeting_state = skill_state.targeting_state;

    const EntityID target_entity_id = target_entity.GetID();
    if (skill_targeting_state.targets_saved_data.count(target_entity_id) == 0)
    {
        // Target wasn't in lost list, nothing to restore
        return;
    }

    skill_targeting_state.available_targets.emplace(target_entity_id);
}

bool AbilitySystem::TryInitSkillTargeting(const Entity& entity, AbilityStatePtr& ability, bool activation_targeting)
{
    ILLUVIUM_PROFILE_FUNCTION();

    SkillState* skill_state = nullptr;

    // For activation current skill index might be not valid, because it resets after activation
    if (activation_targeting)
    {
        // TODO(vampy): Make a branch using ability action settings Activation.TargetingCheckType
        // For now we just checking first skill
        // See: https://illuvium.atlassian.net/wiki/spaces/AB/pages/272728065/Activation.TargetingCheckType
        skill_state = &ability->skills.front();
    }
    else
    {
        skill_state = &ability->GetCurrentSkillState();
    }

    UpdateSkillTargeting(entity, ability, *skill_state, false);

    return skill_state->HasValidTargeting();
}

void AbilitySystem::HandleReservingPositionIfNeeded(const Entity& entity, AbilityStatePtr& state)
{
    ILLUVIUM_PROFILE_FUNCTION();

    const SkillState& current_skill_state = state->GetCurrentSkillState();

    // Whether we should reserve a position on the board
    bool needs_to_reserve_position = false;
    ReservedPositionType reserved_position_type = ReservedPositionType::kAcross;

    // Check for reserved positions
    if (current_skill_state.data->ReservesAPosition())
    {
        needs_to_reserve_position = true;

        // Dash uses position on receiver unless jump across
        if (current_skill_state.data->deployment.type == SkillDeploymentType::kDash)
        {
            // Might also want to be behind the receiver
            if (current_skill_state.data->dash.land_behind)
            {
                reserved_position_type = ReservedPositionType::kBehindReceiver;
            }
            else
            {
                reserved_position_type = ReservedPositionType::kNearReceiver;
            }
        }
        else
        {
            for (const auto& effect : current_skill_state.data->effect_package.effects)
            {
                // Blink effect specifies reservation
                if (effect.type_id.type == EffectType::kBlink)
                {
                    reserved_position_type = effect.blink_target;
                }
            }
        }
    }

    if (needs_to_reserve_position)
    {
        const auto& predicted_targets = current_skill_state.targeting_state.available_targets;
        const GridHelper& grid_helper = world_->GetGridHelper();
        grid_helper.TryToReservePosition(entity, predicted_targets, reserved_position_type);
    }
}

bool AbilitySystem::ChooseAndTryToActivateInnateAbilities(
    const AbilityStateActivatorContext& activator_context,
    const EntityID sender_id,
    const ActivationTriggerType trigger_type)
{
    if (!world_->HasEntity(sender_id))
    {
        return false;
    }

    const auto& sender_entity = world_->GetByID(sender_id);
    if (!sender_entity.Has<AbilitiesComponent>())
    {
        return false;
    }

    return ChooseAndTryToActivateInnateAbilities(activator_context, sender_entity, trigger_type);
}

bool AbilitySystem::ChooseAndTryToActivateInnateAbilities(
    const AbilityStateActivatorContext& activator_context,
    const Entity& sender_entity,
    const ActivationTriggerType trigger_type)
{
    if (!sender_entity.Has<AbilitiesComponent>())
    {
        return false;
    }

    // Choose an ability
    auto& abilities_component = sender_entity.Get<AbilitiesComponent>();
    const std::vector<AbilityStatePtr>& abilities = abilities_component.ChooseInnateAbility(trigger_type);
    if (abilities.empty())
    {
        return false;
    }

    // Try to activate all the abilities
    bool activated = false;
    const bool logs_enable_innate_abilities = world_->GetLogsConfig().enable_innate_abilities;
    if (logs_enable_innate_abilities)
    {
        LogDebug(
            sender_entity.GetID(),
            "AbilitySystem::ChooseAndTryToActivateInnateAbilities - abilities.size() = {}, trigger_type = {}",
            abilities.size(),
            trigger_type);
    }

    // Abilities can change size while inside this, this can happen, for example, if an innate spawns a mark that adds
    // another innate
    const size_t size_before_loop = abilities.size();
    for (size_t index = 0; index < size_before_loop; index++)
    {
        AbilityStatePtr ability = abilities[index];
        assert(ability);

        // TODO(vampy): This does not work very well with duplicates and innates that are waiting for a long time
        ability->activator_contexts.push_back(activator_context);

        // Activate innate ability
        bool activated_ability = false;
        bool added_to_queue = false;
        TryToActivateInnateOrAddToQueue(sender_entity, ability, &activated_ability, &added_to_queue);

        if (added_to_queue)
        {
            activated = true;
            LogDebug(
                sender_entity.GetID(),
                "AbilitySystem::ChooseAndTryToActivateInnateAbilities - Added to QUEUE - {}",
                *ability);
        }
        else
        {
            // Remove last
            ability->activator_contexts.pop_back();
            LogDebug(
                sender_entity.GetID(),
                "AbilitySystem::ChooseAndTryToActivateInnateAbilities - Couldn't add to QUEUE - {}",
                *ability);
        }
    }
    return activated;
}

bool AbilitySystem::ActivateAllInstantInnateAbilities(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    if (!entity.Has<AbilitiesComponent>())
    {
        return false;
    }
    const bool logs_enable_innate_abilities = world_->GetLogsConfig().enable_innate_abilities;
    auto& abilities_component = entity.Get<AbilitiesComponent>();

    // Keep track of previous active ability so that we can restore it
    auto previously_active_ability = abilities_component.GetActiveAbility();
    abilities_component.SetActiveAbility(nullptr);

    // Activate all instant innates
    bool activated_any = false;
    const size_t size_instant_innates = abilities_component.GetSizeInstantInnatesWaitingActivation();
    if (size_instant_innates > 0)
    {
        LogDebug(
            entity.GetID(),
            "AbilitySystem::ActivateAllInstantInnateAbilities - size_instant_innates = {}",
            size_instant_innates);
    }

    size_t total_activated = 0;
    std::vector<AbilityStatePtr> innates_to_add_back;
    for (size_t i = 0; i < size_instant_innates; i++)
    {
        AbilityStatePtr instant_innate = abilities_component.PopInstantInnateWaitingActivation();
        if (!instant_innate)
        {
            continue;
        }

        if (TryToActivateInnate(entity, instant_innate))
        {
            activated_any = true;
            total_activated++;
        }
        else
        {
            // Add back to queue if we couldn't activate
            if (logs_enable_innate_abilities)
            {
                LogDebug(
                    entity.GetID(),
                    "AbilitySystem::ActivateAllInstantInnateAbilities - couldn't activate, ADDING BACK TO QUEUE - "
                    "{}",
                    *instant_innate);
            }

            if (!instant_innate->data->activation_trigger_data.immediate_activation_only)
            {
                innates_to_add_back.push_back(instant_innate);
            }
        }
    }

    // Add back the innates that we couldn't activate
    for (const auto& instant_innate : innates_to_add_back)
    {
        abilities_component.AddInnateWaitingActivation(instant_innate);
    }
    innates_to_add_back.clear();

    // Restore previous ability
    abilities_component.SetActiveAbility(previously_active_ability);

    if (activated_any)
    {
        LogDebug(entity.GetID(), "- total_innates_activated = {}", total_activated);
    }

    return activated_any;
}

bool AbilitySystem::AddInnateToQueue(
    const Entity& sender_entity,
    const AbilityStatePtr& ability,
    const bool force_add_to_queue)
{
    ILLUVIUM_PROFILE_FUNCTION();

    const EntityID sender_id = sender_entity.GetID();
    const bool logs_enable_innate_abilities = world_->GetLogsConfig().enable_innate_abilities;

    if (!ability || !ability->data)
    {
        LogErr(sender_id, "| Ability is invalid. You have a BUG IN YOUR CODE.");
        return false;
    }

    if (ability->ability_type != AbilityType::kInnate)
    {
        assert(false);
        LogErr(sender_id, "| Not an Innate. You have a BUG IN YOUR CODE.");
        return false;
    }

    if (!DoesPassContextRequirement(sender_entity, ability->data->activation_trigger_data.context_requirement))
    {
        return false;
    }

    // Extra logs, very spammy
    if (logs_enable_innate_abilities)
    {
        LogDebug(sender_id, "AbilitySystem::AddInnateToQueue - {}", *ability);
    }

    // Can't activate
    // NOTE: this is moved here so that the logs aren't too spamy
    if (!force_add_to_queue && !CanActivateAbility(sender_entity, *ability, world_->GetTimeStepCount()))
    {
        if (logs_enable_innate_abilities)
        {
            LogDebug(sender_id, "| Can't activate, denied by ability");
        }
        return false;
    }

    // No extra logs, prevent spam in log
    if (!logs_enable_innate_abilities)
    {
        LogDebug(sender_id, "AbilitySystem::AddInnateToQueue - {}", *ability);
    }

    // Add to innates queue
    auto& abilities_component = sender_entity.Get<AbilitiesComponent>();
    if (abilities_component.AddInnateWaitingActivation(ability))
    {
        LogDebug(
            sender_id,
            "| Added innate to Queue Waiting activation, num_waiting_activation = {}",
            abilities_component.GetSizeInnatesWaitingActivation());
        return true;
    }

    LogDebug(sender_id, "| Can't add innate to Queue, as a duplicate innate already exists!");
    return true;
}

void AbilitySystem::TryToActivateInnateOrAddToQueue(
    const Entity& sender_entity,
    const AbilityStatePtr& ability,
    bool* out_activated,
    bool* out_added_to_queue)
{
    ILLUVIUM_PROFILE_FUNCTION();

    *out_activated = false;
    *out_added_to_queue = false;
    const bool logs_enable_innate_abilities = world_->GetLogsConfig().enable_innate_abilities;

    const EntityID sender_id = sender_entity.GetID();
    if (world_->IsBattleFinished())
    {
        LogDebug(sender_id, "| Battle finished, can't activate any innate");
        return;
    }

    const bool force_add_to_queue = ability->data->activation_trigger_data.force_add_to_queue_on_activation;

    // Add the innate to the activation queue
    if (!AddInnateToQueue(sender_entity, ability, force_add_to_queue))
    {
        return;
    }
    *out_added_to_queue = true;

    // Stop ability if there is another active ability running.
    auto& abilities_component = sender_entity.Get<AbilitiesComponent>();
    if (abilities_component.HasActiveAbility())
    {
        if (logs_enable_innate_abilities)
        {
            LogDebug(
                sender_id,
                "| Can't activate innate with duration, there is an active ability already running. But try to "
                "activate "
                "all the instant ones");
        }

        // Activate all instants
        ActivateAllInstantInnateAbilities(sender_entity);
        return;
    }

    // Get from the queue and activate that ability
    if (AbilityStatePtr ability_to_activate = abilities_component.PopInnateWaitingActivation())
    {
        // Activate all innates with duration
        LogDebug(
            sender_id,
            "| Getting Innate from Queue, after num_waiting_activation = {}",
            abilities_component.GetSizeInnatesWaitingActivation());

        *out_activated = TryToActivateInnate(sender_entity, ability_to_activate);
    }
    else
    {
        // Activate all instants
        ActivateAllInstantInnateAbilities(sender_entity);
    }
}

bool AbilitySystem::TryToActivateInnate(const Entity& sender_entity, AbilityStatePtr& ability)
{
    ILLUVIUM_PROFILE_FUNCTION();

    const EntityID sender_id = sender_entity.GetID();

    LogDebug(sender_id, "AbilitySystem::TryToActivateInnate - {}", *ability);

    if (ability->ability_type != AbilityType::kInnate)
    {
        assert(false);
        LogErr(sender_id, "| Not an Innate. You have a BUG IN YOUR CODE.");
        return false;
    }

    // Stop ability if there is another active ability running.
    auto& abilities_component = sender_entity.Get<AbilitiesComponent>();
    if (abilities_component.HasActiveAbility())
    {
        LogDebug(sender_id, "| Can't activate, there is an active ability already running.");
        return false;
    }

    // Check Ability conditions
    if (!CanActivateAbility(sender_entity, *ability, world_->GetTimeStepCount()))
    {
        if (world_->GetLogsConfig().enable_innate_abilities)
        {
            LogDebug(sender_id, "| Can't activate, denied by ability (removed from queue)");
        }

        return false;
    }

    // Try to perform targeting
    if (!TryInitSkillTargeting(sender_entity, ability, true))
    {
        LogDebug(sender_id, "| Can't activate innate, denied by ability skill targeting");
        return false;
    }

    // Activate
    Activate(sender_entity, ability);

    return true;
}

}  // namespace simulation
