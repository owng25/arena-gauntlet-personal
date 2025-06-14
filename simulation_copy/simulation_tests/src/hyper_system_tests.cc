#include <memory>

#include "base_test_fixtures.h"
#include "components/combat_synergy_component.h"
#include "components/stats_component.h"
#include "data/containers/game_data_container.h"
#include "ecs/world.h"
#include "gtest/gtest.h"
#include "systems/ability_system.h"

namespace simulation
{
class HyperHelperTest : public BaseTest
{
    typedef BaseTest Super;

protected:
    void SetUp() override
    {
        Super::SetUp();
    }

    bool UseSynergiesData() const override
    {
        return true;
    }
    bool UseHyperData() const override
    {
        return true;
    }

    void SetAffinityAndDominantAffinity(Entity& entity, const CombatAffinity combat_affinity)
    {
        auto& combat_synergy_component = entity.Get<CombatSynergyComponent>();
        combat_synergy_component.SetCombatAffinity(combat_affinity);
        combat_synergy_component.SetDominantCombatAffinity(combat_affinity);
    }
    void SetAffinityAndDominantAffinity(
        Entity& entity,
        const CombatAffinity combat_affinity,
        const CombatAffinity dominant_combat_affinity)
    {
        auto& combat_synergy_component = entity.Get<CombatSynergyComponent>();
        combat_synergy_component.SetCombatAffinity(combat_affinity);
        combat_synergy_component.SetDominantCombatAffinity(dominant_combat_affinity);
    }

    void SetClassAndDominantClass(Entity& entity, const CombatClass combat_class)
    {
        auto& combat_synergy_component = entity.Get<CombatSynergyComponent>();
        combat_synergy_component.SetCombatClass(combat_class);
        combat_synergy_component.SetDominantCombatClass(combat_class);
    }
    void
    SetClassAndDominantClass(Entity& entity, const CombatClass combat_class, const CombatClass dominant_combat_class)
    {
        auto& combat_synergy_component = entity.Get<CombatSynergyComponent>();
        combat_synergy_component.SetCombatClass(combat_class);
        combat_synergy_component.SetDominantCombatClass(dominant_combat_class);
    }
};

class HyperSystemTest : public HyperHelperTest
{
    typedef HyperHelperTest Super;

protected:
    virtual void InitCombatUnitData()
    {
        blue_data = CreateCombatUnitData();
        red_data = CreateCombatUnitData();

        blue_instance = CreateCombatUnitInstanceData();
        red_instance = CreateCombatUnitInstanceData();

        // Some sane defaults
        blue_instance.team = Team::kBlue;
        blue_instance.position = {10, 10};
        red_instance.team = Team::kRed;
        red_instance.position = {30, 30};

        // Stats
        blue_data.radius_units = 1;
        blue_data.type_data.combat_affinity = GetData1CombatAffinity();
        blue_data.type_data.combat_class = CombatClass::kBehemoth;
        blue_data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 100_fp);
        blue_data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 5_fp);
        blue_data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 7_fp);
        blue_data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        blue_data.type_data.stats.Set(StatType::kAttackPhysicalDamage, 5_fp);
        blue_data.type_data.stats.Set(StatType::kAttackEnergyDamage, 10_fp);

        blue_data.type_data.stats.Set(StatType::kWillpowerPercentage, 11_fp);
        blue_data.type_data.stats.Set(StatType::kTenacityPercentage, 12_fp);
        // 15^2 == 225
        // square distance between blue and red is 200
        blue_data.type_data.stats.Set(StatType::kAttackRangeUnits, 15_fp);

        blue_data.type_data.stats.Set(StatType::kGrit, 10_fp);
        blue_data.type_data.stats.Set(StatType::kResolve, 20_fp);

        // Resists
        blue_data.type_data.stats.Set(StatType::kPhysicalResist, 10_fp);
        blue_data.type_data.stats.Set(StatType::kEnergyResist, 5_fp);

        blue_data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        blue_data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        blue_data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);

        // Stop Movement
        blue_data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);

        // Copy the data object
        red_data = blue_data;
        red_data.type_data.combat_affinity = GetData2CombatAffinity();
    }

    virtual CombatAffinity GetData1CombatAffinity() const
    {
        return CombatAffinity::kWater;
    }

    virtual CombatAffinity GetData2CombatAffinity() const
    {
        return CombatAffinity::kFire;
    }

    void SetUp() override
    {
        Super::SetUp();

        InitCombatUnitData();

        world->SubscribeToEvent(
            EventType::kGoneHyperactive,
            [&](const Event& event)
            {
                events_gone_hyperactive.emplace_back(event.Get<event_data::Hyperactive>());
            });
    }

    bool UseHyperConfig() const override
    {
        return true;
    }

    // Combat units data and instance used
    CombatUnitData blue_data;
    CombatUnitData red_data;
    CombatUnitInstanceData blue_instance;
    CombatUnitInstanceData red_instance;

    std::vector<event_data::Hyperactive> events_gone_hyperactive;
};

TEST_F(HyperSystemTest, SingleCombatAffinitySubHyperGain)
{
    // Add blue entity with hyper
    Entity* blue_entity = nullptr;

    blue_instance.id = GenerateRandomUniqueID();
    SpawnCombatUnit(blue_instance, blue_data, blue_entity);

    SetAffinityAndDominantAffinity(*blue_entity, CombatAffinity::kWater);

    for (int i = 0; i < 3; i++)
    {
        // Add red entities with components
        Entity* red_entity = nullptr;

        red_instance.id = GenerateRandomUniqueID();
        red_instance.position = {10, 10 + (i + 1) * 3};
        SpawnCombatUnit(red_instance, red_data, red_entity);

        SetAffinityAndDominantAffinity(*red_entity, CombatAffinity::kFire);
    }

    world->TimeStep();

    const auto& stats_component = blue_entity->Get<StatsComponent>();
    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 3_fp * kPrecisionFactorFP);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 3_fp);

    world->TimeStep();

    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 6_fp * kPrecisionFactorFP);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 6_fp);

    world->TimeStep();

    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 9_fp * kPrecisionFactorFP);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 9_fp);
}

TEST_F(HyperSystemTest, CompositeCombatAffinityAgainstCompositeCombatAffinitySubHyperGain)
{
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, blue_data, blue_entity);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {15, 15}, red_data, red_entity);

    SetAffinityAndDominantAffinity(*blue_entity, CombatAffinity::kTsunami, CombatAffinity::kWater);
    SetAffinityAndDominantAffinity(*red_entity, CombatAffinity::kGranite, CombatAffinity::kEarth);
    SetClassAndDominantClass(*blue_entity, CombatClass::kEmpath);
    SetClassAndDominantClass(*red_entity, CombatClass::kEmpath);

    world->TimeStep();

    const auto& stats_component = blue_entity->Get<StatsComponent>();
    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 2_fp * kPrecisionFactorFP);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 2_fp);

    world->TimeStep();

    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 4_fp * kPrecisionFactorFP);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 4_fp);

    world->TimeStep();

    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 6_fp * kPrecisionFactorFP);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 6_fp);
}

TEST_F(HyperSystemTest, SingleSameCombatClassHyperGain)
{
    // same data
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, blue_data, blue_entity);
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {30, 30}, blue_data, red_entity);

    SetAffinityAndDominantAffinity(*blue_entity, CombatAffinity::kFire);
    SetAffinityAndDominantAffinity(*red_entity, CombatAffinity::kFire);
    SetClassAndDominantClass(*blue_entity, CombatClass::kFighter);
    SetClassAndDominantClass(*red_entity, CombatClass::kFighter);

    world->TimeStep();

    // We will go Fighter against Fighter
    // Result: 0
    const auto& stats_component = blue_entity->Get<StatsComponent>();
    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 0_fp);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 0_fp);

    world->TimeStep();

    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 0_fp);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 0_fp);

    world->TimeStep();

    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 0_fp);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 0_fp);
}

TEST_F(HyperSystemTest, CompositeSameCombatClassHyperGain)
{
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, blue_data, blue_entity);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {30, 30}, blue_data, red_entity);

    SetAffinityAndDominantAffinity(*blue_entity, CombatAffinity::kFire);
    SetAffinityAndDominantAffinity(*red_entity, CombatAffinity::kFire);
    SetClassAndDominantClass(*blue_entity, CombatClass::kTemplar, CombatClass::kFighter);
    SetClassAndDominantClass(*red_entity, CombatClass::kTemplar, CombatClass::kFighter);

    world->TimeStep();

    // Templar against Templar
    const auto& stats_component = blue_entity->Get<StatsComponent>();
    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 0_fp);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 0_fp);

    world->TimeStep();

    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 0_fp);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 0_fp);

    world->TimeStep();

    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 0_fp);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 0_fp);
}

TEST_F(HyperSystemTest, NoSubHyperGain)
{
    // Data for creating the combat units
    CombatUnitData data_blue_vanguard = CreateCombatUnitData();

    // Data for creating the combat units
    CombatUnitData data_red_arcanite = CreateCombatUnitData();

    // Addattack abilities
    {
        auto& ability = data_blue_vanguard.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }
    data_blue_vanguard.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Add Entities
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data_blue_vanguard, blue_entity);
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 20}, data_red_arcanite, red_entity);

    SetAffinityAndDominantAffinity(*blue_entity, CombatAffinity::kFire);
    SetAffinityAndDominantAffinity(*red_entity, CombatAffinity::kFire);
    SetClassAndDominantClass(*blue_entity, CombatClass::kVanguard, CombatClass::kBulwark);
    SetClassAndDominantClass(*red_entity, CombatClass::kArcanite, CombatClass::kFighter);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());
    // Execute a large number of time steps to gain hyper
    for (int i = 0; i < 5; i++)
    {
        world->TimeStep();
    }

    const auto& stats_component = blue_entity->Get<StatsComponent>();

    // Vanguard(Bulwark+ Rogue) Arcanite (Fighter+Psion)
    // Bulwark :-1 Fighter +2 Psion
    // Rogue : -2 Fighter  +1 Psion
    // Total 0 * 5
    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 0_fp);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 0_fp);
}

TEST_F(HyperSystemTest, HyperCombatClassBonusApplied)
{
    // Data for creating the combat units
    CombatUnitData class_bonus_unit_data = CreateCombatUnitData();
    class_bonus_unit_data.type_data.stats.Set(StatType::kMaxHealth, 10000_fp);

    // Data for creating the combat units
    CombatUnitData data_red = CreateCombatUnitData();
    data_red.type_data.stats.Set(StatType::kMaxHealth, 10000_fp);

    // Data for creating the combat units
    CombatUnitData data_red_psion = CreateCombatUnitData();
    data_red_psion.type_data.stats.Set(StatType::kMaxHealth, 10000_fp);

    // Addattack abilities
    {
        auto& ability = class_bonus_unit_data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }
    class_bonus_unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Add Entities
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {-20, -40}, class_bonus_unit_data, blue_entity);
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {-15, -30}, data_red, red_entity);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, {-10, -20}, data_red, red_entity2);
    Entity* red_entity3 = nullptr;
    SpawnCombatUnit(Team::kRed, {-5, -10}, data_red, red_entity3);
    Entity* red_entity4 = nullptr;
    SpawnCombatUnit(Team::kRed, {15, 30}, data_red_psion, red_entity4);

    // Set correct affinities and classes
    SetAffinityAndDominantAffinity(*blue_entity, CombatAffinity::kEarth);
    SetClassAndDominantClass(*blue_entity, CombatClass::kFighter);

    SetAffinityAndDominantAffinity(*red_entity, CombatAffinity::kFire);
    SetClassAndDominantClass(*red_entity, CombatClass::kRogue);

    SetAffinityAndDominantAffinity(*red_entity2, CombatAffinity::kFire);
    SetClassAndDominantClass(*red_entity2, CombatClass::kRogue);

    SetAffinityAndDominantAffinity(*red_entity3, CombatAffinity::kFire);
    SetClassAndDominantClass(*red_entity3, CombatClass::kRogue);

    SetAffinityAndDominantAffinity(*red_entity4, CombatAffinity::kAir);
    SetClassAndDominantClass(*red_entity4, CombatClass::kPsion);

    auto& combat_synergy_component = blue_entity->Get<CombatSynergyComponent>();
    combat_synergy_component.SetCombatClass(CombatClass::kFighter);

    const auto& stats_component = blue_entity->Get<StatsComponent>();
    StatsData live_stats = world->GetLiveStats(*blue_entity);

    ASSERT_EQ(live_stats.Get(StatType::kEnergyResist), 0_fp);

    ASSERT_TRUE(TimeStepUntilGoneHyperactive());

    // Went hyperactive
    ASSERT_EQ(events_gone_hyperactive.size(), 1);
    EXPECT_EQ(events_gone_hyperactive.at(0).entity_id, blue_entity->GetID());

    // (2*3 Rogue effectiveness - 2 Psion effectiveness )* 100 time step = 400
    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 100_fp * kPrecisionFactorFP);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 100_fp);

    live_stats = world->GetLiveStats(*blue_entity);

    ASSERT_EQ(live_stats.Get(StatType::kEnergyResist), 50_fp);
}

TEST_F(HyperSystemTest, HyperCombatClassCompositeBonusApplied)
{
    // Data for creating the combat units
    CombatUnitData double_bonus_unit_data = CreateCombatUnitData();
    double_bonus_unit_data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

    // Data for creating the combat units
    CombatUnitData data_red_psion = CreateCombatUnitData();
    data_red_psion.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

    // Data for creating the combat units
    CombatUnitData data_red_empath = CreateCombatUnitData();
    data_red_empath.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

    // Addattack abilities
    {
        auto& ability = double_bonus_unit_data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));
    }
    double_bonus_unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Add Entities
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, double_bonus_unit_data, blue_entity);
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 20}, data_red_psion, red_entity);
    Entity* red_entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, {30, 30}, data_red_empath, red_entity2);

    // Set correct affinities and classes
    SetAffinityAndDominantAffinity(*blue_entity, CombatAffinity::kEarth);
    SetClassAndDominantClass(*blue_entity, CombatClass::kPredator, CombatClass::kRogue);

    SetAffinityAndDominantAffinity(*red_entity, CombatAffinity::kFire);
    SetClassAndDominantClass(*red_entity, CombatClass::kPsion);

    SetAffinityAndDominantAffinity(*red_entity2, CombatAffinity::kNature);
    SetClassAndDominantClass(*red_entity2, CombatClass::kEmpath);

    const auto& stats_component = blue_entity->Get<StatsComponent>();
    StatsData live_stats = world->GetLiveStats(*blue_entity);

    ASSERT_EQ(live_stats.Get(StatType::kCritChancePercentage), 25_fp);

    ASSERT_TRUE(TimeStepUntilGoneHyperactive());

    // Went hyperactive
    ASSERT_EQ(events_gone_hyperactive.size(), 1);
    EXPECT_EQ(events_gone_hyperactive.at(0).entity_id, blue_entity->GetID());

    // (2*3 Rogue effectiveness - 2 Psion effectiveness )* 100 time step = 400
    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 100_fp * kPrecisionFactorFP);
    EXPECT_EQ(stats_component.GetCurrentHyper(), 100_fp);

    live_stats = world->GetLiveStats(*blue_entity);

    EXPECT_EQ(live_stats.Get(StatType::kCritChancePercentage), 35_fp);
}

TEST_F(HyperHelperTest, GetCombatAffinityEffectiveness)
{
    const HyperData& hyper_data = world->GetGameDataContainer().GetHyperData();

    EXPECT_EQ(0, hyper_data.GetCombatAffinityEffectiveness(CombatAffinity::kNature, CombatAffinity::kNature));
    EXPECT_EQ(2, hyper_data.GetCombatAffinityEffectiveness(CombatAffinity::kNature, CombatAffinity::kAir));
    EXPECT_EQ(-1, hyper_data.GetCombatAffinityEffectiveness(CombatAffinity::kNature, CombatAffinity::kEarth));
}

TEST_F(HyperHelperTest, HyperEffectivenessInExpression)
{
    // Red entity
    HexGridPosition red_pos{10, 10};
    CombatUnitData data_red = CreateCombatUnitData();
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, red_pos, data_red, red_entity);
    ASSERT_NE(red_entity, nullptr);
    SetAffinityAndDominantAffinity(*red_entity, CombatAffinity::kFire);
    SetClassAndDominantClass(*red_entity, CombatClass::kRogue);

    // Blue entity
    CombatUnitData data_blue = CreateCombatUnitData();
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, Reflect(red_pos), data_blue, blue_entity);
    ASSERT_NE(blue_entity, nullptr);
    SetAffinityAndDominantAffinity(*blue_entity, CombatAffinity::kWater);
    SetClassAndDominantClass(*blue_entity, CombatClass::kFighter);

    auto expression = EffectExpression::FromHyperEffectiveness(ExpressionDataSourceType::kSender);
    ASSERT_EQ(world->EvaluateExpression(expression, red_entity->GetID()), -1_fp);
    ASSERT_EQ(world->EvaluateExpression(expression, blue_entity->GetID()), 1_fp);
}

TEST_F(HyperHelperTest, RangerWithItems)
{
    // Add weapon data
    constexpr CombatAffinity weapon_affinity = CombatAffinity::kWater;
    constexpr CombatClass weapon_class = CombatClass::kFighter;
    CombatWeaponTypeID weapon_id;
    weapon_id.name = "Weapon";
    {
        auto weapon_data = std::make_shared<CombatUnitWeaponData>();
        weapon_data->type_id = weapon_id;
        weapon_data->combat_affinity = weapon_affinity;
        weapon_data->dominant_combat_affinity = weapon_affinity;
        weapon_data->combat_class = weapon_class;
        weapon_data->dominant_combat_class = weapon_class;
        weapon_data->weapon_type = WeaponType::kNormal;
        game_data_container_->AddWeaponData(weapon_id, weapon_data);
    }

    // Add suit data
    CombatSuitTypeID suit_id;
    suit_id.name = "Suit";
    {
        auto suit_data = std::make_shared<CombatUnitSuitData>();
        suit_data->type_id = suit_id;
        game_data_container_->AddSuitData(suit_id, suit_data);
    }

    // Red entity (illuvial)
    constexpr HexGridPosition red_pos{10, 10};
    FullCombatUnitData data_red;
    data_red.data.radius_units = 1;
    data_red.data.type_id.type = CombatUnitType::kIlluvial;
    data_red.data.type_id.line_name = "Illuvial";
    data_red.data.type_data.combat_affinity = CombatAffinity::kFire;
    data_red.data.type_data.combat_class = CombatClass::kRogue;
    data_red.instance.id = GenerateRandomUniqueID();
    data_red.instance.team = Team::kRed;
    data_red.instance.position = red_pos;
    data_red.instance.dominant_combat_affinity = CombatAffinity::kFire;
    data_red.instance.dominant_combat_class = CombatClass::kRogue;

    Entity* red_entity = nullptr;
    SpawnCombatUnit(*world, data_red, red_entity);
    ASSERT_NE(red_entity, nullptr);
    ASSERT_EQ(red_entity->Get<CombatSynergyComponent>().GetDominantCombatAffinity(), CombatAffinity::kFire);
    ASSERT_EQ(red_entity->Get<CombatSynergyComponent>().GetDominantCombatClass(), CombatClass::kRogue);

    // Blue entity

    FullCombatUnitData data_blue;
    data_blue.data.radius_units = 1;
    data_blue.data.type_id.type = CombatUnitType::kRanger;
    data_blue.data.type_id.line_name = "Ranger";
    data_blue.instance.equipped_weapon.type_id = weapon_id;
    data_blue.instance.equipped_weapon.id = GenerateRandomUniqueID();
    data_blue.instance.equipped_suit.type_id = suit_id;
    data_blue.instance.equipped_suit.id = GenerateRandomUniqueID();
    data_blue.instance.id = GenerateRandomUniqueID();
    data_blue.instance.team = Team::kBlue;
    data_blue.instance.position = Reflect(red_pos);

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(*world, data_blue, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    auto expression = EffectExpression::FromHyperEffectiveness(ExpressionDataSourceType::kSender);
    ASSERT_EQ(world->EvaluateExpression(expression, red_entity->GetID()), -1_fp);
    ASSERT_EQ(world->EvaluateExpression(expression, blue_entity->GetID()), 1_fp);
}

TEST_F(HyperHelperTest, BondedRanger)
{
    // Add weapon data
    CombatWeaponTypeID weapon_id;
    weapon_id.name = "Weapon";
    {
        auto weapon_data = std::make_shared<CombatUnitWeaponData>();
        weapon_data->type_id = weapon_id;
        weapon_data->weapon_type = WeaponType::kNormal;
        game_data_container_->AddWeaponData(weapon_id, weapon_data);
    }

    // Add suit data
    CombatSuitTypeID suit_id;
    suit_id.name = "Suit";
    {
        auto suit_data = std::make_shared<CombatUnitSuitData>();
        suit_data->type_id = suit_id;
        game_data_container_->AddSuitData(suit_id, suit_data);
    }

    // Red entity (illuvial)
    constexpr HexGridPosition red_pos{10, 10};
    FullCombatUnitData red_data;
    red_data.data.radius_units = 0;
    red_data.data.type_id.type = CombatUnitType::kIlluvial;
    red_data.data.type_id.line_name = "Red Illuvial";
    red_data.data.type_data.combat_affinity = CombatAffinity::kFire;
    red_data.data.type_data.combat_class = CombatClass::kRogue;
    red_data.instance.id = GenerateRandomUniqueID();
    red_data.instance.team = Team::kRed;
    red_data.instance.position = red_pos;
    red_data.instance.dominant_combat_affinity = CombatAffinity::kFire;
    red_data.instance.dominant_combat_class = CombatClass::kRogue;

    Entity* red_entity = nullptr;
    SpawnCombatUnit(*world, red_data, red_entity);
    ASSERT_NE(red_entity, nullptr);
    ASSERT_EQ(red_entity->Get<CombatSynergyComponent>().GetDominantCombatAffinity(), CombatAffinity::kFire);
    ASSERT_EQ(red_entity->Get<CombatSynergyComponent>().GetDominantCombatClass(), CombatClass::kRogue);

    // Blue illuviual
    FullCombatUnitData blue_illuvial_data;
    blue_illuvial_data.data.radius_units = 0;
    blue_illuvial_data.data.type_id.type = CombatUnitType::kIlluvial;
    blue_illuvial_data.data.type_id.line_name = "Blue Illuvial";
    blue_illuvial_data.data.type_data.combat_affinity = CombatAffinity::kWater;
    blue_illuvial_data.data.type_data.combat_class = CombatClass::kFighter;
    blue_illuvial_data.instance.id = GenerateRandomUniqueID();
    blue_illuvial_data.instance.team = Team::kBlue;
    blue_illuvial_data.instance.position = Reflect(red_pos);
    blue_illuvial_data.instance.dominant_combat_affinity = CombatAffinity::kWater;
    blue_illuvial_data.instance.dominant_combat_class = CombatClass::kFighter;

    Entity* blue_illuvial = nullptr;
    SpawnCombatUnit(*world, blue_illuvial_data, blue_illuvial);
    ASSERT_NE(blue_illuvial, nullptr);
    ASSERT_EQ(blue_illuvial->Get<CombatSynergyComponent>().GetDominantCombatAffinity(), CombatAffinity::kWater);
    ASSERT_EQ(blue_illuvial->Get<CombatSynergyComponent>().GetDominantCombatClass(), CombatClass::kFighter);

    // Blue ranger
    FullCombatUnitData blue_ranger_data;
    blue_ranger_data.data.radius_units = 0;
    blue_ranger_data.data.type_id.type = CombatUnitType::kRanger;
    blue_ranger_data.data.type_id.line_name = "Ranger";
    blue_ranger_data.instance.equipped_weapon.type_id = weapon_id;
    blue_ranger_data.instance.equipped_weapon.id = GenerateRandomUniqueID();
    blue_ranger_data.instance.equipped_suit.type_id = suit_id;
    blue_ranger_data.instance.equipped_suit.id = GenerateRandomUniqueID();
    blue_ranger_data.instance.id = GenerateRandomUniqueID();
    blue_ranger_data.instance.team = Team::kBlue;
    blue_ranger_data.instance.position = Reflect(red_pos) + HexGridPosition{1, 1};
    blue_ranger_data.instance.bonded_id = blue_illuvial_data.instance.id;

    Entity* blue_ranger = nullptr;
    SpawnCombatUnit(*world, blue_ranger_data, blue_ranger);
    ASSERT_NE(blue_ranger, nullptr);

    world->TimeStep();

    ASSERT_EQ(blue_ranger->Get<CombatSynergyComponent>().GetDominantCombatAffinity(), CombatAffinity::kWater);
    ASSERT_EQ(blue_ranger->Get<CombatSynergyComponent>().GetDominantCombatClass(), CombatClass::kFighter);

    auto expression = EffectExpression::FromHyperEffectiveness(ExpressionDataSourceType::kSender);
    ASSERT_EQ(world->EvaluateExpression(expression, red_entity->GetID()), -2_fp);
    ASSERT_EQ(world->EvaluateExpression(expression, blue_illuvial->GetID()), 1_fp);
    ASSERT_EQ(world->EvaluateExpression(expression, blue_ranger->GetID()), 1_fp);
}

TEST_F(HyperHelperTest, BondedRangerWithSynergiesFromItems)
{
    // Add weapon data
    CombatWeaponTypeID weapon_id;
    weapon_id.name = "Weapon";
    {
        auto weapon_data = std::make_shared<CombatUnitWeaponData>();
        weapon_data->combat_affinity = CombatAffinity::kWater;
        weapon_data->dominant_combat_affinity = CombatAffinity::kWater;
        weapon_data->combat_class = CombatClass::kFighter;
        weapon_data->dominant_combat_class = CombatClass::kFighter;
        weapon_data->type_id = weapon_id;
        weapon_data->weapon_type = WeaponType::kNormal;
        game_data_container_->AddWeaponData(weapon_id, weapon_data);
    }

    // Add suit data
    CombatSuitTypeID suit_id;
    suit_id.name = "Suit";
    {
        auto suit_data = std::make_shared<CombatUnitSuitData>();
        suit_data->type_id = suit_id;
        game_data_container_->AddSuitData(suit_id, suit_data);
    }

    // Red entity (illuvial)
    constexpr HexGridPosition red_pos{10, 10};
    FullCombatUnitData red_data;
    red_data.data.radius_units = 0;
    red_data.data.type_id.type = CombatUnitType::kIlluvial;
    red_data.data.type_id.line_name = "Red Illuvial";
    red_data.data.type_data.combat_affinity = CombatAffinity::kFire;
    red_data.data.type_data.combat_class = CombatClass::kRogue;
    red_data.instance.id = GenerateRandomUniqueID();
    red_data.instance.team = Team::kRed;
    red_data.instance.position = red_pos;
    red_data.instance.dominant_combat_affinity = CombatAffinity::kFire;
    red_data.instance.dominant_combat_class = CombatClass::kRogue;

    Entity* red_entity = nullptr;
    SpawnCombatUnit(*world, red_data, red_entity);
    ASSERT_NE(red_entity, nullptr);
    ASSERT_EQ(red_entity->Get<CombatSynergyComponent>().GetDominantCombatAffinity(), CombatAffinity::kFire);
    ASSERT_EQ(red_entity->Get<CombatSynergyComponent>().GetDominantCombatClass(), CombatClass::kRogue);

    // Blue illuviual
    FullCombatUnitData blue_illuvial_data;
    blue_illuvial_data.data.radius_units = 0;
    blue_illuvial_data.data.type_id.type = CombatUnitType::kIlluvial;
    blue_illuvial_data.data.type_id.line_name = "Blue Illuvial";
    blue_illuvial_data.data.type_data.combat_affinity = CombatAffinity::kFire;
    blue_illuvial_data.data.type_data.combat_class = CombatClass::kBulwark;
    blue_illuvial_data.instance.id = GenerateRandomUniqueID();
    blue_illuvial_data.instance.team = Team::kBlue;
    blue_illuvial_data.instance.position = Reflect(red_pos);
    blue_illuvial_data.instance.dominant_combat_affinity = CombatAffinity::kFire;
    blue_illuvial_data.instance.dominant_combat_class = CombatClass::kBulwark;

    Entity* blue_illuvial = nullptr;
    SpawnCombatUnit(*world, blue_illuvial_data, blue_illuvial);
    ASSERT_NE(blue_illuvial, nullptr);
    ASSERT_EQ(blue_illuvial->Get<CombatSynergyComponent>().GetDominantCombatAffinity(), CombatAffinity::kFire);
    ASSERT_EQ(blue_illuvial->Get<CombatSynergyComponent>().GetDominantCombatClass(), CombatClass::kBulwark);

    // Blue ranger
    FullCombatUnitData blue_ranger_data;
    blue_ranger_data.data.radius_units = 0;
    blue_ranger_data.data.type_id.type = CombatUnitType::kRanger;
    blue_ranger_data.data.type_id.line_name = "Ranger";
    blue_ranger_data.instance.equipped_weapon.type_id = weapon_id;
    blue_ranger_data.instance.equipped_weapon.id = GenerateRandomUniqueID();
    blue_ranger_data.instance.equipped_suit.type_id = suit_id;
    blue_ranger_data.instance.equipped_suit.id = GenerateRandomUniqueID();
    blue_ranger_data.instance.id = GenerateRandomUniqueID();
    blue_ranger_data.instance.team = Team::kBlue;
    blue_ranger_data.instance.position = Reflect(red_pos) + HexGridPosition{1, 1};
    blue_ranger_data.instance.bonded_id = blue_illuvial_data.instance.id;

    Entity* blue_ranger = nullptr;
    SpawnCombatUnit(*world, blue_ranger_data, blue_ranger);
    ASSERT_NE(blue_ranger, nullptr);

    world->TimeStep();

    // Dominants from weapon have priority
    ASSERT_EQ(blue_ranger->Get<CombatSynergyComponent>().GetDominantCombatAffinity(), CombatAffinity::kWater);
    ASSERT_EQ(blue_ranger->Get<CombatSynergyComponent>().GetDominantCombatClass(), CombatClass::kFighter);

    auto expression = EffectExpression::FromHyperEffectiveness(ExpressionDataSourceType::kSender);
    ASSERT_EQ(world->EvaluateExpression(expression, red_entity->GetID()), -1_fp);
    ASSERT_EQ(world->EvaluateExpression(expression, blue_illuvial->GetID()), 0_fp);
    ASSERT_EQ(world->EvaluateExpression(expression, blue_ranger->GetID()), 1_fp);
}

}  // namespace simulation
