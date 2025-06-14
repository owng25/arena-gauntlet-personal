#include "effect_package.h"

namespace simulation
{

EffectPackage EffectPackage::CreateDeepCopy() const
{
    EffectPackage copy;

    copy.effects.clear();
    for (const EffectData& effect_data : effects)
    {
        copy.effects.emplace_back(effect_data.CreateDeepCopy());
    }

    copy.attributes = attributes.CreateDeepCopy();

    return copy;
}

}  // namespace simulation
