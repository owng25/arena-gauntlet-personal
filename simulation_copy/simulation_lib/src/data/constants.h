#pragma once

#include <cstddef>
#include <limits>
#include <vector>

#include "utility/fixed_point.h"

namespace simulation
{
// Unique identifier for an entity, just an integer
using EntityID = int;

// Upper bound roll for random modifier of specific stat type
static constexpr int kRandomStatModifierMaxRoll = 5;

// Percentage by which random modifier stats get incremental for each roll(ex, roll = 2, (2*5) )
static constexpr int kRandomStatModifierRollIncrementPercentage = 5;

// Milliseconds in a second
static constexpr int kMsPerSecond = 1000;

// Number of time steps each second
static constexpr int kTimeStepsPerSecond = 10;

// Number of ms that represents one time step
static constexpr int kMsPerTimeStep = kMsPerSecond / kTimeStepsPerSecond;

// Number of subunits per unit
// Each position (x, y) is composed of kSubUnitsPerUnit * kSubUnitsPerUnit subunits
static constexpr int kSubUnitsPerUnit = 1000;

// Half of subunits per unit
static constexpr int kHalfOfSubUnitsPerUnit = kSubUnitsPerUnit / 2;

/**
 * Used when doing division and other calculations where the conversion to an integer might lose
 * us too much precision. We multiply by this factor before we calculate and divide by it at
 * the end.
 *
 * DO NOT MODIFY THIS WITHOUT ALSO MODIFYING SINE TABLE IN math.h
 */
static constexpr int kPrecisionFactor = 1000;
static constexpr FixedPoint kPrecisionFactorFP = 1000_fp;

// Square root of 2 scaled by kPrecisionFactor
static constexpr int kSqrt2Scaled = 1414;

// Square root of 3 scaled by kPrecisionFactor
static constexpr int kSqrt3Scaled = 1732;

// Time amount which represents infinite time which basically runs forever
static constexpr int kTimeInfinite = -1;

// Constant for an invalid index
static constexpr size_t kInvalidIndex = (std::numeric_limits<size_t>::max)();

// Constant for an invalid Entity ID
static constexpr EntityID kInvalidEntityID = -1;

// Constant for an invalid position inside the grid coordinates (units/subunits)
static constexpr int kInvalidPosition = (std::numeric_limits<int>::min)();

// Maximum number of time steps to go without pathfinding for an entity
static constexpr int kPathfindingUpdateMaxTimeSteps = 18;

// Omega duration after which we should always refocus
static constexpr int kRefocusAfterOmegaMs = 2000;

// Maximum number of time steps in a battle
// Chosen so that we don't get an integer overflow when multiplied by kPrecisionFactor
static constexpr int kMaxNumberOfTimeSteps = 40000;

// Minimum level of an illuvial
static constexpr int kMinLevel = 1;

// Max level of an illuvial
static constexpr int kMaxLevel = 60;
static constexpr FixedPoint kMaxLevelFP = FixedPoint::FromInt(kMaxLevel);

// Constant for an invalid global collision ID
static constexpr int kInvalidGlobalCollisionID = -1;

//
// Stats constants
//

// Min/Max percentage values
// Used for stats like hit chance
static constexpr int kMinPercentage = 0;
static constexpr int kMaxPercentage = 100;

static constexpr FixedPoint kMinPercentageFP = FixedPoint::FromInt(kMinPercentage);
static constexpr FixedPoint kMaxPercentageFP = FixedPoint::FromInt(kMaxPercentage);

static constexpr FixedPoint kDefaultAttackSpeed = 100_fp;
static constexpr FixedPoint kDefaultCritChancePercentage = 25_fp;
static constexpr FixedPoint kDefaultCritAmplificationPercentage = 150_fp;
static constexpr FixedPoint kDefaultHitChancePercentage = kMaxPercentageFP;
static constexpr FixedPoint kDefaultHealthGainEfficiencyPercentage = kMaxPercentageFP;
static constexpr FixedPoint kDefaultEnergyGainEfficiencyPercentage = kMaxPercentageFP;
static constexpr FixedPoint kDefaultShieldGainEfficiencyPercentage = kMaxPercentageFP;
static constexpr FixedPoint kDefaultOmegaPowerPercentage = kMaxPercentageFP;
static constexpr FixedPoint kDefaultOmegaDamagePercentage = kMaxPercentageFP;
static constexpr FixedPoint kDefaultVulnerabilityPercentage = kMaxPercentageFP;

static constexpr int kHyperCounterMax = 8;
static constexpr FixedPoint kMaxHyper = 100_fp;
static constexpr FixedPoint kMaxSubHyper = kMaxHyper * FixedPoint::FromInt(kPrecisionFactor);

// Overflow stats are used to have an effect occur when they are >= 100
// https://illuvium.atlassian.net/wiki/spaces/AB/pages/209747993/Random+Overflow+Stat
static constexpr int kRandomOverflowStatMax = 100;

// Time steps between fainted and disappear combat unit states
static constexpr int kDisappearTimeSteps = 5;

// At what frequency do the attached effects tick
static constexpr int kDefaultAttachedEffectsFrequencyMs = 100;

// Tolerance to switch to angle based focus selection, in number of hexes
static constexpr int kAngleFocusHexTolerance = 3;

// Angles for specific relationships
static constexpr int kAngleRedFacingBlue = 90;
static constexpr int kAngleBlueFacingRed = 270;

// Used for movement
using ObstaclesMapType = std::vector<uint8_t>;

}  // namespace simulation
