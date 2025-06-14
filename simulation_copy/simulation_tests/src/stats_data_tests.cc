#include "base_test_fixtures.h"
#include "components/stats_component.h"
#include "data/stats_data.h"
#include "ecs/world.h"
#include "gtest/gtest.h"

namespace simulation
{
class StatsDataTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        data.Set(StatType::kMoveSpeedSubUnits, GetCounter());
        data.Set(StatType::kAttackRangeUnits, GetCounter());
        data.Set(StatType::kOmegaRangeUnits, GetCounter());
        data.Set(StatType::kAttackSpeed, GetCounter());
        data.Set(StatType::kHitChancePercentage, GetCounter());
        data.Set(StatType::kAttackDodgeChancePercentage, GetCounter());
        data.Set(StatType::kAttackPhysicalDamage, GetCounter());
        data.Set(StatType::kAttackEnergyDamage, GetCounter());
        data.Set(StatType::kAttackPureDamage, GetCounter());
        data.Set(StatType::kMaxHealth, GetCounter());
        data.Set(StatType::kCurrentHealth, GetCounter());
        data.Set(StatType::kEnergyCost, GetCounter());
        data.Set(StatType::kStartingEnergy, GetCounter());
        data.Set(StatType::kCurrentEnergy, GetCounter());
        data.Set(StatType::kPhysicalResist, GetCounter());
        data.Set(StatType::kEnergyResist, GetCounter());
        data.Set(StatType::kTenacityPercentage, GetCounter());
        data.Set(StatType::kWillpowerPercentage, GetCounter());
        data.Set(StatType::kGrit, GetCounter());
        data.Set(StatType::kResolve, GetCounter());
        data.Set(StatType::kVulnerabilityPercentage, GetCounter());
        data.Set(StatType::kCritChancePercentage, GetCounter());
        data.Set(StatType::kCritAmplificationPercentage, GetCounter());
        data.Set(StatType::kOmegaPowerPercentage, GetCounter());
        data.Set(StatType::kEnergyGainEfficiencyPercentage, GetCounter());
        data.Set(StatType::kHealthGainEfficiencyPercentage, GetCounter());
        data.Set(StatType::kOnActivationEnergy, GetCounter());
        data.Set(StatType::kPhysicalVampPercentage, GetCounter());
        data.Set(StatType::kPureVampPercentage, GetCounter());
        data.Set(StatType::kEnergyVampPercentage, GetCounter());
        data.Set(StatType::kEnergyPiercingPercentage, GetCounter());
        data.Set(StatType::kPhysicalPiercingPercentage, GetCounter());
        data.Set(StatType::kThorns, GetCounter());
        data.Set(StatType::kStartingShield, GetCounter());
    }

    FixedPoint GetCounter()
    {
        const FixedPoint counter = combat_class_counter;
        combat_class_counter += 1_fp;
        return counter;
    }

    std::shared_ptr<World> world;
    Entity* blue_entity = nullptr;
    FixedPoint combat_class_counter = 1_fp;
    StatsData data;
};

class StatOmegaRangeTest : public BaseTest
{
    typedef BaseTest Super;

protected:
    virtual void InitCombatUnitData()
    {
        InitStats();
    }

    virtual void InitStats()
    {
        data = CreateCombatUnitData();

        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 15_fp);
        data.type_data.stats.Set(StatType::kOmegaRangeUnits, 5_fp);
    }

    virtual void SpawnCombatUnits()
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
        SpawnCombatUnits();
    }

    Entity* blue_entity = nullptr;
    CombatUnitData data;
};

class StatMoveSpeedTest : public BaseTest
{
    typedef BaseTest Super;

protected:
    virtual void InitCombatUnitData()
    {
        InitStats();
    }

    virtual void InitStats()
    {
        data = CreateCombatUnitData();

        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 15_fp);
        data.type_data.stats.Set(StatType::kOmegaRangeUnits, 5_fp);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 1000_fp);
    }

    virtual void SpawnCombatUnits()
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
        SpawnCombatUnits();
    }

    Entity* blue_entity = nullptr;
    CombatUnitData data;
};

TEST_F(StatsDataTest, Get)
{
    FixedPoint counter = 1_fp;
    EXPECT_EQ(data.Get(StatType::kNone), 0_fp);
    EXPECT_EQ(data.Get(StatType::kNum), 0_fp);
    EXPECT_EQ(data.Get(StatType::kMoveSpeedSubUnits), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kAttackRangeUnits), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kOmegaRangeUnits), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kAttackSpeed), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kHitChancePercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kAttackDodgeChancePercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kAttackPhysicalDamage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kAttackEnergyDamage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kAttackPureDamage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kMaxHealth), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kCurrentHealth), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kEnergyCost), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kStartingEnergy), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kCurrentEnergy), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kPhysicalResist), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kEnergyResist), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kTenacityPercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kWillpowerPercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kGrit), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kResolve), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kVulnerabilityPercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kCritChancePercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kCritAmplificationPercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kOmegaPowerPercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kEnergyGainEfficiencyPercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kHealthGainEfficiencyPercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kOnActivationEnergy), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kPhysicalVampPercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kPureVampPercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kEnergyVampPercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kEnergyPiercingPercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kPhysicalPiercingPercentage), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kThorns), counter);
    counter += 1_fp;
    EXPECT_EQ(data.Get(StatType::kStartingShield), counter);
}

TEST_F(StatsDataTest, Set)  // NOLINT (too big test)
{
    FixedPoint counter = 1_fp;
    // EXPECT_TRUE(data.Get(StatType::kNone) == nullptr);
    // EXPECT_TRUE(data.Get(StatType::kNum) == nullptr);

    const FixedPoint modify_by = 42_fp;
    {
        const FixedPoint old_value = data.Get(StatType::kMoveSpeedSubUnits);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kMoveSpeedSubUnits));

        // Modify
        data.Set(StatType::kMoveSpeedSubUnits, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kMoveSpeedSubUnits), data.Get(StatType::kMoveSpeedSubUnits));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kAttackRangeUnits);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kAttackRangeUnits));

        // Modify
        data.Set(StatType::kAttackRangeUnits, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kAttackRangeUnits), data.Get(StatType::kAttackRangeUnits));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kOmegaRangeUnits);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kOmegaRangeUnits));

        // Modify
        data.Set(StatType::kOmegaRangeUnits, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kOmegaRangeUnits), data.Get(StatType::kOmegaRangeUnits));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kAttackSpeed);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kAttackSpeed));

        // Modify
        data.Set(StatType::kAttackSpeed, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kAttackSpeed), data.Get(StatType::kAttackSpeed));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kHitChancePercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kHitChancePercentage));

        // Modify
        data.Set(StatType::kHitChancePercentage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kHitChancePercentage), data.Get(StatType::kHitChancePercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kAttackDodgeChancePercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kAttackDodgeChancePercentage));

        // Modify
        data.Set(StatType::kAttackDodgeChancePercentage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kAttackDodgeChancePercentage), data.Get(StatType::kAttackDodgeChancePercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kAttackPhysicalDamage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kAttackPhysicalDamage));

        // Modify
        data.Set(StatType::kAttackPhysicalDamage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kAttackPhysicalDamage), data.Get(StatType::kAttackPhysicalDamage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kAttackEnergyDamage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kAttackEnergyDamage));

        // Modify
        data.Set(StatType::kAttackEnergyDamage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kAttackEnergyDamage), data.Get(StatType::kAttackEnergyDamage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kAttackPureDamage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kAttackPureDamage));

        // Modify
        data.Set(StatType::kAttackPureDamage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kAttackPureDamage), data.Get(StatType::kAttackPureDamage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kMaxHealth);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kMaxHealth));

        // Modify
        data.Set(StatType::kMaxHealth, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kMaxHealth), data.Get(StatType::kMaxHealth));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kCurrentHealth);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kCurrentHealth));

        // Modify
        data.Set(StatType::kCurrentHealth, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kCurrentHealth), data.Get(StatType::kCurrentHealth));
    }
    counter += 1_fp;
    {
        const FixedPoint old_value = data.Get(StatType::kEnergyCost);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kEnergyCost));

        // Modify
        data.Set(StatType::kEnergyCost, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kEnergyCost), data.Get(StatType::kEnergyCost));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kStartingEnergy);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kStartingEnergy));

        // Modify
        data.Set(StatType::kStartingEnergy, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kStartingEnergy), data.Get(StatType::kStartingEnergy));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kCurrentEnergy);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kCurrentEnergy));

        // Modify
        data.Set(StatType::kCurrentEnergy, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kCurrentEnergy), data.Get(StatType::kCurrentEnergy));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kPhysicalResist);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kPhysicalResist));

        // Modify
        data.Set(StatType::kPhysicalResist, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kPhysicalResist), data.Get(StatType::kPhysicalResist));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kEnergyResist);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kEnergyResist));

        // Modify
        data.Set(StatType::kEnergyResist, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kEnergyResist), data.Get(StatType::kEnergyResist));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kTenacityPercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kTenacityPercentage));

        // Modify
        data.Set(StatType::kTenacityPercentage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kTenacityPercentage), data.Get(StatType::kTenacityPercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kWillpowerPercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kWillpowerPercentage));

        // Modify
        data.Set(StatType::kWillpowerPercentage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kWillpowerPercentage), data.Get(StatType::kWillpowerPercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kGrit);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kGrit));

        // Modify
        data.Set(StatType::kGrit, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kGrit), data.Get(StatType::kGrit));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kResolve);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kResolve));

        // Modify
        data.Set(StatType::kResolve, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kResolve), data.Get(StatType::kResolve));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kVulnerabilityPercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kVulnerabilityPercentage));

        // Modify
        data.Set(StatType::kVulnerabilityPercentage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kVulnerabilityPercentage), data.Get(StatType::kVulnerabilityPercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kCritChancePercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kCritChancePercentage));

        // Modify
        data.Set(StatType::kCritChancePercentage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kCritChancePercentage), data.Get(StatType::kCritChancePercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kCritAmplificationPercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kCritAmplificationPercentage));

        // Modify
        data.Set(StatType::kCritAmplificationPercentage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kCritAmplificationPercentage), data.Get(StatType::kCritAmplificationPercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kOmegaPowerPercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kOmegaPowerPercentage));

        // Modify
        data.Set(StatType::kOmegaPowerPercentage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kOmegaPowerPercentage), data.Get(StatType::kOmegaPowerPercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kEnergyGainEfficiencyPercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kEnergyGainEfficiencyPercentage));

        // Modify
        data.Set(StatType::kEnergyGainEfficiencyPercentage, old_value + modify_by);
        EXPECT_EQ(
            data.Get(StatType::kEnergyGainEfficiencyPercentage),
            data.Get(StatType::kEnergyGainEfficiencyPercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kHealthGainEfficiencyPercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kHealthGainEfficiencyPercentage));

        // Modify
        data.Set(StatType::kHealthGainEfficiencyPercentage, old_value + modify_by);
        EXPECT_EQ(
            data.Get(StatType::kHealthGainEfficiencyPercentage),
            data.Get(StatType::kHealthGainEfficiencyPercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kOnActivationEnergy);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kOnActivationEnergy));

        // Modify
        data.Set(StatType::kOnActivationEnergy, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kOnActivationEnergy), data.Get(StatType::kOnActivationEnergy));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kPhysicalVampPercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kPhysicalVampPercentage));

        // Modify
        data.Set(StatType::kPhysicalVampPercentage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kPhysicalVampPercentage), data.Get(StatType::kPhysicalVampPercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kPureVampPercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kPureVampPercentage));

        // Modify
        data.Set(StatType::kPureVampPercentage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kPureVampPercentage), data.Get(StatType::kPureVampPercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kEnergyVampPercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kEnergyVampPercentage));

        // Modify
        data.Set(StatType::kEnergyVampPercentage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kEnergyVampPercentage), data.Get(StatType::kEnergyVampPercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kEnergyPiercingPercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kEnergyPiercingPercentage));

        // Modify
        data.Set(StatType::kEnergyPiercingPercentage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kEnergyPiercingPercentage), data.Get(StatType::kEnergyPiercingPercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kPhysicalPiercingPercentage);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kPhysicalPiercingPercentage));

        // Modify
        data.Set(StatType::kPhysicalPiercingPercentage, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kPhysicalPiercingPercentage), data.Get(StatType::kPhysicalPiercingPercentage));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kThorns);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kThorns));

        // Modify
        data.Set(StatType::kThorns, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kThorns), data.Get(StatType::kThorns));
    }
    counter += 1_fp;

    {
        const FixedPoint old_value = data.Get(StatType::kStartingShield);

        // Value is equal
        EXPECT_EQ(old_value, counter);
        EXPECT_EQ(old_value, data.Get(StatType::kStartingShield));

        // Modify
        data.Set(StatType::kStartingShield, old_value + modify_by);
        EXPECT_EQ(data.Get(StatType::kStartingShield), data.Get(StatType::kStartingShield));
    }
    counter += 1_fp;
}

TEST_F(StatsDataTest, MeteredStatCorrespondingMaxValue)
{
    {
        // Get Metered types
        const auto health_corresponding_max = data.GetMeteredStatCorrespondingMaxValue(StatType::kCurrentHealth);
        const auto energy_corresponding_max = data.GetMeteredStatCorrespondingMaxValue(StatType::kCurrentEnergy);
        const auto sub_hyper_corresponding_max = data.GetMeteredStatCorrespondingMaxValue(StatType::kCurrentSubHyper);

        // Check value equality
        EXPECT_EQ(health_corresponding_max, data.Get(StatType::kMaxHealth));
        EXPECT_EQ(energy_corresponding_max, data.Get(StatType::kEnergyCost));
        EXPECT_EQ(sub_hyper_corresponding_max, kMaxHyper);
    }

    {
        // UnMetered types
        const auto invalid_value1 = data.GetMeteredStatCorrespondingMaxValue(StatType::kAttackSpeed);
        const auto invalid_value2 = data.GetMeteredStatCorrespondingMaxValue(StatType::kAttackDamage);

        EXPECT_EQ(invalid_value1, 0_fp);
        EXPECT_EQ(invalid_value2, 0_fp);
    }
}

TEST_F(StatOmegaRangeTest, OmegaRangeAdjustmentCheck)
{
    auto& stats_component = blue_entity->Get<StatsComponent>();

    StatsData live_stats = world->GetLiveStats(blue_entity->GetID());

    // Omega is initialized to 5 but is automatically updated to be at least
    // The same as attack_range_units
    EXPECT_EQ(live_stats.Get(StatType::kAttackRangeUnits), 15_fp);
    EXPECT_EQ(live_stats.Get(StatType::kOmegaRangeUnits), 15_fp);

    stats_component.GetMutableTemplateStats().Set(StatType::kAttackRangeUnits, 25_fp);
    live_stats = world->GetLiveStats(blue_entity->GetID());

    // Attack range is updated to a value that would cause omega_range < attack_range
    // So omega is adjusted
    EXPECT_EQ(live_stats.Get(StatType::kAttackRangeUnits), 25_fp);
    EXPECT_EQ(live_stats.Get(StatType::kOmegaRangeUnits), 25_fp);

    stats_component.GetMutableTemplateStats().Set(StatType::kAttackRangeUnits, 26_fp);
    live_stats = world->GetLiveStats(blue_entity->GetID());

    // Attack range is updated to a value that would cause omega_range < attack_range
    // So omega is adjusted
    EXPECT_EQ(live_stats.Get(StatType::kAttackRangeUnits), 26_fp);
    EXPECT_EQ(live_stats.Get(StatType::kOmegaRangeUnits), 26_fp);

    stats_component.GetMutableTemplateStats().Set(StatType::kOmegaRangeUnits, 30_fp);
    live_stats = world->GetLiveStats(blue_entity->GetID());

    // Omega_range can be a higher value than attack_range so only omega_range should change
    EXPECT_EQ(live_stats.Get(StatType::kAttackRangeUnits), 26_fp);
    EXPECT_EQ(live_stats.Get(StatType::kOmegaRangeUnits), 30_fp);

    stats_component.GetMutableTemplateStats().Set(StatType::kAttackRangeUnits, 16_fp);
    live_stats = world->GetLiveStats(blue_entity->GetID());

    // Adjusting only attack_range should not affect omega_range as long as omega_range > attack_range
    EXPECT_EQ(live_stats.Get(StatType::kAttackRangeUnits), 16_fp);
    EXPECT_EQ(live_stats.Get(StatType::kOmegaRangeUnits), 30_fp);

    stats_component.GetMutableTemplateStats().Set(StatType::kOmegaRangeUnits, 14_fp);
    live_stats = world->GetLiveStats(blue_entity->GetID());

    // Attempting to set omega_range to a lower value than attack_range should be prevented
    EXPECT_EQ(live_stats.Get(StatType::kAttackRangeUnits), 16_fp);
    EXPECT_EQ(live_stats.Get(StatType::kOmegaRangeUnits), 16_fp);
}

}  // namespace simulation
