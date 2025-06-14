#pragma once

#include <unordered_set>

#include "data/constants.h"
#include "data/effect_package.h"
#include "data/source_context.h"
#include "utility/enum_set.h"

namespace simulation
{
struct ChainData
{
    // The source of this chain
    SourceContextData source_context;

    // Source of this chain spawn
    EntityID sender_id = kInvalidEntityID;

    // The original combat unit who spawned this chain
    // NOTE: For the first chain spawn this is equal to the sender_id
    EntityID combat_unit_sender_id = kInvalidEntityID;

    // The id of the receiver that first received the chain propagation package
    // Aka from where the chain starts to propagate
    EntityID first_propagation_receiver_id = kInvalidEntityID;

    // The time in steps between the hit from the first Effect Package and when we begin Deployment
    // to the next Receiver.
    int chain_delay_ms = 0;

    // Integer that determines how many times this will chain. Keep in mind that the original skill
    // that specified the Propagation of chain does not count. (i.e. If you have a projectile with
    // propagation of chain, and a Chain Number of 1, when that projectile reached the target, it
    // will bounce one more time.
    int chain_number = 1;

    // Furthest distance a chain entity may bounce from the intended target
    int chain_bounce_max_distance_units = 0;

    // Pre-deployment delay percentage for auto-generated chain skill
    int pre_deployment_delay_percentage = 0;

    // Keep a list of targets that we have already bounced to, and prioritise other ones.
    bool prioritize_new_targets = false;

    // Keep a list of targets that we have already bounced to, and never bounce to them again.
    bool only_new_targets = false;

    // If true, we exclude the first propagation receiver from the target list
    bool ignore_first_propagation_receiver = false;

    // Whether the chain is a crit
    bool is_critical = false;

    // Keep track of old entities we targeted
    std::unordered_set<EntityID> old_target_entities;

    // The propagation effect package
    EffectPackage propagation_effect_package;

    // The group of entities used to select next propagation target
    AllegianceType targeting_group = AllegianceType::kEnemy;

    // Whether propagation skill can deploy to ground, airborne or both, defaults to Ground.
    EnumSet<GuidanceType> deployment_guidance = MakeEnumSet(GuidanceType::kGround);

    // Whether propagation skill can target ground, airborne or both, defaults to Ground.
    EnumSet<GuidanceType> targeting_guidance = MakeEnumSet(GuidanceType::kGround);
};

}  // namespace simulation
