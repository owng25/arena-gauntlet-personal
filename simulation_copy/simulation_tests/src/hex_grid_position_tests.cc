#include "base_test_fixtures.h"
#include "gtest/gtest.h"
#include "utility/hex_grid_position.h"
#include "utility/ivector2d.h"

namespace simulation
{
class HexGridPositionTest : public BaseTest
{
};

TEST(HexGridPosition, Create)
{
    // Default
    {
        auto v = HexGridPosition();
        ASSERT_EQ(0, v.q);
        ASSERT_EQ(0, v.r);
    }

    // Int arguments
    {
        const int q = -55;
        const int r = 60;
        auto v = HexGridPosition(q, r);
        ASSERT_EQ(q, v.q);
        ASSERT_EQ(r, v.r);
    }

    // Other vector
    {
        auto other = HexGridPosition(-55, 60);
        auto v = HexGridPosition(other);
        ASSERT_EQ(v.q, other.q);
        ASSERT_EQ(v.r, other.r);

        // ==
        ASSERT_EQ(v, other);

        // !=
        other.q += 5;
        ASSERT_NE(v, other);

        // =
        other = v;
        ASSERT_EQ(v, other);
    }

    // Move constructor and move =
    {
        auto v = HexGridPosition(HexGridPosition(-55, 60));
        ASSERT_EQ(-55, v.q);
        ASSERT_EQ(60, v.r);

        v = HexGridPosition(42, 32);
        ASSERT_EQ(42, v.q);
        ASSERT_EQ(32, v.r);
    }
}

TEST(HexGridPosition, ArithmeticAndAssignOperators)
{
    // +=
    {
        auto v = HexGridPosition(-55, 60);
        v += HexGridPosition(100, 200);
        ASSERT_EQ(45, v.q);
        ASSERT_EQ(260, v.r);
    }

    // -=
    {
        auto v = HexGridPosition(-55, 60);
        v -= HexGridPosition(100, 200);
        ASSERT_EQ(-155, v.q);
        ASSERT_EQ(-140, v.r);
    }

    // + vector
    {
        auto lhs = HexGridPosition(-55, 60);
        auto rhs = HexGridPosition(40, -10);
        auto r = lhs + rhs;
        ASSERT_EQ(-15, r.q);
        ASSERT_EQ(50, r.r);
    }

    // - vector
    {
        auto lhs = HexGridPosition(-55, 60);
        auto rhs = HexGridPosition(40, -10);
        auto r = lhs - rhs;
        ASSERT_EQ(-95, r.q);
        ASSERT_EQ(70, r.r);
    }

    // / scalar
    {
        auto lhs = HexGridPosition(-55, 60);
        auto rhs = 5;
        auto r = lhs / rhs;
        ASSERT_EQ(-11, r.q);
        ASSERT_EQ(12, r.r);
    }

    // % scalar
    {
        auto lhs = HexGridPosition(50, 60);
        auto rhs = 7;
        auto r = lhs % rhs;
        ASSERT_EQ(1, r.q);
        ASSERT_EQ(4, r.r);
    }

    // * scalar
    {
        auto lhs = HexGridPosition(50, 60);
        auto rhs = 7;
        auto r = lhs * rhs;
        ASSERT_EQ(350, r.q);
        ASSERT_EQ(420, r.r);
    }
}

TEST_F(HexGridPositionTest, FromWorldPosition)
{
    EXPECT_EQ(world->FromWorldPosition(IVector2D(0, 0)), HexGridPosition(0, 0));
    EXPECT_EQ(world->FromWorldPosition(IVector2D(11, 0)), HexGridPosition(1, 0));
    EXPECT_EQ(world->FromWorldPosition(IVector2D(55, 33)), HexGridPosition(2, 2));
    EXPECT_EQ(world->FromWorldPosition(IVector2D(11, 88)), HexGridPosition(-2, 6));
    EXPECT_EQ(world->FromWorldPosition(IVector2D(155, 133)), HexGridPosition(4, 9));
    EXPECT_EQ(world->FromWorldPosition(IVector2D(202, 188)), HexGridPosition(5, 13));
    EXPECT_EQ(world->FromWorldPosition(IVector2D(255, 233)), HexGridPosition(7, 15));
    EXPECT_EQ(world->FromWorldPosition(IVector2D(303, 282)), HexGridPosition(8, 19));
    EXPECT_EQ(world->FromWorldPosition(IVector2D(311, 299)), HexGridPosition(8, 20));
    EXPECT_EQ(world->FromWorldPosition(IVector2D(322, 303)), HexGridPosition(9, 20));
    EXPECT_EQ(world->FromWorldPosition(IVector2D(333, 311)), HexGridPosition(9, 21));
    EXPECT_EQ(world->FromWorldPosition(IVector2D(1500, 1500)), HexGridPosition(37, 100));

    const HexGridConfig& grid_config = GetGridConfig();
    EXPECT_EQ(world->FromWorldPosition(world->ToWorldPosition(HexGridPosition(-10, -10))), HexGridPosition(-10, -10));
    EXPECT_EQ(
        world->FromWorldPosition(world->ToWorldPosition(grid_config.GetMinHexGridPosition())),
        grid_config.GetMinHexGridPosition());
    EXPECT_EQ(
        world->FromWorldPosition(world->ToWorldPosition(grid_config.GetMaxHexGridPosition())),
        grid_config.GetMaxHexGridPosition());
}

TEST_F(HexGridPositionTest, ToWorldPosition)
{
    EXPECT_EQ(world->ToWorldPosition(HexGridPosition(0, 0)), IVector2D(0, 0));
    EXPECT_EQ(world->ToWorldPosition(HexGridPosition(1, 0)), IVector2D(17, 0));
    EXPECT_EQ(world->ToWorldPosition(HexGridPosition(1, 1)), IVector2D(25, 15));

    EXPECT_EQ(world->ToWorldPosition(HexGridPosition(-10, -10)), IVector2D(-259, -150));
    EXPECT_EQ(world->ToWorldPosition(HexGridPosition(10, 8)), IVector2D(242, 120));
    EXPECT_EQ(world->ToWorldPosition(HexGridPosition(15, 13)), IVector2D(372, 195));
    EXPECT_EQ(world->ToWorldPosition(HexGridPosition(20, 18)), IVector2D(502, 270));
    EXPECT_EQ(world->ToWorldPosition(HexGridPosition(25, 23)), IVector2D(632, 345));
    EXPECT_EQ(world->ToWorldPosition(HexGridPosition(30, 28)), IVector2D(762, 420));
    EXPECT_EQ(world->ToWorldPosition(HexGridPosition(31, 29)), IVector2D(788, 435));
    EXPECT_EQ(world->ToWorldPosition(HexGridPosition(32, 30)), IVector2D(814, 450));
    EXPECT_EQ(world->ToWorldPosition(HexGridPosition(33, 31)), IVector2D(840, 465));

    // Verified with: https://ideone.com/65ulvd
    const HexGridConfig& grid_config = GetGridConfig();
    EXPECT_EQ(world->ToWorldPosition(grid_config.GetMinHexGridPosition()), IVector2D(-1290, -1125));
    EXPECT_EQ(
        world->ToWorldPosition(HexGridPosition(
            grid_config.MapRectangleQLimitMax(grid_config.MapRectangleRLimitMin()),
            grid_config.MapRectangleRLimitMin())),
        IVector2D(1307, -1125));

    EXPECT_EQ(world->ToWorldPosition(grid_config.GetMaxHexGridPosition()), IVector2D(1307, 1125));
    EXPECT_EQ(
        world->ToWorldPosition(HexGridPosition(
            grid_config.MapRectangleQLimitMin(grid_config.MapRectangleRLimitMax()),
            grid_config.MapRectangleRLimitMax())),
        IVector2D(-1290, 1125));
}

TEST(HexGridPosition, Length)
{
    EXPECT_EQ(HexGridPosition(0, 0).Length(), 0);
    EXPECT_EQ(HexGridPosition(40, 0).Length(), 40);
    EXPECT_EQ(HexGridPosition(40, 40).Length(), 80);
    EXPECT_EQ(HexGridPosition(-20, 20).Length(), 20);
}

TEST_F(HexGridPositionTest, MapRectangleQLimitMin)
{
    const HexGridConfig& grid_config = GetGridConfig();
    // Center position, range [-kGridExtent, kGridExtent]
    EXPECT_EQ(grid_config.MapRectangleQLimitMin(0), -75);
    EXPECT_EQ(grid_config.MapRectangleQLimitMin(0), grid_config.MapRectangleRLimitMin());

    EXPECT_EQ(grid_config.MapRectangleQLimitMin(-1), -74);
    EXPECT_EQ(grid_config.MapRectangleQLimitMin(-2), -74);

    EXPECT_EQ(grid_config.MapRectangleQLimitMin(1), -75);
    EXPECT_EQ(grid_config.MapRectangleQLimitMin(2), -76);

    EXPECT_EQ(grid_config.MapRectangleQLimitMin(-50), -50);
    EXPECT_EQ(grid_config.MapRectangleQLimitMin(50), -100);

    EXPECT_EQ(grid_config.MapRectangleQLimitMin(grid_config.MapRectangleRLimitMin()), -37);
    EXPECT_EQ(grid_config.MapRectangleQLimitMin(grid_config.MapRectangleRLimitMax()), -112);
    EXPECT_EQ(grid_config.MapRectangleQLimitMin(grid_config.MapRectangleRLimitMax() - 1), -112);
}

TEST_F(HexGridPositionTest, MapRectangleQLimitMax)
{
    const HexGridConfig& grid_config = GetGridConfig();
    // Center position, range [-kGridExtent, kGridExtent]
    EXPECT_EQ(grid_config.MapRectangleQLimitMax(0), 75);
    EXPECT_EQ(grid_config.MapRectangleQLimitMax(0), grid_config.MapRectangleRLimitMax());

    EXPECT_EQ(grid_config.MapRectangleQLimitMax(-1), 76);
    EXPECT_EQ(grid_config.MapRectangleQLimitMax(-2), 76);

    EXPECT_EQ(grid_config.MapRectangleQLimitMax(1), 75);
    EXPECT_EQ(grid_config.MapRectangleQLimitMax(2), 74);

    EXPECT_EQ(grid_config.MapRectangleQLimitMax(-50), 100);
    EXPECT_EQ(grid_config.MapRectangleQLimitMax(50), 50);

    EXPECT_EQ(grid_config.MapRectangleQLimitMax(grid_config.MapRectangleRLimitMin()), 113);
    EXPECT_EQ(grid_config.MapRectangleQLimitMax(grid_config.MapRectangleRLimitMax()), 38);
}

TEST_F(HexGridPositionTest, QLimit)
{
    const HexGridConfig& grid_config = GetGridConfig();
    // Difference at the limits should be 0
    EXPECT_EQ(
        Math::Abs(
            grid_config.MapRectangleQLimitMax(grid_config.MapRectangleRLimitMin()) -
            grid_config.MapRectangleQLimitMin(grid_config.MapRectangleRLimitMin())),
        grid_config.GetGridWidth() - 1);
    EXPECT_EQ(
        Math::Abs(
            grid_config.MapRectangleQLimitMax(grid_config.MapRectangleRLimitMax()) -
            grid_config.MapRectangleQLimitMin(grid_config.MapRectangleRLimitMax())),
        grid_config.GetGridWidth() - 1);
}

TEST_F(HexGridPositionTest, RLimit)
{
    const HexGridConfig& grid_config = GetGridConfig();
    EXPECT_EQ(grid_config.MapRectangleRLimitMin(), -grid_config.GetRectangleHeightExtent());
    EXPECT_EQ(grid_config.MapRectangleRLimitMax(), grid_config.GetRectangleHeightExtent());
}

TEST_F(HexGridPositionTest, ToOffSetOddR)
{
    const HexGridConfig& grid_config = GetGridConfig();
    EXPECT_EQ(HexGridPosition(0, 0).ToOffSetOddR(), IVector2D(0, 0));
    EXPECT_EQ(HexGridPosition(+1, 0).ToOffSetOddR(), IVector2D(1, 0));
    EXPECT_EQ(HexGridPosition(-1, 0).ToOffSetOddR(), IVector2D(-1, 0));
    EXPECT_EQ(HexGridPosition(+1, -1).ToOffSetOddR(), IVector2D(0, -1));
    EXPECT_EQ(HexGridPosition(0, -1).ToOffSetOddR(), IVector2D(-1, -1));
    EXPECT_EQ(HexGridPosition(-1, +1).ToOffSetOddR(), IVector2D(-1, 1));
    EXPECT_EQ(HexGridPosition(0, +1).ToOffSetOddR(), IVector2D(0, 1));

    EXPECT_EQ(
        grid_config.GetMinHexGridPosition().ToOffSetOddR(),
        IVector2D(-grid_config.GetRectangleHeightExtent(), -grid_config.GetRectangleHeightExtent()));
    EXPECT_EQ(
        grid_config.GetMaxHexGridPosition().ToOffSetOddR(),
        IVector2D(grid_config.GetRectangleHeightExtent(), grid_config.GetRectangleHeightExtent()));

    EXPECT_EQ(HexGridPosition(-1, -5).ToOffSetOddR(), IVector2D(-4, -5));
    EXPECT_EQ(HexGridPosition(1, 10).ToOffSetOddR(), IVector2D(6, 10));
}

TEST_F(HexGridPositionTest, FromOffsetOddR)
{
    const HexGridConfig& grid_config = GetGridConfig();
    EXPECT_EQ(HexGridPosition::FromOffsetOddR(IVector2D(0, 0)), HexGridPosition(0, 0));
    EXPECT_EQ(HexGridPosition::FromOffsetOddR(IVector2D(1, 0)), HexGridPosition(+1, 0));
    EXPECT_EQ(HexGridPosition::FromOffsetOddR(IVector2D(-1, 0)), HexGridPosition(-1, 0));
    EXPECT_EQ(HexGridPosition::FromOffsetOddR(IVector2D(0, -1)), HexGridPosition(+1, -1));
    EXPECT_EQ(HexGridPosition::FromOffsetOddR(IVector2D(-1, -1)), HexGridPosition(0, -1));
    EXPECT_EQ(HexGridPosition::FromOffsetOddR(IVector2D(-1, +1)), HexGridPosition(-1, +1));
    EXPECT_EQ(HexGridPosition::FromOffsetOddR(IVector2D(0, +1)), HexGridPosition(0, +1));

    EXPECT_EQ(
        HexGridPosition::FromOffsetOddR(
            IVector2D(-grid_config.GetRectangleHeightExtent(), -grid_config.GetRectangleHeightExtent())),
        grid_config.GetMinHexGridPosition());
    EXPECT_EQ(
        HexGridPosition::FromOffsetOddR(
            IVector2D(grid_config.GetRectangleHeightExtent(), grid_config.GetRectangleHeightExtent())),
        grid_config.GetMaxHexGridPosition());

    EXPECT_EQ(HexGridPosition::FromOffsetOddR(IVector2D(-4, -5)), HexGridPosition(-1, -5));
    EXPECT_EQ(HexGridPosition::FromOffsetOddR(IVector2D(6, 10)), HexGridPosition(1, 10));
}

}  // namespace simulation
