#pragma once

#include "components/attached_effects_component.h"
#include "ecs/system.h"

namespace simulation
{

namespace event_data
{
struct AbilityDeactivated;
}

/* -------------------------------------------------------------------------------------------------------
 * AttachedEffectsSystem
 *
 * This class handles effects attached to an entity
 * --------------------------------------------------------------------------------------------------------
 */
class AttachedEffectsSystem : public System
{
    typedef System Super;
    typedef AttachedEffectsSystem Self;

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;
    void PostTimeStep(const Entity&) override;
    void PostSystemTimeStep() override;

    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kAttachedEffect;
    }

    // Removes all attached effects in the world
    void RemoveAllEffects();

private:
    void UpdateDynamicBuffsDebuffs();

    // Helper function to time step the attached effects
    void TimeStepAttachedEffects(const Entity& receiver_entity);

    // Time step just this attached_effect
    // NOTE: Only used if the effect is is Waiting or Active state
    // If the effect is expired it will be marked for destruction
    void TimeStepAttachedEffect(const Entity& receiver_entity, const AttachedEffectStatePtr& attached_effect);

    // Listen to world events
    void OnAbilityDeactivated(const event_data::AbilityDeactivated& data);

    // Helper function that is called on ability deactivated that just increases the consumable activations
    void AbilityDeactivatedIncreaseConsumableActivations(
        const Entity& sender,
        const std::vector<AttachedEffectStatePtr>& attached_effects,
        const event_data::AbilityDeactivated& event) const;

    // Initialize the constant data of the effect
    bool InitEffectStateData(const Entity& receiver_entity, AttachedEffectState& attached_effect_state) const;

    // Activate the effect on the entity
    void ActivateEffect(const Entity& receiver_entity, AttachedEffectState& attached_effect_state) const;

    // TimeStep the values of an effect that is applied over time
    void TimeStepApplyEffectOverTime(const Entity& receiver_entity, AttachedEffectState& attached_effect_state) const;

    // Helper function to handle blink timing
    void TimeStepBlink(const Entity& receiver_entity, AttachedEffectState& attached_effect_state) const;

    // Apply the effects to the entity
    void ApplyEffect(
        const EntityID sender_id,
        const EntityID receiver_id,
        const EffectData& effect_data,
        const EffectState& effect_state) const;

    // Checking if an effect needs to be activated
    bool ShouldActivateEffect(
        const EntityID receiver_id,
        const EntityID sender_id,
        const EffectValidations& effect_validations) const;

    // Check if effect state should be remove removed in PostTimeStep instead of TimeStep
    static bool ShouldBeRemovedInPostTimeStep(const AttachedEffectStatePtr& attached_effect_state);
};

}  // namespace simulation
