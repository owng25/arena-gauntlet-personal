#include "skill_data.h"

namespace simulation
{

std::shared_ptr<SkillData> SkillData::CreateDeepCopy() const
{
    std::shared_ptr<SkillData> copy = std::make_shared<SkillData>(*this);

    copy->effect_package = effect_package.CreateDeepCopy();

    return copy;
}

}  // namespace simulation
