#pragma once

#include <vector>

#include "data/effect_data.h"
#include "data/effect_package_attributes.h"

namespace simulation
{
// Container for a list of effects and their atributes
// Also known as an Effect package
// https://illuvium.atlassian.net/wiki/spaces/AB/pages/72354445/Effect+Package
struct EffectPackage
{
    EffectPackage CreateDeepCopy() const;

    // Adds an effect with just a value
    EffectData& AddEffect(const EffectData& effect_data)
    {
        return effects.emplace_back(effect_data);
    }

    // Adds a damage effect with just a value
    EffectData& AddDamageEffect(const EffectDamageType damage_type, const EffectExpression& expression)
    {
        return effects.emplace_back(EffectData::CreateDamage(damage_type, expression));
    }

    // Is this effect package empty?
    constexpr bool IsEmpty() const
    {
        return attributes.IsEmpty() && effects.empty();
    }

    // Modify all the instant metered combat stat effects expression to spread evenly amongst all receivers_size
    // See https://illuvium.atlassian.net/wiki/spaces/AB/pages/279249249/Direct.SpreadEffectPackage
    EffectPackage& SpreadEvenlyForInstantMeteredTypes(const size_t receivers_size)
    {
        const auto RightOperand = EffectExpression::FromValue(FixedPoint::FromInt(static_cast<int>(receivers_size)));

        // Spread the damage throughout all targets
        for (auto& effect_data : effects)
        {
            // Only spread instant damage
            if (!effect_data.IsInstantMeteredType())
            {
                continue;
            }

            // Create new expression for the value after it has been spread to targets
            EffectExpression spread_value;
            spread_value.operation_type = EffectOperationType::kDivide;
            spread_value.operands = {effect_data.GetExpression(), RightOperand};

            effect_data.SetExpression(spread_value);
        }

        return *this;
    }

    // Does this effect package have any detrimental effects
    bool HasAnyDetrimentalEffects() const
    {
        for (const auto& effect : effects)
        {
            if (effect.IsTypeDetrimental())
            {
                return true;
            }
        }

        return false;
    }

    // The attributes for the effect package
    EffectPackageAttributes attributes;

    // The list of effects
    std::vector<EffectData> effects;
};

}  // namespace simulation
