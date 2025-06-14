#include "base_test_fixtures.h"
#include "components/decision_component.h"
#include "components/focus_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/stats_component.h"
#include "data/combat_unit_data.h"
#include "data/effect_data.h"
#include "gtest/gtest.h"
#include "systems/movement_system.h"
#include "utility/attached_effects_helper.h"
#include "utility/grid_helper.h"

namespace simulation
{
class MovementSystemTest : public BaseTest
{
    typedef BaseTest Super;

protected:
    void SetUp() override
    {
        Super::SetUp();

        SetStats();
    }

    virtual void SetStats()
    {
        stats.Set(StatType::kAttackRangeUnits, 2_fp);
        stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);
    }

    StatsData stats;
};

class MovementSystemFastMovementTest : public MovementSystemTest
{
    typedef MovementSystemTest Super;

protected:
    void SetStats() override
    {
        Super::SetStats();
        stats.Set(StatType::kMoveSpeedSubUnits, 500000_fp);
    }
};

TEST_F(MovementSystemTest, FindPathSimple)
{
    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    auto& blue_stats_component = blue_entity.Add<StatsComponent>();
    blue_stats_component.SetTemplateStats(stats);
    const auto& blue_movement_component = blue_entity.Add<MovementComponent>();

    // Set position of entity at 5, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(5, 10);
    blue_position_component.SetRadius(1);
    blue_position_component.SetTakingSpace(true);

    // Add red entity with components
    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    red_entity.Add<FocusComponent>();
    red_entity.Add<StatsComponent>();
    red_entity.Add<MovementComponent>();

    // Set position of entity at 20, 40
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(20, 40);
    red_position_component.SetRadius(1);
    red_position_component.SetTakingSpace(true);

    // Run systems
    world->TimeStep();

    // Check outcome
    ASSERT_TRUE(blue_movement_component.HasWaypoints());
    const HexGridPosition& waypoint = blue_movement_component.TopWaypoint();

    // Should be going direct to the target
    EXPECT_EQ(waypoint.q, 18);
    EXPECT_EQ(waypoint.r, 37);
}

TEST_F(MovementSystemTest, FindPathComplex)
{
    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    auto& blue_stats_component = blue_entity.Add<StatsComponent>();
    blue_stats_component.SetTemplateStats(stats);
    const auto& blue_movement_component = blue_entity.Add<MovementComponent>();

    // Set position of entity at 5, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(5, 10);
    blue_position_component.SetRadius(5);
    blue_position_component.SetTakingSpace(true);

    // Add red entity with components
    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    red_entity.Add<FocusComponent>();
    red_entity.Add<StatsComponent>();
    red_entity.Add<MovementComponent>();

    // Set position of entity at 25, 50
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(25, 50);
    red_position_component.SetRadius(5);
    red_position_component.SetTakingSpace(true);

    // Add blue blocker entity with position component
    auto& blocker_entity = world->AddEntity(Team::kBlue);
    blocker_entity.Add<PositionComponent>();

    // Set position of entity at 13, 25
    auto& blocker_position_component = blocker_entity.Get<PositionComponent>();
    blocker_position_component.SetPosition(13, 25);
    blocker_position_component.SetRadius(5);
    blocker_position_component.SetTakingSpace(true);

    // Run systems
    world->TimeStep();

    // Check outcome
    ASSERT_TRUE(blue_movement_component.HasWaypoints());
    HexGridPosition waypoint = blue_movement_component.TopWaypoint();

    // Should be on the path
    EXPECT_EQ(waypoint.q, 13);
    EXPECT_EQ(waypoint.r, 14);
}

TEST_F(MovementSystemFastMovementTest, FindPathFastMoving)
{
    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    auto& blue_stats_component = blue_entity.Add<StatsComponent>();
    blue_stats_component.SetTemplateStats(stats);
    auto& blue_movement_component = blue_entity.Add<MovementComponent>();

    // Set movement speed really fast
    blue_movement_component.SetMovementSpeedSubUnits(stats.Get(StatType::kMoveSpeedSubUnits));

    // Set position of entity at 5, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(5, 10);
    blue_position_component.SetRadius(5);
    blue_position_component.SetTakingSpace(true);

    // Add red entity with components
    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    red_entity.Add<FocusComponent>();
    red_entity.Add<StatsComponent>();
    const auto& red_movement_component = red_entity.Add<MovementComponent>();

    // Set position of entity at 25, 50
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(25, 50);
    red_position_component.SetRadius(5);
    red_position_component.SetTakingSpace(true);

    // Add blue blocker entity with position component
    auto& blocker_entity = world->AddEntity(Team::kBlue);
    blocker_entity.Add<PositionComponent>();

    // Set position of entity at 13, 25
    auto& blocker_position_component = blocker_entity.Get<PositionComponent>();
    blocker_position_component.SetPosition(13, 25);
    blocker_position_component.SetRadius(5);
    blocker_position_component.SetTakingSpace(true);

    // Run systems
    world->TimeStep();

    // Check outcome
    // Waypoints are consumed
    ASSERT_FALSE(blue_movement_component.HasWaypoints());
    ASSERT_FALSE(red_movement_component.HasWaypoints());

    // Should be on the path
    // Blue moving fast
    EXPECT_EQ(blue_position_component.GetQ(), 24);
    EXPECT_EQ(blue_position_component.GetR(), 38);
    // Red moving slow
    EXPECT_EQ(red_position_component.GetQ(), 25);
    EXPECT_EQ(red_position_component.GetR(), 48);
}

TEST_F(MovementSystemTest, FindPathBlocked)
{
    // Add blue entities with components
    auto& blue_entity1 = world->AddEntity(Team::kBlue);
    blue_entity1.Add<PositionComponent>();
    auto& blue_focus_component = blue_entity1.Add<FocusComponent>();
    blue_entity1.Add<StatsComponent>();
    auto& blue_movement_component = blue_entity1.Add<MovementComponent>();
    blue_entity1.Add<DecisionComponent>();

    auto& blue_entity2 = world->AddEntity(Team::kBlue);
    blue_entity2.Add<PositionComponent>();
    blue_entity2.Add<FocusComponent>();
    blue_entity2.Add<StatsComponent>();

    auto& blue_entity3 = world->AddEntity(Team::kBlue);
    blue_entity3.Add<PositionComponent>();
    blue_entity3.Add<FocusComponent>();
    blue_entity3.Add<StatsComponent>();

    auto& blue_entity4 = world->AddEntity(Team::kBlue);
    blue_entity4.Add<PositionComponent>();
    blue_entity4.Add<FocusComponent>();
    blue_entity4.Add<StatsComponent>();

    // Set position of first blue entity
    auto& blue_position_component1 = blue_entity1.Get<PositionComponent>();
    blue_position_component1.SetPosition(27, 45);
    blue_position_component1.SetRadius(2);
    blue_position_component1.SetTakingSpace(true);

    // Set position of second blue entity
    auto& blue_position_component2 = blue_entity2.Get<PositionComponent>();
    blue_position_component2.SetPosition(20, 48);
    blue_position_component2.SetRadius(2);
    blue_position_component2.SetTakingSpace(true);

    // Set position of third blue entity
    auto& blue_position_component3 = blue_entity3.Get<PositionComponent>();
    blue_position_component3.SetPosition(24, 45);
    blue_position_component3.SetRadius(2);
    blue_position_component3.SetTakingSpace(true);

    // Set position of fourth blue entity
    auto& blue_position_component4 = blue_entity4.Get<PositionComponent>();
    blue_position_component4.SetPosition(25, 48);
    blue_position_component4.SetRadius(2);
    blue_position_component4.SetTakingSpace(true);

    // Add red entities with components
    auto& red_entity1 = world->AddEntity(Team::kRed);
    red_entity1.Add<PositionComponent>();
    red_entity1.Add<FocusComponent>();
    red_entity1.Add<StatsComponent>();

    auto& red_entity2 = world->AddEntity(Team::kRed);
    red_entity2.Add<PositionComponent>();
    red_entity2.Add<FocusComponent>();
    red_entity2.Add<StatsComponent>();

    // Set position of first red entity
    auto& red_position_component1 = red_entity1.Get<PositionComponent>();
    red_position_component1.SetPosition(22, 48);
    red_position_component1.SetRadius(1);
    red_position_component1.SetTakingSpace(true);

    // Set position of second red entity
    auto& red_position_component2 = red_entity2.Get<PositionComponent>();
    red_position_component2.SetPosition(5, 7);
    red_position_component2.SetRadius(1);
    red_position_component2.SetTakingSpace(true);

    // Set blue movement speed
    blue_movement_component.SetMovementSpeedSubUnits(1000_fp);

    // Track events
    int unreachable_events = 0;
    int focus_found_events = 0;
    world->SubscribeToEvent(
        EventType::kFocusUnreachable,
        [&unreachable_events](const Event&)
        {
            unreachable_events++;
        });
    world->SubscribeToEvent(
        EventType::kFocusFound,
        [&focus_found_events](const Event&)
        {
            focus_found_events++;
        });

    // Initial focus should be 4, but the pending collision is detected immediately
    ASSERT_EQ(blue_focus_component.GetFocusID(), kInvalidEntityID);
    ASSERT_EQ(unreachable_events, 0);

    // Pathfinding should detect the block here
    // Wait for time limit
    // Subtract one to monitor change
    for (int i = 0; i < world->GetUnreachableFocusTimeStepLimit() - 1; i++)
    {
        world->TimeStep();

        if (i < world->GetUnreachableFocusTimeStepLimit() - 2)
        {
            // Should have ongoing focus events
            EXPECT_EQ(focus_found_events, 5 * i + 10);
        }
    }

    // Should have changed focus and hit unreachable
    EXPECT_EQ(focus_found_events, 25);
    EXPECT_EQ(unreachable_events, 1);
}

TEST_F(MovementSystemTest, MultipleRetriesForPathfindingBeforeSucceeding)
{
    // Add blue entities with components
    auto& blue_entity1 = world->AddEntity(Team::kBlue);
    blue_entity1.Add<PositionComponent>();
    blue_entity1.Add<StatsComponent>();

    auto& blue_entity2 = world->AddEntity(Team::kBlue);
    blue_entity2.Add<PositionComponent>();
    blue_entity2.Add<FocusComponent>();
    blue_entity2.Add<StatsComponent>();

    auto& blue_entity3 = world->AddEntity(Team::kBlue);
    blue_entity3.Add<PositionComponent>();
    blue_entity3.Add<FocusComponent>();
    blue_entity3.Add<StatsComponent>();

    auto& blue_entity4 = world->AddEntity(Team::kBlue);
    blue_entity4.Add<PositionComponent>();
    blue_entity4.Add<FocusComponent>();
    blue_entity4.Add<StatsComponent>();

    auto& blue_entity5 = world->AddEntity(Team::kBlue);
    blue_entity5.Add<PositionComponent>();
    blue_entity5.Add<FocusComponent>();
    blue_entity5.Add<StatsComponent>();

    auto& blue_entity6 = world->AddEntity(Team::kBlue);
    blue_entity6.Add<PositionComponent>();
    blue_entity6.Add<FocusComponent>();
    blue_entity6.Add<StatsComponent>();

    // Six blue entities in a hex shape to leave space in the middle
    // so the red tries to path there, but fails.
    // Set position of first blue entity
    auto& blue_position_component1 = blue_entity1.Get<PositionComponent>();
    blue_position_component1.SetPosition(25, 20);
    blue_position_component1.SetRadius(5);
    blue_position_component1.SetTakingSpace(true);

    // Set position of second blue entity
    auto& blue_position_component2 = blue_entity2.Get<PositionComponent>();
    blue_position_component2.SetPosition(14, 26);
    blue_position_component2.SetRadius(5);
    blue_position_component2.SetTakingSpace(true);

    // Set position of third blue entity
    auto& blue_position_component3 = blue_entity3.Get<PositionComponent>();
    blue_position_component3.SetPosition(30, 26);
    blue_position_component3.SetRadius(5);
    blue_position_component3.SetTakingSpace(true);

    // Set position of fourth blue entity
    auto& blue_position_component4 = blue_entity4.Get<PositionComponent>();
    blue_position_component4.SetPosition(9, 37);
    blue_position_component4.SetRadius(5);
    blue_position_component4.SetTakingSpace(true);

    // Set position of fifth blue entity
    auto& blue_position_component5 = blue_entity5.Get<PositionComponent>();
    blue_position_component5.SetPosition(24, 37);
    blue_position_component5.SetRadius(5);
    blue_position_component5.SetTakingSpace(true);

    // Set position of sixth blue entity
    auto& blue_position_component6 = blue_entity6.Get<PositionComponent>();
    blue_position_component6.SetPosition(13, 44);
    blue_position_component6.SetRadius(5);
    blue_position_component6.SetTakingSpace(true);

    // Add red entity with components
    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    auto& red_focus_component = red_entity.Add<FocusComponent>();
    red_entity.Add<StatsComponent>();
    auto& red_movement_component = red_entity.Add<MovementComponent>();
    red_entity.Add<DecisionComponent>();

    // Set position of red entity
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(8, 54);
    red_position_component.SetRadius(2);
    red_position_component.SetTakingSpace(true);

    // Force red to attack first blue entity
    red_focus_component.SetFocus(world->GetByIDPtr(0));

    // Set red movement speed
    red_movement_component.SetMovementSpeedSubUnits(1000_fp);

    // Let red pathfind and move once
    world->TimeStep();

    // Check for position
    EXPECT_EQ(red_position_component.GetQ(), 7);
    EXPECT_EQ(red_position_component.GetR(), 54);
    const HexGridPosition sub_units = red_position_component.GetSubUnitPosition();
    EXPECT_EQ(sub_units.q, 400);
    EXPECT_EQ(sub_units.r, -400);
}

TEST_F(MovementSystemTest, MoveEntity)
{
    // Add blue entity with position component
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<MovementComponent>();

    // Set position of entity at 10, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);
    blue_position_component.SetTakingSpace(true);

    // Isolated movement system for testing
    MovementSystem movement_system = MovementSystem();
    movement_system.Init(world.get());

    // Need obstacles for collisions
    GetGridHelper().BuildObstacles(&blue_entity);

    // Move the entity
    movement_system.MoveEntity(blue_entity, blue_position_component, HexGridPosition{35000, 35000});

    // Check for position
    EXPECT_EQ(blue_position_component.GetQ(), 45);
    EXPECT_EQ(blue_position_component.GetR(), 45);

    // Move the entity again
    movement_system.MoveEntity(blue_entity, blue_position_component, HexGridPosition{-5000, -5000});

    // Check for position
    EXPECT_EQ(blue_position_component.GetQ(), 40);
    EXPECT_EQ(blue_position_component.GetR(), 40);
}

TEST_F(MovementSystemTest, MoveToward)
{
    // Add blue entity with position component
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<MovementComponent>();

    // Set position of entity at 15, 15
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(15, 15);
    blue_position_component.SetTakingSpace(true);

    // Isolated movement system for testing
    MovementSystem movement_system = MovementSystem();
    movement_system.Init(world.get());

    // Move the entity
    const int distance_moved1 =
        movement_system.MoveToward(blue_entity, blue_position_component, HexGridPosition{20, 20}, 2000);

    // Check for position - 2 * sqrt(2)/2 ~ 2 unit movement
    EXPECT_EQ(blue_position_component.GetQ(), 16);
    EXPECT_EQ(blue_position_component.GetR(), 16);
    EXPECT_EQ(distance_moved1, 2000);

    // Move the entity again
    const int distance_moved2 =
        movement_system.MoveToward(blue_entity, blue_position_component, HexGridPosition{10, 10}, 2000);

    // Check for position - 2 * sqrt(2)/2 ~ 2 unit movement
    EXPECT_EQ(blue_position_component.GetQ(), 15);
    EXPECT_EQ(blue_position_component.GetR(), 15);
    EXPECT_EQ(distance_moved2, 2000);
}

TEST_F(MovementSystemTest, Movement)
{
    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    auto& blue_stats_component = blue_entity.Add<StatsComponent>();
    blue_stats_component.SetTemplateStats(stats);
    blue_entity.Add<MovementComponent>();

    // Set position of entity at 5, 10
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(5, 10);
    blue_position_component.SetRadius(1);
    blue_position_component.SetTakingSpace(true);

    // Add red entity with components
    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    red_entity.Add<FocusComponent>();
    auto& red_stats_component = red_entity.Add<StatsComponent>();
    red_stats_component.SetTemplateStats(stats);
    red_entity.Add<MovementComponent>();

    // Set position of entity at 10, 20
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(10, 20);
    red_position_component.SetRadius(1);
    red_position_component.SetTakingSpace(true);

    // Add blue blocker entity with position component
    auto& blocker_entity = world->AddEntity(Team::kBlue);
    blocker_entity.Add<PositionComponent>();

    // Set position of entity at 13, 15
    auto& blocker_position_component = blocker_entity.Get<PositionComponent>();
    blocker_position_component.SetPosition(13, 15);
    blocker_position_component.SetRadius(1);

    // Take care of setting the destination and movement
    world->TimeStep();

    // TODO: Find better way of calculating test values
    // Manually calculated values
    EXPECT_EQ(blue_position_component.GetQ(), 6);
    EXPECT_EQ(blue_position_component.GetR(), 11);
    EXPECT_EQ(red_position_component.GetQ(), 10);
    EXPECT_EQ(red_position_component.GetR(), 18);
}

TEST_F(MovementSystemTest, LargeMovement)
{
    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    auto& blue_stats_component = blue_entity.Add<StatsComponent>();
    blue_stats_component.SetTemplateStats(stats);
    blue_entity.Add<MovementComponent>();

    // Set position of entity at -23, -45
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(-23, -45);
    blue_position_component.SetRadius(1);
    blue_position_component.SetTakingSpace(true);

    // Set movement speed to 8
    auto& blue_movement_component = blue_entity.Get<MovementComponent>();
    blue_movement_component.SetMovementSpeedSubUnits(8000_fp);

    // Add red entity with components
    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    red_entity.Add<FocusComponent>();
    auto& red_stats_component = red_entity.Add<StatsComponent>();
    red_stats_component.SetTemplateStats(stats);
    red_entity.Add<MovementComponent>();

    // Set position of entity at 23, 45
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(23, 45);
    red_position_component.SetRadius(1);
    red_position_component.SetTakingSpace(true);

    // Set movement speed to 8
    auto& red_movement_component = red_entity.Get<MovementComponent>();
    red_movement_component.SetMovementSpeedSubUnits(8000_fp);

    // Add blue blocker entity with position component
    auto& blocker_entity = world->AddEntity(Team::kBlue);
    blocker_entity.Add<PositionComponent>();

    // Set position of entity at 0, 0
    auto& blocker_position_component = blocker_entity.Get<PositionComponent>();
    blocker_position_component.SetPosition(0, 0);
    blocker_position_component.SetRadius(1);

    // Take care of setting the destination
    world->TimeStep();

    // TODO: Find better way of calculating test values
    // Manually calculated values
    EXPECT_EQ(blue_position_component.GetQ(), -20);
    EXPECT_EQ(blue_position_component.GetR(), -40);
    EXPECT_EQ(red_position_component.GetQ(), 20);
    EXPECT_EQ(red_position_component.GetR(), 40);
}

TEST_F(MovementSystemTest, SmallMovement)
{
    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    auto& blue_stats_component = blue_entity.Add<StatsComponent>();
    blue_stats_component.SetTemplateStats(stats);
    blue_entity.Add<MovementComponent>();

    // Set position of entity at -23, -45
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(-23, -45);
    blue_position_component.SetRadius(1);
    blue_position_component.SetTakingSpace(true);

    // Set movement speed to 20
    auto& blue_movement_component = blue_entity.Get<MovementComponent>();
    blue_movement_component.SetMovementSpeedSubUnits(20_fp);

    // Add red entity with components
    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    red_entity.Add<FocusComponent>();
    auto& red_stats_component = red_entity.Add<StatsComponent>();
    red_stats_component.SetTemplateStats(stats);
    red_entity.Add<MovementComponent>();

    // Set position of entity at 23, 45
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(23, 45);
    red_position_component.SetRadius(1);
    red_position_component.SetTakingSpace(true);

    // Set movement speed to 20
    auto& red_movement_component = red_entity.Get<MovementComponent>();
    red_movement_component.SetMovementSpeedSubUnits(20_fp);

    // Add blue blocker entity with position component
    auto& blocker_entity = world->AddEntity(Team::kBlue);
    blocker_entity.Add<PositionComponent>();

    // Set position of entity at 0, 0
    auto& blocker_position_component = blocker_entity.Get<PositionComponent>();
    blocker_position_component.SetPosition(0, 0);
    blocker_position_component.SetRadius(1);

    // Take care of setting the destination
    world->TimeStep();

    // TODO: Find better way of calculating test values
    // Manually calculated values
    EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(-23, -45));
    EXPECT_EQ(blue_position_component.GetSubUnitPosition(), HexGridPosition(6, 13));
    EXPECT_EQ(red_position_component.GetPosition(), HexGridPosition(23, 45));
    EXPECT_EQ(red_position_component.GetSubUnitPosition(), HexGridPosition(-6, -13));
}

TEST_F(MovementSystemTest, BlueBecomesUnMovable)
{
    // Data for creating the combat units
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 2_fp);

    constexpr HexGridPosition blue_initial_position{5, 10};
    constexpr HexGridPosition red_initial_position{20, 40};

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, blue_initial_position, data, blue_entity);
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, red_initial_position, data, red_entity);

    const auto& blue_position_component = blue_entity->Get<PositionComponent>();
    const auto& red_position_component = red_entity->Get<PositionComponent>();

    world->TimeStep();  // 1. kFindFocus, kMoveToFocus and first move
    EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(6, 11));
    EXPECT_EQ(blue_position_component.GetSubUnitPosition(), HexGridPosition(-350, 350));
    EXPECT_EQ(red_position_component.GetPosition(), HexGridPosition(19, 39));
    EXPECT_EQ(red_position_component.GetSubUnitPosition(), HexGridPosition(370, -368));

    // Blue has the negative state root applied, for 200 ms
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kRoot, 200);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, red_entity->GetID(), effect_data, effect_state);

    world->TimeStep();  // 2. Second move
    EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(6, 11));
    EXPECT_EQ(blue_position_component.GetSubUnitPosition(), HexGridPosition(-350, 350));
    EXPECT_EQ(red_position_component.GetPosition(), HexGridPosition(19, 37));
    EXPECT_EQ(red_position_component.GetSubUnitPosition(), HexGridPosition(-260, 264));

    world->TimeStep();  // 3. Third move
    EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(6, 11));
    EXPECT_EQ(blue_position_component.GetSubUnitPosition(), HexGridPosition(-350, 350));
    EXPECT_EQ(red_position_component.GetPosition(), HexGridPosition(18, 36));
    EXPECT_EQ(red_position_component.GetSubUnitPosition(), HexGridPosition(110, -104));

    // Blue should get unstuck at the end of this call
    world->TimeStep();  // 4. Fourth move
    EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(6, 11));
    EXPECT_EQ(blue_position_component.GetSubUnitPosition(), HexGridPosition(-350, 350));
    EXPECT_EQ(red_position_component.GetPosition(), HexGridPosition(17, 35));
    EXPECT_EQ(red_position_component.GetSubUnitPosition(), HexGridPosition(480, -472));

    // Everyone moves
    // 6. Sixth move
    world->TimeStep();
    EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(6, 13));
    EXPECT_EQ(blue_position_component.GetSubUnitPosition(), HexGridPosition(272, -274));
    EXPECT_EQ(red_position_component.GetPosition(), HexGridPosition(17, 33));
    EXPECT_EQ(red_position_component.GetSubUnitPosition(), HexGridPosition(-150, 160));
}

TEST_F(MovementSystemTest, TestNavigationWithAlternativeRounding)
{
    StatsData dodo_stats;
    dodo_stats.Set(StatType::kAttackRangeUnits, 18_fp);
    dodo_stats.Set(StatType::kMoveSpeedSubUnits, 3250_fp);

    StatsData dummy_stats;

    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<FocusComponent>();
    auto& blue_stats_component = blue_entity.Add<StatsComponent>();
    blue_stats_component.SetTemplateStats(dodo_stats);
    auto& blue_movement_component = blue_entity.Add<MovementComponent>();
    blue_movement_component.SetMovementSpeedSubUnits(dodo_stats.Get(StatType::kMoveSpeedSubUnits));

    // Set position of entity at 12, 19
    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(12, 19);
    blue_position_component.SetRadius(6);
    blue_position_component.SetTakingSpace(true);

    auto& blue_entity2 = world->AddEntity(Team::kBlue);
    blue_entity2.Add<PositionComponent>();
    auto& blue_stats_component2 = blue_entity2.Add<StatsComponent>();
    blue_stats_component2.SetTemplateStats(dummy_stats);

    // Set position of entity at -3, 31
    auto& blue_position_component2 = blue_entity2.Get<PositionComponent>();
    blue_position_component2.SetPosition(-3, 31);
    blue_position_component2.SetRadius(7);
    blue_position_component2.SetTakingSpace(true);

    // Add red entity with components
    auto& red_entity = world->AddEntity(Team::kRed);
    red_entity.Add<PositionComponent>();
    red_entity.Add<StatsComponent>();

    // Set position of entity at 20, 40
    auto& red_position_component = red_entity.Get<PositionComponent>();
    red_position_component.SetPosition(-21, 17);
    red_position_component.SetRadius(6);
    red_position_component.SetTakingSpace(true);

    // Run systems
    world->TimeStep();

    ASSERT_TRUE(blue_movement_component.HasWaypoints());
    HexGridPosition waypoint = blue_movement_component.TopWaypoint();

    // Should be going direct to the target
    EXPECT_EQ(waypoint.q, 10);
    EXPECT_EQ(waypoint.r, 17);

    world->TimeStep();

    ASSERT_FALSE(blue_movement_component.HasWaypoints());
    EXPECT_EQ(blue_position_component.GetPosition(), HexGridPosition(10, 17));
}

}  // namespace simulation
