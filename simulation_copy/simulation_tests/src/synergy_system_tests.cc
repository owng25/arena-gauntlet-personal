#include <memory>

#include "base_test_fixtures.h"
#include "components/augment_component.h"
#include "components/stats_component.h"
#include "data/combat_synergy_bonus.h"
#include "data/combat_unit_type_id.h"
#include "ecs/world.h"
#include "gtest/gtest.h"
#include "systems/synergy_system.h"
#include "utility/hex_grid_position.h"

namespace simulation
{
class SynergySystemTest : public BaseTest
{
    using Super = BaseTest;

protected:
    void SetUp() override
    {
        Super::SetUp();

        // Listen to events
        world->SubscribeToEvent(
            EventType::kBattleStarted,
            [this](const Event& event)
            {
                events_on_game_started.emplace_back(event.Get<event_data::BattleStarted>());
            });
    }
    bool UseSynergiesData() const override
    {
        return true;
    }

    void TestOneWater(const FixedPoint after_expected_energy_regeneration)
    {
        CombatUnitData data1 = CreateCombatUnitData();
        data1.type_id = CombatUnitTypeID("Axolotl", 3);
        data1.type_data.combat_affinity = CombatAffinity::kWater;
        data1.type_data.combat_class = CombatClass::kFighter;
        data1.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

        // Spawn the CombatUnit
        Entity* entity1 = nullptr;
        SpawnCombatUnit(Team::kBlue, {10, 10}, data1, entity1);

        // Before
        StatsData entity1_live_stats = world->GetLiveStats(entity1->GetID());
        EXPECT_EQ(events_on_game_started.size(), 0);
        EXPECT_EQ(entity1_live_stats.Get(StatType::kEnergyRegeneration), 0_fp);

        // Fire events
        world->TimeStep();

        // After, nothing should have happened
        entity1_live_stats = world->GetLiveStats(entity1->GetID());
        EXPECT_EQ(events_on_game_started.size(), 1);
        EXPECT_EQ(entity1_live_stats.Get(StatType::kEnergyRegeneration), after_expected_energy_regeneration);
    }

    void TestTwoWater(const FixedPoint after_expected_energy_regeneration)
    {
        CombatUnitData data1 = CreateCombatUnitData();
        data1.type_id = CombatUnitTypeID("Axolotl", 3);
        data1.type_data.combat_affinity = CombatAffinity::kWater;
        data1.type_data.combat_class = CombatClass::kFighter;
        data1.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

        CombatUnitData data2 = CreateCombatUnitData();
        data2.type_id = CombatUnitTypeID("Turtle", 3);
        data2.type_data.combat_affinity = CombatAffinity::kWater;
        data2.type_data.combat_class = CombatClass::kFighter;
        data2.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

        // Spawn the CombatUnit
        Entity* entity1 = nullptr;
        SpawnCombatUnit(Team::kBlue, {10, 10}, data1, entity1);
        Entity* entity2 = nullptr;
        SpawnCombatUnit(Team::kBlue, {-10, -10}, data2, entity2);

        // Before
        StatsData entity1_live_stats = world->GetLiveStats(entity1->GetID());
        StatsData entity2_live_stats = world->GetLiveStats(entity2->GetID());
        EXPECT_EQ(events_on_game_started.size(), 0);
        EXPECT_EQ(entity1_live_stats.Get(StatType::kEnergyRegeneration), 0_fp);
        EXPECT_EQ(entity2_live_stats.Get(StatType::kEnergyRegeneration), 0_fp);

        // Fire events
        world->TimeStep();

        // After
        entity1_live_stats = world->GetLiveStats(entity1->GetID());
        entity2_live_stats = world->GetLiveStats(entity2->GetID());
        EXPECT_EQ(events_on_game_started.size(), 1);
        EXPECT_EQ(entity1_live_stats.Get(StatType::kEnergyRegeneration), after_expected_energy_regeneration);
        EXPECT_EQ(entity2_live_stats.Get(StatType::kEnergyRegeneration), after_expected_energy_regeneration);
    }

    void TestThreeWater(const FixedPoint after_expected_energy_regeneration)
    {
        CombatUnitData data1 = CreateCombatUnitData();
        data1.type_id = CombatUnitTypeID("Axolotl", 3);
        data1.type_data.combat_affinity = CombatAffinity::kWater;
        data1.type_data.combat_class = CombatClass::kFighter;
        data1.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

        CombatUnitData data2 = CreateCombatUnitData();
        data2.type_id = CombatUnitTypeID("Turtle", 3);
        data2.type_data.combat_affinity = CombatAffinity::kWater;
        data2.type_data.combat_class = CombatClass::kFighter;
        data2.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

        CombatUnitData data3 = CreateCombatUnitData();
        data3.type_id = CombatUnitTypeID("Pig", 3);
        data3.type_data.combat_affinity = CombatAffinity::kWater;
        data3.type_data.combat_class = CombatClass::kFighter;
        data3.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

        // Spawn the CombatUnit
        Entity* entity1 = nullptr;
        SpawnCombatUnit(Team::kBlue, {10, 10}, data1, entity1);
        Entity* entity2 = nullptr;
        SpawnCombatUnit(Team::kBlue, {-10, -10}, data2, entity2);
        Entity* entity3 = nullptr;
        SpawnCombatUnit(Team::kBlue, {5, 5}, data3, entity3);

        // Before
        StatsData entity1_live_stats = world->GetLiveStats(entity1->GetID());
        StatsData entity2_live_stats = world->GetLiveStats(entity2->GetID());
        StatsData entity3_live_stats = world->GetLiveStats(entity3->GetID());
        EXPECT_EQ(events_on_game_started.size(), 0);
        EXPECT_EQ(entity1_live_stats.Get(StatType::kEnergyRegeneration), 0_fp);
        EXPECT_EQ(entity2_live_stats.Get(StatType::kEnergyRegeneration), 0_fp);
        EXPECT_EQ(entity3_live_stats.Get(StatType::kEnergyRegeneration), 0_fp);

        // Fire events
        world->TimeStep();

        // After
        entity1_live_stats = world->GetLiveStats(entity1->GetID());
        entity2_live_stats = world->GetLiveStats(entity2->GetID());
        entity3_live_stats = world->GetLiveStats(entity3->GetID());
        EXPECT_EQ(events_on_game_started.size(), 1);
        EXPECT_EQ(entity1_live_stats.Get(StatType::kEnergyRegeneration), after_expected_energy_regeneration);
        EXPECT_EQ(entity2_live_stats.Get(StatType::kEnergyRegeneration), after_expected_energy_regeneration);
        EXPECT_EQ(entity3_live_stats.Get(StatType::kEnergyRegeneration), after_expected_energy_regeneration);
    }

    void TestFourWater(const FixedPoint after_expected_energy_regeneration)
    {
        CombatUnitData data1 = CreateCombatUnitData();
        data1.type_id = CombatUnitTypeID("Axolotl", 3);
        data1.type_data.combat_affinity = CombatAffinity::kWater;
        data1.type_data.combat_class = CombatClass::kFighter;
        data1.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

        CombatUnitData data2 = CreateCombatUnitData();
        data2.type_id = CombatUnitTypeID("Turtle", 3);
        data2.type_data.combat_affinity = CombatAffinity::kWater;
        data2.type_data.combat_class = CombatClass::kFighter;
        data2.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

        CombatUnitData data3 = CreateCombatUnitData();
        data3.type_id = CombatUnitTypeID("Pig", 3);
        data3.type_data.combat_affinity = CombatAffinity::kWater;
        data3.type_data.combat_class = CombatClass::kFighter;
        data3.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

        CombatUnitData data4 = CreateCombatUnitData();
        data4.type_id = CombatUnitTypeID("SkyPig", 3);
        data4.type_data.combat_affinity = CombatAffinity::kWater;
        data4.type_data.combat_class = CombatClass::kFighter;
        data4.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

        // Spawn the CombatUnit
        Entity* entity1 = nullptr;
        SpawnCombatUnit(Team::kBlue, {10, 10}, data1, entity1);
        Entity* entity2 = nullptr;
        SpawnCombatUnit(Team::kBlue, {-10, -10}, data2, entity2);
        Entity* entity3 = nullptr;
        SpawnCombatUnit(Team::kBlue, {5, 5}, data3, entity3);
        Entity* entity4 = nullptr;
        SpawnCombatUnit(Team::kBlue, {-5, -5}, data4, entity4);

        // Before
        StatsData entity1_live_stats = world->GetLiveStats(entity1->GetID());
        StatsData entity2_live_stats = world->GetLiveStats(entity2->GetID());
        StatsData entity3_live_stats = world->GetLiveStats(entity3->GetID());
        StatsData entity4_live_stats = world->GetLiveStats(entity4->GetID());
        EXPECT_EQ(events_on_game_started.size(), 0);
        EXPECT_EQ(entity1_live_stats.Get(StatType::kEnergyRegeneration), 0_fp);
        EXPECT_EQ(entity2_live_stats.Get(StatType::kEnergyRegeneration), 0_fp);
        EXPECT_EQ(entity3_live_stats.Get(StatType::kEnergyRegeneration), 0_fp);
        EXPECT_EQ(entity4_live_stats.Get(StatType::kEnergyRegeneration), 0_fp);

        // Fire events
        world->TimeStep();

        // After
        entity1_live_stats = world->GetLiveStats(entity1->GetID());
        entity2_live_stats = world->GetLiveStats(entity2->GetID());
        entity3_live_stats = world->GetLiveStats(entity3->GetID());
        entity4_live_stats = world->GetLiveStats(entity4->GetID());
        EXPECT_EQ(events_on_game_started.size(), 1);
        EXPECT_EQ(entity1_live_stats.Get(StatType::kEnergyRegeneration), after_expected_energy_regeneration);
        EXPECT_EQ(entity2_live_stats.Get(StatType::kEnergyRegeneration), after_expected_energy_regeneration);
        EXPECT_EQ(entity3_live_stats.Get(StatType::kEnergyRegeneration), after_expected_energy_regeneration);
        EXPECT_EQ(entity4_live_stats.Get(StatType::kEnergyRegeneration), after_expected_energy_regeneration);
    }

    std::vector<event_data::BattleStarted> events_on_game_started;
};

class SynergySystemTest_WithThresholdAbilities : public SynergySystemTest
{
    using Super = BaseTest;

protected:
    void AddWaterSynergyThresholdAbility(
        AbilitiesData& abilities_data,
        const int duration_ms,
        const FixedPoint expression_value) const
    {
        auto& ability = abilities_data.AddAbility();
        ability.name = "Water Ability";
        ability.activation_trigger_data.trigger_type = ActivationTriggerType::kOnBattleStart;
        ability.total_duration_ms = 0;

        auto& skill = ability.AddSkill();
        skill.name = "Water Skill";
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.targeting.type = SkillTargetingType::kSelf;

        const EffectData buff_effect = EffectData::CreateBuff(
            StatType::kEnergyRegeneration,
            EffectExpression::FromValue(expression_value),
            duration_ms);
        skill.AddEffect(buff_effect);
    }

    void GetSynergyDataThresholdsAbilities(
        const CombatSynergyBonus combat_synergy,
        const bool has_components,
        std::unordered_map<int, std::vector<std::shared_ptr<const AbilityData>>>* out_synergy_threshold_abilities)
        const override
    {
        if (combat_synergy == CombatAffinity::kWater)
        {
            const std::array<int, 3> array_duration_ms{5000, 7500, 10000};
            const std::array<FixedPoint, 3> array_expression{3_fp, 7_fp, 12_fp};

            const std::vector<int> synergy_thresholds = GetSynergyDataThresholds(has_components);
            ASSERT_EQ(synergy_thresholds.size(), array_duration_ms.size());

            for (size_t i = 0; i < synergy_thresholds.size(); i++)
            {
                const int synergy_threshold = synergy_thresholds[i];
                const int duration_ms = array_duration_ms[i];
                const FixedPoint expression_value = array_expression[i];

                // Create ability for this threshold
                AbilitiesData abilities_data;
                AddWaterSynergyThresholdAbility(abilities_data, duration_ms, expression_value);

                // Add to the results
                (*out_synergy_threshold_abilities)[synergy_threshold] = abilities_data.abilities;
            }
        }
    }
};

class SynergySystemTest_WithIntrinsicAbilities : public SynergySystemTest
{
    using Super = BaseTest;

protected:
    void GetSynergyDataIntrinsicAbilities(
        std::vector<std::shared_ptr<const AbilityData>>& out_intrinsic_abilities) const override
    {
        auto ability = AbilityData::Create();
        ability->name = "Synergy Count Ability";
        ability->activation_trigger_data.trigger_type = ActivationTriggerType::kOnBattleStart;
        ability->total_duration_ms = 0;

        auto& skill = ability->AddSkill();
        skill.name = "SynergyCount Skill Name";
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.targeting.type = SkillTargetingType::kSelf;

        // 3 x SynergyCount Water
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands.push_back(EffectExpression::FromValue(3_fp));
        expression.operands.push_back(EffectExpression::FromSynergyCount(
            CombatSynergyBonus{CombatAffinity::kWater},
            ExpressionDataSourceType::kSender));

        const EffectData buff_effect = EffectData::CreateBuff(StatType::kEnergyRegeneration, expression, kTimeInfinite);
        skill.AddEffect(buff_effect);

        out_intrinsic_abilities.emplace_back(ability);
    }
};

TEST_F(SynergySystemTest_WithThresholdAbilities, OneWater)
{
    TestOneWater(0_fp);
}

TEST_F(SynergySystemTest_WithIntrinsicAbilities, OneWater)
{
    TestOneWater(3_fp);
}

TEST_F(SynergySystemTest_WithThresholdAbilities, TwoWater)
{
    TestTwoWater(0_fp);
}

TEST_F(SynergySystemTest_WithIntrinsicAbilities, TwoWater)
{
    TestTwoWater(6_fp);
}

TEST_F(SynergySystemTest_WithThresholdAbilities, ThreeWater)
{
    TestThreeWater(3_fp);
}

TEST_F(SynergySystemTest_WithIntrinsicAbilities, ThreeWater)
{
    TestThreeWater(9_fp);
}

TEST_F(SynergySystemTest_WithThresholdAbilities, FourWater)
{
    TestFourWater(3_fp);
}

TEST_F(SynergySystemTest_WithIntrinsicAbilities, FourWater)
{
    TestFourWater(12_fp);
}

TEST_F(SynergySystemTest, GetAllPartsOfCombatSynergy)
{
    const auto& synergies_helper = world->GetSynergiesHelper();

    // Primary
    {
        const std::vector<CombatSynergyBonus> expected{CombatClass::kFighter};
        EXPECT_EQ(synergies_helper.GetAllPartsOfCombatSynergy(CombatClass::kFighter), expected);
    }
    {
        const std::vector<CombatSynergyBonus> expected{CombatAffinity::kWater};
        EXPECT_EQ(synergies_helper.GetAllPartsOfCombatSynergy(CombatAffinity::kWater), expected);
    }

    // Composite
    {
        const std::vector<CombatSynergyBonus> expected{
            CombatClass::kBehemoth,
            CombatClass::kFighter,
            CombatClass::kBulwark};
        EXPECT_EQ(synergies_helper.GetAllPartsOfCombatSynergy(CombatClass::kBehemoth), expected);
    }
    {
        const std::vector<CombatSynergyBonus> expected{
            CombatClass::kBerserker,
            CombatClass::kFighter,
            CombatClass::kFighter};
        EXPECT_EQ(synergies_helper.GetAllPartsOfCombatSynergy(CombatClass::kBerserker), expected);
    }
    {
        const std::vector<CombatSynergyBonus> expected{
            CombatAffinity::kMud,
            CombatAffinity::kWater,
            CombatAffinity::kEarth};
        EXPECT_EQ(synergies_helper.GetAllPartsOfCombatSynergy(CombatAffinity::kMud), expected);
    }
    {
        const std::vector<CombatSynergyBonus> expected{
            CombatAffinity::kTsunami,
            CombatAffinity::kWater,
            CombatAffinity::kWater};
        EXPECT_EQ(synergies_helper.GetAllPartsOfCombatSynergy(CombatAffinity::kTsunami), expected);
    }
}

TEST_F(SynergySystemTest, NoSynergy)
{
    std::array<CombatClass, 4> combat_class_list{
        CombatClass::kFighter,
        CombatClass::kBehemoth,
        CombatClass::kHarbinger,
        CombatClass::kPsion};

    CombatUnitData data = CreateCombatUnitData();
    data.type_id = CombatUnitTypeID("Axolotl", 1);
    for (size_t i = 0; i != combat_class_list.size(); i++)
    {
        data.type_data.combat_class = combat_class_list[i];

        Entity* blue_entity = nullptr;
        SpawnCombatUnit(Team::kBlue, {static_cast<int>(10 + i * 2), static_cast<int>(10 + i)}, data, blue_entity);

        // Set same stats
        auto& stats_component = blue_entity->Get<StatsComponent>();
        stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, 100_fp);
        stats_component.SetCurrentHealth(100_fp);
    }

    // TimeStep
    world->TimeStep();

    // Iterate the entities
    auto& entities = world->GetAll();
    for (auto& entity : entities)
    {
        // Should not have fighter synergy
        const StatsData live_stats = world->GetLiveStats(entity->GetID());
        EXPECT_EQ(live_stats.Get(StatType::kAttackPhysicalDamage), 0_fp);
    }
}

TEST_F(SynergySystemTest, AddDifferentCombatUnits)
{
    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.type_id = CombatUnitTypeID("Axolotl", 1);
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;

    CombatUnitData data2 = CreateCombatUnitData();
    data2.type_id = CombatUnitTypeID("Turtle", 1);
    data2.type_data.combat_affinity = CombatAffinity::kEarth;
    data2.type_data.combat_class = CombatClass::kBehemoth;
    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);
    Entity* entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {40, 40}, data2, entity2);

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
    const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

    ASSERT_EQ(combat_classes.size(), 1);
    // Behemoth = Fighter + Bulwark
    ASSERT_EQ(combat_classes[0].combat_units.size(), 2);
    EXPECT_EQ(combat_classes[0].combat_units[0].type_id.line_name, "Axolotl");
    EXPECT_EQ(combat_classes[0].combat_units[1].type_id.line_name, "Turtle");
    EXPECT_EQ(combat_classes[0].combat_synergy.GetClass(), CombatClass::kBehemoth);
    EXPECT_EQ(combat_classes[0].synergy_stacks, 2);

    ASSERT_EQ(combat_affinities.size(), 2);
    ASSERT_EQ(combat_affinities[0].combat_units.size(), 1);
    EXPECT_EQ(combat_affinities[0].combat_units[0].type_id.line_name, "Axolotl");
    EXPECT_EQ(combat_affinities[0].combat_synergy.GetAffinity(), CombatAffinity::kFire);
    EXPECT_EQ(combat_affinities[0].synergy_stacks, 1);

    ASSERT_EQ(combat_affinities[1].combat_units.size(), 1);
    EXPECT_EQ(combat_affinities[1].combat_units[0].type_id.line_name, "Turtle");
    EXPECT_EQ(combat_affinities[1].combat_synergy.GetAffinity(), CombatAffinity::kEarth);
    EXPECT_EQ(combat_affinities[1].synergy_stacks, 1);
}

TEST_F(SynergySystemTest, AddOverlapSameLineThreeDifferentStage)
{
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/108200183/Synergy+Overlap

    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.type_id = CombatUnitTypeID("Axolotl", 1);
    data.type_data.combat_affinity = CombatAffinity::kWater;
    data.type_data.combat_class = CombatClass::kBulwark;

    CombatUnitData data2 = CreateCombatUnitData();
    data2.type_id = CombatUnitTypeID("Axolotl", 2);
    data2.type_data.combat_affinity = CombatAffinity::kWater;
    data2.type_data.combat_class = CombatClass::kHarbinger;  // Bulwark + Psion

    CombatUnitData data3 = CreateCombatUnitData();
    data3.type_id = CombatUnitTypeID("Axolotl", 3);
    data3.type_data.combat_affinity = CombatAffinity::kTsunami;  // Water + Water
    data3.type_data.combat_class = CombatClass::kHarbinger;      // Bulwark + Psion

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);
    Entity* entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {15, 15}, data2, entity2);
    Entity* entity3 = nullptr;
    SpawnCombatUnit(Team::kBlue, {20, 20}, data3, entity3);

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
    const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

    ASSERT_EQ(combat_classes.size(), 2);

    EXPECT_EQ(combat_classes[0].combat_units.size(), 2);
    EXPECT_EQ(combat_classes[0].combat_synergy.GetClass(), CombatClass::kHarbinger);
    EXPECT_EQ(combat_classes[0].synergy_stacks, 1);

    EXPECT_EQ(combat_classes[1].combat_units.size(), 1);
    EXPECT_EQ(combat_classes[1].combat_synergy.GetClass(), CombatClass::kBulwark);
    EXPECT_EQ(combat_classes[1].synergy_stacks, 1);

    ASSERT_EQ(combat_affinities.size(), 2);
    ASSERT_EQ(combat_affinities[0].combat_units.size(), 2);
    EXPECT_EQ(combat_affinities[0].combat_synergy.GetAffinity(), CombatAffinity::kWater);
    EXPECT_EQ(combat_affinities[0].synergy_stacks, 1);

    EXPECT_EQ(combat_affinities[1].combat_synergy.GetAffinity(), CombatAffinity::kTsunami);
    EXPECT_EQ(combat_affinities[1].synergy_stacks, 1);
}

TEST_F(SynergySystemTest, AddOverlapSameLineTwoDifferentStage)
{
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/108200183/Synergy+Overlap

    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.type_id = CombatUnitTypeID("Axolotl", 1);
    data.type_data.combat_affinity = CombatAffinity::kNature;
    data.type_data.combat_class = CombatClass::kBerserker;

    CombatUnitData data2 = CreateCombatUnitData();
    data2.type_id = CombatUnitTypeID("Axolotl", 2);
    data2.type_data.combat_affinity = CombatAffinity::kVerdant;
    data2.type_data.combat_class = CombatClass::kBerserker;

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);
    Entity* entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {40, 40}, data2, entity2);

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
    const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

    ASSERT_EQ(combat_classes.size(), 1);
    EXPECT_EQ(combat_classes[0].combat_units.size(), 2);
    EXPECT_EQ(combat_classes[0].combat_synergy.GetClass(), CombatClass::kBerserker);
    EXPECT_EQ(combat_classes[0].synergy_stacks, 1);

    ASSERT_EQ(combat_affinities.size(), 2);
    ASSERT_EQ(combat_affinities[0].combat_units.size(), 1);
    EXPECT_EQ(combat_affinities[0].combat_units[0].type_id.line_name, "Axolotl");
    EXPECT_EQ(combat_affinities[0].combat_synergy.GetAffinity(), CombatAffinity::kVerdant);
    EXPECT_EQ(combat_affinities[0].synergy_stacks, 1);

    ASSERT_EQ(combat_affinities[1].combat_units.size(), 1);
    EXPECT_EQ(combat_affinities[1].combat_units[0].type_id.line_name, "Axolotl");
    EXPECT_EQ(combat_affinities[1].combat_synergy.GetAffinity(), CombatAffinity::kNature);
    EXPECT_EQ(combat_affinities[1].synergy_stacks, 1);
}

TEST_F(SynergySystemTest, AddOverlapDifferentLineTwoDifferentStageComplex)
{
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/108200183/Synergy+Overlap

    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.type_id = CombatUnitTypeID("Axolotl", 1);
    data.type_data.combat_class = CombatClass::kSlayer;         // Fighter + Rogue
    data.type_data.combat_affinity = CombatAffinity::kTsunami;  // Water + Water

    CombatUnitData data2 = CreateCombatUnitData();
    data2.type_id = CombatUnitTypeID("Turtle", 2);
    data2.type_data.combat_class = CombatClass::kPhantom;      // Rogue + Psion
    data2.type_data.combat_affinity = CombatAffinity::kFrost;  // Water + Air

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);
    Entity* entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {40, 40}, data2, entity2);

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
    const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

    ASSERT_EQ(combat_classes.size(), 2);
    EXPECT_EQ(combat_classes[0].combat_units.size(), 1);
    EXPECT_EQ(combat_classes[0].combat_synergy.GetClass(), CombatClass::kPhantom);
    EXPECT_EQ(combat_classes[0].synergy_stacks, 1);

    EXPECT_EQ(combat_classes[1].combat_units.size(), 1);
    EXPECT_EQ(combat_classes[1].combat_synergy.GetClass(), CombatClass::kSlayer);
    EXPECT_EQ(combat_classes[1].synergy_stacks, 1);

    ASSERT_EQ(combat_affinities.size(), 2);
    ASSERT_EQ(combat_affinities[0].combat_units.size(), 1);
    EXPECT_EQ(combat_affinities[0].combat_synergy.GetAffinity(), CombatAffinity::kFrost);
    EXPECT_EQ(combat_affinities[0].synergy_stacks, 1);

    ASSERT_EQ(combat_affinities[1].combat_units.size(), 1);
    EXPECT_EQ(combat_affinities[1].combat_synergy.GetAffinity(), CombatAffinity::kTsunami);
    EXPECT_EQ(combat_affinities[1].synergy_stacks, 1);
}

TEST_F(SynergySystemTest, RemoveOverlapSameLineDifferentStage)
{
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/108200183/Synergy+Overlap

    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.type_id = CombatUnitTypeID("Axolotl", 1);
    data.type_data.combat_affinity = CombatAffinity::kNature;
    data.type_data.combat_class = CombatClass::kBehemoth;

    CombatUnitData data2 = CreateCombatUnitData();
    data2.type_id = CombatUnitTypeID("Axolotl", 2);
    data2.type_data.combat_affinity = CombatAffinity::kVerdant;
    data2.type_data.combat_class = CombatClass::kBehemoth;

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);
    Entity* entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {40, 40}, data2, entity2);

    world->RemoveCombatUnitBeforeBattleStarted(entity->GetID());
    world->RemoveCombatUnitBeforeBattleStarted(entity2->GetID());
    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
    const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

    ASSERT_EQ(combat_classes.size(), 0);
    ASSERT_EQ(combat_affinities.size(), 0);
}

TEST_F(SynergySystemTest, AddOverlapDifferentLineDifferentStage)
{
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/108200183/Synergy+Overlap

    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.type_id = CombatUnitTypeID("Turtle", 1);
    data.type_data.combat_affinity = CombatAffinity::kNature;
    data.type_data.combat_class = CombatClass::kBerserker;

    CombatUnitData data2 = CreateCombatUnitData();
    data2.type_id = CombatUnitTypeID("Axolotl", 2);
    data2.type_data.combat_affinity = CombatAffinity::kVerdant;
    data2.type_data.combat_class = CombatClass::kBerserker;

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);
    Entity* entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {40, 40}, data2, entity2);

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
    const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

    ASSERT_EQ(combat_classes.size(), 1);
    EXPECT_EQ(combat_classes[0].combat_units.size(), 2);
    EXPECT_EQ(combat_classes[0].combat_synergy.GetClass(), CombatClass::kBerserker);
    EXPECT_EQ(combat_classes[0].synergy_stacks, 2);

    ASSERT_EQ(combat_affinities.size(), 2);
    ASSERT_EQ(combat_affinities[0].combat_units.size(), 1);
    EXPECT_EQ(combat_affinities[0].combat_units[0].type_id.line_name, "Axolotl");
    EXPECT_EQ(combat_affinities[0].combat_synergy.GetAffinity(), CombatAffinity::kVerdant);
    EXPECT_EQ(combat_affinities[0].synergy_stacks, 1);

    ASSERT_EQ(combat_affinities[1].combat_units.size(), 1);
    EXPECT_EQ(combat_affinities[1].combat_units[0].type_id.line_name, "Turtle");
    EXPECT_EQ(combat_affinities[1].combat_synergy.GetAffinity(), CombatAffinity::kNature);
    EXPECT_EQ(combat_affinities[1].synergy_stacks, 1);
}

TEST_F(SynergySystemTest, AddSameCombatUnits)
{
    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.type_id = CombatUnitTypeID("Axolotl", 1);
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    Entity* entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {40, 40}, data, entity2);

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
    const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

    ASSERT_EQ(combat_classes.size(), 1);

    // Behemoth = Fighter + Bulwark
    EXPECT_EQ(combat_classes[0].combat_units.size(), 2);
    EXPECT_EQ(combat_classes[0].combat_synergy.GetClass(), CombatClass::kBehemoth);
    EXPECT_EQ(combat_classes[0].synergy_stacks, 1);

    ASSERT_EQ(combat_affinities.size(), 1);
    EXPECT_EQ(combat_affinities[0].combat_units.size(), 2);
    EXPECT_EQ(combat_affinities[0].combat_synergy.GetAffinity(), CombatAffinity::kFire);
    EXPECT_EQ(combat_affinities[0].synergy_stacks, 1);
}

TEST_F(SynergySystemTest, RemoveSingleDoubleClassAffinity)
{
    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.type_id = CombatUnitTypeID("Axolotl", 2);
    data.type_data.combat_affinity = CombatAffinity::kTsunami;
    data.type_data.combat_class = CombatClass::kBerserker;

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    {
        const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
        const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

        ASSERT_EQ(combat_classes.size(), 1);
        EXPECT_EQ(combat_classes[0].combat_units.size(), 1);
        EXPECT_EQ(combat_classes[0].combat_synergy.GetClass(), CombatClass::kBerserker);
        EXPECT_EQ(combat_classes[0].synergy_stacks, 1);

        ASSERT_EQ(combat_affinities.size(), 1);
        EXPECT_EQ(combat_affinities[0].combat_units.size(), 1);
        EXPECT_EQ(combat_affinities[0].combat_synergy.GetAffinity(), CombatAffinity::kTsunami);
        EXPECT_EQ(combat_affinities[0].synergy_stacks, 1);
    }

    // Remove
    {
        world->RemoveCombatUnitBeforeBattleStarted(entity->GetID());
        const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
        const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

        ASSERT_EQ(combat_classes.size(), 0);
        ASSERT_EQ(combat_affinities.size(), 0);
    }
}

TEST_F(SynergySystemTest, RemoveCombatUnitBeforeBattleStarteds)
{
    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.type_id = CombatUnitTypeID("Axolotl", 1);
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBulwark;

    CombatUnitData data2 = CreateCombatUnitData();
    data2.type_id = CombatUnitTypeID("Beetle2", 1);
    data2.type_data.combat_affinity = CombatAffinity::kWildfire;
    data2.type_data.combat_class = CombatClass::kBulwark;

    CombatUnitData data3 = CreateCombatUnitData();
    data3.type_id = CombatUnitTypeID("Axolotl", 1);
    data3.type_data.combat_affinity = CombatAffinity::kFire;
    data3.type_data.combat_class = CombatClass::kBulwark;

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    Entity* entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {40, 40}, data2, entity2);

    Entity* entity3 = nullptr;
    SpawnCombatUnit(Team::kBlue, {-40, -40}, data3, entity3);

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    {
        const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
        const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

        ASSERT_EQ(combat_classes.size(), 1);
        EXPECT_EQ(combat_classes[0].combat_units.size(), 3);
        EXPECT_EQ(combat_classes[0].combat_synergy.GetClass(), CombatClass::kBulwark);
        EXPECT_EQ(combat_classes[0].synergy_stacks, 2);

        ASSERT_EQ(combat_affinities.size(), 2);
        EXPECT_EQ(combat_affinities[0].combat_units.size(), 2);
        EXPECT_EQ(combat_affinities[0].combat_synergy.GetAffinity(), CombatAffinity::kFire);
        EXPECT_EQ(combat_affinities[0].synergy_stacks, 1);

        EXPECT_EQ(combat_affinities[1].combat_units.size(), 1);
        EXPECT_EQ(combat_affinities[1].combat_synergy.GetAffinity(), CombatAffinity::kWildfire);
        EXPECT_EQ(combat_affinities[1].synergy_stacks, 1);

        const auto& synergies_order =
            synergies_state_container.GetTeamAllSynergies(Team::kBlue, world->IsBattleStarted());
        ASSERT_EQ(synergies_order.size(), 3);
        EXPECT_EQ(synergies_order[0].synergy_stacks, 2);
        EXPECT_EQ(synergies_order[0].combat_synergy.GetClass(), CombatClass::kBulwark);
        EXPECT_EQ(synergies_order[0].combat_synergy.GetAffinity(), CombatAffinity::kNone);
    }

    {
        world->RemoveCombatUnitBeforeBattleStarted(entity->GetID());
        const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
        const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

        // expect only combat unit to be 1 because we removed the same unit
        ASSERT_EQ(combat_classes.size(), 1);
        EXPECT_EQ(combat_classes[0].combat_units.size(), 2);
        EXPECT_EQ(combat_classes[0].combat_synergy.GetClass(), CombatClass::kBulwark);
        EXPECT_EQ(combat_classes[0].synergy_stacks, 2);

        // expect only combat unit to be 1 because we removed the same unit
        ASSERT_EQ(combat_affinities.size(), 2);
        EXPECT_EQ(combat_affinities[0].combat_units.size(), 1);
        EXPECT_EQ(combat_affinities[0].combat_synergy.GetAffinity(), CombatAffinity::kWildfire);
        EXPECT_EQ(combat_affinities[0].synergy_stacks, 1);

        EXPECT_EQ(combat_affinities[1].combat_units.size(), 1);
        EXPECT_EQ(combat_affinities[1].combat_synergy.GetAffinity(), CombatAffinity::kFire);
        EXPECT_EQ(combat_affinities[1].synergy_stacks, 1);
    }

    {
        world->RemoveCombatUnitBeforeBattleStarted(entity2->GetID());
        const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
        const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

        // expect only combat unit to be 1 because we removed the same unit
        ASSERT_EQ(combat_classes.size(), 1);
        EXPECT_EQ(combat_classes[0].combat_units.size(), 1);
        EXPECT_EQ(combat_classes[0].combat_synergy.GetClass(), CombatClass::kBulwark);
        EXPECT_EQ(combat_classes[0].synergy_stacks, 1);

        // expect only combat unit to be 1 because we removed the same unit
        ASSERT_EQ(combat_affinities.size(), 1);
        EXPECT_EQ(combat_affinities[0].combat_units.size(), 1);
        EXPECT_EQ(combat_affinities[0].combat_synergy.GetAffinity(), CombatAffinity::kFire);
        EXPECT_EQ(combat_affinities[0].synergy_stacks, 1);
    }

    {
        world->RemoveCombatUnitBeforeBattleStarted(entity3->GetID());
        const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
        const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

        ASSERT_EQ(combat_classes.size(), 0);
        ASSERT_EQ(combat_affinities.size(), 0);
    }
}

TEST_F(SynergySystemTest, AddRemoveCombatUnitBeforeBattleStartedsDifferentTeams)
{
    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.type_id = CombatUnitTypeID("Axolotl", 1);
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;

    CombatUnitData data2 = CreateCombatUnitData();
    data2.type_id = CombatUnitTypeID("Turtle", 1);
    data2.type_data.combat_affinity = CombatAffinity::kEarth;
    data2.type_data.combat_class = CombatClass::kBehemoth;

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);
    Entity* entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, {40, 40}, data2, entity2);

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    const auto& combat_classes_blue = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
    const auto& combat_affinities_blue = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

    ASSERT_EQ(combat_classes_blue.size(), 1);
    ASSERT_EQ(combat_classes_blue[0].combat_units.size(), 1);
    EXPECT_EQ(combat_classes_blue[0].combat_units[0].type_id.line_name, "Axolotl");
    EXPECT_EQ(combat_classes_blue[0].combat_synergy.GetClass(), CombatClass::kBehemoth);
    EXPECT_EQ(combat_classes_blue[0].synergy_stacks, 1);

    ASSERT_EQ(combat_affinities_blue.size(), 1);
    ASSERT_EQ(combat_affinities_blue[0].combat_units.size(), 1);
    EXPECT_EQ(combat_affinities_blue[0].combat_units[0].type_id.line_name, "Axolotl");
    EXPECT_EQ(combat_affinities_blue[0].combat_synergy.GetAffinity(), CombatAffinity::kFire);
    EXPECT_EQ(combat_affinities_blue[0].synergy_stacks, 1);

    const auto& combat_classes_red = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kRed);
    const auto& combat_affinities_red = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kRed);

    ASSERT_EQ(combat_classes_red.size(), 1);
    ASSERT_EQ(combat_classes_red[0].combat_units.size(), 1);
    EXPECT_EQ(combat_classes_red[0].combat_units[0].type_id.line_name, "Turtle");
    EXPECT_EQ(combat_classes_red[0].combat_synergy.GetClass(), CombatClass::kBehemoth);
    EXPECT_EQ(combat_classes_red[0].synergy_stacks, 1);

    ASSERT_EQ(combat_affinities_red.size(), 1);
    ASSERT_EQ(combat_affinities_red[0].combat_units.size(), 1);
    EXPECT_EQ(combat_affinities_red[0].combat_units[0].type_id.line_name, "Turtle");
    EXPECT_EQ(combat_affinities_red[0].combat_synergy.GetAffinity(), CombatAffinity::kEarth);
    EXPECT_EQ(combat_affinities_red[0].synergy_stacks, 1);

    world->RemoveCombatUnitBeforeBattleStarted(entity->GetID());

    const auto& combat_classes_red_1 = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
    const auto& combat_affinities_red_1 = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kRed);
    ASSERT_EQ(combat_classes_red_1.size(), 0);
    ASSERT_EQ(combat_affinities_red_1.size(), 1);
}

class SynergyFromAugmentTest : public SynergySystemTest
{
    using Super = SynergySystemTest;

    void AddSynergyThresholdAbility(
        AbilitiesData& abilities_data,
        const int duration_ms,
        const int expression_value,
        const std::string_view name) const
    {
        auto& ability = abilities_data.AddAbility();
        ability.name = name;
        ability.activation_trigger_data.trigger_type = ActivationTriggerType::kOnBattleStart;
        ability.total_duration_ms = 0;

        auto& skill = ability.AddSkill();
        skill.name = "Skill";
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.targeting.type = SkillTargetingType::kSelf;

        const EffectData buff_effect = EffectData::CreateBuff(
            StatType::kEnergyRegeneration,
            EffectExpression::FromValue(FixedPoint::FromInt(expression_value)),
            duration_ms);
        skill.AddEffect(buff_effect);
    }

    void GetSynergyDataThresholdsAbilities(
        const CombatSynergyBonus combat_synergy,
        const bool has_components,
        std::unordered_map<int, std::vector<std::shared_ptr<const AbilityData>>>* out_synergy_threshold_abilities)
        const override
    {
        if (combat_synergy == CombatAffinity::kBloom || combat_synergy == CombatAffinity::kGranite)
        {
            const std::array<int, 3> array_duration_ms{5000, 7500, 10000};
            const std::array<int, 3> array_expression{3, 7, 12};
            const std::vector<int> synergy_thresholds = GetSynergyDataThresholds(has_components);
            ASSERT_EQ(synergy_thresholds.size(), array_duration_ms.size());

            for (size_t i = 0; i < synergy_thresholds.size(); i++)
            {
                const int synergy_threshold = synergy_thresholds[i];
                const int duration_ms = array_duration_ms[i];
                const int expression_value = array_expression[i];

                // Create ability for this threshold
                AbilitiesData abilities_data;
                AddSynergyThresholdAbility(
                    abilities_data,
                    duration_ms,
                    expression_value,
                    fmt::format("{} Ability", combat_synergy.GetAffinity()));

                // Add to the results
                (*out_synergy_threshold_abilities)[synergy_threshold] = abilities_data.abilities;
            }
        }
    }

    void FillAugmentsData(GameDataContainer& game_data) const override
    {
        auto augment = AugmentData::Create();
        augment->type_id = AugmentTypeID{"Heal", 1, ""};
        augment->combat_affinities[CombatAffinity::kBloom] = 3;
        augment->combat_affinities[CombatAffinity::kGranite] = 1;
        augment->combat_classes[CombatClass::kPhantom] = 1;
        game_data.AddAugmentData(augment->type_id, augment);
    }
};

TEST_F(SynergyFromAugmentTest, LegendaryAugmentWithSynergies)
{
    Entity* entity = nullptr;
    {
        // Set up a dummy CombatUnit type
        CombatUnitData data = CreateCombatUnitData();
        data.type_id = CombatUnitTypeID("Axolotl", 2);
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kEmpath;

        // Spawn the CombatUnit
        SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);
    }

    AugmentInstanceData augment_instance;
    augment_instance.type_id = AugmentTypeID{"Heal", 1, ""};

    // Add augment to entity
    ASSERT_TRUE(world->AddAugmentBeforeBattleStarted(*entity, augment_instance));
    const auto augment_component = entity->Get<AugmentComponent>();
    ASSERT_EQ(augment_component.GetEquippedAugmentsSize(), 1);

    // Ensure that HasCombatAffinity and HasCombatClass check also synergies from augments
    ASSERT_TRUE(world->GetSynergiesHelper().HasCombatAffinity(*entity, CombatAffinity::kBloom));
    ASSERT_TRUE(world->GetSynergiesHelper().HasCombatClass(*entity, CombatClass::kPhantom));

    // spawn extra water combat unit to get innate
    Entity* water_entity = nullptr;
    Entity* water_entity_2 = nullptr;
    {
        // Set up a dummy CombatUnit type
        CombatUnitData data = CreateCombatUnitData();
        data.type_id = CombatUnitTypeID("Axolotle", 2);
        data.type_data.combat_affinity = CombatAffinity::kGranite;
        data.type_data.combat_class = CombatClass::kEmpath;

        // Spawn the CombatUnit
        SpawnCombatUnit(Team::kBlue, {15, 20}, data, water_entity);

        data.type_id = CombatUnitTypeID("Axolotler", 2);
        SpawnCombatUnit(Team::kBlue, {20, 20}, data, water_entity_2);
    }

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    {
        const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
        ASSERT_EQ(combat_classes.size(), 2);

        EXPECT_EQ(combat_classes[0].combat_units.size(), 3);
        EXPECT_EQ(combat_classes[0].combat_synergy.GetClass(), CombatClass::kEmpath);
        EXPECT_EQ(combat_classes[0].synergy_stacks, 3);

        EXPECT_EQ(combat_classes[1].combat_units.size(), 0);
        EXPECT_EQ(combat_classes[1].combat_synergy.GetClass(), CombatClass::kPhantom);
        EXPECT_EQ(combat_classes[1].synergy_stacks, 1);
    }
    {
        const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);
        ASSERT_EQ(combat_affinities.size(), 3);

        EXPECT_EQ(combat_affinities[0].combat_synergy.GetAffinity(), CombatAffinity::kGranite);
        EXPECT_EQ(combat_affinities[0].combat_units.size(), 2);
        EXPECT_EQ(combat_affinities[0].synergy_stacks, 3);

        EXPECT_EQ(combat_affinities[1].combat_synergy.GetAffinity(), CombatAffinity::kBloom);
        EXPECT_EQ(combat_affinities[1].combat_units.size(), 0);
        EXPECT_EQ(combat_affinities[1].synergy_stacks, 3);

        EXPECT_EQ(combat_affinities[2].combat_synergy.GetAffinity(), CombatAffinity::kFire);
        EXPECT_EQ(combat_affinities[2].combat_units.size(), 1);
        EXPECT_EQ(combat_affinities[2].synergy_stacks, 1);
    }

    {
        // On time step synergy system adds corresponding innate abilities
        world->GetSystem<SynergySystem>().TimeStep(*entity);

        const auto& abilities = entity->Get<AbilitiesComponent>().GetDataInnateAbilities().abilities;
        ASSERT_EQ(abilities.size(), 2);
        EXPECT_EQ(abilities[1]->name, "Bloom Ability");
        EXPECT_EQ(abilities[0]->name, "Granite Ability");
    }

    // Remove
    {
        world->RemoveCombatUnitBeforeBattleStarted(entity->GetID());
        const auto& combat_classes = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);
        const auto& combat_affinities = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

        // Extra water unit is still alive
        EXPECT_EQ(combat_classes.size(), 1);
        EXPECT_EQ(combat_affinities.size(), 1);
    }
}

TEST_F(SynergyFromAugmentTest, DecomposeCombatAffinities)
{
    const auto& game_data = world->GetGameDataContainer();
    ASSERT_TRUE(game_data.HasCombatAffinitySynergyData(CombatAffinity::kGranite));
    const auto synergy_data = game_data.GetCombatAffinitySynergyData(CombatAffinity::kGranite);

    EXPECT_EQ(
        synergy_data->GetUniqueComponentsAsCombatAffinities(),
        EnumSet<CombatAffinity>::Create(CombatAffinity::kEarth));

    const std::vector<CombatAffinity> expected_affinities{CombatAffinity::kEarth, CombatAffinity::kEarth};
    EXPECT_EQ(synergy_data->GetComponentsAsCombatAffinities(), expected_affinities);
}

TEST_F(SynergyFromAugmentTest, DecomposeCombatClasses)
{
    const auto& game_data = world->GetGameDataContainer();
    ASSERT_TRUE(game_data.HasCombatClassSynergyData(CombatClass::kBerserker));
    const auto synergy_data = game_data.GetCombatClassSynergyData(CombatClass::kBerserker);

    EXPECT_EQ(synergy_data->GetUniqueComponentsAsCombatClasses(), EnumSet<CombatClass>::Create(CombatClass::kFighter));

    const std::vector<CombatClass> expected_classes{CombatClass::kFighter, CombatClass::kFighter};
    EXPECT_EQ(synergy_data->GetComponentsAsCombatClasses(), expected_classes);
}

class EvolutionSynergySystemTest : public BaseTest
{
public:
    bool UseSynergiesData() const override
    {
        return true;
    }

    bool UseMaxStagePlacementRadius() const override
    {
        return true;
    }

    void AddSynergyThresholdAbility(
        const CombatSynergyBonus& synergy,
        AbilitiesData& abilities_data,
        const int duration_ms) const
    {
        auto& ability = abilities_data.AddAbility();
        ability.name = fmt::format("{} Ability", synergy);
        ability.activation_trigger_data.trigger_type = ActivationTriggerType::kOnBattleStart;
        ability.total_duration_ms = 0;

        auto& skill = ability.AddSkill();
        skill.name = fmt::format("{} Skill", synergy);
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.targeting.type = SkillTargetingType::kSelf;

        const EffectData buff_effect =
            EffectData::CreateBuff(StatType::kEnergyRegeneration, EffectExpression::FromValue(10_fp), duration_ms);
        skill.AddEffect(buff_effect);
    }

    void GetSynergyDataThresholdsAbilities(
        const CombatSynergyBonus combat_synergy,
        const bool has_components,
        std::unordered_map<int, std::vector<std::shared_ptr<const AbilityData>>>* out_synergy_threshold_abilities)
        const override
    {
        const std::array<int, 3> array_duration_ms{10000, 20000, 30000};
        const std::vector<int> synergy_thresholds = GetSynergyDataThresholds(has_components);
        ASSERT_EQ(synergy_thresholds.size(), array_duration_ms.size());

        for (size_t i = 0; i < synergy_thresholds.size(); i++)
        {
            const int synergy_threshold = synergy_thresholds[i];
            const int duration_ms = array_duration_ms[i];

            // Create ability for this threshold
            AbilitiesData abilities_data;
            AddSynergyThresholdAbility(combat_synergy, abilities_data, duration_ms);

            // Add to the results
            (*out_synergy_threshold_abilities)[synergy_threshold] = abilities_data.abilities;
        }
    }
};

TEST_F(EvolutionSynergySystemTest, SynergyStacksWithEvolution)
{
    constexpr std::array<std::string_view, 3> lines{"Axolol", "Bee", "Cat"};
    std::vector<Entity*> blue;
    constexpr HexGridPosition blue_initial_position{10, 20};
    constexpr HexGridPosition blue_position_step{5, 0};

    // Prepare and spawn 3 blue combat units of stage 3
    auto unit_data = CreateCombatUnitData();

    for (size_t line_index = 0; line_index != lines.size(); ++line_index)
    {
        unit_data.type_data.combat_class = CombatClass::kHarbinger;
        unit_data.type_data.combat_affinity = CombatAffinity::kTsunami;
        unit_data.type_id = CombatUnitTypeID(std::string(lines[line_index]), 3);

        CombatUnitInstanceData unit_instance = CreateCombatUnitInstanceData();
        unit_instance.team = Team::kBlue;
        unit_instance.position = blue_initial_position + blue_position_step * static_cast<int>(line_index);
        unit_instance.dominant_combat_class = CombatClass::kBulwark;
        unit_instance.dominant_combat_affinity = CombatAffinity::kWater;

        SpawnCombatUnit(unit_instance, unit_data, blue.emplace_back());
        ASSERT_NE(blue.back(), nullptr);
    }

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();

    // Combat classes
    {
        const auto& synergies = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kBlue);

        ASSERT_EQ(synergies.size(), 2);
        ASSERT_EQ(synergies[0].combat_units.size(), 3);
        EXPECT_EQ(synergies[0].combat_synergy, CombatClass::kHarbinger);
        EXPECT_EQ(synergies[0].synergy_stacks, 3);

        ASSERT_EQ(synergies[1].combat_units.size(), 3);
        EXPECT_EQ(synergies[1].combat_synergy, CombatClass::kBulwark);
        EXPECT_EQ(synergies[1].synergy_stacks, 3);
    }

    // Combat affinities
    {
        const auto& synergies = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kBlue);

        ASSERT_EQ(synergies.size(), 2);
        ASSERT_EQ(synergies[0].combat_units.size(), 3);
        EXPECT_EQ(synergies[0].combat_synergy, CombatAffinity::kTsunami);
        EXPECT_EQ(synergies[0].synergy_stacks, 3);

        ASSERT_EQ(synergies[1].combat_units.size(), 3);
        EXPECT_EQ(synergies[1].combat_synergy, CombatAffinity::kWater);
        EXPECT_EQ(synergies[1].synergy_stacks, 3);
    }

    // Add red combat unit so that battle does not end on the first time step
    Entity* red = nullptr;
    SpawnCombatUnit(Team::kRed, Reflect(blue_initial_position), unit_data, red);
    ASSERT_NE(red, nullptr);

    world->TimeStep();

    auto check_ability = [](const Entity& entity, const std::string_view name, const int expected_duration)
    {
        const AbilityData* ability = nullptr;
        const auto& abilities_component = entity.Get<AbilitiesComponent>();
        for (auto& ability_data_ptr : abilities_component.GetAbilities(AbilityType::kInnate).data.abilities)
        {
            if (ability_data_ptr->name == name)
            {
                ability = ability_data_ptr.get();
                break;
            }
        }

        if (!ability)
        {
            fmt::print("Entity \"{}\" does not have \"{}\"\n", entity.GetID(), name);
            return false;
        }

        const int actual_duration = ability->skills.front()->effect_package.effects.front().lifetime.duration_time_ms;
        if (actual_duration != expected_duration)
        {
            fmt::print(
                "Entity \"{}\" with ability \"{}\" has unexpected duration of {}ms ({}ms expected) \n",
                entity.GetID(),
                name,
                actual_duration,
                expected_duration);
            return false;
        }

        return true;
    };

    EXPECT_TRUE(check_ability(*blue.front(), "Water Ability", 10'000));
    EXPECT_TRUE(check_ability(*blue.front(), "Tsunami Ability", 20'000));
    EXPECT_TRUE(check_ability(*blue.front(), "Bulwark Ability", 10'000));
    EXPECT_TRUE(check_ability(*blue.front(), "Harbinger Ability", 20'000));
}

}  // namespace simulation
