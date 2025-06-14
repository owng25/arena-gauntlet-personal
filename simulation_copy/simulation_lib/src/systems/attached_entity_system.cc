#include "systems/attached_entity_system.h"

#include "components/abilities_component.h"
#include "components/attached_entity_component.h"
#include "components/deferred_destruction_component.h"
#include "components/mark_component.h"
#include "components/shield_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "factories/entity_factory.h"
#include "utility/attached_effects_helper.h"
#include "utility/entity_helper.h"

namespace simulation
{
void AttachedEntitySystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kShieldCreated>(this, &Self::OnShieldCreated);
    world_->SubscribeMethodToEvent<EventType::kShieldDestroyed>(this, &Self::OnShieldDestroyed);
    world_->SubscribeMethodToEvent<EventType::kShieldWasHit>(this, &Self::OnShieldWasHit);
    world_->SubscribeMethodToEvent<EventType::kShieldExpired>(this, &Self::OnOtherShieldEvent);
    world_->SubscribeMethodToEvent<EventType::kShieldPendingDestroyed>(this, &Self::OnOtherShieldEvent);
    world_->SubscribeMethodToEvent<EventType::kApplyShieldGain>(this, &Self::OnGainShieldEvent);
    world_->SubscribeMethodToEvent<EventType::kMarkCreated>(this, &Self::OnMarkCreated);
    world_->SubscribeMethodToEvent<EventType::kMarkDestroyed>(this, &Self::OnMarkDestroyed);
}

void AttachedEntitySystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    TimeStepShieldEntity(entity);
    TimeStepMarkEntity(entity);
    ApplyStartingShield(entity);
}

void AttachedEntitySystem::TimeStepMarkEntity(const Entity& mark_entity) const
{
    if (!EntityHelper::IsAMark(mark_entity))
    {
        return;
    }

    const auto& mark_component = mark_entity.Get<MarkComponent>();
    const EntityID mark_id = mark_entity.GetID();
    if (!mark_component.ShouldDestroyOnSenderDeath())
    {
        return;
    }

    // Check if sender is still alive
    if (!world_->HasEntity(mark_component.GetSenderID()))
    {
        LogErr(
            mark_id,
            "AttachedEntitySystem::TimeStepMarkEntity - sender_id = {} does not exist",
            mark_component.GetSenderID());
        return;
    }

    if (!world_->GetByID(mark_component.GetSenderID()).IsActive())
    {
        LogDebug(
            mark_id,
            "AttachedEntitySystem::TimeStepMarkEntity - sender_id = {} is not active anymore, destroying mark",
            mark_component.GetSenderID());

        // Let it be destroyed
        auto& destruction_component = mark_entity.Get<DeferredDestructionComponent>();
        if (destruction_component.IsPendingDestruction())
        {
            // Already pending destruction
            return;
        }

        destruction_component.SetPendingDestruction(DestructionReason::kExpired);
    }
}

void AttachedEntitySystem::TimeStepShieldEntity(const Entity& shield_entity) const
{
    if (!EntityHelper::IsAShield(shield_entity))
    {
        return;
    }

    // Still has health
    const auto& shield_component = shield_entity.Get<ShieldComponent>();
    if (shield_component.GetValue() > 0_fp)
    {
        return;
    }

    // Let it be destroyed
    const EntityID shield_id = shield_entity.GetID();
    auto& destruction_component = shield_entity.Get<DeferredDestructionComponent>();
    if (destruction_component.IsPendingDestruction())
    {
        // Already pending destruction
        return;
    }

    destruction_component.SetPendingDestruction(DestructionReason::kDamaged);

    // Send pending destroyed event
    LogDebug(shield_id, "AttachedEntitySystem::TimeStepShieldEntity - Shield pending destruction because value is 0");
    world_->BuildAndEmitEvent<EventType::kShieldPendingDestroyed>(
        shield_id,
        shield_component.GetSenderID(),
        shield_component.GetReceiverID());
}

void AttachedEntitySystem::OnShieldCreated(const event_data::ShieldCreated& data)
{
    // We are only interested in shields
    if (!EntityHelper::IsAShield(*world_, data.entity_id))
    {
        return;
    }
    const auto& entity = world_->GetByID(data.entity_id);
    const auto& shield_stats_component = entity.Get<ShieldComponent>();

    // Add attached effects on creation
    const auto& attached_effects = shield_stats_component.GetAttachedEffects();
    const AttachedEffectsHelper& attached_effects_helper = world_->GetAttachedEffectsHelper();
    for (const auto& attached_effect : attached_effects)
    {
        attached_effects_helper.AddAttachedEffect(entity, data.sender_id, attached_effect, EffectState{});
    }

    TryToActivateAbilitiesForEventType(entity, EventType::kShieldCreated);
}

void AttachedEntitySystem::OnShieldDestroyed(const event_data::ShieldDestroyed& data)
{
    // We are only interested in shields
    if (!EntityHelper::IsAShield(*world_, data.entity_id))
    {
        return;
    }
    const auto& entity = world_->GetByID(data.entity_id);
    const auto& shield_stats_component = entity.Get<ShieldComponent>();
    const EntityID receiver_id = shield_stats_component.GetReceiverID();

    // Remove the shield from its owner
    const auto& receiver_entity = world_->GetByID(receiver_id);
    auto& attached_entity_component = receiver_entity.Get<AttachedEntityComponent>();
    attached_entity_component.RemoveAttachedEntity(data.entity_id);

    TryToActivateAbilitiesForEventType(entity, EventType::kShieldDestroyed);
}

void AttachedEntitySystem::OnShieldWasHit(const event_data::ShieldWasHit& data)
{
    // We are only interested in shields
    if (!EntityHelper::IsAShield(*world_, data.entity_id))
    {
        return;
    }
    const auto& entity = world_->GetByID(data.entity_id);

    TryToActivateAbilitiesForEventType(entity, EventType::kShieldWasHit);
}

void AttachedEntitySystem::OnOtherShieldEvent(const Event& event)
{
    const auto& data = event.Get<event_data::Shield>();

    // We are only interested in shields
    if (!EntityHelper::IsAShield(*world_, data.entity_id))
    {
        return;
    }
    const auto& entity = world_->GetByID(data.entity_id);

    TryToActivateAbilitiesForEventType(entity, event.GetType());
}

void AttachedEntitySystem::OnMarkCreated(const event_data::MarkCreated& data)
{
    // We are only interested in marks
    if (!EntityHelper::IsAMark(*world_, data.entity_id))
    {
        return;
    }

    const auto& receiver_entity = world_->GetByID(data.receiver_id);
    const auto& mark_entity = world_->GetByID(data.entity_id);
    const auto& mark_component = mark_entity.Get<MarkComponent>();

    // Create the abilities
    std::vector<std::shared_ptr<const AbilityData>> innate_abilities;

    const auto& abilities_data = mark_component.GetAbilities();
    for (const auto& sample_ability_data : abilities_data)
    {
        // TODO: move attached_from_entity_id to some transient
        // object (ability state?) to avoid copying of ability data
        auto ability_data = sample_ability_data->CreateDeepCopy();

        // Keep the ID of the mark
        ability_data->attached_from_entity_id = data.entity_id;
        ability_data->source_context = data.source_context;

        // Add ability
        innate_abilities.emplace_back(ability_data);
    }

    // Add innate ability to mark owner entity.
    auto& receiver_abilities_component = receiver_entity.Get<AbilitiesComponent>();
    receiver_abilities_component.AddDataInnateAbilities(innate_abilities);
}

void AttachedEntitySystem::OnMarkDestroyed(const event_data::MarkDestroyed& data)
{
    // We are only interested in marks
    if (!EntityHelper::IsAMark(*world_, data.entity_id))
    {
        return;
    }

    const auto& mark_entity = world_->GetByID(data.entity_id);
    const auto& mark_component = mark_entity.Get<MarkComponent>();
    const EntityID receiver_id = mark_component.GetReceiverID();

    // Remove the mark from its owner
    const auto& receiver_entity = world_->GetByID(receiver_id);
    auto& attached_entity_component = receiver_entity.Get<AttachedEntityComponent>();
    attached_entity_component.RemoveAttachedEntity(data.entity_id);

    // TODO(vampy): Handle cases when the mark dies but the ability still exists inside
    // innate_abilities_waiting_activation_
    auto& abilities_component = receiver_entity.Get<AbilitiesComponent>();
    abilities_component.RemoveInnateAbilityByAttachedEntityID(data.entity_id);
}

void AttachedEntitySystem::TryToActivateAbilitiesForEventType(const Entity& shield_entity, const EventType event_type)
    const
{
    auto& shield_component = shield_entity.Get<ShieldComponent>();

    // Activate the skill if applicable
    const auto event_skill = shield_component.GetSkillForEvent(event_type);
    if (!event_skill)
    {
        return;
    }

    // Create the attack abilities
    AbilitiesData attack_abilities{};

    // Ability timers should be at zero
    attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Create the skill
    auto& ability = attack_abilities.AddAbility();
    // Use only once - this ability will sit idle until it is replaced
    ability.is_use_once = true;

    // Shield abilities shouldn't use the illuvial's attack speed
    ability.ignore_attack_speed = true;

    ability.AddSkill(event_skill);
    ability.source_context = shield_component.GetSourceContextData();

    // Set/reset abilities
    auto& abilities_component = shield_entity.Get<AbilitiesComponent>();
    abilities_component.SetAbilitiesData(attack_abilities, AbilityType::kAttack);
}

void AttachedEntitySystem::OnGainShieldEvent(const event_data::ApplyShieldGain& shield_gain)
{
    ApplyShieldGain(shield_gain.receiver_id, shield_gain.gain_amount, shield_gain.duration_time_ms);
}

void AttachedEntitySystem::ApplyShieldGain(
    const EntityID receiver_id,
    const FixedPoint gain_amount,
    const int duration_time_ms)
{
    const auto& receiver_entity = world_->GetByID(receiver_id);

    // Create new shield
    ShieldData new_shield_data;
    new_shield_data.sender_id = receiver_id;
    new_shield_data.receiver_id = receiver_id;
    new_shield_data.duration_ms = duration_time_ms;
    new_shield_data.value = gain_amount;

    // Spawn a new shield based on values
    EntityFactory::SpawnShieldAndAttach(*world_, receiver_entity.GetTeam(), new_shield_data);

    LogDebug(
        receiver_id,
        "- ApplyShieldGain = SpawnShield, gain_amount = {}, receiver = {}, duration_time_ms = {}",
        gain_amount,
        receiver_id,
        duration_time_ms);
}

void AttachedEntitySystem::ApplyStartingShield(const Entity& entity)
{
    // Check if combat unit
    if (!entity.Has<AttachedEntityComponent>())
    {
        return;
    }

    const EntityID entity_id = entity.GetID();
    if (starting_shield_activated_.count(entity_id) > 0)
    {
        // Already in set
        return;
    }
    starting_shield_activated_.insert(entity_id);

    // Skip if starting shield is 0
    const StatsData& live_stats = world_->GetLiveStats(entity);
    if (live_stats.Get(StatType::kStartingShield) == 0_fp)
    {
        return;
    }

    LogDebug(entity_id, "AttachedEntitySystem::ApplyStartingShield - Create Starting Shield");
    ApplyShieldGain(entity_id, live_stats.Get(StatType::kStartingShield), kTimeInfinite);
}

}  // namespace simulation
