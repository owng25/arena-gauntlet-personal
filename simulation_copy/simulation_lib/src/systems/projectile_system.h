#pragma once

#include "ecs/system.h"

namespace simulation
{
namespace event_data
{
struct Moved;
struct AbilityDeactivated;
struct EffectPackage;
struct Focus;
}  // namespace event_data

/* -------------------------------------------------------------------------------------------------------
 * ProjectileSystem
 *
 * Handles the projectiles inside the world
 * --------------------------------------------------------------------------------------------------------
 */
class ProjectileSystem : public System
{
    typedef System Super;
    typedef ProjectileSystem Self;

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kSpawnable;
    }

private:
    // Listen to world events
    void OnEntityMovedStep(const event_data::Moved& data);
    void OnAbilityDeactivated(const event_data::AbilityDeactivated& data);
    void OnFocusNeverDeactivated(const event_data::Focus& data);
    void OnEffectPackageReceived(const event_data::EffectPackage& data);

    void HandleTargetPositionCheck(const Entity& entity);

private:
    std::vector<EntityID> collisions_;
};

}  // namespace simulation
