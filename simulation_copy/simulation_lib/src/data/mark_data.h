#pragma once

#include "data/ability_data.h"

namespace simulation
{
class EffectData;

struct MarkData
{
    // The source of this mark
    SourceContextData source_context;

    // Source of this mark
    EntityID sender_id = kInvalidEntityID;

    // Receiver of this mark
    EntityID receiver_id = kInvalidEntityID;

    // Mark duration in ms
    int duration_ms = 0;

    // Should the attached entity (shield/mark) be destroyed if the combat unit who sent this dies?
    bool should_destroy_on_sender_death = false;

    // Abilities to add to a combat unit
    std::vector<std::shared_ptr<const AbilityData>> abilities_data;
};

}  // namespace simulation
