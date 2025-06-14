#pragma once

#include <list>

#include "data/constants.h"
#include "data/effect_data.h"
#include "utility/ensure_enum_size.h"
#include "utility/grid_helper.h"
#include "utility/hex_grid_position.h"
#include "utility/ivector2d.h"

namespace simulation
{
namespace event_data
{
struct Created
{
    EntityID entity_id = kInvalidEntityID;
};

struct Marked
{
    // Entity id which is marked for this event action
    EntityID entity_id = kInvalidEntityID;
};

struct Moved
{
    EntityID entity_id = kInvalidEntityID;
    HexGridPosition position{};
    std::list<HexGridPosition> waypoints{};
};

struct Blinked
{
    EntityID entity_id = kInvalidEntityID;
    HexGridPosition position{};
    EntityID predicted_focus_id = kInvalidEntityID;
};

struct StoppedMovement
{
    EntityID entity_id = kInvalidEntityID;
};

struct FindPath
{
    EntityID entity_id = kInvalidEntityID;
    HexGridPosition position{};
};

struct Deactivated
{
    // The entity which has been deactivated
    EntityID entity_id = kInvalidEntityID;
};

struct Fainted
{
    // Source of this faint
    SourceContextData source_context;

    // The entity which has been fainted/vanquished
    EntityID entity_id = kInvalidEntityID;

    // Reason of death
    DeathReasonType death_reason = DeathReasonType::kNone;

    // Which entity caused the entity_id to faint
    EntityID vanquisher_id = kInvalidEntityID;
};

struct AbilityActivated
{
    // Source of this ability
    SourceContextData source_context;

    // The entity who sent this ability
    EntityID sender_id = kInvalidEntityID;

    // The original combat unit who sent this ability
    // Can match or not the sender_id
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // The activated ability name
    std::string ability_name;

    // Type of the ability being used
    AbilityType ability_type = AbilityType::kNone;

    // The index of this ability
    size_t ability_index = kInvalidIndex;

    // The attack speed percentage of the sender_id
    // Only set if this is an activation of a attack ability
    FixedPoint attack_speed = 0_fp;

    // Currently works only for attack abilities
    int previous_overflow_ms = 0;

    // Total duration of the ability in ms
    // NOTE: Only useful for abilities besides Attack ability
    int total_duration_ms = 0;

    // Is this an ability that contains some skills that are critical
    bool is_critical = false;

    // Activation event for innate ability
    // NOTE: Only used for innate abilities
    ActivationTriggerType trigger_type = ActivationTriggerType::kNone;

    // Predicted Ids receivers of this ability
    // NOTE: This can be different than the receiver_ids when a skill deploys
    std::vector<EntityID> predicted_receiver_ids{};
};

struct AbilityDeactivated
{
    // Source of this ability
    SourceContextData source_context;

    // The entity who sent this ability
    EntityID sender_id = kInvalidEntityID;

    // The original combat unit who sent this ability
    // Can match or not the sender_id
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // The deactivated ability name
    std::string ability_name;

    // Type of the ability being used
    AbilityType ability_type = AbilityType::kNone;

    // The index of this ability
    size_t ability_index = kInvalidIndex;

    // Does this deactivate ability has a skill with movement? Most likely dash
    bool has_skill_with_movement = false;

    // Total duration of the ability in ms
    int total_duration_ms = 0;

    // Activation event for innate ability
    // NOTE: Only used for innate abilities
    ActivationTriggerType trigger_type = ActivationTriggerType::kNone;

    // Is this an ability that contains some skills that are critical
    bool is_critical = false;
};

struct AbilityInterrupted
{
    EntityID sender_id = kInvalidEntityID;
    AbilityType ability_type = AbilityType::kNone;
    size_t ability_index = kInvalidIndex;
};

// Used for the skill types
struct Skill
{
    // The entity who sent this skill
    EntityID sender_id = kInvalidEntityID;

    // Type of the ability being used
    AbilityType ability_type = AbilityType::kNone;

    // The index of this ability
    size_t ability_index = kInvalidIndex;

    // The skill index inside the ability
    size_t skill_index = kInvalidIndex;
};

// Used to know what targets did a skill
struct SkillTargets
{
    // The entity who sent this skill
    EntityID sender_id = kInvalidEntityID;

    // Type of the ability being used
    AbilityType ability_type = AbilityType::kNone;

    // The index of this ability
    size_t ability_index = kInvalidIndex;

    // The skill index inside state.skills
    size_t skill_index = kInvalidIndex;

    // Optional argument that indicates to the visualization that it should rotate to the target
    bool rotate_to_target = false;

    // Ids of the receivers
    std::vector<EntityID> receiver_ids;

    // Location of the deployment
    std::vector<HexGridPosition> deployment_locations;
};

struct EffectPackage
{
    // The entity that send this effect package
    EntityID sender_id = kInvalidEntityID;

    // The original combat unit who sent this effect package
    // Can match or not the sender_id
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // The target of the attack
    EntityID receiver_id = kInvalidEntityID;

    // Type of the ability being used
    AbilityType ability_type = AbilityType::kNone;

    // Is this a critical attack
    bool is_critical = false;

    // The attributes of the effect package
    EffectPackageAttributes attributes;

    // Source of this effect package
    SourceContextData source_context{};

    // Receive location - for the case when package sent to location
    // (receiver_id will be kInvalidEntityID)
    HexGridPosition receiver_position = kInvalidHexHexGridPosition;
};

struct StatChanged
{
    // Which entity caused this stat to change
    EntityID caused_by_id = kInvalidEntityID;

    // For which entity did this stat change
    EntityID entity_id = kInvalidEntityID;

    // Delta for the stat that changed
    FixedPoint delta = 0_fp;
};

struct HealthGain
{
    // The source of this heal
    SourceContextData source_context;

    // Which entity caused this stat to change
    EntityID caused_by_id = kInvalidEntityID;

    // Which combat unit caused this stat to change
    // Can match or not the caused_by_id
    EntityID caused_by_combat_unit_id = kInvalidEntityID;

    // For which entity did the health change
    EntityID entity_id = kInvalidEntityID;

    // How much health was gained
    FixedPoint gain = 0_fp;

    // Current health of entity_id
    FixedPoint current_health = 0_fp;

    // Method of health gain
    HealthGainType health_gain_type = HealthGainType::kNone;
};

struct AppliedDamage
{
    // The source of this damage
    SourceContextData source_context;

    // The original combat unit who sent this effect package
    // Can match or not the sender_id
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // The entity that initiated the attack
    EntityID sender_id = kInvalidEntityID;

    // The target of the attack
    EntityID receiver_id = kInvalidEntityID;

    // Damage Value
    FixedPoint damage_value = 0_fp;

    // Is this a critical attack
    bool is_critical = false;

    // Damage type applied
    EffectDamageType damage_type = EffectDamageType::kNone;
};

struct CombatUnitCreated
{
    // Entity id of the combat unit
    EntityID entity_id = kInvalidEntityID;

    // Current spawn position
    HexGridPosition position{};

    // Type of the combat unit
    CombatUnitType unit_type = CombatUnitType::kNone;

    // Entity id who spawned this pet
    EntityID parent_id = kInvalidEntityID;
};

struct ChainCreated
{
    // The source of this chain
    SourceContextData source_context;

    // Entity id of the chain
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this chain
    // NOTE: this can be a non combat unit (Zone/Projectile/etc), use the combat_unit_sender_id
    EntityID sender_id = kInvalidEntityID;

    // The combat unit id which spawned the original chain
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // The id of the receiver entity chain
    EntityID receiver_id = kInvalidEntityID;

    // Chain source position
    HexGridPosition chain_position{};

    // Bounce position near chain receiver
    HexGridPosition chain_bounce_position{};

    // The position of the receiver_id
    HexGridPosition receiver_position{};

    // Delay for this chain instance
    int delay_ms = 0;

    // Whether the chain is a crit
    bool is_critical = false;
};

struct ChainDestroyed
{
    // Entity id of the chain
    // NOTE: do no try to access the entity data from the next time step
    EntityID entity_id = kInvalidEntityID;

    // Last known position before being destroyed
    HexGridPosition position{};

    // Is this the last chain being destroyed
    bool is_last_chain = false;
};

struct SplashCreated
{
    // The source of this splash
    SourceContextData source_context;

    // Entity id of the splash
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this chain
    // NOTE: this can be a non combat unit (Zone/Projectile/etc), use the combat_unit_sender_id
    EntityID sender_id = kInvalidEntityID;

    // The original combat unit who spawned this splash
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // Position the splash entity is created at
    HexGridPosition position{};

    // Whether the splash is a crit
    bool is_critical = false;
};

struct SplashDestroyed
{
    // Entity id of the splash
    // NOTE: do no try to access the entity data from the next time step
    EntityID entity_id = kInvalidEntityID;

    // Last known position before being destroyed
    HexGridPosition position{};
};

struct ProjectileCreated
{
    // The source of this projectile
    SourceContextData source_context;

    // Entity id of the projectile
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this projectile
    // NOTE: this can be a non combat unit (Zone/Projectile/etc), use the combat_unit_sender_id
    EntityID sender_id = kInvalidEntityID;

    // The original combat unit who spawned this projectile
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // Receiver entity of the projectile
    EntityID receiver_id = kInvalidEntityID;

    // If blockable is set to false it will continue until it reaches the target, and will not be
    // blocked by other combat units that it collides with first. If blockable is set to true then
    // the projectile will be blocked by the first valid target on the way and apply the effect to
    // them instead of the target. Attacks should usually be coded with blockable set to
    // false. An "Apply All" modifier can be set if this is false, which would apply the projectile
    // to any unit hit on the way through to its target. NOTE: is_blockable == !is_passthrough (old
    // teminology)
    bool is_blockable = false;

    // An "Apply All" modifier can be set if this is used, which would apply the projectile to any
    // unit hit on the way through to its target. This only has an effect if blockable is set to
    // false.
    bool apply_to_all = false;

    // If enabled the projectile will keep track of the current location of the target and move
    // towards it. If disabled the projectile will only store the original location of the target,
    // and might miss if the target moves.
    bool is_homing = false;

    // Whether the projectile is a crit
    bool is_critical = false;

    // Current spawn position
    HexGridPosition position{};

    // The position of the receiver_id.
    // - is_homing = true - this just represents the position that the receiver_id was at projectile
    // creation. To get the true position you must query the receiver_id position.
    // - is_homing = false - this represents the position the projectile will target.
    HexGridPosition receiver_position{};

    // Movement speed in sub units per time step
    int move_speed_sub_units = 0;
};

struct ProjectileDestroyed
{
    // Entity id of the projectile
    // NOTE: do no try to access the entity data from the next time step
    EntityID entity_id = kInvalidEntityID;

    // Last known position before being destroyed
    HexGridPosition position{};
};

struct ProjectileBlocked
{
    // Entity id of the projectile
    EntityID projectile_entity_id = kInvalidEntityID;

    // Entity id that blocked the projectile
    EntityID blocked_by_entity_id = kInvalidEntityID;
};

struct ZoneCreated
{
    // The source of this zone
    SourceContextData source_context;

    // Entity id of the zone
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this zone
    // NOTE: this can be a non combat unit (Zone/Projectile/etc), use the combat_unit_sender_id
    EntityID sender_id = kInvalidEntityID;

    // The original combat unit who spawned this zone
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // Current spawn position
    HexGridPosition position{};

    // Zone effect shape
    ZoneEffectShape shape = ZoneEffectShape::kNone;

    // Radius of the zone, in subunits - used if shape is not rectangle
    int radius_sub_units = 0;

    // Max radius of a growable zone in subunits
    int max_radius_sub_units = 0;

    // Width of the zone, in subunits - used if shape is rectangle
    int width_sub_units = 0;

    // Height of the zone, in subunits - used if shape is rectangle
    int height_sub_units = 0;

    // Direction the zone is spawned, if applicable, in degrees
    int spawn_direction_degrees = 0;

    // Direction the zone is facing, if applicable, in degrees
    int direction_degrees = 0;

    // How long this zone lasts
    int duration_ms = 0;

    // Whether the zone is a crit
    bool is_critical = false;
};

struct DashCreated
{
    // The source of this dash
    SourceContextData source_context;

    // Entity id of the dash
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this dash
    // NOTE: this can be a non combat unit (Zone/Projectile/etc), use the combat_unit_sender_id
    EntityID sender_id = kInvalidEntityID;

    // The original combat unit who spawned this zone
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // Receiver entity of the dash
    EntityID receiver_id = kInvalidEntityID;

    // The dash target position
    HexGridPosition target_position;
};

struct DashDestroyed
{
    // Entity id of the dash
    // NOTE: do no try to access the entity data from the next time step
    EntityID entity_id = kInvalidEntityID;

    // Dash sender
    EntityID sender_id = kInvalidEntityID;

    // The last known position of dash entity
    HexGridPosition last_position = kInvalidHexHexGridPosition;
};

struct ApplyHealthGain
{
    // The source of this heal
    SourceContextData source_context;

    // Which entity caused the health to change
    EntityID caused_by_id = kInvalidEntityID;

    // Entity ID of the receiver entity
    EntityID receiver_id = kInvalidEntityID;

    // Method of health gain
    HealthGainType health_gain_type = HealthGainType::kNone;

    // Value to be passed to the system
    FixedPoint value = 0_fp;

    // Percentage of post_mitigated_damage to be sent back to the sender
    FixedPoint vampiric_percentage = 0_fp;

    // Bool when the excess vamp should go to shield
    bool excess_vamp_to_shield = false;

    // Duration of excess_to_vamp shield
    int excess_vamp_to_shield_duration_ms = 0;

    // Integer value above 0 representing the max shield value to be gained from the health being,
    // In the case where the Amount - MissingHealth > 0
    FixedPoint excess_heal_to_shield = 0_fp;

    // Integer value between 0 - 100 representing the percentage of missing health that gets
    // added
    // to the heal amount
    FixedPoint missing_health_percentage_to_health = 0_fp;

    // Integer value between 0 - 100 representing the percentage of max health that gets added to
    // the Amount
    FixedPoint max_health_percentage_to_health = 0_fp;
};

struct ApplyEnergyGain
{
    // Which entity caused this stat to change
    EntityID caused_by_id = kInvalidEntityID;

    // Entity ID of the receiver entity
    EntityID receiver_id = kInvalidEntityID;

    // The type of the energy gain
    EnergyGainType energy_gain_type = EnergyGainType::kNone;

    // How much energy
    FixedPoint value = 0_fp;
};

struct ApplyShieldGain
{
    // Entity ID of the receiver entity
    EntityID receiver_id = kInvalidEntityID;

    // Post mitigated damage to be passed to the system
    FixedPoint gain_amount = 0_fp;

    // For how long
    int duration_time_ms = 0;
};

struct ZoneDestroyed
{
    // Entity id of the zone
    // NOTE: do no try to access the entity data from the next time step
    EntityID entity_id = kInvalidEntityID;

    // Last known position before being destroyed
    HexGridPosition position{};
};

struct ZoneActivated
{
    // Entity id of the zone
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this zone
    EntityID sender_id = kInvalidEntityID;

    // Current spawn position
    HexGridPosition position{};
};

struct BeamCreated
{
    // Entity id of the beam
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this beam
    // NOTE: this can be a non combat unit (Zone/Projectile/etc), use the combat_unit_sender_id
    EntityID sender_id = kInvalidEntityID;

    // The original combat unit who spawned this zone
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // Receiver of this beam
    EntityID receiver_id = kInvalidEntityID;

    // Beam direction in degrees (angle from sender to receiver)
    // Going counter-clockwise from where x > 0 and y = 0
    // This is the arctangent of the difference between the positions
    int beam_direction_degrees = 0;

    // Beam length in subunits
    int beam_world_length_sub_units = 0;

    // Beam width in subunits
    int beam_width_sub_units = 0;

    // Max total time this beam can stay alive
    // NOTE: That a beam can be interrupted and can live less than this total
    int max_total_time_ms = 0;

    // Current spawn position
    HexGridPosition position{};

    // Where does this beam come from
    SourceContextData source_context{};

    // Whether the beam is a crit
    bool is_critical = false;
};

struct BeamDestroyed
{
    // Entity id of the beam
    // NOTE: do no try to access the entity data from the next time step
    EntityID entity_id = kInvalidEntityID;

    // Last known position before being destroyed
    HexGridPosition position{};
};

struct BeamUpdated
{
    // Entity id of the beam
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this beam
    EntityID sender_id = kInvalidEntityID;

    // Receiver of this beam
    EntityID receiver_id = kInvalidEntityID;

    // Beam direction in degrees (angle from sender to receiver)
    // Going counter-clockwise from where x > 0 and y = 0
    // This is the arctangent of the difference between the positions
    int beam_direction_degrees = 0;

    // Beam length in subunits
    int beam_world_length_sub_units = 0;

    // Beam width in subunits
    int beam_width_sub_units = 0;

    // Current spawn position
    IVector2D world_position{};
};

struct BeamActivated
{
    // Entity id of the beam
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this beam
    EntityID sender_id = kInvalidEntityID;

    // Receiver of this beam
    EntityID receiver_id = kInvalidEntityID;

    // Beam direction in degrees (angle from sender to receiver)
    // Going counter-clockwise from where x > 0 and y = 0
    // This is the arctangent of the difference between the positions
    int beam_direction_degrees = 0;

    // Beam length in subunits
    int beam_world_length_sub_units = 0;

    // Beam width in subunits
    int beam_width_sub_units = 0;

    // Current spawn position
    IVector2D world_position{};
};

struct ShieldCreated
{
    // Entity id of the shield
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this shield
    EntityID sender_id = kInvalidEntityID;

    // The original combat unit who spawned this shield
    // Can match or not the sender_id
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // The id of the entity that is the target of this shield
    EntityID receiver_id = kInvalidEntityID;

    // Context of the shield being created
    SourceContextData source_context;

    // Shield value
    FixedPoint value = 0_fp;
};

struct Shield
{
    // Entity id of the shield
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this shield
    EntityID sender_id = kInvalidEntityID;

    // The id of the entity that is the target of this shield
    EntityID receiver_id = kInvalidEntityID;
};

struct ShieldWasHit
{
    // Entity id of the shield
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this shield
    EntityID sender_id = kInvalidEntityID;

    // The id of the entity that is the target of this shield
    EntityID receiver_id = kInvalidEntityID;

    // Who hit the shield
    EntityID hit_by = kInvalidEntityID;
};

struct ShieldDestroyed
{
    // Entity id of the shield
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this shield
    EntityID sender_id = kInvalidEntityID;

    // The id of the entity that is the target of this shield
    EntityID receiver_id = kInvalidEntityID;

    // Destruction reason
    DestructionReason destruction_reason = DestructionReason::kNone;
};

struct MarkCreated
{
    // Entity id of the mark
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this mark
    EntityID sender_id = kInvalidEntityID;

    // The original combat unit who spawned this mark
    // Can match or not the sender_id
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // The id of the entity that is the target of this mark
    EntityID receiver_id = kInvalidEntityID;

    // The source of this mark
    SourceContextData source_context;
};

struct MarkDestroyed
{
    // Entity id of the mark
    EntityID entity_id = kInvalidEntityID;

    // The id of the entity that spawned this mark
    EntityID sender_id = kInvalidEntityID;

    // The id of the entity that is the target of this mark
    EntityID receiver_id = kInvalidEntityID;

    // Destruction reason
    DestructionReason destruction_reason = DestructionReason::kNone;
};

struct Displacement
{
    // The id of the entity that caused the displacement
    EntityID sender_id = kInvalidEntityID;

    // The original combat unit who sent this displacement
    // Can match or not the sender_id
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // The id of the entity that is the target of this displacement
    EntityID receiver_id = kInvalidEntityID;

    // Type of displacement
    EffectDisplacementType displacement_type = EffectDisplacementType::kNone;

    // Duration of displacement
    int duration_time_ms = 0;

    // Distance of the displacement, if applicable
    int distance_sub_units = 0;
};

struct BattleStarted
{
    // TODO(vampy): Fill this with some info
};

struct OverloadStarted
{
    // Seconds since overload started
    int second_since_start = 0;
};

struct BattleFinished
{
    // TODO(vampy): Add fields for the final state of the battle

    // The team that won the game
    Team winning_team = Team::kNone;
};

struct TimeStepped
{
    // Number of new finished time step
    int step_number = 0;
};

struct Effect
{
    // Sender entity id of this effect
    EntityID sender_id = kInvalidEntityID;

    // Receiver entity id of this effect
    EntityID receiver_id = kInvalidEntityID;

    // The effect constant data
    EffectData data{};

    // The effect state data
    EffectState state{};

    // For the cases when applied to location
    HexGridPosition receiver_position = kInvalidHexHexGridPosition;
};

struct ActivateAnyAbility
{
    // To which entity should we try to activate this ability
    EntityID entity_id = kInvalidEntityID;

    // Can we activate an attack ability?
    bool can_do_attack = false;

    // Can we activate an omega ability?
    bool can_do_omega = false;
};

struct Focus
{
    // To which entity the focus has something happened to it
    EntityID entity_id = kInvalidEntityID;

    // Was there a target for this focus event?
    EntityID receiver_id = kInvalidEntityID;
};

struct CombatUnitStateChanged
{
    // Which entity's state has changed
    EntityID entity_id = kInvalidEntityID;

    // New state
    CombatUnitState state = CombatUnitState::kNone;
};

struct Hyperactive
{
    // To entity which has gone hyperactive
    EntityID entity_id = kInvalidEntityID;
};

struct PathfindingDebugData
{
    // Start node index of pathfinding request
    size_t start_node = kInvalidIndex;

    // Target node index of pathfinding request
    size_t target_node = kInvalidIndex;

    // Reached node index of pathfinding request
    size_t reached_node = kInvalidIndex;

    // Radius of source entity
    int source_radius = 0;

    // A* graph result with visited nodes
    AStarGraph path_graph;

    // Obstacles map for this pathfinding request
    ObstaclesMapType obstacles;
};

struct ZoneRadiusChanged
{
    // Which zone's radius has changed
    EntityID entity_id = kInvalidEntityID;

    // Previous zone radius in sub units
    int old_radius_sub_units = 0;

    // Current/new zone radius in sub units
    int new_radius_sub_units = 0;
};

struct AuraCreated
{
    EntityID entity_id = kInvalidEntityID;
};

struct AuraDestroyed
{
    EntityID entity_id = kInvalidEntityID;
};

}  // namespace event_data

namespace event_impl
{
// Implementation of mapping from event type enumeration to actual data type

template <EventType event_type>
struct EventTypeToDataTypeImpl
{
    static constexpr bool mapped = false;
};

#ifdef MAP_EVENT_TYPE_
static_assert(false, "Macro name collission or double include");
#endif

#define MAP_EVENT_TYPE_(event_type, data_type) \
    template <>                                \
    struct EventTypeToDataTypeImpl<event_type> \
    {                                          \
        using Type = data_type;                \
        static constexpr bool mapped = true;   \
    }

MAP_EVENT_TYPE_(EventType::kNone, void);
MAP_EVENT_TYPE_(EventType::kCreated, event_data::Created);
MAP_EVENT_TYPE_(EventType::kMoved, event_data::Moved);
MAP_EVENT_TYPE_(EventType::kMovedStep, event_data::Moved);
MAP_EVENT_TYPE_(EventType::kBlinked, event_data::Blinked);
MAP_EVENT_TYPE_(EventType::kStoppedMovement, event_data::StoppedMovement);
MAP_EVENT_TYPE_(EventType::kFindPath, event_data::FindPath);
MAP_EVENT_TYPE_(EventType::kDeactivated, event_data::Deactivated);
MAP_EVENT_TYPE_(EventType::kFainted, event_data::Fainted);
MAP_EVENT_TYPE_(EventType::kAbilityActivated, event_data::AbilityActivated);
MAP_EVENT_TYPE_(EventType::kAbilityDeactivated, event_data::AbilityDeactivated);
MAP_EVENT_TYPE_(EventType::kAfterAbilityDeactivated, event_data::AbilityDeactivated);
MAP_EVENT_TYPE_(EventType::kAbilityInterrupted, event_data::AbilityInterrupted);
MAP_EVENT_TYPE_(EventType::kSkillWaiting, event_data::Skill);
MAP_EVENT_TYPE_(EventType::kSkillDeploying, event_data::Skill);
MAP_EVENT_TYPE_(EventType::kSkillChanneling, event_data::Skill);
MAP_EVENT_TYPE_(EventType::kSkillFinished, event_data::Skill);
MAP_EVENT_TYPE_(EventType::kSkillNoTargets, event_data::Skill);
MAP_EVENT_TYPE_(EventType::kSkillTargetsUpdated, event_data::SkillTargets);
MAP_EVENT_TYPE_(EventType::kEffectPackageReceived, event_data::EffectPackage);
MAP_EVENT_TYPE_(EventType::kEffectPackageMissed, event_data::EffectPackage);
MAP_EVENT_TYPE_(EventType::kEffectPackageDodged, event_data::EffectPackage);
MAP_EVENT_TYPE_(EventType::kEffectPackageBlocked, event_data::EffectPackage);
MAP_EVENT_TYPE_(EventType::kOnDamage, event_data::AppliedDamage);
MAP_EVENT_TYPE_(EventType::kHealthChanged, event_data::StatChanged);
MAP_EVENT_TYPE_(EventType::kEnergyChanged, event_data::StatChanged);
MAP_EVENT_TYPE_(EventType::kHyperChanged, event_data::StatChanged);
MAP_EVENT_TYPE_(EventType::kOnHealthGain, event_data::HealthGain);
MAP_EVENT_TYPE_(EventType::kApplyHealthGain, event_data::ApplyHealthGain);
MAP_EVENT_TYPE_(EventType::kApplyShieldGain, event_data::ApplyShieldGain);
MAP_EVENT_TYPE_(EventType::kApplyEnergyGain, event_data::ApplyEnergyGain);
MAP_EVENT_TYPE_(EventType::kOnEffectApplied, event_data::Effect);
MAP_EVENT_TYPE_(EventType::kTryToApplyEffect, event_data::Effect);
MAP_EVENT_TYPE_(EventType::kOnAttachedEffectApplied, event_data::Effect);
MAP_EVENT_TYPE_(EventType::kOnAttachedEffectRemoved, event_data::Effect);
MAP_EVENT_TYPE_(EventType::kActivateAnyAbility, event_data::ActivateAnyAbility);
MAP_EVENT_TYPE_(EventType::kCombatUnitCreated, event_data::CombatUnitCreated);
MAP_EVENT_TYPE_(EventType::kChainCreated, event_data::ChainCreated);
// TODO(konstantin): implement chain bounced event or delete it from enum
MAP_EVENT_TYPE_(EventType::kChainBounced, void);
MAP_EVENT_TYPE_(EventType::kChainDestroyed, event_data::ChainDestroyed);
MAP_EVENT_TYPE_(EventType::kSplashCreated, event_data::SplashCreated);
MAP_EVENT_TYPE_(EventType::kSplashDestroyed, event_data::SplashDestroyed);
MAP_EVENT_TYPE_(EventType::kProjectileCreated, event_data::ProjectileCreated);
MAP_EVENT_TYPE_(EventType::kProjectileBlocked, event_data::ProjectileBlocked);
MAP_EVENT_TYPE_(EventType::kProjectileDestroyed, event_data::ProjectileDestroyed);
MAP_EVENT_TYPE_(EventType::kZoneCreated, event_data::ZoneCreated);
MAP_EVENT_TYPE_(EventType::kZoneDestroyed, event_data::ZoneDestroyed);
MAP_EVENT_TYPE_(EventType::kZoneActivated, event_data::ZoneActivated);
MAP_EVENT_TYPE_(EventType::kBeamCreated, event_data::BeamCreated);
MAP_EVENT_TYPE_(EventType::kBeamDestroyed, event_data::BeamDestroyed);
MAP_EVENT_TYPE_(EventType::kBeamUpdated, event_data::BeamUpdated);
MAP_EVENT_TYPE_(EventType::kBeamActivated, event_data::BeamActivated);
MAP_EVENT_TYPE_(EventType::kDashCreated, event_data::DashCreated);
MAP_EVENT_TYPE_(EventType::kDashDestroyed, event_data::DashDestroyed);
MAP_EVENT_TYPE_(EventType::kShieldCreated, event_data::ShieldCreated);
MAP_EVENT_TYPE_(EventType::kShieldDestroyed, event_data::ShieldDestroyed);
MAP_EVENT_TYPE_(EventType::kShieldWasHit, event_data::ShieldWasHit);
MAP_EVENT_TYPE_(EventType::kShieldExpired, event_data::Shield);
MAP_EVENT_TYPE_(EventType::kShieldPendingDestroyed, event_data::Shield);
MAP_EVENT_TYPE_(EventType::kDisplacementStarted, event_data::Displacement);
MAP_EVENT_TYPE_(EventType::kDisplacementStopped, event_data::Displacement);
MAP_EVENT_TYPE_(EventType::kMarkCreated, event_data::MarkCreated);
MAP_EVENT_TYPE_(EventType::kMarkDestroyed, event_data::MarkDestroyed);
MAP_EVENT_TYPE_(EventType::kMarkChainAsDestroyed, event_data::Marked);
MAP_EVENT_TYPE_(EventType::kMarkSplashAsDestroyed, event_data::Marked);
MAP_EVENT_TYPE_(EventType::kMarkProjectileAsDestroyed, event_data::Marked);
MAP_EVENT_TYPE_(EventType::kMarkZoneAsDestroyed, event_data::Marked);
MAP_EVENT_TYPE_(EventType::kMarkBeamAsDestroyed, event_data::Marked);
MAP_EVENT_TYPE_(EventType::kMarkDashAsDestroyed, event_data::Marked);
MAP_EVENT_TYPE_(EventType::kMarkAuraAsDestroyed, event_data::Marked);
MAP_EVENT_TYPE_(EventType::kMarkAttachedEntityAsDestroyed, event_data::Marked);
MAP_EVENT_TYPE_(EventType::kFocusNeverDeactivated, event_data::Focus);
MAP_EVENT_TYPE_(EventType::kFocusFound, event_data::Focus);
MAP_EVENT_TYPE_(EventType::kFocusUnreachable, event_data::Focus);
MAP_EVENT_TYPE_(EventType::kCombatUnitStateChanged, event_data::CombatUnitStateChanged);
MAP_EVENT_TYPE_(EventType::kBattleStarted, event_data::BattleStarted);
MAP_EVENT_TYPE_(EventType::kBattleFinished, event_data::BattleFinished);
MAP_EVENT_TYPE_(EventType::kGoneHyperactive, event_data::Hyperactive);
MAP_EVENT_TYPE_(EventType::kOverloadStarted, event_data::OverloadStarted);
MAP_EVENT_TYPE_(EventType::kTimeStepped, event_data::TimeStepped);
MAP_EVENT_TYPE_(EventType::kPathfindingDebugData, event_data::PathfindingDebugData);
MAP_EVENT_TYPE_(EventType::kZoneRadiusChanged, event_data::ZoneRadiusChanged);
MAP_EVENT_TYPE_(EventType::kAuraCreated, event_data::AuraCreated);
MAP_EVENT_TYPE_(EventType::kAuraDestroyed, event_data::AuraDestroyed);
ILLUVIUM_ENSURE_ENUM_SIZE(EventType, 84);  // Forgot to add enum to mapping?

#undef MAP_EVENT_TYPE_

}  // namespace event_impl
}  // namespace simulation
