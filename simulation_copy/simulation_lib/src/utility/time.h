#pragma once

#include "data/constants.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * Time
 *
 * This defines defines time functions working on integers
 * --------------------------------------------------------------------------------------------------------
 */
class Time
{
public:
    // Converts milliseconds into time steps
    static constexpr int MsToTimeSteps(const int time_ms)
    {
        // Special case for infinite time
        if (time_ms == kTimeInfinite)
        {
            return kTimeInfinite;
        }

        return kTimeStepsPerSecond * time_ms / kMsPerSecond;
    }

    // Converts time steps into seconds
    static constexpr int TimeStepsToSeconds(const int time_steps)
    {
        // Special case for infinite time
        if (time_steps == kTimeInfinite)
        {
            return kTimeInfinite;
        }

        return TimeStepsToMs(time_steps) / kMsPerSecond;
    }

    // Converts time steps into milliseconds
    static constexpr int TimeStepsToMs(const int time_steps)
    {
        // Special case for infinite time
        if (time_steps == kTimeInfinite)
        {
            return kTimeInfinite;
        }

        return time_steps * kMsPerTimeStep;
    }

    // Convert to time steps
    // 200% = 2000 ms = 20 time steps
    // 100% = 1000 ms = 10 time steps
    // 60% = 600 ms = 6 time steps
    // 50% = 500 ms = 5 time steps

    // Converts the attacks speed percentage per second into time steps
    static constexpr int AttackSpeedToTimeSteps(const int attack_speed)
    {
        // Avoid division by zero
        if (attack_speed == 0)
        {
            return kMaxNumberOfTimeSteps;
        }

        const int time_steps = kMsPerSecond / attack_speed;
        return time_steps;
    }

    // Converts time steps into attacks speed percentage per second
    static constexpr int TimeStepsToAttackSpeed(const int time_steps)
    {
        // Avoid division by zero
        if (time_steps == 0)
        {
            return TimeStepsToAttackSpeed(1);
        }

        return kMsPerSecond / time_steps;
    }

    // Converts the attack speed percentage per second into ms
    static constexpr int AttackSpeedToMs(const int attack_speed)
    {
        return TimeStepsToMs(AttackSpeedToTimeSteps(attack_speed));
    }

    // Converts the attack speed percentage per second into ms
    // and returns the exact number of milliseconds (flooring the
    // value under one millisecond)
    static constexpr int AttackSpeedToMsNoRounding(const int attack_speed)
    {
        // Avoid division by zero
        if (attack_speed == 0)
        {
            return TimeStepsToMs(kMaxNumberOfTimeSteps);
        }

        return (kMsPerSecond * kMaxPercentage) / attack_speed;
    }

    // Converts ms to attack speed percentage
    static constexpr int MsToAttackSpeed(const int time_ms)
    {
        return TimeStepsToAttackSpeed(MsToTimeSteps(time_ms));
    }

    // Convert the speed units into game time steps
    static constexpr int SpeedUnitsToTimeSteps(const int speed_units)
    {
        // Avoid division by zero, do the maximum amount of time steps
        if (speed_units == 0)
        {
            return kTimeStepsPerSecond;
        }

        // Everything over kTimeStepsPerSecond we can do it less than one time step
        // but cap at minimum of one time step.
        if (speed_units > kTimeStepsPerSecond)
        {
            return 1;
        }

        return kTimeStepsPerSecond / speed_units;
    }

    // Calculate amount of time steps required to traverse distance_units distance with speed
    // speed_units / time step
    static constexpr int TimeStepsFromDistanceAndSpeedUnits(const int distance_units, const int speed_units)
    {
        if (speed_units == 0)
        {
            return 1;
        }

        return distance_units / speed_units;
    }

    // Converts milliseconds into subunits per time step over distance
    static constexpr int MsToSubUnitsPerTimeStep(const int distance_sub_units, const int time_ms)
    {
        const int time_steps = MsToTimeSteps(time_ms);

        // Avoid division by zero, return the entire distance, so it completes instantly
        if (time_steps == 0)
        {
            return distance_sub_units;
        }

        return distance_sub_units / time_steps;
    }
};

}  // namespace simulation
