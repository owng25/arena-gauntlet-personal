#pragma once

#include "ecs/entity.h"
#include "ecs/world.h"

namespace simulation
{

/* -------------------------------------------------------------------------------------------------------
 * EntityHelper
 *
 * Helper methods for an entity type/state/etc.
 * --------------------------------------------------------------------------------------------------------
 */
class EntityHelper
{
public:
    //
    // Entity type
    //

    // Initializes the metered stats of the entity
    static void InitMeteredStats(const World& world, const Entity& entity);

    // Initializes the overflow stats using random 0-100
    static void InitRandomOverflowStats(World& world, const Entity& entity);

    // Helper method that tells us if the entity with id is a combat unit
    static bool IsACombatUnit(const Entity& entity);

    // Gets the unique id of the specified entity (if any).
    static const std::string& GetUniqueID(const World& world, const EntityID entity_id);
    static const std::string& GetUniqueID(const Entity& entity);

    // Helper method that deactivates/faints/kills this entity
    static void Kill(Entity& entity);

    // Helper method that tells us if the entity with id is a combat unit with a type
    static bool IsAIlluvialCombatUnit(const Entity& entity);
    static bool IsARangerCombatUnit(const Entity& entity);
    static bool IsAPetCombatUnit(const Entity& entity);

    // Helper method that tells us if the entity with id is a chain
    static bool IsAChain(const Entity& entity);

    // Helper method that tells us if the entity with id is a splash
    static bool IsASplash(const Entity& entity);

    // Helper method that tells us if the entity with id is a projectile
    static bool IsAProjectile(const Entity& entity);

    // Helper method that tells us if the entity with id is a zone
    static bool IsAZone(const Entity& entity);

    // Helper method that tells us if the entity with id is a beam
    static bool IsABeam(const Entity& entity);

    // Helper method that tells us if the entity with id is a dash
    static bool IsADash(const Entity& entity);

    // Helper method that tells us if the entity with id is a shield
    static bool IsAShield(const Entity& entity);

    // Helper method that tells us if the entity with id is an aura
    static bool IsAnAura(const Entity& entity);

    // Helper method that tells us if the entity with id is a mark
    static bool IsAMark(const Entity& entity);

    // Helper method that tells us if the entity with an ID can be attached
    static bool IsAttachable(const Entity& entity)
    {
        return IsAShield(entity) || IsAMark(entity);
    }

    // Helper method that tells us if the entity with id is a synergy unit
    static bool IsASynergy(const Entity& entity);

    // Helper method that tells us if the entity with id is a synergy unit
    static bool IsADroneAugment(const Entity& entity);

    // Helper method that tells us if this entity is spawned (zone/chain/beam)
    // NOTE: spawned entities can be destroyed.
    static bool IsSpawned(const Entity& entity)
    {
        return !IsACombatUnit(entity);
    }

    //
    // Entity state and stats helper methods
    //

    // Helper method that tells us if we can target the entity with id
    static bool IsTargetable(const Entity& entity, const AllegianceType allegiance = AllegianceType::kEnemy);

    // Helper method that tells us if the entity id is immune to detrimental attachables
    static bool IsImmuneToDetrimentalEffect(const Entity& entity, const EffectTypeID& type_id);

    // Helper method that tells us if the entity id is immune to ALL detrimental effects
    static bool IsImmuneToAllDetrimentalEffects(const Entity& entity);

    static bool IsImmuneToNegativeState(const Entity& entity, EffectNegativeState state)
    {
        EffectTypeID type_id;
        type_id.type = EffectType::kNegativeState;
        type_id.negative_state = state;
        return IsImmuneToDetrimentalEffect(entity, type_id);
    }

    static bool HasNonImmunizedNegativeState(const Entity& entity, EffectNegativeState state);

    // Helper method that tells us if the entity id has truesight
    static bool HasTruesight(const Entity& entity)
    {
        return HasPositiveState(entity, EffectPositiveState::kTruesight);
    }

    // Helper method that tells if this entity has this positive state
    static bool HasPositiveState(const Entity& entity, const EffectPositiveState positive_state);
    static bool HasPositiveState(const World& world, const EntityID id, const EffectPositiveState positive_state)
    {
        if (!world.HasEntity(id))
        {
            return false;
        }

        return HasPositiveState(world.GetByID(id), positive_state);
    }

    // Helper method that tells if this entity has this negative state
    static bool HasNegativeState(const Entity& entity, const EffectNegativeState negative_state);
    static bool HasNegativeState(const World& world, const EntityID id, const EffectNegativeState negative_state)
    {
        if (!world.HasEntity(id))
        {
            return false;
        }

        return HasNegativeState(world.GetByID(id), negative_state);
    }

    // Helper method that tells us if the entity id is indomitable and cannot die
    static bool IsIndomitable(const Entity& entity);

    // Helper method that tells us if we can move the entity with id
    static bool IsMovable(const Entity& entity);

    // Helper method that tells us if an entity can gain energy
    static bool CanGainEnergy(const Entity& entity);

    // Helper method that tells us if an entity can dodge
    static bool CanDodge(const Entity& entity);

    // Helper method that tells us if entity is poisoned
    static bool IsPoisoned(const Entity& entity);

    // Helper method that tells us if entity is burned
    static bool IsBurned(const Entity& entity);

    // Helper method that tells us if entity is frosted
    static bool IsFrosted(const Entity& entity);

    // Helper method that tells us if entity is focused
    static bool IsFocused(const Entity& entity);

    // Helper method that tells us if entity is frozen
    static bool IsFrozen(const Entity& entity);

    // Helper method that tells us if entity is Invulnerable
    static bool IsInvulnerable(const Entity& entity);

    // Helper method that tells us if entity is blinded
    static bool IsBlinded(const Entity& entity);

    // Helper method that tells us if entity is wounded
    static bool IsWounded(const Entity& entity);

    // Helper method that tells us if entity is taunted
    static bool IsTaunted(const Entity& entity);

    // Helper method that tells us if entity is charmed
    static bool IsCharmed(const Entity& entity);

    // Helper method that tells us if entity is fleeing
    static bool IsFleeing(const Entity& entity);

    // Can this ability_type be interrupted?
    static bool CanInterruptAbility(const Entity& entity, const AbilityType ability_type);

    // Helper method that tells us if we can activate attack ability
    static bool CanActivateAbility(const Entity& entity, const AbilityType ability_type);

    static bool CanApplyEffect(const Entity& entity);

    // Helper method to check if the entity with id can do omega abilities
    static bool HasEnoughEnergyForOmega(const Entity& entity);

    // Helper function.
    // Can the ranger_id bond with the illuvial_id ?
    static bool CanRangerBondWithIlluvial(const World& world, const EntityID ranger_id, const EntityID illuvial_id);
    static bool CanRangerBondWithIlluvial(const Entity& ranger_entity, const Entity& illuvial_entity);
    static std::optional<std::tuple<CombatAffinity, CombatClass>> GetIlluvialPrimarySynergies(const Entity& entity);

    // Helper function
    // Check if the entity is active, taking up space and can be collided with by other entities
    static bool IsCollidable(const Entity& entity);

    // Create overload variants that just specify the id and where we have to find the entity
#define ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(OverloadedMethodName)                 \
    static bool OverloadedMethodName(const World& world, const EntityID entity_id) \
    {                                                                              \
        if (!world.HasEntity(entity_id))                                           \
        {                                                                          \
            return false;                                                          \
        }                                                                          \
                                                                                   \
        return OverloadedMethodName(world.GetByID(entity_id));                     \
    }

    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsACombatUnit)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsAIlluvialCombatUnit)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsARangerCombatUnit)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsAPetCombatUnit)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsAChain)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsASplash)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsAProjectile)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsAZone)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsABeam)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsADash)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsAShield)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsAnAura)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsAMark)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsAttachable)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsASynergy)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsADroneAugment)

    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(HasTruesight)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsIndomitable)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsMovable)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(CanGainEnergy)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(CanDodge)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsPoisoned)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsTaunted)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsCharmed)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsFleeing)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsBlinded)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsInvulnerable)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsBurned)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsFrosted)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsFocused)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsFrozen)
    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsWounded)

    ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL(IsSpawned)

    static bool
    IsTargetable(const World& world, const EntityID entity_id, const AllegianceType allegiance = AllegianceType::kEnemy)
    {
        if (!world.HasEntity(entity_id))
        {
            return false;
        }

        return IsTargetable(world.GetByID(entity_id), allegiance);
    }

#undef ILLUVIUM_CREATE_OVERLOAD_ENTITY_BOOL
};

}  // namespace simulation
