#include "base_test_fixtures.h"
#include "components/position_component.h"
#include "data/combat_unit_data.h"
#include "ecs/event_types_data.h"
#include "gtest/gtest.h"
#include "utility/grid_helper.h"

namespace simulation
{
class SpawningTest : public BaseTest
{
    typedef BaseTest Super;

protected:
    virtual void InitCombatUnitData()
    {
        InitStats();
        InitAttackAbilities();
    }

    virtual void InitStats()
    {
        // CombatUnit stats
        // No abilities
        data_no_spawning.radius_units = GetRadius();
        data_no_spawning.type_data.combat_affinity = CombatAffinity::kFire;
        data_no_spawning.type_data.combat_class = CombatClass::kBehemoth;
        data_no_spawning.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data_no_spawning.type_data.stats.Set(StatType::kAttackRangeUnits, 2000_fp);
        data_no_spawning.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        data_no_spawning.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data_no_spawning.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
        data_no_spawning.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 20000_fp);

        // Spawning Combat Units
        data_with_spawning.radius_units = GetRadius();
        data_with_spawning.type_data.combat_affinity = CombatAffinity::kFire;
        data_with_spawning.type_data.combat_class = CombatClass::kBehemoth;
        data_with_spawning.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data_with_spawning.type_data.stats.Set(StatType::kAttackRangeUnits, 2000_fp);
        data_with_spawning.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        data_with_spawning.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data_with_spawning.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
        data_with_spawning.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 20000_fp);
    }

    virtual void InitAttackAbilities()
    {
        // Add attack abilities
        {
            auto& ability = data_with_spawning.type_data.attack_abilities.AddAbility();
            ability.name = "Spawning Ability";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Attack Spawning";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.targeting.group = AllegianceType::kEnemy;
            skill1.targeting.num = 1;
            skill1.deployment.type = SkillDeploymentType::kSpawnedCombatUnit;
            skill1.spawn.combat_unit = std::make_shared<CombatUnitData>(data_no_spawning);
            skill1.spawn.direction = GetSpawnDirection();
        }

        data_no_spawning.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
        data_with_spawning.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual void SpawnCombatUnits()
    {
        // Add blue Combat Units
        SpawnCombatUnit(Team::kBlue, HexGridPosition{0, 2}, data_with_spawning, blue_entity1);
        SpawnCombatUnit(Team::kBlue, HexGridPosition{-2, -25}, data_no_spawning, blue_entity2);

        // Add red Combat Units
        SpawnCombatUnit(Team::kRed, HexGridPosition{0, 10}, data_no_spawning, red_entity1);
        SpawnCombatUnit(Team::kRed, HexGridPosition{-2, 15}, data_no_spawning, red_entity2);
    }

    virtual int GetRadius() const
    {
        return 1;
    }

    virtual HexGridCardinalDirection GetSpawnDirection() const
    {
        return HexGridCardinalDirection::kRight;
    }

    void SetUpEventListeners()
    {
        // Listen to spawned Combat Units
        world->SubscribeToEvent(
            EventType::kCombatUnitCreated,
            [this](const Event& event)
            {
                events_combat_unit_created.emplace_back(event.Get<event_data::CombatUnitCreated>());
            });
    }

    void SetUp() override
    {
        Super::SetUp();

        InitCombatUnitData();
        SpawnCombatUnits();
        SetUpEventListeners();
    }

    CombatUnitData data_no_spawning;
    CombatUnitData data_with_spawning;

    Entity* blue_entity1 = nullptr;
    Entity* blue_entity2 = nullptr;
    Entity* blue_entity3 = nullptr;
    Entity* blue_entity4 = nullptr;
    Entity* red_entity1 = nullptr;
    Entity* red_entity2 = nullptr;

    std::vector<event_data::CombatUnitCreated> events_combat_unit_created;
};

class SpawningTestBlocked : public SpawningTest
{
    void SpawnCombatUnits() override
    {
        SpawnCombatUnit(Team::kBlue, HexGridPosition{0, -11}, data_with_spawning, blue_entity1);

        SpawnCombatUnit(Team::kBlue, HexGridPosition{0, 30}, data_no_spawning, blue_entity3);
        SpawnCombatUnit(Team::kBlue, HexGridPosition{30, 30}, data_no_spawning, blue_entity4);

        SpawnCombatUnit(Team::kRed, HexGridPosition{0, -40}, data_no_spawning, red_entity1);
    }

    int GetRadius() const override
    {
        return 10;
    }

    HexGridCardinalDirection GetSpawnDirection() const override
    {
        return HexGridCardinalDirection::kTopRight;
    }
};

TEST_F(SpawningTest, GetOpenPositionNear)
{
    // WTF?
    ASSERT_EQ(blue_entity1->Get<PositionComponent>().GetPosition(), HexGridPosition(0, 2));
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNear(*blue_entity1, HexGridCardinalDirection::kTopLeft, 1),
        HexGridPosition(0, -1));
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNear(*blue_entity1, HexGridCardinalDirection::kTopRight, 1),
        HexGridPosition(3, -1));
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNear(*blue_entity1, HexGridCardinalDirection::kRight, 1),
        HexGridPosition(3, 2));
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNear(*blue_entity1, HexGridCardinalDirection::kBottomRight, 1),
        HexGridPosition(0, 5));
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNear(*blue_entity1, HexGridCardinalDirection::kBottomLeft, 1),
        HexGridPosition(-3, 5));
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNear(*blue_entity1, HexGridCardinalDirection::kLeft, 1),
        HexGridPosition(-3, 2));
}

TEST_F(SpawningTest, SimpleSpawn)
{
    // Run for 3 time steps
    for (int time_step = 0; time_step < 1; time_step++)
    {
        // TimeStep
        world->TimeStep();
    }

    // 1 CombatUnit should have been spawned
    ASSERT_EQ(events_combat_unit_created.size(), 1);

    // Get entities and count
    const auto& entities = world->GetAll();
    ASSERT_EQ(entities.size(), 7);

    // Get new entity and position component
    const auto& new_entity = world->GetByID(6);
    const auto& position_component = new_entity.Get<PositionComponent>();

    // Verify position - to the right of red_entity1 (0, 10)
    EXPECT_EQ(position_component.GetQ(), 3);
    EXPECT_EQ(position_component.GetR(), 10);
}

TEST_F(SpawningTestBlocked, SpawnEdgeOfGrid)
{
    // Run for 3 time steps
    ASSERT_EQ(world->GetAll().size(), 4);
    for (int time_step = 0; time_step < 1; time_step++)
    {
        // TimeStep
        world->TimeStep();
    }

    // 1 CombatUnit should have been spawned
    ASSERT_EQ(events_combat_unit_created.size(), 1);

    // Get entities and count
    ASSERT_EQ(world->GetAll().size(), 7);

    // Get new entity and position component
    const auto& new_entity = world->GetByID(6);
    const auto& position_component = new_entity.Get<PositionComponent>();

    // Verify position - closest position going clockwise from preferred direction
    // Up of red 0, -30
    EXPECT_EQ(position_component.GetQ(), 21);
    EXPECT_EQ(position_component.GetR(), -61);
}

}  // namespace simulation
