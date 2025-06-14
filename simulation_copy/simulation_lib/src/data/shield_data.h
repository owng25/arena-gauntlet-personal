#pragma once

#include <memory>
#include <unordered_map>

#include "data/constants.h"
#include "data/skill_data.h"
#include "data/source_context.h"
#include "ecs/event.h"

namespace simulation
{
class EffectData;

struct ShieldData
{
    // Source of this shield
    EntityID sender_id = kInvalidEntityID;

    // Receiver of this shield
    EntityID receiver_id = kInvalidEntityID;

    // Shield value - damage the shield can absorb
    FixedPoint value = 0_fp;

    // Shield duration in ms
    int duration_ms = kTimeInfinite;

    // Context of the shield being created
    SourceContextData source_context;

    // Skills that trigger on shield events
    std::unordered_map<EventType, std::shared_ptr<SkillData>> event_skills;

    // Attached effects for this shield
    std::vector<EffectData> effects;
};

}  // namespace simulation
