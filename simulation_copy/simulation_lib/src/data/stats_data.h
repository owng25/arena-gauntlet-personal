#pragma once

#include <array>
#include <cassert>

#include "data/constants.h"
#include "data/effect_enums.h"
#include "utility/enum_as_index.h"
#include "utility/enum_set.h"

namespace simulation
{
// Enum that describes every field of StatsData
enum class StatType
{
    kNone = 0,

    // Movement speed in units per time step
    // See PositionComponent and MovementComponent
    kMoveSpeedSubUnits,

    // Range of the attack abilities in units
    kAttackRangeUnits,

    // Range of the omega  abilities in units
    kOmegaRangeUnits,

    // Rate of attack abilities measured in attack completion percentage per second.
    // This stat can be affected by each Combat Units RSM (random stat modifier)
    //
    // NOTE: This could have been simply attacks per second but we can't use floats
    // so we just multiply by 100 and use the unity of percentage per second
    //
    // Example:
    // value = 50 - does an attack every 2000ms
    // value = 60 - does an attack every 1600ms
    // value = 100 - does an attack every 1000ms
    // value = 200 - does an attack every 500ms
    // value = 500 - does an attack every 200ms
    // value = 1000 - does an attack every 100ms
    //
    // Defaults: to an attack every second
    kAttackSpeed,

    // Percentage chance for attack abilities to be "on target".
    // On target attacks still need to be applied to the target, where they may be dodged or
    // mitigated. Range [0, 100]
    // NOTE: hit chance starts at 100%. This stat is only modified by debuffs or disables (blind).
    // Two 40% hit chance debuffs would be calculated as (100%-40%) * (100% - 40%) = 36% hit chance.
    // A dodge is a different thing than a miss. Some abilities can be triggered on dodge, while
    // others can be triggered on miss.
    kHitChancePercentage,

    // Percentage chance to dodge incoming attack abilities.
    // Range [0, 100]
    kAttackDodgeChancePercentage,

    // Virtual stat: kAttackPhysicalDamage + kAttackEnergyDamage + kAttackPureDamage
    kAttackDamage,

    // The Physical Damage of Attacks.
    kAttackPhysicalDamage,

    // The Energy Damage of Attacks.
    kAttackEnergyDamage,

    // The Pure Damage of Attacks.
    kAttackPureDamage,

    kMaxHealth,
    kCurrentHealth,

    // Gives a flat amount of Health per TimeStep
    kHealthRegeneration,

    // Percentage that is multiplied by healing values to determine final result,
    // e.g. health_gain_efficiency = 100%, 100% * 5(heal amount) = final result heal of 5
    kHealthGainEfficiencyPercentage,

    // Percentage of post_mitigated physical damage that heals the sender
    kPhysicalVampPercentage,

    // Percentage of post_mitigated pure damage that heals the sender
    kPureVampPercentage,

    // Percentage of post_mitigated energy damage that heals the sender
    kEnergyVampPercentage,

    // The energy cost to do an omega ability
    kEnergyCost,

    kStartingEnergy,
    kCurrentEnergy,

    // Gives a flat amount of Energy per TimeStep
    kEnergyRegeneration,

    // Energy efficiency is a multiplier to all energy gain. energy_gain_efficiency_percentage = 100
    // is (energy_gain * 1)
    kEnergyGainEfficiencyPercentage,

    // Shield gain efficiency is a multiplier to all shield gain
    kShieldGainEfficiencyPercentage,

    // On activation energy is energy a combat unit will receive after activation their omega
    // ability
    kOnActivationEnergy,

    // Determines the amount of Energy Resist to ignore when applying Energy damage.
    kEnergyPiercingPercentage,

    // Determines the amount of Armour to ignore when applying Energy damage.
    kPhysicalPiercingPercentage,

    // Determines what percentage of Energy Damage they take as damage.
    kEnergyResist,

    // Determines what percentage of Physical Damage they take as damage.
    // Example: 100 armour means it will only apply 50% of the physical damage
    kPhysicalResist,

    // Determines the duration of Negative States. Does not affect Displacements.
    kTenacityPercentage,

    // Determines the effectiveness of a Debuffs. Does not affect Buffs.
    // If willpower increases it means that the debuff applied value is less.
    // Range[0, 100]
    kWillpowerPercentage,

    // Reduces a discrete amount of Premitigated Physical Damage per EffectPackage.
    kGrit,

    // Reduces a discrete amount of Premitigated Energy Damage per EffectPackage.
    kResolve,

    // Reduces all pre mitigation damage by a flat percentage. It is effective against all Damage
    // Effect Types.
    kVulnerabilityPercentage,

    // Critical Strike chance percentage
    // By default this only applies to attack abilities.
    // Range[0, 100]
    kCritChancePercentage,

    // Critical Strike Damage percentage
    kCritAmplificationPercentage,

    // Modulates the base value of an effect
    kOmegaPowerPercentage,

    // Modulates the damage of omega damage
    kOmegaDamagePercentage,

    // Determines the amount of physical damage to return to the send when receiving damage from an
    // Attack Ability
    kThorns,

    // Starting Shield for the unit
    kStartingShield,

    // Used to determine the current hyper which is used for Combat Class & Combat Affinity bonuses
    kCurrentSubHyper,

    // Critical Strike Damage reduction percentage
    kCritReductionPercentage,

    // -new values can be added above this line
    kNum
};

ILLUVIUM_ENUM_AS_INDEX_IGNORE_FIRST_VALUE(StatType);

// Enum that describes how the StatType enum is evaluated
enum class StatEvaluationType
{
    kNone = 0,

    // Live = Base + Bonus
    kLive,

    // Base value of the stat
    kBase,

    // The debuff/bonus value
    kBonus,

    // -new values can be added above this line
    kNum
};

static constexpr std::array<StatType, 4> kRandomCombatStatTypes{
    StatType::kMaxHealth,
    StatType::kPhysicalResist,
    StatType::kEnergyResist,
    StatType::kAttackDamage,
    // TODO: Fix kAttackSpeed, can be random up to 25% which will be confusing to work with in
    // Unreal
    // StatType::kAttackSpeed,
};

// Required stat types for the suit
static constexpr std::array<StatType, 5> kSuitStatTypes{
    StatType::kEnergyResist,
    StatType::kPhysicalResist,
    StatType::kMaxHealth,
    StatType::kGrit,
    StatType::kResolve,
};

// Required stat types for the weapon
static constexpr std::array<StatType, 17> kWeaponStatTypes{
    StatType::kMaxHealth,
    StatType::kMoveSpeedSubUnits,
    StatType::kEnergyCost,
    StatType::kAttackPhysicalDamage,
    StatType::kAttackEnergyDamage,
    StatType::kAttackRangeUnits,
    StatType::kHitChancePercentage,
    StatType::kAttackSpeed,
    StatType::kEnergyPiercingPercentage,
    StatType::kPhysicalPiercingPercentage,
    StatType::kCritAmplificationPercentage,
    StatType::kCritChancePercentage,
    StatType::kPhysicalResist,
    StatType::kEnergyResist,
    StatType::kOnActivationEnergy,
    StatType::kStartingEnergy,
    StatType::kOmegaPowerPercentage};

// StatType::kAttackDamage is composed of all of these
static constexpr std::array<StatType, 3> kAttackDamageStatTypes{
    StatType::kAttackPhysicalDamage,
    StatType::kAttackEnergyDamage,
    StatType::kAttackPureDamage,
};

static constexpr std::array<StatType, 6> kHyperCombatClassBonusStatTypes{
    StatType::kAttackSpeed,
    StatType::kPhysicalResist,
    StatType::kEnergyResist,
    StatType::kCritChancePercentage,
    StatType::kOnActivationEnergy,
    StatType::kHealthRegeneration,
};

// All stats that are changed based on level
static constexpr std::array<StatType, 5> kLevelGrowthStatTypes{
    StatType::kMaxHealth,
    StatType::kAttackPhysicalDamage,
    StatType::kAttackEnergyDamage,
    StatType::kAttackPureDamage,
    StatType::kOmegaPowerPercentage,
};

// Keep track of some stats for some entities
struct StatsHistoryData final
{
public:
    void AddDamageReceived(const EffectDamageType damage_type, const FixedPoint value)
    {
        switch (damage_type)
        {
        case EffectDamageType::kPhysical:
            physical_damage_received += value;
            break;

        case EffectDamageType::kEnergy:
            energy_damage_received += value;
            break;

        case EffectDamageType::kPure:
        case EffectDamageType::kPurest:
        default:
            pure_damage_received += value;
            break;
        };

        total_damage_received += value;
    }

    void AddDamageSent(const EffectDamageType damage_type, const FixedPoint value)
    {
        switch (damage_type)
        {
        case EffectDamageType::kPhysical:
            physical_damage_sent += value;
            break;

        case EffectDamageType::kEnergy:
            energy_damage_sent += value;
            break;

        case EffectDamageType::kPure:
        case EffectDamageType::kPurest:
        default:
            pure_damage_sent += value;
            break;
        };

        total_damage_sent += value;
    }

    FixedPoint total_damage_received = 0_fp;
    FixedPoint physical_damage_received = 0_fp;
    FixedPoint energy_damage_received = 0_fp;
    FixedPoint pure_damage_received = 0_fp;

    FixedPoint total_damage_sent = 0_fp;
    FixedPoint physical_damage_sent = 0_fp;
    FixedPoint energy_damage_sent = 0_fp;
    FixedPoint pure_damage_sent = 0_fp;

    // You took down an enemy combat unit (aka combat unit fainted) and you are the vanquisher.
    int count_faint_vanquishes = 0;

    // You were part of the kill (you added damage, any detrimental effect)
    int count_faint_assists = 0;
};

// Holds the data for the RPG stats
struct StatsData final
{
    // Copyable or Movable
    constexpr StatsData() = default;
    constexpr StatsData(const StatsData&) = default;
    constexpr StatsData& operator=(const StatsData&) = default;
    constexpr StatsData(StatsData&&) = default;
    constexpr StatsData& operator=(StatsData&&) = default;

    // Returns current stat value by type.
    // Note: do not remove const qualifier from return type in this method,
    // otherwise it will be possible to do .Get(some_stat) = 42_fp
    // without any warning.
    constexpr FixedPoint Get(const StatType stat_type) const
    {
        switch (stat_type)
        {
        case StatType::kNone:
        case StatType::kNum:
            return 0_fp;

        default:
            return stats_[EnumToIndex(stat_type)];
        }
    }

    // Helper method that adds value_to_add to the StatType.
    // NOTE: This is just shorthand for, Set(type, Get() + value_to_add)
    void Add(const StatType stat_type, const FixedPoint& value)
    {
        Set(stat_type, Get(stat_type) + value);
    }

    // Sets value for the specified stat
    constexpr StatsData& Set(const StatType stat_type, const FixedPoint& value)
    {
        // Hooks
        FixedPoint& stat_ref = GetRef(stat_type);
        switch (stat_type)
        {
        case StatType::kNone:
        case StatType::kNum:
            // Ignore service values
            break;
        case StatType::kAttackDamage:
            // Can't set "virtual" stat
            break;
        case StatType::kOmegaRangeUnits:
        {
            const auto& attack_range_units = Get(StatType::kAttackRangeUnits);
            if (value < attack_range_units && value != 0_fp)
            {
                stat_ref = attack_range_units;
            }
            else
            {
                stat_ref = value;
            }
        }
        break;
        default:
            // Just assign by default
            stat_ref = value;
            break;
        }

        switch (stat_type)
        {
        case StatType::kAttackPhysicalDamage:
        case StatType::kAttackPureDamage:
        case StatType::kAttackEnergyDamage:
            GetRef(StatType::kAttackDamage) = Get(StatType::kAttackPhysicalDamage) +
                                              Get(StatType::kAttackEnergyDamage) + Get(StatType::kAttackPureDamage);
            break;
        default:
            break;
        }

        return *this;
    }

    // Resets all the metered stats
    constexpr void ResetMeteredStats()
    {
        Set(StatType::kCurrentEnergy, 0_fp);
        Set(StatType::kCurrentHealth, 0_fp);
        Set(StatType::kCurrentSubHyper, 0_fp);
    }

    // Initialize to the starting value all metered stats
    constexpr void InitializeMeteredStats()
    {
        Set(StatType::kCurrentEnergy, Get(StatType::kStartingEnergy));
        Set(StatType::kCurrentHealth, Get(StatType::kMaxHealth));
    }

    // Sets all the stats from another StatsData
    constexpr void SetAll(const StatsData& stats)
    {
        for (size_t index = 0; index != stats_.size(); ++index)
        {
            const auto stat_type = IndexToEnum<StatType>(index);
            Set(stat_type, stats.Get(stat_type));
        }

        InitializeMeteredStats();
    }

    constexpr void ClearAll()
    {
        for (size_t index = 0; index != stats_.size(); ++index)
        {
            const auto stat_type = IndexToEnum<StatType>(index);
            Set(stat_type, 0_fp);
        }

        InitializeMeteredStats();
    }

    constexpr FixedPoint GetMeteredStatCorrespondingMaxValue(const StatType type) const
    {
        switch (type)
        {
        case StatType::kCurrentHealth:
        {
            return Get(StatType::kMaxHealth);
        }
        case StatType::kCurrentEnergy:
        {
            return Get(StatType::kEnergyCost);
        }
        case StatType::kCurrentSubHyper:
        {
            return kMaxHyper;
        }
        default:
            return 0_fp;
        }
    }

    StatsData& operator+=(const StatsData& another)
    {
        for (const StatType stat_type : EnumSet<StatType>::MakeFull())
        {
            Add(stat_type, another.Get(stat_type));
        }

        return *this;
    }

    StatsData operator+(const StatsData& another) const
    {
        StatsData r = *this;
        r += another;
        return r;
    }

private:
    // This getter is private because set may have different behaviour for different stats
    // so we don't want these side effects to by avoided by taking a reference to stat and
    // modifying it outside of this class control
    constexpr FixedPoint& GetRef(const StatType stat_type)
    {
        return stats_[EnumToIndex(stat_type)];
    }

    // As we implement EnumAsIndex interface for StatType, we can place all stats into array and convert stat
    // type to an index to access them. But since we start indexing stats from first valid stat (not kNone),
    // we have to check stat_type != StatType::kNone (or StatType::kNum) before calling EnumToIndex
    std::array<FixedPoint, GetEnumEntriesCount<StatType>()> stats_;
};

// Helper struct that contains the base and live stats (which we can infer the bonus stats).
// Allows us to pass these two structs as one field.
struct FullStatsData
{
    StatsData base;
    StatsData live;

    // Which would also allow us to utilize some code here
    FixedPoint Get(const StatType stat, const StatEvaluationType stat_evaluation_type) const
    {
        switch (stat_evaluation_type)
        {
        case StatEvaluationType::kBase:
            return base.Get(stat);

        case StatEvaluationType::kLive:
            return live.Get(stat);

        case StatEvaluationType::kBonus:
            // Bonus is just the difference between live and base
            return live.Get(stat) - base.Get(stat);

        default:
            assert(false);
            return 0_fp;
        }
    }
};

}  // namespace simulation
