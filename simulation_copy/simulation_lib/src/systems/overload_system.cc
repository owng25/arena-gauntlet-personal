#include "systems/overload_system.h"

#include "components/combat_unit_component.h"
#include "components/stats_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "systems/effect_system.h"
#include "utility/entity_helper.h"

namespace simulation
{

void OverloadSystem::Init(World* world)
{
    Super::Init(world);
}

void OverloadSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    // We are only interested in combat units
    if (!EntityHelper::IsACombatUnit(entity))
    {
        return;
    }

    if (!world_->CanApplyOverloadDamage())
    {
        return;
    }

    ApplyOverloadDamage(entity);
}

void OverloadSystem::ApplyOverloadDamage(const Entity& entity)
{
    const FixedPoint overload_damage_percentage = world_->GetOverloadDamagePercentage();
    if (overload_damage_percentage == 0_fp)
    {
        return;
    }

    UpdateTeamUnitsCount();

    const StatsData live_stats = world_->GetLiveStats(entity);
    const int alive_team_members_count = team_units_count_.GetOrAdd(entity.GetTeam());
    const FixedPoint max_health = live_stats.Get(StatType::kMaxHealth);
    const FixedPoint overload_damage =
        Math::CalculateOverloadDamage(overload_damage_percentage, max_health, alive_team_members_count);
    if (overload_damage == 0_fp)
    {
        return;
    }

    LogDebug(
        entity.GetID(),
        "OverloadSystem::ApplyOverloadDamage - Apply Purest Damage Max Health = {}, Overload Damage Percent = {}, "
        "Overload Damage = {}, Alive Team Members Count = {}",
        max_health,
        overload_damage_percentage,
        overload_damage,
        alive_team_members_count);

    EffectState effect_state;
    effect_state.source_context.Add(SourceContextType::kOverload);
    effect_state.sender_stats = world_->GetFullStats(entity);
    const auto purest_effect_data =
        EffectData::CreateDamage(EffectDamageType::kPurest, EffectExpression::FromValue(overload_damage));

    world_->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
        entity.GetID(),
        entity.GetID(),
        purest_effect_data,
        effect_state);
}

void OverloadSystem::UpdateTeamUnitsCount()
{
    // Do that only once per time step
    const int current_time_step = GetTimeStepCount();
    if (current_time_step == team_units_count_last_update_)
    {
        return;
    }

    team_units_count_last_update_ = current_time_step;
    team_units_count_.Clear();
    world_->SafeWalkAll(
        [&](const Entity& entity)
        {
            if (!entity.IsActive())
            {
                return;
            }

            if (!EntityHelper::IsACombatUnit(entity))
            {
                return;
            }

            const Team team = entity.GetTeam();
            team_units_count_.GetOrAdd(team) += 1;
        });
}

}  // namespace simulation
