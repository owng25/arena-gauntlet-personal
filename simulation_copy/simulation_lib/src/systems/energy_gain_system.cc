#include "systems/energy_gain_system.h"

#include "components/stats_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "utility/entity_helper.h"
#include "utility/enum.h"
#include "utility/math.h"

namespace simulation
{
void EnergyGainSystem::Init(World* world)
{
    Super::Init(world);

    world_->SubscribeMethodToEvent<EventType::kApplyEnergyGain>(this, &Self::ApplyEnergyGain);
}

void EnergyGainSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    // Not active
    if (!entity.IsActive())
    {
        return;
    }
}

void EnergyGainSystem::ApplyEnergyGain(const event_data::ApplyEnergyGain& energy_gain_data)
{
    if (!world_->HasEntity(energy_gain_data.receiver_id))
    {
        LogDebug("| ApplyEnergyGain receiver_id = {} does not exist anymore. ABORTING", energy_gain_data.receiver_id);
        return;
    }

    // Only for combat units
    const auto& receiver_entity = world_->GetByID(energy_gain_data.receiver_id);
    if (!EntityHelper::IsACombatUnit(receiver_entity))
    {
        return;
    }

    const StatsData live_stats = world_->GetLiveStats(receiver_entity);
    LogDebug(
        energy_gain_data.receiver_id,
        "| ApplyEnergyGain - energy_gain_type = {}, value = {}, energy_gain_efficiency = {}",
        energy_gain_data.energy_gain_type,
        energy_gain_data.value,
        live_stats.Get(StatType::kEnergyGainEfficiencyPercentage));

    // Check for Lethargic effect
    if (!EntityHelper::CanGainEnergy(receiver_entity))
    {
        LogDebug(
            energy_gain_data.caused_by_id,
            "| ApplyEnergyGain - Rejected, can't gain energy  - receiver_id= {}",
            energy_gain_data.receiver_id);
        return;
    }

    auto& stats_component = receiver_entity.Get<StatsComponent>();

    FixedPoint pre_energy_gain = 0_fp;
    std::string log_energy = "";
    switch (energy_gain_data.energy_gain_type)
    {
    case EnergyGainType::kRegeneration:
    {
        pre_energy_gain = live_stats.Get(StatType::kEnergyRegeneration);
        log_energy = fmt::format("energy_regeneration = {}", live_stats.Get(StatType::kEnergyRegeneration));
        break;
    }
    case EnergyGainType::kOnTakeDamage:
    {
        pre_energy_gain = energy_gain_data.value / world_->GetOneEneryGainPerEffectiveDamage();
        log_energy =
            fmt::format("{} effective taken damage gives {} pre_energy_gain", energy_gain_data.value, pre_energy_gain);
        break;
    }
    case EnergyGainType::kAttack:
    {
        // Getter so EnergyGainPerAttack is scaled to the world

        // Add equipment attack speed because rangers attack speed comes from equipment
        const FixedPoint attack_speed = stats_component.GetTemplateStats().Get(StatType::kAttackSpeed) +
                                        stats_component.GetEquipmentStats().Get(StatType::kAttackSpeed);

        pre_energy_gain = Math::CalculateEnergyGain(world_->GetBaseEnergyGainPerAttack(), attack_speed);
        log_energy = "Attack";
        break;
    }
    case EnergyGainType::kOnActivation:
    {
        pre_energy_gain = live_stats.Get(StatType::kOnActivationEnergy);
        log_energy = fmt::format(
            "Omega Ability Activation, on_activation_energy = {}",
            live_stats.Get(StatType::kOnActivationEnergy));
        break;
    }
    case EnergyGainType::kRefund:
    {
        pre_energy_gain = energy_gain_data.value;
        log_energy = fmt::format("Refund Energy, refund_energy = {}", energy_gain_data.value);
        break;
    }
    default:
        break;
    }

    // Modify by efficiency
    const FixedPoint energy_gain =
        live_stats.Get(StatType::kEnergyGainEfficiencyPercentage).AsPercentageOf(pre_energy_gain);

    // Calculate new energy
    const FixedPoint current_energy = live_stats.Get(StatType::kCurrentEnergy);
    const FixedPoint new_energy = current_energy + energy_gain;
    const FixedPoint delta = new_energy - current_energy;

    // Sets energy based on enum case
    stats_component.SetCurrentEnergy(new_energy);

    if (delta != 0_fp)
    {
        LogDebug(
            energy_gain_data.receiver_id,
            "| - gains {} energy because of {} [new_energy = {}]",
            delta,
            log_energy,
            stats_component.GetCurrentEnergy());

        // Send event
        world_->BuildAndEmitEvent<EventType::kEnergyChanged>(
            energy_gain_data.caused_by_id,
            energy_gain_data.receiver_id,
            delta);
    }
}
}  // namespace simulation
