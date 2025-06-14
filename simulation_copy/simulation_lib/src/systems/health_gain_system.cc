#include "systems/health_gain_system.h"

#include "components/stats_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "utility/entity_helper.h"

namespace simulation
{
void HealthGainSystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kApplyHealthGain>(this, &Self::ApplyHealthGain);
}

void HealthGainSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not active
    if (!entity.IsActive())
    {
        return;
    }
}

void HealthGainSystem::ApplyHealthGain(const event_data::ApplyHealthGain& health_gain_data)
{
    const EntityID receiver_id = health_gain_data.receiver_id;
    auto& receiver_entity = world_->GetByID(receiver_id);

    // Only for combat units
    if (!EntityHelper::IsACombatUnit(receiver_entity))
    {
        return;
    }

    auto& stats_component = receiver_entity.Get<StatsComponent>();
    const StatsData receiver_stats = world_->GetLiveStats(receiver_entity);

    FixedPoint pre_health_gain = 0_fp;
    FixedPoint health_gain_percentage = 0_fp;
    FixedPoint health_gain = 0_fp;
    std::string log_health = "";
    if (health_gain_data.missing_health_percentage_to_health > 0_fp)
    {
        const FixedPoint health_missing =
            receiver_stats.Get(StatType::kMaxHealth) - receiver_stats.Get(StatType::kCurrentHealth);
        const FixedPoint missing_health_gain =
            health_gain_data.missing_health_percentage_to_health.AsPercentageOf(health_missing);
        pre_health_gain += missing_health_gain;

        if (AreDebugLogsEnabled())
        {
            log_health = fmt::format(
                "[missing_health_gain = {}, missing_health_percentage_to_health = {}] ",
                missing_health_gain,
                health_gain_data.missing_health_percentage_to_health);
        }
    }

    switch (health_gain_data.health_gain_type)
    {
    case HealthGainType::kInstantHeal:
    {
        // Set pre_health_gain as passed value
        pre_health_gain += health_gain_data.value;

        if (AreDebugLogsEnabled())
        {
            log_health += fmt::format("kInstantHeal does pre_health_gain = {}", pre_health_gain);
        }
        break;
    }
    case HealthGainType::kInstantPureHeal:
    {
        // health_gain is not modified by efficiency for pure heals
        pre_health_gain += health_gain_data.value;

        if (AreDebugLogsEnabled())
        {
            log_health += fmt::format("kInstantPureHeal does pre_health_gain = {}", pre_health_gain);
        }
        break;
    }
    case HealthGainType::kRegeneration:
    {
        pre_health_gain = receiver_stats.Get(StatType::kHealthRegeneration);

        if (AreDebugLogsEnabled())
        {
            log_health = fmt::format("health_regeneration = {}", receiver_stats.Get(StatType::kHealthRegeneration));
        }
        break;
    }
    case HealthGainType::kPhysicalVamp:
    {
        health_gain_percentage =
            health_gain_data.vampiric_percentage + receiver_stats.Get(StatType::kPhysicalVampPercentage);
        pre_health_gain = health_gain_percentage.AsPercentageOf(health_gain_data.value);

        if (AreDebugLogsEnabled())
        {
            log_health = fmt::format(
                "post_mitigated_damage = {}, physical_vamp = {} does pre_health = {}",
                health_gain_data.value,
                health_gain_percentage,
                pre_health_gain);
        }
        break;
    }
    case HealthGainType::kEnergyVamp:
    {
        health_gain_percentage =
            health_gain_data.vampiric_percentage + receiver_stats.Get(StatType::kEnergyVampPercentage);
        pre_health_gain = health_gain_percentage.AsPercentageOf(health_gain_data.value);

        if (AreDebugLogsEnabled())
        {
            log_health = fmt::format(
                "post_mitigated_damage = {}, energy_vamp = {} does pre_health = {}",
                health_gain_data.value,
                health_gain_percentage,
                pre_health_gain);
        }
        break;
    }
    case HealthGainType::kPureVamp:
    {
        health_gain_percentage =
            health_gain_data.vampiric_percentage + receiver_stats.Get(StatType::kPureVampPercentage);
        pre_health_gain = health_gain_percentage.AsPercentageOf(health_gain_data.value);

        if (AreDebugLogsEnabled())
        {
            log_health = fmt::format(
                "post_mitigated_damage = {}, pure_vamp = {} does pre_health = {}",
                health_gain_data.value,
                health_gain_percentage,
                pre_health_gain);
        }
        break;
    }
    default:
        break;
    }

    // Modify by efficiency if not a pure heal
    if (health_gain_data.health_gain_type != HealthGainType::kInstantPureHeal)
    {
        health_gain = receiver_stats.Get(StatType::kHealthGainEfficiencyPercentage).AsPercentageOf(pre_health_gain);
    }
    else
    {
        health_gain = pre_health_gain;
    }

    // Modify for max_health_gain
    const FixedPoint max_health_gain =
        health_gain_data.max_health_percentage_to_health.AsPercentageOf(receiver_stats.Get(StatType::kMaxHealth));

    // Calculate new health
    const FixedPoint max_health = receiver_stats.Get(StatType::kMaxHealth);
    const FixedPoint current_health = receiver_stats.Get(StatType::kCurrentHealth);
    const FixedPoint new_health_unclamped = current_health + health_gain + max_health_gain;
    const FixedPoint new_health = std::min(new_health_unclamped, max_health);
    const FixedPoint delta = new_health - current_health;

    // Sets health based on enum case
    stats_component.SetCurrentHealth(new_health);

    // Send log and event if health changed
    if (delta > 0_fp)
    {
        LogDebug(
            receiver_id,
            "| ApplyHealthGain - gains {} health because {} [health_efficiency = {}, "
            "max_health_percentage_to_health = {}, max_health_gain = {}, new_health "
            "= {}, old_health = {}, new_health (clamped to max_health) = {}]",
            delta,
            log_health,
            receiver_stats.Get(StatType::kHealthGainEfficiencyPercentage),
            health_gain_data.max_health_percentage_to_health,
            max_health_gain,
            new_health,
            current_health,
            stats_component.GetCurrentHealth());

        // Send event
        world_->BuildAndEmitEvent<EventType::kHealthChanged>(health_gain_data.caused_by_id, receiver_id, delta);
    }

    // After health is adjusted see if we need to send the excess vamp to the shield
    if (health_gain_data.excess_vamp_to_shield)
    {
        const FixedPoint excess_amount = new_health_unclamped - receiver_stats.Get(StatType::kMaxHealth);
        if (excess_amount <= 0_fp)
        {
            return;
        }

        // Send excess health from vamp as a shield
        world_->BuildAndEmitEvent<EventType::kApplyShieldGain>(
            receiver_id,
            excess_amount,
            health_gain_data.excess_vamp_to_shield_duration_ms);
    }

    // After health is adjusted see if we need to send the excess heal to the shield
    if (health_gain_data.excess_heal_to_shield != 0_fp)
    {
        const FixedPoint excess_amount = new_health_unclamped - receiver_stats.Get(StatType::kMaxHealth);
        if (excess_amount <= 0_fp)
        {
            return;
        }

        // Send excess health from heal to the shield
        world_->BuildAndEmitEvent<EventType::kApplyShieldGain>(receiver_id, excess_amount, kTimeInfinite);
    }

    if (delta > 0_fp)
    {
        // Send event for UI in unreal
        event_data::HealthGain event_data;
        event_data.caused_by_id = health_gain_data.caused_by_id;
        event_data.caused_by_combat_unit_id = world_->GetCombatUnitParentID(health_gain_data.caused_by_id);
        event_data.entity_id = receiver_id;
        event_data.gain = delta;
        event_data.current_health = stats_component.GetCurrentHealth();
        event_data.health_gain_type = health_gain_data.health_gain_type;
        event_data.source_context = health_gain_data.source_context;

        world_->EmitEvent<EventType::kOnHealthGain>(event_data);
    }
}

}  // namespace simulation
