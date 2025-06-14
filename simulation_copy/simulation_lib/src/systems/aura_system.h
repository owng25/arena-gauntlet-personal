#pragma once

#include <unordered_map>
#include <unordered_set>

#include "ecs/system.h"

namespace simulation
{
class AttachedEffectState;

class AuraSystem : public System
{
    using Super = System;
    using Self = AuraSystem;

    struct Helper;

    struct EffectInfo
    {
        EntityID receiver = kInvalidEntityID;
        std::shared_ptr<AttachedEffectState> effect;
    };

public:
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kAura;
    }

private:
    bool ShouldDestroyAura(const Entity& aura) const;
    void RemoveAuraEffects(const Entity& aura);
    void DestroyAura(const Entity& aura);
    void RefreshAuraEffects(const Entity& aura);
    static void FindAuraTargets(World& world, const Entity& aura, std::vector<EntityID>* out_targets);

    // Just to avoid allocating vectors on each frame to find aura targets
    std::vector<EntityID> targeting_cache_;

    // Key: aura entity id
    // Value: list of effects applied by aura
    std::unordered_map<EntityID, std::vector<EffectInfo>> applied_effects_;
};
}  // namespace simulation
