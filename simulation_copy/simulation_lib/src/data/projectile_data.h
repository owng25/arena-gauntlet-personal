#pragma once

#include "data/constants.h"
#include "data/enums.h"
#include "data/skill_data.h"
#include "utility/enum_set.h"

namespace simulation
{
struct ProjectileData
{
    // The source of this projectile
    SourceContextData source_context;

    // Source of this projectile
    EntityID sender_id = kInvalidEntityID;

    // Target of this projectile
    EntityID receiver_id = kInvalidEntityID;

    // Size of this projectile
    int radius_units = 0;

    // Movement speed in subunits per time step
    // See PositionComponent and MovementComponent
    int move_speed_sub_units = 0;

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

    // Determines if the projectile can proceed after hitting the target entity.
    // Incompatible with "is_homing"
    bool continue_after_target = false;

    // Deployment guidance for the projectile
    EnumSet<GuidanceType> deployment_guidance = MakeEnumSet(GuidanceType::kGround);

    // The skill this projectile is transporting.
    // Pointer to the original data
    std::shared_ptr<SkillData> skill_data = nullptr;
};

}  // namespace simulation
