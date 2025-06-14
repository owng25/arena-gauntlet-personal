#pragma once

#include <algorithm>
#include <unordered_map>

#include "data/stats_data.h"
#include "ecs/component.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * StatsComponent
 *
 * This class holds the base stats for an entity.
 * Composed of the following stats:
 * - Template
 * - Random Stats
 * --------------------------------------------------------------------------------------------------------
 */
class StatsComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<StatsComponent>(*this);
    }

    constexpr void SetCombatUnitParentLiveStats(const StatsData& live_stats)
    {
        combat_unit_parent_live_stats_ = live_stats;
    }
    constexpr const StatsData& GetCombatUnitParentLiveStats() const
    {
        return combat_unit_parent_live_stats_;
    }

    constexpr void SetTemplateStats(const StatsData& stats)
    {
        template_stats_.SetAll(stats);
    }

    constexpr void SetRandomModifierStats(const StatsData& stats)
    {
        random_modifier_stats_.SetAll(stats);
        random_modifier_stats_.ResetMeteredStats();
    }

    constexpr void SetRandomPercentageModifierStats(const StatsData& stats)
    {
        random_percentage_modifier_stats_ = stats;
    }

    // Set all the data for the equipment
    void SetEquipmentStats(const StatsData& weapon_stats, const StatsData& suit_stats)
    {
        weapon_stats_ = weapon_stats;
        suit_stats_ = suit_stats;

        weapon_stats_.ResetMeteredStats();
        suit_stats_.ResetMeteredStats();

        total_equipment_stats_ = weapon_stats_ + suit_stats_;
    }

    const StatsData& GetEquipmentStats() const
    {
        return total_equipment_stats_;
    }

    // Get and Set level of illuvial/ranger
    constexpr void SetLevel(const int level)
    {
        level_ = level;
    }

    constexpr int GetLevel() const
    {
        return level_;
    }

    // Set level bonus to new values
    constexpr void SetLevelBonusStats(const StatsData& level_bonus_stats)
    {
        level_bonus_stats_ = level_bonus_stats;
    }

    // Compute the Base Stats
    // Base = Template + Random Stats
    StatsData GetBaseStats() const;

    constexpr const StatsData& GetTemplateStats() const
    {
        return template_stats_;
    }

    constexpr const StatsData& GetRandomModifierStats() const
    {
        return random_modifier_stats_;
    }

    constexpr const StatsData& GetRandomPercentageModifierStats() const
    {
        return random_percentage_modifier_stats_;
    }

    constexpr const StatsData& GetSuitStats() const
    {
        return suit_stats_;
    }

    constexpr const StatsData& GetWeaponStats() const
    {
        return weapon_stats_;
    }

    constexpr const StatsData& GetLevelBonusStats() const
    {
        return level_bonus_stats_;
    }

    // Use this if you want to modify the template stats for some reason (used in tests)
    constexpr StatsData& GetMutableTemplateStats()
    {
        return template_stats_;
    }

    // Gets sets the currents stats for health/shield/energy/hyper
    // We can use any stats, but we choose the template stats to contain the current global
    // health/energy
    constexpr FixedPoint GetCurrentHealth() const
    {
        return template_stats_.Get(StatType::kCurrentHealth);
    }
    constexpr FixedPoint GetCurrentEnergy() const
    {
        return template_stats_.Get(StatType::kCurrentEnergy);
    }
    constexpr FixedPoint GetCurrentSubHyper() const
    {
        return template_stats_.Get(StatType::kCurrentSubHyper);
    }

    FixedPoint GetAttackDamage() const;
    FixedPoint GetMaxHealth() const;
    FixedPoint GetEnergyCost() const;

    constexpr FixedPoint GetCritChanceOverflowPercentage() const
    {
        return crit_chance_overflow_percentage_;
    }
    constexpr FixedPoint GetAttackDodgeChanceOverflowPercentage() const
    {
        return attack_dodge_chance_overflow_percentage_;
    }
    constexpr FixedPoint GetHitChanceOverflowPercentage() const
    {
        return hit_chance_overflow_percentage_;
    }
    constexpr void SetCritChanceOverflowPercentage(const FixedPoint value)
    {
        crit_chance_overflow_percentage_ = value;
    }
    constexpr void SetAttackDodgeChanceOverflowPercentage(const FixedPoint value)
    {
        attack_dodge_chance_overflow_percentage_ = value;
    }
    constexpr void SetHitChanceOverflowPercentage(const FixedPoint value)
    {
        hit_chance_overflow_percentage_ = value;
    }

    // Sets the current overflow for this type
    constexpr void SetOverflowForType(const StatType type, const FixedPoint value)
    {
        switch (type)
        {
        case StatType::kCritChancePercentage:
            SetCritChanceOverflowPercentage(value);
            break;
        case StatType::kAttackDodgeChancePercentage:
            SetAttackDodgeChanceOverflowPercentage(value);
            break;
        case StatType::kHitChancePercentage:
            SetHitChanceOverflowPercentage(value);
            break;
        default:
            break;
        }
    }

    void OverrideBaseStat(const StatType stat_type, const FixedPoint value)
    {
        stats_overrides_[stat_type] = value;
    }

    void OverrideBaseStats(const std::unordered_map<StatType, FixedPoint>& stats)
    {
        for (const auto& [stat, value] : stats)
        {
            OverrideBaseStat(stat, value);
        }
    }

    const std::unordered_map<StatType, FixedPoint>& GetStatsOverrides() const
    {
        return stats_overrides_;
    }

    // Gets the current overflow value for this type
    constexpr FixedPoint GetOverflowForType(const StatType type) const
    {
        switch (type)
        {
        case StatType::kCritChancePercentage:
            return GetCritChanceOverflowPercentage();
        case StatType::kAttackDodgeChancePercentage:
            return GetAttackDodgeChanceOverflowPercentage();
        case StatType::kHitChancePercentage:
            return GetHitChanceOverflowPercentage();
        default:
            return 0_fp;
        }
    }

    // Helper to increase the overflow for a stat type
    constexpr void IncreaseOverflowForType(const StatType type, const FixedPoint increase_value)
    {
        const FixedPoint current_value = GetOverflowForType(type);
        SetOverflowForType(type, current_value + increase_value);
    }

    // Helper to decrease the overflow for a stat type
    constexpr void DecreaseOverflowForType(const StatType type, const FixedPoint decrease_value)
    {
        const FixedPoint current_value = GetOverflowForType(type);
        SetOverflowForType(type, current_value - decrease_value);
    }

    FixedPoint GetPreviousAttackSpeed() const
    {
        return previous_attack_speed;
    }

    void SetPreviousAttackSpeed(const FixedPoint value)
    {
        previous_attack_speed = value;
    }

    void SetCurrentHealth(const FixedPoint value)
    {
        template_stats_.Set(StatType::kCurrentHealth, std::clamp(value, 0_fp, GetMaxHealth()));
    }

    // Sets the current energy, ensures is in the range [0, energy_cost]
    void SetCurrentEnergy(const FixedPoint value)
    {
        template_stats_.Set(StatType::kCurrentEnergy, std::clamp(value, 0_fp, GetEnergyCost()));
    }

    // Sets the current sub hyper, required for hyper calculation
    constexpr void SetCurrentSubHyper(const FixedPoint value)
    {
        template_stats_.Set(StatType::kCurrentSubHyper, std::clamp(value, 0_fp, kMaxSubHyper));
    }

    // Has this unit gone hyperactive?
    constexpr bool IsHyperactive() const
    {
        return GetCurrentHyper() >= kMaxHyper;
    }

    // This value will be use to calculate leveling bonus
    FixedPoint GetBaseValueForTypeBeforeLeveling(const StatType type) const
    {
        FixedPoint template_stat_value = 0_fp;
        if (stats_overrides_.contains(type))
        {
            template_stat_value = stats_overrides_.at(type);
        }
        else
        {
            template_stat_value = template_stats_.Get(type);
        }

        FixedPoint stats_modifier = 0_fp;
        if (stats_modifiers_.contains(type))
        {
            stats_modifier = stats_modifiers_.at(type);
        }

        return template_stat_value + stats_modifier + random_modifier_stats_.Get(type) +
               total_equipment_stats_.Get(type);
    }

    // Gets the Base value for a stat:
    // Base = Template + Random Stats + Level Bonus Stats
    FixedPoint GetBaseValueForType(const StatType type) const
    {
        return GetBaseValueForTypeBeforeLeveling(type) + level_bonus_stats_.Get(type);
    }

    void SetAllStatsModifiers(std::unordered_map<StatType, FixedPoint>&& stats_modifiers)
    {
        stats_modifiers_ = std::move(stats_modifiers);
    }

    void SetStatModifier(StatType stat_type, const FixedPoint value)
    {
        stats_modifiers_[stat_type] = value;
    }

    void ClearStatModifiers()
    {
        stats_modifiers_.clear();
    }

    // Get Current Hyper based on sub hyper using the tanh formula
    constexpr FixedPoint GetCurrentHyper() const
    {
        // Scale down back to normal precision
        const FixedPoint current_hyper = GetCurrentSubHyper() / kPrecisionFactorFP;
        return (std::min)(current_hyper, kMaxHyper);
    }

    const StatsHistoryData& GetHistoryData() const
    {
        return stats_history_data_;
    }
    StatsHistoryData& GetMutableHistoryData()
    {
        return stats_history_data_;
    }

private:
    // The template combat stats, these should be pretty much immutable
    StatsData template_stats_{};

    // Each combat unit has a random stat modifier that determines how much their stats change from
    // the base. Only some attributes are changed by this system, as some should default to 0, or
    // are too volatile to be added to the list.
    StatsData random_modifier_stats_{};

    // The percentage stats used to compute random_modifier_stats_, keep track of this because consumers might need
    // it.
    StatsData random_percentage_modifier_stats_;

    // The total stats of the equipment
    StatsData total_equipment_stats_{};

    // Stats that are provided as a bonus from the suit equipped on the ranger
    StatsData suit_stats_{};

    // Stats that are provided as a bonus from weapon equipped on the ranger
    StatsData weapon_stats_{};

    // Stats that are provided for each level
    StatsData level_bonus_stats_{};

    // Last known data of the combat unit parent current stats (includes buffs/debuff)
    StatsData combat_unit_parent_live_stats_{};

    // Stores overrides for stats
    // Key: stat type
    // Value: value which overrides stat specified in the key
    std::unordered_map<StatType, FixedPoint> stats_overrides_;

    // Stores modifiers for stats
    // Key: stat type
    // Value: value which will be added to stat specified in the key
    std::unordered_map<StatType, FixedPoint> stats_modifiers_;

    // Level of the illuvial/ranger
    int level_ = 0;

    // Used for dodge chance overflow to determine dodge change
    FixedPoint attack_dodge_chance_overflow_percentage_ = 0_fp;

    // Used for crit chance overflow to determine crit
    FixedPoint crit_chance_overflow_percentage_ = 0_fp;

    // Used for hit_chance overflow to determine when an attack should hit
    FixedPoint hit_chance_overflow_percentage_ = 0_fp;

    // Used by ability system to calculate how fast the
    // timer should be incremented for the attacks
    FixedPoint previous_attack_speed = 0_fp;

    // Keep track of the history data
    StatsHistoryData stats_history_data_;
};

}  // namespace simulation
