#include "ability_system_data_fixtures.h"
#include "components/drone_augment_component.h"
#include "components/focus_component.h"
#include "components/stats_component.h"
#include "data/constants.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "gtest/gtest.h"
#include "systems/ability_system.h"
#include "utility/attached_effects_helper.h"

namespace simulation
{
class AbilitySystemBaseTargetingTest : public AbilitySystemBaseTest
{
    typedef AbilitySystemBaseTest Super;

protected:
    virtual void InitCombatUnitData()
    {
        data = CreateCombatUnitData();
        data2 = CreateCombatUnitData();
        data3 = CreateCombatUnitData();
        data4 = CreateCombatUnitData();
        data5 = CreateCombatUnitData();

        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kMaxHealth, 50_fp);

        // Stats
        data2.radius_units = 1;
        data2.type_data.combat_affinity = CombatAffinity::kFire;
        data2.type_data.combat_class = CombatClass::kBehemoth;
        data2.type_data.stats.Set(StatType::kMaxHealth, 100_fp);

        // Stats
        data3.radius_units = 1;
        data3.type_data.combat_affinity = CombatAffinity::kFire;
        data3.type_data.combat_class = CombatClass::kBehemoth;
        data3.type_data.stats.Set(StatType::kMaxHealth, 200_fp);

        // Stats
        data4.radius_units = 1;
        data4.type_data.combat_affinity = CombatAffinity::kFire;
        data4.type_data.combat_class = CombatClass::kBehemoth;
        data4.type_data.stats.Set(StatType::kMaxHealth, 300_fp);

        // Stats
        data5.radius_units = 1;
        data5.type_data.combat_affinity = CombatAffinity::kFire;
        data5.type_data.combat_class = CombatClass::kBehemoth;
        data5.type_data.stats.Set(StatType::kMaxHealth, 400_fp);

        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual void SpawnCombatUnits() {}

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
    }

    Entity* blue_entity = nullptr;
    Entity* blue_entity2 = nullptr;
    Entity* blue_entity3 = nullptr;
    Entity* blue_entity4 = nullptr;
    Entity* blue_entity5 = nullptr;
    Entity* red_entity = nullptr;
    Entity* red_entity2 = nullptr;
    Entity* red_entity3 = nullptr;
    Entity* red_entity4 = nullptr;
    CombatUnitData data;
    CombatUnitData data2;
    CombatUnitData data3;
    CombatUnitData data4;
    CombatUnitData data5;
};

class AbilitySystemTestCombatStatCheckEnemy : public AbilitySystemBaseTargetingTest
{
    typedef AbilitySystemBaseTargetingTest Super;

protected:
    void SpawnCombatUnits() override
    {
        // Spawn 5 combat units
        SpawnCombatUnit(Team::kBlue, {-5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {15, 30}, data2, red_entity);
        SpawnCombatUnit(Team::kRed, {8, 15}, data3, red_entity2);
        SpawnCombatUnit(Team::kRed, {10, 20}, data4, red_entity3);
        SpawnCombatUnit(Team::kRed, {13, 25}, data5, red_entity4);

        ASSERT_FALSE(world->AreAllies(blue_entity->GetID(), red_entity->GetID()));
        ASSERT_TRUE(
            world->AreAllies(red_entity->GetID(), red_entity2->GetID(), red_entity3->GetID(), red_entity4->GetID()));
    }
};

class AbilitySystemTestAllegianceType : public AbilitySystemBaseTargetingTest
{
    typedef AbilitySystemBaseTargetingTest Super;

protected:
    void SpawnCombatUnits() override
    {
        // Spawn 5 combat units
        SpawnCombatUnit(Team::kBlue, {-5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kBlue, {15, 30}, data2, blue_entity2);
        SpawnCombatUnit(Team::kRed, {8, 15}, data3, red_entity);
        SpawnCombatUnit(Team::kRed, {10, 20}, data4, red_entity2);
        SpawnCombatUnit(Team::kRed, {13, 25}, data5, red_entity3);

        ASSERT_FALSE(world->AreAllies(blue_entity->GetID(), red_entity->GetID()));
        ASSERT_TRUE(world->AreAllies(blue_entity->GetID(), blue_entity2->GetID()));
        ASSERT_TRUE(world->AreAllies(red_entity->GetID(), red_entity2->GetID(), red_entity3->GetID()));
    }
};

class AbilitySystemTestCombatStatCheckAlly : public AbilitySystemBaseTargetingTest
{
    typedef AbilitySystemBaseTargetingTest Super;

protected:
    void SpawnCombatUnits() override
    {
        // Spawn 5 combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kBlue, {15, 30}, data2, blue_entity2);
        SpawnCombatUnit(Team::kBlue, {8, 15}, data3, blue_entity3);
        SpawnCombatUnit(Team::kBlue, {10, 20}, data4, blue_entity4);
        SpawnCombatUnit(Team::kBlue, {13, 25}, data5, blue_entity5);
    }

    virtual void SpawnSingleCombatUnit()
    {
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
    }
};

class AbilitySystemTestDistanceCheck : public AbilitySystemBaseTargetingTest
{
    typedef AbilitySystemBaseTargetingTest Super;

protected:
    void SpawnCombatUnits() override
    {
        // Spawn 5 combat units
        SpawnCombatUnit(Team::kBlue, {-25, 2}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {-14, -25}, data2, red_entity);
        SpawnCombatUnit(Team::kRed, {-3, 45}, data3, red_entity2);
        SpawnCombatUnit(Team::kRed, {-33, -15}, data4, red_entity3);
        SpawnCombatUnit(Team::kRed, {-8, 35}, data5, red_entity4);
    }
};

class AbilitySystemTestDistanceCheckAlly : public AbilitySystemBaseTargetingTest
{
    typedef AbilitySystemBaseTargetingTest Super;

protected:
    void SpawnCombatUnits() override
    {
        // Spawn 5 combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kBlue, {10, 20}, data2, blue_entity2);
        SpawnCombatUnit(Team::kBlue, {2, 4}, data3, blue_entity3);
        SpawnCombatUnit(Team::kBlue, {3, 7}, data4, blue_entity4);
        SpawnCombatUnit(Team::kBlue, {9, 17}, data5, blue_entity5);
    }
};

class AbilitySystemTestAllAlliesWithinX : public AbilitySystemBaseTargetingTest
{
    typedef AbilitySystemBaseTargetingTest Super;

protected:
    void SpawnCombatUnits() override
    {
        // Spawn 5 combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kBlue, {7, 13}, data2, blue_entity2);
        SpawnCombatUnit(Team::kBlue, {2, 4}, data3, blue_entity3);
        SpawnCombatUnit(Team::kRed, {3, 7}, data4, red_entity);
        SpawnCombatUnit(Team::kBlue, {9, 17}, data5, blue_entity4);
    }
};

class AbilitySystemTestAllEnemiesWithinX : public AbilitySystemBaseTargetingTest
{
    typedef AbilitySystemBaseTargetingTest Super;

protected:
    void SpawnCombatUnits() override
    {
        // Spawn 5 combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kBlue, {7, 13}, data2, blue_entity2);
        SpawnCombatUnit(Team::kRed, {2, 4}, data3, red_entity);
        SpawnCombatUnit(Team::kRed, {4, 7}, data4, red_entity2);
        SpawnCombatUnit(Team::kRed, {9, 17}, data5, red_entity3);
    }
};

class AbilitySystemTestAllSynergies : public AbilitySystemBaseTargetingTest
{
    typedef AbilitySystemBaseTargetingTest Super;

protected:
    void InitCombatUnitData() override
    {
        Super::InitCombatUnitData();

        // Stats
        data.type_data.combat_affinity = CombatAffinity::kWater;
        data.type_data.combat_class = CombatClass::kBehemoth;

        data2.type_data.combat_affinity = CombatAffinity::kTsunami;
        data2.type_data.combat_class = CombatClass::kBehemoth;

        data3.type_data.combat_affinity = CombatAffinity::kFire;
        data3.type_data.combat_class = CombatClass::kColossus;

        data4.type_data.combat_affinity = CombatAffinity::kFire;
        data4.type_data.combat_class = CombatClass::kTemplar;

        data5.type_data.combat_affinity = CombatAffinity::kFire;
        data5.type_data.combat_class = CombatClass::kAegis;
    }

    void SpawnCombatUnits() override
    {
        // Spawn 5 combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kBlue, {7, 13}, data2, blue_entity2);
        SpawnCombatUnit(Team::kRed, {2, 4}, data3, red_entity);
        SpawnCombatUnit(Team::kRed, {4, 7}, data4, red_entity2);
        SpawnCombatUnit(Team::kRed, {9, 17}, data5, red_entity3);
    }

    void SetUp() override
    {
        Super::SetUp();
    }

    // Needed to enable this because we are testing synergy targeting
    bool UseSynergiesData() const override
    {
        return true;
    }
};

class AbilitySystemTestPreviousTarget : public AbilitySystemBaseTargetingTest
{
    typedef AbilitySystemBaseTargetingTest Super;

protected:
    void SpawnCombatUnits() override
    {
        // Spawn 5 combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {7, 13}, data2, red_entity);
        SpawnCombatUnit(Team::kRed, {2, 4}, data3, red_entity2);
        SpawnCombatUnit(Team::kRed, {4, 7}, data4, red_entity3);
        SpawnCombatUnit(Team::kRed, {9, 17}, data5, red_entity4);
    }
};

class AbilitySystemTestTargetingGuidance : public AbilitySystemBaseTargetingTest
{
    typedef AbilitySystemBaseTargetingTest Super;

protected:
    void SpawnCombatUnits() override
    {
        // Spawn 5 combat units
        SpawnCombatUnit(Team::kBlue, {-5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kBlue, {15, 30}, data2, blue_entity2);
        SpawnCombatUnit(Team::kRed, {8, 15}, data3, red_entity);
        SpawnCombatUnit(Team::kRed, {10, 20}, data4, red_entity2);
        SpawnCombatUnit(Team::kRed, {13, 25}, data5, red_entity3);

        // Get attached effects components
        auto& blue_attached_effects_component = blue_entity->Get<AttachedEffectsComponent>();
        auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();

        // Make first blue and red airborne
        const auto effect_data = EffectData::CreatePlaneChange(EffectPlaneChange::kAirborne, 300);
        const EntityID blue_entity_id = blue_entity->GetID();
        const EntityID red_entity_id = red_entity->GetID();
        {
            EffectState effect_state{};
            effect_state.sender_stats = world->GetFullStats(blue_entity_id);
            GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);
        }
        {
            EffectState effect_state{};
            effect_state.sender_stats = world->GetFullStats(red_entity_id);
            GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity_id, effect_data, effect_state);
        }

        ASSERT_TRUE(blue_attached_effects_component.HasPlaneChange(EffectPlaneChange::kAirborne));
        ASSERT_TRUE(red_attached_effects_component.HasPlaneChange(EffectPlaneChange::kAirborne));
    }
};

class AbilitySystemRetargetingTest : public AbilitySystemTestDistanceCheck
{
    typedef AbilitySystemTestDistanceCheck Super;

protected:
    void InitAbility(size_t target_number)
    {
        auto& ability = data.type_data.omega_abilities.AddAbility();
        ability.total_duration_ms = 300;

        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.lowest = true;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.num = target_number;
        skill1.deployment.pre_deployment_delay_percentage = 100;
        skill1.deployment.pre_deployment_retargeting_percentage = 50;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }

    void SpawnCombatUnits() override
    {
        // Spawn 5 combat units
        SpawnCombatUnit(Team::kBlue, {-25, 2}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {-14, -25}, data2, red_entity);
        SpawnCombatUnit(Team::kRed, {-3, 45}, data3, red_entity2);
        SpawnCombatUnit(Team::kRed, {-33, -15}, data4, red_entity3);
        SpawnCombatUnit(Team::kRed, {-8, 35}, data5, red_entity4);
        SpawnCombatUnit(Team::kRed, {25, 25}, data5, red_entity5);

        // Make a blue ready to deploy omega
        auto& blue_stats_component = blue_entity->Get<StatsComponent>();
        auto& blue_template_stats = blue_stats_component.GetMutableTemplateStats();
        blue_template_stats.Set(StatType::kCurrentEnergy, blue_template_stats.Get(StatType::kCurrentEnergy));
    }

    Entity* red_entity5 = nullptr;
};

TEST_F(AbilitySystemTestCombatStatCheckEnemy, HighestEnemyMaxHP)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCombatStatCheck;
        skill1.targeting.stat_type = StatType::kMaxHealth;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.lowest = false;
        skill1.targeting.num = 3;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 3) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, red_entity2->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, red_entity3->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[2].receiver_id, red_entity4->GetID())
        << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTestCombatStatCheckEnemy, LowestEnemyMaxHP)
{
    // Initialize the abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCombatStatCheck;
        skill1.targeting.stat_type = StatType::kMaxHealth;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.lowest = true;
        skill1.targeting.num = 3;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 3) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, red_entity->GetID()) << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, red_entity2->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[2].receiver_id, red_entity3->GetID())
        << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTestCombatStatCheckAlly, HighestAllyMaxHP)
{
    // Initialize the abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();
        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.name = "Omega Attack Chain";
        skill1.targeting.type = SkillTargetingType::kCombatStatCheck;
        skill1.targeting.stat_type = StatType::kMaxHealth;
        skill1.targeting.group = AllegianceType::kAlly;
        skill1.targeting.lowest = false;
        skill1.targeting.num = 3;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddEffect(
            EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(120_fp), kTimeInfinite));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& stats_component = blue_entity3->Get<StatsComponent>();
    auto& attached_effects_component = blue_entity3->Get<AttachedEffectsComponent>();
    EXPECT_EQ(stats_component.GetMaxHealth(), 200_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 200_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    world->TimeStep();
    ASSERT_EQ(attached_effects_component.GetAttachedEffects().size(), 1);

    ASSERT_EQ(events_effect_package_received.size(), 3) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, blue_entity3->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, blue_entity4->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[2].receiver_id, blue_entity5->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(stats_component.GetMaxHealth(), 320_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 320_fp);
}

TEST_F(AbilitySystemTestCombatStatCheckAlly, LowestAllyMaxHP)
{
    // Initialize the abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();
        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.name = "Omega Attack Chain";
        skill1.targeting.type = SkillTargetingType::kCombatStatCheck;
        skill1.targeting.stat_type = StatType::kMaxHealth;
        skill1.targeting.group = AllegianceType::kAlly;
        skill1.targeting.lowest = true;
        skill1.targeting.num = 3;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddEffect(
            EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(120_fp), kTimeInfinite));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 3) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, blue_entity2->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, blue_entity3->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[2].receiver_id, blue_entity4->GetID())
        << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTestDistanceCheck, ClosestEnemyTargeting)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.lowest = true;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.num = 3;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 3) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, red_entity->GetID()) << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, red_entity3->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[2].receiver_id, red_entity4->GetID())
        << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTestDistanceCheck, FarthestEnemyTargeting)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.lowest = false;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.num = 3;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 3) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, red_entity->GetID()) << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, red_entity2->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[2].receiver_id, red_entity4->GetID())
        << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTestDistanceCheckAlly, ClosestAllyTargeting)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.lowest = true;
        skill1.targeting.group = AllegianceType::kAlly;
        skill1.targeting.num = 3;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 3) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, blue_entity3->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, blue_entity4->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[2].receiver_id, blue_entity5->GetID())
        << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTestDistanceCheckAlly, FarthestAllyTargeting)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.lowest = false;
        skill1.targeting.group = AllegianceType::kAlly;
        skill1.targeting.num = 3;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 3) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, blue_entity2->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, blue_entity3->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[2].receiver_id, blue_entity5->GetID())
        << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTestAllAlliesWithinX, AllieswithinX)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kInZone;
        skill1.targeting.group = AllegianceType::kAlly;
        skill1.targeting.radius_units = 9;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 2) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, blue_entity2->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, blue_entity3->GetID())
        << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTestAllAlliesWithinX, AllWithinX)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kInZone;
        skill1.targeting.group = AllegianceType::kAll;
        skill1.targeting.radius_units = 9;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 3) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, blue_entity2->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, blue_entity3->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[2].receiver_id, red_entity->GetID()) << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTestAllEnemiesWithinX, EnemiesWithinX)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kInZone;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.radius_units = 9;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 2) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, red_entity->GetID()) << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, red_entity2->GetID())
        << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTestAllegianceType, AllegianceTargetAllies)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kAllegiance;
        skill1.targeting.group = AllegianceType::kAlly;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddEffect(
            EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(120_fp), kTimeInfinite));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& blue2_stats_component = blue_entity2->Get<StatsComponent>();

    ASSERT_EQ(blue2_stats_component.GetMaxHealth(), 100_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);

    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 1) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, blue_entity2->GetID())
        << "Skill should have hit the entity";
    ASSERT_EQ(blue2_stats_component.GetMaxHealth(), 220_fp);
}

TEST_F(AbilitySystemTestAllegianceType, AllegianceTargetEnemies)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kAllegiance;
        skill1.targeting.group = AllegianceType::kEnemy;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddEffect(
            EffectData::CreateDebuff(StatType::kMaxHealth, EffectExpression::FromValue(50_fp), kTimeInfinite));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red2_stats_component = red_entity2->Get<StatsComponent>();
    auto& red3_stats_component = red_entity3->Get<StatsComponent>();

    ASSERT_EQ(red_stats_component.GetMaxHealth(), 200_fp);
    ASSERT_EQ(red2_stats_component.GetMaxHealth(), 300_fp);
    ASSERT_EQ(red3_stats_component.GetMaxHealth(), 400_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 3) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, red_entity->GetID()) << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, red_entity2->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[2].receiver_id, red_entity3->GetID())
        << "Skill should have hit the entity";

    ASSERT_EQ(red_stats_component.GetMaxHealth(), 150_fp);
    ASSERT_EQ(red2_stats_component.GetMaxHealth(), 250_fp);
    ASSERT_EQ(red3_stats_component.GetMaxHealth(), 350_fp);
}

TEST_F(AbilitySystemTestAllSynergies, SynergyTargetAlliesNotWater)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kSynergy;
        skill1.targeting.group = AllegianceType::kAlly;
        skill1.targeting.not_combat_synergy = CombatAffinity::kWater;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddEffect(
            EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(50_fp), kTimeInfinite));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    const auto& blue2_stats_component = blue_entity2->Get<StatsComponent>();

    ASSERT_EQ(blue2_stats_component.GetMaxHealth(), 100_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 1) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, blue_entity2->GetID())
        << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTestAllSynergies, SynergyTargetRogueAlliesNotWater)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kSynergy;
        skill1.targeting.group = AllegianceType::kAlly;
        skill1.targeting.combat_synergy = CombatClass::kRogue;
        skill1.targeting.not_combat_synergy = CombatAffinity::kWater;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddEffect(
            EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(50_fp), kTimeInfinite));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    const auto& blue2_stats_component = blue_entity2->Get<StatsComponent>();

    ASSERT_EQ(blue2_stats_component.GetMaxHealth(), 100_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 0) << "skill should not have fire";
}

TEST_F(AbilitySystemTestAllSynergies, SynergyTargetAlliesWater)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kSynergy;
        skill1.targeting.group = AllegianceType::kAlly;
        skill1.targeting.combat_synergy = CombatAffinity::kWater;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddEffect(
            EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(50_fp), kTimeInfinite));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 0) << "skill should not have fire";
}

TEST_F(AbilitySystemTestAllSynergies, SynergyTargetAlliesTsunami)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kSynergy;
        skill1.targeting.group = AllegianceType::kAlly;
        skill1.targeting.combat_synergy = CombatAffinity::kTsunami;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddEffect(
            EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(50_fp), kTimeInfinite));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    const auto& blue2_stats_component = blue_entity2->Get<StatsComponent>();

    ASSERT_EQ(blue2_stats_component.GetMaxHealth(), 100_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 1) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, blue_entity2->GetID())
        << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTestAllSynergies, SynergyTargetEnemiesColossus)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kSynergy;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.combat_synergy = CombatClass::kColossus;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddEffect(
            EffectData::CreateDebuff(StatType::kMaxHealth, EffectExpression::FromValue(50_fp), kTimeInfinite));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 1) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, red_entity->GetID()) << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTestAllSynergies, SynergyTargetEnemiesFire)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kSynergy;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.combat_synergy = CombatAffinity::kFire;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddEffect(
            EffectData::CreateDebuff(StatType::kMaxHealth, EffectExpression::FromValue(50_fp), kTimeInfinite));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 3) << "skill should have fire";
}

TEST_F(AbilitySystemTestDistanceCheck, MoreTargetsThanEntities)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.lowest = true;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.num = 5;
        skill1.effect_package.attributes.cumulative_damage = true;
        skill1.effect_package.attributes.split_damage = false;

        // Set the projectile data
        skill1.projectile.size_units = 3;
        skill1.projectile.speed_sub_units = 3000;
        skill1.projectile.is_homing = true;
        skill1.projectile.apply_to_all = false;
        skill1.projectile.is_blockable = true;

        skill1.deployment.type = SkillDeploymentType::kProjectile;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));

        ability.total_duration_ms = 0;
        skill1.deployment.pre_deployment_delay_percentage = 0;
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Listen to spawned projectiles
    std::vector<event_data::ProjectileCreated> events_projectile_created;
    world->SubscribeToEvent(
        EventType::kProjectileCreated,
        [&events_projectile_created](const Event& event)
        {
            events_projectile_created.emplace_back(event.Get<event_data::ProjectileCreated>());
        });

    // Listen to spawned projectiles
    std::vector<event_data::Moved> events_projectiles_moved;
    world->SubscribeToEvent(
        EventType::kMoved,
        [&events_projectiles_moved, this](const Event& event)
        {
            const auto& data = event.Get<event_data::Moved>();
            if (EntityHelper::IsAProjectile(*world, data.entity_id))
            {
                events_projectiles_moved.emplace_back(data);
            }
        });

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kAttackRangeUnits, 100_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    world->TimeStep();

    // Check events contains what is needed
    ASSERT_EQ(events_projectile_created.size(), 10) << "Projectiles were not created";
    ASSERT_EQ(events_projectile_created[0].sender_id, blue_entity->GetID())
        << "First projectile did not originate from blue";
    ASSERT_EQ(events_projectile_created[1].sender_id, blue_entity->GetID())
        << "Second projectile target from red is not blue";
    ASSERT_EQ(events_projectile_created[2].sender_id, blue_entity->GetID())
        << "Third projectile target from red is not blue";
    ASSERT_EQ(events_projectile_created[3].sender_id, blue_entity->GetID())
        << "Fourth projectile target from red is not blue";
    ASSERT_EQ(events_projectile_created[4].sender_id, blue_entity->GetID())
        << "Fifth projectile target from red is not blue";
    ASSERT_EQ(events_projectile_created[0].receiver_id, red_entity->GetID())
        << "First projectile did not reach correct target";
    ASSERT_EQ(events_projectile_created[1].receiver_id, red_entity2->GetID())
        << "Second projectile did not reach correct target";
    ASSERT_EQ(events_projectile_created[2].receiver_id, red_entity3->GetID())
        << "Third projectile did not reach correct target";
    ASSERT_EQ(events_projectile_created[3].receiver_id, red_entity4->GetID())
        << "Fourth projectile did not reach correct target";
    ASSERT_EQ(events_projectile_created[4].receiver_id, red_entity->GetID())
        << "Fifth projectile did not reach correct target";
    ASSERT_TRUE(world->HasEntity(events_projectile_created[0].entity_id));
    ASSERT_TRUE(world->HasEntity(events_projectile_created[1].entity_id));

    events_projectiles_moved.clear();
    world->TimeStep();

    ASSERT_EQ(10, events_projectiles_moved.size()) << "10 projectiles should have moved";
}

TEST_F(AbilitySystemTestDistanceCheck, MoreTargetsThanEntitiesSplitDamage)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.lowest = true;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.num = 8;
        skill1.effect_package.attributes.cumulative_damage = false;
        skill1.effect_package.attributes.split_damage = true;

        // Set the projectile data
        skill1.projectile.size_units = 3;
        skill1.projectile.speed_sub_units = 3000;
        skill1.projectile.is_homing = true;
        skill1.projectile.apply_to_all = false;
        skill1.projectile.is_blockable = true;

        skill1.deployment.type = SkillDeploymentType::kProjectile;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));

        ability.total_duration_ms = 0;
        skill1.deployment.pre_deployment_delay_percentage = 0;
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Listen to spawned projectiles
    std::vector<event_data::ProjectileCreated> events_projectile_created;
    world->SubscribeToEvent(
        EventType::kProjectileCreated,
        [&events_projectile_created](const Event& event)
        {
            events_projectile_created.emplace_back(event.Get<event_data::ProjectileCreated>());
        });

    // Listen to spawned projectiles
    std::vector<event_data::Moved> events_projectiles_moved;
    world->SubscribeToEvent(
        EventType::kMoved,
        [&events_projectiles_moved, this](const Event& event)
        {
            const auto& data = event.Get<event_data::Moved>();
            if (EntityHelper::IsAProjectile(*world, data.entity_id))
            {
                events_projectiles_moved.emplace_back(data);
            }
        });

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kAttackRangeUnits, 100_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    world->TimeStep();

    // Check events contains what is needed
    ASSERT_EQ(events_projectile_created.size(), 16) << "Projectiles were not created";
    ASSERT_EQ(events_projectile_created[0].sender_id, blue_entity->GetID())
        << "First projectile did not originate from blue";
    ASSERT_EQ(events_projectile_created[1].sender_id, blue_entity->GetID())
        << "Second projectile target from red is not blue";
    ASSERT_EQ(events_projectile_created[2].sender_id, blue_entity->GetID())
        << "Third projectile target from red is not blue";
    ASSERT_EQ(events_projectile_created[3].sender_id, blue_entity->GetID())
        << "Fourth projectile target from red is not blue";
    ASSERT_EQ(events_projectile_created[4].sender_id, blue_entity->GetID())
        << "Fifth projectile target from red is not blue";
    ASSERT_EQ(events_projectile_created[5].sender_id, blue_entity->GetID())
        << "Sixth projectile target from red is not blue";
    ASSERT_EQ(events_projectile_created[6].sender_id, blue_entity->GetID())
        << "Seventh projectile target from red is not blue";
    ASSERT_EQ(events_projectile_created[7].sender_id, blue_entity->GetID())
        << "Eighth projectile target from red is not blue";
    ASSERT_EQ(events_projectile_created[0].receiver_id, red_entity->GetID())
        << "First projectile did not reach correct target";
    ASSERT_EQ(events_projectile_created[1].receiver_id, red_entity2->GetID())
        << "Second projectile did not reach correct target";
    ASSERT_EQ(events_projectile_created[2].receiver_id, red_entity3->GetID())
        << "Fourth projectile did not reach correct target";
    ASSERT_EQ(events_projectile_created[3].receiver_id, red_entity4->GetID())
        << "Second projectile did not reach correct target";
    ASSERT_EQ(events_projectile_created[4].receiver_id, red_entity->GetID())
        << "Fourth projectile did not reach correct target";
    ASSERT_EQ(events_projectile_created[5].receiver_id, red_entity2->GetID())
        << "Fourth projectile did not reach correct target";
    ASSERT_EQ(events_projectile_created[6].receiver_id, red_entity3->GetID())
        << "Second projectile did not reach correct target";
    ASSERT_EQ(events_projectile_created[7].receiver_id, red_entity4->GetID())
        << "Fourth projectile did not reach correct target";
    ASSERT_TRUE(world->HasEntity(events_projectile_created[0].entity_id));
    ASSERT_TRUE(world->HasEntity(events_projectile_created[1].entity_id));

    events_projectiles_moved.clear();
    world->TimeStep();

    ASSERT_EQ(16, events_projectiles_moved.size()) << "16 projectiles should have moved";
}

TEST_F(AbilitySystemTestDistanceCheck, OmegaNoOverflow)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.omega_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.lowest = true;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.num = 4;
        skill1.effect_package.attributes.cumulative_damage = false;
        skill1.effect_package.attributes.split_damage = true;

        // Set the projectile data
        skill1.projectile.size_units = 3;
        skill1.projectile.speed_sub_units = 3000;
        skill1.projectile.is_homing = true;
        skill1.projectile.apply_to_all = false;
        skill1.projectile.is_blockable = true;

        skill1.deployment.type = SkillDeploymentType::kProjectile;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));

        ability.total_duration_ms = 0;
        skill1.deployment.pre_deployment_delay_percentage = 0;
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Listen to spawned projectiles
    std::vector<event_data::ProjectileCreated> events_projectile_created;
    world->SubscribeToEvent(
        EventType::kProjectileCreated,
        [&events_projectile_created](const Event& event)
        {
            events_projectile_created.emplace_back(event.Get<event_data::ProjectileCreated>());
        });

    // Listen to spawned projectiles
    std::vector<event_data::Moved> events_projectiles_moved;
    world->SubscribeToEvent(
        EventType::kMoved,
        [&events_projectiles_moved, this](const Event& event)
        {
            const auto& data = event.Get<event_data::Moved>();
            if (EntityHelper::IsAProjectile(*world, data.entity_id))
            {
                events_projectiles_moved.emplace_back(data);
            }
        });

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kAttackRangeUnits, 100_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& omega_ability = blue_abilities_component.GetStateOmegaAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, omega_ability);

    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 0_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 0_fp);

    world->TimeStep();

    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 88_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 96_fp);

    // Check events contains what is needed
    ASSERT_EQ(events_projectile_created.size(), 8) << "Projectiles were not created";
    ASSERT_EQ(events_projectile_created[0].sender_id, blue_entity->GetID())
        << "First projectile did not originate from blue";
    ASSERT_EQ(events_projectile_created[1].sender_id, blue_entity->GetID())
        << "Second projectile target from red is not blue";
    ASSERT_EQ(events_projectile_created[2].sender_id, blue_entity->GetID())
        << "Third projectile target from red is not blue";
    ASSERT_EQ(events_projectile_created[3].sender_id, blue_entity->GetID())
        << "Fourth projectile target from red is not blue";
    ASSERT_EQ(events_projectile_created[0].receiver_id, red_entity->GetID())
        << "First projectile did not reach correct target";
    ASSERT_EQ(events_projectile_created[1].receiver_id, red_entity2->GetID())
        << "Second projectile did not reach correct target";
    ASSERT_EQ(events_projectile_created[2].receiver_id, red_entity3->GetID())
        << "Fourth projectile did not reach correct target";
    ASSERT_EQ(events_projectile_created[3].receiver_id, red_entity4->GetID())
        << "Second projectile did not reach correct target";
    ASSERT_TRUE(world->HasEntity(events_projectile_created[0].entity_id));
    ASSERT_TRUE(world->HasEntity(events_projectile_created[1].entity_id));

    events_projectiles_moved.clear();
    world->TimeStep();

    EXPECT_EQ(red_stats_component.GetAttackDodgeChanceOverflowPercentage(), 88_fp);
    EXPECT_EQ(blue_stats_component.GetCritChanceOverflowPercentage(), 96_fp);

    ASSERT_EQ(8, events_projectiles_moved.size()) << "8 projectiles should have moved";
}

TEST_F(AbilitySystemTestPreviousTarget, ApplyDebufToPreviousTarget)
{
    constexpr auto first_skill_damage = 100_fp;

    // Initialize abilities
    {
        auto& ability = data.type_data.omega_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.name = "Omega Attack";
        skill1.targeting.type = SkillTargetingType::kInZone;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.radius_units = 9;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(first_skill_damage));

        auto& skill2 = ability.AddSkill();
        skill2.name = "Omega Attack Debuff";
        skill2.targeting.type = SkillTargetingType::kPreviousTargetList;
        skill2.deployment.type = SkillDeploymentType::kDirect;
        skill2.AddEffect(EffectData::CreateNegativeState(EffectNegativeState::kStun, 1000));

        // Activate omega at start
        data.type_data.stats.Set(StatType::kStartingEnergy, data.type_data.stats.Get(StatType::kEnergyCost));

        ability.total_duration_ms = 1000;
        skill1.percentage_of_ability_duration = 60;
        skill2.percentage_of_ability_duration = 40;
        skill1.deployment.pre_deployment_delay_percentage = 10;
        skill2.deployment.pre_deployment_delay_percentage = 20;
    }

    SpawnCombatUnits();

    TimeStepForSeconds(1);

    EXPECT_EQ(events_skill_deployed.size(), 2) << "two skills should be deployed";

    // One killed by first skill, two damaged by first and debuffed by second.
    ASSERT_EQ(events_effect_package_received.size(), 5) << "skill should have fire";

    const auto& red_stats_component_1 = red_entity->Get<StatsComponent>();
    const auto& red_stats_component_2 = red_entity2->Get<StatsComponent>();
    const auto& red_stats_component_3 = red_entity3->Get<StatsComponent>();
    const auto& red_stats_component_4 = red_entity4->Get<StatsComponent>();

    ASSERT_EQ(red_stats_component_1.GetCurrentHealth(), 0_fp) << "the entity should be dead";
    ASSERT_EQ(red_stats_component_2.GetCurrentHealth(), red_stats_component_2.GetMaxHealth() - first_skill_damage)
        << "the entity should be damaged";
    ASSERT_EQ(red_stats_component_3.GetCurrentHealth(), red_stats_component_3.GetMaxHealth() - first_skill_damage)
        << "the entity should be damaged";
    ASSERT_EQ(red_stats_component_4.GetCurrentHealth(), red_stats_component_4.GetMaxHealth())
        << "the entity should have full health";

    const auto& red_attached_effects_component_1 = red_entity->Get<AttachedEffectsComponent>();
    const auto& red_attached_effects_component_2 = red_entity2->Get<AttachedEffectsComponent>();
    const auto& red_attached_effects_component_3 = red_entity3->Get<AttachedEffectsComponent>();
    const auto& red_attached_effects_component_4 = red_entity4->Get<AttachedEffectsComponent>();

    ASSERT_FALSE(red_attached_effects_component_1.HasNegativeState(EffectNegativeState::kStun))
        << "the entity should not have debuff";
    ASSERT_TRUE(red_attached_effects_component_2.HasNegativeState(EffectNegativeState::kStun))
        << "the entity should have debuff";
    ASSERT_TRUE(red_attached_effects_component_3.HasNegativeState(EffectNegativeState::kStun))
        << "the entity should have debuff";
    ASSERT_FALSE(red_attached_effects_component_4.HasNegativeState(EffectNegativeState::kStun))
        << "the entity should not have debuff";
}

TEST_F(AbilitySystemTestCombatStatCheckAlly, LowestAllyCurrentHP)
{
    // Initialize the abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();
        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.name = "Omega Attack Chain";
        skill1.targeting.type = SkillTargetingType::kExpressionCheck;
        skill1.targeting.stat_type = StatType::kCurrentHealth;
        skill1.targeting.group = AllegianceType::kAlly;
        skill1.targeting.lowest = true;
        skill1.targeting.num = 2;

        EffectExpression operand1;
        operand1.operation_type = EffectOperationType::kMultiply;
        operand1.operands = {
            EffectExpression::FromStat(
                StatType::kCurrentHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromValue(100_fp)};

        skill1.targeting.expression.operation_type = EffectOperationType::kDivide;
        skill1.targeting.expression.operands = {
            operand1,
            EffectExpression::FromStat(
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender)};

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddEffect(
            EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(120_fp), kTimeInfinite));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    auto& blue2_stat_component = blue_entity2->Get<StatsComponent>();
    auto& blue3_stat_component = blue_entity3->Get<StatsComponent>();
    auto& blue4_stat_component = blue_entity4->Get<StatsComponent>();
    auto& blue5_stat_component = blue_entity5->Get<StatsComponent>();

    blue2_stat_component.GetMutableTemplateStats().Set(StatType::kCurrentHealth, 90_fp);
    blue3_stat_component.GetMutableTemplateStats().Set(StatType::kCurrentHealth, 120_fp);
    blue4_stat_component.GetMutableTemplateStats().Set(StatType::kCurrentHealth, 275_fp);
    blue5_stat_component.GetMutableTemplateStats().Set(StatType::kCurrentHealth, 200_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 2) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, blue_entity3->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, blue_entity5->GetID())
        << "Skill should have hit the entity";
}

TEST_F(AbilitySystemTestTargetingGuidance, GuidanceTargetGround)
{
    // Initialize abilities

    // The first ability attacks all enemies
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kAllegiance;
        skill1.targeting.guidance = MakeEnumSet(GuidanceType::kGround);
        skill1.targeting.group = AllegianceType::kEnemy;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.deployment.guidance = MakeEnumSet(GuidanceType::kGround);
        skill1.AddEffect(
            EffectData::CreateDebuff(StatType::kMaxHealth, EffectExpression::FromValue(50_fp), kTimeInfinite));
    }

    // The second ability heals all allies
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kAllegiance;
        skill1.targeting.guidance = MakeEnumSet(GuidanceType::kGround);
        skill1.targeting.group = AllegianceType::kAlly;
        skill1.targeting.self = true;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.deployment.guidance = MakeEnumSet(GuidanceType::kGround);
        skill1.AddEffect(
            EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(50_fp), kTimeInfinite));
    }

    SpawnCombatUnits();

    // Also make the second blue entity airborne
    {
        const auto effect_data = EffectData::CreatePlaneChange(EffectPlaneChange::kAirborne, 300);
        Entity* target_entity = blue_entity2;
        const EntityID blue_entity_id = target_entity->GetID();
        {
            EffectState effect_state{};
            effect_state.sender_stats = world->GetFullStats(blue_entity_id);
            GetAttachedEffectsHelper().AddAttachedEffect(*target_entity, blue_entity_id, effect_data, effect_state);
        }
    }

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red2_stats_component = red_entity2->Get<StatsComponent>();
    auto& red3_stats_component = red_entity3->Get<StatsComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& blue2_stats_component = blue_entity2->Get<StatsComponent>();

    ASSERT_EQ(red_stats_component.GetMaxHealth(), 200_fp);
    ASSERT_EQ(red2_stats_component.GetMaxHealth(), 300_fp);
    ASSERT_EQ(red3_stats_component.GetMaxHealth(), 400_fp);

    ASSERT_EQ(blue_stats_component.GetMaxHealth(), 50_fp);
    ASSERT_EQ(blue2_stats_component.GetMaxHealth(), 100_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    // Only two red entites must be hit since first red entity is airborne and
    // and targeting guidance of the first skill is set to kGround
    ForcedActivateAbility(*blue_entity, ability_system, blue_abilities_component.GetStateAttackAbilities().at(0));
    ASSERT_EQ(events_effect_package_received.size(), 2) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, red_entity2->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, red_entity3->GetID())
        << "Skill should have hit the entity";

    ASSERT_EQ(red_stats_component.GetMaxHealth(), 200_fp);
    ASSERT_EQ(red2_stats_component.GetMaxHealth(), 250_fp);
    ASSERT_EQ(red3_stats_component.GetMaxHealth(), 350_fp);

    // All ally entities must be healed no matter on which plane they currently reside
    events_effect_package_received.clear();
    ForcedActivateAbility(*blue_entity, ability_system, blue_abilities_component.GetStateAttackAbilities().at(1));
    ASSERT_EQ(events_effect_package_received.size(), 2) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, blue_entity->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, blue_entity2->GetID())
        << "Skill should have hit the entity";
    ASSERT_EQ(blue_stats_component.GetMaxHealth(), 100_fp);
    ASSERT_EQ(blue2_stats_component.GetMaxHealth(), 150_fp);
}

TEST_F(AbilitySystemTestTargetingGuidance, GuidanceTargetAirborne)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kAllegiance;
        skill1.targeting.guidance = MakeEnumSet(GuidanceType::kGround, GuidanceType::kAirborne);
        skill1.targeting.group = AllegianceType::kEnemy;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.deployment.guidance = MakeEnumSet(GuidanceType::kGround, GuidanceType::kAirborne);
        skill1.AddEffect(
            EffectData::CreateDebuff(StatType::kMaxHealth, EffectExpression::FromValue(50_fp), kTimeInfinite));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red2_stats_component = red_entity2->Get<StatsComponent>();
    auto& red3_stats_component = red_entity3->Get<StatsComponent>();

    ASSERT_EQ(red_stats_component.GetMaxHealth(), 200_fp);
    ASSERT_EQ(red2_stats_component.GetMaxHealth(), 300_fp);
    ASSERT_EQ(red3_stats_component.GetMaxHealth(), 400_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 3) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, red_entity->GetID()) << "Skill should have hit the entity";

    ASSERT_EQ(red_stats_component.GetMaxHealth(), 150_fp);
    ASSERT_EQ(red2_stats_component.GetMaxHealth(), 250_fp);
    ASSERT_EQ(red3_stats_component.GetMaxHealth(), 350_fp);
}

TEST_F(AbilitySystemTestTargetingGuidance, GuidanceTargetAny)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kAllegiance;
        skill1.targeting.guidance =
            MakeEnumSet(GuidanceType::kGround, GuidanceType::kAirborne, GuidanceType::kUnderground);
        skill1.targeting.group = AllegianceType::kEnemy;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.deployment.guidance =
            MakeEnumSet(GuidanceType::kGround, GuidanceType::kAirborne, GuidanceType::kUnderground);
        skill1.AddEffect(
            EffectData::CreateDebuff(StatType::kMaxHealth, EffectExpression::FromValue(50_fp), kTimeInfinite));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red2_stats_component = red_entity2->Get<StatsComponent>();
    auto& red3_stats_component = red_entity3->Get<StatsComponent>();

    ASSERT_EQ(red_stats_component.GetMaxHealth(), 200_fp);
    ASSERT_EQ(red2_stats_component.GetMaxHealth(), 300_fp);
    ASSERT_EQ(red3_stats_component.GetMaxHealth(), 400_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 3) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, red_entity->GetID()) << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, red_entity2->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[2].receiver_id, red_entity3->GetID())
        << "Skill should have hit the entity";

    ASSERT_EQ(red_stats_component.GetMaxHealth(), 150_fp);
    ASSERT_EQ(red2_stats_component.GetMaxHealth(), 250_fp);
    ASSERT_EQ(red3_stats_component.GetMaxHealth(), 350_fp);
}

TEST_F(AbilitySystemTestCombatStatCheckAlly, LowestAllyCurrentHPNoAllies)
{
    // Initialize the abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();
        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.name = "Omega Attack Chain";
        skill1.targeting.type = SkillTargetingType::kExpressionCheck;
        skill1.targeting.stat_type = StatType::kCurrentHealth;
        skill1.targeting.group = AllegianceType::kAlly;
        skill1.targeting.lowest = true;
        skill1.targeting.num = 2;
        skill1.targeting.self = true;

        EffectExpression operand1;
        operand1.operation_type = EffectOperationType::kMultiply;
        operand1.operands = {
            EffectExpression::FromStat(
                StatType::kCurrentHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromValue(100_fp)};

        skill1.targeting.expression.operation_type = EffectOperationType::kDivide;
        skill1.targeting.expression.operands = {
            operand1,
            EffectExpression::FromStat(
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender)};

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddEffect(EffectData::CreateHeal(EffectHealType::kNormal, EffectExpression::FromValue(120_fp)));
    }

    SpawnSingleCombatUnit();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, 500_fp);
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 50_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    // No allies on the field but the ability should still hit self
    ASSERT_EQ(events_effect_package_received.size(), 1) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, blue_entity->GetID())
        << "Skill should have hit the entity";

    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 170_fp);
}

TEST_F(AbilitySystemTestAllEnemiesWithinX, SpreadEffectPackage)
{
    // Initialize abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kInZone;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.radius_units = 50;
        skill1.spread_effect_package = true;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(150_fp));
    }

    SpawnCombatUnits();

    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& red2_stats_component = red_entity2->Get<StatsComponent>();
    auto& red3_stats_component = red_entity3->Get<StatsComponent>();

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 200_fp);
    EXPECT_EQ(red2_stats_component.GetCurrentHealth(), 300_fp);
    EXPECT_EQ(red3_stats_component.GetCurrentHealth(), 400_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_EQ(events_effect_package_received.size(), 3) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, red_entity->GetID()) << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, red_entity2->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[2].receiver_id, red_entity3->GetID())
        << "Skill should have hit the entity";

    // 50 damage should be done to red_entity and red_entity2 since the 100 effect damage is spread evenly
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 150_fp);
    EXPECT_EQ(red2_stats_component.GetCurrentHealth(), 250_fp);
    EXPECT_EQ(red3_stats_component.GetCurrentHealth(), 350_fp);
}

TEST_F(AbilitySystemRetargetingTest, MultipleTargets)
{
    constexpr size_t target_number = 2;

    InitAbility(target_number);
    SpawnCombatUnits();

    const auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // 1. First time step, activates ability and init targeting
    world->TimeStep();

    // Get active ability and skill
    const auto& active_ability = blue_abilities_component.GetActiveAbility();
    const auto& active_skill = blue_abilities_component.GetActiveSkill();

    ASSERT_TRUE(active_ability.get());
    ASSERT_TRUE(active_skill);

    ASSERT_EQ(active_ability->total_current_time_ms, 0) << "After first time step ability time is 0";
    ASSERT_TRUE(active_ability->CanRetargetCurrentSkill()) << "We allowed to retarget skill at first time step";

    // Three valid targets
    ASSERT_EQ(active_skill->targeting_state.GetTotalTargetsSize(), target_number);
    ASSERT_EQ(active_skill->targeting_state.GetAvailableTargetsVector().size(), target_number);
    ASSERT_EQ(active_skill->targeting_state.GetLostTargetsLastKnownPosition().size(), 0);

    // Kill first red entity
    EntityID entity_to_kill = red_entity->GetID();
    ASSERT_TRUE(VectorHelper::ContainsValue(active_skill->targeting_state.GetAvailableTargetsVector(), entity_to_kill));
    KillEntity(blue_entity->GetID(), entity_to_kill);
    ASSERT_FALSE(
        VectorHelper::ContainsValue(active_skill->targeting_state.GetAvailableTargetsVector(), entity_to_kill));

    // Three valid targets
    ASSERT_EQ(active_skill->targeting_state.GetTotalTargetsSize(), target_number);
    ASSERT_EQ(active_skill->targeting_state.GetAvailableTargetsVector().size(), target_number);
    ASSERT_EQ(active_skill->targeting_state.GetLostTargetsLastKnownPosition().size(), 0);

    // 2. Step ability to 100 from 300 ms of duration
    world->TimeStep();

    ASSERT_EQ(active_ability->total_current_time_ms, 100) << "After second time step ability time is 100";
    ASSERT_TRUE(active_ability->CanRetargetCurrentSkill())
        << "Skill is allowed to retarget skill after second time step";

    // Kill second red entity
    entity_to_kill = red_entity3->GetID();
    ASSERT_TRUE(VectorHelper::ContainsValue(active_skill->targeting_state.GetAvailableTargetsVector(), entity_to_kill));
    KillEntity(blue_entity->GetID(), entity_to_kill);
    ASSERT_FALSE(
        VectorHelper::ContainsValue(active_skill->targeting_state.GetAvailableTargetsVector(), entity_to_kill));

    ASSERT_EQ(active_skill->targeting_state.GetTotalTargetsSize(), target_number);
    ASSERT_EQ(active_skill->targeting_state.GetAvailableTargetsVector().size(), target_number);
    ASSERT_EQ(active_skill->targeting_state.GetLostTargetsLastKnownPosition().size(), 0);

    // 3. Step ability to 200 from 300 ms of duration
    world->TimeStep();

    ASSERT_EQ(active_ability->total_current_time_ms, 200) << "After third time step ability time is 100";
    ASSERT_FALSE(active_ability->CanRetargetCurrentSkill()) << "Skill is not allowed to retarget skill after 50%";

    // Kill third red entity
    entity_to_kill = red_entity2->GetID();
    ASSERT_TRUE(VectorHelper::ContainsValue(active_skill->targeting_state.GetAvailableTargetsVector(), entity_to_kill));
    KillEntity(blue_entity->GetID(), entity_to_kill);
    ASSERT_FALSE(
        VectorHelper::ContainsValue(active_skill->targeting_state.GetAvailableTargetsVector(), entity_to_kill));

    ASSERT_EQ(active_skill->targeting_state.GetTotalTargetsSize(), target_number);
    ASSERT_EQ(active_skill->targeting_state.GetAvailableTargetsVector().size(), target_number - 1);
    ASSERT_EQ(active_skill->targeting_state.GetLostTargetsLastKnownPosition().size(), 1);
    ASSERT_EQ(active_skill->targeting_state.targets_saved_data.begin()->first, entity_to_kill);

    // 4. Step ability to 300 from 300 ms of duration
    world->TimeStep();
    ASSERT_EQ(active_skill->state, SkillStateType::kDeploying);

    ASSERT_EQ(events_effect_package_received.size(), 2) << "skill should have fire";
    EXPECT_EQ(events_effect_package_received[0].receiver_id, red_entity4->GetID())
        << "Skill should have hit the entity";
    EXPECT_EQ(events_effect_package_received[1].receiver_id, kInvalidEntityID)
        << "Skill should have hit the fallback location";
}

TEST_F(AbilitySystemRetargetingTest, SingleTargets)
{
    constexpr size_t target_number = 1;

    InitAbility(target_number);
    SpawnCombatUnits();

    const auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // 1. First time step, activates ability and init targeting
    world->TimeStep();

    // Get active ability and skill
    const auto& active_ability = blue_abilities_component.GetActiveAbility();
    const auto& active_skill = blue_abilities_component.GetActiveSkill();

    ASSERT_TRUE(active_ability.get());
    ASSERT_TRUE(active_skill);

    ASSERT_EQ(active_ability->total_current_time_ms, 0) << "After first time step ability time is 0";
    ASSERT_TRUE(active_ability->CanRetargetCurrentSkill()) << "We allowed to retarget skill at first time step";

    // Three valid targets
    ASSERT_EQ(active_skill->targeting_state.GetTotalTargetsSize(), target_number);
    ASSERT_EQ(active_skill->targeting_state.GetAvailableTargetsVector().size(), target_number);
    ASSERT_EQ(active_skill->targeting_state.GetLostTargetsLastKnownPosition().size(), 0);

    // Kill first red entity
    EntityID entity_to_kill = red_entity3->GetID();
    ASSERT_TRUE(VectorHelper::ContainsValue(active_skill->targeting_state.GetAvailableTargetsVector(), entity_to_kill));
    KillEntity(blue_entity->GetID(), entity_to_kill);
    ASSERT_FALSE(
        VectorHelper::ContainsValue(active_skill->targeting_state.GetAvailableTargetsVector(), entity_to_kill));

    // Three valid targets
    ASSERT_EQ(active_skill->targeting_state.GetTotalTargetsSize(), target_number);
    ASSERT_EQ(active_skill->targeting_state.GetAvailableTargetsVector().size(), target_number);
    ASSERT_EQ(active_skill->targeting_state.GetLostTargetsLastKnownPosition().size(), 0);

    // 2. Step ability to 100 from 300 ms of duration
    world->TimeStep();

    ASSERT_EQ(active_ability->total_current_time_ms, 100) << "After second time step ability time is 100";
    ASSERT_TRUE(active_ability->CanRetargetCurrentSkill())
        << "Skill is allowed to retarget skill after second time step";

    // Kill second red entity
    entity_to_kill = red_entity->GetID();
    ASSERT_TRUE(VectorHelper::ContainsValue(active_skill->targeting_state.GetAvailableTargetsVector(), entity_to_kill));
    KillEntity(blue_entity->GetID(), entity_to_kill);
    ASSERT_FALSE(
        VectorHelper::ContainsValue(active_skill->targeting_state.GetAvailableTargetsVector(), entity_to_kill));

    ASSERT_EQ(active_skill->targeting_state.GetTotalTargetsSize(), target_number);
    ASSERT_EQ(active_skill->targeting_state.GetAvailableTargetsVector().size(), target_number);
    ASSERT_EQ(active_skill->targeting_state.GetLostTargetsLastKnownPosition().size(), 0);

    // 3. Step ability to 200 from 300 ms of duration
    world->TimeStep();

    ASSERT_EQ(active_ability->total_current_time_ms, 200) << "After third time step ability time is 100";
    ASSERT_FALSE(active_ability->CanRetargetCurrentSkill()) << "Skill is not allowed to retarget skill after 50%";

    // Kill third red entity
    entity_to_kill = red_entity4->GetID();
    ASSERT_TRUE(VectorHelper::ContainsValue(active_skill->targeting_state.GetAvailableTargetsVector(), entity_to_kill));
    KillEntity(blue_entity->GetID(), entity_to_kill);
    ASSERT_FALSE(
        VectorHelper::ContainsValue(active_skill->targeting_state.GetAvailableTargetsVector(), entity_to_kill));

    ASSERT_EQ(active_skill->targeting_state.GetTotalTargetsSize(), target_number);
    ASSERT_EQ(active_skill->targeting_state.GetAvailableTargetsVector().size(), target_number - 1);
    ASSERT_EQ(active_skill->targeting_state.GetLostTargetsLastKnownPosition().size(), 1);
    ASSERT_EQ(active_skill->targeting_state.targets_saved_data.begin()->first, entity_to_kill);

    // Ability is not interrupted, because direct deployment can be deployed to locations
    ASSERT_EQ(active_skill->state, SkillStateType::kWaiting);
    ASSERT_EQ(events_ability_interrupted.size(), 0) << "skill should not be interrupted";
}

class AbilitySystemCurrentFocusersTest : public BaseTest
{
public:
    static CombatUnitData MakeCombatUnitData()
    {
        CombatUnitData unit_data = CreateCombatUnitData();
        unit_data.type_id.line_name = "default";
        unit_data.radius_units = 2;
        unit_data.type_data.combat_affinity = CombatAffinity::kFire;
        unit_data.type_data.combat_class = CombatClass::kBehemoth;
        unit_data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 300_fp);
        unit_data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);             // No critical attacks
        unit_data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);      // No Dodge
        unit_data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);  // No Miss
        unit_data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);                   // Once every tick
        unit_data.type_data.stats.Set(
            StatType::kAttackRangeUnits,
            1000_fp);  // Very big attack radius so no need to move
        unit_data.type_data.stats.Set(StatType::kOmegaRangeUnits, 1000_fp);  // Very big omega radius so no need to move
        unit_data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);
        unit_data.type_data.stats.Set(StatType::kEnergyCost, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP);
        unit_data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);  // don't move
        return unit_data;
    }

    Entity* Spawn(const Team team, const CombatUnitData& unit_data, const HexGridPosition position)
    {
        Entity* entity = nullptr;
        SpawnCombatUnit(team, position, unit_data, entity);
        return entity;
    }

    Entity* SpawnPet(const Entity* parent, const CombatUnitData& unit_data, const HexGridPosition position)
    {
        FullCombatUnitData full_data;
        full_data.data = unit_data;
        full_data.instance = CreateCombatUnitInstanceData();
        full_data.instance.team = parent->GetTeam();
        full_data.instance.position = position;

        std::string error_message;
        Entity* pet = EntityFactory::SpawnCombatUnit(*world, full_data, parent->GetID(), &error_message);
        if (!pet)
        {
            world->LogErr("Failed to spawn pet. {}", error_message);
        }
        return pet;
    }

    static bool NotNull(const Entity* entity)
    {
        return entity != nullptr;
    }
};

TEST_F(AbilitySystemCurrentFocusersTest, CurrentFocusers)
{
    // Red entities positions at uniform intervals from one another
    const std::vector<HexGridPosition> red_entities_positions{
        {-60, 6},
        {-30, 6},
        {0, 6},
        {30, 6},
        {60, 6},
    };

    // Create blue entities in such a manner that two blue entities are assigned to focus on each red entity
    const HexGridPosition step_left{3, 0};
    const HexGridPosition step_right{-3, 0};
    const auto blue_data = [&]()
    {
        auto data = MakeCombatUnitData();
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
        return data;
    }();

    std::vector<Entity*> blue_entities;
    blue_entities.reserve(red_entities_positions.size() * 2);
    for (const HexGridPosition& red_position : red_entities_positions)
    {
        const auto reflected = Reflect(red_position);
        blue_entities.push_back(Spawn(Team::kBlue, blue_data, reflected + step_left));
        blue_entities.push_back(Spawn(Team::kBlue, blue_data, reflected + step_right));
    }
    ASSERT_TRUE(std::all_of(blue_entities.begin(), blue_entities.end(), NotNull));

    // Spawn red entities
    const auto red_data = [&]()
    {
        auto data = MakeCombatUnitData();
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
        auto& abilities = data.type_data.attack_abilities;
        auto& ability = abilities.AddAbility();
        auto& skill = ability.AddSkill();
        skill.targeting.type = SkillTargetingType::kInZone;
        skill.targeting.radius_units = 50;
        skill.targeting.num = 10;
        skill.targeting.only_current_focusers = true;
        skill.targeting.group = AllegianceType::kAll;
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(1_fp));
        return data;
    }();
    std::vector<Entity*> red_entities;
    red_entities.reserve(red_entities_positions.size());
    for (const HexGridPosition& position : red_entities_positions)
    {
        red_entities.push_back(Spawn(Team::kRed, red_data, position));
    }
    ASSERT_TRUE(std::all_of(red_entities.begin(), red_entities.end(), NotNull));

    world->TimeStep();
    // gather focusers data
    std::unordered_map<EntityID, std::vector<EntityID>> focused_by;
    for (const auto& entity_ptr : world->GetAll())
    {
        if (EntityHelper::IsACombatUnit(*entity_ptr))
        {
            auto& focus_component = entity_ptr->Get<FocusComponent>();
            if (const EntityID focus_id = focus_component.GetFocusID(); focus_id != kInvalidEntityID)
            {
                focused_by[focus_id].push_back(entity_ptr->GetID());
            }
        }
    }

    // Ensure that each red entity is focused by two blue entities
    for (const auto red_entity : red_entities)
    {
        const auto red_entity_id = red_entity->GetID();
        ASSERT_EQ(focused_by[red_entity_id].size(), 2) << "entity id: " << red_entity_id;
    }

    EventHistory<EventType::kOnDamage> on_damage(*world);
    auto sort_damage_events = [&]()
    {
        std::sort(
            on_damage.events.begin(),
            on_damage.events.end(),
            [](const event_data::AppliedDamage& a, const event_data::AppliedDamage& b)
            {
                if (a.sender_id == b.sender_id) return a.receiver_id < b.receiver_id;
                return a.sender_id < b.sender_id;
            });
    };
    world->TimeStep();
    sort_damage_events();

    // Ensure that each red entity deals damage to its two focusers
    ASSERT_EQ(on_damage.Size(), blue_entities.size());
    for (size_t i = 0; i != blue_entities.size(); ++i)
    {
        const auto& event = on_damage.events[i];
        ASSERT_EQ(event.sender_id, red_entities[i / 2]->GetID());
        ASSERT_EQ(event.receiver_id, blue_entities[i]->GetID());
    }

    // Spawn red pets - the same combat units but pets
    constexpr HexGridPosition pet_offset = {0, 6};
    const auto red_pet_data = [&]()
    {
        auto data = red_data;
        data.type_id.type = CombatUnitType::kPet;
        return data;
    }();
    std::vector<Entity*> red_pets;
    red_pets.reserve(red_entities_positions.size());
    for (size_t i = 0; i != red_entities_positions.size(); ++i)
    {
        const auto parent = red_entities[i];
        const auto position = red_entities_positions[i] + pet_offset;
        red_pets.push_back(SpawnPet(parent, red_pet_data, position));
    }
    ASSERT_TRUE(std::all_of(red_pets.begin(), red_pets.end(), NotNull));

    on_damage.Clear();
    world->TimeStep();
    sort_damage_events();

    ASSERT_EQ(on_damage.Size(), blue_entities.size() * 2);

    // Ensure that each red combat unit deals damage to its two focusers
    for (size_t i = 0; i != blue_entities.size(); ++i)
    {
        const auto& event = on_damage.events[i];
        ASSERT_EQ(event.sender_id, red_entities[i / 2]->GetID());
        ASSERT_EQ(event.receiver_id, blue_entities[i]->GetID());
    }

    // Ensure that each red pet deals damage to those who focuses its parent
    for (size_t i = 0; i != blue_entities.size(); ++i)
    {
        const auto& event = on_damage.events[blue_entities.size() + i];
        ASSERT_EQ(event.sender_id, red_pets[i / 2]->GetID());
        ASSERT_EQ(event.receiver_id, blue_entities[i]->GetID());
    }
}

class AbilitySystemTierTargetingTest : public BaseTest
{
public:
    uint64_t GetWorldRandomSeed() const override
    {
        return 2;
    }

    static CombatUnitData MakeCombatUnitData(int tier)
    {
        CombatUnitData unit_data;
        unit_data.type_id.line_name = fmt::format("default_{}", tier);
        unit_data.type_data.tier = tier;
        unit_data.radius_units = 2;
        unit_data.type_data.combat_affinity = CombatAffinity::kFire;
        unit_data.type_data.combat_class = CombatClass::kBehemoth;
        unit_data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 300_fp);
        unit_data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);             // No critical attacks
        unit_data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);      // No Dodge
        unit_data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);  // No Miss
        unit_data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);                   // Once every tick
        unit_data.type_data.stats.Set(
            StatType::kAttackRangeUnits,
            1000_fp);  // Very big attack radius so no need to move
        unit_data.type_data.stats.Set(StatType::kOmegaRangeUnits, 1000_fp);  // Very big omega radius so no need to move
        unit_data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);
        unit_data.type_data.stats.Set(StatType::kEnergyCost, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP);
        unit_data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);  // don't move
        return unit_data;
    }

    static DroneAugmentData MakeTargetingDroneData(int tier, AllegianceType allegiance, size_t max = 0)
    {
        DroneAugmentData drone_data;

        drone_data.type_id.name = fmt::format("Test_{}_{}_{}", tier, allegiance, max);
        drone_data.type_id.stage = 1;
        drone_data.drone_augment_type = DroneAugmentType::kSimulation;
        auto& abilities = drone_data.innate_abilities;
        abilities.selection_type = AbilitySelectionType::kCycle;
        auto& ability = abilities.AddAbility();
        ability.name = "Targeting skill";
        ability.activation_trigger_data.trigger_type = ActivationTriggerType::kOnBattleStart;
        ability.total_duration_ms = 2000;

        auto& skill = ability.AddSkill();
        skill.targeting.type = SkillTargetingType::kTier;
        skill.targeting.num = max;
        skill.targeting.tier = tier;
        skill.targeting.group = allegiance;
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.AddEffect(EffectData::CreateHeal(EffectHealType::kNormal, EffectExpression::FromValue(10_fp)));
        return drone_data;
    }

    Entity* SpawnDroneAugment(Team team, const DroneAugmentData& data) const
    {
        EntityFactory::SpawnDroneAugmentEntity(*world, data, team);

        std::unordered_map<EntityID, std::vector<EntityID>> focused_by;
        for (const auto& entity_ptr : world->GetAll())
        {
            if (EntityHelper::IsADroneAugment(*entity_ptr))
            {
                const DroneAugmentEntityComponent& component = entity_ptr->Get<DroneAugmentEntityComponent>();
                if (component.GetDroneAugmentTypeID() == data.type_id)
                {
                    return entity_ptr.get();
                }
            }
        }

        return nullptr;
    }
};

TEST_F(AbilitySystemTierTargetingTest, TierTargeting)
{
    // Position, Team, Tier,
    using UnitData = std::tuple<HexGridPosition, Team, int>;
    const std::vector<UnitData> test_entities{
        {{-60, 6}, Team::kRed, 0},
        {{-30, 6}, Team::kRed, 0},
        {{0, 6}, Team::kRed, 0},
        {{30, 6}, Team::kRed, 1},
        {{60, 6}, Team::kRed, 2},
        {{0, 20}, Team::kBlue, 0},
        {{0, -20}, Team::kBlue, 3},
        {{10, -20}, Team::kBlue, 3},
    };

    GameDataContainer& game_data_container = const_cast<GameDataContainer&>(world->GetGameDataContainer());

    for (const UnitData& data : test_entities)
    {
        const int unit_tier = std::get<2>(data);
        auto unit_data = MakeCombatUnitData(unit_tier);

        game_data_container.AddCombatUnitData(unit_data.type_id, std::make_shared<CombatUnitData>(unit_data));

        Entity* entity = nullptr;
        SpawnCombatUnit(std::get<1>(data), std::get<0>(data), unit_data, entity);
    }

    ASSERT_EQ(world->GetAll().size(), test_entities.size());

    const auto drone_data_ally_1 = MakeTargetingDroneData(1, AllegianceType::kAlly);
    Entity* targeting_entity_ally_1 = SpawnDroneAugment(Team::kRed, drone_data_ally_1);

    const auto drone_data_ally_0 = MakeTargetingDroneData(0, AllegianceType::kAlly);
    Entity* targeting_entity_ally_0 = SpawnDroneAugment(Team::kRed, drone_data_ally_0);

    const auto drone_data_ally_0_max2 = MakeTargetingDroneData(0, AllegianceType::kAlly, 2);
    Entity* targeting_entity_ally_0_max2 = SpawnDroneAugment(Team::kRed, drone_data_ally_0_max2);

    const auto drone_data_enemy_3 = MakeTargetingDroneData(3, AllegianceType::kEnemy);
    Entity* targeting_entity_enemy_3 = SpawnDroneAugment(Team::kRed, drone_data_enemy_3);

    const auto drone_data_all_0 = MakeTargetingDroneData(0, AllegianceType::kAll);
    Entity* targeting_entity_all_0 = SpawnDroneAugment(Team::kBlue, drone_data_all_0);

    world->TimeStep();

    // Verify 1 1-tier in same team
    {
        const auto& targeting_abilities_component = targeting_entity_ally_1->Get<AbilitiesComponent>();
        const auto& active_skill = targeting_abilities_component.GetActiveSkill();
        ASSERT_NE(active_skill, nullptr);
        ASSERT_EQ(active_skill->targeting_state.GetTotalTargetsSize(), 1);
    }

    // Verify 3 0-tier in same team
    {
        const auto& targeting_abilities_component = targeting_entity_ally_0->Get<AbilitiesComponent>();
        const auto& active_skill = targeting_abilities_component.GetActiveSkill();
        ASSERT_NE(active_skill, nullptr);
        ASSERT_EQ(active_skill->targeting_state.GetTotalTargetsSize(), 3);
    }

    // Verify selected 1 0-tier in same team
    {
        const auto& targeting_abilities_component = targeting_entity_ally_0_max2->Get<AbilitiesComponent>();
        const auto& active_skill = targeting_abilities_component.GetActiveSkill();
        ASSERT_NE(active_skill, nullptr);

        const std::vector<EntityID> expected_targets{0, 2};
        ASSERT_EQ(active_skill->targeting_state.GetTotalTargetsSize(), expected_targets.size());
        ASSERT_EQ(active_skill->targeting_state.GetAvailableTargetsVector(), expected_targets);
    }

    // Verify 2 3-tier in enemy team
    {
        const auto& targeting_abilities_component = targeting_entity_enemy_3->Get<AbilitiesComponent>();
        const auto& active_skill = targeting_abilities_component.GetActiveSkill();
        ASSERT_NE(active_skill, nullptr);
        ASSERT_EQ(active_skill->targeting_state.GetTotalTargetsSize(), 2);
    }

    // Verify 4 0-tier in both teams
    {
        const auto& targeting_abilities_component = targeting_entity_all_0->Get<AbilitiesComponent>();
        const auto& active_skill = targeting_abilities_component.GetActiveSkill();
        ASSERT_NE(active_skill, nullptr);
        ASSERT_EQ(active_skill->targeting_state.GetTotalTargetsSize(), 4);
    }
}

}  // namespace simulation
