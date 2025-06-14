#include "systems/hyper_system.h"

#include "components/combat_synergy_component.h"
#include "components/stats_component.h"
#include "data/containers/game_data_container.h"
#include "ecs/world.h"
#include "utility/entity_helper.h"
#include "utility/hyper_helper.h"

namespace simulation
{

void HyperSystem::PreBattleStarted(const Entity& entity)
{
    static constexpr std::string_view method_name = "HyperSystem::PreBattleStarted";

    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    // Check if combat unit
    if (!EntityHelper::IsACombatUnit(entity))
    {
        return;
    }

    // Has already gone hyperactive
    auto& stats_component = entity.Get<StatsComponent>();
    if (stats_component.IsHyperactive())
    {
        return;
    }

    const auto& synergy_component = entity.Get<CombatSynergyComponent>();
    const CombatClass dominant_combat_class = synergy_component.GetDominantCombatClass();
    if (dominant_combat_class == CombatClass::kNone)
    {
        return;
    }

    const auto& game_data = world_->GetGameDataContainer();
    const auto& synergy_data = game_data.GetCombatClassSynergyData(dominant_combat_class);
    if (!synergy_data)
    {
        LogErr(
            entity.GetID(),
            "{} - attempted to add hyper abilities for an entitiy but could not find synergy data. Dominant combat "
            "class: {}.",
            method_name,
            dominant_combat_class);
        return;
    }

    LogDebug(
        entity.GetID(),
        "{} - Adding {} hyper abilities (from {})",
        method_name,
        synergy_data->hyper_abilities.size(),
        dominant_combat_class);

    auto& abilities_component = entity.Get<AbilitiesComponent>();
    abilities_component.AddDataInnateAbilities(synergy_data->hyper_abilities);
}

void HyperSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION()

    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    // Check if combat unit
    if (!EntityHelper::IsACombatUnit(entity))
    {
        return;
    }

    // Has already gone hyperactive
    auto& stats_component = entity.Get<StatsComponent>();
    if (stats_component.IsHyperactive())
    {
        return;
    }

    const EntityID entity_id = entity.GetID();
    const HyperConfig& hyper_config = world_->GetHyperConfig();

    // Calculate combat unit effectiveness for current state
    const int hyper_growth_effectiveness = HyperHelper::CalculateHyperGrowthEffectiveness(entity);

    // We can only gain hyper
    auto sub_hyper = FixedPoint::FromInt(std::max(0, hyper_growth_effectiveness));

    // Ignore non positive hyper
    if (sub_hyper <= 0_fp)
    {
        return;
    }

    // Scale kPrecisionFactor so that we can have higher precision
    sub_hyper *= FixedPoint::FromInt(kPrecisionFactor);

    const FixedPoint scaled_sub_hyper = hyper_config.sub_hyper_scale_percentage.AsPercentageOf(sub_hyper);

    const FixedPoint previous_hyper = stats_component.GetCurrentHyper();
    const FixedPoint new_sub_hyper = stats_component.GetCurrentSubHyper() + scaled_sub_hyper;
    stats_component.SetCurrentSubHyper(new_sub_hyper);

    const FixedPoint delta = stats_component.GetCurrentHyper() - previous_hyper;
    if (delta != 0_fp)
    {
        world_->BuildAndEmitEvent<EventType::kHyperChanged>(entity_id, entity_id, delta);

        LogDebug(
            entity.GetID(),
            "HyperSystem::TimeStep - sub_hyper = {}, scaled_sub_hyper = {}, new_sub_hyper = {}",
            sub_hyper,
            scaled_sub_hyper,
            new_sub_hyper);

        // Has gone HYPERACTIVE
        if (stats_component.IsHyperactive())
        {
            LogDebug(entity.GetID(), "| Has gone HYPERACTIVE");
            world_->BuildAndEmitEvent<EventType::kGoneHyperactive>(entity.GetID());
        }
    }
}

}  // namespace simulation
