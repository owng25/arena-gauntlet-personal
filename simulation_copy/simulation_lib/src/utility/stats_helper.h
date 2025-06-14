#pragma once

#include "data/effect_enums.h"
#include "data/enums.h"
#include "data/stats_data.h"
#include "ecs/entity.h"
#include "utility/ensure_enum_size.h"
#include "utility/fixed_point.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * StatsHelper
 *
 * This defines the helper functions to handle the different stat types
 * --------------------------------------------------------------------------------------------------------
 */
class StatsHelper
{
public:
    // When adding new stat revisit arrays below
    ILLUVIUM_ENSURE_ENUM_SIZE(StatType, 42);

    // Standard Combat Stat
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/100335901/Standard+Combat+Stat
    static constexpr std::array<StatType, 36> kStandardStatTypes{
        StatType::kMaxHealth,
        StatType::kAttackPhysicalDamage,
        StatType::kAttackEnergyDamage,
        StatType::kAttackPureDamage,
        StatType::kAttackDamage,
        StatType::kEnergyResist,
        StatType::kPhysicalResist,
        StatType::kAttackRangeUnits,
        StatType::kOmegaRangeUnits,
        StatType::kResolve,
        StatType::kGrit,
        StatType::kStartingEnergy,
        StatType::kMoveSpeedSubUnits,
        StatType::kEnergyRegeneration,
        StatType::kHealthRegeneration,
        StatType::kThorns,
        StatType::kOmegaPowerPercentage,
        StatType::kOmegaDamagePercentage,
        StatType::kCritAmplificationPercentage,
        StatType::kEnergyPiercingPercentage,
        StatType::kPhysicalPiercingPercentage,
        StatType::kWillpowerPercentage,
        StatType::kTenacityPercentage,
        StatType::kHealthGainEfficiencyPercentage,
        StatType::kShieldGainEfficiencyPercentage,
        StatType::kEnergyGainEfficiencyPercentage,
        StatType::kHitChancePercentage,
        StatType::kPhysicalVampPercentage,
        StatType::kEnergyVampPercentage,
        StatType::kPureVampPercentage,
        StatType::kAttackDodgeChancePercentage,
        StatType::kCritChancePercentage,
        StatType::kAttackSpeed,
        StatType::kOnActivationEnergy,
        StatType::kStartingShield,
        StatType::kCritReductionPercentage};

    // Negated Combat Stat
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/206569663/Negated+Combat+Stat
    static constexpr std::array<StatType, 2> kNegatedStatTypes{
        StatType::kEnergyCost,
        StatType::kVulnerabilityPercentage};

    // Percentage Combat Stat
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/272728087/Percentage+Combat+Stat
    static constexpr std::array<StatType, 18> kPercentageStatTypes{
        StatType::kOmegaPowerPercentage,
        StatType::kOmegaDamagePercentage,
        StatType::kCritAmplificationPercentage,
        StatType::kPhysicalPiercingPercentage,
        StatType::kEnergyPiercingPercentage,
        StatType::kHitChancePercentage,
        StatType::kAttackDodgeChancePercentage,
        StatType::kEnergyGainEfficiencyPercentage,
        StatType::kShieldGainEfficiencyPercentage,
        StatType::kHealthGainEfficiencyPercentage,
        StatType::kPhysicalVampPercentage,
        StatType::kEnergyVampPercentage,
        StatType::kPureVampPercentage,
        StatType::kTenacityPercentage,
        StatType::kWillpowerPercentage,
        StatType::kVulnerabilityPercentage,
        StatType::kCritChancePercentage,
        StatType::kCritReductionPercentage};

    // All the stat types we should clamp in the interval [kMinPercentage, kMaxPercentage]
    static constexpr std::array<StatType, 11> kStatPercentageTypesToClamp{
        StatType::kWillpowerPercentage,
        StatType::kTenacityPercentage,
        StatType::kEnergyVampPercentage,
        StatType::kPureVampPercentage,
        StatType::kPhysicalVampPercentage,
        StatType::kCritChancePercentage,
        StatType::kHitChancePercentage,
        StatType::kAttackDodgeChancePercentage,
        StatType::kEnergyPiercingPercentage,
        StatType::kPhysicalPiercingPercentage,
        StatType::kCritReductionPercentage,
    };

    // Standard Combat Stat
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/100335901/Standard+Combat+Stat
    static constexpr bool IsStandardStatType(const StatType stat)
    {
        for (const StatType stat_type : kStandardStatTypes)
        {
            if (stat == stat_type)
            {
                return true;
            }
        }
        return false;
    }

    // Computes damage amplification for attack abilities if
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44270345/Attack+Speed#Max
    static FixedPoint GetDamageAmplificationForAbilityType(
        const AbilityType ability_type,
        const FullStatsData& sender_stats,
        const FixedPoint max_attack_speed)
    {
        if (ability_type == AbilityType::kAttack)
        {
            const auto attack_speed = sender_stats.Get(StatType::kAttackSpeed, StatEvaluationType::kLive);
            if (attack_speed > max_attack_speed)
            {
                return (kMaxPercentageFP * attack_speed) / max_attack_speed - kMaxPercentageFP;
            }
        }
        if (ability_type == AbilityType::kOmega)
        {
            // Scale damage for omega sourced damages
            const FixedPoint omega_damage_percentage =
                sender_stats.Get(StatType::kOmegaDamagePercentage, StatEvaluationType::kLive);
            if (omega_damage_percentage != kMaxPercentageFP)
            {
                return omega_damage_percentage - kMaxPercentageFP;
            }
        }
        return 0_fp;
    }

    // Negated Combat Stat
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/206569663/Negated+Combat+Stat
    static constexpr bool IsNegatedStatType(const StatType stat)
    {
        for (const StatType stat_type : kNegatedStatTypes)
        {
            if (stat == stat_type)
            {
                return true;
            }
        }
        return false;
    }

    // Percentage Combat Stat
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/272728087/Percentage+Combat+Stat
    static constexpr bool IsPercentageType(const StatType stat)
    {
        for (const StatType stat_type : kPercentageStatTypes)
        {
            if (stat == stat_type)
            {
                return true;
            }
        }
        return false;
    }

    // Same as IsPercentageType but for some stat types we consider some stats to be percentages inside expressions
    static constexpr bool IsPercentageTypeForExpression(const StatType stat)
    {
        return IsPercentageType(stat) || stat == StatType::kAttackSpeed || stat == StatType::kPhysicalResist ||
               stat == StatType::kEnergyResist;
    }

    static constexpr bool IsPercentageTypeToClamp(const StatType stat)
    {
        for (const StatType stat_type : kStatPercentageTypesToClamp)
        {
            if (stat == stat_type)
            {
                return true;
            }
        }
        return false;
    }

    // Check what stats to ignore when reading from JSON
    static constexpr bool IsStatIgnoredForReading(const StatType type)
    {
        return IsMeteredStatType(type) || type == StatType::kAttackDamage;
    }

    // Return default value for stat if exist
    static void SetDefaults(StatsData& stats_data);
    static bool GetDefaultStatValue(const StatType type, FixedPoint* out_value);

    static constexpr bool IsMeteredStatType(const StatType type)
    {
        return type == StatType::kCurrentHealth || type == StatType::kCurrentEnergy ||
               type == StatType::kCurrentSubHyper;
    }

    // Converts a StatType Damage Type to an EffectDamageType
    static constexpr EffectDamageType StatDamageTypeToEffectDamageType(const StatType stat_type)
    {
        switch (stat_type)
        {
        case StatType::kAttackPhysicalDamage:
        {
            return EffectDamageType::kPhysical;
        }
        case StatType::kAttackEnergyDamage:
        {
            return EffectDamageType::kEnergy;
        }
        case StatType::kAttackPureDamage:
        {
            return EffectDamageType::kPure;
        }
        default:
            return EffectDamageType::kNone;
        }
    }

    // Converts a EffectDamageType to a StatType
    static constexpr StatType EffectDamageTypeToStatType(const EffectDamageType damage_type)
    {
        switch (damage_type)
        {
        case EffectDamageType::kPhysical:
        {
            return StatType::kAttackPhysicalDamage;
        }
        case EffectDamageType::kEnergy:
        {
            return StatType::kAttackEnergyDamage;
        }
        case EffectDamageType::kPure:
        {
            return StatType::kAttackPureDamage;
        }
        default:
            return StatType::kNone;
        }
    }

    // Time step the overflow values for this stat_type.
    // Return true if it overflowed and false otherwise.
    static bool TimeStepOverflowStat(const Entity& entity, const StatsData& live_stats, const StatType stat_type);

    // Create the random modifier stats from some template stats
    static StatsData CreateRandomModifierStats(World& world, const StatsData& stats);
};

}  // namespace simulation
