#pragma once

#include <unordered_set>

#include "ecs/system.h"

namespace simulation
{
namespace event_data
{
struct AbilityDeactivated;
struct Effect;
struct Focus;
}  // namespace event_data

/* -------------------------------------------------------------------------------------------------------
 * FocusSystem
 *
 * This system deals with the logic for the focus selection that is used by the attack abilities and
 * the movement system.
 *
 * See https://illuvium.atlassian.net/wiki/spaces/AB/pages/204308495/Focusing+and+Refocusing
 * --------------------------------------------------------------------------------------------------------
 */
class FocusSystem : public System
{
    typedef System Super;
    typedef FocusSystem Self;

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;
    void InitialTimeStep(const Entity& entity) override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kFocus;
    }

    // Whether there is another potential focus available
    bool HasOtherPotentialFocus(const Entity& entity) const;

    // Whether another target is significantly closer
    bool HasOtherTargetSignificantlyCloserThanFocus(const Entity& entity) const;

    // Find and update target when enemy enters range
    bool FindAndUpdateTargetInRange(const Entity& entity);

    // Choose a focus based on selection type
    std::shared_ptr<Entity> ChooseFocus(const Entity& entity, const std::unordered_set<EntityID>& exclude_entities)
        const;

    // Check whether current focus exist and in attack/omega range
    // is_omega_range is false to check attack range, is_omega_range is true to use omega range
    static bool HasFocusInAttackRange(const Entity& entity, bool is_omega_range);

private:
    // Listen to world events
    void OnFocusUnreachable(const event_data::Focus& data);
    void OnAbilityDeactivated(const event_data::AbilityDeactivated& data);
    void OnEffectApplied(const event_data::Effect& data);
    void OnAttachedEffectRemoved(const event_data::Effect& data);

    bool ConsiderRefocus(const Entity& entity);

    // Check for an active focused effect
    bool CheckForFocusedEffect(const Entity& entity);

    // Check for an active taunted effect
    bool CheckForTauntedEffect(const Entity& entity);

};  // class FocusSystem

}  // namespace simulation
