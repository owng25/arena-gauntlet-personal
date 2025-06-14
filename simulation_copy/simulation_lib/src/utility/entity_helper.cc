#include "entity_helper.h"

#include "components/aura_component.h"
#include "components/beam_component.h"
#include "components/chain_component.h"
#include "components/combat_synergy_component.h"
#include "components/combat_unit_component.h"
#include "components/dash_component.h"
#include "components/drone_augment_component.h"
#include "components/focus_component.h"
#include "components/mark_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/projectile_component.h"
#include "components/shield_component.h"
#include "components/splash_component.h"
#include "components/stats_component.h"
#include "components/zone_component.h"

namespace simulation
{

void EntityHelper::InitMeteredStats(const World& world, const Entity& entity)
{
    if (!IsACombatUnit(entity))
    {
        return;
    }

    // Initialize with the current stats
    const StatsData live_stats = world.GetLiveStats(entity);
    auto& stats_component = entity.Get<StatsComponent>();

    stats_component.SetCurrentHealth(live_stats.Get(StatType::kMaxHealth));
    stats_component.SetCurrentEnergy(live_stats.Get(StatType::kStartingEnergy));
}

void EntityHelper::InitRandomOverflowStats(World& world, const Entity& entity)
{
    if (!IsACombatUnit(entity))
    {
        return;
    }

    // Initialize overflow stats
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/209747993/Random+Overflow+Stat
    auto& stats_component = entity.Get<StatsComponent>();

    stats_component.IncreaseOverflowForType(
        StatType::kCritChancePercentage,
        FixedPoint::FromInt(world.RandomRange(kMinPercentage, kMaxPercentage)));
    stats_component.IncreaseOverflowForType(
        StatType::kHitChancePercentage,
        FixedPoint::FromInt(world.RandomRange(kMinPercentage, kMaxPercentage)));
    stats_component.IncreaseOverflowForType(
        StatType::kAttackDodgeChancePercentage,
        FixedPoint::FromInt(world.RandomRange(kMinPercentage, kMaxPercentage)));
}

bool EntityHelper::IsACombatUnit(const Entity& entity)
{
    return entity.Has<CombatUnitComponent>();
}

const std::string& EntityHelper::GetUniqueID(const World& world, const EntityID entity_id)
{
    static const std::string empty_string;
    if (!world.HasEntity(entity_id))
    {
        return empty_string;
    }

    return GetUniqueID(world.GetByID(entity_id));
}

const std::string& EntityHelper::GetUniqueID(const Entity& entity)
{
    static const std::string empty_string;
    if (!IsACombatUnit(entity))
    {
        return empty_string;
    }

    return entity.Get<CombatUnitComponent>().GetUniqueID();
}

void EntityHelper::Kill(Entity& entity)
{
    entity.Deactivate();

    // Reduce health to zero
    if (entity.Has<StatsComponent>())
    {
        auto& stats_component = entity.Get<StatsComponent>();
        stats_component.SetCurrentHealth(0_fp);
    }
}

bool EntityHelper::IsASynergy(const Entity& entity)
{
    return entity.Has<SynergyEntityComponent>();
}

bool EntityHelper::IsADroneAugment(const Entity& entity)
{
    return entity.Has<DroneAugmentEntityComponent>();
}

bool EntityHelper::IsAIlluvialCombatUnit(const Entity& entity)
{
    if (!entity.Has<CombatUnitComponent>())
    {
        return false;
    }

    const auto& component = entity.Get<CombatUnitComponent>();
    return component.IsAIlluvial();
}

bool EntityHelper::IsARangerCombatUnit(const Entity& entity)
{
    if (!entity.Has<CombatUnitComponent>())
    {
        return false;
    }

    const auto& component = entity.Get<CombatUnitComponent>();
    return component.IsARanger();
}

bool EntityHelper::IsAPetCombatUnit(const Entity& entity)
{
    if (!entity.Has<CombatUnitComponent>())
    {
        return false;
    }

    const auto& component = entity.Get<CombatUnitComponent>();
    return component.IsAPet();
}

bool EntityHelper::IsAChain(const Entity& entity)
{
    return entity.Has<ChainComponent>();
}

bool EntityHelper::IsASplash(const Entity& entity)
{
    return entity.Has<SplashComponent>();
}

bool EntityHelper::IsAProjectile(const Entity& entity)
{
    return entity.Has<ProjectileComponent>();
}

bool EntityHelper::IsAZone(const Entity& entity)
{
    return entity.Has<ZoneComponent>();
}

bool EntityHelper::IsABeam(const Entity& entity)
{
    return entity.Has<BeamComponent>();
}

bool EntityHelper::IsADash(const Entity& entity)
{
    return entity.Has<DashComponent>();
}

bool EntityHelper::IsAShield(const Entity& entity)
{
    return entity.Has<ShieldComponent>();
}

bool EntityHelper::IsAnAura(const Entity& entity)
{
    return entity.Has<AuraComponent>();
}

bool EntityHelper::IsAMark(const Entity& entity)
{
    return entity.Has<MarkComponent>();
}

bool EntityHelper::IsTargetable(const Entity& entity, const AllegianceType allegiance)
{
    // Ignore entities that are not taking space
    if (!EntityHelper::IsCollidable(entity))
    {
        return false;
    }

    if (allegiance == AllegianceType::kEnemy)
    {
        // Check if has any special effects
        if (entity.Has<AttachedEffectsComponent>())
        {
            const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();

            // Not targetable because it has a positive effect that says so
            if (attached_effects_component.HasPositiveState(EffectPositiveState::kUntargetable))
            {
                return false;
            }

            // Not targetable for enemies due to blink
            if (attached_effects_component.HasBlink())
            {
                return false;
            }
        }
    }

    return true;
}

bool EntityHelper::IsImmuneToDetrimentalEffect(const Entity& entity, const EffectTypeID& type_id)
{
    if (entity.Has<AttachedEffectsComponent>())
    {
        const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
        return attached_effects_component.HasImmunityTo(type_id);
    }

    return false;
}

bool EntityHelper::HasNonImmunizedNegativeState(const Entity& entity, EffectNegativeState state)
{
    if (entity.Has<AttachedEffectsComponent>())
    {
        const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
        return attached_effects_component.HasNonImmunizedNegativeState(state);
    }

    return false;
}

bool EntityHelper::IsImmuneToAllDetrimentalEffects(const Entity& entity)
{
    // Not active
    if (!entity.IsActive())
    {
        return false;
    }

    if (entity.Has<AttachedEffectsComponent>())
    {
        const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
        return attached_effects_component.HasImmunityToAllDetrimentalEffects();
    }

    return false;
}

bool EntityHelper::HasPositiveState(const Entity& entity, const EffectPositiveState positive_state)
{
    if (!entity.Has<AttachedEffectsComponent>())
    {
        return false;
    }

    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
    return attached_effects_component.HasPositiveState(positive_state);
}

bool EntityHelper::HasNegativeState(const Entity& entity, const EffectNegativeState negative_state)
{
    if (!entity.Has<AttachedEffectsComponent>())
    {
        return false;
    }

    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
    return attached_effects_component.HasNegativeState(negative_state);
}

bool EntityHelper::IsIndomitable(const Entity& entity)
{
    // Not active
    if (!entity.IsActive())
    {
        return false;
    }

    if (!entity.Has<AttachedEffectsComponent>())
    {
        return false;
    }

    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
    return attached_effects_component.HasPositiveState(EffectPositiveState::kIndomitable);
}

bool EntityHelper::IsMovable(const Entity& entity)
{
    // Not active
    if (!entity.IsActive())
    {
        return false;
    }

    // Only apply to entities with FocusComponent and MovementComponent
    if (!entity.Has<FocusComponent, MovementComponent>())
    {
        return false;
    }

    // Only apply to entities with PositionComponent or DashComponent
    if (!(entity.Has<PositionComponent>() || entity.Has<DashComponent>()))
    {
        return false;
    }

    if (!entity.Has<AttachedEffectsComponent>())
    {
        return true;
    }

    // Absolute immunity check
    if (IsImmuneToAllDetrimentalEffects(entity))
    {
        return true;
    }

    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();

    if (attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kRoot) ||
        attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kStun) ||
        attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kFrozen))
    {
        return false;
    }

    // Check for active displacement
    if (attached_effects_component.HasDisplacement())
    {
        return false;
    }

    // No special effect stops us, can move
    return true;
}

bool EntityHelper::CanGainEnergy(const Entity& entity)
{
    // Does not even exist
    if (!entity.Has<AttachedEffectsComponent>())
    {
        return true;
    }

    // Absolute immunity check
    if (IsImmuneToAllDetrimentalEffects(entity))
    {
        return true;
    }

    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();

    // Not able to activate gain energy because it has a negative effect that says so
    if (attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kLethargic) ||
        attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kFrozen))
    {
        return false;
    }

    return true;
}

bool EntityHelper::CanDodge(const Entity& entity)
{
    // Does not even exist
    if (!entity.Has<AttachedEffectsComponent>())
    {
        return true;
    }

    // Absolute immunity check
    if (IsImmuneToAllDetrimentalEffects(entity))
    {
        return true;
    }

    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
    if (attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kClumsy))
    {
        return false;
    }

    return true;
}

bool EntityHelper::IsPoisoned(const Entity& entity)
{
    if (!entity.Has<AttachedEffectsComponent>())
    {
        return false;
    }

    // Check if has any special effects
    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
    return attached_effects_component.HasCondition(EffectConditionType::kPoison);
}

bool EntityHelper::IsTaunted(const Entity& entity)
{
    return HasNonImmunizedNegativeState(entity, EffectNegativeState::kTaunted);
}

bool EntityHelper::IsCharmed(const Entity& entity)
{
    return HasNonImmunizedNegativeState(entity, EffectNegativeState::kCharm);
}

bool EntityHelper::IsFleeing(const Entity& entity)
{
    return HasNonImmunizedNegativeState(entity, EffectNegativeState::kFlee);
}

bool EntityHelper::IsBlinded(const Entity& entity)
{
    if (!entity.Has<AttachedEffectsComponent>())
    {
        return false;
    }

    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();

    if (attached_effects_component.HasPositiveState(EffectPositiveState::kTruesight))
    {
        return false;
    }

    if (attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kBlind))
    {
        return true;
    }

    return false;
}

bool EntityHelper::IsInvulnerable(const Entity& entity)
{
    if (!entity.Has<AttachedEffectsComponent>())
    {
        return false;
    }

    // Check if has any special effects
    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
    return attached_effects_component.HasPositiveState(EffectPositiveState::kInvulnerable);
}

bool EntityHelper::IsBurned(const Entity& entity)
{
    if (!entity.Has<AttachedEffectsComponent>())
    {
        return false;
    }

    // Check if has any special effects
    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
    return attached_effects_component.HasCondition(EffectConditionType::kBurn);
}

bool EntityHelper::IsFrosted(const Entity& entity)
{
    if (!entity.Has<AttachedEffectsComponent>())
    {
        return false;
    }

    // Check if has any special effects
    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
    return attached_effects_component.HasCondition(EffectConditionType::kFrost);
}

bool EntityHelper::IsFocused(const Entity& entity)
{
    return HasNonImmunizedNegativeState(entity, EffectNegativeState::kFocused);
}

bool EntityHelper::IsFrozen(const Entity& entity)
{
    return HasNonImmunizedNegativeState(entity, EffectNegativeState::kFrozen);
}

bool EntityHelper::IsWounded(const Entity& entity)
{
    if (!entity.Has<AttachedEffectsComponent>())
    {
        return false;
    }

    // Check if has any special effects
    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
    return attached_effects_component.HasCondition(EffectConditionType::kWound);
}

bool EntityHelper::CanInterruptAbility(const Entity& entity, const AbilityType ability_type)
{
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/205357313/Interrupt
    // Bool to determine if an ability is interrupted

    // Not active
    if (!entity.IsActive())
    {
        return false;
    }

    if (!entity.Has<AttachedEffectsComponent>())
    {
        return false;
    }

    // Absolute immunity check
    if (IsImmuneToAllDetrimentalEffects(entity))
    {
        return false;
    }

    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();

    if (ability_type == AbilityType::kOmega)
    {
        // Not able to activate omega attack because it has a negative effect that says so
        if (attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kStun) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kSilenced) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kFrozen) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kTaunted) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kCharm) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kFlee))
        {
            return true;
        }
    }
    else if (ability_type == AbilityType::kAttack)
    {
        // Not able to activate attack ability/innate because it has a negative effect that says so
        if (attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kStun) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kDisarm) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kFrozen) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kCharm) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kFlee))
        {
            return true;
        }
    }

    // Check for active displacement
    if (attached_effects_component.HasDisplacement())
    {
        return true;
    }

    return false;
}

bool EntityHelper::CanActivateAbility(const Entity& entity, const AbilityType ability_type)
{
    // Not active
    if (!entity.IsActive())
    {
        return false;
    }

    if (!entity.Has<AttachedEffectsComponent>())
    {
        return true;
    }

    // Absolute immunity check
    if (IsImmuneToAllDetrimentalEffects(entity))
    {
        return true;
    }

    const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();

    if (ability_type == AbilityType::kOmega)
    {
        // Not able to activate omega attack because it has a negative effect that says so
        if (attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kSilenced) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kFrozen) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kStun) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kTaunted) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kCharm) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kFlee))
        {
            return false;
        }
    }
    else if (ability_type == AbilityType::kAttack)
    {
        // Not able to activate attack ability/innate because it has a negative effect that says so
        if (attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kDisarm) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kFrozen) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kStun) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kCharm) ||
            attached_effects_component.HasNonImmunizedNegativeState(EffectNegativeState::kFlee))
        {
            return false;
        }
    }

    // Check for active displacement
    if (attached_effects_component.HasDisplacement())
    {
        return false;
    }

    return true;
}

bool EntityHelper::CanApplyEffect(const Entity& entity)
{
    const std::shared_ptr<World> world = entity.GetOwnerWorld();
    const EntityID combat_unit_sender_id = world->GetCombatUnitParentID(entity);

    return combat_unit_sender_id != kInvalidEntityID || IsASynergy(entity) || IsADroneAugment(entity);
}

bool EntityHelper::HasEnoughEnergyForOmega(const Entity& entity)
{
    // Does not have the required stats component
    if (!entity.Has<StatsComponent>())
    {
        return false;
    }

    const auto& stats_component = entity.Get<StatsComponent>();
    return stats_component.GetCurrentEnergy() >= stats_component.GetEnergyCost();
}

bool EntityHelper::CanRangerBondWithIlluvial(const World& world, const EntityID ranger_id, const EntityID illuvial_id)
{
    // Must exist
    if (!world.HasEntity(ranger_id))
    {
        return false;
    }
    if (!world.HasEntity(illuvial_id))
    {
        return false;
    }

    const Entity& ranger_entity = world.GetByID(ranger_id);
    const Entity& illuvial_entity = world.GetByID(illuvial_id);
    return CanRangerBondWithIlluvial(ranger_entity, illuvial_entity);
}

std::optional<std::tuple<CombatAffinity, CombatClass>> EntityHelper::GetIlluvialPrimarySynergies(const Entity& entity)
{
    const auto world = entity.GetOwnerWorld();
    if (!world || !IsAIlluvialCombatUnit(entity))
    {
        return std::nullopt;
    }

    const auto& synergies_component = entity.Get<CombatSynergyComponent>();
    return std::make_tuple(
        synergies_component.GetDominantCombatAffinity(),
        synergies_component.GetDominantCombatClass());
}

bool EntityHelper::CanRangerBondWithIlluvial(const Entity& ranger_entity, const Entity& illuvial_entity)
{
    // Must be the correct type
    if (!IsARangerCombatUnit(ranger_entity))
    {
        return false;
    }
    if (!IsAIlluvialCombatUnit(illuvial_entity))
    {
        return false;
    }

    // Must be allies
    if (!ranger_entity.IsAlliedWith(illuvial_entity))
    {
        return false;
    }

    // Has both combat affinity and combat class null
    const auto [primary_affinity, primary_class] = *GetIlluvialPrimarySynergies(illuvial_entity);
    if (primary_affinity == CombatAffinity::kNone && primary_class == CombatClass::kNone)
    {
        return false;
    }

    return true;
}

bool EntityHelper::IsCollidable(const Entity& entity)
{
    // Ignore entities no longer active
    if (!entity.IsActive())
    {
        return false;
    }

    // Ignore entities without positions
    if (!entity.Has<PositionComponent>())
    {
        return false;
    }

    // Ignore entites not taking up space
    const auto& position_component = entity.Get<PositionComponent>();
    if (!position_component.IsTakingSpace())
    {
        return false;
    }

    return true;
}

}  // namespace simulation
