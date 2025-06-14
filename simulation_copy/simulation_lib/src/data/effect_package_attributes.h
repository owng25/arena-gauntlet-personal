#pragma once

#include <memory>

#include "data/effect_enums.h"
#include "data/effect_expression.h"
#include "data/enums.h"
#include "source_context.h"

namespace simulation
{

struct EffectPackage;

// Hold only attributes specific for effect propagation
class EffectPackagePropagationAttributes
{
public:
    static constexpr AllegianceType kChainDefaultTargetingType = AllegianceType::kEnemy;

    EffectPackagePropagationAttributes CreateDeepCopy() const;

    constexpr bool IsEmpty() const
    {
        return type == EffectPropagationType::kNone && !splash_radius_units && chain_number == 1 &&
               chain_delay_ms == 0 && chain_bounce_max_distance_units == 0 && !prioritize_new_targets &&
               !only_new_targets && targeting_group == kChainDefaultTargetingType &&
               !ignore_first_propagation_receiver && !add_original_effect_package && !effect_package;
    }

    EffectPackagePropagationAttributes& operator-=(const EffectPackagePropagationAttributes&)
    {
        // not compatible with disempower
        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/245465089/Disempower#List-of-Effect-Package-Attributes-Not-Compatible-With-Disempowers
        return *this;
    }

    EffectPackagePropagationAttributes& operator+=(const EffectPackagePropagationAttributes& rhs);

    void FormatTo(fmt::format_context& ctx) const;

    EffectPropagationType type = EffectPropagationType::kNone;

    // Integer that determines how many times this will chain. Keep in mind that the original skill
    // that specified the Propagation of chain does not count. (i.e. If you have a projectile with
    // propagation of chain, and a Chain Number of 1, when that projectile reached the target, it
    // will bounce one more time.
    //
    // Used if the propagation type is EffectPropagationType::kChain
    int chain_number = 1;

    // The time in steps between the hit from the first Effect Package and when we begin Deployment
    // to the next Receiver.
    //
    // Used if the propagation type is EffectPropagationType::kChain
    int chain_delay_ms = 0;

    // Furthest distance a chain entity may bounce from the intended target
    //
    // Used if the propagation type is EffectPropagationType::kChain
    int chain_bounce_max_distance_units = 0;

    // Determines the radius in hex tiles of the splash
    //
    // Used if the propagation type is EffectPropagationType::kSplash
    int splash_radius_units = 0;

    // Delay before each chain skill is actually deployed
    //
    // Used if the propagation type is EffectPropagationType::kChain
    int pre_deployment_delay_percentage = 0;

    // Keep a list of targets that we have already bounced to, and prioritise other ones.
    //
    // Used if the propagation type is EffectPropagationType::kChain
    bool prioritize_new_targets = false;

    // Keep a list of targets that we have already bounced to, and never bounce to them again.
    //
    // Used if propagation type is EffectPropagationType::kChain
    bool only_new_targets = false;

    // What entities this skill is targeting, only used if targeting_type is
    // different than current or self.
    //
    // Used if propagation type is EffectPropagationType::kChain
    AllegianceType targeting_group = kChainDefaultTargetingType;

    // If true, we exclude the first propagation receiver from the target list
    bool ignore_first_propagation_receiver = false;

    // Optional attribute.
    // If this is set to TRUE, the Effect Package of the original Skill will be added to the one
    // specified in the propagation. This allows for an easy way to keep the propagation consistent,
    // while still providing the flexibility to add or change the Effect Type.
    //
    // Note: If you wanted to only use the original Effect Package you would set this to TRUE and
    // then specify NO Effect Package in the Propagation.
    bool add_original_effect_package = false;

    // Optional attribute.
    // If true, we should not apply the original effect after creating propagation entity
    //
    // Note: For example if this propagation was created using Empower, the original Effect Package can be skipped
    // if set this property to true. And can be included to propagation effects if add_original_effect_package is true.
    bool skip_original_effect_package = false;

    // Whether propagation skill can deploy to ground, airborne or both, defaults to Ground.
    EnumSet<GuidanceType> deployment_guidance = MakeEnumSet(GuidanceType::kGround);

    // Whether propagation skill can target ground, airborne or both, defaults to Ground.
    EnumSet<GuidanceType> targeting_guidance = MakeEnumSet(GuidanceType::kGround);

    // Specifies an effect package that will be added during skill propagation
    // Used if `propagation_type` is not EffectPropagationType::kNone
    std::shared_ptr<EffectPackage> effect_package;

    // The source of the propagation effect
    SourceContextData source_context;
};

// Effect package attributes
// https://illuvium.atlassian.net/wiki/spaces/AB/pages/236718344/Effect+Package+Attribute
class EffectPackageAttributes
{
public:
    EffectPackageAttributes CreateDeepCopy() const;

    // Empower EffectPackageAttributes
    // See https://illuvium.atlassian.net/wiki/spaces/AB/pages/44205000/Empower+Effect
    EffectPackageAttributes& operator+=(const EffectPackageAttributes& rhs)
    {
        EmpowerIgnorePropagation(rhs);
        propagation += rhs.propagation;
        return *this;
    }

    EffectPackageAttributes& EmpowerIgnorePropagation(const EffectPackageAttributes& rhs)
    {
        is_trueshot = is_trueshot || rhs.is_trueshot;
        cumulative_damage = cumulative_damage || rhs.cumulative_damage;
        split_damage = split_damage || rhs.split_damage;
        can_crit = can_crit || rhs.can_crit;
        always_crit = always_crit || rhs.always_crit;
        rotate_to_target = rotate_to_target || rhs.rotate_to_target;
        excess_vamp_to_shield = excess_vamp_to_shield || rhs.excess_vamp_to_shield;
        exploit_weakness = exploit_weakness || rhs.exploit_weakness;
        refund_health += rhs.refund_health;
        refund_energy += rhs.refund_energy;
        damage_bonus += rhs.damage_bonus;
        energy_damage_bonus += rhs.energy_damage_bonus;
        physical_damage_bonus += rhs.physical_damage_bonus;
        pure_damage_bonus += rhs.pure_damage_bonus;
        damage_amplification += rhs.damage_amplification;
        physical_damage_amplification += rhs.physical_damage_amplification;
        energy_damage_amplification += rhs.energy_damage_amplification;
        pure_damage_amplification += rhs.pure_damage_amplification;
        energy_gain_bonus += rhs.energy_gain_bonus;
        energy_gain_amplification += rhs.energy_gain_amplification;
        heal_bonus += rhs.heal_bonus;
        heal_amplification += rhs.heal_amplification;
        energy_burn_bonus += rhs.energy_burn_bonus;
        energy_burn_amplification += rhs.energy_burn_amplification;
        piercing_percentage += rhs.piercing_percentage;
        physical_piercing_percentage += rhs.physical_piercing_percentage;
        energy_piercing_percentage += rhs.energy_piercing_percentage;
        crit_reduction_piercing_percentage += rhs.crit_reduction_piercing_percentage;
        shield_amplification += rhs.shield_amplification;
        shield_bonus += rhs.shield_bonus;
        vampiric_percentage += rhs.vampiric_percentage;

        if (excess_vamp_to_shield_duration_ms == kTimeInfinite ||
            rhs.excess_vamp_to_shield_duration_ms == kTimeInfinite)
        {
            excess_vamp_to_shield_duration_ms = kTimeInfinite;
        }
        else
        {
            excess_vamp_to_shield_duration_ms += rhs.excess_vamp_to_shield_duration_ms;
        }

        return *this;
    }

    // Disempower EffectPackageAttributes
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/245465089/Disempower
    EffectPackageAttributes& operator-=(const EffectPackageAttributes& rhs)
    {
        // NOTE: bools are not subtracted
        is_trueshot = is_trueshot || rhs.is_trueshot;
        cumulative_damage = cumulative_damage || rhs.cumulative_damage;
        split_damage = split_damage || rhs.split_damage;
        can_crit = can_crit || rhs.can_crit;
        always_crit = always_crit || rhs.always_crit;
        rotate_to_target = rotate_to_target || rhs.rotate_to_target;
        excess_vamp_to_shield = excess_vamp_to_shield || rhs.excess_vamp_to_shield;
        exploit_weakness = exploit_weakness || rhs.exploit_weakness;

        refund_health -= rhs.refund_health;
        refund_energy -= rhs.refund_energy;
        damage_bonus -= rhs.damage_bonus;
        energy_damage_bonus -= rhs.energy_damage_bonus;
        physical_damage_bonus -= rhs.physical_damage_bonus;
        pure_damage_bonus -= rhs.pure_damage_bonus;
        damage_amplification -= rhs.damage_amplification;
        physical_damage_amplification -= rhs.physical_damage_amplification;
        energy_damage_amplification -= rhs.energy_damage_amplification;
        pure_damage_amplification -= rhs.pure_damage_amplification;
        energy_gain_bonus -= rhs.energy_gain_bonus;
        energy_gain_amplification -= rhs.energy_gain_amplification;
        heal_bonus -= rhs.heal_bonus;
        heal_amplification -= rhs.heal_amplification;
        energy_burn_bonus -= rhs.energy_burn_bonus;
        energy_burn_amplification -= rhs.energy_burn_amplification;
        piercing_percentage -= rhs.piercing_percentage;
        physical_piercing_percentage -= rhs.physical_piercing_percentage;
        energy_piercing_percentage -= rhs.energy_piercing_percentage;
        crit_reduction_piercing_percentage -= rhs.crit_reduction_piercing_percentage;
        shield_amplification -= rhs.shield_amplification;
        shield_bonus -= rhs.shield_bonus;
        vampiric_percentage -= rhs.vampiric_percentage;

        if (rhs.excess_vamp_to_shield_duration_ms == kTimeInfinite)
        {
            excess_vamp_to_shield_duration_ms = 0;
        }
        else if (excess_vamp_to_shield_duration_ms != kTimeInfinite)
        {
            excess_vamp_to_shield_duration_ms -= rhs.excess_vamp_to_shield_duration_ms;
            // Prevent from becoming negative
            excess_vamp_to_shield_duration_ms = (std::max)(excess_vamp_to_shield_duration_ms, 0);
        }

        propagation -= rhs.propagation;

        return *this;
    }

    constexpr bool IsEmpty() const
    {
        return !is_trueshot && !can_crit && !always_crit && !rotate_to_target && !cumulative_damage && !split_damage &&
               !excess_vamp_to_shield && !exploit_weakness && refund_health.IsEmpty() && refund_energy.IsEmpty() &&
               physical_damage_bonus.IsEmpty() && energy_damage_bonus.IsEmpty() && pure_damage_bonus.IsEmpty() &&
               physical_damage_amplification.IsEmpty() && energy_damage_amplification.IsEmpty() &&
               pure_damage_amplification.IsEmpty() && energy_gain_bonus.IsEmpty() &&
               energy_gain_amplification.IsEmpty() && heal_bonus.IsEmpty() && heal_amplification.IsEmpty() &&
               energy_burn_bonus.IsEmpty() && energy_burn_amplification.IsEmpty() && damage_bonus.IsEmpty() &&
               damage_amplification.IsEmpty() && piercing_percentage.IsEmpty() &&
               physical_piercing_percentage.IsEmpty() && energy_piercing_percentage.IsEmpty() &&
               crit_reduction_piercing_percentage.IsEmpty() && shield_amplification.IsEmpty() &&
               shield_bonus.IsEmpty() && vampiric_percentage.IsEmpty() && excess_vamp_to_shield_duration_ms == 0 &&
               propagation.IsEmpty();
    }

    // Helper functions
    FixedPoint GetDamageBonusForDamageType(
        const ExpressionEvaluationContext& context,
        const EffectDamageType damage_type,
        const FullStatsData& sender_stats) const
    {
        ExpressionStatsSource stats_source{};
        stats_source.Set(ExpressionDataSourceType::kSender, sender_stats, nullptr);

        switch (damage_type)
        {
        case EffectDamageType::kPhysical:
            return damage_bonus.Evaluate(context, stats_source) + physical_damage_bonus.Evaluate(context, stats_source);
        case EffectDamageType::kEnergy:
            return damage_bonus.Evaluate(context, stats_source) + energy_damage_bonus.Evaluate(context, stats_source);
        case EffectDamageType::kPure:
            return damage_bonus.Evaluate(context, stats_source) + pure_damage_bonus.Evaluate(context, stats_source);
        default:
            return 0_fp;
        }
    }
    FixedPoint GetDamageAmplificationForDamageType(
        const ExpressionEvaluationContext& context,
        const EffectDamageType damage_type,
        const FullStatsData& sender_stats) const
    {
        ExpressionStatsSource stats_source{};
        stats_source.Set(ExpressionDataSourceType::kSender, sender_stats, nullptr);

        switch (damage_type)
        {
        case EffectDamageType::kPhysical:
            return damage_amplification.Evaluate(context, stats_source) +
                   physical_damage_amplification.Evaluate(context, stats_source);
        case EffectDamageType::kEnergy:
            return damage_amplification.Evaluate(context, stats_source) +
                   energy_damage_amplification.Evaluate(context, stats_source);
        case EffectDamageType::kPure:
            return damage_amplification.Evaluate(context, stats_source) +
                   pure_damage_amplification.Evaluate(context, stats_source);
        default:
            return 0_fp;
        }
    }

    void FormatTo(fmt::format_context& ctx) const;

    // Should this effect package ignore the dodge chance and can't be dodged
    bool is_trueshot = false;

    // Is true then if targeting num < actual_targets the extra effect packages will all go to the same target
    // Mutually excluse to split_damage
    bool cumulative_damage = false;

    // If true then if targeting num < actual_targets the extra effect packages will cycle between available targets
    // Mutually excluse to cumulative_damage
    bool split_damage = false;

    // If this is true, the will roll the crit chance to see if it is a critical hit and if true it
    // will multiply the damage by the crit_amplification attribute.
    bool can_crit = false;

    // If this is true it changes the crit chance to 100 (the maximum) for the duration
    // of this effect.
    bool always_crit = false;

    // Specifies whether an effect package could miss
    bool use_hit_chance = false;

    // Indicates to the visualization that it should rotate to the target
    bool rotate_to_target = false;

    // Effect propagation related attributes
    EffectPackagePropagationAttributes propagation;

    // Optional attribute.
    // Boolean for when the excess vamp should be converted to shield.
    bool excess_vamp_to_shield = false;

    // Optional attribute.
    // Adds an exploit_weakness to damage calculation = pre_mitigation * 100% + % of missing health
    bool exploit_weakness = false;

    // How much health should return to the sender after deploying the effect package.
    EffectExpression refund_health;

    // How much energy should return to the sender after deploying the effect package.
    EffectExpression refund_energy;

    // Additional damage of any type to the effect package
    EffectExpression damage_bonus;

    // Additional physical damage to the effect package
    EffectExpression physical_damage_bonus;

    // Additional energy damage to the effect package
    EffectExpression energy_damage_bonus;

    // Adds additional pure damage to the effect package
    EffectExpression pure_damage_bonus;

    // Adds additional damage of any type based on a percentage to the effect package
    EffectExpression damage_amplification;

    // Adds additional physical damage based on a percentage to the effect package
    EffectExpression physical_damage_amplification;

    // Adds additional energy damage based on a percentage to the effect package
    EffectExpression energy_damage_amplification;

    // Adds additional pure damage based on a percentage to the effect package
    EffectExpression pure_damage_amplification;

    // How much bonus energy should be gained
    EffectExpression energy_gain_bonus;

    // How much amplified energy should be gained
    EffectExpression energy_gain_amplification;

    // Adds additional heal to the effect package
    EffectExpression heal_bonus;

    // Adds additional heal based on a percentage to the effect package
    EffectExpression heal_amplification;

    // Adds additional energy burn to the effect package
    EffectExpression energy_burn_bonus;

    // Amplifies additional energy burn to the effect package
    EffectExpression energy_burn_amplification;

    // Adds piercing percentage to the effect package
    EffectExpression piercing_percentage;

    // Adds physical piercing percentage to the effect package
    EffectExpression physical_piercing_percentage;

    // Adds energy piercing percentage to the effect package
    EffectExpression energy_piercing_percentage;

    // Determines an additional amount of crit reduction to ignore.
    EffectExpression crit_reduction_piercing_percentage;

    // Adds bonus shield to effect package
    EffectExpression shield_bonus;

    // Adds additional shield to the effect package based on a percentage
    EffectExpression shield_amplification;

    // Heals the sender of an effect for a % of post mitigated damage.
    EffectExpression vampiric_percentage;

    // Duration for how long the excess_vamp_to_shield should last.
    int excess_vamp_to_shield_duration_ms = 0;
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(EffectPackagePropagationAttributes, FormatTo);
ILLUVIUM_MAKE_STRUCT_FORMATTER(EffectPackageAttributes, FormatTo);
