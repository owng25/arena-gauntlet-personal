#pragma once

#include "data/constants.h"
#include "data/effect_package.h"
#include "data/source_context.h"
#include "utility/enum_set.h"

namespace simulation
{
struct SplashData
{
    // The source of this splash
    SourceContextData source_context;

    // Source of this splash
    // NOTE: This can be the id of a Zone/Projectile/Combat unit
    EntityID sender_id = kInvalidEntityID;

    // Whether the splash is a crit
    bool is_critical = false;

    // Whether the splash will also apply to the focus of the attack
    bool ignore_first_propagation_receiver = false;

    // Radius of affected area
    int splash_radius_units = false;

    // Whether propagation skill can deploy to ground, airborne or both, defaults to Ground.
    EnumSet<GuidanceType> deployment_guidance = MakeEnumSet(GuidanceType::kGround);

    // Whether propagation skill can target ground, airborne or both, defaults to Ground.
    EnumSet<GuidanceType> targeting_guidance = MakeEnumSet(GuidanceType::kGround);

    // The propagation effect package
    EffectPackage propagation_effect_package;
};

}  // namespace simulation
