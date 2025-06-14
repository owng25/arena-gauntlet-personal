#pragma once

#include "data/constants.h"
#include "data/source_context.h"
#include "effect_data.h"

namespace simulation
{
class AuraData
{
public:
    // A list of effects this aura applies
    EffectData effect{};

    // For now effect_sender and aura_sender are going to be the same
    // but we might want a JSON option for precise control in the future
    EntityID aura_sender_id = kInvalidEntityID;
    EntityID effect_sender_id = kInvalidEntityID;
    EntityID receiver_id = kInvalidEntityID;
    SourceContextData source_context;
};
}  // namespace simulation
