#pragma once

#include "data/constants.h"
#include "data/skill_data.h"
#include "data/source_context.h"

namespace simulation
{
class BeamData
{
public:
    // The source of this beam
    SourceContextData source_context;

    // Source of this beam
    EntityID sender_id = kInvalidEntityID;

    // Receiver of this beam
    EntityID receiver_id = kInvalidEntityID;

    // Width of the beam in subunits
    int width_sub_units = 0;

    // Length of the beam in subunits
    // NOTE: Calculated at runtime as the distance between the sender_id and receiver_id
    int length_world_sub_units = 0;

    // How frequent is the beam in ms
    int frequency_ms = 0;

    // Direction of the beam, if applicable - calculated at runtime based on angle between
    // spawning entity and its focus
    int direction_degrees = 0;

    // Whether beam effect will only be applied once
    bool apply_once = false;

    // Whether the beam is homing
    bool is_homing = false;

    // Whether the beam is blockable
    bool is_blockable = false;

    // Whether the beam is critical
    bool is_critical = false;

    // Which allegiance is able to block the beam
    AllegianceType block_allegiance = AllegianceType::kEnemy;

    // The skill this beam is transporting.
    // Pointer to the original data
    std::shared_ptr<SkillData> skill_data = nullptr;
};

}  // namespace simulation
