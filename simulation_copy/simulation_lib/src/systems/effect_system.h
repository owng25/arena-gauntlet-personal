#pragma once

#include "components/attached_entity_component.h"
#include "data/effect_data.h"
#include "ecs/event_types_data.h"
#include "ecs/system.h"
#include "utility/gtest_friend.h"

namespace simulation
{

namespace event_data
{
struct Effect;
}
/* -------------------------------------------------------------------------------------------------------
 * EffectSystem
 *
 * Handles applying the effects from an effect package
 * --------------------------------------------------------------------------------------------------------
 */
class EffectSystem : public System
{
    typedef System Super;
    typedef EffectSystem Self;

public:
    void Init(World* world) override;
    void TimeStep(const Entity& entity) override;

    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kEffect;
    }

private:
    // Listen to world events
    void OnTryToApplyEffect(const event_data::Effect& event_data);
    void OnAttachedEffectRemoved(const event_data::Effect& event_data);

    // Apply the skill effects to the target
    void
    ApplyEffect(const EntityID sender_id, const EntityID receiver_id, const EffectData& data, const EffectState& state);

    // Apply damage effect
    void ApplyDamageEffect(
        const Entity& sender_entity,
        const Entity& receiver_entity,
        const EffectData& data,
        const EffectState& state,
        const FixedPoint effect_value,
        const FullStatsData& receiver_stats,
        DeathReasonType& out_death_reason,
        SourceContextData& out_source_context);

    // Apply damage effect
    void
    ApplyShieldBurnEffect(const Entity& sender_entity, const Entity& receiver_entity, const FixedPoint effect_value);

    // Apply displacement effect
    void ApplyDisplacementEffect(
        const Entity& sender_entity,
        const Entity& receiver_entity,
        const EffectData& data,
        const EffectState& state);

    // Apply damage to shields
    FixedPoint ApplyShieldDamage(
        const Entity& sender_entity,
        const std::vector<AttachedEntity>& attached_shields,
        const FixedPoint pm_damage);

    void ApplyStatOverrideEffect(const Entity& entity, const StatType stat_type, const FixedPoint value);

    // Adjusts the pre_mitigated_damage based on exploit weakness
    void CalculateExploitWeakness(
        const StatsData& receiver_live_stats,
        FixedPoint* out_pre_mitigated_damage,
        std::string* out_log_exploit_weakness) const;

    // Calculates post mitigation damage
    void CalculatePostMitigation(
        const FixedPoint effect_package_piercing_percentage,
        const FixedPoint sender_piercing_percentage,
        const FixedPoint flat_damage_reduction,
        const FixedPoint pre_mitigated_damage,
        const FixedPoint receiver_resist,
        FixedPoint* out_post_mitigated_damage,
        std::string* out_log_mitigation) const;

    // Apply execute if the effect has it
    bool ApplyExecuteEffect(
        const FixedPoint receiver_health,
        const AttachedEffectStatePtr& execute_effect,
        const EffectState& state,
        const EntityID sender_id,
        const EntityID receiver_id,
        const StatsData& receiver_live_stats,
        FixedPoint* out_receiver_new_health,
        FixedPoint* out_post_mitigated_damage) const;

    FixedPoint ApplyBonusDamageAndAmplify(
        const EntityID sender_id,
        const EntityID receiver_id,
        const EffectData& data,
        const EffectState& state,
        const EffectPackageAttributes& effect_package_attributes,
        const FixedPoint pre_mitigated_damage) const;

    // Checking the agency
    bool IsDeniedByAgency(
        const Entity& sender_entity,
        const Entity& receiver_entity,
        const EffectData& data,
        const EffectState& state) const;

    // The time step that receiver did purest damage
    int time_step_receivers_with_purest_damage_ = 0;

    // The array of the receivers that did purest damage
    std::unordered_set<EntityID> receivers_with_purest_damage_{};
};

}  // namespace simulation
