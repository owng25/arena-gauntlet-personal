#pragma once

#include "data/stats_data.h"
#include "effect_enums.h"

namespace simulation
{
// Defines the config values for the condition effects
// See https://illuvium.atlassian.net/wiki/spaces/AB/pages/250447441/Condition.Type
class WorldEffectConditionConfig
{
public:
    // How long does it last
    int duration_ms = 0;

    // Frequency of all the DOT conditions, once per second
    int frequency_time_ms = 1000;

    int max_stacks = 5;
    bool cleanse_on_max_stacks = false;

    // Value for the damage over time.
    // NOTE: This is in high precision percentage see FixedPoint::AsHighPrecisionPercentageOf
    FixedPoint dot_high_precision_percentage = 0_fp;

    // The damage type for the DOT, if any
    EffectDamageType dot_damage_type = EffectDamageType::kNone;

    // Value of the debuff
    FixedPoint debuff_percentage = 0_fp;

    // What stat type to debuff, if any
    StatType debuff_stat_type = StatType::kNone;
};

// Configurable params for effects
class WorldEffectsConfig
{
public:
    WorldEffectsConfig() = default;

    std::unique_ptr<WorldEffectsConfig> CreateDeepCopy() const
    {
        return std::make_unique<WorldEffectsConfig>(*this);
    }

    WorldEffectConditionConfig& GetConditionType(const EffectConditionType type)
    {
        return type_to_config[EnumToIndex(type)];
    }

    const WorldEffectConditionConfig& GetConditionType(const EffectConditionType type) const
    {
        return type_to_config[EnumToIndex(type)];
    }

private:
    // An array indexed by EffectConditionType converted to index
    static constexpr size_t kArraySize = GetEnumEntriesCount<EffectConditionType>();
    std::array<WorldEffectConditionConfig, kArraySize> type_to_config{};
};
}  // namespace simulation
