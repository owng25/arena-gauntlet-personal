#include "base_test_fixtures.h"
#include "components/position_component.h"
#include "data/combat_unit_data.h"
#include "gtest/gtest.h"
#include "utility/grid_helper.h"
#include "utility/hex_grid_position.h"

namespace simulation
{
class GridTest : public BaseTest
{
    typedef BaseTest Super;

protected:
    virtual void InitCombatUnitData()
    {
        InitStats();
    }

    virtual void InitStats()
    {
        // CombatUnit stats
        // No abilities
        data.radius_units = GetRadius();
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 2000_fp);
        data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 20000_fp);
    }

    virtual void SpawnCombatUnits()
    {
        // Add blue Combat Units
        SpawnCombatUnit(Team::kBlue, HexGridPosition(-25, 25), data, blue_entity1);
        SpawnCombatUnit(Team::kBlue, HexGridPosition(-15, 25), data, blue_entity2);

        // Add red Combat Units
        SpawnCombatUnit(Team::kRed, HexGridPosition(-25, 40), data, red_entity1);
        SpawnCombatUnit(Team::kRed, HexGridPosition(-15, 40), data, red_entity2);
    }

    virtual int GetRadius() const
    {
        return 1;
    }

    void SetUp() override
    {
        Super::SetUp();

        InitCombatUnitData();
        SpawnCombatUnits();

        GetGridHelper().BuildObstacles(GetRadius());
    }

    CombatUnitData data;

    Entity* blue_entity1 = nullptr;
    Entity* blue_entity2 = nullptr;
    Entity* red_entity1 = nullptr;
    Entity* red_entity2 = nullptr;
};  // class GridTest

// Test with no limits
TEST_F(GridTest, GetOpenPositionNearLocationOnPath)
{
    // Get positions
    const auto& blue_position_component1 = blue_entity1->Get<PositionComponent>();
    const auto& blue_position_component2 = blue_entity1->Get<PositionComponent>();
    const auto& red_position_component1 = red_entity1->Get<PositionComponent>();
    const auto& red_position_component2 = red_entity2->Get<PositionComponent>();

    // Path from blue_entity1 to red_entity1
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component1.GetPosition(),
            red_position_component1.GetPosition(),
            blue_position_component1.GetRadius(),
            world->GetRangeInfinite().AsInt()),
        HexGridPosition(-25, 37));

    // Path from blue_entity1 to red_entity2
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component1.GetPosition(),
            red_position_component2.GetPosition(),
            blue_position_component1.GetRadius(),
            world->GetRangeInfinite().AsInt()),
        HexGridPosition(-15, 37));

    // Path from blue_entity2 to red_entity1
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component2.GetPosition(),
            red_position_component1.GetPosition(),
            blue_position_component2.GetRadius(),
            world->GetRangeInfinite().AsInt()),
        HexGridPosition(-25, 37));

    // Path from blue_entity2 to red_entity2
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component2.GetPosition(),
            red_position_component2.GetPosition(),
            blue_position_component2.GetRadius(),
            world->GetRangeInfinite().AsInt()),
        HexGridPosition(-15, 37));

    // Only around blue_entity1
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component1.GetPosition(),
            blue_position_component1.GetPosition(),
            blue_position_component1.GetRadius(),
            world->GetRangeInfinite().AsInt()),
        HexGridPosition(-25, 22));
}

// Test with limits within range
TEST_F(GridTest, GetOpenPositionNearLocationOnPathLimit)
{
    // Get positions
    const auto& blue_position_component1 = blue_entity1->Get<PositionComponent>();
    const auto& blue_position_component2 = blue_entity1->Get<PositionComponent>();
    const auto& red_position_component1 = red_entity1->Get<PositionComponent>();
    const auto& red_position_component2 = red_entity2->Get<PositionComponent>();

    // Path from blue_entity1 to red_entity1
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component1.GetPosition(),
            red_position_component1.GetPosition(),
            blue_position_component1.GetRadius(),
            4),
        HexGridPosition(-25, 37));

    // Path from blue_entity1 to red_entity2
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component1.GetPosition(),
            red_position_component2.GetPosition(),
            blue_position_component1.GetRadius(),
            4),
        HexGridPosition(-15, 37));

    // Path from blue_entity2 to red_entity1
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component2.GetPosition(),
            red_position_component1.GetPosition(),
            blue_position_component2.GetRadius(),
            4),
        HexGridPosition(-25, 37));

    // Path from blue_entity2 to red_entity2
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component2.GetPosition(),
            red_position_component2.GetPosition(),
            blue_position_component2.GetRadius(),
            4),
        HexGridPosition(-15, 37));

    // Only around blue_entity1
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component1.GetPosition(),
            blue_position_component1.GetPosition(),
            blue_position_component1.GetRadius(),
            4),
        HexGridPosition(-25, 22));
}

// Test with limits out of range
TEST_F(GridTest, GetOpenPositionNearLocationOnPathLimitExceeded)
{
    // Get positions
    const auto& blue_position_component1 = blue_entity1->Get<PositionComponent>();
    const auto& blue_position_component2 = blue_entity1->Get<PositionComponent>();
    const auto& red_position_component1 = red_entity1->Get<PositionComponent>();
    const auto& red_position_component2 = red_entity2->Get<PositionComponent>();

    // Path from blue_entity1 to red_entity1
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component1.GetPosition(),
            red_position_component1.GetPosition(),
            blue_position_component1.GetRadius(),
            3),
        HexGridPosition(kInvalidPosition, kInvalidPosition));

    // Path from blue_entity1 to red_entity2
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component1.GetPosition(),
            red_position_component2.GetPosition(),
            blue_position_component1.GetRadius(),
            3),
        HexGridPosition(kInvalidPosition, kInvalidPosition));

    // Path from blue_entity2 to red_entity1
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component2.GetPosition(),
            red_position_component1.GetPosition(),
            blue_position_component2.GetRadius(),
            3),
        HexGridPosition(kInvalidPosition, kInvalidPosition));

    // Path from blue_entity2 to red_entity2
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component2.GetPosition(),
            red_position_component2.GetPosition(),
            blue_position_component2.GetRadius(),
            3),
        HexGridPosition(kInvalidPosition, kInvalidPosition));

    // Only around blue_entity1
    EXPECT_EQ(
        GetGridHelper().GetOpenPositionNearLocationOnPath(
            blue_position_component1.GetPosition(),
            blue_position_component1.GetPosition(),
            blue_position_component1.GetRadius(),
            3),
        HexGridPosition(kInvalidPosition, kInvalidPosition));
}

// Test position in range
TEST_F(GridTest, GetOpenPositionsInRange)
{
    // Get positions
    const auto& blue_position_component1 = blue_entity1->Get<PositionComponent>();
    const auto& blue_position_component2 = blue_entity2->Get<PositionComponent>();
    const auto& red_position_component1 = red_entity1->Get<PositionComponent>();
    const auto& red_position_component2 = red_entity2->Get<PositionComponent>();

    // Path from blue_entity1 to red_entity1
    EXPECT_EQ(
        GetGridHelper()
            .GetOpenPositionsInRange(
                blue_position_component1.GetPosition(),
                red_position_component1.GetPosition(),
                blue_position_component1.GetRadius(),
                10,
                blue_position_component1.GetRadius() + red_position_component1.GetRadius(),
                1)
            .front(),
        HexGridPosition(-25, 30));

    // Path from blue_entity1 to red_entity2
    EXPECT_EQ(
        GetGridHelper()
            .GetOpenPositionsInRange(
                blue_position_component1.GetPosition(),
                red_position_component2.GetPosition(),
                blue_position_component1.GetRadius(),
                10,
                blue_position_component1.GetRadius() + red_position_component2.GetRadius(),
                1)
            .front(),
        HexGridPosition(-15, 30));

    // Path from blue_entity2 to red_entity1
    EXPECT_EQ(
        GetGridHelper()
            .GetOpenPositionsInRange(
                blue_position_component2.GetPosition(),
                red_position_component1.GetPosition(),
                blue_position_component2.GetRadius(),
                10,
                blue_position_component2.GetRadius() + red_position_component1.GetRadius(),
                1)
            .front(),
        HexGridPosition(-20, 30));

    // Path from blue_entity2 to red_entity2
    EXPECT_EQ(
        GetGridHelper()
            .GetOpenPositionsInRange(
                blue_position_component2.GetPosition(),
                red_position_component2.GetPosition(),
                blue_position_component2.GetRadius(),
                10,
                blue_position_component2.GetRadius() + red_position_component2.GetRadius(),
                1)
            .front(),
        HexGridPosition(-15, 30));

    // Only around blue_entity1
    EXPECT_EQ(
        GetGridHelper()
            .GetOpenPositionsInRange(
                blue_position_component1.GetPosition(),
                blue_position_component1.GetPosition(),
                blue_position_component1.GetRadius(),
                10,
                blue_position_component1.GetRadius(),
                1)
            .front(),
        HexGridPosition(-25, 15));
}

// Test position nearby
TEST_F(GridTest, GetOpenPositionNearby)
{
    // Get positions
    const auto& blue_position_component1 = blue_entity1->Get<PositionComponent>();
    const auto& blue_position_component2 = blue_entity2->Get<PositionComponent>();
    const auto& red_position_component1 = red_entity1->Get<PositionComponent>();
    const auto& red_position_component2 = red_entity2->Get<PositionComponent>();

    static constexpr size_t neighbours_offsets_size = kHexGridNeighboursOffsets.size();

    // Use positions for tests
    std::vector<HexGridPosition> test_positions;
    test_positions.push_back(blue_position_component1.GetPosition());
    test_positions.push_back(blue_position_component2.GetPosition());
    test_positions.push_back(red_position_component1.GetPosition());
    test_positions.push_back(red_position_component2.GetPosition());

    // Try at various distances from requested positions
    const int max_distance_blocked = GetRadius() * 2;
    const int first_open_distance = max_distance_blocked + 1;
    const int clear_space_distance = first_open_distance + 1;

    for (int test_distance = 0; test_distance <= clear_space_distance; test_distance++)
    {
        // Try all requested positions
        for (const auto& test_position : test_positions)
        {
            // Evaluate neighbours at distances
            HexGridPosition hex = test_position + kHexGridNeighboursOffsets[4] * test_distance;

            // Check all sides
            for (size_t i = 0; i < neighbours_offsets_size; i++)
            {
                // NOTE: j is not used on purpose, just used as an iterator
                for (int j = 0; j < test_distance; j++)
                {
                    const auto found_position = GetGridHelper().GetOpenPositionNearby(hex, 0);
                    if (test_distance == clear_space_distance)
                    {
                        // Tested space clear, so distance should be exactly clear_space_distance
                        EXPECT_EQ((found_position - test_position).Length(), clear_space_distance);
                    }
                    else
                    {
                        // Tested space not clear, should be first_open_distance from requested position regardless of
                        // actual position tested
                        EXPECT_EQ((found_position - test_position).Length(), first_open_distance);
                    }
                    hex = GridHelper::GetAxialNeighbour(hex, i);
                }
            }
        }
    }
}

// Test position nearby
TEST_F(GridTest, GetOpenPositionNearby2D)
{
    // Uncomment to visualize found locations
    // VISUALIZE_TEST_STEPS

    std::vector test_entities = {blue_entity1, blue_entity2, red_entity1, red_entity2};

    for (size_t index = 0; index < test_entities.size(); ++index)
    {
        Entity* entity = test_entities[index];
        entity->SetTeam(Team::kRed);

        const int new_radius = static_cast<int>(index) + 1;
        auto& position_component = entity->Get<PositionComponent>();
        position_component.SetRadius(new_radius);
    }

    std::array expected_near_locations = {
        HexGridPosition{-27, 23},
        HexGridPosition{-18, 22},
        HexGridPosition{-29, 36},
        HexGridPosition{-5, 35}};

    std::vector<Entity*> spawned_entities;
    for (size_t index = 0; index < test_entities.size(); ++index)
    {
        Entity* entity = test_entities[index];
        const auto& position_component = entity->Get<PositionComponent>();
        const HexGridPosition position = position_component.GetPosition();

        // Try to spawn entity with radius +1 in same location
        const int test_radius = position_component.GetRadius() + 1;
        GetGridHelper().BuildObstacles(test_radius);
        const HexGridPosition found_position = GetGridHelper().GetOpenPositionNearby2D(position, test_radius);

        // Make sure we can spawn unit in found location
        Entity* test_entity = nullptr;
        CombatUnitData test_data = data;
        test_data.radius_units = test_radius;
        SpawnCombatUnit(Team::kBlue, found_position, test_data, test_entity);
        ASSERT_NE(test_entity, nullptr);

        // Verify locations
        EXPECT_EQ(expected_near_locations[index], found_position);
    }
}

// Test build obstacles for specific team
TEST_F(GridTest, BuildObstaclesForTeam)
{
    constexpr int radius_needed = 1;
    constexpr int invalid_radius_needed = 2;

    GridHelper::BuildObstaclesParameters build_parameters;
    build_parameters.mark_borders_as_obstacles = true;

    // Top half board corners with unit radius 1
    constexpr std::array red_team_locations = {
        HexGridPosition{-73, -2},
        HexGridPosition{0, -2},
        HexGridPosition{75, -2},
        HexGridPosition{111, -74},
        HexGridPosition{0, -74},
        HexGridPosition{-37, -74},
    };

    // Bottom half board corners with unit radius 1
    constexpr std::array blue_team_locations = {
        HexGridPosition{-75, 2},
        HexGridPosition{0, 2},
        HexGridPosition{73, 2},
        HexGridPosition{-111, 74},
        HexGridPosition{0, 74},
        HexGridPosition{37, 74},
    };

    // Red team, valid radius
    build_parameters.source = red_entity1;
    build_parameters.radius_needed = radius_needed;
    build_parameters.mark_team_side_as_obstacles = Team::kRed;
    GetGridHelper().BuildObstacles(build_parameters);

    for (size_t index = 0; index < red_team_locations.size(); index++)
    {
        const HexGridPosition red_test_location = red_team_locations[index];
        const HexGridPosition open_location_red =
            GetGridHelper().GetOpenPositionNearby(red_test_location, radius_needed);
        EXPECT_EQ(open_location_red, red_test_location) << "Red team obstacles check fails for valid location";

        const HexGridPosition blue_test_location = blue_team_locations[index];
        const HexGridPosition open_location_blue =
            GetGridHelper().GetOpenPositionNearby(blue_test_location, radius_needed);
        EXPECT_NE(open_location_blue, blue_test_location) << "Red team obstacles check fails for not valid position";
    }

    // Red team, extra radius
    build_parameters.radius_needed = invalid_radius_needed;
    GetGridHelper().BuildObstacles(build_parameters);

    for (const HexGridPosition& red_test_location : red_team_locations)
    {
        const HexGridPosition open_location_red =
            GetGridHelper().GetOpenPositionNearby(red_test_location, invalid_radius_needed);
        EXPECT_NE(open_location_red, red_test_location) << "Red team obstacles check fails for invalid radius";
    }

    // Blue team, valid radius
    build_parameters.source = blue_entity1;
    build_parameters.radius_needed = radius_needed;
    build_parameters.mark_team_side_as_obstacles = Team::kBlue;
    GetGridHelper().BuildObstacles(build_parameters);

    for (size_t index = 0; index < red_team_locations.size(); index++)
    {
        const HexGridPosition blue_test_location = blue_team_locations[index];
        const HexGridPosition open_location_blue =
            GetGridHelper().GetOpenPositionNearby2D(blue_test_location, radius_needed);
        EXPECT_EQ(open_location_blue, blue_test_location) << "Blue team obstacles check fails for valid location";

        const HexGridPosition red_test_location = red_team_locations[index];
        const HexGridPosition open_location_red =
            GetGridHelper().GetOpenPositionNearby2D(red_test_location, radius_needed);
        EXPECT_NE(open_location_red, red_test_location) << "Blue team obstacles check fails for not valid position";
    }

    // Blue team, extra radius
    build_parameters.radius_needed = invalid_radius_needed;
    GetGridHelper().BuildObstacles(build_parameters);

    for (const HexGridPosition& blue_test_location : blue_team_locations)
    {
        const HexGridPosition open_location_blue =
            GetGridHelper().GetOpenPositionNearby(blue_test_location, invalid_radius_needed);
        EXPECT_NE(open_location_blue, blue_test_location) << "Red team obstacles check fails for invalid radius";
    }
}

// Test build obstacles for specific team
TEST_F(GridTest, BuildObstaclesWithMargins)
{
    GridHelper::BuildObstaclesParameters build_parameters;
    build_parameters.radius_needed = 1;
    build_parameters.mark_borders_as_obstacles = true;

    // Top half board corners with unit radius 1
    constexpr std::array test_locations = {
        HexGridPosition{-73, -2},
        HexGridPosition{75, -2},
        HexGridPosition{111, -74},
        HexGridPosition{0, -74},
        HexGridPosition{-37, -74},
        HexGridPosition{-75, 2},
        HexGridPosition{73, 2},
        HexGridPosition{-111, 74},
        HexGridPosition{0, 74},
        HexGridPosition{37, 74},
    };

    GetGridHelper().BuildObstacles(build_parameters);

    for (const HexGridPosition& test_location : test_locations)
    {
        const HexGridPosition open_location = GetGridHelper().GetOpenPositionNearby(test_location, 0);
        EXPECT_EQ(open_location, test_location) << "Red team obstacles check fails for invalid radius";
    }

    build_parameters.q_margin = 1;
    build_parameters.r_margin = 1;
    GetGridHelper().BuildObstacles(build_parameters);

    for (const HexGridPosition& test_location : test_locations)
    {
        const HexGridPosition open_location = GetGridHelper().GetOpenPositionNearby(test_location, 0);
        EXPECT_NE(open_location, test_location) << "Red team obstacles check fails for invalid radius";
    }
}

// Test position behind
TEST_F(GridTest, GetOpenPositionBehind)
{
    // Get positions
    const auto& blue_position_component1 = blue_entity1->Get<PositionComponent>();
    const auto& blue_position_component2 = blue_entity2->Get<PositionComponent>();
    const auto& red_position_component1 = red_entity1->Get<PositionComponent>();
    const auto& red_position_component2 = red_entity2->Get<PositionComponent>();

    static constexpr size_t neighbours_offsets_size = kHexGridNeighboursOffsets.size();

    // Use positions for tests
    std::vector<HexGridPosition> test_positions;
    test_positions.push_back(blue_position_component1.GetPosition());
    test_positions.push_back(blue_position_component2.GetPosition());
    test_positions.push_back(red_position_component1.GetPosition());
    test_positions.push_back(red_position_component2.GetPosition());

    // Try at various distances from requested positions
    const int max_distance_blocked = GetRadius() * 2;
    const int first_open_distance = max_distance_blocked + 1;
    const int clear_space_distance = first_open_distance + 1;

    // Arbitrary point used for source
    const HexGridPosition reference_hex = HexGridPosition{33, -22};

    for (int test_distance = 0; test_distance <= clear_space_distance; test_distance++)
    {
        // Try all requested positions
        for (const auto& test_position : test_positions)
        {
            // Evaluate neighbours at distances
            HexGridPosition hex = test_position + kHexGridNeighboursOffsets[4] * test_distance;

            // Check all sides
            for (size_t i = 0; i < neighbours_offsets_size; i++)
            {
                // NOTE: j is not used on purpose, just used as an iterator
                for (int j = 0; j < test_distance; j++)
                {
                    // Relation between hex and reference
                    const HexGridPosition relative_hex = hex - reference_hex;

                    const auto found_position = GetGridHelper().GetOpenPositionBehind(reference_hex, hex, 0);
                    if (test_distance == clear_space_distance)
                    {
                        // Tested space clear, so distance should be exactly clear_space_distance
                        EXPECT_EQ((found_position - test_position).Length(), clear_space_distance);
                    }
                    else
                    {
                        // Tested space not clear, should be first_open_distance from requested position regardless of
                        // actual position tested
                        EXPECT_EQ((found_position - test_position).Length(), first_open_distance);
                    }

                    // Ensure it's behind
                    // Q should be negative, R should be positive (opposite of reference)
                    EXPECT_LT((found_position + relative_hex).q, 0);
                    EXPECT_GT((found_position + relative_hex).r, 0);

                    hex = GridHelper::GetAxialNeighbour(hex, i);
                }
            }
        }
    }
}

// Test position across board
TEST_F(GridTest, GetOpenPositionAcross)
{
    const HexGridConfig& grid_config = GetGridConfig();
    const GridHelper& grid_helper = GetGridHelper();

    // Get positions
    const auto& blue_position_component1 = blue_entity1->Get<PositionComponent>();
    const auto& blue_position_component2 = blue_entity2->Get<PositionComponent>();
    const auto& red_position_component1 = red_entity1->Get<PositionComponent>();
    const auto& red_position_component2 = red_entity2->Get<PositionComponent>();

    // Across from blue_entity1
    EXPECT_EQ(grid_helper.GetOpenPositionAcross(blue_position_component1.GetPosition(), 1), HexGridPosition(24, -74));

    // Across from blue_entity2
    EXPECT_EQ(grid_helper.GetOpenPositionAcross(blue_position_component2.GetPosition(), 1), HexGridPosition(34, -74));

    // Across from blue_entity1
    EXPECT_EQ(grid_helper.GetOpenPositionAcross(red_position_component1.GetPosition(), 1), HexGridPosition(32, -74));

    // Across from red_entity2
    EXPECT_EQ(grid_helper.GetOpenPositionAcross(red_position_component2.GetPosition(), 1), HexGridPosition(42, -74));

    // Test some more positions on red side
    EXPECT_EQ(grid_helper.GetOpenPositionAcross(HexGridPosition(-25, -25), 1), HexGridPosition(-75, 74));
    EXPECT_EQ(grid_helper.GetOpenPositionAcross(HexGridPosition(-25, -40), 1), HexGridPosition(-82, 74));
    EXPECT_EQ(grid_helper.GetOpenPositionAcross(HexGridPosition(-15, -25), 1), HexGridPosition(-65, 74));
    EXPECT_EQ(grid_helper.GetOpenPositionAcross(HexGridPosition(-15, -40), 1), HexGridPosition(-72, 74));

    // Positions just inside corners of grid
    const IVector2D top_left_vector(
        -grid_config.GetRectangleWidthExtent() + 1,
        -grid_config.GetRectangleHeightExtent() + 1);
    const IVector2D top_right_vector(
        grid_config.GetRectangleWidthExtent() - 1,
        -grid_config.GetRectangleHeightExtent() + 1);
    const IVector2D bottom_left_vector(
        -grid_config.GetRectangleWidthExtent() + 1,
        grid_config.GetRectangleHeightExtent() - 1);
    const IVector2D bottom_right_vector(
        grid_config.GetRectangleWidthExtent() - 1,
        grid_config.GetRectangleHeightExtent() - 1);

    // Convert to axial
    const HexGridPosition top_left = HexGridPosition::FromOffsetOddR(top_left_vector);
    const HexGridPosition top_right = HexGridPosition::FromOffsetOddR(top_right_vector);
    const HexGridPosition bottom_left = HexGridPosition::FromOffsetOddR(bottom_left_vector);
    const HexGridPosition bottom_right = HexGridPosition::FromOffsetOddR(bottom_right_vector);

    // Test radius for corners
    static constexpr int test_radius = 5;

    // Expected results for corners
    const HexGridPosition expected_top_left(-105, 70);
    const HexGridPosition expected_top_right(36, 69);
    const HexGridPosition expected_bottom_left(-35, -70);
    const HexGridPosition expected_bottom_right(105, -70);

    // Ensure integrity of test expectations by making sure values above are as intended
    ASSERT_FALSE(grid_config.IsHexagonInGridLimits(top_left, test_radius));
    ASSERT_FALSE(grid_config.IsHexagonInGridLimits(top_right, test_radius));
    ASSERT_FALSE(grid_config.IsHexagonInGridLimits(bottom_left, test_radius));
    ASSERT_FALSE(grid_config.IsHexagonInGridLimits(bottom_right, test_radius));
    ASSERT_TRUE(grid_config.IsHexagonInGridLimits(expected_top_left, test_radius));
    ASSERT_TRUE(grid_config.IsHexagonInGridLimits(expected_top_right, test_radius));
    ASSERT_TRUE(grid_config.IsHexagonInGridLimits(expected_bottom_left, test_radius));
    ASSERT_TRUE(grid_config.IsHexagonInGridLimits(expected_bottom_right, test_radius));

    // Test on corners of map with bigger radius
    EXPECT_EQ(grid_helper.GetOpenPositionAcross(top_left, test_radius), expected_top_left);
    EXPECT_EQ(grid_helper.GetOpenPositionAcross(top_right, test_radius), expected_top_right);
    EXPECT_EQ(grid_helper.GetOpenPositionAcross(bottom_left, test_radius), expected_bottom_left);
    EXPECT_EQ(grid_helper.GetOpenPositionAcross(bottom_right, test_radius), expected_bottom_right);
}

class GridHelperTest : public BaseTest
{
};

TEST(GridHelper, ApplyExcessiveSubUnitsToUnits)
{
    HexGridPosition test_unit_position, test_sub_unit_position;

    // No subunits
    test_unit_position = HexGridPosition{250, 250};
    test_sub_unit_position = HexGridPosition{0, 0};
    GridHelper::ApplyExcessiveSubUnitsToUnits(&test_unit_position, &test_sub_unit_position);
    EXPECT_EQ(test_unit_position, HexGridPosition(250, 250));
    EXPECT_EQ(test_sub_unit_position, HexGridPosition(0, 0));

    // Basic addition
    test_unit_position = HexGridPosition{200, 200};
    test_sub_unit_position = HexGridPosition{200, 200};
    GridHelper::ApplyExcessiveSubUnitsToUnits(&test_unit_position, &test_sub_unit_position);
    EXPECT_EQ(test_unit_position, HexGridPosition(200, 200));
    EXPECT_EQ(test_sub_unit_position, HexGridPosition(200, 200));

    // Basic subtraction
    test_unit_position = HexGridPosition{400, 400};
    test_sub_unit_position = HexGridPosition{-200, -200};
    GridHelper::ApplyExcessiveSubUnitsToUnits(&test_unit_position, &test_sub_unit_position);
    EXPECT_EQ(test_unit_position, HexGridPosition(400, 400));
    EXPECT_EQ(test_sub_unit_position, HexGridPosition(-200, -200));

    // Overflow positive
    test_unit_position = HexGridPosition{100, 100};
    test_sub_unit_position = HexGridPosition{20000, 20000};
    GridHelper::ApplyExcessiveSubUnitsToUnits(&test_unit_position, &test_sub_unit_position);
    EXPECT_EQ(test_unit_position, HexGridPosition(120, 120));
    EXPECT_EQ(test_sub_unit_position, HexGridPosition(0, 0));

    // Overflow negative
    test_unit_position = HexGridPosition{400, 400};
    test_sub_unit_position = HexGridPosition{-20000, -20000};
    GridHelper::ApplyExcessiveSubUnitsToUnits(&test_unit_position, &test_sub_unit_position);
    EXPECT_EQ(test_unit_position, HexGridPosition(380, 380));
    EXPECT_EQ(test_sub_unit_position, HexGridPosition(0, 0));

    // Overflow positive with remainder
    test_unit_position = HexGridPosition{100, 100};
    test_sub_unit_position = HexGridPosition{20010, 20010};
    GridHelper::ApplyExcessiveSubUnitsToUnits(&test_unit_position, &test_sub_unit_position);
    EXPECT_EQ(test_unit_position, HexGridPosition(120, 120));
    EXPECT_EQ(test_sub_unit_position, HexGridPosition(10, 10));

    // Overflow negative with remainder
    test_unit_position = HexGridPosition{400, 400};
    test_sub_unit_position = HexGridPosition{-20010, -20010};
    GridHelper::ApplyExcessiveSubUnitsToUnits(&test_unit_position, &test_sub_unit_position);
    EXPECT_EQ(test_unit_position, HexGridPosition(380, 380));
    EXPECT_EQ(test_sub_unit_position, HexGridPosition(-10, -10));
}

TEST(GridHelper, DirectionToNeighbourPositionOffset)
{
    // https://www.redblobgames.com/grids/hexagons/#neighbors-axial
    ASSERT_EQ(static_cast<size_t>(HexGridCardinalDirection::kNum) - 1, kHexGridNeighboursOffsets.size());
    EXPECT_EQ(
        GridHelper::DirectionToNeighbourPositionOffset(HexGridCardinalDirection::kNone),
        kInvalidHexHexGridPosition);
    EXPECT_EQ(
        GridHelper::DirectionToNeighbourPositionOffset(HexGridCardinalDirection::kNum),
        kInvalidHexHexGridPosition);
    EXPECT_EQ(
        GridHelper::DirectionToNeighbourPositionOffset(HexGridCardinalDirection::kRight),
        kHexGridNeighboursOffsets[0]);
    EXPECT_EQ(GridHelper::DirectionToNeighbourPositionOffset(HexGridCardinalDirection::kRight), HexGridPosition(1, 0));
    EXPECT_EQ(
        GridHelper::DirectionToNeighbourPositionOffset(HexGridCardinalDirection::kTopRight),
        kHexGridNeighboursOffsets[1]);
    EXPECT_EQ(
        GridHelper::DirectionToNeighbourPositionOffset(HexGridCardinalDirection::kTopRight),
        HexGridPosition(1, -1));
    EXPECT_EQ(
        GridHelper::DirectionToNeighbourPositionOffset(HexGridCardinalDirection::kTopLeft),
        kHexGridNeighboursOffsets[2]);
    EXPECT_EQ(
        GridHelper::DirectionToNeighbourPositionOffset(HexGridCardinalDirection::kTopLeft),
        HexGridPosition(0, -1));
    EXPECT_EQ(
        GridHelper::DirectionToNeighbourPositionOffset(HexGridCardinalDirection::kLeft),
        kHexGridNeighboursOffsets[3]);
    EXPECT_EQ(GridHelper::DirectionToNeighbourPositionOffset(HexGridCardinalDirection::kLeft), HexGridPosition(-1, 0));
    EXPECT_EQ(
        GridHelper::DirectionToNeighbourPositionOffset(HexGridCardinalDirection::kBottomLeft),
        kHexGridNeighboursOffsets[4]);
    EXPECT_EQ(
        GridHelper::DirectionToNeighbourPositionOffset(HexGridCardinalDirection::kBottomLeft),
        HexGridPosition(-1, +1));
    EXPECT_EQ(
        GridHelper::DirectionToNeighbourPositionOffset(HexGridCardinalDirection::kBottomRight),
        kHexGridNeighboursOffsets[5]);
    EXPECT_EQ(
        GridHelper::DirectionToNeighbourPositionOffset(HexGridCardinalDirection::kBottomRight),
        HexGridPosition(0, +1));
}

TEST_F(GridHelperTest, GetCoordinates)
{
    const HexGridConfig& grid_config = GetGridConfig();
    EXPECT_EQ(grid_config.GetCoordinates(0), HexGridPosition(-37, -75));
    EXPECT_EQ(grid_config.GetCoordinates(0), grid_config.GetMinHexGridPosition());
    EXPECT_EQ(grid_config.GetCoordinates(grid_config.GetGridSize() - 1), HexGridPosition(38, 75));
    EXPECT_EQ(grid_config.GetCoordinates(grid_config.GetGridSize() - 1), grid_config.GetMaxHexGridPosition());
}

TEST_F(GridHelperTest, GetGridIndex)
{
    const HexGridConfig& grid_config = GetGridConfig();
    EXPECT_EQ(grid_config.GetGridIndex(HexGridPosition{-1, 0}), 11399);
    EXPECT_EQ(grid_config.GetGridIndex(HexGridPosition{0, 0}), 11400);
    EXPECT_EQ(grid_config.GetGridIndex(HexGridPosition{1, 0}), 11401);
    EXPECT_EQ(grid_config.GetGridIndex(HexGridPosition{-1, 1}), 11550);
    EXPECT_EQ(grid_config.GetGridIndex(HexGridPosition{0, 1}), 11551);
    EXPECT_EQ(grid_config.GetGridIndex(HexGridPosition{1, 1}), 11552);
    EXPECT_EQ(grid_config.GetGridIndex(HexGridPosition{3, 2}), 11706);
    EXPECT_EQ(grid_config.GetGridIndex(grid_config.GetMinHexGridPosition()), 0);
    EXPECT_EQ(grid_config.GetGridIndex(grid_config.GetMaxHexGridPosition()), 22800);
    EXPECT_EQ(grid_config.GetGridIndex(grid_config.GetMaxHexGridPosition()), grid_config.GetGridSize() - 1);
}

TEST(GridHelper, DoHexagonsIntersect)
{
    EXPECT_TRUE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 0, HexGridPosition(0, 0), 0));
    EXPECT_FALSE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 0, HexGridPosition(0, 1), 0));
    EXPECT_FALSE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 0, HexGridPosition(1, 0), 0));

    EXPECT_TRUE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(0, 1), 0));
    EXPECT_TRUE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(0, 2), 0));
    EXPECT_TRUE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(0, 1), 1));
    EXPECT_TRUE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(0, 1), 1));

    // Diagonally up
    EXPECT_FALSE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(3, -3), 0));
    EXPECT_TRUE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(3, -3), 1));
    EXPECT_FALSE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(4, -4), 1));
    EXPECT_TRUE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(4, -4), 2));
    EXPECT_FALSE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(5, -5), 2));
    EXPECT_TRUE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(5, -5), 3));
    EXPECT_FALSE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(6, -6), 3));
    EXPECT_TRUE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 3, HexGridPosition(6, -6), 3));

    EXPECT_FALSE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(0, 3), 0));
    EXPECT_FALSE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(1, 2), 0));
    EXPECT_FALSE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(2, 1), 0));
    EXPECT_FALSE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(-3, 2), 0));
    EXPECT_FALSE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(3, -2), 0));
    EXPECT_FALSE(GridHelper::DoHexagonsIntersect(HexGridPosition(0, 0), 2, HexGridPosition(-1, -2), 0));
}

TEST(GridHelper, DoesHexagonOverlapEnemyAllyLine)
{
    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 0), 1));
    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 0), 0));

    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 1), 0));
    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -1), 0));
    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 1), 1));
    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -1), 1));

    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 2), 1));
    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -2), 1));
    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 2), 2));
    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -2), 2));

    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 3), 1));
    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -3), 1));
    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 3), 2));
    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -3), 2));
    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 3), 3));
    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -3), 3));

    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -3), 3));

    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -50), 20));
    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 50), 20));
    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -50), 49));
    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 50), 49));
    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -50), 50));
    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 50), 50));

    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 0), 1, 10));
    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 0), -1, 10));

    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 30), 10, 5));
    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -30), 20, 5));
    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -30), 20, 5));
    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -30), 24, 5));
    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -30), 25, 5));

    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -50), 44, 5));
    EXPECT_FALSE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 50), 44, 5));
    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, -50), 45, 5));
    EXPECT_TRUE(GridHelper::DoesHexagonOverlapEnemyAllyLine(HexGridPosition(0, 50), 45, 5));
}

TEST_F(GridTest, GetFarthestOpenPositionFromPointInRange)
{
    // Get positions
    const auto& blue_position_component1 = blue_entity1->Get<PositionComponent>();
    const auto& red_position_component1 = red_entity1->Get<PositionComponent>();
    const auto& red_position_component2 = red_entity2->Get<PositionComponent>();

    // Build obstacle map for pathfinding and collision detection
    GetGridHelper().BuildObstacles(red_entity1);

    EXPECT_EQ(
        GetGridHelper().GetFarthestOpenPositionFromPointInRange(
            red_position_component1.GetPosition(),
            blue_position_component1.GetPosition(),
            1),
        red_position_component1.GetPosition() + HexGridPosition(0, 1));

    EXPECT_EQ(
        GetGridHelper().GetFarthestOpenPositionFromPointInRange(
            red_position_component1.GetPosition(),
            blue_position_component1.GetPosition(),
            3),
        red_position_component1.GetPosition() + HexGridPosition(0, 3));

    EXPECT_EQ(
        GetGridHelper().GetFarthestOpenPositionFromPointInRange(
            red_position_component1.GetPosition(),
            red_position_component2.GetPosition(),
            1),
        red_position_component1.GetPosition() + HexGridPosition(-1, 1));

    EXPECT_EQ(
        GetGridHelper().GetFarthestOpenPositionFromPointInRange(
            red_position_component1.GetPosition(),
            red_position_component2.GetPosition(),
            3),
        red_position_component1.GetPosition() + HexGridPosition(-3, 3));
}

TEST_F(GridTest, Heuristics)
{
    auto& gc = GetGridConfig();

    HexGridPosition source = gc.GetMinHexGridPosition();
    HexGridPosition target = gc.GetMaxHexGridPosition();

    HexGridPosition position_a{-36, -75};
    HexGridPosition position_b{-37, -74};

    auto dist_a = GridHelper::CalculateAStarHeuristicAngle(source, position_a, target);
    auto dist_b = GridHelper::CalculateAStarHeuristicAngle(source, position_b, target);
    EXPECT_GT(dist_a, dist_b);

    // Overflow test
    for (size_t i = 0; i != gc.GetGridSize(); ++i)
    {
        const auto position = gc.GetCoordinates(i);
        if (position != source && position != target)
        {
            // Squared distance cannot be negative
            ASSERT_GE(GridHelper::CalculateAStarHeuristicAngle(source, position, target), 0);
        }
    }
}

// Test build obstacles
TEST_F(GridTest, BuildObstaclesWithInvalidEntity)
{
    constexpr int radius_needed = 1;

    // Simulate invalidation of one entity using some hacks,
    // wasn't able to reproduce it using RemoveCombatUnitBeforeBattleStarted or something else
    const std::vector<std::shared_ptr<Entity>>& entities = world->GetAll();
    for (const std::shared_ptr<Entity>& entity : entities)
    {
        if (entity->GetID() == red_entity1->GetID())
        {
            std::shared_ptr<Entity>& entity_delete = const_cast<std::shared_ptr<Entity>&>(entity);
            entity_delete.reset();
        }
    }

    GridHelper::BuildObstaclesParameters build_parameters;
    build_parameters.mark_borders_as_obstacles = true;
    build_parameters.source = red_entity2;
    build_parameters.radius_needed = radius_needed;
    build_parameters.mark_team_side_as_obstacles = Team::kRed;
    GetGridHelper().BuildObstacles(build_parameters);

    build_parameters.source = blue_entity1;
    build_parameters.mark_team_side_as_obstacles = Team::kBlue;
    GetGridHelper().BuildObstacles(build_parameters);
}
}  // namespace simulation
