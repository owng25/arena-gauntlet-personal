#include "ability_system_data_fixtures.h"
#include "gtest/gtest.h"
#include "utility/attached_effects_helper.h"

namespace simulation
{
class StatsOverflowTest : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return true;
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Foo";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.SetDefaults(AbilityType::kAttack);
            skill1.name = "Attack";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddDamageEffect(
                EffectDamageType::kEnergy,
                EffectExpression::FromSenderLiveStat(StatType::kAttackEnergyDamage));
            skill1.AddDamageEffect(
                EffectDamageType::kPhysical,
                EffectExpression::FromSenderLiveStat(StatType::kAttackPhysicalDamage));
            skill1.AddDamageEffect(
                EffectDamageType::kPure,
                EffectExpression::FromSenderLiveStat(StatType::kAttackPureDamage));
            skill1.AddDamageEffect(EffectDamageType::kEnergy, EffectExpression::FromValue(100_fp));

            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kAttackSpeed, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 100000_fp);  // make entities not die
        // Ignore movement
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
        data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 150_fp);
        data.type_data.stats.Set(StatType::kCritChancePercentage, 20_fp);
        data.type_data.stats.Set(StatType::kAttackPhysicalDamage, 100_fp);
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
    }

    Entity* red_entity2 = nullptr;
    Entity* red_entity3 = nullptr;
    Entity* red_entity4 = nullptr;
};

TEST_F(StatsOverflowTest, DodgeChanceOverflowOverTime)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    blue_template_stats.Set(StatType::kAttackDodgeChancePercentage, 25_fp);
    red_template_stats.Set(StatType::kAttackDodgeChancePercentage, 25_fp);

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 100000_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 0_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 100000_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 13_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99800_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 38_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99600_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 63_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99400_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 88_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99400_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 13_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99100_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 38_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 98900_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 63_fp);
}

TEST_F(AbilitySystemTestWithStartAttackEvery1Second, HighHitChanceOverflowTest)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Make entities not die
    SetHealthVeryHigh();
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kHitChancePercentage, 90_fp);
    blue_stats_component.SetHitChanceOverflowPercentage(90_fp);

    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);

    for (int i = 0; i < 10; i++)
    {
        // 1s
        TimeStepForSeconds(1);
        ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
        ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
        ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
        events_ability_activated.clear();
        ASSERT_EQ(events_effect_package_missed.size(), 0) << "Attack should not miss " << i;
    }

    // 11s
    TimeStepForSeconds(1);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_effect_package_missed.size(), 1) << "Attack should miss ";
    events_ability_activated.clear();
}

TEST_F(AbilitySystemTestWithStartAttackEvery1Second, LowHitChanceOverflowTest)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Make entities not die
    SetHealthVeryHigh();
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kHitChancePercentage, 10_fp);

    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);

    for (int i = 0; i < 10; i++)
    {
        if (i == 8)
        {
            // 1s
            TimeStepForSeconds(1);
            ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
            ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
            ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
            ASSERT_EQ(events_effect_package_missed.size(), 0) << "Attack should not miss " << i;
            events_ability_activated.clear();
            events_effect_package_missed.clear();
        }
        else
        {
            // 1s
            TimeStepForSeconds(1);
            ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
            ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
            ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
            ASSERT_EQ(events_effect_package_missed.size(), 1) << "Attack should miss " << i;
            events_ability_activated.clear();
            events_effect_package_missed.clear();
        }
    }

    // 11s
    TimeStepForSeconds(1);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_effect_package_missed.size(), 1);
    events_ability_activated.clear();
}

TEST_F(StatsOverflowTest, CritChanceOverflowHit)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();

    blue_template_stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 100000_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 0_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99700_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 16_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99500_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 36_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99300_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 56_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99100_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 76_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 98900_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 96_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 98600_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 16_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 98400_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 36_fp);
}

TEST_F(StatsOverflowTest, CritChanceOverflowHitBuff)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();

    // Add a buff to Crit Chance
    auto effect_data =
        EffectData::CreateBuff(StatType::kCritChancePercentage, EffectExpression::FromValue(25_fp), kTimeInfinite);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

    blue_template_stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 100000_fp);
    EXPECT_EQ(red_stats_component.GetBaseValueForType(StatType::kCritChancePercentage), 20_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 0_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    StatsData live_stats = world->GetLiveStats(blue_entity->GetID());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99700_fp);
    EXPECT_EQ(live_stats.Get(StatType::kCritChancePercentage), 45_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 41_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99500_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 86_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99200_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 31_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99000_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 76_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 98700_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 21_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 98500_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 66_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 98200_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 11_fp);
}

TEST_F(StatsOverflowTest, DodgeChanceOverflowOverTimeBuff)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
    auto& red_template_stats = red_stats_component.GetMutableTemplateStats();

    // Add a buff to dodge chance
    auto effect_data = EffectData::CreateBuff(
        StatType::kAttackDodgeChancePercentage,
        EffectExpression::FromValue(25_fp),
        kTimeInfinite);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());

    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity->GetID(), effect_data, effect_state);

    blue_template_stats.Set(StatType::kAttackDodgeChancePercentage, 25_fp);
    red_template_stats.Set(StatType::kAttackDodgeChancePercentage, 25_fp);

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 100000_fp);
    EXPECT_EQ(red_stats_component.GetBaseValueForType(StatType::kAttackDodgeChancePercentage), 25_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 0_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    StatsData live_stats = world->GetLiveStats(red_entity->GetID());
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 100000_fp);
    EXPECT_EQ(live_stats.Get(StatType::kAttackDodgeChancePercentage), 50_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 38_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99800_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 88_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99800_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 38_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99600_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 88_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99600_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 38_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99300_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 88_fp);

    // Fire basic attack
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 99300_fp);
    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 38_fp);
}

TEST_F(AbilitySystemTestWithStartAttackEvery1Second, HighHitChanceOverflowTestBuff)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Make entities not die
    SetHealthVeryHigh();
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kHitChancePercentage, 50_fp);
    blue_stats_component.SetHitChanceOverflowPercentage(50_fp);

    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);

    // Add a buff to CritChancePercentage
    auto effect_data =
        EffectData::CreateBuff(StatType::kHitChancePercentage, EffectExpression::FromValue(100_fp), 10000);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

    StatsData live_stats = world->GetLiveStats(blue_entity->GetID());
    EXPECT_EQ(live_stats.Get(StatType::kHitChancePercentage), 100_fp);
    // None of these attacks should miss
    for (int i = 0; i < 11; i++)
    {
        live_stats = world->GetLiveStats(blue_entity->GetID());
        EXPECT_EQ(live_stats.Get(StatType::kHitChancePercentage), 100_fp);
        // 1s
        TimeStepForSeconds(1);
        ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
        ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
        ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
        events_ability_activated.clear();
        ASSERT_EQ(events_effect_package_missed.size(), 0) << "Attack should not miss " << i;
    }
    // 21s
    TimeStepForSeconds(1);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_effect_package_missed.size(), 0) << "Attack should miss ";
    events_ability_activated.clear();

    TimeStepForSeconds(1);

    live_stats = world->GetLiveStats(blue_entity->GetID());
    EXPECT_EQ(live_stats.Get(StatType::kHitChancePercentage), 50_fp);
    ASSERT_EQ(events_ability_activated.size(), 2) << "Should have only fired once for both entities";
    ASSERT_EQ(events_ability_activated.at(0).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_ability_activated.at(1).ability_type, AbilityType::kAttack);
    ASSERT_EQ(events_effect_package_missed.size(), 1) << "Attack should miss ";
    events_ability_activated.clear();
}

}  // namespace simulation
