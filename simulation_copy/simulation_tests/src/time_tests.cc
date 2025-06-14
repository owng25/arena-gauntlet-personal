#include <climits>

#include "gtest/gtest.h"
#include "utility/time.h"

namespace simulation
{
TEST(Time, MsToTimeSteps)
{
    EXPECT_EQ(kTimeInfinite, Time::MsToTimeSteps(kTimeInfinite));
    EXPECT_EQ(0, Time::MsToTimeSteps(50));
    EXPECT_EQ(1, Time::MsToTimeSteps(100));
    EXPECT_EQ(5, Time::MsToTimeSteps(500));
    EXPECT_EQ(10, Time::MsToTimeSteps(1000));
    EXPECT_EQ(15, Time::MsToTimeSteps(1500));
    EXPECT_EQ(20, Time::MsToTimeSteps(2000));
    EXPECT_EQ(20, Time::MsToTimeSteps(2050));
}

TEST(Time, TimeStepsToMs)
{
    EXPECT_EQ(kTimeInfinite, Time::TimeStepsToMs(kTimeInfinite));
    EXPECT_EQ(0, Time::TimeStepsToMs(0));
    EXPECT_EQ(100, Time::TimeStepsToMs(1));
    EXPECT_EQ(500, Time::TimeStepsToMs(5));
    EXPECT_EQ(1000, Time::TimeStepsToMs(10));
    EXPECT_EQ(1500, Time::TimeStepsToMs(15));
    EXPECT_EQ(2000, Time::TimeStepsToMs(20));
    EXPECT_EQ(2300, Time::TimeStepsToMs(23));
}

TEST(Time, TimeSeconds)
{
    EXPECT_EQ(kTimeInfinite, Time::TimeStepsToSeconds(kTimeInfinite));
    EXPECT_EQ(0, Time::TimeStepsToSeconds(0));
    EXPECT_EQ(0, Time::TimeStepsToSeconds(1));
    EXPECT_EQ(0, Time::TimeStepsToSeconds(9));
    EXPECT_EQ(1, Time::TimeStepsToSeconds(10));
    EXPECT_EQ(1, Time::TimeStepsToSeconds(11));
    EXPECT_EQ(20, Time::TimeStepsToSeconds(200));
    EXPECT_EQ(40, Time::TimeStepsToSeconds(400));
    EXPECT_EQ(400, Time::TimeStepsToSeconds(4000));
}

TEST(Time, AttackSpeedToTimeSteps)
{
    EXPECT_EQ(kMaxNumberOfTimeSteps, Time::AttackSpeedToTimeSteps(0));

    EXPECT_EQ(100, Time::AttackSpeedToTimeSteps(10));
    EXPECT_EQ(20, Time::AttackSpeedToTimeSteps(50));
    EXPECT_EQ(16, Time::AttackSpeedToTimeSteps(60));
    EXPECT_EQ(16, Time::AttackSpeedToTimeSteps(62));
    EXPECT_EQ(13, Time::AttackSpeedToTimeSteps(75));
    EXPECT_EQ(13, Time::AttackSpeedToTimeSteps(76));
    EXPECT_EQ(11, Time::AttackSpeedToTimeSteps(85));
    EXPECT_EQ(11, Time::AttackSpeedToTimeSteps(90));
    EXPECT_EQ(10, Time::AttackSpeedToTimeSteps(100));
    EXPECT_EQ(8, Time::AttackSpeedToTimeSteps(120));
    EXPECT_EQ(8, Time::AttackSpeedToTimeSteps(125));
    EXPECT_EQ(6, Time::AttackSpeedToTimeSteps(150));
    EXPECT_EQ(6, Time::AttackSpeedToTimeSteps(166));
    EXPECT_EQ(5, Time::AttackSpeedToTimeSteps(200));
    EXPECT_EQ(3, Time::AttackSpeedToTimeSteps(300));
    EXPECT_EQ(3, Time::AttackSpeedToTimeSteps(333));
    EXPECT_EQ(2, Time::AttackSpeedToTimeSteps(400));
    EXPECT_EQ(2, Time::AttackSpeedToTimeSteps(500));
    EXPECT_EQ(1, Time::AttackSpeedToTimeSteps(600));
    EXPECT_EQ(1, Time::AttackSpeedToTimeSteps(700));
    EXPECT_EQ(1, Time::AttackSpeedToTimeSteps(800));
    EXPECT_EQ(1, Time::AttackSpeedToTimeSteps(900));
    EXPECT_EQ(1, Time::AttackSpeedToTimeSteps(1000));

    // Should not be capped
    EXPECT_EQ(0, Time::AttackSpeedToTimeSteps(1100));
    EXPECT_EQ(0, Time::AttackSpeedToTimeSteps(4000));
    EXPECT_EQ(0, Time::AttackSpeedToTimeSteps(10001));
}

TEST(Time, TimeStepsToAttackSpeed)
{
    EXPECT_EQ(1000, Time::TimeStepsToAttackSpeed(0));

    EXPECT_EQ(1, Time::TimeStepsToAttackSpeed(1000));
    EXPECT_EQ(10, Time::TimeStepsToAttackSpeed(100));
    EXPECT_EQ(50, Time::TimeStepsToAttackSpeed(20));
    EXPECT_EQ(62, Time::TimeStepsToAttackSpeed(16));
    EXPECT_EQ(76, Time::TimeStepsToAttackSpeed(13));
    EXPECT_EQ(90, Time::TimeStepsToAttackSpeed(11));
    EXPECT_EQ(100, Time::TimeStepsToAttackSpeed(10));
    EXPECT_EQ(125, Time::TimeStepsToAttackSpeed(8));
    EXPECT_EQ(166, Time::TimeStepsToAttackSpeed(6));
    EXPECT_EQ(200, Time::TimeStepsToAttackSpeed(5));
    EXPECT_EQ(333, Time::TimeStepsToAttackSpeed(3));
    EXPECT_EQ(500, Time::TimeStepsToAttackSpeed(2));
    EXPECT_EQ(1000, Time::TimeStepsToAttackSpeed(1));
}

TEST(Time, MsToAttackSpeed)
{
    EXPECT_EQ(1000, Time::MsToAttackSpeed(0));

    EXPECT_EQ(1, Time::MsToAttackSpeed(100000));
    EXPECT_EQ(10, Time::MsToAttackSpeed(10000));
    EXPECT_EQ(50, Time::MsToAttackSpeed(2000));
    EXPECT_EQ(62, Time::MsToAttackSpeed(1600));
    EXPECT_EQ(76, Time::MsToAttackSpeed(1300));
    EXPECT_EQ(90, Time::MsToAttackSpeed(1100));
    EXPECT_EQ(100, Time::MsToAttackSpeed(1000));
    EXPECT_EQ(125, Time::MsToAttackSpeed(800));
    EXPECT_EQ(166, Time::MsToAttackSpeed(600));
    EXPECT_EQ(200, Time::MsToAttackSpeed(500));
    EXPECT_EQ(333, Time::MsToAttackSpeed(300));
    EXPECT_EQ(500, Time::MsToAttackSpeed(200));
    EXPECT_EQ(1000, Time::MsToAttackSpeed(100));
}

TEST(Time, AttackSpeedToMs)
{
    EXPECT_EQ(kMaxNumberOfTimeSteps * kMsPerTimeStep, Time::AttackSpeedToMs(0));

    EXPECT_EQ(10000, Time::AttackSpeedToMs(10));
    EXPECT_EQ(5000, Time::AttackSpeedToMs(20));
    EXPECT_EQ(2000, Time::AttackSpeedToMs(50));
    EXPECT_EQ(1900, Time::AttackSpeedToMs(52));
    EXPECT_EQ(1800, Time::AttackSpeedToMs(55));
    EXPECT_EQ(1700, Time::AttackSpeedToMs(58));
    EXPECT_EQ(1600, Time::AttackSpeedToMs(60));
    EXPECT_EQ(1500, Time::AttackSpeedToMs(65));
    EXPECT_EQ(1400, Time::AttackSpeedToMs(71));
    EXPECT_EQ(1300, Time::AttackSpeedToMs(75));
    EXPECT_EQ(1300, Time::AttackSpeedToMs(76));
    EXPECT_EQ(1200, Time::AttackSpeedToMs(83));
    EXPECT_EQ(1100, Time::AttackSpeedToMs(85));
    EXPECT_EQ(1000, Time::AttackSpeedToMs(100));
    EXPECT_EQ(900, Time::AttackSpeedToMs(105));
    EXPECT_EQ(800, Time::AttackSpeedToMs(120));
    EXPECT_EQ(600, Time::AttackSpeedToMs(150));
    EXPECT_EQ(500, Time::AttackSpeedToMs(200));
    EXPECT_EQ(300, Time::AttackSpeedToMs(300));
    EXPECT_EQ(200, Time::AttackSpeedToMs(400));
    EXPECT_EQ(200, Time::AttackSpeedToMs(500));
    EXPECT_EQ(100, Time::AttackSpeedToMs(600));
    EXPECT_EQ(100, Time::AttackSpeedToMs(700));
    EXPECT_EQ(100, Time::AttackSpeedToMs(800));
    EXPECT_EQ(100, Time::AttackSpeedToMs(900));
    EXPECT_EQ(100, Time::AttackSpeedToMs(1000));

    // Should not be capped
    EXPECT_EQ(0, Time::AttackSpeedToMs(1100));
    EXPECT_EQ(0, Time::AttackSpeedToMs(4000));
    EXPECT_EQ(0, Time::AttackSpeedToMs(10001));
}

TEST(Time, AttackSpeedToMsNoRounding)
{
    EXPECT_EQ(kMaxNumberOfTimeSteps * kMsPerTimeStep, Time::AttackSpeedToMsNoRounding(0));

    EXPECT_EQ(10000, Time::AttackSpeedToMsNoRounding(10));
    EXPECT_EQ(5000, Time::AttackSpeedToMsNoRounding(20));
    EXPECT_EQ(2000, Time::AttackSpeedToMsNoRounding(50));
    EXPECT_EQ(1923, Time::AttackSpeedToMsNoRounding(52));
    EXPECT_EQ(1818, Time::AttackSpeedToMsNoRounding(55));
    EXPECT_EQ(1724, Time::AttackSpeedToMsNoRounding(58));
    EXPECT_EQ(1666, Time::AttackSpeedToMsNoRounding(60));
    EXPECT_EQ(1538, Time::AttackSpeedToMsNoRounding(65));
    EXPECT_EQ(1408, Time::AttackSpeedToMsNoRounding(71));
    EXPECT_EQ(1333, Time::AttackSpeedToMsNoRounding(75));
    EXPECT_EQ(1315, Time::AttackSpeedToMsNoRounding(76));
    EXPECT_EQ(1204, Time::AttackSpeedToMsNoRounding(83));
    EXPECT_EQ(1176, Time::AttackSpeedToMsNoRounding(85));
    EXPECT_EQ(1000, Time::AttackSpeedToMsNoRounding(100));
    EXPECT_EQ(952, Time::AttackSpeedToMsNoRounding(105));
    EXPECT_EQ(833, Time::AttackSpeedToMsNoRounding(120));
    EXPECT_EQ(666, Time::AttackSpeedToMsNoRounding(150));
    EXPECT_EQ(500, Time::AttackSpeedToMsNoRounding(200));
    EXPECT_EQ(333, Time::AttackSpeedToMsNoRounding(300));
    EXPECT_EQ(250, Time::AttackSpeedToMsNoRounding(400));
    EXPECT_EQ(200, Time::AttackSpeedToMsNoRounding(500));
    EXPECT_EQ(166, Time::AttackSpeedToMsNoRounding(600));
    EXPECT_EQ(142, Time::AttackSpeedToMsNoRounding(700));
    EXPECT_EQ(125, Time::AttackSpeedToMsNoRounding(800));
    EXPECT_EQ(111, Time::AttackSpeedToMsNoRounding(900));
    EXPECT_EQ(100, Time::AttackSpeedToMsNoRounding(1000));

    // Should not be capped
    EXPECT_EQ(90, Time::AttackSpeedToMsNoRounding(1100));
    EXPECT_EQ(25, Time::AttackSpeedToMsNoRounding(4000));
    EXPECT_EQ(9, Time::AttackSpeedToMsNoRounding(10001));
}

TEST(Time, SpeedUnitsToTimeSteps)
{
    // Do the maximum
    EXPECT_EQ(10, Time::SpeedUnitsToTimeSteps(0));

    EXPECT_EQ(10, Time::SpeedUnitsToTimeSteps(1));
    EXPECT_EQ(5, Time::SpeedUnitsToTimeSteps(2));
    EXPECT_EQ(3, Time::SpeedUnitsToTimeSteps(3));
    EXPECT_EQ(2, Time::SpeedUnitsToTimeSteps(4));
    EXPECT_EQ(2, Time::SpeedUnitsToTimeSteps(5));

    // >5 speed units can happen in the same time step
    EXPECT_EQ(1, Time::SpeedUnitsToTimeSteps(6));
    EXPECT_EQ(1, Time::SpeedUnitsToTimeSteps(7));
    EXPECT_EQ(1, Time::SpeedUnitsToTimeSteps(8));
    EXPECT_EQ(1, Time::SpeedUnitsToTimeSteps(9));
    EXPECT_EQ(1, Time::SpeedUnitsToTimeSteps(10));
    EXPECT_EQ(1, Time::SpeedUnitsToTimeSteps(11));
    EXPECT_EQ(1, Time::SpeedUnitsToTimeSteps(15));
}

TEST(Time, MsToSubUnitsPerTimeStep)
{
    // Single step
    EXPECT_EQ(250, Time::MsToSubUnitsPerTimeStep(250, 1000 / kTimeStepsPerSecond));
    EXPECT_EQ(1250, Time::MsToSubUnitsPerTimeStep(1250, 1000 / kTimeStepsPerSecond));

    // Multiple steps
    EXPECT_EQ(250, Time::MsToSubUnitsPerTimeStep(1000, 4000 / kTimeStepsPerSecond));
    EXPECT_EQ(333, Time::MsToSubUnitsPerTimeStep(1000, 3000 / kTimeStepsPerSecond));
    EXPECT_EQ(400, Time::MsToSubUnitsPerTimeStep(800, 2000 / kTimeStepsPerSecond));

    // Entire distance by input of zero
    EXPECT_EQ(500, Time::MsToSubUnitsPerTimeStep(500, 0));
    EXPECT_EQ(15000, Time::MsToSubUnitsPerTimeStep(15000, 0));

    // Negatives - not used right now, but we may in the future

    // Single step negatives
    EXPECT_EQ(-250, Time::MsToSubUnitsPerTimeStep(-250, 1000 / kTimeStepsPerSecond));
    EXPECT_EQ(-250, Time::MsToSubUnitsPerTimeStep(250, -1000 / kTimeStepsPerSecond));

    // Multiple steps negatives
    EXPECT_EQ(-250, Time::MsToSubUnitsPerTimeStep(-1000, 4000 / kTimeStepsPerSecond));
    EXPECT_EQ(-250, Time::MsToSubUnitsPerTimeStep(1000, -4000 / kTimeStepsPerSecond));
    EXPECT_EQ(-333, Time::MsToSubUnitsPerTimeStep(-1000, 3000 / kTimeStepsPerSecond));
    EXPECT_EQ(-333, Time::MsToSubUnitsPerTimeStep(1000, -3000 / kTimeStepsPerSecond));
    EXPECT_EQ(-400, Time::MsToSubUnitsPerTimeStep(-800, 2000 / kTimeStepsPerSecond));
    EXPECT_EQ(-400, Time::MsToSubUnitsPerTimeStep(800, -2000 / kTimeStepsPerSecond));

    // Entire distance negative by input of zero
    EXPECT_EQ(-500, Time::MsToSubUnitsPerTimeStep(-500, 0));

    // Double negatives for possible future use

    // Single step
    EXPECT_EQ(250, Time::MsToSubUnitsPerTimeStep(-250, -1000 / kTimeStepsPerSecond));
    EXPECT_EQ(1250, Time::MsToSubUnitsPerTimeStep(-1250, -1000 / kTimeStepsPerSecond));

    // Multiple steps
    EXPECT_EQ(250, Time::MsToSubUnitsPerTimeStep(-1000, -4000 / kTimeStepsPerSecond));
    EXPECT_EQ(333, Time::MsToSubUnitsPerTimeStep(-1000, -3000 / kTimeStepsPerSecond));
    EXPECT_EQ(400, Time::MsToSubUnitsPerTimeStep(-800, -2000 / kTimeStepsPerSecond));

    // Zero in, zero out
    EXPECT_EQ(0, Time::MsToSubUnitsPerTimeStep(0, 20000 / kTimeStepsPerSecond));
    EXPECT_EQ(0, Time::MsToSubUnitsPerTimeStep(0, -50000 / kTimeStepsPerSecond));
}

}  // namespace simulation
