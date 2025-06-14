#include "base_test_fixtures.h"

namespace simulation
{
AbilitiesData BaseTest::GetSynergyHyperAbilities(CombatSynergyBonus synergy)
{
    AbilitiesData result;

    const auto generic_ability = [&]() -> SkillData&
    {
        auto& ability = result.AddAbility();
        ability.total_duration_ms = 0;
        ability.name = fmt::format("{} ability", synergy);
        ability.activation_trigger_data.trigger_type = ActivationTriggerType::kOnHyperactive;

        auto& skill = ability.AddSkill();
        skill.name = fmt::format("{} skill", synergy);
        skill.targeting.type = SkillTargetingType::kSelf;
        skill.deployment.type = SkillDeploymentType::kDirect;

        return skill;
    };

    if (synergy.IsCombatClass() && UseHyperConfig())
    {
        switch (synergy.GetClass())
        {
        case CombatClass::kFighter:
        {
            auto& skill = generic_ability();
            skill.AddEffect(
                EffectData::CreateBuff(StatType::kEnergyResist, EffectExpression::FromValue(50_fp), kTimeInfinite));
            break;
        }
        case CombatClass::kBulwark:
        {
            auto& skill = generic_ability();
            skill.AddEffect(
                EffectData::CreateBuff(StatType::kPhysicalResist, EffectExpression::FromValue(20_fp), kTimeInfinite));
            skill.AddEffect(EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, kTimeInfinite));
            break;
        }
        case CombatClass::kRogue:
        {
            auto& skill = generic_ability();
            skill.AddEffect(EffectData::CreateBuff(
                StatType::kCritChancePercentage,
                EffectExpression::FromValue(10_fp),
                kTimeInfinite));
            break;
        }
        case CombatClass::kPsion:
        {
            auto& skill = generic_ability();
            skill.AddEffect(EffectData::CreateBuff(
                StatType::kCritChancePercentage,
                EffectExpression::FromValue(20_fp),
                kTimeInfinite));
            break;
        }
        case CombatClass::kEmpath:
        {
            auto& skill = generic_ability();
            skill.AddEffect(EffectData::CreateBuff(
                StatType::kCritChancePercentage,
                EffectExpression::FromValue(30_fp),
                kTimeInfinite));
            break;
        }
        default:
            break;
        }
    }

    return result;
}

}  // namespace simulation
