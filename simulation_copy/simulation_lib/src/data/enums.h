#pragma once

#include "utility/enum_as_index.h"
#include "utility/fixed_point.h"

namespace simulation
{
// Player team
enum class Team : int
{
    kNone = 0,
    kBlue,
    kRed,

    // -new values can be added above this line
    kNum
};

ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(Team);

// Tells use the shape of the zone
enum class ZoneEffectShape
{
    kNone = 0,

    kHexagon,
    kRectangle,
    kTriangle,

    // -new values can be added above this line
    kNum
};

// Combat affinity enum
// Matches https://illuvium.atlassian.net/wiki/spaces/GD/pages/20414574/Affinity
enum class CombatAffinity
{
    kNone = 0,

    //
    // Simple
    //

    kWater,
    kEarth,
    kFire,
    kNature,
    kAir,

    //
    // Combined
    //

    kTsunami,  // Water + Water
    kMud,      // Water + Earth
    kSteam,    // Water + Fire
    kToxic,    // Water + Nature
    kFrost,    // Water + Air

    kGranite,  // Earth + Earth
    kMagma,    // Earth + Fire
    kBloom,    // Earth + Nature
    kDust,     // Earth + Air

    kInferno,   // Fire + Fire
    kWildfire,  // Fire + Nature
    kShock,     // Fire + Air

    kVerdant,  // Nature + Nature
    kSpore,    // Nature + Air

    kTempest,  // Air + Air

    // -new values can be added above this line
    kNum
};

ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(CombatAffinity);

// Combat class enum
// Matches https://illuvium.atlassian.net/wiki/spaces/GD/pages/20513108/Class
enum class CombatClass
{
    kNone = 0,

    //
    // Simple
    //

    kFighter,
    kBulwark,
    kRogue,
    kPsion,
    kEmpath,

    //
    // Combined
    //

    kBerserker,  // Fighter + Fighter
    kBehemoth,   // Fighter + Bulwark
    kSlayer,     // Fighter + Rogue
    kArcanite,   // Fighter + Psion
    kTemplar,    // Fighter + Empath

    kColossus,   // Bulwark + Bulwark
    kVanguard,   // Bulwark + Rogue
    kHarbinger,  // Bulwark + Psion
    kAegis,      // Bulwark + Empath

    kPredator,  // Rogue + Rogue
    kPhantom,   // Rogue + Psion
    kRevenant,  // Rogue + Empath

    kInvoker,    // Psion + Psion
    kEnchanter,  // Psion + Empath

    kMystic,  // Empath + Empath

    // -new values can be added above this line
    kNum
};

ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(CombatClass);

// Used in the energy gain system to see what method we gain energy through
enum class EnergyGainType
{
    kNone,

    // Each Attack Ability increases the Energy
    kAttack,

    // When a Combat Unit takes damage, they gain Energy.
    kOnTakeDamage,

    kRegeneration,
    kOnActivation,
    kRefund,

    // -new values can be added above this line
    kNum
};

// Used in the health gain system to see what method we gain health through
enum class HealthGainType
{
    kNone = 0,

    kRegeneration,
    kPureVamp,
    kPhysicalVamp,
    kEnergyVamp,
    kInstantHeal,
    kInstantPureHeal,

    // -new values can be added above this line
    kNum
};

// The current state of entities that are combat units
enum class CombatUnitState
{
    kNone = 0,

    kAlive,
    kFainted,
    kDisappeared,

    // -new values can be added above this line
    kNum
};

// The type of ability, used by abilities
enum class AbilityType
{
    kNone = 0,

    // Use this value when checking for attack abilities
    kAttack,

    // Use this value when checking for Omega abilities
    kOmega,

    // Use this value when checking for Innate abilities
    kInnate,

    // -new values can be added above this line
    kNum
};

ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(AbilityType);

// The type of update action for an ability
enum class AbilityUpdateType
{
    // No action defined, ignore
    kNone = 0,

    // Add to the existing ability list
    kAdd,

    // Replace an existing ability with the same name
    kReplace,

    // -new values can be added above this line
    kNum
};

// Describes which types of entities skills can interact with.
enum class GuidanceType
{
    kNone = 0,

    // Ground - The Skill can interact with ground entities.
    kGround,

    // Airborne - The Skill can interact with airborne entities.
    kAirborne,

    // Underground - The Skill can interact with underground entities.
    kUnderground,

    // -new values can be added above this line
    kNum
};

ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(GuidanceType);

// Enum that describes from the source of this
enum class SourceContextType
{
    kNone = 0,

    // Originated from the AttachedEffectsSystem
    kAttachedEffect,

    // Originated from condition reaching max stacks
    kReachedMaxStacks,

    // Originated from the synergy
    kSynergy,

    // Originated from the augment
    kAugment,

    // Originated from the hyper system
    kHyper,

    // Originated from a shield entity
    kShield,

    // Originated from a mark entity
    kMark,

    // Originated from overload
    kOverload,

    // Originated from splash
    kSplash,

    // Originated from displacement
    kDisplacement,

    // Originated from consumable
    kConsumable,

    // From encounter mod
    kEncounterMod,

    // From suit
    kSuit,

    // From aura
    kAura,

    // Originated from the drone augment
    kDroneAugment,

    // -new values can be added above this line
    kNum
};

// Types of reserved positions
enum class ReservedPositionType : int
{
    kNone = 0,

    kAcross,
    kNearReceiver,
    kBehindReceiver,

    // -new values can be added above this line
    kNum
};

// Describes the skill targeting group, if any.
enum class AllegianceType
{
    kNone = 0,

    kSelf,
    kAlly,
    kEnemy,
    kAll,

    // Can add more above
    kNum
};

// Types of comparison
enum class ComparisonType
{
    // No Comparison
    kNone = 0,

    // >
    kGreater,

    // <
    kLess,

    // ==
    kEqual,

    // -new values can be added above this line
    kNum
};

bool EvaluateComparison(
    const ComparisonType comparison_type,
    const FixedPoint left_value,
    const FixedPoint right_value);

enum class DestructionReason
{
    kNone = 0,

    // Destroyed by damage
    kDamaged,

    // Dies naturally
    kExpired,

    kNum
};

enum class ActivationTriggerType
{
    // The empty activation trigger
    kNone = 0,

    // When the game starts
    kOnBattleStart,

    // Activates every x Time
    // Example: every 15 seconds
    kEveryXTime,

    // Effect Package received by combat unit
    // Possible sender and receiver is specified by sender_allegiance and receiver_allegiance
    kOnHit,

    // Shield damaged
    kOnShieldHit,

    // A critical Effect Package was received by another combat unit
    kOnDealCrit,

    // Effect Package was missed by the sender.
    // Allegiance controls whose events can activate an ability trigger
    kOnMiss,

    // Entity dodged an incoming attack
    // AbilityActivationTriggerData::allegiance determines which entity can trigger an ability by dodging an attack
    kOnDodge,

    // Damage happens.
    // Damage source and target constrains specified in sender_allegiance and receiver_allegiance
    kOnDamage,

    // You took down an enemy combat unit (aka combat unit fainted) and you are the vanquisher.
    kOnVanquish,

    // Someone fainted. Who should be killed is controlled by sender_allegiance
    kOnFaint,

    // You were part of the kill (you added damage, any detrimental effect)
    kOnAssist,

    // When health is the specified range
    // Example: When Nature Illuvials drop below 50% Health
    kHealthInRange,

    // When self activates a set number of a particular ability type
    kOnActivateXAbilities,

    // Triggers whenever self deploys a set multiple of a particular ability type
    kOnDeployXSkills,

    // Activates when two Illuvials in the range.
    // Example: If Poisoned enemy is within 20 hex range of Poison Illuvial, energy starts regen
    kInRange,

    // Activates whenever energy of self is equal to energy cost
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/250479333/OnEnergyFull
    kOnEnergyFull,

    // Activates when a Combat Unit receives a set number of effect packages from a particular ability type.
    // AbilityActivationTriggerData::ability_type - ability type to track
    // AbilityActivationTriggerData::number_of_effect_packages_received - a number of required effect packages.
    kOnReceiveXEffectPackages,

    // When unit becomes hyperactive
    kOnHyperactive,

    // -new versions can be added above this line
    kNum
};

ILLUVIUM_ENUM_AS_INDEX(ActivationTriggerType);

enum class AttachedEffectStateType
{
    // Pending activation, will occur in the next time step
    kPendingActivation = 0,

    // Currently active
    kActive,

    // This means another effect from the same source has a larger value
    kWaiting,

    // Actually destroyed
    kDestroyed,

    // -new values can be added above this line
    kNum
};

enum class AttachedEntityType : int
{
    kNone,
    kShield,
    kMark,
    kNum
};

// Tells us what the next action of the entity is
enum class DecisionNextAction
{
    kNone = 0,       // Do nothing
    kFindFocus,      // Find a focus
    kMovement,       // Perform movement towards or from the focus
    kAttackAbility,  // Perform attack ability
    kOmegaAbility,   // Perform omega ability

    // -new values can be added above this line
    kNum
};

// Log levels - numbers match spdlog
enum class LogLevel : int
{
    kTrace = 0,
    kDebug = 1,
    kInfo = 2,
    kWarn = 3,
    kErr = 4,
    kCritical = 5,

    // -new values can be added above this line
    kNum
};

enum class AugmentType : int
{
    kNone = 0,

    kNormal,
    kComponent,

    // Legendary augments

    // These are similar to a normal Augment except they have greater power. They should aim to fill a niche that
    // enhances the decision space.
    kGreaterPower,

    // These are the only Augment that can add an additional Combat Affinity or Combat Class to an Illuvial Combat Unit
    kSynergyBonus,

    // -new values can be added above this line
    kNum,
};

enum class SuitType : int
{
    kNone = 0,

    kNormal,
    kAmplifier,

    // -new values can be added above this line
    kNum,
};

enum class WeaponType : int
{
    kNone = 0,

    kNormal,
    kAmplifier,

    // -new values can be added above this line
    kNum,
};

enum class DroneAugmentType : int
{
    kNone = 0,

    // Affects only simulation, requires having list of abilities to apply
    kSimulation,

    // Affects game client, doesn't have any abilities for simulation
    kGame,

    // Affects server and game client, doesn't have any abilities for simulation
    kServerAndGame,

    // -new values can be added above this line
    kNum,
};

enum class DeathReasonType : int
{
    kNone = 0,
    kDamage,
    kExecution,
    kOverload,
    kNum
};

ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(DeathReasonType);

/* Describes some fixed position. They may depend on team or grid configuration
 * Can be used to spawn zone which moves from one side of board to another
 */
enum class PredefinedGridPosition
{
    kNone,
    kAllyBorderCenter,
    kEnemyBorderCenter,

    kNum  // Further entries must be above this one
};

}  // namespace simulation
