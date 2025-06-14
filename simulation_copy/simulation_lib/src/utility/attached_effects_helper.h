#pragma once

#include "components/abilities_component.h"
#include "components/attached_effects_component.h"
#include "data/effect_package.h"
#include "data/enums.h"
#include "ecs/entity.h"
#include "utility/logger_consumer.h"

namespace simulation
{
class World;

/* -------------------------------------------------------------------------------------------------------
 * AttachedEffectsHelper
 *
 * Helper methods for the attached effects
 * --------------------------------------------------------------------------------------------------------
 */
class AttachedEffectsHelper final : public LoggerConsumer
{
public:
    AttachedEffectsHelper() = default;
    explicit AttachedEffectsHelper(World* world);

    //
    // LoggerConsumer interface
    //

    std::shared_ptr<Logger> GetLogger() const override;
    void BuildLogPrefixFor(const EntityID id, std::string* out_string) const override;
    int GetTimeStepCount() const override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kAttachedEffect;
    }

    //
    // Own functions
    //

    // Adds an root attached effect to this entity
    void AddAttachedEffect(const Entity& receiver_entity, const AttachedEffectStatePtr& attached_effect) const
    {
        AddRootAttachedEffect(receiver_entity, attached_effect);
    }
    AttachedEffectState& AddAttachedEffect(
        const Entity& receiver_entity,
        const EntityID sender_id,
        const EffectData& effect_data,
        const EffectState& effect_state) const
    {
        const auto attached_effect = AttachedEffectState::Create(sender_id, effect_data, effect_state);
        AddRootAttachedEffect(receiver_entity, attached_effect);
        return *attached_effect.get();
    }

    // Removes an attached effect
    void RemoveAttachedEffect(
        const Entity& receiver_entity,
        const AttachedEffectStatePtr& attached_effect,
        const int delay_time_steps = 0) const;

    // Helper method to figure out if the this attached_effect (empower/disempower) can be used now
    bool CanUseEmpowerOrDisempower(
        const EntityID entity_id,
        const AbilityType ability_type,
        const AttachedEffectStatePtr& attached_effect,
        const bool is_empower,
        const bool is_critical) const;

    // Helper method to figure out if the attached effect matches the ability that was activated to iterate
    bool CanUseAttachedEffectForAbilityType(
        const EntityID entity_id,
        const AttachedEffectStatePtr& attached_effect,
        const AbilityType ability_activated,
        const bool is_critical,
        const bool use_log = false,
        const std::string_view& log_prefix = "") const;

    // Helper function to increase the values for all the consumable attached effects
    bool IncreaseConsumableActivations(
        const EntityID entity_id,
        const AttachedEffectStatePtr& attached_effect,
        const AbilityType ability_activated,
        const bool is_critical) const;

    //
    // Special cases for buffs/debuffs
    //

    // Refreshes captured value of dynamic effect and handles corner cases (like max health)
    void RefreshDynamicBuffsDebuffs(const Entity& receiver_entity, AttachedEffectState& attached_effect) const;

    void AdjustFromBuffsDebuffs(const Entity& receiver_entity, const AttachedEffectState& attached_effect) const;
    void UndoAdjustFromBuffsDebuffs(const Entity& receiver_entity, const AttachedEffectState& attached_effect) const;

private:
    // Enable extra logs for debugging for this class
    bool EnableExtraLogs() const;

    // Adds only a root attached effect
    void AddRootAttachedEffect(const Entity& receiver_entity, const AttachedEffectStatePtr& attached_effect) const;

    // Add a child attached effect that is the child of the parent attached effect
    void AddChildAttachedEffect(
        const Entity& receiver_entity,
        const AttachedEffectStatePtr& parent,
        const AttachedEffectStatePtr& child) const;

    // Removes all children of this attached effect
    void RemoveAllChildrenFromAttachedEffect(
        const Entity& receiver_entity,
        const AttachedEffectStatePtr& attached_effect,
        const int delay_time_steps) const;

    // Keep track of this attached effect by adding it to the attached_effect vector.
    void KeepTrackOfAttachedEffect(const Entity& receiver_entity, const AttachedEffectStatePtr& attached_effect) const;

    // Syncs the condition with the current statcks
    void SyncConditionWithCurrentCurrentStacks(
        const Entity& receiver_entity,
        const AttachedEffectStatePtr& condition_root_effect) const;

    // Adds the attached effect to the active container
    void AddAttachedEffectToActive(
        const Entity& receiver_entity,
        const AttachedEffectStatePtr& attached_effect,
        const bool do_buffs_adjustments) const;

    // Removes the attached effect from the active container
    void RemoveAttachedEffectFromActive(
        const Entity& receiver_entity,
        const AttachedEffectStatePtr& attached_effect,
        const bool remove_children) const;

    // Processes all the attached effects of this effect_type_id
    void ProcessAllAttachedEffectsOfTypeID(
        const Entity& receiver_entity,
        const EffectTypeID& effect_type_id,
        const bool from_remove) const;

    // What it says
    void KeepAttachedEffectInWaitingAndRemoveFromActive(
        const Entity& receiver_entity,
        const AttachedEffectStatePtr& attached_effect) const;

    // Process the attached_effects using the process_type
    void ProcessAttachedEffects(
        const Entity& receiver_entity,
        const EffectOverlapProcessType process_type,
        const std::vector<AttachedEffectStatePtr>& attached_effects,
        const bool from_remove) const;

    EffectOverlapProcessType GetDefaultOverlapType(const EffectType effect_type, const bool same_ability) const;

    // Gets the comparison value used by this effect type
    // Returns false if we can't get a comparison value
    bool GetEffectComparisonValue(
        const EffectData& effect_data,
        const ExpressionEvaluationContext& context,
        const ExpressionStatsSource& stats_source,
        FixedPoint* out_comparison_value) const;

    // Helper to get the attached effect with the highest value.
    // See: https://illuvium.atlassian.net/wiki/spaces/AB/pages/280297473/Highest+Processing
    // NOTE: Assumes all the attached_effects are of the same type.
    AttachedEffectStatePtr GetHighestValueAttachedEffect(
        const Entity& receiver_entity,
        const std::vector<AttachedEffectStatePtr>& attached_effects) const;

    // Helper to merge all the attached effects into the most impactful one.
    // See: https://illuvium.atlassian.net/wiki/spaces/AB/pages/280330315/Merge+Processing
    // NOTE: Assumes all the attached_effects are of the same type.
    AttachedEffectStatePtr MergeAllAttachedEffects(
        const Entity& receiver_entity,
        const std::vector<AttachedEffectStatePtr>& attached_effects) const;

    // Helper method to stack all the attached effects.
    // See: https://illuvium.atlassian.net/wiki/spaces/AB/pages/280330241/Stacking+Processing
    // NOTE: Assumes all the attached_effects are of the same type.
    AttachedEffectStatePtr StackAllAttachedEffects(
        const Entity& receiver_entity,
        const std::vector<AttachedEffectStatePtr>& attached_effects,
        bool* out_changed_value) const;

    // Helper method to stack all the condition attached effects.
    // See: https://illuvium.atlassian.net/wiki/spaces/AB/pages/280330257/Condition+Processing
    // NOTE: Assumes all the attached_effects are of the same type.
    AttachedEffectStatePtr StackAllConditionAttachedEffects(
        const Entity& receiver_entity,
        const std::vector<AttachedEffectStatePtr>& attached_effects) const;

    // Helper method to create a new expression that is lhs * stacks
    static EffectExpression CreateStacksMultiplierExpression(const EffectExpression& lhs, const int stacks);

    // Helper method to merge the lifetime of src and dst into dst.
    // This handles both duration and consumables
    void StackLifetimeOfEffectsData(const Entity& receiver_entity, const EffectData& src, EffectData& dst) const;
    void MergeLifetimeOfEffectsData(const Entity& receiver_entity, const AttachedEffectState& src, EffectData& dst)
        const;

    // Helper function to increase the stacks count
    void IncreaseStacksCount(const Entity& receiver_entity, const AttachedEffectStatePtr& attached_effect) const;

    // Notification when the current_stacks_count reached the max_stacks count
    void OnReachedMaxStacks(const Entity& receiver_entity, const AttachedEffectStatePtr& attached_effect) const;
    void OnConditionReachedMaxStacks(const Entity& receiver_entity, const AttachedEffectStatePtr& attached_effect)
        const;

    // Special cases for buffs/debuffs
    void AdjustHealthFromBuffs(const Entity& receiver_entity, const AttachedEffectState& attached_effect) const;
    void UndoAdjustHealthFromBuffs(const Entity& receiver_entity, const AttachedEffectState& attached_effect) const;

    void AdjustMovementSpeedFromBuffs(const Entity& receiver_entity, const AttachedEffectState& attached_effect) const;

    // NOTE: This does not have an undo on purpose
    void AdjustAttackDamageFromBuffs(const Entity& receiver_entity, const AttachedEffectState& attached_effect_state)
        const;

    // Owner world of this helper
    World* world_ = nullptr;
};

}  // namespace simulation
