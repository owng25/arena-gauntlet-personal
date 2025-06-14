#pragma once

#include "data/constants.h"
#include "data/skill_data.h"

namespace simulation
{
struct ZoneData
{
    // The source of this zone
    SourceContextData source_context;

    // The skill this zone is transporting.
    // Pointer to the original data
    std::shared_ptr<SkillData> skill_data = nullptr;

    // Allows skipping activations by index
    std::vector<int> skip_activations;

    // Source of this zone
    EntityID sender_id = kInvalidEntityID;

    // Orignal source of this projectile, i.e. splash
    EntityID original_sender_id = kInvalidEntityID;

    // Shape of the zone if any
    ZoneEffectShape shape = ZoneEffectShape::kNone;

    // Radius of the zone in subunits
    int radius_sub_units = 0;

    // Max radius of the zone in subunits
    int max_radius_sub_units = 0;

    // Width of the zone in subunits
    int width_sub_units = 0;

    // Height of the zone in subunits
    int height_sub_units = 0;

    // For how long the zone lasts in ms
    int duration_ms = 0;

    // How frequent is the zone in ms
    int frequency_ms = 0;

    // Direction the zone is spawned, if applicable - calculated at runtime based on angle
    // between spawning entity and its focus
    int spawn_direction_degrees = 0;

    // Direction of the zone, if applicable
    int direction_degrees = 0;

    // Movement speed of the zone in subunits per time step
    int movement_speed_sub_units_per_time_step = 0;

    // Growth rate of the zone in subunits per time step
    int growth_rate_sub_units_per_time_step = 0;

    // Handle collision for zone globally if set
    int global_collision_id = kInvalidGlobalCollisionID;

    // Whether zone is attached to an entity
    EntityID attach_to_entity = kInvalidEntityID;

    // Whether zone effect will only be applied once
    bool apply_once = false;

    // Whether zone effect will critical
    bool is_critical = false;

    // Whether zone was channeled
    bool is_channeled = false;

    // Zone will be destroyed if sender faints
    bool destroy_with_sender = false;
};

}  // namespace simulation
