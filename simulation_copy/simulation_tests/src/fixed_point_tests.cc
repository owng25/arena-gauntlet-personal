#include "gtest/gtest.h"
#include "utility/fixed_point.h"

namespace simulation
{

static_assert("10.0"_fp == 10_fp);
static_assert("10.1"_fp == 101_fp / 10_fp);
static_assert("10.12"_fp == 1012_fp / 100_fp);
static_assert("10.123"_fp == 10123_fp / 1000_fp);

TEST(FixedPoint, PercentageOf)
{
    EXPECT_EQ(0_fp, (0_fp).AsPercentageOf(0_fp));
    EXPECT_EQ("0.5"_fp, (5_fp).AsPercentageOf(10_fp));
    EXPECT_EQ(1_fp, (10_fp).AsPercentageOf(10_fp));
    EXPECT_EQ(10_fp, (10_fp).AsPercentageOf(100_fp));
    EXPECT_EQ(25_fp, (25_fp).AsPercentageOf(100_fp));
    EXPECT_EQ(50_fp, (50_fp).AsPercentageOf(100_fp));
    EXPECT_EQ(100_fp, (100_fp).AsPercentageOf(100_fp));
    EXPECT_EQ(1234_fp, (100_fp).AsPercentageOf(1234_fp));
    EXPECT_EQ(750_fp, (75_fp).AsPercentageOf(1000_fp));
    EXPECT_EQ(100_fp, (20_fp).AsPercentageOf(500_fp));
    EXPECT_EQ(-50_fp, (50_fp).AsPercentageOf(-100_fp));

    EXPECT_EQ(50_fp, (500_fp).AsPercentageOf(10_fp));
    EXPECT_EQ(51_fp, (510_fp).AsPercentageOf(10_fp));
    EXPECT_EQ(60_fp, (600_fp).AsPercentageOf(10_fp));

    EXPECT_EQ(7500000_fp, (75_fp).AsPercentageOf(10000000_fp));
    EXPECT_EQ(6600000_fp, (33_fp).AsPercentageOf(20'000'000_fp));

    EXPECT_EQ("503923421.5"_fp, (25_fp).AsPercentageOf(2'015'693'686_fp));

    EXPECT_EQ(1_fp, (100_fp).AsPercentageOf(1_fp));
    EXPECT_EQ(2_fp, (100_fp).AsPercentageOf(2_fp));
}

TEST(FixedPoint, AsHighPrecisionPercentageOf)
{
    EXPECT_EQ(0_fp, (0_fp).AsHighPrecisionPercentageOf(0_fp));

    EXPECT_EQ("0.5"_fp, (500_fp).AsHighPrecisionPercentageOf(10_fp));
    EXPECT_EQ(1_fp, (1000_fp).AsHighPrecisionPercentageOf(10_fp));
    EXPECT_EQ(10_fp, (1000_fp).AsHighPrecisionPercentageOf(100_fp));
    EXPECT_EQ(25_fp, (2500_fp).AsHighPrecisionPercentageOf(100_fp));
    EXPECT_EQ(50_fp, (5000_fp).AsHighPrecisionPercentageOf(100_fp));
    EXPECT_EQ(100_fp, (10000_fp).AsHighPrecisionPercentageOf(100_fp));
    EXPECT_EQ(1234_fp, (10000_fp).AsHighPrecisionPercentageOf(1234_fp));
    EXPECT_EQ(750_fp, (7500_fp).AsHighPrecisionPercentageOf(1000_fp));
    EXPECT_EQ(100_fp, (2000_fp).AsHighPrecisionPercentageOf(500_fp));
    EXPECT_EQ(-50_fp, (5000_fp).AsHighPrecisionPercentageOf(-100_fp));
    EXPECT_EQ("6.25"_fp, (625_fp).AsHighPrecisionPercentageOf(100_fp));

    EXPECT_EQ(-"3.12"_fp, (-312_fp).AsHighPrecisionPercentageOf(100_fp));
    EXPECT_EQ(-3120_fp, (-312_fp).AsHighPrecisionPercentageOf(100000_fp));

    EXPECT_EQ(2500_fp, (125_fp).AsHighPrecisionPercentageOf(200000_fp));
    static_assert((125_fp).AsHighPrecisionPercentageOf(200'000_fp) == 2500_fp);

    EXPECT_EQ(625000_fp, (625_fp).AsHighPrecisionPercentageOf(10'000'000_fp));
    EXPECT_EQ(50000000_fp, (5000_fp).AsHighPrecisionPercentageOf(100'000'000_fp));
}

TEST(FixedPoint, AsProportionalPercentageOf)
{
    EXPECT_EQ(0_fp, (0_fp).AsProportionalPercentageOf(10_fp));
    EXPECT_EQ(50_fp, (50_fp).AsProportionalPercentageOf(100_fp));
    EXPECT_EQ(10_fp, (100_fp).AsProportionalPercentageOf(1000_fp));
    EXPECT_EQ(150_fp, (150_fp).AsProportionalPercentageOf(100_fp));
    EXPECT_EQ(500_fp, (5000_fp).AsProportionalPercentageOf(1000_fp));
    EXPECT_EQ(33_fp, (67_fp).AsProportionalPercentageOf(200_fp));
    EXPECT_EQ(48_fp, (12_fp).AsProportionalPercentageOf(25_fp));
    EXPECT_EQ(33_fp, (20_fp).AsProportionalPercentageOf(60_fp));
    EXPECT_EQ(20_fp, (20_fp).AsProportionalPercentageOf(100_fp));
    EXPECT_EQ(-500_fp, (-5000_fp).AsProportionalPercentageOf(1000_fp));
    EXPECT_EQ(0_fp, (3_fp).AsProportionalPercentageOf(400_fp));

    EXPECT_EQ(10_fp, (100'000_fp).AsProportionalPercentageOf(1'000'000_fp));
    EXPECT_EQ(1_fp, (100'000_fp).AsProportionalPercentageOf(10'000'000_fp));
    EXPECT_EQ(10_fp, (1'000'000_fp).AsProportionalPercentageOf(10'000'000_fp));

    EXPECT_EQ(50_fp, (1'000'000'000_fp).AsProportionalPercentageOf(2'000'000'000_fp));
}

TEST(FixedPoint, PrintWithFmtLib)
{
    auto to_string = [](FixedPoint value) -> std::string
    {
        return fmt::format("{}", value);
    };

    EXPECT_EQ(to_string(123_fp), "123");
    EXPECT_EQ(to_string("123.0"_fp), "123");
    EXPECT_EQ(to_string("123.4"_fp), "123.400");
    EXPECT_EQ(to_string("123.45"_fp), "123.450");
    EXPECT_EQ(to_string("123.456"_fp), "123.456");
    EXPECT_EQ(to_string("123.010"_fp), "123.010");
}

TEST(FixedPoint, FormatWithExactDigits)
{
    auto to_string = [](FixedPoint value) -> std::string
    {
        return fmt::format("{:2}", value);
    };
    EXPECT_EQ(to_string(123_fp), "123.00");
    EXPECT_EQ(to_string("123.0"_fp), "123.00");
    EXPECT_EQ(to_string("123.4"_fp), "123.40");
    EXPECT_EQ(to_string("123.45"_fp), "123.45");
    EXPECT_EQ(to_string("123.456"_fp), "123.45");
    EXPECT_EQ(to_string("123.010"_fp), "123.01");
}

}  // namespace simulation
