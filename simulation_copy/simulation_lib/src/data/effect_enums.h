#pragma once

#include "utility/enum_as_index.h"

namespace simulation
{
// Tells us the type of the effect package
enum class EffectType
{
    kNone = 0,

    // Spawns a shield in the world
    kSpawnShield,

    // Spawn a mark in the world
    kSpawnMark,

    // Used to create an effect with just damage
    kInstantDamage,

    // Instantly heals the target
    kInstantHeal,

    // Has bools to cleanse multiple detrimental effects
    kCleanse,

    // Does some damage over a period of time.
    kDamageOverTime,

    // A Condition is a special type of Attachable Effect that allows us to combine a stacking Metered Combat Stat
    // Change Over Time or Combat Stat Modifier with a Mark in a way that is generic.
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/72356344/Condition
    kCondition,

    // Relates to shutting down some functionality of the combat unit.
    // See EffectNegativeState for the list of possible values.
    kNegativeState,

    // Modifies a stat in a negative way
    kDebuff,

    // Displaces the target in some way
    kDisplacement,

    // Does some heal over a period of time
    kHealOverTime,

    // Does some energy gain over a period of time
    kEnergyGainOverTime,

    // Modifies a stat in a positive way
    kBuff,

    // An Empower is an Attachable Effect that specifies one or more Effect and/or an Effect Attribute and/or Effect
    // Package Attribute to be added to the current Skill.EffectPackage of an Ability.
    kEmpower,

    // A Disempower is an Attachable Effect that specifies an Effect Package Attribute to be subtracted from the current
    // Skill.EffectPackage of an Ability.
    kDisempower,

    // Relates to removing the possibility of some negative functionality of the combat unit
    // See EffectPositiveState for the list of possible values.
    kPositiveState,

    // Removes a set amount of energy from the receiver
    kInstantEnergyBurn,

    // Gains a certain amount of energy
    kInstantEnergyGain,

    // Removes a set amount of energy per time step
    kEnergyBurnOverTime,

    // Causes a Combat Unit to be immediately Taken Down when their current % Health is below the value
    kExecute,

    // Causes a Combat Unit to be teleported to the edge of the opposite side of the board
    kBlink,

    // Gains a set amount of hyper per time step
    kHyperGainOverTime,

    // Removes a set amount of hyper from the receiver
    kHyperBurnOverTime,

    // Gains a certain amount of hyper
    kInstantHyperGain,

    // Removes a set amount of hyper from the receiver
    kInstantHyperBurn,

    // Change to a different plane
    kPlaneChange,

    // Changes stat base value. It is a permanent change and can only be overwritten by
    // another stat override effect. Should only be applied by augments
    kStatOverride,

    // Deals instant damage to shields
    kInstantShieldBurn,

    // Auras bring buff or debuff around the unit they are attached to
    kAura,

    // -new values can be added above this line
    kNum
};

ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(EffectType);

// Enum that lists all the possible damage types
enum class EffectDamageType
{
    kNone = 0,

    kPhysical,
    kEnergy,
    kPure,
    kPurest,

    // -new values can be added above this line
    kNum
};

ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(EffectDamageType);

// Used for a health gain effect type
enum class EffectHealType
{
    kNone = 0,

    kNormal,
    kPure,

    // -new values can be added above this line
    kNum
};

// Tells us the type of the positive state
// Used with EffectType::kPositiveState.
enum class EffectPositiveState
{
    kNone = 0,

    // Attached detrimental Effects are disabled for the duration.
    kImmune,

    // EffectPackageBlock is a type of Positive State Effect that disallows a Combat Unit to receive an Effect Package
    // that contains a Detrimental Effect.
    kEffectPackageBlock,

    // Can't have its health reduced to zero.
    kIndomitable,

    // Can't be damaged.
    kInvulnerable,

    // The combat unit is not able to be blinded
    // TODO(vampy): Redo this
    kTruesight,

    // Can't be targeted by enemies.
    kUntargetable,

    // -new values can be added above this line
    kNum
};

// Tells us the type of the negative state
// Used with EffectType::kNegativeState.
enum class EffectNegativeState
{
    kNone = 0,

    // Forces focus onto a combat unit, if possible
    kFocused,

    // Can’t attack ability, move, or activate Omega Abilities.
    // Cannot gain energy.
    // Cannot gain Frost stacks
    kFrozen,

    // Can’t attack ability, move, or activate Omega Abilities.
    // Energy gain still occurs.
    kStun,

    // Cannot attack ability.
    kDisarm,

    // Misses all attack abilities.
    kBlind,

    // Cannot move.
    kRoot,

    // Cannot activate Omega Abilities.
    kSilenced,

    // Cannot gain energy from any source.
    kLethargic,

    // Cannot dodge
    kClumsy,

    // Forced to target unit that applied taunt + can't activate omega abilities
    kTaunted,

    // Forced to move in the direction from the sender entity.
    // Cannot attack.
    // Cannot activate Omega Abilities.
    kCharm,

    // Forced to move in the opposite direction from the sender entity.
    // Cannot attack.
    // Cannot activate Omega Abilities.
    kFlee,

    // -new values can be added above this line
    kNum
};

enum class EffectPlaneChange
{
    kNone = 0,

    // Entity is in the air
    kAirborne,

    // Entity is underground
    kUnderground,

    // -new values can be added above this line
    kNum
};

// Lists all the predefined condition
enum class EffectConditionType
{
    kNone = 0,

    // Poison is a Condition.Type of a Condition that deals Pure Damage based on the  Max Health. At
    // Condition.MaxStacks, it applies a Debuff to Health Gain Efficiency.
    kPoison,

    // Wound is a Condition.Type of a Condition that deals a percentage of the Physical Damage applied by the Effect
    // Package. At Max Stacks, Crit Amp (CritAmplification) is reduced.
    kWound,

    // A Burn is a Condition.Type that deals a percentage of missing Health as Energy Damage. At Max Stacks, the Combat
    // Unit ignites, dealing AOE damage in a small zone.
    kBurn,

    // Frost is a Condition.Type that reduces the Attack Speed of the affected Combat Unit. At Max Stacks, the Combat
    // Unit is Frozen.
    kFrost,

    // -new values can be added above this line
    kNum
};
ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(EffectConditionType);

// Conditions that must be true for an effect to apply
enum class ConditionType
{
    kNone = 0,

    // Used to track conditions on effects, this corresponds to the EffectConditionType::kPoison
    kPoisoned,

    // Used to track conditions on effects, this corresponds to the EffectConditionType::kWound
    kWounded,

    // Used to track conditions on effects, this corresponds to the EffectConditionType::kBurn
    kBurned,

    // Used to track conditions on effects, this corresponds to the EffectConditionType::kFrost
    kFrosted,

    // Used to track conditions on effects, this corresponds to the EffectType::kSpawnShield
    kShielded,

    // -new values can be added above this line
    kNum
};

ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(ConditionType);

// How does the EffectType::kPropagation propagate
enum class EffectPropagationType
{
    kNone = 0,

    kChain,
    kSplash,

    // -new values can be added above this line
    kNum
};

// What displacement type does this effect contain
enum class EffectDisplacementType
{
    kNone = 0,

    kKnockBack,
    kKnockUp,
    kPull,
    kThrowToFurthestEnemy,
    kThrowToHighestEnemyDensity,

    // -new values can be added above this line
    kNum
};

// The types for the overlap processing
// https://illuvium.atlassian.net/wiki/spaces/AB/pages/280068257/Overlap+Processing+Type
enum class EffectOverlapProcessType
{
    kNone = 0,

    kSum,
    kHighest,
    kStacking,
    kShield,
    kCondition,
    kMerge,

    // -new values can be added above this line
    kNum
};

enum class MarkEffectType
{
    kNone = 0,

    kBeneficial,
    kDetrimental,

    kNum
};

}  // namespace simulation
