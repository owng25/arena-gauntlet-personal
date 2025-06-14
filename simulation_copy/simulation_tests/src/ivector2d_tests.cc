#include "base_test_fixtures.h"
#include "gtest/gtest.h"
#include "utility/ivector2d.h"

namespace simulation
{
class IVector2DTest : public BaseTest
{
};

TEST(IVector2D, Create)
{
    // Default
    {
        auto v = IVector2D();
        ASSERT_EQ(0, v.x);
        ASSERT_EQ(0, v.y);
    }

    // Int arguments
    {
        const int x = -55;
        const int y = 60;
        auto v = IVector2D(x, y);
        ASSERT_EQ(x, v.x);
        ASSERT_EQ(y, v.y);
    }

    // Other vector
    {
        auto other = IVector2D(-55, 60);
        auto v = IVector2D(other);
        ASSERT_EQ(v.x, other.x);
        ASSERT_EQ(v.y, other.y);

        // ==
        ASSERT_EQ(v, other);

        // !=
        other.x += 5;
        ASSERT_NE(v, other);

        // =
        other = v;
        ASSERT_EQ(v, other);
    }

    // Move constructor and move =
    {
        auto v = IVector2D(IVector2D(-55, 60));
        ASSERT_EQ(-55, v.x);
        ASSERT_EQ(60, v.y);

        v = IVector2D(42, 32);
        ASSERT_EQ(42, v.x);
        ASSERT_EQ(32, v.y);
    }
}

TEST(IVector2D, ArithmeticAndAssignOperators)
{
    // +=
    {
        auto v = IVector2D(-55, 60);
        v += IVector2D(100, 200);
        ASSERT_EQ(45, v.x);
        ASSERT_EQ(260, v.y);
    }

    // -=
    {
        auto v = IVector2D(-55, 60);
        v -= IVector2D(100, 200);
        ASSERT_EQ(-155, v.x);
        ASSERT_EQ(-140, v.y);
    }

    // + vector
    {
        auto lhs = IVector2D(-55, 60);
        auto rhs = IVector2D(40, -10);
        auto r = lhs + rhs;
        ASSERT_EQ(-15, r.x);
        ASSERT_EQ(50, r.y);
    }

    // - vector
    {
        auto lhs = IVector2D(-55, 60);
        auto rhs = IVector2D(40, -10);
        auto r = lhs - rhs;
        ASSERT_EQ(-95, r.x);
        ASSERT_EQ(70, r.y);
    }

    // / scalar
    {
        auto lhs = IVector2D(-55, 60);
        auto rhs = 5;
        auto r = lhs / rhs;
        ASSERT_EQ(-11, r.x);
        ASSERT_EQ(12, r.y);
    }

    // % scalar
    {
        auto lhs = IVector2D(50, 60);
        auto rhs = 7;
        auto r = lhs % rhs;
        ASSERT_EQ(1, r.x);
        ASSERT_EQ(4, r.y);
    }

    // * scalar
    {
        auto lhs = IVector2D(50, 60);
        auto rhs = 7;
        auto r = lhs * rhs;
        ASSERT_EQ(350, r.x);
        ASSERT_EQ(420, r.y);
    }
}

TEST(IVector2D, Magnitude)
{
    ASSERT_EQ(6625, IVector2D(-55, -60).SquareMagnitude());
    ASSERT_EQ(200, IVector2D(10, 10).SquareMagnitude());
    ASSERT_EQ(81, IVector2D(-55, -60).Magnitude());
    ASSERT_EQ(81, IVector2D(-55, -60).Length());
    ASSERT_EQ(0, IVector2D(0, 0).Length());

    // Small numbers
    ASSERT_EQ(0, IVector2D(0, 0).Magnitude());
    ASSERT_EQ(1, IVector2D(1, 1).Magnitude());
    ASSERT_EQ(2, IVector2D(1, 2).Magnitude());
    ASSERT_EQ(7, IVector2D(5, 5).Magnitude());
}

TEST(IVector2D, Rotate)
{
    EXPECT_EQ(IVector2D(3, 10), IVector2D(5, 10).Rotate(10));
    EXPECT_EQ(IVector2D(7, 8), IVector2D(10, 5).Rotate(20));
    EXPECT_EQ(IVector2D(61, 93), IVector2D(100, 50).Rotate(30));
    EXPECT_EQ(IVector2D(35, -106), IVector2D(-50, -100).Rotate(45));
    EXPECT_EQ(IVector2D(0, 0), IVector2D(0, 0).Rotate(90));

    EXPECT_EQ(IVector2D(-10, 3), IVector2D(5, 10).Rotate(100));
    EXPECT_EQ(IVector2D(-9, 6), IVector2D(10, 5).Rotate(120));
    EXPECT_EQ(IVector2D(-102, 44), IVector2D(100, 50).Rotate(130));
    EXPECT_EQ(IVector2D(98, 53), IVector2D(-50, -100).Rotate(145));
    EXPECT_EQ(IVector2D(0, 0), IVector2D(0, 0).Rotate(180));

    EXPECT_EQ(IVector2D(-25, -50), IVector2D(-50, 25).Rotate(90));
    EXPECT_EQ(IVector2D(-50, -25), IVector2D(-25, 50).Rotate(90));
    EXPECT_EQ(IVector2D(-50, 25), IVector2D(50, -25).Rotate(180));
    EXPECT_EQ(IVector2D(-25, 50), IVector2D(25, -50).Rotate(180));
}

TEST(IVector2D, AngleToPosition)
{
    EXPECT_EQ(0, IVector2D(5, 5).AngleToPosition({5, 5}));
    EXPECT_EQ(90, IVector2D(5, 5).AngleToPosition({5, 10}));
    EXPECT_EQ(0, IVector2D(5, 5).AngleToPosition({10, 5}));
    EXPECT_EQ(0, IVector2D(5, 5).AngleToPosition({430, 5}));
    EXPECT_EQ(341, IVector2D(4, 10).AngleToPosition({10, 8}));
    EXPECT_EQ(45, IVector2D(50, 50).AngleToPosition({60, 60}));

    // Compare 25 manually calculated angles with random positions
    struct AngleTest
    {
        int x1, y1, x2, y2;
        int expected_angle;
    };
    std::array<AngleTest, 25> values = {AngleTest{255, 376, -216, -159, 228}, AngleTest{183, 389, -61, -262, 249},
                                        AngleTest{-421, 285, -15, -399, 300}, AngleTest{467, 32, 252, 10, 186},
                                        AngleTest{428, 462, -149, -493, 239}, AngleTest{-494, -407, -380, -350, 26},
                                        AngleTest{-202, -102, -84, 42, 50},   AngleTest{-216, -393, 7, -10, 60},
                                        AngleTest{-290, 266, -437, -77, 246}, AngleTest{-387, -339, -155, -23, 53},
                                        AngleTest{444, 293, 462, -89, 272},   AngleTest{40, -343, 319, -280, 12},
                                        AngleTest{144, 46, 186, -494, 274},   AngleTest{454, 78, 412, -102, 256},
                                        AngleTest{-69, 451, -13, 174, 281},   AngleTest{400, -245, -311, 89, 155},
                                        AngleTest{-266, -6, -381, -114, 223}, AngleTest{160, -290, -166, -374, 194},
                                        AngleTest{62, -133, -286, 193, 136},  AngleTest{-114, -321, 69, 395, 75},
                                        AngleTest{55, 390, -189, 305, 199},   AngleTest{90, -220, 43, 212, 96},
                                        AngleTest{-96, 317, -499, 278, 185},  AngleTest{-447, -65, 199, 134, 17},
                                        AngleTest{133, -137, 85, -80, 130}};

    // Test them
    for (const AngleTest& test : values)
    {
        // Allow a small spread to account for rounding and minor error which could go either way

        // Make sure the value is greater than the known value - 2
        EXPECT_GT(IVector2D(test.x1, test.y1).AngleToPosition({test.x2, test.y2}), test.expected_angle - 2)
            << "Angle between (" << test.x1 << "," << test.y1 << ") should be more than " << test.expected_angle - 2;

        // Make sure the value is less than the known value + 2
        EXPECT_LT(IVector2D(test.x1, test.y1).AngleToPosition({test.x2, test.y2}), test.expected_angle + 2)
            << "Angle between (" << test.x1 << "," << test.y1 << ") should be less than " << test.expected_angle + 2;
    }
}

TEST(IVector2D, TriangleVertices)
{
    IVector2D triangle_a, triangle_b, triangle_c;

    IVector2D{0, 0}.GetTriangleVertices(0, 0, &triangle_a, &triangle_b, &triangle_c);
    EXPECT_EQ(triangle_a, IVector2D(0, 0));
    EXPECT_EQ(triangle_b, IVector2D(0, 0));
    EXPECT_EQ(triangle_c, IVector2D(0, 0));

    IVector2D{0, 0}.GetTriangleVertices(0, 5, &triangle_a, &triangle_b, &triangle_c);
    EXPECT_EQ(triangle_a, IVector2D(0, 0));
    EXPECT_EQ(triangle_b, IVector2D(5, -2));
    EXPECT_EQ(triangle_c, IVector2D(5, 2));

    IVector2D{0, 0}.GetTriangleVertices(90, 7, &triangle_a, &triangle_b, &triangle_c);
    EXPECT_EQ(triangle_a, IVector2D(0, 0));
    EXPECT_EQ(triangle_b, IVector2D(4, 7));
    EXPECT_EQ(triangle_c, IVector2D(-4, 7));

    IVector2D{55, -110}.GetTriangleVertices(0, 0, &triangle_a, &triangle_b, &triangle_c);
    EXPECT_EQ(triangle_a, IVector2D(55, -110));
    EXPECT_EQ(triangle_b, IVector2D(55, -110));
    EXPECT_EQ(triangle_c, IVector2D(55, -110));

    IVector2D{-110, 55}.GetTriangleVertices(0, 5, &triangle_a, &triangle_b, &triangle_c);
    EXPECT_EQ(triangle_a, IVector2D(-110, 55));
    EXPECT_EQ(triangle_b, IVector2D(-105, 53));
    EXPECT_EQ(triangle_c, IVector2D(-105, 57));

    IVector2D{-217, -714}.GetTriangleVertices(90, 7, &triangle_a, &triangle_b, &triangle_c);
    EXPECT_EQ(triangle_a, IVector2D(-217, -714));
    EXPECT_EQ(triangle_b, IVector2D(-213, -707));
    EXPECT_EQ(triangle_c, IVector2D(-221, -707));

    IVector2D{55, -110}.GetTriangleVertices(216, 0, &triangle_a, &triangle_b, &triangle_c);
    EXPECT_EQ(triangle_a, IVector2D(55, -110));
    EXPECT_EQ(triangle_b, IVector2D(55, -110));
    EXPECT_EQ(triangle_c, IVector2D(55, -110));

    IVector2D{-110, 55}.GetTriangleVertices(132, 5, &triangle_a, &triangle_b, &triangle_c);
    EXPECT_EQ(triangle_a, IVector2D(-110, 55));
    EXPECT_EQ(triangle_b, IVector2D(-111, 60));
    EXPECT_EQ(triangle_c, IVector2D(-114, 57));

    IVector2D{-217, -714}.GetTriangleVertices(27, 7, &triangle_a, &triangle_b, &triangle_c);
    EXPECT_EQ(triangle_a, IVector2D(-217, -714));
    EXPECT_EQ(triangle_b, IVector2D(-209, -714));
    EXPECT_EQ(triangle_c, IVector2D(-213, -708));
}

TEST(IVector2D, TriangleArea)
{
    EXPECT_EQ(IVector2D::TriangleArea(IVector2D(0, 0), IVector2D(0, 0), IVector2D(0, 0)), 0);
    EXPECT_EQ(IVector2D::TriangleArea(IVector2D(1, 0), IVector2D(0, 1), IVector2D(1, 1)), 0);
    EXPECT_EQ(IVector2D::TriangleArea(IVector2D(10, 0), IVector2D(0, 10), IVector2D(10, 10)), 50);
    EXPECT_EQ(IVector2D::TriangleArea(IVector2D(100, 0), IVector2D(0, 100), IVector2D(100, 100)), 5000);
    EXPECT_EQ(IVector2D::TriangleArea(IVector2D(0, -1), IVector2D(-1, 0), IVector2D(-1, -1)), 0);
    EXPECT_EQ(IVector2D::TriangleArea(IVector2D(0, -10), IVector2D(-10, 0), IVector2D(-10, -10)), 50);
    EXPECT_EQ(IVector2D::TriangleArea(IVector2D(0, -100), IVector2D(-100, 0), IVector2D(-100, -100)), 5000);
    EXPECT_EQ(IVector2D::TriangleArea(IVector2D(287, -462), IVector2D(109, 41), IVector2D(914, -217)), 179495);
    EXPECT_EQ(IVector2D::TriangleArea(IVector2D(-512, 218), IVector2D(742, -811), IVector2D(-410, 926)), 496395);
}

}  // namespace simulation
