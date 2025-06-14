#pragma once

#include <memory>
#include <string>

#include "data/effect_data.h"
#include "data/effect_package.h"
#include "data/enums.h"
#include "utility/ensure_enum_size.h"
#include "utility/enum_set.h"
#include "utility/hex_grid_position.h"

namespace simulation
{
class CombatUnitData;

// How the skill is targeted
enum class SkillTargetingType
{
    kNone = 0,

    // Uses the current attack ability focus as the target
    kCurrentFocus,

    // Uses self as the target.
    kSelf,

    // Allies / Enemies within X - Targets either allies or enemies within a certain radius from the
    // caster.
    kInZone,

    // Finds the X closest / furthest allies / enemies and uses them as targets.
    kDistanceCheck,

    // Highest / Lowest Y Stat X Allies / Enemies - Finds X allies / enemies with the highest /
    // lowest Y stat.
    kCombatStatCheck,

    // Targets group based on allegiance
    kAllegiance,

    // Every Illuvial Combat Unit with a particular Illuvial Affinity or Illuvial Class will be targeted.
    kSynergy,

    // Targets the illuvial who made this illuvial faint
    // NOTE: This is used just for innates with Trigger Type OnUnitTakenDown
    kVanquisher,

    // Uses the target from a previous Skill in the Sub Ability.
    // NOTE: Should only be used for at least second skill in ability.
    kPreviousTargetList,

    // Activator Targeting is a Targeting.Type that specifies using the Activator List for targeting.
    // See https://illuvium.atlassian.net/wiki/spaces/AB/pages/256379211/Activator+List
    kActivator,

    // Targets illuvials based on an expression
    kExpressionCheck,

    // Uses the size of the deployment to determine where best to deploy in order to hit the most units
    kDensity,

    // Targets pets only. This targeting type determines allegiance using pet parent unit.
    // It means that if group was specified as "Self" then it will select only sender own pets.
    kPets,

    // Every Illuvial Combat Unit with a particular tier will be targeted.
    kTier,

    // -new values can be added above this line
    kNum
};

ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(SkillTargetingType);

// Describes how the skill gets to its target.
enum class SkillDeploymentType
{
    kNone = 0,

    // Direct - The Skill is applied directly to each target. It can still be blocked, or the target
    // can be immune, but the Skill will at least be applied. In this mode the target is the
    // instance of a combat unit, rather than a location.
    kDirect,

    // Zone - The target's location is defined as the center of a zone where the ability will be
    // deployed.
    //
    // In the case of multiple targets the center of those targets will be the center of
    // the zone. There will be three predefined shapes (Hexagon [radius required as input], Rectangle
    // [height and width required as input], and Triangle [radius and direction_degrees required as
    // input].
    // Note with Triangle and multiple targets the two targets at the widest angle will be used
    // and a bisecting angle would be used as the direction_degrees for the triangle to be deployed.
    // Triangles always start from the center of the deploying unit).
    //
    // Additionally, there will be a modifier to see if the zone endures. If it does endure then any
    // combat units inside the zone will have the Effect deployed to them periodically. If it does
    // not endure then the Skill will only be deployed to the combat units in the zone once. Note:
    // Validation still needs to be done for all combat units inside the zone.
    kZone,

    // Projectile - Projectiles are instantiated objects in the game that physically travel to the
    // target with a speed.
    //
    // There are modifiers to specify various properties:
    // - Pass Through - If pass through is enabled it will continue until it reaches the target, and
    // will not be blocked by other Combat Units that it collides with first. If pass through is
    // disabled then the projectile will be blocked by the first valid target on the way and apply
    // the effect to them instead of the target. An "Apply All" modifier can be set if this is used,
    // which would apply the projectile to any unit hit on the way through to its target.
    // - Homing - If enabled the projectile will keep track of the current location of the target
    // and move towards it. If disabled the projectile will only store the original location of the
    // target, and might miss if the target moves.
    kProjectile,

    // Beam - Beams are similar to common projectiles in that they are straight-line shots. The main
    // difference to projectiles is that they are not a discrete object in the game that moves
    // towards the target but rather an "area" from the origin to the target with some thickness.
    // There are modifiers to specify various properties:
    // - Pass Through - If pass through is enabled the beam will extend all the way to the target.
    // If disabled the beam will only extend as far as the first valid unit it collides with.
    // - Homing - If homing is enabled the beam's endpoint will move as the target moves. If
    // disabled the beam's endpoint will be fixed at the initial location of the target.
    kBeam,

    // Dash - A dash causes the unit deploying the effect package to move towards the target,
    // where they will apply the effect package either at the end of the journey, or along the way,
    // depending on parameters.
    // - Pass Through - If pass through is enabled the CombatUnit will continue on until it reaches
    // the target without being blocked along the way. If disabled the CombatUnit will stop at the
    // first valid target on the path.
    // - Homing - If homing is enabled the CombatUnit's endpoint will move as the target moves. If
    // disabled the CombatUnit's endpoint will be fixed at the initial location of the target.
    kDash,

    // A creature will be spawned at the target, such as an extra unit. There is a modifier when
    // spawning for "Linked" vs "Unlinked". If the spawned creature is linked, then Disruptions
    // that affect the Spawner also affected the Spawned creature.
    kSpawnedCombatUnit,

    // -new values can be added above this line
    kNum
};

/* Parameters used by SkillDeploymentType::kDash */
struct SkillDashData
{
    // An "Apply All" modifier can be set if this is used, which would apply the dash effect to any
    // unit hit on the way through to its target.
    bool apply_to_all = true;

    // Whether the dash ends behind the target
    bool land_behind = true;
};

/* Parameters used by SkillDeplymentType::kSpawnedCombatUnit */
struct SkillSpawnedCombatUnitData
{
    std::shared_ptr<const CombatUnitData> combat_unit = nullptr;
    HexGridCardinalDirection direction = HexGridCardinalDirection::kNone;
    bool linked = false;
    bool on_reserved_position = false;
};

/* Properties of SkillDeploymentType::kZone */
struct SkillZoneData
{
    // Shape of the zone if any
    ZoneEffectShape shape = ZoneEffectShape::kNone;

    // Radius of the zone in units
    int radius_units = 0;

    // Max radius of a growable zone in units
    int max_radius_units = 0;

    // Width of the zone in units
    int width_units = 0;

    // Height of the zone in units
    int height_units = 0;

    // Direction of the zone, if applicable, in degrees
    int direction_degrees = 0;

    // For how long the zone lasts in ms
    int duration_ms = 0;

    // How frequent is the zone in ms
    int frequency_ms = 0;

    // Movement speed of the zone in subunits per time step
    int movement_speed_sub_units_per_time_step = 0;

    // Growth rate of the zone in subunits per time step
    int growth_rate_sub_units_per_time_step = 0;

    // Handle collision for zone globally per id if not -1
    int global_collision_id = kInvalidGlobalCollisionID;

    // Whether zone should be deployed on predefined position
    PredefinedGridPosition predefined_spawn_position = PredefinedGridPosition::kNone;

    // Maybe this should be a new targeting type. For now it is MUCH simpler to implement this way
    PredefinedGridPosition predefined_target_position = PredefinedGridPosition::kNone;

    // Whether zone effect will only be applied once
    bool apply_once = false;

    // Whether zone should be attached to target entity
    // (to have the same position)
    bool attach_to_target = false;

    // Zone will be destroyed if sender faints
    bool destroy_with_sender = false;

    // Allows having abilities with custom activation pattern
    std::vector<int> skip_activations;
};

/* Properties of SkillDeploymentType::kProjectile */
struct SkillProjectileData
{
    // The size of the projectile
    int size_units = 0;

    // The projectile speed in subunits per time step
    int speed_sub_units = 0;

    // Keep track of the current location of the target
    bool is_homing = false;

    // If blockable is set to false it will continue until it reaches the target, and will not be
    // blocked by other Combat Units that it collides with first. If blockable is set to true then
    // the projectile will be blocked by the first valid target on the way and apply the effect to
    // them instead of the target.
    bool is_blockable = false;

    // An "Apply All" modifier can be set if this is used, which would apply the projectile to any
    // unit hit on the way through to its target. This only has an effect if blockable is set to
    // false.
    bool apply_to_all = false;

    // Determines if the projectile can proceed after hitting the target entity.
    // Incompatible with "is_homing"
    bool continue_after_target = false;
};

/* Properties of SkillDeploymentType::kBeam */
struct SkillBeamData
{
    // Width of the beam in units
    int width_units = 0;

    // How frequent is the beam in ms
    int frequency_ms = 0;

    // Whether beam effect will only be applied once
    bool apply_once = false;

    // Whether the beam is homing
    bool is_homing = false;

    // Whether the beam is blockable
    bool is_blockable = false;

    // Determines who can block the beam
    AllegianceType block_allegiance = AllegianceType::kNone;
};

/* -------------------------------------------------------------------------------------------------------
 * SkillTargetingData
 *
 * This structs holds the details of an CombatUnit Skill targeting.
 */
class SkillTargetingData
{
public:
    // The skill targeting type uses the stat_type value.
    static bool UsesTargetingStatType(const SkillTargetingType type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(SkillTargetingType, 15);
        return type == SkillTargetingType::kCombatStatCheck;
    }

    // The skill targeting type uses the allegiance_type value.
    static bool UsesTargetingGroup(const SkillTargetingType type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(SkillTargetingType, 15);
        return type == SkillTargetingType::kInZone || type == SkillTargetingType::kDistanceCheck ||
               type == SkillTargetingType::kCombatStatCheck || type == SkillTargetingType::kAllegiance ||
               type == SkillTargetingType::kSynergy || type == SkillTargetingType::kExpressionCheck ||
               type == SkillTargetingType::kDensity || type == SkillTargetingType::kPets ||
               type == SkillTargetingType::kTier;
    }

    // The skill targeting type uses the lowest value.
    static bool UsesTargetingLowest(const SkillTargetingType type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(SkillTargetingType, 15);
        return type == SkillTargetingType::kCombatStatCheck || type == SkillTargetingType::kExpressionCheck ||
               type == SkillTargetingType::kDensity;
    }

    // The skill targeting type uses the targeting value.
    static bool UsesDistanceCheck(const SkillTargetingType type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(SkillTargetingType, 15);
        return type == SkillTargetingType::kDistanceCheck;
    }

    // The skill targeting type uses the num value.
    static bool UsesTargetingNum(const SkillTargetingType type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(SkillTargetingType, 15);
        return type == SkillTargetingType::kCombatStatCheck || type == SkillTargetingType::kDistanceCheck ||
               type == SkillTargetingType::kExpressionCheck || type == SkillTargetingType::kDensity;
    }

    // The skill targeting type uses the num value. (but its optional)
    static bool UsesTargetingNumOptional(const SkillTargetingType type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(SkillTargetingType, 15);
        return type == SkillTargetingType::kTier;
    }

    // The skill targeting type uses the tier value.
    static bool UsesTargetingTier(const SkillTargetingType type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(SkillTargetingType, 15);
        return type == SkillTargetingType::kTier;
    }

    // The skill targeting type uses the num value.
    static bool UsesTargetingCombatSynergy(const SkillTargetingType type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(SkillTargetingType, 15);
        return type == SkillTargetingType::kSynergy;
    }

    // The skill targeting type uses the radius_units value.
    static bool UsesTargetingRadiusUnits(const SkillTargetingType type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(SkillTargetingType, 15);
        return type == SkillTargetingType::kInZone;
    }

    static constexpr bool IsDensityTargetingSupported(const SkillDeploymentType deployment_type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(SkillDeploymentType, 7);
        switch (deployment_type)
        {
        case SkillDeploymentType::kZone:
        case SkillDeploymentType::kBeam:
        case SkillDeploymentType::kDash:
        case SkillDeploymentType::kProjectile:
            return true;
        default:
            return false;
        }
    }

    static constexpr bool SupportsDeploymentType(
        const SkillTargetingType targeting_type,
        const SkillDeploymentType deployment_type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(SkillDeploymentType, 7);
        ILLUVIUM_ENSURE_ENUM_SIZE(SkillTargetingType, 15);
        if (targeting_type == SkillTargetingType::kDensity)
        {
            return IsDensityTargetingSupported(deployment_type);
        }

        return true;
    }

    static constexpr bool SupportsCurrentFocusers(const SkillTargetingType targeting_type)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(SkillTargetingType, 15);
        switch (targeting_type)
        {
        case SkillTargetingType::kInZone:
            return true;
        default:
            return false;
        }
    }

    // How do we find our target for this skill
    SkillTargetingType type = SkillTargetingType::kNone;

    // Whether this skill can target ground, airborne or both, defaults to Ground.
    EnumSet<GuidanceType> guidance = MakeEnumSet(GuidanceType::kGround);

    // What entities this skill is targeting, only used if targeting.type is
    // different than current or self.
    AllegianceType group = AllegianceType::kNone;

    // Targets specific stats if needed
    StatType stat_type = StatType::kNone;

    // How many targets for multi-target spells
    size_t num = 0;

    // Use when targeting within range
    int radius_units = 0;

    // Used for synergy targeting
    CombatSynergyBonus combat_synergy;

    // Used for synergy targeting
    CombatSynergyBonus not_combat_synergy;

    // EffectExpression for targeting
    EffectExpression expression{};

    // Determine whether the lowest stat/distance is being targeted or not
    bool lowest = false;

    // Determine whether or not the skill can target the caster
    bool self = false;

    // Will target only these combat unit who focuses sender
    // (or it's parent if sender is spawned entity)
    bool only_current_focusers = false;

    int tier = -1;
};

/* -------------------------------------------------------------------------------------------------------
 * SkillDeploymentData
 *
 * This structs holds the details of an CombatUnit skill deployment.
 */
class SkillDeploymentData
{
public:
    // How do we deploy this skill (is it instant, a projectile, etc)
    // Describes how the ability gets to its target.There are many ways to deploy a skill.
    SkillDeploymentType type = SkillDeploymentType::kNone;

    // Whether this skill can deploy to ground, airborne or both, defaults to Ground.
    EnumSet<GuidanceType> guidance = MakeEnumSet(GuidanceType::kGround);

    // How much time it takes from activation to deployment.
    // Calculated based on duration_of_ability_percentage.
    int pre_deployment_delay_percentage = 0;

    // How much time it is allowed to retarget during pre deployment delay.
    // Calculated based on pre-deployment delay duration.
    int pre_deployment_retargeting_percentage = 0;
};

/* -------------------------------------------------------------------------------------------------------
 * SkillData
 *
 * This struct holds the details of an CombatUnit Skill. A skill contains a type and a list of
 * attributes for that type. Skills are part of an ability.
 * --------------------------------------------------------------------------------------------------------
 */
struct SkillData
{
    // Create a new instance
    static std::shared_ptr<SkillData> Create()
    {
        return std::make_shared<SkillData>();
    }

    // Helper function to create a deep copy of this SkillData
    std::shared_ptr<SkillData> CreateDeepCopy() const;

    // Adds an effect with just a value
    EffectData& AddEffect(const EffectData& effect_data)
    {
        return effect_package.AddEffect(effect_data);
    }

    // Adds a damage effect with just a value
    EffectData& AddDamageEffect(const EffectDamageType damage_type, const EffectExpression& expression)
    {
        return effect_package.AddDamageEffect(damage_type, expression);
    }

    // Will this skill reserve a position on the board
    bool ReservesAPosition() const
    {
        // Effects that needs an open position
        for (const auto& effect : effect_package.effects)
        {
            switch (effect.type_id.type)
            {
            case EffectType::kBlink:
                return true;
            default:
                break;
            }
        }

        // Also for dash deployment
        if (deployment.type == SkillDeploymentType::kDash)
        {
            return true;
        }

        return false;
    }

    // Is the channel_time_ms optional?
    static bool IsChannelTimeMsOptional(const SkillDeploymentType deployment_type)
    {
        return deployment_type != SkillDeploymentType::kBeam;
    }
    bool IsChannelTimeMsOptional() const
    {
        return IsChannelTimeMsOptional(deployment.type);
    }

    void SetDefaults(AbilityType ability_type)
    {
        if (ability_type == AbilityType::kAttack)
        {
            // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44368532/Hit+Chance
            effect_package.attributes.use_hit_chance = true;
        }
    }

    // Set splash defaults
    void SetPropagationSkillSplashDefaults()
    {
        deployment.type = SkillDeploymentType::kZone;
        targeting.type = SkillTargetingType::kSelf;
        zone.shape = ZoneEffectShape::kHexagon;
        zone.duration_ms = 0;
        zone.frequency_ms = 100;
        zone.apply_once = true;
    }

    // Human readable name for the skill
    std::string name;

    SkillSpawnedCombatUnitData spawn{};
    SkillProjectileData projectile;
    SkillZoneData zone{};
    SkillBeamData beam{};
    SkillDashData dash{};
    SkillTargetingData targeting{};
    SkillDeploymentData deployment{};
    EffectPackage effect_package{};

    //
    // Timers
    //

    // How long does this skill last, this is a percentage of the AbilityData::total_duration_ms
    // For attack abilities this is set to 100 as only one skill can exist in one attack ability
    // ability.
    // This is the duration of the Skill divided by the Ability duration.
    // Example:
    // Ability total_duration_ms = 5000 ms.
    // Skill 1 percentage_of_ability_duration = 50% (2500 ms).
    // Skill 1 pre_deployment_delay_percentage = 25% => delay time is 625 ms before deployment.
    int percentage_of_ability_duration = kMaxPercentage;

    // For how long the skill will channel
    int channel_time_ms = 0;

    // Whether the skill is a critical
    // NOTE: Only set at runtime
    bool is_critical = false;

    //
    // Direct
    //
    bool spread_effect_package = false;
};

}  // namespace simulation
