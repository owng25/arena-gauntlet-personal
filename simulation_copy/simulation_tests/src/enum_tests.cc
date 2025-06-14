
#include "gtest/gtest.h"
#include "utility/enum.h"

namespace simulation
{
TEST(Enum, CombatUnitTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", CombatUnitType::kNone));
    ASSERT_EQ("", fmt::format("{}", CombatUnitType::kNum));
    ASSERT_EQ("Illuvial", fmt::format("{}", CombatUnitType::kIlluvial));
    ASSERT_EQ("Ranger", fmt::format("{}", CombatUnitType::kRanger));
    ASSERT_EQ("Pet", fmt::format("{}", CombatUnitType::kPet));
}

TEST(Enum, StringToCombatUnitType)
{
    ASSERT_EQ(CombatUnitType::kNone, Enum::StringToCombatUnitType(""));
    ASSERT_EQ(CombatUnitType::kNone, Enum::StringToCombatUnitType("1"));
    ASSERT_EQ(CombatUnitType::kNone, Enum::StringToCombatUnitType("whatever"));
    ASSERT_EQ(CombatUnitType::kNone, Enum::StringToCombatUnitType("illuvial"));
    ASSERT_EQ(CombatUnitType::kIlluvial, Enum::StringToCombatUnitType("Illuvial"));
    ASSERT_EQ(CombatUnitType::kNone, Enum::StringToCombatUnitType("ranger"));
    ASSERT_EQ(CombatUnitType::kRanger, Enum::StringToCombatUnitType("Ranger"));
    ASSERT_EQ(CombatUnitType::kNone, Enum::StringToCombatUnitType("pet"));
    ASSERT_EQ(CombatUnitType::kPet, Enum::StringToCombatUnitType("Pet"));
}

TEST(Enum, EnergyGainTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", EnergyGainType::kNone));
    ASSERT_EQ("Regeneration", fmt::format("{}", EnergyGainType::kRegeneration));
    ASSERT_EQ("Attack", fmt::format("{}", EnergyGainType::kAttack));
    ASSERT_EQ("OnActivation", fmt::format("{}", EnergyGainType::kOnActivation));
    ASSERT_EQ("Refund", fmt::format("{}", EnergyGainType::kRefund));
    ASSERT_EQ("OnTakeDamage", fmt::format("{}", EnergyGainType::kOnTakeDamage));
}

TEST(Enum, StringToEnergyGainType)
{
    ASSERT_EQ(EnergyGainType::kNone, Enum::StringToEnergyGainType(""));
    ASSERT_EQ(EnergyGainType::kNone, Enum::StringToEnergyGainType("regeneration"));
    ASSERT_EQ(EnergyGainType::kRegeneration, Enum::StringToEnergyGainType("Regeneration"));
    ASSERT_EQ(EnergyGainType::kNone, Enum::StringToEnergyGainType("attack"));
    ASSERT_EQ(EnergyGainType::kAttack, Enum::StringToEnergyGainType("Attack"));
    ASSERT_EQ(EnergyGainType::kNone, Enum::StringToEnergyGainType("onactivation"));
    ASSERT_EQ(EnergyGainType::kOnActivation, Enum::StringToEnergyGainType("OnActivation"));
    ASSERT_EQ(EnergyGainType::kNone, Enum::StringToEnergyGainType("refund"));
    ASSERT_EQ(EnergyGainType::kRefund, Enum::StringToEnergyGainType("Refund"));
    ASSERT_EQ(EnergyGainType::kNone, Enum::StringToEnergyGainType("ontakeDamage"));
    ASSERT_EQ(EnergyGainType::kOnTakeDamage, Enum::StringToEnergyGainType("OnTakeDamage"));
}

TEST(Enum, StringToHealthGainType)
{
    ASSERT_EQ(HealthGainType::kNone, Enum::StringToHealthGainType(""));
    ASSERT_EQ(HealthGainType::kNone, Enum::StringToHealthGainType("regeneration"));
    ASSERT_EQ(HealthGainType::kRegeneration, Enum::StringToHealthGainType("Regeneration"));
    ASSERT_EQ(HealthGainType::kNone, Enum::StringToHealthGainType("energyvamp"));
    ASSERT_EQ(HealthGainType::kEnergyVamp, Enum::StringToHealthGainType("EnergyVamp"));
    ASSERT_EQ(HealthGainType::kNone, Enum::StringToHealthGainType("instantheal"));
    ASSERT_EQ(HealthGainType::kInstantHeal, Enum::StringToHealthGainType("InstantHeal"));
    ASSERT_EQ(HealthGainType::kNone, Enum::StringToHealthGainType("instantpureheal"));
    ASSERT_EQ(HealthGainType::kInstantPureHeal, Enum::StringToHealthGainType("InstantPureHeal"));
    ASSERT_EQ(HealthGainType::kNone, Enum::StringToHealthGainType("physicalvamp"));
    ASSERT_EQ(HealthGainType::kPhysicalVamp, Enum::StringToHealthGainType("PhysicalVamp"));
    ASSERT_EQ(HealthGainType::kNone, Enum::StringToHealthGainType("purevamp"));
    ASSERT_EQ(HealthGainType::kPureVamp, Enum::StringToHealthGainType("PureVamp"));
}

TEST(Enum, StringToEffectHealType)
{
    ASSERT_EQ(EffectHealType::kNone, Enum::StringToEffectHealType(""));
    ASSERT_EQ(EffectHealType::kNone, Enum::StringToEffectHealType("normal"));
    ASSERT_EQ(EffectHealType::kNormal, Enum::StringToEffectHealType("Normal"));
    ASSERT_EQ(EffectHealType::kNone, Enum::StringToEffectHealType("pure"));
    ASSERT_EQ(EffectHealType::kPure, Enum::StringToEffectHealType("Pure"));
}

TEST(Enum, EffectHealTypeToString)
{
    ASSERT_EQ(Enum::StringToEffectHealType(""), EffectHealType::kNone);
    ASSERT_EQ(Enum::StringToEffectHealType("normal"), EffectHealType::kNone);
    ASSERT_EQ(Enum::StringToEffectHealType("Normal"), EffectHealType::kNormal);
    ASSERT_EQ(Enum::StringToEffectHealType("pure"), EffectHealType::kNone);
    ASSERT_EQ(Enum::StringToEffectHealType("Pure"), EffectHealType::kPure);
}

TEST(Enum, StringToAbilityType)
{
    ASSERT_EQ(AbilityType::kNone, Enum::StringToAbilityType(""));
    ASSERT_EQ(AbilityType::kNone, Enum::StringToAbilityType("1"));
    ASSERT_EQ(AbilityType::kNone, Enum::StringToAbilityType("whatever"));
    ASSERT_EQ(AbilityType::kNone, Enum::StringToAbilityType("attack"));
    ASSERT_EQ(AbilityType::kAttack, Enum::StringToAbilityType("Attack"));
    ASSERT_EQ(AbilityType::kNone, Enum::StringToAbilityType("omega"));
    ASSERT_EQ(AbilityType::kOmega, Enum::StringToAbilityType("Omega"));
    ASSERT_EQ(AbilityType::kNone, Enum::StringToAbilityType("innate"));
    ASSERT_EQ(AbilityType::kInnate, Enum::StringToAbilityType("Innate"));
}

TEST(Enum, AbilityUpdateTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", AbilityUpdateType::kNone));
    ASSERT_EQ("Add", fmt::format("{}", AbilityUpdateType::kAdd));
    ASSERT_EQ("Replace", fmt::format("{}", AbilityUpdateType::kReplace));
}

TEST(Enum, AbilityTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", AbilityType::kNone));
    ASSERT_EQ("Attack", fmt::format("{}", AbilityType::kAttack));
    ASSERT_EQ("Omega", fmt::format("{}", AbilityType::kOmega));
    ASSERT_EQ("Innate", fmt::format("{}", AbilityType::kInnate));
}

TEST(Enum, StringToTeam)
{
    ASSERT_EQ(Team::kNone, Enum::StringToTeam(""));
    ASSERT_EQ(Team::kNone, Enum::StringToTeam("1"));
    ASSERT_EQ(Team::kNone, Enum::StringToTeam("whatever"));
    ASSERT_EQ(Team::kNone, Enum::StringToTeam("red"));
    ASSERT_EQ(Team::kRed, Enum::StringToTeam("Red"));
    ASSERT_EQ(Team::kNone, Enum::StringToTeam("blue"));
    ASSERT_EQ(Team::kBlue, Enum::StringToTeam("Blue"));
}

TEST(Enum, TeamToString)
{
    ASSERT_EQ("", fmt::format("{}", Team::kNone));
    ASSERT_EQ("Red", fmt::format("{}", Team::kRed));
    ASSERT_EQ("Blue", fmt::format("{}", Team::kBlue));
}

TEST(Enum, EffectTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", EffectType::kNone));
    ASSERT_EQ("", fmt::format("{}", EffectType::kNum));
    ASSERT_EQ("InstantDamage", fmt::format("{}", EffectType::kInstantDamage));
    ASSERT_EQ("DOT", fmt::format("{}", EffectType::kDamageOverTime));
    ASSERT_EQ("Condition", fmt::format("{}", EffectType::kCondition));
    ASSERT_EQ("HOT", fmt::format("{}", EffectType::kHealOverTime));
    ASSERT_EQ("EnergyGainOverTime", fmt::format("{}", EffectType::kEnergyGainOverTime));
    ASSERT_EQ("InstantHyperBurn", fmt::format("{}", EffectType::kInstantHyperBurn));
    ASSERT_EQ("InstantHyperGain", fmt::format("{}", EffectType::kInstantHyperGain));
    ASSERT_EQ("HyperGainOverTime", fmt::format("{}", EffectType::kHyperGainOverTime));
    ASSERT_EQ("HyperBurnOverTime", fmt::format("{}", EffectType::kHyperBurnOverTime));
    ASSERT_EQ("Displacement", fmt::format("{}", EffectType::kDisplacement));
    ASSERT_EQ("Empower", fmt::format("{}", EffectType::kEmpower));
    ASSERT_EQ("Disempower", fmt::format("{}", EffectType::kDisempower));
    ASSERT_EQ("SpawnShield", fmt::format("{}", EffectType::kSpawnShield));
    ASSERT_EQ("SpawnMark", fmt::format("{}", EffectType::kSpawnMark));
    ASSERT_EQ("PositiveState", fmt::format("{}", EffectType::kPositiveState));
    ASSERT_EQ("NegativeState", fmt::format("{}", EffectType::kNegativeState));
    ASSERT_EQ("Debuff", fmt::format("{}", EffectType::kDebuff));
    ASSERT_EQ("Buff", fmt::format("{}", EffectType::kBuff));
    ASSERT_EQ("Aura", fmt::format("{}", EffectType::kAura));
    ASSERT_EQ("InstantHeal", fmt::format("{}", EffectType::kInstantHeal));
    ASSERT_EQ("Cleanse", fmt::format("{}", EffectType::kCleanse));
    ASSERT_EQ("InstantEnergyBurn", fmt::format("{}", EffectType::kInstantEnergyBurn));
    ASSERT_EQ("InstantEnergyGain", fmt::format("{}", EffectType::kInstantEnergyGain));
    ASSERT_EQ("EnergyBurnOverTime", fmt::format("{}", EffectType::kEnergyBurnOverTime));
    ASSERT_EQ("Execute", fmt::format("{}", EffectType::kExecute));
    ASSERT_EQ("Blink", fmt::format("{}", EffectType::kBlink));
    ASSERT_EQ("PlaneChange", fmt::format("{}", EffectType::kPlaneChange));
    ASSERT_EQ("InstantShieldBurn", fmt::format("{}", EffectType::kInstantShieldBurn));
}

TEST(Enum, StringToEffectType)
{
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType(""));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("1"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("whatever"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("instantdamage"));
    ASSERT_EQ(EffectType::kInstantDamage, Enum::StringToEffectType("InstantDamage"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("dot"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("DoT"));
    ASSERT_EQ(EffectType::kDamageOverTime, Enum::StringToEffectType("DOT"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("condition"));
    ASSERT_EQ(EffectType::kCondition, Enum::StringToEffectType("Condition"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("hot"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("HoT"));
    ASSERT_EQ(EffectType::kHealOverTime, Enum::StringToEffectType("HOT"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("energygainovertime"));
    ASSERT_EQ(EffectType::kEnergyGainOverTime, Enum::StringToEffectType("EnergyGainOverTime"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("instanthyperburn"));
    ASSERT_EQ(EffectType::kInstantHyperBurn, Enum::StringToEffectType("InstantHyperBurn"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("instanthypergain"));
    ASSERT_EQ(EffectType::kInstantHyperGain, Enum::StringToEffectType("InstantHyperGain"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("hypergainovertime"));
    ASSERT_EQ(EffectType::kHyperGainOverTime, Enum::StringToEffectType("HyperGainOverTime"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("hyperburnovertime"));
    ASSERT_EQ(EffectType::kHyperBurnOverTime, Enum::StringToEffectType("HyperBurnOverTime"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("displacement"));
    ASSERT_EQ(EffectType::kDisplacement, Enum::StringToEffectType("Displacement"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("empower"));
    ASSERT_EQ(EffectType::kEmpower, Enum::StringToEffectType("Empower"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("disempower"));
    ASSERT_EQ(EffectType::kDisempower, Enum::StringToEffectType("Disempower"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("spawnshield"));
    ASSERT_EQ(EffectType::kSpawnShield, Enum::StringToEffectType("SpawnShield"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("spawnmark"));
    ASSERT_EQ(EffectType::kSpawnMark, Enum::StringToEffectType("SpawnMark"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("positivestate"));
    ASSERT_EQ(EffectType::kPositiveState, Enum::StringToEffectType("PositiveState"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("negativestate"));
    ASSERT_EQ(EffectType::kNegativeState, Enum::StringToEffectType("NegativeState"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("debuff"));
    ASSERT_EQ(EffectType::kDebuff, Enum::StringToEffectType("Debuff"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("buff"));
    ASSERT_EQ(EffectType::kBuff, Enum::StringToEffectType("Buff"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("aura"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("instantheal"));
    ASSERT_EQ(EffectType::kInstantHeal, Enum::StringToEffectType("InstantHeal"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("cleanse"));
    ASSERT_EQ(EffectType::kCleanse, Enum::StringToEffectType("Cleanse"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("instantenergyburn"));
    ASSERT_EQ(EffectType::kInstantEnergyBurn, Enum::StringToEffectType("InstantEnergyBurn"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("instantenergygain"));
    ASSERT_EQ(EffectType::kInstantEnergyGain, Enum::StringToEffectType("InstantEnergyGain"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("energyburnovertime"));
    ASSERT_EQ(EffectType::kEnergyBurnOverTime, Enum::StringToEffectType("EnergyBurnOverTime"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("execute"));
    ASSERT_EQ(EffectType::kExecute, Enum::StringToEffectType("Execute"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("blink"));
    ASSERT_EQ(EffectType::kBlink, Enum::StringToEffectType("Blink"));
    ASSERT_EQ(EffectType::kNone, Enum::StringToEffectType("planechange"));
    ASSERT_EQ(EffectType::kPlaneChange, Enum::StringToEffectType("PlaneChange"));
    ASSERT_EQ(EffectType::kAura, Enum::StringToEffectType("Aura"));
}

TEST(Enum, ConditionTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", ConditionType::kNone));
    ASSERT_EQ("", fmt::format("{}", ConditionType::kNum));
    ASSERT_EQ("Poisoned", fmt::format("{}", ConditionType::kPoisoned));
    ASSERT_EQ("Burned", fmt::format("{}", ConditionType::kBurned));
    ASSERT_EQ("Wounded", fmt::format("{}", ConditionType::kWounded));
    ASSERT_EQ("Frosted", fmt::format("{}", ConditionType::kFrosted));
    ASSERT_EQ("Shielded", fmt::format("{}", ConditionType::kShielded));
}

TEST(Enum, StringToConditionType)
{
    ASSERT_EQ(ConditionType::kNone, Enum::StringToConditionType(""));
    ASSERT_EQ(ConditionType::kNone, Enum::StringToConditionType("1"));
    ASSERT_EQ(ConditionType::kNone, Enum::StringToConditionType("whatever"));
    ASSERT_EQ(ConditionType::kNone, Enum::StringToConditionType("poisoned"));
    ASSERT_EQ(ConditionType::kPoisoned, Enum::StringToConditionType("Poisoned"));
    ASSERT_EQ(ConditionType::kNone, Enum::StringToConditionType("burned"));
    ASSERT_EQ(ConditionType::kBurned, Enum::StringToConditionType("Burned"));
    ASSERT_EQ(ConditionType::kNone, Enum::StringToConditionType("wounded"));
    ASSERT_EQ(ConditionType::kWounded, Enum::StringToConditionType("Wounded"));
    ASSERT_EQ(ConditionType::kNone, Enum::StringToConditionType("frosted"));
    ASSERT_EQ(ConditionType::kFrosted, Enum::StringToConditionType("Frosted"));
    ASSERT_EQ(ConditionType::kNone, Enum::StringToConditionType("shielded"));
    ASSERT_EQ(ConditionType::kShielded, Enum::StringToConditionType("Shielded"));
}

TEST(Enum, EffectValueTypeToString)
{
    ASSERT_EQ("Value", fmt::format("{}", EffectValueType::kValue));
    ASSERT_EQ("Stat", fmt::format("{}", EffectValueType::kStat));
    ASSERT_EQ("StatPercentage", fmt::format("{}", EffectValueType::kStatPercentage));
    ASSERT_EQ("StatHighPrecisionPercentage", fmt::format("{}", EffectValueType::kStatHighPrecisionPercentage));
    ASSERT_EQ("SynergyCount", fmt::format("{}", EffectValueType::kSynergyCount));
    ASSERT_EQ("MeteredStatPercentage", fmt::format("{}", EffectValueType::kMeteredStatPercentage));
}

TEST(Enum, StringToEffectValueType)
{
    ASSERT_EQ(EffectValueType::kValue, Enum::StringToEffectValueType(""));
    ASSERT_EQ(EffectValueType::kValue, Enum::StringToEffectValueType("1"));
    ASSERT_EQ(EffectValueType::kValue, Enum::StringToEffectValueType("value"));
    ASSERT_EQ(EffectValueType::kValue, Enum::StringToEffectValueType("Value"));
    ASSERT_EQ(EffectValueType::kValue, Enum::StringToEffectValueType("stat"));
    ASSERT_EQ(EffectValueType::kStat, Enum::StringToEffectValueType("Stat"));
    ASSERT_EQ(EffectValueType::kValue, Enum::StringToEffectValueType("statpercentage"));
    ASSERT_EQ(EffectValueType::kStatPercentage, Enum::StringToEffectValueType("StatPercentage"));
    ASSERT_EQ(EffectValueType::kValue, Enum::StringToEffectValueType("stathighprecisionpercentage"));
    ASSERT_EQ(
        EffectValueType::kStatHighPrecisionPercentage,
        Enum::StringToEffectValueType("StatHighPrecisionPercentage"));
    ASSERT_EQ(EffectValueType::kValue, Enum::StringToEffectValueType("synergycount"));
    ASSERT_EQ(EffectValueType::kSynergyCount, Enum::StringToEffectValueType("SynergyCount"));
    ASSERT_EQ(EffectValueType::kValue, Enum::StringToEffectValueType("meteredstatpercentage"));
    ASSERT_EQ(EffectValueType::kMeteredStatPercentage, Enum::StringToEffectValueType("MeteredStatPercentage"));
}

TEST(Enum, EffectOperationTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", EffectOperationType::kNone));
    ASSERT_EQ("", fmt::format("{}", EffectOperationType::kNum));
    ASSERT_EQ("+", fmt::format("{}", EffectOperationType::kAdd));
    ASSERT_EQ("-", fmt::format("{}", EffectOperationType::kSubtract));
    ASSERT_EQ("*", fmt::format("{}", EffectOperationType::kMultiply));
    ASSERT_EQ("/", fmt::format("{}", EffectOperationType::kDivide));
    ASSERT_EQ("%", fmt::format("{}", EffectOperationType::kPercentageOf));
    ASSERT_EQ("%%", fmt::format("{}", EffectOperationType::kHighPrecisionPercentageOf));
}

TEST(Enum, StringToEffectOperationType)
{
    ASSERT_EQ(EffectOperationType::kNone, Enum::StringToEffectOperationType(""));
    ASSERT_EQ(EffectOperationType::kNone, Enum::StringToEffectOperationType("1"));
    ASSERT_EQ(EffectOperationType::kNone, Enum::StringToEffectOperationType("whatever"));
    ASSERT_EQ(EffectOperationType::kAdd, Enum::StringToEffectOperationType("+"));
    ASSERT_EQ(EffectOperationType::kSubtract, Enum::StringToEffectOperationType("-"));
    ASSERT_EQ(EffectOperationType::kMultiply, Enum::StringToEffectOperationType("*"));
    ASSERT_EQ(EffectOperationType::kDivide, Enum::StringToEffectOperationType("/"));
    ASSERT_EQ(EffectOperationType::kPercentageOf, Enum::StringToEffectOperationType("%"));
    ASSERT_EQ(EffectOperationType::kHighPrecisionPercentageOf, Enum::StringToEffectOperationType("%%"));
}

TEST(Enum, ComparisonTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", ComparisonType::kNone));
    ASSERT_EQ("", fmt::format("{}", ComparisonType::kNum));
    ASSERT_EQ(">", fmt::format("{}", ComparisonType::kGreater));
    ASSERT_EQ("<", fmt::format("{}", ComparisonType::kLess));
    ASSERT_EQ("==", fmt::format("{}", ComparisonType::kEqual));
}

TEST(Enum, StringToComparisonType)
{
    ASSERT_EQ(ComparisonType::kNone, Enum::StringToComparisonType(""));
    ASSERT_EQ(ComparisonType::kNone, Enum::StringToComparisonType("1"));
    ASSERT_EQ(ComparisonType::kNone, Enum::StringToComparisonType("whatever"));
    ASSERT_EQ(ComparisonType::kGreater, Enum::StringToComparisonType(">"));
    ASSERT_EQ(ComparisonType::kLess, Enum::StringToComparisonType("<"));
    ASSERT_EQ(ComparisonType::kEqual, Enum::StringToComparisonType("=="));
}

TEST(Enum, EffectDamageTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", EffectDamageType::kNone));
    ASSERT_EQ("", fmt::format("{}", EffectDamageType::kNum));
    ASSERT_EQ("Physical", fmt::format("{}", EffectDamageType::kPhysical));
    ASSERT_EQ("Energy", fmt::format("{}", EffectDamageType::kEnergy));
    ASSERT_EQ("Pure", fmt::format("{}", EffectDamageType::kPure));
}

TEST(Enum, StringToEffectDamageType)
{
    ASSERT_EQ(EffectDamageType::kNone, Enum::StringToEffectDamageType(""));
    ASSERT_EQ(EffectDamageType::kNone, Enum::StringToEffectDamageType("1"));
    ASSERT_EQ(EffectDamageType::kNone, Enum::StringToEffectDamageType("whatever"));
    ASSERT_EQ(EffectDamageType::kPhysical, Enum::StringToEffectDamageType("Physical"));
    ASSERT_EQ(EffectDamageType::kEnergy, Enum::StringToEffectDamageType("Energy"));
    ASSERT_EQ(EffectDamageType::kPure, Enum::StringToEffectDamageType("Pure"));
}

TEST(Enum, EffectPositiveStateToString)
{
    ASSERT_EQ("", fmt::format("{}", EffectPositiveState::kNone));
    ASSERT_EQ("", fmt::format("{}", EffectPositiveState::kNum));
    ASSERT_EQ("Immune", fmt::format("{}", EffectPositiveState::kImmune));
    ASSERT_EQ("Truesight", fmt::format("{}", EffectPositiveState::kTruesight));
    ASSERT_EQ("Indomitable", fmt::format("{}", EffectPositiveState::kIndomitable));
    ASSERT_EQ("EffectPackageBlock", fmt::format("{}", EffectPositiveState::kEffectPackageBlock));
    ASSERT_EQ("Invulnerable", fmt::format("{}", EffectPositiveState::kInvulnerable));
    ASSERT_EQ("Untargetable", fmt::format("{}", EffectPositiveState::kUntargetable));
}

TEST(Enum, StringToEffectPositiveState)
{
    ASSERT_EQ(EffectPositiveState::kNone, Enum::StringToEffectPositiveState(""));
    ASSERT_EQ(EffectPositiveState::kNone, Enum::StringToEffectPositiveState("1"));
    ASSERT_EQ(EffectPositiveState::kNone, Enum::StringToEffectPositiveState("whatever"));
    ASSERT_EQ(EffectPositiveState::kNone, Enum::StringToEffectPositiveState("immune"));
    ASSERT_EQ(EffectPositiveState::kImmune, Enum::StringToEffectPositiveState("Immune"));
    ASSERT_EQ(EffectPositiveState::kNone, Enum::StringToEffectPositiveState("truesight"));
    ASSERT_EQ(EffectPositiveState::kTruesight, Enum::StringToEffectPositiveState("Truesight"));
    ASSERT_EQ(EffectPositiveState::kNone, Enum::StringToEffectPositiveState("indomitable"));
    ASSERT_EQ(EffectPositiveState::kIndomitable, Enum::StringToEffectPositiveState("Indomitable"));
    ASSERT_EQ(EffectPositiveState::kNone, Enum::StringToEffectPositiveState("effectpackageblock"));
    ASSERT_EQ(EffectPositiveState::kEffectPackageBlock, Enum::StringToEffectPositiveState("EffectPackageBlock"));
    ASSERT_EQ(EffectPositiveState::kNone, Enum::StringToEffectPositiveState("invulnerable"));
    ASSERT_EQ(EffectPositiveState::kInvulnerable, Enum::StringToEffectPositiveState("Invulnerable"));
    ASSERT_EQ(EffectPositiveState::kNone, Enum::StringToEffectPositiveState("untargetable"));
    ASSERT_EQ(EffectPositiveState::kUntargetable, Enum::StringToEffectPositiveState("Untargetable"));
}

TEST(Enum, EffectNegativeStateToString)
{
    ASSERT_EQ("", fmt::format("{}", EffectNegativeState::kNone));
    ASSERT_EQ("", fmt::format("{}", EffectNegativeState::kNum));
    ASSERT_EQ("Focused", fmt::format("{}", EffectNegativeState::kFocused));
    ASSERT_EQ("Stun", fmt::format("{}", EffectNegativeState::kStun));
    ASSERT_EQ("Disarm", fmt::format("{}", EffectNegativeState::kDisarm));
    ASSERT_EQ("Blind", fmt::format("{}", EffectNegativeState::kBlind));
    ASSERT_EQ("Root", fmt::format("{}", EffectNegativeState::kRoot));
    ASSERT_EQ("Clumsy", fmt::format("{}", EffectNegativeState::kClumsy));
    ASSERT_EQ("Silenced", fmt::format("{}", EffectNegativeState::kSilenced));
    ASSERT_EQ("Lethargic", fmt::format("{}", EffectNegativeState::kLethargic));
    ASSERT_EQ("Taunted", fmt::format("{}", EffectNegativeState::kTaunted));
    ASSERT_EQ("Charm", fmt::format("{}", EffectNegativeState::kCharm));
    ASSERT_EQ("Flee", fmt::format("{}", EffectNegativeState::kFlee));
}

TEST(Enum, StringToEffectNegativeState)
{
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState(""));
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState("1"));
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState("whatever"));
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState("focused"));
    ASSERT_EQ(EffectNegativeState::kFocused, Enum::StringToEffectNegativeState("Focused"));
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState("frozen"));
    ASSERT_EQ(EffectNegativeState::kFrozen, Enum::StringToEffectNegativeState("Frozen"));
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState("stun"));
    ASSERT_EQ(EffectNegativeState::kStun, Enum::StringToEffectNegativeState("Stun"));
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState("disarm"));
    ASSERT_EQ(EffectNegativeState::kDisarm, Enum::StringToEffectNegativeState("Disarm"));
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState("blind"));
    ASSERT_EQ(EffectNegativeState::kBlind, Enum::StringToEffectNegativeState("Blind"));
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState("root"));
    ASSERT_EQ(EffectNegativeState::kRoot, Enum::StringToEffectNegativeState("Root"));
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState("clumsy"));
    ASSERT_EQ(EffectNegativeState::kClumsy, Enum::StringToEffectNegativeState("Clumsy"));
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState("silenced"));
    ASSERT_EQ(EffectNegativeState::kSilenced, Enum::StringToEffectNegativeState("Silenced"));
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState("lethargic"));
    ASSERT_EQ(EffectNegativeState::kLethargic, Enum::StringToEffectNegativeState("Lethargic"));
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState("taunted"));
    ASSERT_EQ(EffectNegativeState::kTaunted, Enum::StringToEffectNegativeState("Taunted"));
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState("charm"));
    ASSERT_EQ(EffectNegativeState::kCharm, Enum::StringToEffectNegativeState("Charm"));
    ASSERT_EQ(EffectNegativeState::kNone, Enum::StringToEffectNegativeState("flee"));
    ASSERT_EQ(EffectNegativeState::kFlee, Enum::StringToEffectNegativeState("Flee"));
}

TEST(Enum, EffectPlaneChangeToString)
{
    ASSERT_EQ("", fmt::format("{}", EffectPlaneChange::kNone));
    ASSERT_EQ("", fmt::format("{}", EffectPlaneChange::kNum));
    ASSERT_EQ("Airborne", fmt::format("{}", EffectPlaneChange::kAirborne));
    ASSERT_EQ("Underground", fmt::format("{}", EffectPlaneChange::kUnderground));
}

TEST(Enum, StringToEffectPlaneChange)
{
    ASSERT_EQ(EffectPlaneChange::kNone, Enum::StringToEffectPlaneChange(""));
    ASSERT_EQ(EffectPlaneChange::kNone, Enum::StringToEffectPlaneChange("1"));
    ASSERT_EQ(EffectPlaneChange::kNone, Enum::StringToEffectPlaneChange("whatever"));
    ASSERT_EQ(EffectPlaneChange::kNone, Enum::StringToEffectPlaneChange("airborne"));
    ASSERT_EQ(EffectPlaneChange::kAirborne, Enum::StringToEffectPlaneChange("Airborne"));
    ASSERT_EQ(EffectPlaneChange::kNone, Enum::StringToEffectPlaneChange("underground"));
    ASSERT_EQ(EffectPlaneChange::kUnderground, Enum::StringToEffectPlaneChange("Underground"));
}

TEST(Enum, EffectConditionTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", EffectConditionType::kNone));
    ASSERT_EQ("", fmt::format("{}", EffectConditionType::kNum));
    ASSERT_EQ("Poison", fmt::format("{}", EffectConditionType::kPoison));
    ASSERT_EQ("Wound", fmt::format("{}", EffectConditionType::kWound));
    ASSERT_EQ("Burn", fmt::format("{}", EffectConditionType::kBurn));
    ASSERT_EQ("Frost", fmt::format("{}", EffectConditionType::kFrost));
}

TEST(Enum, StringToEffectConditionType)
{
    ASSERT_EQ(EffectConditionType::kNone, Enum::StringToEffectConditionType(""));
    ASSERT_EQ(EffectConditionType::kNone, Enum::StringToEffectConditionType("1"));
    ASSERT_EQ(EffectConditionType::kNone, Enum::StringToEffectConditionType("whatever"));
    ASSERT_EQ(EffectConditionType::kNone, Enum::StringToEffectConditionType("poison"));
    ASSERT_EQ(EffectConditionType::kPoison, Enum::StringToEffectConditionType("Poison"));
    ASSERT_EQ(EffectConditionType::kNone, Enum::StringToEffectConditionType("wound"));
    ASSERT_EQ(EffectConditionType::kWound, Enum::StringToEffectConditionType("Wound"));
    ASSERT_EQ(EffectConditionType::kNone, Enum::StringToEffectConditionType("burn"));
    ASSERT_EQ(EffectConditionType::kBurn, Enum::StringToEffectConditionType("Burn"));
    ASSERT_EQ(EffectConditionType::kNone, Enum::StringToEffectConditionType("frost"));
    ASSERT_EQ(EffectConditionType::kFrost, Enum::StringToEffectConditionType("Frost"));
}

TEST(Enum, EffectPropagationTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", EffectPropagationType::kNone));
    ASSERT_EQ("", fmt::format("{}", EffectPropagationType::kNum));
    ASSERT_EQ("Chain", fmt::format("{}", EffectPropagationType::kChain));
    ASSERT_EQ("Splash", fmt::format("{}", EffectPropagationType::kSplash));
}

TEST(Enum, StringToEffectPropagationType)
{
    ASSERT_EQ(EffectPropagationType::kNone, Enum::StringToEffectPropagationType(""));
    ASSERT_EQ(EffectPropagationType::kNone, Enum::StringToEffectPropagationType("1"));
    ASSERT_EQ(EffectPropagationType::kNone, Enum::StringToEffectPropagationType("whatever"));
    ASSERT_EQ(EffectPropagationType::kNone, Enum::StringToEffectPropagationType("chain"));
    ASSERT_EQ(EffectPropagationType::kChain, Enum::StringToEffectPropagationType("Chain"));
    ASSERT_EQ(EffectPropagationType::kNone, Enum::StringToEffectPropagationType("splash"));
    ASSERT_EQ(EffectPropagationType::kSplash, Enum::StringToEffectPropagationType("Splash"));
}

TEST(Enum, EffectDisplacementTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", EffectDisplacementType::kNone));
    ASSERT_EQ("", fmt::format("{}", EffectDisplacementType::kNum));
    ASSERT_EQ("KnockBack", fmt::format("{}", EffectDisplacementType::kKnockBack));
    ASSERT_EQ("KnockUp", fmt::format("{}", EffectDisplacementType::kKnockUp));
    ASSERT_EQ("Pull", fmt::format("{}", EffectDisplacementType::kPull));
    ASSERT_EQ("ThrowToFurthestEnemy", fmt::format("{}", EffectDisplacementType::kThrowToFurthestEnemy));
    ASSERT_EQ("ThrowToHighestEnemyDensity", fmt::format("{}", EffectDisplacementType::kThrowToHighestEnemyDensity));
}

TEST(Enum, StringToEffectDisplacementType)
{
    ASSERT_EQ(EffectDisplacementType::kNone, Enum::StringToEffectDisplacementType(""));
    ASSERT_EQ(EffectDisplacementType::kNone, Enum::StringToEffectDisplacementType("1"));
    ASSERT_EQ(EffectDisplacementType::kNone, Enum::StringToEffectDisplacementType("whatever"));
    ASSERT_EQ(EffectDisplacementType::kNone, Enum::StringToEffectDisplacementType("knockback"));
    ASSERT_EQ(EffectDisplacementType::kKnockBack, Enum::StringToEffectDisplacementType("KnockBack"));
    ASSERT_EQ(EffectDisplacementType::kNone, Enum::StringToEffectDisplacementType("knockup"));
    ASSERT_EQ(EffectDisplacementType::kKnockUp, Enum::StringToEffectDisplacementType("KnockUp"));
    ASSERT_EQ(EffectDisplacementType::kNone, Enum::StringToEffectDisplacementType("pull"));
    ASSERT_EQ(EffectDisplacementType::kPull, Enum::StringToEffectDisplacementType("Pull"));
    ASSERT_EQ(EffectDisplacementType::kNone, Enum::StringToEffectDisplacementType("throw"));
    ASSERT_EQ(
        EffectDisplacementType::kThrowToFurthestEnemy,
        Enum::StringToEffectDisplacementType("ThrowToFurthestEnemy"));
    ASSERT_EQ(
        EffectDisplacementType::kThrowToHighestEnemyDensity,
        Enum::StringToEffectDisplacementType("ThrowToHighestEnemyDensity"));
}

TEST(Enum, EffectOverlapProcessTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", EffectOverlapProcessType::kNone));
    ASSERT_EQ("", fmt::format("{}", EffectOverlapProcessType::kNum));
    ASSERT_EQ("Sum", fmt::format("{}", EffectOverlapProcessType::kSum));
    ASSERT_EQ("Highest", fmt::format("{}", EffectOverlapProcessType::kHighest));
    ASSERT_EQ("Stacking", fmt::format("{}", EffectOverlapProcessType::kStacking));
    ASSERT_EQ("Shield", fmt::format("{}", EffectOverlapProcessType::kShield));
    ASSERT_EQ("Condition", fmt::format("{}", EffectOverlapProcessType::kCondition));
    ASSERT_EQ("Merge", fmt::format("{}", EffectOverlapProcessType::kMerge));
}

TEST(Enum, StringToEffectOverlapProcessType)
{
    ASSERT_EQ(EffectOverlapProcessType::kNone, Enum::StringToEffectOverlapProcessType(""));
    ASSERT_EQ(EffectOverlapProcessType::kNone, Enum::StringToEffectOverlapProcessType("1"));
    ASSERT_EQ(EffectOverlapProcessType::kNone, Enum::StringToEffectOverlapProcessType("whatever"));
    ASSERT_EQ(EffectOverlapProcessType::kNone, Enum::StringToEffectOverlapProcessType("sum"));
    ASSERT_EQ(EffectOverlapProcessType::kSum, Enum::StringToEffectOverlapProcessType("Sum"));
    ASSERT_EQ(EffectOverlapProcessType::kNone, Enum::StringToEffectOverlapProcessType("highest"));
    ASSERT_EQ(EffectOverlapProcessType::kHighest, Enum::StringToEffectOverlapProcessType("Highest"));
    ASSERT_EQ(EffectOverlapProcessType::kNone, Enum::StringToEffectOverlapProcessType("stacking"));
    ASSERT_EQ(EffectOverlapProcessType::kStacking, Enum::StringToEffectOverlapProcessType("Stacking"));
    ASSERT_EQ(EffectOverlapProcessType::kNone, Enum::StringToEffectOverlapProcessType("shield"));
    ASSERT_EQ(EffectOverlapProcessType::kShield, Enum::StringToEffectOverlapProcessType("Shield"));
    ASSERT_EQ(EffectOverlapProcessType::kNone, Enum::StringToEffectOverlapProcessType("condition"));
    ASSERT_EQ(EffectOverlapProcessType::kCondition, Enum::StringToEffectOverlapProcessType("Condition"));
    ASSERT_EQ(EffectOverlapProcessType::kNone, Enum::StringToEffectOverlapProcessType("merge"));
    ASSERT_EQ(EffectOverlapProcessType::kMerge, Enum::StringToEffectOverlapProcessType("Merge"));
}

TEST(Enum, MarkEffectTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", MarkEffectType::kNone));
    ASSERT_EQ("Beneficial", fmt::format("{}", MarkEffectType::kBeneficial));
    ASSERT_EQ("Detrimental", fmt::format("{}", MarkEffectType::kDetrimental));
}

TEST(Enum, StringToMarkEffectType)
{
    ASSERT_EQ(MarkEffectType::kNone, Enum::StringToMarkEffectType(""));
    ASSERT_EQ(MarkEffectType::kBeneficial, Enum::StringToMarkEffectType("Beneficial"));
    ASSERT_EQ(MarkEffectType::kNone, Enum::StringToMarkEffectType("beneficial"));
    ASSERT_EQ(MarkEffectType::kDetrimental, Enum::StringToMarkEffectType("Detrimental"));
    ASSERT_EQ(MarkEffectType::kNone, Enum::StringToMarkEffectType("detrimental"));
}

TEST(Enum, CombatClassToString)
{
    ASSERT_EQ("", fmt::format("{}", CombatClass::kNone));
    ASSERT_EQ("", fmt::format("{}", CombatClass::kNum));
    ASSERT_EQ("Fighter", fmt::format("{}", CombatClass::kFighter));
    ASSERT_EQ("Bulwark", fmt::format("{}", CombatClass::kBulwark));
    ASSERT_EQ("Rogue", fmt::format("{}", CombatClass::kRogue));
    ASSERT_EQ("Psion", fmt::format("{}", CombatClass::kPsion));
    ASSERT_EQ("Empath", fmt::format("{}", CombatClass::kEmpath));
    ASSERT_EQ("Berserker", fmt::format("{}", CombatClass::kBerserker));
    ASSERT_EQ("Behemoth", fmt::format("{}", CombatClass::kBehemoth));
    ASSERT_EQ("Colossus", fmt::format("{}", CombatClass::kColossus));
    ASSERT_EQ("Slayer", fmt::format("{}", CombatClass::kSlayer));
    ASSERT_EQ("Arcanite", fmt::format("{}", CombatClass::kArcanite));
    ASSERT_EQ("Vanguard", fmt::format("{}", CombatClass::kVanguard));
    ASSERT_EQ("Predator", fmt::format("{}", CombatClass::kPredator));
    ASSERT_EQ("Harbinger", fmt::format("{}", CombatClass::kHarbinger));
    ASSERT_EQ("Phantom", fmt::format("{}", CombatClass::kPhantom));
    ASSERT_EQ("Invoker", fmt::format("{}", CombatClass::kInvoker));
    ASSERT_EQ("Templar", fmt::format("{}", CombatClass::kTemplar));
    ASSERT_EQ("Aegis", fmt::format("{}", CombatClass::kAegis));
    ASSERT_EQ("Revenant", fmt::format("{}", CombatClass::kRevenant));
    ASSERT_EQ("Enchanter", fmt::format("{}", CombatClass::kEnchanter));
    ASSERT_EQ("Mystic", fmt::format("{}", CombatClass::kMystic));
}

TEST(Enum, StringToCombatClass)
{
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass(""));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("1"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("whatever"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("fighter"));
    ASSERT_EQ(CombatClass::kFighter, Enum::StringToCombatClass("Fighter"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("bulwark"));
    ASSERT_EQ(CombatClass::kBulwark, Enum::StringToCombatClass("Bulwark"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("rogue"));
    ASSERT_EQ(CombatClass::kRogue, Enum::StringToCombatClass("Rogue"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("psion"));
    ASSERT_EQ(CombatClass::kPsion, Enum::StringToCombatClass("Psion"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("empath"));
    ASSERT_EQ(CombatClass::kEmpath, Enum::StringToCombatClass("Empath"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("berserker"));
    ASSERT_EQ(CombatClass::kBerserker, Enum::StringToCombatClass("Berserker"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("behemoth"));
    ASSERT_EQ(CombatClass::kBehemoth, Enum::StringToCombatClass("Behemoth"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("colossus"));
    ASSERT_EQ(CombatClass::kColossus, Enum::StringToCombatClass("Colossus"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("slayer"));
    ASSERT_EQ(CombatClass::kSlayer, Enum::StringToCombatClass("Slayer"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("arcanite"));
    ASSERT_EQ(CombatClass::kArcanite, Enum::StringToCombatClass("Arcanite"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("vanguard"));
    ASSERT_EQ(CombatClass::kVanguard, Enum::StringToCombatClass("Vanguard"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("predator"));
    ASSERT_EQ(CombatClass::kPredator, Enum::StringToCombatClass("Predator"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("harbinger"));
    ASSERT_EQ(CombatClass::kHarbinger, Enum::StringToCombatClass("Harbinger"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("phantom"));
    ASSERT_EQ(CombatClass::kPhantom, Enum::StringToCombatClass("Phantom"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("invoker"));
    ASSERT_EQ(CombatClass::kInvoker, Enum::StringToCombatClass("Invoker"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("templar"));
    ASSERT_EQ(CombatClass::kTemplar, Enum::StringToCombatClass("Templar"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("aegis"));
    ASSERT_EQ(CombatClass::kAegis, Enum::StringToCombatClass("Aegis"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("revenant"));
    ASSERT_EQ(CombatClass::kRevenant, Enum::StringToCombatClass("Revenant"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("enchanter"));
    ASSERT_EQ(CombatClass::kEnchanter, Enum::StringToCombatClass("Enchanter"));
    ASSERT_EQ(CombatClass::kNone, Enum::StringToCombatClass("mystic"));
    ASSERT_EQ(CombatClass::kMystic, Enum::StringToCombatClass("Mystic"));
}

TEST(Enum, CombatAffinityToString)
{
    ASSERT_EQ("", fmt::format("{}", CombatAffinity::kNone));
    ASSERT_EQ("", fmt::format("{}", CombatAffinity::kNum));
    ASSERT_EQ("Water", fmt::format("{}", CombatAffinity::kWater));
    ASSERT_EQ("Earth", fmt::format("{}", CombatAffinity::kEarth));
    ASSERT_EQ("Fire", fmt::format("{}", CombatAffinity::kFire));
    ASSERT_EQ("Nature", fmt::format("{}", CombatAffinity::kNature));
    ASSERT_EQ("Air", fmt::format("{}", CombatAffinity::kAir));
    ASSERT_EQ("Tsunami", fmt::format("{}", CombatAffinity::kTsunami));
    ASSERT_EQ("Mud", fmt::format("{}", CombatAffinity::kMud));
    ASSERT_EQ("Steam", fmt::format("{}", CombatAffinity::kSteam));
    ASSERT_EQ("Toxic", fmt::format("{}", CombatAffinity::kToxic));
    ASSERT_EQ("Frost", fmt::format("{}", CombatAffinity::kFrost));
    ASSERT_EQ("Granite", fmt::format("{}", CombatAffinity::kGranite));
    ASSERT_EQ("Magma", fmt::format("{}", CombatAffinity::kMagma));
    ASSERT_EQ("Bloom", fmt::format("{}", CombatAffinity::kBloom));
    ASSERT_EQ("Dust", fmt::format("{}", CombatAffinity::kDust));
    ASSERT_EQ("Inferno", fmt::format("{}", CombatAffinity::kInferno));
    ASSERT_EQ("Wildfire", fmt::format("{}", CombatAffinity::kWildfire));
    ASSERT_EQ("Verdant", fmt::format("{}", CombatAffinity::kVerdant));
    ASSERT_EQ("Spore", fmt::format("{}", CombatAffinity::kSpore));
    ASSERT_EQ("Tempest", fmt::format("{}", CombatAffinity::kTempest));
}

TEST(Enum, StringToCombatAffinity)
{
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity(""));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("1"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("whatever"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("water"));
    ASSERT_EQ(CombatAffinity::kWater, Enum::StringToCombatAffinity("Water"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("earth"));
    ASSERT_EQ(CombatAffinity::kEarth, Enum::StringToCombatAffinity("Earth"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("fire"));
    ASSERT_EQ(CombatAffinity::kFire, Enum::StringToCombatAffinity("Fire"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("nature"));
    ASSERT_EQ(CombatAffinity::kNature, Enum::StringToCombatAffinity("Nature"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("air"));
    ASSERT_EQ(CombatAffinity::kAir, Enum::StringToCombatAffinity("Air"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("tsunami"));
    ASSERT_EQ(CombatAffinity::kTsunami, Enum::StringToCombatAffinity("Tsunami"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("mud"));
    ASSERT_EQ(CombatAffinity::kMud, Enum::StringToCombatAffinity("Mud"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("steam"));
    ASSERT_EQ(CombatAffinity::kSteam, Enum::StringToCombatAffinity("Steam"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("toxic"));
    ASSERT_EQ(CombatAffinity::kToxic, Enum::StringToCombatAffinity("Toxic"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("frost"));
    ASSERT_EQ(CombatAffinity::kFrost, Enum::StringToCombatAffinity("Frost"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("granite"));
    ASSERT_EQ(CombatAffinity::kGranite, Enum::StringToCombatAffinity("Granite"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("magma"));
    ASSERT_EQ(CombatAffinity::kMagma, Enum::StringToCombatAffinity("Magma"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("bloom"));
    ASSERT_EQ(CombatAffinity::kBloom, Enum::StringToCombatAffinity("Bloom"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("dust"));
    ASSERT_EQ(CombatAffinity::kDust, Enum::StringToCombatAffinity("Dust"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("inferno"));
    ASSERT_EQ(CombatAffinity::kInferno, Enum::StringToCombatAffinity("Inferno"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("wildfire"));
    ASSERT_EQ(CombatAffinity::kWildfire, Enum::StringToCombatAffinity("Wildfire"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("shock"));
    ASSERT_EQ(CombatAffinity::kShock, Enum::StringToCombatAffinity("Shock"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("verdant"));
    ASSERT_EQ(CombatAffinity::kVerdant, Enum::StringToCombatAffinity("Verdant"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("spore"));
    ASSERT_EQ(CombatAffinity::kSpore, Enum::StringToCombatAffinity("Spore"));
    ASSERT_EQ(CombatAffinity::kNone, Enum::StringToCombatAffinity("tempest"));
    ASSERT_EQ(CombatAffinity::kTempest, Enum::StringToCombatAffinity("Tempest"));
}

TEST(Enum, ZoneEffectShapeToString)
{
    ASSERT_EQ("", fmt::format("{}", ZoneEffectShape::kNone));
    ASSERT_EQ("", fmt::format("{}", ZoneEffectShape::kNum));
    ASSERT_EQ("Hexagon", fmt::format("{}", ZoneEffectShape::kHexagon));
    ASSERT_EQ("Rectangle", fmt::format("{}", ZoneEffectShape::kRectangle));
    ASSERT_EQ("Triangle", fmt::format("{}", ZoneEffectShape::kTriangle));
}

TEST(Enum, StringToZoneEffectShape)
{
    ASSERT_EQ(ZoneEffectShape::kNone, Enum::StringToZoneEffectShape(""));
    ASSERT_EQ(ZoneEffectShape::kNone, Enum::StringToZoneEffectShape("1"));
    ASSERT_EQ(ZoneEffectShape::kNone, Enum::StringToZoneEffectShape("whatever"));
    ASSERT_EQ(ZoneEffectShape::kNone, Enum::StringToZoneEffectShape("hexagon"));
    ASSERT_EQ(ZoneEffectShape::kHexagon, Enum::StringToZoneEffectShape("Hexagon"));
    ASSERT_EQ(ZoneEffectShape::kNone, Enum::StringToZoneEffectShape("rectangle"));
    ASSERT_EQ(ZoneEffectShape::kRectangle, Enum::StringToZoneEffectShape("Rectangle"));
    ASSERT_EQ(ZoneEffectShape::kNone, Enum::StringToZoneEffectShape("triangle"));
    ASSERT_EQ(ZoneEffectShape::kTriangle, Enum::StringToZoneEffectShape("Triangle"));
}

TEST(Enum, SkillTargetingTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", SkillTargetingType::kNone));
    ASSERT_EQ("", fmt::format("{}", SkillTargetingType::kNum));
    ASSERT_EQ("CurrentFocus", fmt::format("{}", SkillTargetingType::kCurrentFocus));
    ASSERT_EQ("Self", fmt::format("{}", SkillTargetingType::kSelf));
    ASSERT_EQ("InZone", fmt::format("{}", SkillTargetingType::kInZone));
    ASSERT_EQ("DistanceCheck", fmt::format("{}", SkillTargetingType::kDistanceCheck));
    ASSERT_EQ("CombatStatCheck", fmt::format("{}", SkillTargetingType::kCombatStatCheck));
    ASSERT_EQ("Allegiance", fmt::format("{}", SkillTargetingType::kAllegiance));
    ASSERT_EQ("Synergy", fmt::format("{}", SkillTargetingType::kSynergy));
    ASSERT_EQ("Vanquisher", fmt::format("{}", SkillTargetingType::kVanquisher));
    ASSERT_EQ("ExpressionCheck", fmt::format("{}", SkillTargetingType::kExpressionCheck));
    ASSERT_EQ("Tier", fmt::format("{}", SkillTargetingType::kTier));
}

TEST(Enum, StringToSkillTargetingType)
{
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType(""));
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType("1"));
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType("whatever"));
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType("currentfocus"));
    ASSERT_EQ(SkillTargetingType::kCurrentFocus, Enum::StringToSkillTargetingType("CurrentFocus"));
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType("self"));
    ASSERT_EQ(SkillTargetingType::kSelf, Enum::StringToSkillTargetingType("Self"));
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType("inzone"));
    ASSERT_EQ(SkillTargetingType::kInZone, Enum::StringToSkillTargetingType("InZone"));
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType("distancecheck"));
    ASSERT_EQ(SkillTargetingType::kDistanceCheck, Enum::StringToSkillTargetingType("DistanceCheck"));
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType("combatstatcheck"));
    ASSERT_EQ(SkillTargetingType::kCombatStatCheck, Enum::StringToSkillTargetingType("CombatStatCheck"));
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType("allegiance"));
    ASSERT_EQ(SkillTargetingType::kAllegiance, Enum::StringToSkillTargetingType("Allegiance"));
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType("synergy"));
    ASSERT_EQ(SkillTargetingType::kSynergy, Enum::StringToSkillTargetingType("Synergy"));
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType("vanquisher"));
    ASSERT_EQ(SkillTargetingType::kVanquisher, Enum::StringToSkillTargetingType("Vanquisher"));
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType("previoustargetlist"));
    ASSERT_EQ(SkillTargetingType::kPreviousTargetList, Enum::StringToSkillTargetingType("PreviousTargetList"));
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType("activator"));
    ASSERT_EQ(SkillTargetingType::kActivator, Enum::StringToSkillTargetingType("Activator"));
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType("expressioncheck"));
    ASSERT_EQ(SkillTargetingType::kExpressionCheck, Enum::StringToSkillTargetingType("ExpressionCheck"));
    ASSERT_EQ(SkillTargetingType::kNone, Enum::StringToSkillTargetingType("tier"));
    ASSERT_EQ(SkillTargetingType::kTier, Enum::StringToSkillTargetingType("Tier"));
}

TEST(Enum, SkillDeploymentTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", SkillDeploymentType::kNone));
    ASSERT_EQ("", fmt::format("{}", SkillDeploymentType::kNum));
    ASSERT_EQ("Direct", fmt::format("{}", SkillDeploymentType::kDirect));
    ASSERT_EQ("Zone", fmt::format("{}", SkillDeploymentType::kZone));
    ASSERT_EQ("Projectile", fmt::format("{}", SkillDeploymentType::kProjectile));
    ASSERT_EQ("Beam", fmt::format("{}", SkillDeploymentType::kBeam));
    ASSERT_EQ("Dash", fmt::format("{}", SkillDeploymentType::kDash));
    ASSERT_EQ("SpawnedCombatUnit", fmt::format("{}", SkillDeploymentType::kSpawnedCombatUnit));
}

TEST(Enum, StringToSkillDeploymentType)
{
    ASSERT_EQ(SkillDeploymentType::kNone, Enum::StringToSkillDeploymentType(""));
    ASSERT_EQ(SkillDeploymentType::kNone, Enum::StringToSkillDeploymentType("1"));
    ASSERT_EQ(SkillDeploymentType::kNone, Enum::StringToSkillDeploymentType("whatever"));
    ASSERT_EQ(SkillDeploymentType::kNone, Enum::StringToSkillDeploymentType("direct"));
    ASSERT_EQ(SkillDeploymentType::kDirect, Enum::StringToSkillDeploymentType("Direct"));
    ASSERT_EQ(SkillDeploymentType::kNone, Enum::StringToSkillDeploymentType("zone"));
    ASSERT_EQ(SkillDeploymentType::kZone, Enum::StringToSkillDeploymentType("Zone"));
    ASSERT_EQ(SkillDeploymentType::kNone, Enum::StringToSkillDeploymentType("projectile"));
    ASSERT_EQ(SkillDeploymentType::kProjectile, Enum::StringToSkillDeploymentType("Projectile"));
    ASSERT_EQ(SkillDeploymentType::kNone, Enum::StringToSkillDeploymentType("beam"));
    ASSERT_EQ(SkillDeploymentType::kBeam, Enum::StringToSkillDeploymentType("Beam"));
    ASSERT_EQ(SkillDeploymentType::kNone, Enum::StringToSkillDeploymentType("dash"));
    ASSERT_EQ(SkillDeploymentType::kDash, Enum::StringToSkillDeploymentType("Dash"));
    ASSERT_EQ(SkillDeploymentType::kNone, Enum::StringToSkillDeploymentType("spawnedcombatunit"));
    ASSERT_EQ(SkillDeploymentType::kSpawnedCombatUnit, Enum::StringToSkillDeploymentType("SpawnedCombatUnit"));
}

TEST(Enum, GuidanceTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", GuidanceType::kNone));
    ASSERT_EQ("", fmt::format("{}", GuidanceType::kNum));
    ASSERT_EQ("Ground", fmt::format("{}", GuidanceType::kGround));
    ASSERT_EQ("Airborne", fmt::format("{}", GuidanceType::kAirborne));
    ASSERT_EQ("Underground", fmt::format("{}", GuidanceType::kUnderground));
}

TEST(Enum, StringToGuidanceType)
{
    ASSERT_EQ(GuidanceType::kNone, Enum::StringToGuidanceType(""));
    ASSERT_EQ(GuidanceType::kNone, Enum::StringToGuidanceType("1"));
    ASSERT_EQ(GuidanceType::kNone, Enum::StringToGuidanceType("whatever"));
    ASSERT_EQ(GuidanceType::kNone, Enum::StringToGuidanceType("ground"));
    ASSERT_EQ(GuidanceType::kGround, Enum::StringToGuidanceType("Ground"));
    ASSERT_EQ(GuidanceType::kNone, Enum::StringToGuidanceType("airborne"));
    ASSERT_EQ(GuidanceType::kAirborne, Enum::StringToGuidanceType("Airborne"));
    ASSERT_EQ(GuidanceType::kNone, Enum::StringToGuidanceType("underground"));
    ASSERT_EQ(GuidanceType::kUnderground, Enum::StringToGuidanceType("Underground"));
}

TEST(Enum, HexGridCardinalDirectionToString)
{
    ASSERT_EQ("", fmt::format("{}", HexGridCardinalDirection::kNone));
    ASSERT_EQ("", fmt::format("{}", HexGridCardinalDirection::kNum));
    ASSERT_EQ("TopLeft", fmt::format("{}", HexGridCardinalDirection::kTopLeft));
    ASSERT_EQ("TopRight", fmt::format("{}", HexGridCardinalDirection::kTopRight));
    ASSERT_EQ("Right", fmt::format("{}", HexGridCardinalDirection::kRight));
    ASSERT_EQ("BottomRight", fmt::format("{}", HexGridCardinalDirection::kBottomRight));
    ASSERT_EQ("BottomLeft", fmt::format("{}", HexGridCardinalDirection::kBottomLeft));
    ASSERT_EQ("Left", fmt::format("{}", HexGridCardinalDirection::kLeft));
}

TEST(Enum, StringToHexGridCardinalDirection)
{
    ASSERT_EQ(HexGridCardinalDirection::kNone, Enum::StringToHexGridCardinalDirection(""));
    ASSERT_EQ(HexGridCardinalDirection::kNone, Enum::StringToHexGridCardinalDirection("1"));
    ASSERT_EQ(HexGridCardinalDirection::kNone, Enum::StringToHexGridCardinalDirection("whatever"));
    ASSERT_EQ(HexGridCardinalDirection::kNone, Enum::StringToHexGridCardinalDirection("topleft"));
    ASSERT_EQ(HexGridCardinalDirection::kTopLeft, Enum::StringToHexGridCardinalDirection("TopLeft"));
    ASSERT_EQ(HexGridCardinalDirection::kNone, Enum::StringToHexGridCardinalDirection("topright"));
    ASSERT_EQ(HexGridCardinalDirection::kTopRight, Enum::StringToHexGridCardinalDirection("TopRight"));
    ASSERT_EQ(HexGridCardinalDirection::kNone, Enum::StringToHexGridCardinalDirection("right"));
    ASSERT_EQ(HexGridCardinalDirection::kRight, Enum::StringToHexGridCardinalDirection("Right"));
    ASSERT_EQ(HexGridCardinalDirection::kNone, Enum::StringToHexGridCardinalDirection("bottomright"));
    ASSERT_EQ(HexGridCardinalDirection::kBottomRight, Enum::StringToHexGridCardinalDirection("BottomRight"));
    ASSERT_EQ(HexGridCardinalDirection::kNone, Enum::StringToHexGridCardinalDirection("bottomleft"));
    ASSERT_EQ(HexGridCardinalDirection::kBottomLeft, Enum::StringToHexGridCardinalDirection("BottomLeft"));
    ASSERT_EQ(HexGridCardinalDirection::kNone, Enum::StringToHexGridCardinalDirection("left"));
    ASSERT_EQ(HexGridCardinalDirection::kLeft, Enum::StringToHexGridCardinalDirection("Left"));
}

TEST(Enum, StringToActivationTriggerType)
{
    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType(""));
    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("1"));

    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("onbattlestart"));
    ASSERT_EQ(ActivationTriggerType::kOnBattleStart, Enum::StringToActivationTriggerType("OnBattleStart"));

    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("everyxtime"));
    ASSERT_EQ(ActivationTriggerType::kEveryXTime, Enum::StringToActivationTriggerType("EveryXTime"));

    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("ondodge"));
    ASSERT_EQ(ActivationTriggerType::kOnDodge, Enum::StringToActivationTriggerType("OnDodge"));

    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("ondealcrit"));
    ASSERT_EQ(ActivationTriggerType::kOnDealCrit, Enum::StringToActivationTriggerType("OnDealCrit"));

    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("onhit"));
    ASSERT_EQ(ActivationTriggerType::kOnHit, Enum::StringToActivationTriggerType("OnHit"));

    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("onshieldhit"));
    ASSERT_EQ(ActivationTriggerType::kOnShieldHit, Enum::StringToActivationTriggerType("OnShieldHit"));

    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("onmiss"));
    ASSERT_EQ(ActivationTriggerType::kOnMiss, Enum::StringToActivationTriggerType("OnMiss"));

    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("ondamage"));
    ASSERT_EQ(ActivationTriggerType::kOnDamage, Enum::StringToActivationTriggerType("OnDamage"));

    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("onvanquish"));
    ASSERT_EQ(ActivationTriggerType::kOnVanquish, Enum::StringToActivationTriggerType("OnVanquish"));

    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("onfaint"));
    ASSERT_EQ(ActivationTriggerType::kOnFaint, Enum::StringToActivationTriggerType("OnFaint"));

    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("onassist"));
    ASSERT_EQ(ActivationTriggerType::kOnAssist, Enum::StringToActivationTriggerType("OnAssist"));

    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("healthinrange"));
    ASSERT_EQ(ActivationTriggerType::kHealthInRange, Enum::StringToActivationTriggerType("HealthInRange"));

    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("onactivatexabilities"));
    ASSERT_EQ(
        ActivationTriggerType::kOnActivateXAbilities,
        Enum::StringToActivationTriggerType("OnActivateXAbilities"));

    ASSERT_EQ(ActivationTriggerType::kNone, Enum::StringToActivationTriggerType("inrange"));
    ASSERT_EQ(ActivationTriggerType::kInRange, Enum::StringToActivationTriggerType("InRange"));
}

TEST(Enum, ActivationTriggerTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", ActivationTriggerType::kNone));
    ASSERT_EQ("", fmt::format("{}", ActivationTriggerType::kNum));
    ASSERT_EQ("OnBattleStart", fmt::format("{}", ActivationTriggerType::kOnBattleStart));
    ASSERT_EQ("EveryXTime", fmt::format("{}", ActivationTriggerType::kEveryXTime));
    ASSERT_EQ("OnDodge", fmt::format("{}", ActivationTriggerType::kOnDodge));
    ASSERT_EQ("OnHit", fmt::format("{}", ActivationTriggerType::kOnHit));
    ASSERT_EQ("OnMiss", fmt::format("{}", ActivationTriggerType::kOnMiss));
    ASSERT_EQ("OnDamage", fmt::format("{}", ActivationTriggerType::kOnDamage));
    ASSERT_EQ("OnVanquish", fmt::format("{}", ActivationTriggerType::kOnVanquish));
    ASSERT_EQ("OnFaint", fmt::format("{}", ActivationTriggerType::kOnFaint));
    ASSERT_EQ("OnAssist", fmt::format("{}", ActivationTriggerType::kOnAssist));
    ASSERT_EQ("HealthInRange", fmt::format("{}", ActivationTriggerType::kHealthInRange));
    ASSERT_EQ("OnActivateXAbilities", fmt::format("{}", ActivationTriggerType::kOnActivateXAbilities));
    ASSERT_EQ("InRange", fmt::format("{}", ActivationTriggerType::kInRange));
}

TEST(Enum, StatTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", StatType::kNone));
    ASSERT_EQ("MoveSpeedSubUnits", fmt::format("{}", StatType::kMoveSpeedSubUnits));
    ASSERT_EQ("AttackRangeUnits", fmt::format("{}", StatType::kAttackRangeUnits));
    ASSERT_EQ("OmegaRangeUnits", fmt::format("{}", StatType::kOmegaRangeUnits));
    ASSERT_EQ("HitChancePercentage", fmt::format("{}", StatType::kHitChancePercentage));
    ASSERT_EQ("AttackSpeed", fmt::format("{}", StatType::kAttackSpeed));
    ASSERT_EQ("MaxHealth", fmt::format("{}", StatType::kMaxHealth));
    ASSERT_EQ("CurrentHealth", fmt::format("{}", StatType::kCurrentHealth));
    ASSERT_EQ("EnergyCost", fmt::format("{}", StatType::kEnergyCost));
    ASSERT_EQ("StartingEnergy", fmt::format("{}", StatType::kStartingEnergy));
    ASSERT_EQ("CurrentEnergy", fmt::format("{}", StatType::kCurrentEnergy));
    ASSERT_EQ("EnergyRegeneration", fmt::format("{}", StatType::kEnergyRegeneration));
    ASSERT_EQ("HealthRegeneration", fmt::format("{}", StatType::kHealthRegeneration));
    ASSERT_EQ("EnergyGainEfficiencyPercentage", fmt::format("{}", StatType::kEnergyGainEfficiencyPercentage));
    ASSERT_EQ("OnActivationEnergy", fmt::format("{}", StatType::kOnActivationEnergy));
    ASSERT_EQ("PhysicalResist", fmt::format("{}", StatType::kPhysicalResist));
    ASSERT_EQ("EnergyResist", fmt::format("{}", StatType::kEnergyResist));
    ASSERT_EQ("TenacityPercentage", fmt::format("{}", StatType::kTenacityPercentage));
    ASSERT_EQ("WillpowerPercentage", fmt::format("{}", StatType::kWillpowerPercentage));
    ASSERT_EQ("Grit", fmt::format("{}", StatType::kGrit));
    ASSERT_EQ("Resolve", fmt::format("{}", StatType::kResolve));
    ASSERT_EQ("VulnerabilityPercentage", fmt::format("{}", StatType::kVulnerabilityPercentage));
    ASSERT_EQ("AttackDodgeChancePercentage", fmt::format("{}", StatType::kAttackDodgeChancePercentage));
    ASSERT_EQ("AttackDamage", fmt::format("{}", StatType::kAttackDamage));
    ASSERT_EQ("AttackPhysicalDamage", fmt::format("{}", StatType::kAttackPhysicalDamage));
    ASSERT_EQ("AttackEnergyDamage", fmt::format("{}", StatType::kAttackEnergyDamage));
    ASSERT_EQ("AttackPureDamage", fmt::format("{}", StatType::kAttackPureDamage));
    ASSERT_EQ("CritChancePercentage", fmt::format("{}", StatType::kCritChancePercentage));
    ASSERT_EQ("CritAmplificationPercentage", fmt::format("{}", StatType::kCritAmplificationPercentage));
    ASSERT_EQ("OmegaPowerPercentage", fmt::format("{}", StatType::kOmegaPowerPercentage));
    ASSERT_EQ("HealthGainEfficiencyPercentage", fmt::format("{}", StatType::kHealthGainEfficiencyPercentage));
    ASSERT_EQ("PhysicalVampPercentage", fmt::format("{}", StatType::kPhysicalVampPercentage));
    ASSERT_EQ("PureVampPercentage", fmt::format("{}", StatType::kPureVampPercentage));
    ASSERT_EQ("EnergyVampPercentage", fmt::format("{}", StatType::kEnergyVampPercentage));
    ASSERT_EQ("EnergyPiercingPercentage", fmt::format("{}", StatType::kEnergyPiercingPercentage));
    ASSERT_EQ("PhysicalPiercingPercentage", fmt::format("{}", StatType::kPhysicalPiercingPercentage));
    ASSERT_EQ("Thorns", fmt::format("{}", StatType::kThorns));
    ASSERT_EQ("StartingShield", fmt::format("{}", StatType::kStartingShield));
    ASSERT_EQ("CritReductionPercentage", fmt::format("{}", StatType::kCritReductionPercentage));
}

TEST(Enum, StringToStatType)
{
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType(""));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("movespeedsubunits"));
    ASSERT_EQ(StatType::kMoveSpeedSubUnits, Enum::StringToStatType("MoveSpeedSubUnits"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("attackrangeunits"));
    ASSERT_EQ(StatType::kAttackRangeUnits, Enum::StringToStatType("AttackRangeUnits"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("omegarangeunits"));
    ASSERT_EQ(StatType::kOmegaRangeUnits, Enum::StringToStatType("OmegaRangeUnits"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("hitchancepercentage"));
    ASSERT_EQ(StatType::kHitChancePercentage, Enum::StringToStatType("HitChancePercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("attackspeed"));
    ASSERT_EQ(StatType::kAttackSpeed, Enum::StringToStatType("AttackSpeed"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("maxhealth"));
    ASSERT_EQ(StatType::kMaxHealth, Enum::StringToStatType("MaxHealth"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("currenthealth"));
    ASSERT_EQ(StatType::kCurrentHealth, Enum::StringToStatType("CurrentHealth"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("energycost"));
    ASSERT_EQ(StatType::kEnergyCost, Enum::StringToStatType("EnergyCost"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("startingenergy"));
    ASSERT_EQ(StatType::kStartingEnergy, Enum::StringToStatType("StartingEnergy"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("currentenergy"));
    ASSERT_EQ(StatType::kCurrentEnergy, Enum::StringToStatType("CurrentEnergy"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("energyregeneration"));
    ASSERT_EQ(StatType::kEnergyRegeneration, Enum::StringToStatType("EnergyRegeneration"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("healthregeneration"));
    ASSERT_EQ(StatType::kHealthRegeneration, Enum::StringToStatType("HealthRegeneration"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("energygainefficiencypercentage"));
    ASSERT_EQ(StatType::kEnergyGainEfficiencyPercentage, Enum::StringToStatType("EnergyGainEfficiencyPercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("onactivationenergy"));
    ASSERT_EQ(StatType::kOnActivationEnergy, Enum::StringToStatType("OnActivationEnergy"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("physicalresist"));
    ASSERT_EQ(StatType::kPhysicalResist, Enum::StringToStatType("PhysicalResist"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("energyresist"));
    ASSERT_EQ(StatType::kEnergyResist, Enum::StringToStatType("EnergyResist"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("tenacitypercentage"));
    ASSERT_EQ(StatType::kTenacityPercentage, Enum::StringToStatType("TenacityPercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("willpowerpercentage"));
    ASSERT_EQ(StatType::kWillpowerPercentage, Enum::StringToStatType("WillpowerPercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("grit"));
    ASSERT_EQ(StatType::kGrit, Enum::StringToStatType("Grit"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("resolve"));
    ASSERT_EQ(StatType::kResolve, Enum::StringToStatType("Resolve"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("vulnerabilitypercentage"));
    ASSERT_EQ(StatType::kVulnerabilityPercentage, Enum::StringToStatType("VulnerabilityPercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("attackdodgechancepercentage"));
    ASSERT_EQ(StatType::kAttackDodgeChancePercentage, Enum::StringToStatType("AttackDodgeChancePercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("attackdamage"));
    ASSERT_EQ(StatType::kAttackDamage, Enum::StringToStatType("AttackDamage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("attackphysicaldamage"));
    ASSERT_EQ(StatType::kAttackPhysicalDamage, Enum::StringToStatType("AttackPhysicalDamage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("attackenergydamage"));
    ASSERT_EQ(StatType::kAttackEnergyDamage, Enum::StringToStatType("AttackEnergyDamage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("attackpuredamage"));
    ASSERT_EQ(StatType::kAttackPureDamage, Enum::StringToStatType("AttackPureDamage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("critchancepercentage"));
    ASSERT_EQ(StatType::kCritChancePercentage, Enum::StringToStatType("CritChancePercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("critamplificationpercentage"));
    ASSERT_EQ(StatType::kCritAmplificationPercentage, Enum::StringToStatType("CritAmplificationPercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("omegapowerpercentage"));
    ASSERT_EQ(StatType::kOmegaPowerPercentage, Enum::StringToStatType("OmegaPowerPercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("omegadamagepercentage"));
    ASSERT_EQ(StatType::kOmegaDamagePercentage, Enum::StringToStatType("OmegaDamagePercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("healthgainefficiencypercentage"));
    ASSERT_EQ(StatType::kHealthGainEfficiencyPercentage, Enum::StringToStatType("HealthGainEfficiencyPercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("physicalvampPercentage"));
    ASSERT_EQ(StatType::kPhysicalVampPercentage, Enum::StringToStatType("PhysicalVampPercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("purevamppercentage"));
    ASSERT_EQ(StatType::kPureVampPercentage, Enum::StringToStatType("PureVampPercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("energyvamppercentage"));
    ASSERT_EQ(StatType::kEnergyVampPercentage, Enum::StringToStatType("EnergyVampPercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("energypiercingpercentage"));
    ASSERT_EQ(StatType::kEnergyPiercingPercentage, Enum::StringToStatType("EnergyPiercingPercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("physicalpiercingpercentage"));
    ASSERT_EQ(StatType::kPhysicalPiercingPercentage, Enum::StringToStatType("PhysicalPiercingPercentage"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("thorns"));
    ASSERT_EQ(StatType::kThorns, Enum::StringToStatType("Thorns"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("startingshield"));
    ASSERT_EQ(StatType::kStartingShield, Enum::StringToStatType("StartingShield"));
    ASSERT_EQ(StatType::kNone, Enum::StringToStatType("critreductionpercentage"));
    ASSERT_EQ(StatType::kCritReductionPercentage, Enum::StringToStatType("CritReductionPercentage"));
}

TEST(Enum, StatEvaluationTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", StatEvaluationType::kNone));
    ASSERT_EQ("Live", fmt::format("{}", StatEvaluationType::kLive));
    ASSERT_EQ("Base", fmt::format("{}", StatEvaluationType::kBase));
    ASSERT_EQ("Bonus", fmt::format("{}", StatEvaluationType::kBonus));
}

TEST(Enum, StringToStatEvaluationType)
{
    ASSERT_EQ(StatEvaluationType::kNone, Enum::StringToStatEvaluationType(""));
    ASSERT_EQ(StatEvaluationType::kNone, Enum::StringToStatEvaluationType("live"));
    ASSERT_EQ(StatEvaluationType::kLive, Enum::StringToStatEvaluationType("Live"));
    ASSERT_EQ(StatEvaluationType::kNone, Enum::StringToStatEvaluationType("base"));
    ASSERT_EQ(StatEvaluationType::kBase, Enum::StringToStatEvaluationType("Base"));
    ASSERT_EQ(StatEvaluationType::kNone, Enum::StringToStatEvaluationType("bonus"));
    ASSERT_EQ(StatEvaluationType::kBonus, Enum::StringToStatEvaluationType("Bonus"));
}

TEST(Enum, AbilitySelectionTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", AbilitySelectionType::kNone));
    ASSERT_EQ("", fmt::format("{}", AbilitySelectionType::kNum));
    ASSERT_EQ("Cycle", fmt::format("{}", AbilitySelectionType::kCycle));
    ASSERT_EQ("SelfHealthCheck", fmt::format("{}", AbilitySelectionType::kSelfHealthCheck));
    ASSERT_EQ("SelfAttributeCheck", fmt::format("{}", AbilitySelectionType::kSelfAttributeCheck));
    ASSERT_EQ("EveryXActivations", fmt::format("{}", AbilitySelectionType::kEveryXActivations));
}

TEST(Enum, StringToAbilitySelectionType)
{
    ASSERT_EQ(AbilitySelectionType::kNone, Enum::StringToAbilitySelectionType(""));
    ASSERT_EQ(AbilitySelectionType::kNone, Enum::StringToAbilitySelectionType("1"));
    ASSERT_EQ(AbilitySelectionType::kNone, Enum::StringToAbilitySelectionType("whatever"));
    ASSERT_EQ(AbilitySelectionType::kNone, Enum::StringToAbilitySelectionType("cycle"));
    ASSERT_EQ(AbilitySelectionType::kCycle, Enum::StringToAbilitySelectionType("Cycle"));
    ASSERT_EQ(AbilitySelectionType::kNone, Enum::StringToAbilitySelectionType("selfhealthcheck"));
    ASSERT_EQ(AbilitySelectionType::kSelfHealthCheck, Enum::StringToAbilitySelectionType("SelfHealthCheck"));
    ASSERT_EQ(AbilitySelectionType::kNone, Enum::StringToAbilitySelectionType("selfattributecheck"));
    ASSERT_EQ(AbilitySelectionType::kSelfAttributeCheck, Enum::StringToAbilitySelectionType("SelfAttributeCheck"));
    ASSERT_EQ(AbilitySelectionType::kNone, Enum::StringToAbilitySelectionType("everyxactivations"));
    ASSERT_EQ(AbilitySelectionType::kEveryXActivations, Enum::StringToAbilitySelectionType("EveryXActivations"));
}

TEST(Enum, AllegianceTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", AllegianceType::kNum));
    ASSERT_EQ("", fmt::format("{}", AllegianceType::kNone));
    ASSERT_EQ("Ally", fmt::format("{}", AllegianceType::kAlly));
    ASSERT_EQ("Enemy", fmt::format("{}", AllegianceType::kEnemy));
    ASSERT_EQ("All", fmt::format("{}", AllegianceType::kAll));
}

TEST(Enum, StringToAllegianceType)
{
    ASSERT_EQ(AllegianceType::kNone, Enum::StringToAllegianceType(""));
    ASSERT_EQ(AllegianceType::kNone, Enum::StringToAllegianceType("1"));
    ASSERT_EQ(AllegianceType::kNone, Enum::StringToAllegianceType("whatever"));
    ASSERT_EQ(AllegianceType::kNone, Enum::StringToAllegianceType("all"));
    ASSERT_EQ(AllegianceType::kAll, Enum::StringToAllegianceType("All"));
    ASSERT_EQ(AllegianceType::kNone, Enum::StringToAllegianceType("ally"));
    ASSERT_EQ(AllegianceType::kAlly, Enum::StringToAllegianceType("Ally"));
    ASSERT_EQ(AllegianceType::kNone, Enum::StringToAllegianceType("enemy"));
    ASSERT_EQ(AllegianceType::kEnemy, Enum::StringToAllegianceType("Enemy"));
}

TEST(Enum, CombatUnitStateToString)
{
    ASSERT_EQ("", fmt::format("{}", CombatUnitState::kNone));
    ASSERT_EQ("", fmt::format("{}", CombatUnitState::kNum));
    ASSERT_EQ("Alive", fmt::format("{}", CombatUnitState::kAlive));
    ASSERT_EQ("Fainted", fmt::format("{}", CombatUnitState::kFainted));
    ASSERT_EQ("Disappeared", fmt::format("{}", CombatUnitState::kDisappeared));
}

TEST(Enum, StringToCombatUnitState)
{
    ASSERT_EQ(CombatUnitState::kNone, Enum::StringToCombatUnitState(""));
    ASSERT_EQ(CombatUnitState::kNone, Enum::StringToCombatUnitState("1"));
    ASSERT_EQ(CombatUnitState::kNone, Enum::StringToCombatUnitState("whatever"));
    ASSERT_EQ(CombatUnitState::kNone, Enum::StringToCombatUnitState("alive"));
    ASSERT_EQ(CombatUnitState::kAlive, Enum::StringToCombatUnitState("Alive"));
    ASSERT_EQ(CombatUnitState::kNone, Enum::StringToCombatUnitState("fainted"));
    ASSERT_EQ(CombatUnitState::kFainted, Enum::StringToCombatUnitState("Fainted"));
    ASSERT_EQ(CombatUnitState::kNone, Enum::StringToCombatUnitState("disappeared"));
    ASSERT_EQ(CombatUnitState::kDisappeared, Enum::StringToCombatUnitState("Disappeared"));
}

TEST(Enum, ReservedPositionTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", ReservedPositionType::kNone));
    ASSERT_EQ("", fmt::format("{}", ReservedPositionType::kNum));
    ASSERT_EQ("Across", fmt::format("{}", ReservedPositionType::kAcross));
    ASSERT_EQ("NearReceiver", fmt::format("{}", ReservedPositionType::kNearReceiver));
    ASSERT_EQ("BehindReceiver", fmt::format("{}", ReservedPositionType::kBehindReceiver));
}

TEST(Enum, StringToReservedPositionType)
{
    ASSERT_EQ(ReservedPositionType::kNone, Enum::StringToReservedPositionType(""));
    ASSERT_EQ(ReservedPositionType::kNone, Enum::StringToReservedPositionType("1"));
    ASSERT_EQ(ReservedPositionType::kNone, Enum::StringToReservedPositionType("whatever"));
    ASSERT_EQ(ReservedPositionType::kNone, Enum::StringToReservedPositionType("across"));
    ASSERT_EQ(ReservedPositionType::kAcross, Enum::StringToReservedPositionType("Across"));
    ASSERT_EQ(ReservedPositionType::kNone, Enum::StringToReservedPositionType("nearreceiver"));
    ASSERT_EQ(ReservedPositionType::kNearReceiver, Enum::StringToReservedPositionType("NearReceiver"));
    ASSERT_EQ(ReservedPositionType::kNone, Enum::StringToReservedPositionType("behindreceiver"));
    ASSERT_EQ(ReservedPositionType::kBehindReceiver, Enum::StringToReservedPositionType("BehindReceiver"));
}

TEST(Enum, AttachedEntityTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", AttachedEntityType::kNone));
    ASSERT_EQ("", fmt::format("{}", AttachedEntityType::kNum));
    ASSERT_EQ("Shield", fmt::format("{}", AttachedEntityType::kShield));
    ASSERT_EQ("Mark", fmt::format("{}", AttachedEntityType::kMark));
}

TEST(Enum, StringToAttachedEntityType)
{
    ASSERT_EQ(AttachedEntityType::kNone, Enum::StringToAttachedEntityType(""));
    ASSERT_EQ(AttachedEntityType::kNone, Enum::StringToAttachedEntityType("1"));
    ASSERT_EQ(AttachedEntityType::kNone, Enum::StringToAttachedEntityType("whatever"));
    ASSERT_EQ(AttachedEntityType::kNone, Enum::StringToAttachedEntityType("shield"));
    ASSERT_EQ(AttachedEntityType::kShield, Enum::StringToAttachedEntityType("Shield"));
    ASSERT_EQ(AttachedEntityType::kNone, Enum::StringToAttachedEntityType("mark"));
    ASSERT_EQ(AttachedEntityType::kMark, Enum::StringToAttachedEntityType("Mark"));
}

TEST(Enum, ValidationTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", ValidationType::kNone));
    ASSERT_EQ("", fmt::format("{}", ValidationType::kNum));
    ASSERT_EQ("DistanceCheck", fmt::format("{}", ValidationType::kDistanceCheck));
    ASSERT_EQ("Expression", fmt::format("{}", ValidationType::kExpression));
}

TEST(Enum, StringToValidationType)
{
    ASSERT_EQ(ValidationType::kNone, Enum::StringToValidationType(""));
    ASSERT_EQ(ValidationType::kNone, Enum::StringToValidationType("1"));
    ASSERT_EQ(ValidationType::kNone, Enum::StringToValidationType("whatever"));
    ASSERT_EQ(ValidationType::kNone, Enum::StringToValidationType("distancecheck"));
    ASSERT_EQ(ValidationType::kDistanceCheck, Enum::StringToValidationType("DistanceCheck"));
    ASSERT_EQ(ValidationType::kNone, Enum::StringToValidationType("expression"));
    ASSERT_EQ(ValidationType::kExpression, Enum::StringToValidationType("Expression"));
}

TEST(Enum, StartEntityTypeToString)
{
    ASSERT_EQ("", fmt::format("{}", ValidationStartEntity::kNone));
    ASSERT_EQ("", fmt::format("{}", ValidationStartEntity::kNum));
    ASSERT_EQ("Self", fmt::format("{}", ValidationStartEntity::kSelf));
    ASSERT_EQ("CurrentFocus", fmt::format("{}", ValidationStartEntity::kCurrentFocus));
}

TEST(Enum, StringToStartEntity)
{
    ASSERT_EQ(ValidationStartEntity::kNone, Enum::StringToValidationStartEntity(""));
    ASSERT_EQ(ValidationStartEntity::kNone, Enum::StringToValidationStartEntity("1"));
    ASSERT_EQ(ValidationStartEntity::kNone, Enum::StringToValidationStartEntity("whatever"));
    ASSERT_EQ(ValidationStartEntity::kNone, Enum::StringToValidationStartEntity("self"));
    ASSERT_EQ(ValidationStartEntity::kSelf, Enum::StringToValidationStartEntity("Self"));
    ASSERT_EQ(ValidationStartEntity::kNone, Enum::StringToValidationStartEntity("currentfocus"));
    ASSERT_EQ(ValidationStartEntity::kCurrentFocus, Enum::StringToValidationStartEntity("CurrentFocus"));
}

TEST(Enum, PredefinedGridPositionToString)
{
    ASSERT_EQ(Enum::PredefinedGridPositionToString(PredefinedGridPosition::kNone), "");
    ASSERT_EQ(Enum::PredefinedGridPositionToString(PredefinedGridPosition::kAllyBorderCenter), "AllyBorderCenter");
    ASSERT_EQ(Enum::PredefinedGridPositionToString(PredefinedGridPosition::kEnemyBorderCenter), "EnemyBorderCenter");
}

TEST(Enum, StringToPredefinedGridPosition)
{
    ASSERT_EQ(PredefinedGridPosition::kNone, Enum::StringToPredefinedGridPosition(""));
    ASSERT_EQ(PredefinedGridPosition::kNone, Enum::StringToPredefinedGridPosition("1"));
    ASSERT_EQ(PredefinedGridPosition::kNone, Enum::StringToPredefinedGridPosition("whatever"));
    ASSERT_EQ(PredefinedGridPosition::kAllyBorderCenter, Enum::StringToPredefinedGridPosition("AllyBorderCenter"));
    ASSERT_EQ(PredefinedGridPosition::kEnemyBorderCenter, Enum::StringToPredefinedGridPosition("EnemyBorderCenter"));
}

}  // namespace simulation
