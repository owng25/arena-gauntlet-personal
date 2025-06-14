#include <array>
#include <unordered_map>

#include "gtest/gtest.h"
#include "utility/leveling_helper.h"
#include "utility/math.h"

namespace simulation
{

TEST(Math, Abs)
{
    EXPECT_EQ(999999999, Math::Abs(-999999999));
    EXPECT_EQ(256, Math::Abs(-256));
    EXPECT_EQ(1, Math::Abs(-1));
    EXPECT_EQ(0, Math::Abs(-0));
    EXPECT_EQ(0, Math::Abs(0));
    EXPECT_EQ(256, Math::Abs(256));
    EXPECT_EQ(999999999, Math::Abs(999999999));
}

TEST(Math, PercentageOf)
{
    EXPECT_EQ(0, Math::PercentageOf(0, 0));
    EXPECT_EQ(0, Math::PercentageOf(5, 10));
    EXPECT_EQ(1, Math::PercentageOf(10, 10));
    EXPECT_EQ(10, Math::PercentageOf(10, 100));
    EXPECT_EQ(25, Math::PercentageOf(25, 100));
    EXPECT_EQ(50, Math::PercentageOf(50, 100));
    EXPECT_EQ(100, Math::PercentageOf(100, 100));
    EXPECT_EQ(1234, Math::PercentageOf(100, 1234));
    EXPECT_EQ(750, Math::PercentageOf(75, 1000));
    EXPECT_EQ(100, Math::PercentageOf(20, 500));
    EXPECT_EQ(-50, Math::PercentageOf(50, -100));

    EXPECT_EQ(50, Math::PercentageOf(500, 10));
    EXPECT_EQ(51, Math::PercentageOf(510, 10));
    EXPECT_EQ(60, Math::PercentageOf(600, 10));

    EXPECT_EQ(7500000, Math::PercentageOf(75, 10000000));
    EXPECT_EQ(6600000, Math::PercentageOf(33, 20'000'000));

    EXPECT_EQ(503923421, Math::PercentageOf(25, 2'015'693'686));

    EXPECT_EQ(1, Math::PercentageOf(100, 1));
    EXPECT_EQ(2, Math::PercentageOf(100, 2));

    static_assert((25_fp).AsPercentageOf(1000_fp) == 250_fp);
    static_assert(Math::PercentageOf(25, 1000) == 250);
}

TEST(Math, PostMitigationDamage)
{
    EXPECT_EQ(0_fp, Math::PostMitigationDamage(0_fp, 0_fp));
    EXPECT_EQ(42_fp, Math::PostMitigationDamage(42_fp, 0_fp));
    EXPECT_EQ("43.333"_fp, Math::PostMitigationDamage(65_fp, 50_fp));
    EXPECT_EQ(10000_fp, Math::PostMitigationDamage(10000_fp, 0_fp));
    EXPECT_EQ("45.454"_fp, Math::PostMitigationDamage(50_fp, 10_fp));
    EXPECT_EQ("33.333"_fp, Math::PostMitigationDamage(50_fp, 50_fp));
    EXPECT_EQ("35.294"_fp, Math::PostMitigationDamage(60_fp, 70_fp));
    EXPECT_EQ("66.666"_fp, Math::PostMitigationDamage(100_fp, 50_fp));
    EXPECT_EQ(50_fp, Math::PostMitigationDamage(100_fp, 100_fp));
    EXPECT_EQ("33.333"_fp, Math::PostMitigationDamage(100_fp, 200_fp));
    EXPECT_EQ(20_fp, Math::PostMitigationDamage(100_fp, 400_fp));
    EXPECT_EQ("9.090"_fp, Math::PostMitigationDamage(100_fp, 1000_fp));
    EXPECT_EQ("0.99"_fp, Math::PostMitigationDamage(100_fp, 10000_fp));

    EXPECT_EQ("866.666"_fp, Math::PostMitigationDamage(1300_fp, 50_fp));
    EXPECT_EQ("818.181"_fp, Math::PostMitigationDamage(900_fp, 10_fp));
    EXPECT_EQ("809.090"_fp, Math::PostMitigationDamage(890_fp, 10_fp));

    EXPECT_EQ("66666.666"_fp, Math::PostMitigationDamage(100'000_fp, 50_fp));
    EXPECT_EQ(500000_fp, Math::PostMitigationDamage(1'000'000_fp, 100_fp));
}

TEST(Math, VulnerabilityMitigation)
{
    EXPECT_EQ(0_fp, Math::VulnerabilityMitigation(0_fp, 0_fp));
    EXPECT_EQ(0_fp, Math::VulnerabilityMitigation(42_fp, 0_fp));
    EXPECT_EQ("32.500"_fp, Math::VulnerabilityMitigation(65_fp, 50_fp));
    EXPECT_EQ(0_fp, Math::VulnerabilityMitigation(10000_fp, 0_fp));
    EXPECT_EQ(5_fp, Math::VulnerabilityMitigation(50_fp, 10_fp));
    EXPECT_EQ(25_fp, Math::VulnerabilityMitigation(50_fp, 50_fp));
    EXPECT_EQ(42_fp, Math::VulnerabilityMitigation(60_fp, 70_fp));
    EXPECT_EQ("18.500"_fp, Math::VulnerabilityMitigation(37_fp, 50_fp));
    EXPECT_EQ("26.560"_fp, Math::VulnerabilityMitigation(83_fp, 32_fp));
    EXPECT_EQ("22.560"_fp, Math::VulnerabilityMitigation(47_fp, 48_fp));
    EXPECT_EQ("37.950"_fp, Math::VulnerabilityMitigation(55_fp, 69_fp));
    EXPECT_EQ(83_fp, Math::VulnerabilityMitigation(83_fp, 100_fp));
    EXPECT_EQ(10000_fp, Math::VulnerabilityMitigation(10000_fp, 100_fp));
    EXPECT_EQ(200_fp, Math::VulnerabilityMitigation(100_fp, 200_fp));
    EXPECT_EQ(400_fp, Math::VulnerabilityMitigation(100_fp, 400_fp));
    EXPECT_EQ(1000_fp, Math::VulnerabilityMitigation(100_fp, 1000_fp));
    EXPECT_EQ(10000_fp, Math::VulnerabilityMitigation(100_fp, 10000_fp));
    EXPECT_EQ("5083.640"_fp, Math::VulnerabilityMitigation(6689_fp, 76_fp));

    EXPECT_EQ(650_fp, Math::VulnerabilityMitigation(1300_fp, 50_fp));
    EXPECT_EQ(90_fp, Math::VulnerabilityMitigation(900_fp, 10_fp));
    EXPECT_EQ(89_fp, Math::VulnerabilityMitigation(890_fp, 10_fp));

    EXPECT_EQ(50000_fp, Math::VulnerabilityMitigation(100'000_fp, 50_fp));
    EXPECT_EQ(1000000_fp, Math::VulnerabilityMitigation(1'000'000_fp, 100_fp));
}

TEST(Math, Sqrt)
{
    // Integer square root for numbers 0 to 65:
    const std::array<int64_t, 66> expected_results = {0, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4,
                                                      4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6,
                                                      6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8};

    for (std::size_t number = 0; number < expected_results.size(); number++)
    {
        EXPECT_EQ(expected_results[number], Math::Sqrt(static_cast<int64_t>(number)));
    }

    EXPECT_EQ(static_cast<int64_t>(8), Math::Sqrt(64));
    EXPECT_EQ(static_cast<int64_t>(8), Math::Sqrt(70));
    EXPECT_EQ(static_cast<int64_t>(10), Math::Sqrt(100));
    EXPECT_EQ(static_cast<int64_t>(307), Math::Sqrt(94306));
    EXPECT_EQ(static_cast<int64_t>(3162), Math::Sqrt(10000000));
}

TEST(Math, Square)
{
    EXPECT_EQ(0, Math::Square(0));
    EXPECT_EQ(0, Math::Square(-0));
    EXPECT_EQ(1, Math::Square(-1));
    EXPECT_EQ(1, Math::Square(-1));
    EXPECT_EQ(16, Math::Square(4));
    EXPECT_EQ(16, Math::Square(-4));
    EXPECT_EQ(65536, Math::Square(256));
    EXPECT_EQ(65536, Math::Square(-256));
    EXPECT_EQ(999999998000000001, Math::Square(-999999999));
    EXPECT_EQ(999999998000000001, Math::Square(999999999));
}

TEST(Math, RadianTurnsToDegrees)
{
    ASSERT_EQ(Math::kOneDegreeRadianTurns, 182);

    EXPECT_EQ(0, Math::RadianTurnsToDegrees(static_cast<uint16_t>(-1)));
    EXPECT_EQ(0, Math::RadianTurnsToDegrees(0));
    EXPECT_EQ(0, Math::RadianTurnsToDegrees(1));
    EXPECT_EQ(0, Math::RadianTurnsToDegrees(2));
    EXPECT_EQ(0, Math::RadianTurnsToDegrees(4));
    EXPECT_EQ(0, Math::RadianTurnsToDegrees(8));
    EXPECT_EQ(0, Math::RadianTurnsToDegrees(16));
    EXPECT_EQ(0, Math::RadianTurnsToDegrees(32));
    EXPECT_EQ(0, Math::RadianTurnsToDegrees(64));
    EXPECT_EQ(0, Math::RadianTurnsToDegrees(128));
    EXPECT_EQ(1, Math::RadianTurnsToDegrees(182));
    EXPECT_EQ(1, Math::RadianTurnsToDegrees(256));
    EXPECT_EQ(2, Math::RadianTurnsToDegrees(364));
    EXPECT_EQ(2, Math::RadianTurnsToDegrees(512));
    EXPECT_EQ(5, Math::RadianTurnsToDegrees(1024));
    EXPECT_EQ(11, Math::RadianTurnsToDegrees(11 * Math::kOneDegreeRadianTurns));
    EXPECT_EQ(22, Math::RadianTurnsToDegrees(22 * Math::kOneDegreeRadianTurns));
    EXPECT_EQ(45, Math::RadianTurnsToDegrees(45 * Math::kOneDegreeRadianTurns));
    EXPECT_EQ(90, Math::RadianTurnsToDegrees(90 * Math::kOneDegreeRadianTurns));
    EXPECT_EQ(120, Math::RadianTurnsToDegrees(120 * Math::kOneDegreeRadianTurns));
    EXPECT_EQ(180, Math::RadianTurnsToDegrees(180 * Math::kOneDegreeRadianTurns));
    EXPECT_EQ(181, Math::RadianTurnsToDegrees(181 * Math::kOneDegreeRadianTurns));
    EXPECT_EQ(270, Math::RadianTurnsToDegrees(270 * Math::kOneDegreeRadianTurns));

    // NO Wraps around
    EXPECT_EQ(300, Math::RadianTurnsToDegrees(300 * Math::kOneDegreeRadianTurns));
    EXPECT_EQ(359, Math::RadianTurnsToDegrees(359 * Math::kOneDegreeRadianTurns));

    // Full circle
    EXPECT_EQ(0, Math::RadianTurnsToDegrees(360 * Math::kOneDegreeRadianTurns));

    // Check all range
    {
        uint16_t angle = Math::kOneDegreeRadianTurns;
        for (int degrees_expected = 1; degrees_expected < 360; degrees_expected++)
        {
            ASSERT_EQ(degrees_expected, Math::RadianTurnsToDegrees(angle));
            angle += Math::kOneDegreeRadianTurns;
        }
    }
}

TEST(Math, AngleDifference180)
{
    EXPECT_EQ(0, Math::AngleDifference180(0, 0));
    EXPECT_EQ(45, Math::AngleDifference180(0, 45));
    EXPECT_EQ(0, Math::AngleDifference180(-180, 180));
    EXPECT_EQ(180, Math::AngleDifference180(-90, 90));
    EXPECT_EQ(90, Math::AngleDifference180(-45, 45));
    EXPECT_EQ(30, Math::AngleDifference180(60, 90));
    EXPECT_EQ(150, Math::AngleDifference180(120, 270));
    EXPECT_EQ(-45, Math::AngleDifference180(0, -45));
    EXPECT_EQ(-180, Math::AngleDifference180(90, -90));
    EXPECT_EQ(-30, Math::AngleDifference180(90, 60));
    EXPECT_EQ(-180, Math::AngleDifference180(270, 90));

    // Over 180 difference
    EXPECT_EQ(30, Math::AngleDifference180(360 + 60, 360 + 90));
}

TEST(Math, ATan2)
{
    EXPECT_EQ(Math::kOneDegreeRadianTurns, 182);

    EXPECT_EQ(0, Math::ATan2(0, 0));
    EXPECT_EQ(0, Math::ATan2(0, 5));
    EXPECT_EQ(45 * Math::kOneDegreeRadianTurns + 2, Math::ATan2(2, 2));
    EXPECT_EQ(225 * Math::kOneDegreeRadianTurns + 10, Math::ATan2(-2, -2));

    // EXPECT_EQ(320 * Math::kOneDegreeRadianTurns + 6, Math::ATan2(-1, 1));

    // clang-format off
    // Compare some arbitrary angles
    // Maps from test calculation to expected angle
    const std::unordered_map<uint16_t, uint16_t> range_check_map = {
        // 1st quadrant
        {Math::ATan2(1, 7), static_cast<uint16_t>(8)},
        {Math::ATan2(1, 5), static_cast<uint16_t>(11)},
        {Math::ATan2(2, 5), static_cast<uint16_t>(21)},
        {Math::ATan2(2, 3), static_cast<uint16_t>(33)},
        {Math::ATan2(4, 5), static_cast<uint16_t>(38)},
        {Math::ATan2(4, 3), static_cast<uint16_t>(53)},
        {Math::ATan2(3, 1), static_cast<uint16_t>(71)},
        {Math::ATan2(4, 1), static_cast<uint16_t>(75)},

        // 2nd quadrant
        {Math::ATan2(30, -3), static_cast<uint16_t>(95)},
        {Math::ATan2(20, -1), static_cast<uint16_t>(93)},
        {Math::ATan2(15, -3), static_cast<uint16_t>(101)},
        {Math::ATan2(20, -5), static_cast<uint16_t>(104)},
        {Math::ATan2(15, -5), static_cast<uint16_t>(108)},
        {Math::ATan2(3, -2), static_cast<uint16_t>(123)},
        {Math::ATan2(3, -7), static_cast<uint16_t>(156)},

        // 3rd quadrant
        {Math::ATan2(-3, -7), static_cast<uint16_t>(204)},
        {Math::ATan2(-3, -2), static_cast<uint16_t>(237)},
        {Math::ATan2(-15, -5), static_cast<uint16_t>(252)},
        {Math::ATan2(-20, -5), static_cast<uint16_t>(256)},
        {Math::ATan2(-15, -3), static_cast<uint16_t>(259)},
        {Math::ATan2(-30, -3), static_cast<uint16_t>(265)},
        {Math::ATan2(-20, -1), static_cast<uint16_t>(267)},

        // 4th quadrant
        {Math::ATan2(-7, 3), static_cast<uint16_t>(293)},
        {Math::ATan2(-2, 3), static_cast<uint16_t>(326)},
        {Math::ATan2(-5, 15), static_cast<uint16_t>(341)},
        {Math::ATan2(-5, 20), static_cast<uint16_t>(345)},
        {Math::ATan2(-3, 15), static_cast<uint16_t>(348)},
        {Math::ATan2(-3, 30), static_cast<uint16_t>(354)},
        {Math::ATan2(-1, 20), static_cast<uint16_t>(357)}
    };
    // clang-format on

    // Make sure result is within 1 degree of actual value
    for (const auto& [radians, expected_angle] : range_check_map)
    {
        const uint16_t expected_radians = expected_angle * Math::kOneDegreeRadianTurns;
        const int abs_diff = Math::Abs(radians - expected_radians);
        EXPECT_LT(abs_diff, Math::kOneDegreeRadianTurns)
            << "Expression not within acceptable range, abs_diff = " << abs_diff << ", radians = " << radians
            << ", expected_angle = " << expected_angle << ", expected_radians " << expected_radians;
    }

    // Go around the circle
    // std::unordered_set<uint16_t> seen_radian_turns;
    // const int limit = 100;
    // std::cout << "ATan2:" << std::endl;
    // for (int x = -limit; x <= limit; x++)
    // {
    //     for (int y = -limit; y <= limit; y++)
    //     {
    //         const int radians = Math::ATan2(y, x);
    //         // std::cout << "x = " << x << ", y = " << y << ", radians = " << radians <<
    //         std::endl; seen_radian_turns.insert(radians);
    //     }
    // }

    // std::cout << "seen_radian_turns.size() = " << seen_radian_turns.size() << std::endl;
    // ASSERT_GE(seen_radian_turns.size(), 65000);

    // std::vector<uint16_t> seen_radian_turns_vector;
    // for (const int radians : seen_radian_turns)
    // {
    //     seen_radian_turns_vector.push_back(radians);
    // }

    // std::sort(seen_radian_turns_vector.begin(), seen_radian_turns_vector.end(),
    // std::less<uint16_t>()); for (const int radians : seen_radian_turns_vector)
    // {
    //     std::cout << radians << " ";
    // }
    // std::cout << std::endl;
}

TEST(Math, CalculateEnergyGain)
{
    EXPECT_EQ(0_fp, Math::CalculateEnergyGain(0_fp, 0_fp));
    EXPECT_EQ(0_fp, Math::CalculateEnergyGain(0_fp, 20_fp));

    EXPECT_EQ(0_fp, Math::CalculateEnergyGain(10_fp, 0_fp));
    EXPECT_EQ(200_fp, Math::CalculateEnergyGain(10_fp, 5_fp));
    EXPECT_EQ(50_fp, Math::CalculateEnergyGain(10_fp, 20_fp));
    EXPECT_EQ(10_fp, Math::CalculateEnergyGain(10_fp, 100_fp));

    EXPECT_EQ(0_fp, Math::CalculateEnergyGain(20_fp, 0_fp));
    EXPECT_EQ(400_fp, Math::CalculateEnergyGain(20_fp, 5_fp));
    EXPECT_EQ(100_fp, Math::CalculateEnergyGain(20_fp, 20_fp));
    EXPECT_EQ(20_fp, Math::CalculateEnergyGain(20_fp, 100_fp));
}

TEST(Math, CastWillOverflow)
{
    // To smaller type (signed)
    EXPECT_EQ(Math::CastWillOverflow<int8_t>(int32_t(127)), false);
    EXPECT_EQ(Math::CastWillOverflow<int8_t>(int32_t(128)), true);
    EXPECT_EQ(Math::CastWillOverflow<int8_t>(int32_t(-128)), false);
    EXPECT_EQ(Math::CastWillOverflow<int8_t>(int32_t(-129)), true);

    // To bigger type (signed)
    EXPECT_EQ(Math::CastWillOverflow<int16_t>(int8_t(127)), false);
    EXPECT_EQ(Math::CastWillOverflow<int16_t>(int8_t(-128)), false);

    // To smaller type (unsigned)
    EXPECT_EQ(Math::CastWillOverflow<uint8_t>(uint32_t(255)), false);
    EXPECT_EQ(Math::CastWillOverflow<uint8_t>(uint32_t(256)), true);

    // To bigger type (unsigned)
    EXPECT_EQ(Math::CastWillOverflow<uint16_t>(uint8_t(255)), false);
}

TEST(Math, CalculateEnergyGainOnTakeDamage)
{
    EXPECT_EQ("82.500"_fp, Math::CalculateEnergyGainOnTakeDamage(50_fp, 33_fp, 20_fp));
    EXPECT_EQ("86.842"_fp, Math::CalculateEnergyGainOnTakeDamage(50_fp, 33_fp, 19_fp));

    for (FixedPoint health_lost = 0_fp; health_lost <= 100_fp; health_lost += 1_fp)
    {
        EXPECT_EQ(health_lost, Math::CalculateEnergyGainOnTakeDamage(health_lost, 50_fp, 50_fp))
            << " health_lost = " << health_lost;
    }

    EXPECT_EQ(0_fp, Math::CalculateEnergyGainOnTakeDamage(0_fp, 0_fp, 0_fp));
    EXPECT_EQ(0_fp, Math::CalculateEnergyGainOnTakeDamage(0_fp, 50_fp, 0_fp));
    EXPECT_EQ(0_fp, Math::CalculateEnergyGainOnTakeDamage(50_fp, 0_fp, 0_fp));
    EXPECT_EQ(0_fp, Math::CalculateEnergyGainOnTakeDamage(0_fp, 0_fp, 50_fp));
    EXPECT_EQ(0_fp, Math::CalculateEnergyGainOnTakeDamage(50_fp, 0_fp, 50_fp));
    EXPECT_EQ(0_fp, Math::CalculateEnergyGainOnTakeDamage(50_fp, 50_fp, 0_fp));
    EXPECT_EQ(0_fp, Math::CalculateEnergyGainOnTakeDamage(0_fp, 50_fp, 50_fp));

    EXPECT_EQ("999980000.1"_fp, Math::CalculateEnergyGainOnTakeDamage(99'999_fp, 99'999_fp, 10_fp));
}

TEST(Math, UnitsToSubUnits)
{
    EXPECT_EQ(0, Math::UnitsToSubUnits(0));
    EXPECT_EQ(1 * kSubUnitsPerUnit, Math::UnitsToSubUnits(1));
    EXPECT_EQ(30 * kSubUnitsPerUnit, Math::UnitsToSubUnits(30));
}

TEST(Math, SubUnitsToUnits)
{
    EXPECT_EQ(0, Math::SubUnitsToUnits(0));
    EXPECT_EQ(0, Math::SubUnitsToUnits(500));
    EXPECT_EQ(0, Math::SubUnitsToUnits(999));
    EXPECT_EQ(1, Math::SubUnitsToUnits(1001));
    EXPECT_EQ(1, Math::SubUnitsToUnits(1999));
    EXPECT_EQ(2, Math::SubUnitsToUnits(2000));
    EXPECT_EQ(1, Math::SubUnitsToUnits(1100));
    EXPECT_EQ(1, Math::SubUnitsToUnits(1500));
    EXPECT_EQ(1, Math::SubUnitsToUnits(1900));
    EXPECT_EQ(10, Math::SubUnitsToUnits(10000));
    EXPECT_EQ(30, Math::SubUnitsToUnits(30500));
}

TEST(Math, SinScaled)
{
    EXPECT_EQ(0, Math::SinScaled(0));
    EXPECT_EQ(0, Math::SinScaled(180));
    EXPECT_EQ(0, Math::SinScaled(-180));
    EXPECT_EQ(0, Math::SinScaled(360));
    EXPECT_EQ(0, Math::SinScaled(720));

    EXPECT_EQ(500, Math::SinScaled(30));
    EXPECT_EQ(707, Math::SinScaled(45));
    EXPECT_EQ(970, Math::SinScaled(76));
    EXPECT_EQ(-829, Math::SinScaled(-124));

    EXPECT_EQ(1000, Math::SinScaled(-270));
    EXPECT_EQ(1000, Math::SinScaled(90));
    EXPECT_EQ(-1000, Math::SinScaled(270));
}

TEST(Math, CosScaled)
{
    EXPECT_EQ(0, Math::CosScaled(-270));
    EXPECT_EQ(0, Math::CosScaled(90));
    EXPECT_EQ(0, Math::CosScaled(270));

    EXPECT_EQ(866, Math::CosScaled(30));
    EXPECT_EQ(707, Math::CosScaled(45));
    EXPECT_EQ(241, Math::CosScaled(76));
    EXPECT_EQ(-559, Math::CosScaled(-124));

    EXPECT_EQ(1000, Math::CosScaled(0));
    EXPECT_EQ(-1000, Math::CosScaled(180));
    EXPECT_EQ(-1000, Math::CosScaled(-180));
    EXPECT_EQ(1000, Math::CosScaled(360));
    EXPECT_EQ(1000, Math::CosScaled(720));
}

TEST(Math, RotatePointX)
{
    EXPECT_EQ(0, Math::RotatePointX(0, 0, 90));

    EXPECT_EQ(-1, Math::RotatePointX(1, 1, 90));
    EXPECT_EQ(1, Math::RotatePointX(1, 1, 0));
    EXPECT_EQ(-6, Math::RotatePointX(1, 10, 45));
    EXPECT_EQ(-7, Math::RotatePointX(10, 1, 135));
    EXPECT_EQ(2, Math::RotatePointX(1, 4, -30));
}

TEST(Math, RotatePointY)
{
    EXPECT_EQ(0, Math::RotatePointY(0, 0, 90));
    EXPECT_EQ(1, Math::RotatePointY(1, 1, 90));
    EXPECT_EQ(1, Math::RotatePointY(1, 1, 0));
    EXPECT_EQ(7, Math::RotatePointY(1, 10, 45));
    EXPECT_EQ(6, Math::RotatePointY(10, 1, 135));
    EXPECT_EQ(1, Math::RotatePointY(1, 4, -60));
}

TEST(Math, AngleLimitTo360)
{
    EXPECT_EQ(0, Math::AngleLimitTo360(0));
    EXPECT_EQ(0, Math::AngleLimitTo360(-360));
    EXPECT_EQ(0, Math::AngleLimitTo360(360));
    EXPECT_EQ(0, Math::AngleLimitTo360(-720));
    EXPECT_EQ(0, Math::AngleLimitTo360(720));

    EXPECT_EQ(30, Math::AngleLimitTo360(30));
    EXPECT_EQ(30, Math::AngleLimitTo360(-330));
    EXPECT_EQ(30, Math::AngleLimitTo360(390));
    EXPECT_EQ(30, Math::AngleLimitTo360(-690));
    EXPECT_EQ(30, Math::AngleLimitTo360(750));

    EXPECT_EQ(90, Math::AngleLimitTo360(90));
    EXPECT_EQ(90, Math::AngleLimitTo360(-270));
    EXPECT_EQ(90, Math::AngleLimitTo360(450));
    EXPECT_EQ(90, Math::AngleLimitTo360(-630));
    EXPECT_EQ(90, Math::AngleLimitTo360(810));

    EXPECT_EQ(241, Math::AngleLimitTo360(241));
    EXPECT_EQ(241, Math::AngleLimitTo360(-119));
    EXPECT_EQ(241, Math::AngleLimitTo360(601));
    EXPECT_EQ(241, Math::AngleLimitTo360(-479));
    EXPECT_EQ(241, Math::AngleLimitTo360(961));
}

TEST(Math, FractionalCeil)
{
    EXPECT_EQ(1, Math::FractionalCeil(1000, 1000));
    EXPECT_EQ(4, Math::FractionalCeil(1000, 300));
    EXPECT_EQ(4, Math::FractionalCeil(2000, 600));

    EXPECT_EQ(10, Math::FractionalCeil(1000, 100));
    EXPECT_EQ(34, Math::FractionalCeil(1000, 30));
    EXPECT_EQ(34, Math::FractionalCeil(2000, 60));

    EXPECT_EQ(-1, Math::FractionalCeil(-1000, 1000));
    EXPECT_EQ(-3, Math::FractionalCeil(-1000, 300));
    EXPECT_EQ(-3, Math::FractionalCeil(-2000, 600));

    EXPECT_EQ(-10, Math::FractionalCeil(-1000, 100));
    EXPECT_EQ(-33, Math::FractionalCeil(-1000, 30));
    EXPECT_EQ(-33, Math::FractionalCeil(-2000, 60));

    EXPECT_EQ(-1, Math::FractionalCeil(-1000, 1000));
    EXPECT_EQ(-3, Math::FractionalCeil(-1000, 300));
    EXPECT_EQ(-3, Math::FractionalCeil(-2000, 600));

    EXPECT_EQ(-10, Math::FractionalCeil(1000, -100));
    EXPECT_EQ(-33, Math::FractionalCeil(1000, -30));
    EXPECT_EQ(-33, Math::FractionalCeil(2000, -60));
}

TEST(Math, FractionalRound)
{
    EXPECT_EQ(1, Math::FractionalRound(1000, 1000));
    EXPECT_EQ(3, Math::FractionalRound(1000, 300));
    EXPECT_EQ(3, Math::FractionalRound(2000, 600));

    EXPECT_EQ(10, Math::FractionalRound(1000, 100));
    EXPECT_EQ(33, Math::FractionalRound(1000, 30));
    EXPECT_EQ(33, Math::FractionalRound(2000, 60));

    EXPECT_EQ(-1, Math::FractionalRound(-1000, 1000));
    EXPECT_EQ(-3, Math::FractionalRound(-1000, 300));
    EXPECT_EQ(-3, Math::FractionalRound(-2000, 600));

    EXPECT_EQ(-10, Math::FractionalRound(-1000, 100));
    EXPECT_EQ(-33, Math::FractionalRound(-1000, 30));
    EXPECT_EQ(-33, Math::FractionalRound(-2000, 60));

    EXPECT_EQ(-1, Math::FractionalRound(-1000, 1000));
    EXPECT_EQ(-3, Math::FractionalRound(-1000, 300));
    EXPECT_EQ(-3, Math::FractionalRound(-2000, 600));

    EXPECT_EQ(-10, Math::FractionalRound(1000, -100));
    EXPECT_EQ(-33, Math::FractionalRound(1000, -30));
    EXPECT_EQ(-33, Math::FractionalRound(2000, -60));
}

TEST(Math, CalculateStatGrowth)
{
    // No growth percentage
    {
        constexpr LevelingHelper leveling_helper{.growth_rate_percentage = 0_fp, .stat_scale = 1_fp};

        // Should not change value as has grow rate 0
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100000_fp, 60), 100000_fp);
    }

    // Without stat scaling
    {
        constexpr LevelingHelper leveling_helper{.growth_rate_percentage = 5_fp, .stat_scale = 1_fp};

        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100000_fp, 0), 100000_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100000_fp, 1), 100000_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100000_fp, 30), 102458_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100000_fp, 60), 105000_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100000_fp, 120), 105000_fp);

        // Edge case
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100000_fp, -1), 100000_fp);
    }

    // Example from here https://illuvium.atlassian.net/wiki/spaces/AB/pages/44204724/Illuvial+Combat+Stat+Growth
    {
        constexpr LevelingHelper leveling_helper{.growth_rate_percentage = 5_fp, .stat_scale = 1_fp};

        // Health
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(500_fp, 30), 512_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(500_fp, 60), 525_fp);

        // Attack damage
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(80_fp, 30), 82_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(80_fp, 60), 84_fp);

        // Omega power
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100_fp, 30), 102_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100_fp, 53), 104_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100_fp, 60), 105_fp);
    }

    {
        constexpr LevelingHelper leveling_helper{.growth_rate_percentage = 25_fp, .stat_scale = 1_fp};

        // Omega power
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100_fp, 30), 112_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100_fp, 53), 122_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100_fp, 60), 125_fp);

        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100_fp, 60), 125_fp);
    }

    // Simulates unreal
    {
        constexpr LevelingHelper leveling_helper{.growth_rate_percentage = 25_fp, .stat_scale = 1000_fp};

        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100000_fp, 60), 125000_fp);

        EXPECT_EQ(leveling_helper.CalculateStatGrowth(1250000_fp, 53), 1525424_fp);

        // Omega power
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100_fp, 30), 112_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100_fp, 53), 122_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100_fp, 60), 125_fp);
    }

    // With stat scaling
    {
        constexpr LevelingHelper leveling_helper{.growth_rate_percentage = 5_fp, .stat_scale = 1000_fp};

        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100000_fp, 0), 100000_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100000_fp, 1), 100000_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100000_fp, 30), 102458_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100000_fp, 60), 105000_fp);
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100000_fp, 120), 105000_fp);

        // Edge case
        EXPECT_EQ(leveling_helper.CalculateStatGrowth(100000_fp, -1), 100000_fp);
    }
}

TEST(Math, TriangleArea)
{
    EXPECT_EQ(Math::TriangleArea(0, 0, 0, 0, 0, 0), 0);
    EXPECT_EQ(Math::TriangleArea(1, 0, 0, 1, 1, 1), 0);
    EXPECT_EQ(Math::TriangleArea(10, 0, 0, 10, 10, 10), 50);
    EXPECT_EQ(Math::TriangleArea(100, 0, 0, 100, 100, 100), 5000);
    EXPECT_EQ(Math::TriangleArea(0, -1, -1, 0, -1, -1), 0);
    EXPECT_EQ(Math::TriangleArea(0, -10, -10, 0, -10, -10), 50);
    EXPECT_EQ(Math::TriangleArea(0, -100, -100, 0, -100, -100), 5000);
    EXPECT_EQ(Math::TriangleArea(287, -462, 109, 41, 914, -217), 179495);
    EXPECT_EQ(Math::TriangleArea(-512, 218, 742, -811, -410, 926), 496395);
    EXPECT_EQ(
        Math::TriangleArea(-214748368, -214748368, 214748367, 214748367, -214748368, 214748367),
        92233722687830112);
}

TEST(Math, Round)
{
    EXPECT_EQ(Math::Round("42.501"_fp), 43_fp);
    EXPECT_EQ(Math::Round("42.500"_fp), 43_fp);
    EXPECT_EQ(Math::Round("42.499"_fp), 42_fp);
    EXPECT_EQ(Math::Round("0.501"_fp), 1_fp);
    EXPECT_EQ(Math::Round("0.500"_fp), 1_fp);
    EXPECT_EQ(Math::Round("0.499"_fp), 0_fp);
    EXPECT_EQ(Math::Round("0"_fp), 0_fp);
    EXPECT_EQ(Math::Round(-"0.499"_fp), 0_fp);
    EXPECT_EQ(Math::Round(-"0.500"_fp), -1_fp);
    EXPECT_EQ(Math::Round(-"0.501"_fp), -1_fp);
    EXPECT_EQ(Math::Round(-"42.499"_fp), -42_fp);
    EXPECT_EQ(Math::Round(-"42.500"_fp), -43_fp);
    EXPECT_EQ(Math::Round(-"42.501"_fp), -43_fp);
    EXPECT_EQ(Math::Round("9223372036854775.4"_fp), 9223372036854775_fp);
    EXPECT_EQ(Math::Round(-"9223372036854775.4"_fp), -9223372036854775_fp);
}

}  // namespace simulation
