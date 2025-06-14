#pragma once

#include "data/constants.h"
#include "data/skill_data.h"
#include "data/source_context.h"

namespace simulation
{
class DashData
{
public:
    // The source of this dash
    SourceContextData source_context;

    // Source of this dash
    EntityID sender_id = kInvalidEntityID;

    // Target of this dash
    EntityID receiver_id = kInvalidEntityID;

    // An "Apply All" modifier can be set if this is used, which would apply the effect to any
    // unit hit on the way through to its target.
    bool apply_to_all = false;

    // Whether the dash lands behind the receiver
    bool land_behind = false;

    // How close we need to dash before delivering the effect package
    int range_units = 0;

    // The skill this dash is transporting.
    // Pointer to the original data
    std::shared_ptr<SkillData> skill_data = nullptr;
};

}  // namespace simulation
