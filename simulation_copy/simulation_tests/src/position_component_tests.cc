#include "base_test_fixtures.h"
#include "components/position_component.h"
#include "gtest/gtest.h"
#include "utility/grid_helper.h"

namespace simulation
{
class PositionComponentTest : public BaseTest
{
public:
    PositionComponent CreateComponent(const HexGridPosition& position, const int size)
    {
        auto component = PositionComponent{};
        component.Init();
        component.SetPosition(position);
        component.SetRadius(size);

        return component;
    }

    void TestHasOnlyHexagonPositionsTaken(const PositionComponent& position)
    {
        const int radius = position.GetRadius();
        const HexGridPosition& center = position.GetPosition();

        int num_loop = 0;

        // Check it is an hexagon
        const int q_min = HexGridPosition::HexagonQLimitMin(radius);
        const int q_max = HexGridPosition::HexagonQLimitMax(radius);
        for (int q = q_min; q <= q_max; q++)
        {
            const int r_min = HexGridPosition::HexagonRLimitMin(radius, q);
            const int r_max = HexGridPosition::HexagonRLimitMax(radius, q);
            for (int r = r_min; r <= r_max; r++)
            {
                num_loop++;

                // Translate by current position
                const HexGridPosition check_position = HexGridPosition{q, r} + center;
                EXPECT_TRUE(position.IsPositionTaken(check_position));
            }
        }

        // Check rectangle
        int rectangle_taken_num = 0;
        for (int q = position.GetQ() - radius; q <= position.GetQ() + radius; q++)
        {
            for (int r = position.GetR() - radius; r <= position.GetR() + radius; r++)
            {
                const HexGridPosition check_position{q, r};
                const bool is_taken = position.IsPositionTaken(check_position);
                if (is_taken)
                {
                    rectangle_taken_num++;
                }
                // EXPECT_FALSE(is_taken) << "Position = " << CheckPosition;
            }
        }

        // Limits should be taken
        const int r_min = HexGridPosition::HexagonRLimitMin(radius, q_min);
        const int r_max = HexGridPosition::HexagonRLimitMax(radius, q_max);
        EXPECT_TRUE(position.IsPositionTaken(HexGridPosition{q_min, r_min} + position.GetPosition()));
        EXPECT_TRUE(position.IsPositionTaken(HexGridPosition{q_max, r_max} + position.GetPosition()));

        // for (int i = -1; i <= 1; i++)
        // {
        //     if (i == 0) continue;
        //     for (int j = -1; j <= 1; j++)
        //     {
        //         if (j == 0) continue;

        //         const HexGridPosition first =  HexGridPosition{q_min - i, r_min - j} +
        //         position.GetPosition(); const HexGridPosition second =  HexGridPosition{q_max +
        //         i, r_max + j} + position.GetPosition();

        //         EXPECT_FALSE(position.IsPositionTaken(first)) << "first = " << first;
        //         EXPECT_FALSE(position.IsPositionTaken(second)) << "second = " << second;
        //     }
        // }

        // Check loop
        EXPECT_EQ(num_loop, GetGridHelper().GetSpiralRingsPositions(center, radius).size());
        EXPECT_EQ(num_loop, rectangle_taken_num) << "This means that it is not a hexagon!";
    }

    void TestIsPositionsIntersectingSpiralRingPositions(
        const PositionComponent& position,
        const bool expect_taken,
        const int check_radius,
        const int other_size)
    {
        const HexGridPosition& center = position.GetPosition();

        // Check all positions around the check_radius
        for (const HexGridPosition& check_position : GetGridHelper().GetSpiralRingsPositions(center, check_radius))
        {
            const bool is_taken = position.IsPositionsIntersecting(check_position, other_size);
            EXPECT_EQ(is_taken, expect_taken)

                << "position.center = " << fmt::format("{}", center) << ", position.size = " << position.GetRadius()
                << ", check_position = " << fmt::format("{}", check_position) << ", check_radius = " << check_radius
                << ", other_size = " << other_size;
        }
    }

    void TestIsPositionsIntersectingSingleRing(
        const PositionComponent& position,
        const bool expect_taken,
        const int check_radius,
        const int other_size)
    {
        const HexGridPosition& center = position.GetPosition();

        // Check only a single ring
        for (const HexGridPosition& check_position : GetGridHelper().GetSingleRingPositions(center, check_radius))
        {
            const bool is_taken = position.IsPositionsIntersecting(check_position, other_size);
            EXPECT_EQ(is_taken, expect_taken)
                << "position.center = " << fmt::format("{}", center) << ", position.size = " << position.GetRadius()
                << ", check_position = " << fmt::format("{}", check_position) << ", check_radius = " << check_radius
                << ", other_size = " << other_size;
        }
    }

    void TestIsPositionsIntersectingRadius1(const PositionComponent& position)
    {
        ASSERT_EQ(position.GetRadius(), 1);

        // Check spiral rings
        TestIsPositionsIntersectingSpiralRingPositions(position, true, 1, 0);
        TestIsPositionsIntersectingSpiralRingPositions(position, true, 2, 1);
        TestIsPositionsIntersectingSpiralRingPositions(position, true, 2, 2);
        TestIsPositionsIntersectingSpiralRingPositions(position, true, 2, 3);
        TestIsPositionsIntersectingSpiralRingPositions(position, true, 3, 3);
        TestIsPositionsIntersectingSingleRing(position, true, 1, 1);

        // Check outside the ring, ring 3 from the position there should be near the edge of
        // position but not touching
        TestIsPositionsIntersectingSingleRing(position, false, 3, 1);

        // Size 2 on ring 3
        TestIsPositionsIntersectingSingleRing(position, true, 3, 2);

        // Size 2 on ring 4 should fit
        TestIsPositionsIntersectingSingleRing(position, false, 4, 2);
    }

    void TestIsPositionsIntersectingRadius2(const PositionComponent& position)
    {
        ASSERT_EQ(position.GetRadius(), 2);

        // Check spiral rings
        TestIsPositionsIntersectingSpiralRingPositions(position, true, 2, 0);
        TestIsPositionsIntersectingSpiralRingPositions(position, true, 2, 1);
        TestIsPositionsIntersectingSpiralRingPositions(position, true, 3, 1);
        TestIsPositionsIntersectingSpiralRingPositions(position, true, 4, 2);

        // Outside, ring 4
        TestIsPositionsIntersectingSingleRing(position, false, 4, 1);

        // Ring 4 but size is too large
        TestIsPositionsIntersectingSingleRing(position, true, 4, 2);

        // Size 2 but ring 5,can fit
        TestIsPositionsIntersectingSingleRing(position, false, 5, 2);

        // Size 3 but ring 5, can't fit
        TestIsPositionsIntersectingSingleRing(position, true, 5, 3);

        // Outside of the radius
        // Ring 3 size 0
        TestIsPositionsIntersectingSingleRing(position, false, 3, 0);

        // Size but ring 6, can fit
        TestIsPositionsIntersectingSingleRing(position, false, 6, 3);
    }

    void TestIsPositionsIntersectingRadius3(const PositionComponent& position)
    {
        ASSERT_EQ(position.GetRadius(), 3);

        // Check spiral rings
        TestIsPositionsIntersectingSpiralRingPositions(position, true, 3, 0);
        TestIsPositionsIntersectingSpiralRingPositions(position, true, 3, 1);
        TestIsPositionsIntersectingSpiralRingPositions(position, true, 4, 1);
        TestIsPositionsIntersectingSpiralRingPositions(position, true, 5, 2);

        // Outside, ring 4
        TestIsPositionsIntersectingSingleRing(position, false, 4, 0);

        // Ring 4 but size is too large
        TestIsPositionsIntersectingSingleRing(position, true, 4, 1);

        // Ring 5, size 1, can fit
        TestIsPositionsIntersectingSingleRing(position, false, 5, 1);

        // Ring 5, size 2, can't fit
        TestIsPositionsIntersectingSingleRing(position, true, 5, 2);

        // Ring 6, size 2, can fit
        TestIsPositionsIntersectingSingleRing(position, false, 6, 2);

        // Ring 6, size 3, can't fit
        TestIsPositionsIntersectingSingleRing(position, true, 6, 3);

        // Ring 7, size 3, can fit
        TestIsPositionsIntersectingSingleRing(position, false, 7, 3);
    }
};

TEST(PositionComponent, GetRadius)
{
    // kNone
    {
        auto position = PositionComponent{};
        position.Init();
        position.SetRadius(0);
        EXPECT_EQ(0, position.GetRadius());
    }
    // kSmall
    {
        auto position = PositionComponent{};
        position.Init();
        position.SetRadius(1);
        EXPECT_EQ(1, position.GetRadius());
    }
    // kMedium
    {
        auto position = PositionComponent{};
        position.Init();
        position.SetRadius(2);
        EXPECT_EQ(2, position.GetRadius());
    }
    // kLarge
    {
        auto position = PositionComponent{};
        position.Init();
        position.SetRadius(3);
        EXPECT_EQ(3, position.GetRadius());
    }
}

TEST_F(PositionComponentTest, IsPositionTaken)
{
    const HexGridConfig& grid_config = GetGridConfig();

    // kNone
    {
        const auto position = CreateComponent({10, 10}, 0);

        EXPECT_FALSE(position.IsPositionTaken(9, 9));
        EXPECT_FALSE(position.IsPositionTaken(10, 9));
        EXPECT_FALSE(position.IsPositionTaken(11, 10));
        EXPECT_FALSE(position.IsPositionTaken(11, 11));
        EXPECT_TRUE(position.IsPositionTaken(10, 10));
    }
    // kNone, lower limit
    {
        const auto position = CreateComponent({0, 0}, 0);

        EXPECT_FALSE(position.IsPositionTaken(-1, -1));
        EXPECT_FALSE(position.IsPositionTaken(1, 1));
        EXPECT_TRUE(position.IsPositionTaken(0, 0));
    }
    // kNone, higher limit
    {
        const auto position = CreateComponent(
            {grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent()),
             grid_config.GetRectangleHeightExtent()},
            0);

        EXPECT_FALSE(position.IsPositionTaken(1, 1));
        EXPECT_FALSE(position.IsPositionTaken(0, 0));
        EXPECT_FALSE(position.IsPositionTaken(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent() - 1),
            grid_config.GetRectangleHeightExtent() - 1));
        EXPECT_FALSE(position.IsPositionTaken(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent()) - 1,
            grid_config.GetRectangleHeightExtent()));
        EXPECT_TRUE(position.IsPositionTaken(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent()),
            grid_config.GetRectangleHeightExtent()));
    }

    // kSmall
    {
        constexpr int radius = 1;
        const auto position = CreateComponent({10, 10}, radius);

        // Check all positions inside
        EXPECT_TRUE(position.IsPositionTaken(10, 10));
        EXPECT_TRUE(position.IsPositionTaken(11, 10));
        EXPECT_TRUE(position.IsPositionTaken(11, 9));
        EXPECT_TRUE(position.IsPositionTaken(10, 9));
        EXPECT_TRUE(position.IsPositionTaken(9, 10));
        EXPECT_TRUE(position.IsPositionTaken(9, 11));
        EXPECT_TRUE(position.IsPositionTaken(10, 11));

        // Check outside bounds
        EXPECT_FALSE(position.IsPositionTaken(9, 9));
        EXPECT_FALSE(position.IsPositionTaken(8, 9));
        EXPECT_FALSE(position.IsPositionTaken(11, 12));
        EXPECT_FALSE(position.IsPositionTaken(11, 11));

        TestHasOnlyHexagonPositionsTaken(position);
    }
    // kSmall, lower limit
    {
        const int radius = 1;
        const auto position = CreateComponent({1, 1}, radius);
        TestHasOnlyHexagonPositionsTaken(position);

        // Check outside bounds
        EXPECT_FALSE(position.IsPositionTaken(3, 2));
        EXPECT_FALSE(position.IsPositionTaken(2, 3));
        EXPECT_FALSE(position.IsPositionTaken(3, 3));
        EXPECT_FALSE(position.IsPositionTaken(0, 0));
        EXPECT_FALSE(position.IsPositionTaken(2, 2));
    }
    // kSmall, higher limit
    {
        constexpr int radius = 1;
        const auto position = CreateComponent(
            {grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent() - radius) - radius,
             grid_config.GetRectangleHeightExtent() - radius},
            radius);
        TestHasOnlyHexagonPositionsTaken(position);

        // Check outside bounds
        EXPECT_FALSE(position.IsPositionTaken(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent() - 3) - 3,
            grid_config.GetRectangleHeightExtent() - 3));
        EXPECT_FALSE(position.IsPositionTaken(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent() - 4) - 4,
            grid_config.GetRectangleHeightExtent() - 4));
        EXPECT_TRUE(position.IsPositionTaken(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent() - 1) - radius,
            grid_config.GetRectangleHeightExtent() - radius));
    }

    // kMedium
    {
        constexpr int radius = 2;
        const auto position = CreateComponent({10, 10}, radius);
        TestHasOnlyHexagonPositionsTaken(position);

        // Check outside bounds
        EXPECT_FALSE(position.IsPositionTaken(7, 8));
        EXPECT_FALSE(position.IsPositionTaken(13, 12));
        EXPECT_FALSE(position.IsPositionTaken(8, 8));
        EXPECT_FALSE(position.IsPositionTaken(12, 12));
    }
    // kMedium, lower limit
    {
        constexpr int radius = 2;
        const auto position = CreateComponent({2, 2}, radius);
        TestHasOnlyHexagonPositionsTaken(position);

        // Check outside bounds
        EXPECT_FALSE(position.IsPositionTaken(4, 5));
        EXPECT_FALSE(position.IsPositionTaken(5, 4));
        EXPECT_FALSE(position.IsPositionTaken(5, 5));
        EXPECT_FALSE(position.IsPositionTaken(0, 0));
        EXPECT_FALSE(position.IsPositionTaken(4, 4));
    }
    // kMedium, higher limit
    {
        constexpr int radius = 2;
        const auto position = CreateComponent(
            {grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent() - radius) - radius,
             grid_config.GetRectangleHeightExtent() - radius},
            radius);
        TestHasOnlyHexagonPositionsTaken(position);

        // Check outside bounds
        EXPECT_FALSE(position.IsPositionTaken(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent() - 5) - 5,
            grid_config.GetRectangleHeightExtent() - 5));
        EXPECT_FALSE(position.IsPositionTaken(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent() - 6) - 6,
            grid_config.GetRectangleHeightExtent() - 6));
        EXPECT_TRUE(position.IsPositionTaken(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent() - 2) - radius,
            grid_config.GetRectangleHeightExtent() - radius));
    }

    // kLarge
    {
        constexpr int radius = 3;
        const auto position = CreateComponent({10, 10}, radius);
        TestHasOnlyHexagonPositionsTaken(position);

        // Check outside bounds
        EXPECT_FALSE(position.IsPositionTaken(6, 6));
        EXPECT_FALSE(position.IsPositionTaken(6, 7));
        EXPECT_FALSE(position.IsPositionTaken(13, 14));
        EXPECT_FALSE(position.IsPositionTaken(14, 14));
        EXPECT_FALSE(position.IsPositionTaken(7, 7));
        EXPECT_FALSE(position.IsPositionTaken(13, 13));
    }
    // kLarge, lower limit
    {
        constexpr int radius = 3;
        const auto position = CreateComponent({3, 3}, radius);
        TestHasOnlyHexagonPositionsTaken(position);

        // Check outside bounds
        EXPECT_FALSE(position.IsPositionTaken(6, 7));
        EXPECT_FALSE(position.IsPositionTaken(7, 6));
        EXPECT_FALSE(position.IsPositionTaken(7, 7));
        EXPECT_FALSE(position.IsPositionTaken(0, 0));
        EXPECT_FALSE(position.IsPositionTaken(6, 6));
    }
    // kLarge, higher limit
    {
        constexpr int radius = 3;
        const auto position = CreateComponent(
            {grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent() - radius) - radius,
             grid_config.GetRectangleHeightExtent() - radius},
            radius);
        TestHasOnlyHexagonPositionsTaken(position);

        // Check outside bounds
        EXPECT_FALSE(position.IsPositionTaken(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent() - 7) - 7,
            grid_config.GetRectangleHeightExtent() - 7));
        EXPECT_FALSE(position.IsPositionTaken(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent() - 8) - 8,
            grid_config.GetRectangleHeightExtent() - 8));
        EXPECT_FALSE(position.IsPositionTaken(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent() - 6) - 6,
            grid_config.GetRectangleHeightExtent() - 6));
    }
}

TEST_F(PositionComponentTest, IsPositionsIntersecting)
{
    const HexGridConfig& grid_config = GetGridConfig();
    // kNone
    {
        const auto position = CreateComponent({10, 10}, 0);

        EXPECT_FALSE(position.IsPositionsIntersecting(9, 9, 0));
        EXPECT_FALSE(position.IsPositionsIntersecting(10, 9, 0));
        EXPECT_FALSE(position.IsPositionsIntersecting(11, 10, 0));
        EXPECT_FALSE(position.IsPositionsIntersecting(11, 11, 0));
        EXPECT_TRUE(position.IsPositionsIntersecting(10, 10, 0));
    }
    // kNone, lower limit
    {
        const auto position = CreateComponent({0, 0}, 0);

        EXPECT_FALSE(position.IsPositionsIntersecting(-1, -1, 0));
        EXPECT_FALSE(position.IsPositionsIntersecting(1, 1, 0));
        EXPECT_TRUE(position.IsPositionsIntersecting(0, 0, 0));
    }
    // kNone, higher limit
    {
        const auto position = CreateComponent(
            {grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent()),
             grid_config.GetRectangleHeightExtent()},
            0);

        EXPECT_FALSE(position.IsPositionsIntersecting(1, 1, 0));
        EXPECT_FALSE(position.IsPositionsIntersecting(0, 0, 0));
        EXPECT_FALSE(position.IsPositionsIntersecting(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent() - 1) - 1,
            grid_config.GetRectangleHeightExtent() - 1,
            0));
        EXPECT_FALSE(position.IsPositionsIntersecting(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent()) - 2,
            grid_config.GetRectangleHeightExtent(),
            0));
        EXPECT_TRUE(position.IsPositionsIntersecting(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent()),
            grid_config.GetRectangleHeightExtent(),
            0));
    }

    // Radius 1
    TestIsPositionsIntersectingRadius1(CreateComponent({0, 0}, 1));
    TestIsPositionsIntersectingRadius1(CreateComponent({10, 10}, 1));
    TestIsPositionsIntersectingRadius1(CreateComponent(grid_config.GetMinHexGridPosition(), 1));
    TestIsPositionsIntersectingRadius1(CreateComponent(grid_config.GetMaxHexGridPosition(), 1));

    // Radius 2
    TestIsPositionsIntersectingRadius2(CreateComponent({0, 0}, 2));
    TestIsPositionsIntersectingRadius2(CreateComponent({10, 10}, 2));
    TestIsPositionsIntersectingRadius2(CreateComponent({-10, -10}, 2));
    TestIsPositionsIntersectingRadius2(CreateComponent(grid_config.GetMinHexGridPosition(), 2));
    TestIsPositionsIntersectingRadius2(CreateComponent(grid_config.GetMaxHexGridPosition(), 2));

    // Radius 3
    TestIsPositionsIntersectingRadius3(CreateComponent({0, 0}, 3));
    TestIsPositionsIntersectingRadius3(CreateComponent({10, 10}, 3));
    TestIsPositionsIntersectingRadius3(CreateComponent({-10, -10}, 3));
    TestIsPositionsIntersectingRadius3(CreateComponent(grid_config.GetMinHexGridPosition(), 3));
    TestIsPositionsIntersectingRadius3(CreateComponent(grid_config.GetMaxHexGridPosition(), 3));
}

TEST_F(PositionComponentTest, GetTakenPositions)
{
    const HexGridConfig& grid_config = GetGridConfig();
    // kNone
    {
        auto position = PositionComponent{};
        position.Init();
        position.SetPosition(10, 10);
        position.SetRadius(0);

        const std::vector<HexGridPosition> expected = {{10, 10}};
        const std::vector<HexGridPosition> got = position.GetTakenPositions();
        EXPECT_EQ(expected.size(), got.size()) << "Taken positions length does not match expected";
        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(expected[i], got[i]) << "Taken positions does not match for index" << i;
        }
    }
    {
        auto position = PositionComponent{};
        position.Init();
        position.SetPosition(0, 0);
        position.SetRadius(0);

        const std::vector<HexGridPosition> expected = {{0, 0}};
        const std::vector<HexGridPosition> got = position.GetTakenPositions();
        EXPECT_EQ(expected.size(), got.size()) << "Taken positions length does not match expected";
        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(expected[i], got[i]) << "Taken positions does not match for index " << i;
        }
    }
    {
        auto position = PositionComponent{};
        position.Init();
        position.SetPosition(
            grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent()),
            grid_config.GetRectangleHeightExtent());
        position.SetRadius(0);

        // clang-format off
        const std::vector<HexGridPosition> expected = {
            {grid_config.MapRectangleQLimitMax(grid_config.GetRectangleHeightExtent()), grid_config.GetRectangleHeightExtent()}
        };
        // clang-format on
        const std::vector<HexGridPosition> got = position.GetTakenPositions();
        EXPECT_EQ(expected.size(), got.size()) << "Taken positions length does not match expected";
        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(expected[i], got[i]) << "Taken positions does not match for index " << i;
        }
    }
    {
        auto position = PositionComponent{};
        position.Init();
        position.SetPosition(10, 10);
        position.SetRadius(1);

        std::vector<HexGridPosition> expected = {
            {10, 10},
            {9, 11},
            {10, 11},
            {11, 10},
            {11, 9},
            {10, 9},
            {9, 10},
        };
        std::vector<HexGridPosition> got = position.GetTakenPositions();
        std::sort(expected.begin(), expected.end());
        std::sort(got.begin(), got.end());

        EXPECT_EQ(expected.size(), got.size()) << "Taken positions length does not match expected";
        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(expected[i], got[i]) << "Taken positions does not match for index " << i;
        }
    }

    // From center position
    {
        auto position = PositionComponent{};
        position.Init();
        position.SetPosition(0, 0);
        position.SetRadius(0);

        const std::vector<HexGridPosition> expected = {
            {0, 0},
        };
        const std::vector<HexGridPosition> got = position.GetTakenPositions();
        EXPECT_EQ(expected.size(), got.size()) << "Taken positions length does not match expected";
        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(expected[i], got[i]) << "Taken positions does not match for index " << i;
        }
    }
    {
        auto position = PositionComponent{};
        position.Init();
        position.SetPosition(0, 0);
        position.SetRadius(1);

        std::vector<HexGridPosition> expected = {{0, 0}, {-1, 1}, {0, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, 0}};
        std::vector<HexGridPosition> got = position.GetTakenPositions();
        std::sort(expected.begin(), expected.end());
        std::sort(got.begin(), got.end());

        EXPECT_EQ(expected.size(), got.size()) << "Taken positions length does not match expected";
        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(expected[i], got[i]) << "Taken positions does not match for index " << i;
        }
    }
    {
        auto position = PositionComponent{};
        position.Init();
        position.SetPosition(0, 0);
        position.SetRadius(2);

        std::vector<HexGridPosition> expected = {
            {0, 0}, {-1, 1}, {0, 1},  {1, 0},  {1, -1}, {0, -1}, {-1, 0},  {-2, 2}, {-1, 2}, {0, 2},
            {1, 1}, {2, 0},  {2, -1}, {2, -2}, {1, -2}, {0, -2}, {-1, -1}, {-2, 0}, {-2, 1},
        };
        std::vector<HexGridPosition> got = position.GetTakenPositions();
        std::sort(expected.begin(), expected.end());
        std::sort(got.begin(), got.end());

        EXPECT_EQ(expected.size(), got.size()) << "Taken positions length does not match expected";
        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(expected[i], got[i]) << "Taken positions does not match for index " << i;
        }
    }
    {
        auto position = PositionComponent{};
        position.Init();
        position.SetPosition(0, 0);
        position.SetRadius(3);

        std::vector<HexGridPosition> expected = {
            {0, 0},  {-1, 1}, {0, 1},   {1, 0},   {1, -1}, {0, -1}, {-1, 0},  {-2, 2}, {-1, 2}, {0, 2},
            {1, 1},  {2, 0},  {2, -1},  {2, -2},  {1, -2}, {0, -2}, {-1, -1}, {-2, 0}, {-2, 1}, {-3, 3},
            {-2, 3}, {-1, 3}, {0, 3},   {1, 2},   {2, 1},  {3, 0},  {3, -1},  {3, -2}, {3, -3}, {2, -3},
            {1, -3}, {0, -3}, {-1, -2}, {-2, -1}, {-3, 0}, {-3, 1}, {-3, 2}};
        std::vector<HexGridPosition> got = position.GetTakenPositions();
        std::sort(expected.begin(), expected.end());
        std::sort(got.begin(), got.end());

        EXPECT_EQ(expected.size(), got.size()) << "Taken positions length does not match expected";
        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(expected[i], got[i]) << "Taken positions does not match for index " << i;
        }
    }
    {
        auto position = PositionComponent{};
        position.Init();
        position.SetPosition(10, 10);
        position.SetRadius(2);

        // Same as the test above where position was (0, 0) but shifted by 10, 10
        std::vector<HexGridPosition> expected = {
            {0, 0},
            {-1, 1},
            {0, 1},
            {1, 0},
            {1, -1},
            {0, -1},
            {-1, 0},
            {-2, 2},
            {-1, 2},
            {0, 2},
            {1, 1},
            {2, 0},
            {2, -1},
            {2, -2},
            {1, -2},
            {0, -2},
            {-1, -1},
            {-2, 0},
            {-2, 1}};
        for (auto& check_position : expected)
        {
            check_position = check_position + HexGridPosition{10, 10};
        }

        std::vector<HexGridPosition> got = position.GetTakenPositions();
        std::sort(expected.begin(), expected.end());
        std::sort(got.begin(), got.end());

        EXPECT_EQ(expected.size(), got.size()) << "Taken positions length does not match expected";
        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(expected[i], got[i]) << "Taken positions does not match for index " << i;
        }
    }
    {
        auto position = PositionComponent{};
        position.Init();
        position.SetPosition(-10, -10);
        position.SetRadius(3);

        // Same as the test above where position was (0, 0) but shifted by -10, -10
        std::vector<HexGridPosition> expected = {
            {0, 0},  {-1, 1}, {0, 1},   {1, 0},   {1, -1}, {0, -1}, {-1, 0},  {-2, 2}, {-1, 2}, {0, 2},
            {1, 1},  {2, 0},  {2, -1},  {2, -2},  {1, -2}, {0, -2}, {-1, -1}, {-2, 0}, {-2, 1}, {-3, 3},
            {-2, 3}, {-1, 3}, {0, 3},   {1, 2},   {2, 1},  {3, 0},  {3, -1},  {3, -2}, {3, -3}, {2, -3},
            {1, -3}, {0, -3}, {-1, -2}, {-2, -1}, {-3, 0}, {-3, 1}, {-3, 2}};
        for (auto& check_position : expected)
        {
            check_position = check_position + HexGridPosition{-10, -10};
        }
        std::vector<HexGridPosition> got = position.GetTakenPositions();
        std::sort(expected.begin(), expected.end());
        std::sort(got.begin(), got.end());

        EXPECT_EQ(expected.size(), got.size()) << "Taken positions length does not match expected ";
        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(expected[i], got[i]) << "Taken positions does not match for index " << i;
        }
    }
}

TEST(PositionComponent, IsInRange)
{
    // Null Radius, right next to each other
    {
        auto position1 = PositionComponent{};
        position1.Init();
        position1.SetPosition(0, 0);
        position1.SetRadius(0);

        auto position2 = PositionComponent{};
        position2.Init();
        position2.SetPosition(3, 0);
        position2.SetRadius(0);

        EXPECT_FALSE(position1.IsInRange(position2, 0));
        EXPECT_FALSE(position1.IsInRange(position2, 1));
        EXPECT_TRUE(position1.IsInRange(position2, 2));
        EXPECT_TRUE(position1.IsInRange(position2, 3));
    }

    // Radius + Null Radius, right next to each other
    {
        auto position1 = PositionComponent{};
        position1.Init();
        position1.SetPosition(0, 0);
        position1.SetRadius(0);

        auto position2 = PositionComponent{};
        position2.Init();
        position2.SetPosition(3, 0);
        position2.SetRadius(1);

        EXPECT_FALSE(position1.IsInRange(position2, 0));
        EXPECT_TRUE(position1.IsInRange(position2, 1));
        EXPECT_TRUE(position1.IsInRange(position2, 2));
    }

    // Same Radius, right next to each other
    {
        auto position1 = PositionComponent{};
        position1.Init();
        position1.SetPosition(0, 0);
        position1.SetRadius(1);

        auto position2 = PositionComponent{};
        position2.Init();
        position2.SetPosition(3, 0);
        position2.SetRadius(1);

        EXPECT_TRUE(position1.IsInRange(position2, 0));
        EXPECT_TRUE(position1.IsInRange(position2, 1));
        EXPECT_TRUE(position1.IsInRange(position2, 2));
        EXPECT_TRUE(position1.IsInRange(position2, 3));
    }

    // Different radius, right next to each other
    {
        auto position1 = PositionComponent{};
        position1.Init();
        position1.SetPosition(0, 0);
        position1.SetRadius(1);

        auto position2 = PositionComponent{};
        position2.Init();
        position2.SetPosition(4, 0);
        position2.SetRadius(2);

        EXPECT_TRUE(position1.IsInRange(position2, 0));
        EXPECT_TRUE(position1.IsInRange(position2, 1));
        EXPECT_TRUE(position1.IsInRange(position2, 2));
        EXPECT_TRUE(position1.IsInRange(position2, 3));
    }

    // 0 degree angle, sizes 5
    {
        auto position1 = PositionComponent{};
        position1.Init();
        position1.SetPosition(10, 10);
        position1.SetRadius(3);

        auto position2 = PositionComponent{};
        position2.Init();
        position2.SetPosition(50, 10);
        position2.SetRadius(2);

        // Distance 40, sum of sizes 5, should fail on 33 and pass on 34
        EXPECT_FALSE(position1.IsInRange(position2, 33));
        EXPECT_TRUE(position1.IsInRange(position2, 34));
    }

    // 45 degree angle, sizes 8
    {
        auto position1 = PositionComponent{};
        position1.Init();
        position1.SetPosition(10, 10);
        position1.SetRadius(3);

        auto position2 = PositionComponent{};
        position2.Init();
        position2.SetPosition(50, 50);
        position2.SetRadius(5);

        // Distance 80, sum of sizes 8, should fail on 72 and pass on 73
        EXPECT_FALSE(position1.IsInRange(position2, 70));
        EXPECT_TRUE(position1.IsInRange(position2, 71));
    }

    // 63 degree angle, sizes 12
    {
        auto position1 = PositionComponent{};
        position1.Init();
        position1.SetPosition(10, 10);
        position1.SetRadius(4);

        auto position2 = PositionComponent{};
        position2.Init();
        position2.SetPosition(30, 50);
        position2.SetRadius(8);

        // Distance 60, sum of sizes 12, should fail on 46 and pass on 47
        EXPECT_FALSE(position1.IsInRange(position2, 46));
        EXPECT_TRUE(position1.IsInRange(position2, 47));
    }

    // 334 degree angle, sizes 7
    {
        auto position1 = PositionComponent{};
        position1.Init();
        position1.SetPosition(50, 10);
        position1.SetRadius(5);

        auto position2 = PositionComponent{};
        position2.Init();
        position2.SetPosition(30, 50);
        position2.SetRadius(2);

        // Distance 40, sum of sizes 7, should fail on 31 and pass on 32
        EXPECT_FALSE(position1.IsInRange(position2, 31));
        EXPECT_TRUE(position1.IsInRange(position2, 32));
    }
}

}  // namespace simulation
