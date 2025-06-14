#include "base_test_fixtures.h"
#include "utility/stats_helper.h"

namespace simulation
{
class WoundEffectTest : public BaseTest
{
};

TEST_F(WoundEffectTest, Basic)
{
    CombatUnitData data{};
    data.type_id.type = CombatUnitType::kIlluvial;
    data.type_data.combat_affinity = CombatAffinity::kWater;
    data.type_data.combat_class = CombatClass::kBulwark;
    data.type_data.tier = 0;

    StatsHelper::SetDefaults(data.type_data.stats);
    data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);
    data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, 0_fp);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 1000_fp);
    data.type_data.stats.Set(StatType::kStartingShield, 100_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
    data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);
    data.type_data.stats.Set(StatType::kAttackPhysicalDamage, 100_fp);

    // Attack ability
    {
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
        auto& ability = data.type_data.attack_abilities.AddAbility();
        ability.name = "Ability with wound";
        auto& skill = ability.AddSkill();
        skill.name = "Skill with wound";
        skill.percentage_of_ability_duration = kMaxPercentage;
        skill.targeting.type = SkillTargetingType::kCurrentFocus;
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.deployment.guidance = EnumSet<GuidanceType>::MakeFull();
        skill.AddDamageEffect(
            EffectDamageType::kPhysical,
            EffectExpression::FromSenderLiveStat(StatType::kAttackDamage));
        skill.AddEffect(EffectData::CreateCondition(EffectConditionType::kWound, kDefaultAttachedEffectsFrequencyMs));
    }

    constexpr HexGridPosition blue_position{5, 5};
    constexpr HexGridPosition red_position = Reflect(blue_position);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, red_position, data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, blue_position, data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Stun(*blue_entity);

    EventHistory<EventType::kOnDamage> damage_events(*world);

    world->TimeStep();

    ASSERT_EQ(damage_events.Size(), 1);
    ASSERT_EQ(damage_events.events.front().damage_value, "100"_fp);
    ASSERT_TRUE(blue_entity->Get<AttachedEffectsComponent>().HasCondition(EffectConditionType::kWound));

    Stun(*red_entity);

    constexpr FixedPoint total_wound_damage = 100_fp / 10_fp;
    constexpr FixedPoint wound_damage_per_tick = total_wound_damage / 8_fp;
    for (size_t i = 0; i != 8; ++i)  // wound lasts 8 seconds
    {
        damage_events.Clear();
        TimeStepForMs(1000);
        ASSERT_EQ(damage_events.Size(), 1);
        ASSERT_EQ(damage_events.events.front().damage_value, wound_damage_per_tick);
    }

    damage_events.Clear();
    TimeStepForMs(1000);
    ASSERT_TRUE(damage_events.IsEmpty());
}
}  // namespace simulation
