#pragma once

#include "components/abilities_component.h"
#include "ecs/system.h"
#include "utility/hex_grid_position.h"

namespace simulation::event_data
{
struct AbilityActivated;
struct AbilityDeactivated;
struct ActivateAnyAbility;
struct AppliedDamage;
struct ApplyEnergyGain;
struct Effect;
struct EffectPackage;
struct Fainted;
struct StatChanged;
struct Skill;
struct Hyperactive;
struct ShieldWasHit;
}  // namespace simulation::event_data

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * AbilitySystem
 *
 * This class serves to handle the abilities and skills of the game
 * Does attack abilities/omega abilities
 * --------------------------------------------------------------------------------------------------------
 */
class AbilitySystem : public System
{
    typedef System Super;
    typedef AbilitySystem Self;

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;
    void PostTimeStep(const Entity& entity) override;

    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kAbility;
    }

    // Helper function that does the following:
    // - ChooseInnateAbility
    // - TryToActivateInnate
    // NOTE: activator_id is https://illuvium.atlassian.net/wiki/spaces/AB/pages/256379211/Activator+List
    bool ChooseAndTryToActivateInnateAbilities(
        const AbilityStateActivatorContext& activator_context,
        const EntityID sender_id,
        const ActivationTriggerType trigger_type);
    bool ChooseAndTryToActivateInnateAbilities(
        const AbilityStateActivatorContext& activator_context,
        const Entity& sender_entity,
        const ActivationTriggerType trigger_type);

    // Apply the skill effects package to the target
    void ApplyEffectPackage(
        const Entity& sender_entity,
        const AbilityStatePtr& ability,
        const EffectPackage& effect_package,
        const bool is_critical,
        const EntityID receiver_id);

    // Apply the skill effects package to the location
    // This version exists to send same event as version for receiver_id
    // so that VFX in game can be sent/spawned at proper places.
    void ApplyEffectPackage(
        const Entity& sender_entity,
        const AbilityStatePtr& ability,
        const EffectPackage& effect_package,
        const bool is_critical,
        const HexGridPosition receiver_position);

    // Return true if the original effect should be skipped
    static bool ShouldSkipOriginalEffectPackage(const EffectPackage& effect_package);

    // Helper function that empowers propagation effect package with
    // original effect package ignores propagation attribute to avoid
    // recursion if
    void AddOriginalEffectPackage(EffectPackage& destination, const EffectPackage& original);

    // Handles propagation effect if effect package has it
    void ApplyEffectPackagePropagation(
        const Entity& sender_entity,
        const EffectPackage& effect_package,
        const SourceContextData& context,
        const bool is_critical,
        const Entity& receiver_entity);

    // Apply thorns if the value is present
    void ApplyThorns(
        const Entity& sender_entity,
        const AbilityStatePtr& ability,
        const EntityID receiver_id,
        const bool is_critical) const;

    // Finds the targets of the skill and deploys it
    void TargetAndDeploySkill(const Entity& sender_entity, AbilityStatePtr& ability, SkillState& skill);

    // Deploys this skill to the targets
    void DeploySkill(
        const Entity& sender_entity,
        const EntityID original_sender_id,
        AbilityStatePtr& ability,
        SkillState& skill);

    // Check ability activation trigger attributes.
    // If with_trigger_counter is true, special counter will also be tested
    bool CanActivateAbilityForTriggerData(
        const Entity& sender_entity,
        const AbilityState& state,
        const int time_step,
        const bool with_trigger_counter = true) const;

    // Check activator context with specified attributes
    bool CanActivateAbilityByActivatorContext(
        const Entity& sender_entity,
        const AbilityState& state,
        const AbilityStateActivatorContext& ability_state_activator_context,
        const bool with_trigger_counter) const;

    // Tests if entities satisfy allegiance requirement
    bool DoEntitiesMatchAllegianceType(
        const Entity& sender_entity,
        const EntityID activator_entity_id,
        AllegianceType allegiance_type) const;

    // Tests if activator context passes allegiance requirements that come from trigger data
    bool DoesActivatorContextMatchAllegianceType(
        const Entity& sender_entity,
        const AbilityActivationTriggerData& activation_trigger_data,
        const AbilityStateActivatorContext& ability_state_activator_context) const;

    // Helper function that executes one instant skill with predefined sender and receiver
    void DeployInstantTargetedSkill(
        EntityID sender_id,
        EntityID receiver_id,
        const std::shared_ptr<SkillData>& skill_data,
        SourceContextType source_context);

    static bool DoesPassContextRequirement(
        const Entity& receiver_entity,
        const AbilityTriggerContextRequirement context_requirement);

private:
    // Listen to world events
    void OnActivateAnyAbility(const event_data::ActivateAnyAbility& data);
    void OnAbilityActivated(const event_data::AbilityActivated& data);
    void OnEffectPackageMissed(const event_data::EffectPackage& data);
    void OnDamage(const event_data::AppliedDamage& event);
    void OnEffectPackageReceived(const event_data::EffectPackage& data);
    void OnEffectPackageDodged(const event_data::EffectPackage& data);
    void OnCombatUnitFainted(const event_data::Fainted& event);
    void OnHealthChanged(const event_data::StatChanged& data);
    void OnEneryGain(const event_data::ApplyEnergyGain& event);
    void OnEffectApplied(const event_data::Effect& data);
    void OnAttachedEffectApplied(const event_data::Effect& data);
    void OnAttachedEffectRemoved(const event_data::Effect& data);
    void OnSkillFinished(const event_data::Skill& data);
    void OnGoneHyperactive(const event_data::Hyperactive& data);
    void OnShieldWasHit(const event_data::ShieldWasHit& data);

    void DeployDirectSkill(
        const Entity& sender_entity,
        AbilityStatePtr& ability,
        SkillState& skill,
        const std::vector<EntityID>& filtered_receiver_ids,
        const std::shared_ptr<SkillData>& skill_data_with_empowers);

    // Checks if we can continue the time step
    bool TimeStepCheckFocus(const Entity& entity);

    // Tries to activate any ability on this entity
    void ChooseAndActivateAnyAbility(const Entity& sender_entity, const bool can_do_attack, const bool can_do_omega);

    // TimeStep entity and ability
    void TimeStepTimers(const Entity& sender_entity);
    void TimeStepTimerState(const Entity& sender_entity, const AbilityStatePtr& ability, const int attack_speed_ratio);

    // Can this ability be activated?
    bool CanActivateAbility(const Entity& sender_entity, const AbilityState& state, const int time_step) const;

    // Sets the current state for the ability
    void SetSkillState(
        const Entity& sender_entity,
        const SkillStateType new_state_type,
        AbilityStatePtr& ability,
        SkillState& skill);

    // Update current skill targeting
    void UpdateSkillTargeting(
        const Entity& sender_entity,
        const AbilityStatePtr& ability,
        SkillState& skill,
        const bool merge);

    // Force finishes all skills of this ability
    void ForceFinishAllSkills(const Entity& sender_entity, AbilityStatePtr& ability);

    // Force finishes all skills of this ability due to losing targets
    bool HandleTargetingFailure(const Entity& sender_entity, AbilityStatePtr& ability);

    void CancelAttackOnFullEnergy(const Entity& entity);

    // Used to regen health and energy every time step
    void RegenerateHealthAndEnergy(const Entity& receiver_entity) const;

    // Tries Executes this ability by stepping through its skills
    void TryToStepAbility(const Entity& sender_entity, AbilityStatePtr& ability);

    // Actual steps through the ability
    void TimeStepAbility(const Entity& sender_entity, AbilityStatePtr& ability);

    // Just apply the effect package attributes
    void ApplyEffectPackageAttributes(
        const Entity& sender_entity,
        const EffectPackageAttributes& attributes,
        const SourceContextData& context) const;

    // Helper function to check the Positive State EffectPackageBlock of the receiver entity
    // Return true if the receiver blocked he effect package
    // Return false otherwise
    bool ReceiverBlockEffectPackage(
        const Entity& sender_entity,
        const Entity& receiver_entity,
        const AbilityType ability_type);

    // Tries to activate this omega ability
    bool TryToActivateOmega(const Entity& sender_entity, AbilityStatePtr& ability);

    // Tries to activate this attack ability
    bool TryToActivateAttack(const Entity& sender_entity, AbilityStatePtr& ability);

    // Try to clear reserved position
    void TryToClearReservedPosition(const Entity& sender_entity);

    // Is this ability critical?
    void CheckAndSetCriticalSkills(
        const Entity& sender_entity,
        const StatsData& sender_live_stats,
        const AbilityStatePtr& ability,
        const bool always_roll,
        bool* out_ability_is_critical,
        bool* out_random_is_critical) const;

    // Activates an ability
    void Activate(const Entity& sender_entity, AbilityStatePtr& ability);

    // Deactivate an ability
    void Deactivate(const Entity& sender_entity, AbilityStatePtr& ability);

    // TODO(vampy): Move this into the targeting helper?
    void GetFinalReceiverIDs(
        const std::shared_ptr<SkillData>& skill_data,
        const std::vector<EntityID>& receiver_ids,
        std::vector<EntityID>* out_final_receiver_ids);

    // Invalidate target_entity in a entity's targeting state if it there
    void InvalidateTargetIfNeeded(const Entity& entity, const Entity& target_entity);

    // Restore target_entity in a entity's targeting state if it there
    void RestoreTargetIfNeeded(const Entity& entity, const Entity& target_entity);

    // Try start targeting of current/first skill in ability
    // Set activation_targeting to true if we want to check initial targeting
    // In case of activation_targeting is false function is performing current skill targeting check
    bool TryInitSkillTargeting(const Entity& entity, AbilityStatePtr& state, bool activation_targeting);

    // Execute custom logic for point reservation for current skill if needed
    void HandleReservingPositionIfNeeded(const Entity& entity, AbilityStatePtr& state);

    //
    // Innate
    //

    // Activate all the instant innate abilities
    // Returns true if it managed to activate any
    bool ActivateAllInstantInnateAbilities(const Entity& entity);

    // Adds the innate to queue if it does not already exist
    // Returns true: if the innate was added to the queue or the innate already exists
    bool AddInnateToQueue(const Entity& sender_entity, const AbilityStatePtr& ability, const bool force_add_to_queue);

    // Tries to activate this innate ability or add to the queue
    void TryToActivateInnateOrAddToQueue(
        const Entity& sender_entity,
        const AbilityStatePtr& ability,
        bool* out_activated,
        bool* out_added_to_queue);

    // Tries to activate this innate ability
    bool TryToActivateInnate(const Entity& sender_entity, AbilityStatePtr& ability);

    //
    // Internal variables
    //

    // TODO(vampy): Find a better way to deal with this
    // In some cases we can't do an attack ability in the same time step so we must force it to be in the next time step
    bool force_next_time_step_ = false;

    struct WoundRecordingFrame
    {
        EntityID sender = kInvalidEntityID;
        EntityID receiver = kInvalidEntityID;
        AbilityType ability_type = AbilityType::kNone;
        FixedPoint accumulated_damage = 0_fp;
    };

    size_t AddWoundRecordingFrame(const EntityID sender_id, const EntityID receiver_id, const AbilityType ability_type);
    void
    ApplyWounds(const WoundRecordingFrame& frame, const EffectPackage& effect_package, const EffectState& effect_state);

    std::vector<WoundRecordingFrame> wound_recording_frames_;
    bool currently_applying_wound_ = false;

    // Google test friends
#ifdef GOOGLE_UNIT_TEST
    // Ability system tests
    friend class AbilitySystemBaseTest;
#endif  // GOOGLE_UNIT_TEST
};

}  // namespace simulation
