#pragma once

#include "utility/custom_formatter.h"
#include "utility/fixed_point.h"

namespace simulation
{
// Effect Data attributes
// https://illuvium.atlassian.net/wiki/spaces/AB/pages/44204780/Effect+Attribute
struct EffectDataAttributes
{
    // Merge with another EffectPackageAttributes
    // See https://illuvium.atlassian.net/wiki/spaces/AB/pages/44205000/Empower+Effect
    EffectDataAttributes& operator+=(const EffectDataAttributes& rhs)
    {
        excess_heal_to_shield += rhs.excess_heal_to_shield;
        missing_health_percentage_to_health += rhs.missing_health_percentage_to_health;
        to_shield_percentage += rhs.to_shield_percentage;
        to_shield_duration_ms += rhs.to_shield_duration_ms;
        max_health_percentage_to_health += rhs.max_health_percentage_to_health;

        cleanse_negative_states = cleanse_negative_states || rhs.cleanse_negative_states;
        cleanse_conditions = cleanse_conditions || rhs.cleanse_conditions;
        cleanse_bots = cleanse_bots || rhs.cleanse_bots;
        cleanse_dots = cleanse_dots || rhs.cleanse_dots;
        cleanse_debuffs = cleanse_debuffs || rhs.cleanse_debuffs;
        shield_bypass = shield_bypass || rhs.shield_bypass;

        return *this;
    }

    constexpr bool IsEmpty() const
    {
        return excess_heal_to_shield != 0_fp && missing_health_percentage_to_health != 0_fp &&
               to_shield_percentage != 0_fp && !to_shield_duration_ms && max_health_percentage_to_health != 0_fp &&
               !cleanse_negative_states && !cleanse_conditions && !cleanse_bots && !cleanse_dots && !cleanse_debuffs &&
               !shield_bypass;
    }

    void FormatTo(fmt::format_context& ctx) const;

    // Optional attribute.
    // Integer value above 0 representing the max shield value to be gained from the health being,
    // In the case where the Amount - MissingHealth > 0.
    FixedPoint excess_heal_to_shield = 0_fp;

    // Optional attribute.
    // Integer value between 0 - 100 representing the percentage of missing health that gets added
    // to the Amount.
    FixedPoint missing_health_percentage_to_health = 0_fp;

    // Optional attribute.
    // Used when an effect provides the sender with a shield based on the percentage of post
    // mitigated physical damage.
    FixedPoint to_shield_percentage = 0_fp;

    // Optional attribute.
    // Duration for how long the shield should last.
    int to_shield_duration_ms = 0;

    // Optional attribute.
    // Integer value between 0 - 100 representing the percentage of max health that gets added to
    // the Amount.
    FixedPoint max_health_percentage_to_health = 0_fp;

    // Optional attribute.
    // Boolean value that says if the heal removes negative states from the recipient.
    bool cleanse_negative_states = false;

    // Optional attribute.
    // Boolean value that says if the heal removes conditions from the recipient.
    bool cleanse_conditions = false;

    // Optional attribute.
    // Boolean value that says if the heal removes energy burn over time from the recipient.
    bool cleanse_bots = false;

    // Optional attribute.
    // Boolean value that says if the heal removes Damage Over Time from the recipient.
    bool cleanse_dots = false;

    // Optional attribute.
    // Boolean value that says if the heal removes debuffs from the recipient.
    bool cleanse_debuffs = false;

    // Should the damage bypass the shield
    bool shield_bypass = false;
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(EffectDataAttributes, FormatTo);
