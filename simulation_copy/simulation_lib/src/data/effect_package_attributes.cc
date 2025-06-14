#include "effect_package_attributes.h"

#include "utility/enum.h"
#include "utility/string.h"
#include "utility/struct_formatting_helper.h"

namespace simulation
{
EffectPackagePropagationAttributes EffectPackagePropagationAttributes::CreateDeepCopy() const
{
    EffectPackagePropagationAttributes copy{*this};

    if (effect_package)
    {
        copy.effect_package = std::make_shared<EffectPackage>(effect_package->CreateDeepCopy());
    }

    return copy;
}

EffectPackagePropagationAttributes& EffectPackagePropagationAttributes::operator+=(
    const EffectPackagePropagationAttributes& rhs)
{
    const bool self_is_none = type == EffectPropagationType::kNone;
    const bool another_is_none = rhs.type == EffectPropagationType::kNone;
    const bool same_type = type == rhs.type;

    // Always skip if another is none
    // Otherwise skip if self is has propagation but type does not match with another object
    if (another_is_none || !(self_is_none || same_type))
    {
        return *this;
    }

    if (effect_package == nullptr)
    {
        effect_package = std::make_shared<EffectPackage>();
    }

    type = rhs.type;
    chain_number += rhs.chain_number;
    chain_delay_ms += rhs.chain_delay_ms;
    chain_bounce_max_distance_units += rhs.chain_bounce_max_distance_units;
    splash_radius_units += rhs.splash_radius_units;
    prioritize_new_targets |= rhs.prioritize_new_targets;
    only_new_targets |= rhs.only_new_targets;
    ignore_first_propagation_receiver |= rhs.ignore_first_propagation_receiver;
    add_original_effect_package |= rhs.add_original_effect_package;
    skip_original_effect_package |= rhs.skip_original_effect_package;
    effect_package->attributes += rhs.effect_package->attributes;
    effect_package->effects = rhs.effect_package->effects;

    // select non-default option but prefer previous value
    if (targeting_group == kChainDefaultTargetingType)
    {
        targeting_group = rhs.targeting_group;
        assert(targeting_group != AllegianceType::kNone);
    }

    source_context = rhs.source_context;

    return *this;
}

void EffectPackagePropagationAttributes::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("type", type);

    h.FieldIf("ignore_first_propagation_receiver", ignore_first_propagation_receiver);
    h.FieldIf("add_original_effect_package", add_original_effect_package);
    h.FieldIf("skip_original_effect_package", skip_original_effect_package);

    if (!(effect_package == nullptr || effect_package->IsEmpty()))
    {
        // TODO(konstantin): implement it?
    }

    switch (type)
    {
    case simulation::EffectPropagationType::kChain:
        h.Field("chain_number", chain_number);
        h.FieldIf("chain_delay_ms", chain_delay_ms);
        h.FieldIf("chain_bounce_max_distance_units", chain_bounce_max_distance_units);
        h.FieldIf("prioritize_new_targets", prioritize_new_targets);
        h.FieldIf("only_new_targets", only_new_targets);
        break;
    default:
        break;
    }

    h.Write("}}");
}

EffectPackageAttributes EffectPackageAttributes::CreateDeepCopy() const
{
    EffectPackageAttributes copy{*this};
    copy.propagation = propagation.CreateDeepCopy();
    return copy;
}

void EffectPackageAttributes::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.FieldIf("excess_vamp_to_shield", excess_vamp_to_shield);
    h.FieldIf("exploit_weakness", exploit_weakness);
    h.FieldIf("is_trueshot", is_trueshot);
    h.FieldIf("cumulative_damage", cumulative_damage);
    h.FieldIf("split_damage", split_damage);
    h.FieldIf("can_crit", can_crit);
    h.FieldIf("always_crit", always_crit);
    h.FieldIf("rotate_to_target", rotate_to_target);
    h.FieldIf(!refund_health.IsEmpty(), "refund_health", refund_health);
    h.FieldIf(!refund_energy.IsEmpty(), "refund_energy", refund_energy);
    h.FieldIf(!damage_bonus.IsEmpty(), "damage_bonus", damage_bonus);
    h.FieldIf(!damage_amplification.IsEmpty(), "damage_amplification", damage_amplification);
    h.FieldIf(!physical_damage_bonus.IsEmpty(), "physical_damage_bonus", physical_damage_bonus);
    h.FieldIf(!physical_damage_amplification.IsEmpty(), "physical_damage_amplification", physical_damage_amplification);
    h.FieldIf(!energy_damage_bonus.IsEmpty(), "energy_damage_bonus", energy_damage_bonus);
    h.FieldIf(!energy_damage_amplification.IsEmpty(), "energy_damage_amplification", energy_damage_amplification);
    h.FieldIf(!pure_damage_bonus.IsEmpty(), "pure_damage_bonus", pure_damage_bonus);
    h.FieldIf(!pure_damage_amplification.IsEmpty(), "pure_damage_amplification", pure_damage_amplification);
    h.FieldIf(!energy_gain_bonus.IsEmpty(), "energy_gain_bonus", energy_gain_bonus);
    h.FieldIf(!energy_gain_amplification.IsEmpty(), "energy_gain_amplification", energy_gain_amplification);
    h.FieldIf(!heal_bonus.IsEmpty(), "heal_bonus", heal_bonus);
    h.FieldIf(!heal_amplification.IsEmpty(), "heal_amplification", heal_amplification);
    h.FieldIf(!energy_burn_amplification.IsEmpty(), "energy_burn_amplification", energy_burn_amplification);
    h.FieldIf(!energy_burn_bonus.IsEmpty(), "energy_burn_bonus", energy_burn_bonus);
    h.FieldIf(!physical_piercing_percentage.IsEmpty(), "physical_piercing_percentage", physical_piercing_percentage);
    h.FieldIf(!energy_piercing_percentage.IsEmpty(), "energy_piercing_percentage", energy_piercing_percentage);
    h.FieldIf(
        !crit_reduction_piercing_percentage.IsEmpty(),
        "crit_reduction_piercing_percentage",
        crit_reduction_piercing_percentage);
    h.FieldIf(!piercing_percentage.IsEmpty(), "piercing_percentage", piercing_percentage);
    h.FieldIf(!shield_amplification.IsEmpty(), "shield_amplification", shield_amplification);
    h.FieldIf(!shield_bonus.IsEmpty(), "shield_bonus", shield_bonus);
    h.FieldIf(!vampiric_percentage.IsEmpty(), "vampiric_percentage", vampiric_percentage);
    h.FieldIf(excess_vamp_to_shield, "excess_vamp_to_shield_duration_ms", excess_vamp_to_shield_duration_ms);
    h.FieldIf(!propagation.IsEmpty(), "propagation", propagation);
    h.Write("}}");
}

}  // namespace simulation
