#pragma once

#include "data/effect_enums.h"
#include "data/stats_data.h"
#include "utility/custom_formatter.h"

namespace simulation
{
// Helper struct to uniquely identify an effect data type
struct EffectTypeID
{
    // Type of this effect
    // Tells us what effect package we will create
    EffectType type = EffectType::kNone;

    // Positive state type if any
    // Used if the effect type is EffectType::kPositiveState
    EffectPositiveState positive_state = EffectPositiveState::kNone;

    // Negative state type if any
    // Used if the effect type is EffectType::kNegativeState
    EffectNegativeState negative_state = EffectNegativeState::kNone;

    // Plane change type if any
    // Used if the effect type is EffectType::kPlaneChange
    EffectPlaneChange plane_change = EffectPlaneChange::kNone;

    // Type of predefined condition for this effect
    // Only used if the type is EffectType::kCondition
    EffectConditionType condition_type = EffectConditionType::kNone;

    // Type of damage for this effect
    // Only used if type is EffectType::kDamageOverTime
    EffectDamageType damage_type = EffectDamageType::kNone;

    // Type of heal used for this effect
    EffectHealType heal_type = EffectHealType::kNone;

    // Displacement type if any
    // Used if the effect type is EffectType::kDisplacement
    EffectDisplacementType displacement_type = EffectDisplacementType::kNone;

    // Type of propagation of this effect
    // NOTE: Only Used if the effect type is EffectType::kPropagation
    EffectPropagationType propagation_type = EffectPropagationType::kNone;

    // Stat type to use.
    // Used if the effect type is any of: kBuff, kDebuff
    StatType stat_type = StatType::kNone;

    // Mark type if any.
    MarkEffectType mark_type = MarkEffectType::kNone;

    // The effect type uses the stat_type value.
    static constexpr bool UsesStatType(const EffectType type)
    {
        switch (type)
        {
        case EffectType::kBuff:
        case EffectType::kDebuff:
        case EffectType::kStatOverride:
            return true;

        default:
            return false;
        }
    }
    bool UsesStatType() const
    {
        return UsesStatType(type);
    }

    // The effect type uses the damage_type value.
    static bool UsesDamageType(const EffectType type)
    {
        return type == EffectType::kDamageOverTime || type == EffectType::kInstantDamage;
    }
    bool UsesDamageType() const
    {
        return UsesDamageType(type);
    }

    // The effect type uses the effect_heal_type value.
    static bool UsesEffectHealType(const EffectType type)
    {
        return type == EffectType::kHealOverTime || type == EffectType::kInstantHeal;
    }
    bool UsesEffectHealType() const
    {
        return UsesEffectHealType(type);
    }

    static bool UsesCapturedValue(const EffectType type)
    {
        return type == EffectType::kBuff || type == EffectType::kDebuff;
    }
    bool UsesCapturedValue() const
    {
        return UsesCapturedValue(type);
    }

    // Returns true if effect can be used inside aura
    static bool IsValidForAura(const EffectType type)
    {
        return type == EffectType::kBuff || type == EffectType::kDebuff;
    }
    bool IsValidForAura() const
    {
        return IsValidForAura(type);
    }

    bool operator==(const EffectTypeID& other) const
    {
        return type == other.type && positive_state == other.positive_state && negative_state == other.negative_state &&
               condition_type == other.condition_type && damage_type == other.damage_type &&
               displacement_type == other.displacement_type && propagation_type == other.propagation_type &&
               heal_type == other.heal_type && stat_type == other.stat_type;
    }
    bool operator!=(const EffectTypeID& other) const
    {
        return !(*this == other);
    }
    void FormatTo(fmt::format_context& ctx) const;
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(EffectTypeID, FormatTo);
