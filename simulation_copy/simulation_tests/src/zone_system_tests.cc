#include <memory>

#include "ability_system_data_fixtures.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/stats_component.h"
#include "components/zone_component.h"
#include "gtest/gtest.h"

namespace simulation
{
class ZoneSystemTest : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return true;
    }

    void InitStats() override
    {
        data = CreateCombatUnitData();

        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 20_fp);
        data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);
        data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);
        data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Zone Ability";
            // Timers are instant 0

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Zone Attack";
            skill1.targeting.type = SkillTargetingType::kSelf;
            skill1.deployment.type = SkillDeploymentType::kZone;

            // Set the zone data
            skill1.zone.shape = GetZoneShape();
            skill1.zone.radius_units = GetZoneRadiusUnits();
            skill1.zone.max_radius_units = GetZoneMaxRadiusUnits();
            skill1.zone.width_units = GetZoneWidthUnits();
            skill1.zone.height_units = GetZoneHeightUnits();
            skill1.zone.direction_degrees = GetZoneDirectionDegrees();
            skill1.zone.duration_ms = GetZoneDurationMs();
            skill1.zone.growth_rate_sub_units_per_time_step = GetZoneGrowthRateSubUnitsPerTimeStep();
            skill1.zone.frequency_ms = GetFrequencyMs();
            skill1.zone.apply_once = IsApplyOnce();

            skill1.effect_package.attributes.always_crit = GetAlwaysCrit();
            if (IsMoving())
            {
                skill1.zone.movement_speed_sub_units_per_time_step = GetMovementSpeedSubUnits();
            }
            else
            {
                skill1.zone.movement_speed_sub_units_per_time_step = 0;
            }

            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(10_fp));
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual bool GetAlwaysCrit() const
    {
        return false;
    }

    virtual ZoneEffectShape GetZoneShape() const
    {
        return ZoneEffectShape::kHexagon;
    }
    virtual int GetZoneRadiusUnits() const
    {
        return 20;
    }
    virtual int GetZoneMaxRadiusUnits() const
    {
        return GetZoneRadiusUnits();
    }
    virtual int GetZoneWidthUnits() const
    {
        return 40;
    }
    virtual int GetZoneHeightUnits() const
    {
        return 20;
    }
    virtual int GetZoneDirectionDegrees() const
    {
        return 0;
    }
    virtual int GetZoneDurationMs() const
    {
        return 600;
    }
    virtual int GetZoneGrowthRateSubUnitsPerTimeStep() const
    {
        return 0;
    }
    virtual int GetFrequencyMs() const
    {
        return 200;
    }
    virtual bool IsApplyOnce() const
    {
        return false;
    }
    virtual bool IsMoving() const
    {
        return false;
    }
    virtual int GetMovementSpeedSubUnits() const
    {
        return 5000;
    }

    void InitOmegaAbilities() override {}
    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {-32, -65}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {-27, -55}, data, red_entity);
    }

    Entity* SpawnExtra(const HexGridPosition& position)
    {
        // Don't let it move
        auto& extra_data = data;
        extra_data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);

        // Spawn an extra red CombatUnit
        Entity* entity = nullptr;
        SpawnCombatUnit(Team::kRed, position, extra_data, entity);
        return entity;
    }

    void OutOfRangeTest()
    {
        AddShields();

        // Run for 10 time steps
        for (int time_step = 0; time_step < 10; time_step++)
        {
            // TimeStep
            world->TimeStep();

            // Iterate the entities
            auto& entities = world->GetAll();
            for (auto& entity : entities)
            {
                if (!EntityHelper::IsACombatUnit(*entity)) continue;

                auto& stats_component = entity->Get<StatsComponent>();

                // Nothing should change due to out of range
                ASSERT_EQ(world->GetShieldTotal(*entity), 10_fp) << "time step = " << time_step;
                ASSERT_EQ(stats_component.GetCurrentHealth(), 100_fp) << "time step = " << time_step;
            }
        }
    }

    void HitTest(const FixedPoint max_health)
    {
        AddShields();

        // Keep track of health and shield (by entity id)
        std::map<EntityID, FixedPoint> last_health;
        std::map<EntityID, FixedPoint> last_shield;
        std::map<EntityID, int> change_count;

        // Count the critical zones
        int count_zones_is_critical = 0;
        world->SubscribeToEvent(
            EventType::kEffectPackageReceived,
            [&](const Event& event)
            {
                const auto& data = event.Get<event_data::EffectPackage>();
                if (EntityHelper::IsAZone(*world, data.sender_id) && data.is_critical)
                {
                    count_zones_is_critical++;
                }
                events_effect_package_received.emplace_back(data);
            });

        // Run for 10 time steps
        for (int time_step = 0; time_step < 10; time_step++)
        {
            // Iterate the entities to get current stats
            const auto& entities = world->GetAll();
            for (const auto& entity : entities)
            {
                const EntityID entity_id = entity->GetID();
                if (!entity->IsActive()) continue;
                if (!EntityHelper::IsACombatUnit(*entity)) continue;

                const auto& stats_component = entity->Get<StatsComponent>();
                last_health[entity_id] = stats_component.GetCurrentHealth();
                last_shield[entity_id] = world->GetShieldTotal(*entity);

                // Init change count
                if (change_count.count(entity_id) == 0)
                {
                    change_count[entity_id] = 0;
                }
            }

            // TimeStep
            world->TimeStep();

            // Iterate the entities (we don't care about new entities)
            for (const auto& entity : entities)
            {
                const EntityID entity_id = entity->GetID();
                if (!entity->IsActive()) continue;
                if (!EntityHelper::IsACombatUnit(*entity)) continue;

                const auto& stats_component = entity->Get<StatsComponent>();

                // This entity gets missed
                if (entity_id == 2)
                {
                    // Should not be affected
                    ASSERT_EQ(stats_component.GetCurrentHealth(), max_health) << "time step = " << time_step;
                    continue;
                }

                if (time_step == 4)
                {
                    // No damage this time_step
                    ASSERT_EQ(world->GetShieldTotal(*entity), last_shield[entity_id]) << "time_step = " << time_step;
                    ASSERT_EQ(stats_component.GetCurrentHealth(), last_health[entity_id])
                        << "time_step = " << time_step;
                }
                else if (time_step == 9)
                {
                    // Should have sustained some damage by now
                    ASSERT_EQ(world->GetShieldTotal(*entity), 0_fp) << "time_step = " << time_step;
                    if (!IsApplyOnce())
                    {
                        ASSERT_LT(stats_component.GetCurrentHealth(), max_health) << "time_step = " << time_step;
                    }
                }
                else
                {
                    // Either no or some damage this time_step
                    ASSERT_LE(world->GetShieldTotal(*entity), last_shield[entity_id]) << "time_step = " << time_step;
                    ASSERT_LE(stats_component.GetCurrentHealth(), last_health[entity_id])
                        << "time_step = " << time_step;
                }

                // Update change count if changed
                if (world->GetShieldTotal(*entity) != last_shield[entity_id] ||
                    stats_component.GetCurrentHealth() != last_health[entity_id])
                {
                    change_count[entity_id]++;
                }

                last_shield[entity_id] = world->GetShieldTotal(*entity);
                last_health[entity_id] = stats_component.GetCurrentHealth();
            }
        }

        if (GetAlwaysCrit())
        {
            EXPECT_GT(count_zones_is_critical, 0);
        }
        else
        {
            EXPECT_EQ(count_zones_is_critical, 0);
        }

        // If testing for apply once, make sure value only changed once
        if (IsApplyOnce())
        {
            const auto& entities = world->GetAll();
            for (const auto& entity : entities)
            {
                if (!EntityHelper::IsASynergy(*entity))
                {
                    ASSERT_EQ(change_count[entity->GetID()], 1);
                }
            }
        }
    }
};

class ZoneSystemAttachTest : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return true;
    }

    void InitStats() override
    {
        data = CreateCombatUnitData();

        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 2_fp);
        data.type_data.stats.Set(StatType::kOmegaRangeUnits, 50_fp);
        data.type_data.stats.Set(StatType::kStartingEnergy, 100_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);
        data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);
        data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 10000_fp);
        data.type_data.stats.Set(StatType::kCurrentHealth, 10000_fp);
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Zone Ability";
            // Timers are instant 0

            // Skills
            auto& skill = ability.AddSkill();
            skill.name = "Zone Attack";
            skill.targeting.type = SkillTargetingType::kCurrentFocus;
            skill.deployment.type = SkillDeploymentType::kDirect;
            skill.effect_package.attributes.always_crit = false;

            skill.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(10_fp));
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void InitOmegaAbilities() override
    {
        // Add omega ability
        {
            auto& ability = data.type_data.omega_abilities.AddAbility();
            ability.name = "Zone Ability";
            // Timers are instant 0

            // Skills
            auto& skill = ability.AddSkill();
            skill.name = "Zone Attack";
            skill.targeting.type = SkillTargetingType::kCurrentFocus;
            skill.deployment.type = SkillDeploymentType::kZone;
            skill.effect_package.attributes.always_crit = false;

            // Set the zone data
            skill.zone.shape = ZoneEffectShape::kHexagon;
            skill.zone.radius_units = 2;
            skill.zone.max_radius_units = 20;
            skill.zone.width_units = 4;
            skill.zone.height_units = 4;
            skill.zone.duration_ms = 600;
            skill.zone.growth_rate_sub_units_per_time_step = 0;
            skill.zone.frequency_ms = 100;
            skill.zone.attach_to_target = true;

            skill.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(10_fp));
        }

        data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {20, 40}, data, red_entity);
    }
};

class ZoneSystemNoDataTest : public BaseTest
{
};

class ZoneSystemApplyOnceTest : public ZoneSystemTest
{
    typedef ZoneSystemTest Super;

protected:
    bool IsApplyOnce() const override
    {
        return true;
    }
};

class ZoneSystemAlwaysCritTest : public ZoneSystemTest
{
    typedef ZoneSystemTest Super;

protected:
    bool GetAlwaysCrit() const override
    {
        return true;
    }
};

class ZoneSystemRectangleTest : public ZoneSystemTest
{
    typedef ZoneSystemTest Super;

protected:
    ZoneEffectShape GetZoneShape() const override
    {
        return ZoneEffectShape::kRectangle;
    }
};

class ZoneSystemTriangleTest : public ZoneSystemTest
{
    typedef ZoneSystemTest Super;

protected:
    ZoneEffectShape GetZoneShape() const override
    {
        return ZoneEffectShape::kTriangle;
    }
};

class ZoneSystemTestOutOfRange : public ZoneSystemTest
{
    typedef ZoneSystemTest Super;

protected:
    int GetZoneRadiusUnits() const override
    {
        return 5;
    }
};

class ZoneSystemTestRectangleOutOfRange : public ZoneSystemRectangleTest
{
    typedef ZoneSystemTest Super;

protected:
    int GetZoneWidthUnits() const override
    {
        return 5;
    }
    int GetZoneHeightUnits() const override
    {
        return 3;
    }
};

class ZoneSystemTestTriangleOutOfRange : public ZoneSystemTriangleTest
{
    typedef ZoneSystemTest Super;

protected:
    int GetZoneRadiusUnits() const override
    {
        return 5;
    }
};

class ZoneSystemTestExpireWithActiveAbility : public ZoneSystemTest
{
    typedef ZoneSystemTest Super;

protected:
    int GetZoneDurationMs() const override
    {
        return 200;
    }
    int GetFrequencyMs() const override
    {
        return 200;
    }
};

class ZoneSystemTestExpireBeforeActivation : public ZoneSystemTest
{
    typedef ZoneSystemTest Super;

protected:
    int GetZoneDurationMs() const override
    {
        return 100;
    }
    int GetFrequencyMs() const override
    {
        return 200;
    }
};

class ZoneSystemTestGrowing : public ZoneSystemTest
{
    typedef ZoneSystemTest Super;

protected:
    int GetZoneGrowthRateSubUnitsPerTimeStep() const override
    {
        return 100;
    }

    int GetZoneDurationMs() const override
    {
        return kTimeInfinite;
    }
    int GetZoneMaxRadiusUnits() const override
    {
        return 21;
    }
};

class ZoneSystemTestGrowingWithoutDefault : public ZoneSystemTest
{
    typedef ZoneSystemTest Super;

protected:
    int GetZoneGrowthRateSubUnitsPerTimeStep() const override
    {
        return 0;
    }

    int GetZoneDurationMs() const override
    {
        return 500;
    }
    int GetZoneMaxRadiusUnits() const override
    {
        return 21;
    }
};

class ZoneSystemMovingTest : public ZoneSystemTest
{
    typedef ZoneSystemTest Super;

protected:
    void InitAttackAbilities() override
    {
        ZoneSystemTest::InitAttackAbilities();

        const auto& ability = data.type_data.attack_abilities.abilities[0];

        // Get skill data for zone
        auto skill_data = ability->skills[0];
        skill_data->targeting.group = AllegianceType::kEnemy;
        skill_data->targeting.type = SkillTargetingType::kDistanceCheck;
        skill_data->targeting.num = 1;
    }

    bool IsMoving() const override
    {
        return true;
    }
};

class ZoneSystemFastMovingTest : public ZoneSystemMovingTest
{
    typedef ZoneSystemMovingTest Super;

protected:
    int GetMovementSpeedSubUnits() const override
    {
        return 30000;
    }
};

class ZoneSystemTestReturnThorns : public ZoneSystemTest
{
    typedef ZoneSystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return true;
    }

    void InitStats() override
    {
        data = CreateCombatUnitData();
        data2 = CreateCombatUnitData();

        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 20_fp);
        data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);
        data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);
        data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);

        data2.radius_units = 1;
        data2.type_data.combat_affinity = CombatAffinity::kFire;
        data2.type_data.combat_class = CombatClass::kBehemoth;
        data2.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data2.type_data.stats.Set(StatType::kAttackRangeUnits, 20_fp);
        data2.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        data2.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data2.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
        data2.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);
        data2.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);
        data2.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);
        data2.type_data.stats.Set(StatType::kThorns, 25_fp);
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Zone Ability";
            // Timers are instant 0

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Zone Attack";
            skill1.targeting.type = SkillTargetingType::kSelf;
            skill1.deployment.type = SkillDeploymentType::kZone;

            // Set the zone data
            skill1.zone.shape = ZoneEffectShape::kHexagon;
            skill1.zone.radius_units = 20;
            skill1.zone.max_radius_units = 20;
            skill1.zone.width_units = 40;
            skill1.zone.height_units = 20;
            skill1.zone.direction_degrees = 0;
            skill1.zone.duration_ms = 600;
            skill1.zone.growth_rate_sub_units_per_time_step = 0;
            skill1.zone.frequency_ms = 200;
            skill1.zone.apply_once = false;

            if (IsMoving())
            {
                skill1.zone.movement_speed_sub_units_per_time_step = GetMovementSpeedSubUnits();
            }
            else
            {
                skill1.zone.movement_speed_sub_units_per_time_step = 0;
            }

            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(10_fp));
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {-32, -65}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {-27, -55}, data2, red_entity);
    }

    CombatUnitData data2;
};
class ZoneSystemTestCreateShield : public ZoneSystemTest
{
    typedef ZoneSystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return true;
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Zone Ability";
            // Timers are instant 0

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Zone Attack";
            skill1.targeting.type = SkillTargetingType::kSelf;
            skill1.deployment.type = SkillDeploymentType::kZone;

            // Set the zone data
            skill1.zone.shape = ZoneEffectShape::kHexagon;
            skill1.zone.radius_units = 20;
            skill1.zone.max_radius_units = 20;
            skill1.zone.width_units = 40;
            skill1.zone.height_units = 20;
            skill1.zone.direction_degrees = 0;
            skill1.zone.duration_ms = 600;
            skill1.zone.growth_rate_sub_units_per_time_step = 0;
            skill1.zone.frequency_ms = 200;
            skill1.zone.apply_once = false;
            skill1.zone.movement_speed_sub_units_per_time_step = 0;

            auto shield_effect_data = EffectData::Create(EffectType::kSpawnShield, EffectExpression::FromValue(100_fp));

            skill1.AddEffect(shield_effect_data);
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {-32, -65}, data, blue_entity);
        SpawnCombatUnit(Team::kBlue, {-34, -67}, data, blue_entity2);
        SpawnCombatUnit(Team::kRed, {-27, -55}, data2, red_entity);
    }

    CombatUnitData data2;
    Entity* blue_entity2 = nullptr;
};

TEST_F(ZoneSystemNoDataTest, InvalidFrequencyNoCrash)
{
    // CombatUnit stats
    CombatUnitData data;
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 20_fp);
    data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
    data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
    data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);

    // Add attack abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kSelf;
        skill1.deployment.type = SkillDeploymentType::kZone;

        // Set the zone data
        skill1.zone.shape = ZoneEffectShape::kHexagon;
        skill1.zone.radius_units = 20;
        skill1.zone.duration_ms = 0;
        skill1.zone.frequency_ms = 0;

        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(10_fp));
    }
    data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Add blue CombatUnit
    Entity* blue_dummy1 = nullptr;
    SpawnCombatUnit(Team::kBlue, {-32, -65}, data, blue_dummy1);

    // Add red CombatUnit
    Entity* red_dummy1 = nullptr;
    SpawnCombatUnit(Team::kRed, {-27, -55}, data, red_dummy1);

    // Run for 10 time steps
    for (int time_step = 0; time_step < 10; time_step++)
    {
        world->TimeStep();
    }
}

TEST_F(ZoneSystemTestOutOfRange, SimpleZoneOutOfRange)
{
    OutOfRangeTest();
}

TEST_F(ZoneSystemTest, SimpleZoneHit)
{
    SpawnExtra(HexGridPosition(-25, -50));
    HitTest(100_fp);
}

TEST_F(ZoneSystemAlwaysCritTest, AlwaysCrit)
{
    HitTest(100_fp);
}

TEST_F(ZoneSystemTestRectangleOutOfRange, RectangleZoneOutOfRange)
{
    OutOfRangeTest();
}

TEST_F(ZoneSystemRectangleTest, RectangleZoneHit)
{
    SpawnExtra(HexGridPosition(-22, -44));
    HitTest(100_fp);
}

TEST_F(ZoneSystemTestTriangleOutOfRange, TriangleZoneOutOfRange)
{
    OutOfRangeTest();
}

TEST_F(ZoneSystemTriangleTest, TriangleZoneHit)
{
    SpawnExtra(HexGridPosition(-22, -44));
    HitTest(100_fp);
}

TEST_F(ZoneSystemApplyOnceTest, ApplyOnce)
{
    HitTest(100_fp);
}

TEST_F(ZoneSystemTestExpireWithActiveAbility, ExpireWithActiveAbility)
{
    AddShields();

    // Keep track of health and shield (by entity id)
    std::map<int, FixedPoint> last_health;
    std::map<int, FixedPoint> last_shield;

    // Run for 10 time steps
    for (int time_step = 0; time_step < 10; time_step++)
    {
        // Iterate the entities to get current stats
        const auto& entities = world->GetAll();
        for (auto& entity : entities)
        {
            const EntityID entity_id = entity->GetID();
            if (!entity->IsActive()) continue;
            if (!EntityHelper::IsACombatUnit(*entity)) continue;

            const auto& stats_component = entity->Get<StatsComponent>();
            last_health[entity_id] = stats_component.GetCurrentHealth();
            last_shield[entity_id] = world->GetShieldTotal(*entity);
        }

        // TimeStep
        world->TimeStep();

        // Iterate the entities (we don't care about new entities)
        for (auto& entity : entities)
        {
            const EntityID entity_id = entity->GetID();
            if (!entity->IsActive()) continue;
            if (!EntityHelper::IsACombatUnit(*entity)) continue;

            const auto& stats_component = entity->Get<StatsComponent>();
            if (time_step == 4)
            {
                // No damage this time step
                ASSERT_EQ(world->GetShieldTotal(*entity), last_shield[entity_id]) << "time_step = " << time_step;
                ASSERT_EQ(stats_component.GetCurrentHealth(), last_health[entity_id]) << "time_step = " << time_step;
            }
            else if (time_step == 9)
            {
                // Should have sustained some damage by now
                ASSERT_EQ(world->GetShieldTotal(*entity), 0_fp) << "time_step = " << time_step;
                ASSERT_LT(stats_component.GetCurrentHealth(), 100_fp) << "time_step = " << time_step;
            }
            else
            {
                // Either no or some damage this time_step
                ASSERT_LE(world->GetShieldTotal(*entity), last_shield[entity_id]) << "time_step = " << time_step;
                ASSERT_LE(stats_component.GetCurrentHealth(), last_health[entity_id]) << "time_step = " << time_step;
            }

            last_shield[entity_id] = world->GetShieldTotal(*entity);
            last_health[entity_id] = stats_component.GetCurrentHealth();
        }
    }
}

TEST_F(ZoneSystemTestExpireBeforeActivation, ExpireBeforeActivation)
{
    AddShields();

    // Keep track of health and shield (by entity id)
    std::map<int, FixedPoint> last_health;
    std::map<int, FixedPoint> last_shield;

    // Run for 10 time steps
    for (int time_step = 0; time_step < 5; time_step++)
    {
        // Iterate the entities to get current stats
        const auto& entities = world->GetAll();
        for (auto& entity : entities)
        {
            const EntityID entity_id = entity->GetID();
            if (!entity->IsActive()) continue;
            if (!EntityHelper::IsACombatUnit(*entity)) continue;

            const auto& stats_component = entity->Get<StatsComponent>();
            last_health[entity_id] = stats_component.GetCurrentHealth();
            last_shield[entity_id] = world->GetShieldTotal(*entity);
        }

        // TimeStep
        world->TimeStep();

        // Iterate the entities (we don't care about new entities)
        for (const auto& entity : entities)
        {
            const EntityID entity_id = entity->GetID();
            if (!entity->IsActive()) continue;
            if (!EntityHelper::IsACombatUnit(*entity)) continue;

            const auto& stats_component = entity->Get<StatsComponent>();
            if (time_step == 4)
            {
                // No damage this time step
                ASSERT_EQ(world->GetShieldTotal(*entity), last_shield[entity_id]) << "time_step = " << time_step;
                ASSERT_EQ(stats_component.GetCurrentHealth(), last_health[entity_id]) << "time_step = " << time_step;
            }
            else
            {
                // Shield would take damage from first effect
                ASSERT_LE(world->GetShieldTotal(*entity), last_shield[entity_id]) << "time_step = " << time_step;

                // Doesn't last long enough to damage health
                ASSERT_EQ(stats_component.GetCurrentHealth(), last_health[entity_id]) << "time_step = " << time_step;
            }

            last_shield[entity_id] = world->GetShieldTotal(*entity);
            last_health[entity_id] = stats_component.GetCurrentHealth();
        }
    }
}

TEST_F(ZoneSystemTestGrowing, GrowthTest)
{
    constexpr int synergy_offset = 2;

    // Run for 14 time steps
    for (int time_step = 0; time_step < 14; time_step++)
    {
        // TimeStep
        world->TimeStep();

        // Make sure these are zones
        ASSERT_TRUE(EntityHelper::IsAZone(*world, 2 + synergy_offset));
        ASSERT_TRUE(EntityHelper::IsAZone(*world, 3 + synergy_offset));

        // Get the zone entities and components
        const auto& zone_entity1 = world->GetByID(2 + synergy_offset);
        const auto& zone_entity2 = world->GetByID(3 + synergy_offset);
        const auto zone_component1 = zone_entity1.Get<ZoneComponent>();
        const auto zone_component2 = zone_entity2.Get<ZoneComponent>();

        EXPECT_EQ(zone_component1.GetMaxRadiusSubUnits(), 21000);

        if (time_step > 10)
        {
            // Zone is now at max size, should no longer be growing
            EXPECT_EQ(zone_component1.GetRadiusSubUnits(), 21000);
            EXPECT_EQ(zone_component2.GetRadiusSubUnits(), 21000);
        }
        else
        {
            // Confirm it's getting bigger
            EXPECT_EQ(zone_component1.GetRadiusSubUnits(), 20000 + 100 * time_step) << " " << time_step;
            EXPECT_EQ(zone_component2.GetRadiusSubUnits(), 20000 + 100 * time_step) << " " << time_step;
        }
    }
}

TEST_F(ZoneSystemTestGrowingWithoutDefault, GrowthTestWithoutDefault)
{
    // Run for 6 time steps
    for (int time_step = 0; time_step < 6; time_step++)
    {
        constexpr int synergy_offset = 2;
        // TimeStep
        world->TimeStep();

        // Make sure these are zones
        ASSERT_TRUE(EntityHelper::IsAZone(*world, 2 + synergy_offset));
        ASSERT_TRUE(EntityHelper::IsAZone(*world, 3 + synergy_offset));

        // Get the zone entities and components
        const auto& zone_entity1 = world->GetByID(2 + synergy_offset);
        const auto& zone_entity2 = world->GetByID(3 + synergy_offset);
        const auto zone_component1 = zone_entity1.Get<ZoneComponent>();
        const auto zone_component2 = zone_entity2.Get<ZoneComponent>();

        EXPECT_EQ(zone_component1.GetMaxRadiusSubUnits(), 21000);

        // Growth rate should be 200 since the zone lasts for 5 time steps and max radius is 1000 more than the starting
        // 1000 / 5 = 200
        EXPECT_EQ(zone_component1.GetGrowthRateSubUnitsPerStep(), 200);

        // Confirm it's getting bigger
        EXPECT_EQ(zone_component1.GetRadiusSubUnits(), 20000 + 200 * time_step) << " " << time_step;
        EXPECT_EQ(zone_component2.GetRadiusSubUnits(), 20000 + 200 * time_step) << " " << time_step;
    }
}

TEST_F(ZoneSystemMovingTest, MovingZone)
{
    // The farthest position
    // Zone need to reach this position
    constexpr auto farthest_position = HexGridPosition(0, 2);
    // The farthest enemy
    const Entity* red_entity1 = SpawnExtra(farthest_position);

    // Get components
    const auto& blue_entity_position_component = blue_entity->Get<PositionComponent>();
    auto& movement_component = red_entity->Get<MovementComponent>();
    auto& movement_component1 = red_entity1->Get<MovementComponent>();

    // Enemies stand still
    movement_component.SetMovementSpeedSubUnits(0_fp);
    movement_component1.SetMovementSpeedSubUnits(0_fp);

    constexpr int synergy_offset = 2;
    // Zone entity ID
    constexpr EntityID blue_zone_entity_id = 3 + synergy_offset;

    // Spawn zone in first TimeStep
    world->TimeStep();

    // Zone has spawned
    ASSERT_TRUE(EntityHelper::IsAZone(*world, blue_zone_entity_id));

    // Get zone
    const auto& blue_zone_entity = world->GetByID(blue_zone_entity_id);
    const auto& zone_position_component = blue_zone_entity.Get<PositionComponent>();

    // Zone has to spawn in the position of blue combat entity
    ASSERT_EQ(zone_position_component.GetPosition(), blue_entity_position_component.GetPosition());

    // The zone will reach the target in 21 steps.
    for (int time_step = 0; time_step < 21; time_step++)
    {
        world->TimeStep();
    }

    // Does zone exist in last TimeStep
    ASSERT_TRUE(EntityHelper::IsAZone(*world, blue_zone_entity_id));

    // Zone needs to be at the final position
    ASSERT_EQ(zone_position_component.GetPosition(), farthest_position);

    // Time step for zone to expire
    world->TimeStep();

    // Zone should be gone
    ASSERT_FALSE(EntityHelper::IsAZone(*world, blue_zone_entity_id));
}

TEST_F(ZoneSystemFastMovingTest, MovingZoneOffMap)
{
    const HexGridConfig& grid_config = GetGridConfig();

    // The farthest position
    constexpr auto farthest_position = HexGridPosition(0, 2);
    // The furthest enemy
    const Entity* red_entity1 = SpawnExtra(farthest_position);

    // Final zone position
    constexpr auto final_position = HexGridPosition(48, 95);

    // Get components
    const auto& blue_entity_position_component = blue_entity->Get<PositionComponent>();
    auto& movement_component = red_entity->Get<MovementComponent>();
    auto& movement_component1 = red_entity1->Get<MovementComponent>();

    // Enemies stand still
    movement_component.SetMovementSpeedSubUnits(0_fp);
    movement_component1.SetMovementSpeedSubUnits(0_fp);

    constexpr int synergy_offset = 2;

    // Zone entity ID
    constexpr EntityID blue_zone_entity_id = 3 + synergy_offset;

    // Spawn zone in first TimeStep
    world->TimeStep();

    // Zone has spawned
    ASSERT_TRUE(EntityHelper::IsAZone(*world, blue_zone_entity_id));

    // Get zone
    const auto& blue_zone_entity = world->GetByID(blue_zone_entity_id);
    const auto& zone_position_component = blue_zone_entity.Get<PositionComponent>();

    // Zone has to spawn in the position of blue combat entity
    EXPECT_EQ(zone_position_component.GetPosition(), blue_entity_position_component.GetPosition());

    // The zone will reach the target in 8 steps.
    for (int time_step = 0; time_step < 8; time_step++)
    {
        world->TimeStep();
    }

    // Does zone exist in last TimeStep
    ASSERT_TRUE(EntityHelper::IsAZone(*world, blue_zone_entity_id));

    // Zone needs to be beyond the edges of the map
    EXPECT_FALSE(grid_config.IsInMapRectangleLimits(zone_position_component.GetPosition()));

    // At this specific position
    EXPECT_EQ(zone_position_component.GetPosition(), final_position);

    // Time step for zone to fade away
    world->TimeStep();

    // Zone should be gone
    EXPECT_FALSE(EntityHelper::IsAZone(*world, blue_zone_entity_id));
}

TEST_F(ZoneSystemTestReturnThorns, SimpleZoneHitReturnThorns)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    blue_stats_component.GetMutableTemplateStats()
        .Set(StatType::kMaxHealth, 1000_fp)
        .Set(StatType::kCurrentHealth, 1000_fp);
    red_stats_component.GetMutableTemplateStats()
        .Set(StatType::kMaxHealth, 1000_fp)
        .Set(StatType::kCurrentHealth, 1000_fp);
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 1000_fp);

    HitTest(1000_fp);

    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 910_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 970_fp);
}

TEST_F(ZoneSystemAttachTest, AttachToTarget)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 10000_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 10000_fp);

    EventHistory<EventType::kZoneCreated> zone_created(*world);
    EventHistory<EventType::kZoneDestroyed> zone_destroyed(*world);

    world->TimeStep();
    EXPECT_EQ(zone_created.Size(), 2);

    std::shared_ptr<Entity> zone_attached_to_blue;
    std::shared_ptr<Entity> zone_attached_to_red;
    if (zone_created[0].sender_id != red_entity->GetID())
    {
        std::swap(zone_created.events[0], zone_created.events[1]);
    }

    zone_attached_to_blue = world->GetByIDPtr(zone_created[0].entity_id);
    EXPECT_EQ(zone_created[0].sender_id, red_entity->GetID());
    EXPECT_EQ(zone_created[0].position, blue_entity->Get<PositionComponent>().GetPosition());

    zone_attached_to_red = world->GetByIDPtr(zone_created[1].entity_id);
    EXPECT_EQ(zone_created[1].sender_id, blue_entity->GetID());
    EXPECT_EQ(zone_created[1].position, red_entity->Get<PositionComponent>().GetPosition());

    auto prev_blue_pos = blue_entity->Get<PositionComponent>().GetSubUnitPosition();
    auto prev_red_pos = red_entity->Get<PositionComponent>().GetSubUnitPosition();

    // Make a few time steps and ensure that zones move with combat units
    for (int i = 0; i != 3; ++i)
    {
        world->TimeStep();

        auto curr_blue_pos = blue_entity->Get<PositionComponent>().GetSubUnitPosition();
        auto zone_attached_to_blue_pos = zone_attached_to_blue->Get<PositionComponent>().GetSubUnitPosition();
        EXPECT_NE(prev_blue_pos, curr_blue_pos);
        EXPECT_EQ(zone_attached_to_blue_pos, curr_blue_pos);

        auto curr_red_pos = red_entity->Get<PositionComponent>().GetSubUnitPosition();
        EXPECT_NE(prev_red_pos, curr_red_pos);
        auto zone_attached_to_red_pos = zone_attached_to_red->Get<PositionComponent>().GetSubUnitPosition();
        EXPECT_EQ(zone_attached_to_red_pos, curr_red_pos);
    }
}

TEST_F(ZoneSystemTestCreateShield, ZoneCreateShield)
{
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    blue_stats_component.GetMutableTemplateStats()
        .Set(StatType::kMaxHealth, 1000_fp)
        .Set(StatType::kCurrentHealth, 1000_fp);
    red_stats_component.GetMutableTemplateStats()
        .Set(StatType::kMaxHealth, 1000_fp)
        .Set(StatType::kCurrentHealth, 1000_fp);
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 1000_fp);

    AddShields();

    // Keep track of health and shield (by entity id)
    std::map<EntityID, FixedPoint> last_health;
    std::map<EntityID, FixedPoint> last_shield;
    std::map<EntityID, int> change_count;

    // Count the critical zones
    int count_zones_is_critical = 0;
    world->SubscribeToEvent(
        EventType::kEffectPackageReceived,
        [&](const Event& event)
        {
            const auto& data = event.Get<event_data::EffectPackage>();
            if (EntityHelper::IsAZone(*world, data.sender_id) && data.is_critical)
            {
                count_zones_is_critical++;
            }
            events_effect_package_received.emplace_back(data);
        });

    // Run for 10 time steps
    for (int time_step = 0; time_step < 10; time_step++)
    {
        // Iterate the entities to get current stats
        const auto& entities = world->GetAll();
        for (const auto& entity : entities)
        {
            const EntityID entity_id = entity->GetID();
            if (!entity->IsActive()) continue;
            if (!EntityHelper::IsACombatUnit(*entity)) continue;

            const auto& stats_component = entity->Get<StatsComponent>();
            last_health[entity_id] = stats_component.GetCurrentHealth();
            last_shield[entity_id] = world->GetShieldTotal(*entity);

            // Init change count
            if (change_count.count(entity_id) == 0)
            {
                change_count[entity_id] = 0;
            }
        }

        // TimeStep
        world->TimeStep();

        // Iterate the entities (we don't care about new entities)
        for (const auto& entity : entities)
        {
            const EntityID entity_id = entity->GetID();
            if (!entity->IsActive()) continue;
            if (!EntityHelper::IsACombatUnit(*entity)) continue;

            const auto& stats_component = entity->Get<StatsComponent>();

            // This entity gets missed
            if (entity_id == 2)
            {
                // Should not be affected
                ASSERT_EQ(stats_component.GetCurrentHealth(), 1000_fp) << "time step = " << time_step;
                continue;
            }

            if (time_step == 4)
            {
                // No damage this time_step
                ASSERT_EQ(world->GetShieldTotal(*entity), last_shield[entity_id]) << "time_step = " << time_step;
                ASSERT_EQ(stats_component.GetCurrentHealth(), last_health[entity_id]) << "time_step = " << time_step;
            }
            else if (entity_id == blue_entity2->GetID() || entity_id == blue_entity->GetID())
            {
                // Either no or some damage this time_step
                ASSERT_LE(world->GetShieldTotal(*entity), FixedPoint::FromInt(110 + (time_step * 100)))
                    << "time_step = " << time_step;
                ASSERT_LE(stats_component.GetCurrentHealth(), last_health[entity_id]) << "time_step = " << time_step;
            }
            else
            {
                // Either no or some damage this time_step
                ASSERT_LE(world->GetShieldTotal(*entity), last_shield[entity_id])
                    << "time_step = " << time_step << " enttity = " << entity->GetID();
                ASSERT_LE(stats_component.GetCurrentHealth(), last_health[entity_id]) << "time_step = " << time_step;
            }

            // Update change count if changed
            if (world->GetShieldTotal(*entity) != last_shield[entity_id] ||
                stats_component.GetCurrentHealth() != last_health[entity_id])
            {
                change_count[entity_id]++;
            }

            last_shield[entity_id] = world->GetShieldTotal(*entity);
            last_health[entity_id] = stats_component.GetCurrentHealth();
        }
    }

    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 1000_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 1000_fp);
}

TEST_F(ZoneSystemNoDataTest, GlobalCollisionDamage)
{
    // CombatUnit stats
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 20_fp);
    data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
    data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
    data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);
    data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 0_fp);

    // Add attack abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kSelf;
        skill1.deployment.type = SkillDeploymentType::kZone;

        // Set the zone data
        skill1.zone.shape = ZoneEffectShape::kHexagon;
        skill1.zone.radius_units = 30;
        skill1.zone.duration_ms = 400;
        skill1.zone.frequency_ms = 200;
        skill1.zone.global_collision_id = 1;

        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(10_fp));
    }
    data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Add blue CombatUnit
    Entity* blue_entity1 = nullptr;
    SpawnCombatUnit(Team::kBlue, {-32, -65}, data, blue_entity1);

    // Add blue CombatUnit
    Entity* blue_entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {-39, -54}, data, blue_entity2);

    // Add red CombatUnit
    Entity* red_entity1 = nullptr;
    SpawnCombatUnit(Team::kRed, {-27, -55}, data, red_entity1);

    // Run for 10 time steps
    for (int time_step = 0; time_step < 5; time_step++)
    {
        world->TimeStep();
    }

    const auto& blue1_stats_component = blue_entity1->Get<StatsComponent>();
    const auto& blue2_stats_component = blue_entity2->Get<StatsComponent>();
    const auto& red_stats_component = red_entity1->Get<StatsComponent>();

    EXPECT_EQ(blue1_stats_component.GetCurrentHealth(), 80_fp);
    EXPECT_EQ(blue2_stats_component.GetCurrentHealth(), 80_fp);
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 80_fp);
}

class MovingZonesAndPredefinedPositions : public BaseTest
{
public:
    virtual CombatUnitData MakeCombatUnitData()
    {
        CombatUnitData unit_data = CreateCombatUnitData();
        unit_data.type_id.line_name = "default";
        unit_data.radius_units = 2;
        unit_data.type_data.combat_affinity = CombatAffinity::kFire;
        unit_data.type_data.combat_class = CombatClass::kBehemoth;
        unit_data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 100_fp);
        unit_data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 0_fp);
        unit_data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);             // No critical attacks
        unit_data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);      // No Dodge
        unit_data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);  // No Miss
        unit_data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);                   // Once every 100ms
        unit_data.type_data.stats.Set(StatType::kAttackRangeUnits, 1000_fp);  // Big attack radius so no need to move
        unit_data.type_data.stats.Set(StatType::kOmegaRangeUnits, 1000_fp);  // Very big omega radius so no need to move
        unit_data.type_data.stats.Set(StatType::kStartingEnergy, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kEnergyCost, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP);
        unit_data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);  // don't move
        return unit_data;
    }

    Entity* Spawn(const Team team, const CombatUnitData& unit_data, const HexGridPosition position)
    {
        Entity* entity = nullptr;
        SpawnCombatUnit(team, position, unit_data, entity);
        return entity;
    }
};

TEST_F(MovingZonesAndPredefinedPositions, FromAllyBorderToFocus)
{
    // VISUALIZE_TEST_STEPS;

    auto unit_data = MakeCombatUnitData();

    // Omega spawns zone at the center of ally border. The zone moves to the target
    {
        unit_data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
        auto& ability = unit_data.type_data.omega_abilities.AddAbility();
        ability.total_duration_ms = 50;
        auto& skill = ability.AddSkill();
        skill.targeting.type = SkillTargetingType::kCurrentFocus;
        skill.deployment.type = SkillDeploymentType::kZone;
        skill.percentage_of_ability_duration = 100;
        skill.zone.duration_ms = 200'000;
        skill.zone.frequency_ms = 100;
        skill.zone.movement_speed_sub_units_per_time_step = 1500;
        skill.zone.shape = ZoneEffectShape::kHexagon;
        skill.zone.radius_units = 5;
        skill.zone.predefined_spawn_position = PredefinedGridPosition::kAllyBorderCenter;
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(42_fp));
    }

    // Spawn red combat unit. It will be in infinite stun
    {  // hide variable in scope to prevent using it again by mistake
        const auto red = Spawn(Team::kRed, unit_data, {-35, -73});
        ASSERT_NE(red, nullptr);
        Stun(*red);
        ASSERT_TRUE(EntityHelper::HasNegativeState(*red, EffectNegativeState::kStun));
    }

    // Spawn blue combat unit. It will do just one omega which spawns the zone
    const auto blue = Spawn(Team::kBlue, unit_data, {-108, 73});
    ASSERT_NE(blue, nullptr);

    EventHistory<EventType::kZoneCreated> zones_created(*world);
    ASSERT_TRUE(TimeStepUntilEvent<EventType::kZoneCreated>());  // Wait until blue entity spawns zone
    ASSERT_TRUE(TimeStepUntilEvent<EventType::kOnDamage>());     // Wait until red entity receives damage
}

TEST_F(MovingZonesAndPredefinedPositions, FromEnemyBorderToAllyBorder)
{
    // VISUALIZE_TEST_STEPS;

    auto unit_data = MakeCombatUnitData();

    // Omega spawns zone at the center of enemy border. The zone moves to an ally border
    {
        unit_data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
        auto& ability = unit_data.type_data.omega_abilities.AddAbility();
        ability.total_duration_ms = 50;
        auto& skill = ability.AddSkill();
        // Self targeting is the best for this case because it will allow to cast an ability even if there are no
        // targetable entities on the board at the moment
        skill.targeting.type = SkillTargetingType::kSelf;
        skill.deployment.type = SkillDeploymentType::kZone;
        skill.percentage_of_ability_duration = 100;
        skill.zone.duration_ms = 200'000;
        skill.zone.frequency_ms = 100;
        skill.zone.movement_speed_sub_units_per_time_step = 1500;
        skill.zone.shape = ZoneEffectShape::kHexagon;
        skill.zone.radius_units = 5;
        skill.zone.predefined_spawn_position = PredefinedGridPosition::kAllyBorderCenter;
        skill.zone.predefined_target_position = PredefinedGridPosition::kEnemyBorderCenter;
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(42_fp));
    }

    // Spawn red combat unit. It will be in infinite stun
    {  // hide variable in scope to prevent using it again by mistake
        const auto red = Spawn(Team::kRed, unit_data, {33, -73});
        ASSERT_NE(red, nullptr);
        Stun(*red);
        ASSERT_TRUE(EntityHelper::HasNegativeState(*red, EffectNegativeState::kStun));
    }

    // Spawn blue combat unit. It will do just one omega which spawns the zone
    const auto blue = Spawn(Team::kBlue, unit_data, {-108, 73});
    ASSERT_NE(blue, nullptr);

    EventHistory<EventType::kZoneCreated> zones_created(*world);
    ASSERT_TRUE(TimeStepUntilEvent<EventType::kZoneCreated>());  // Wait until blue entity spawns zone
    ASSERT_TRUE(TimeStepUntilEvent<EventType::kOnDamage>());     // Wait until red entity receives damage
}

TEST_F(MovingZonesAndPredefinedPositions, WarpwaveAssault)
{
    // VISUALIZE_TEST_STEPS;

    auto unit_data = MakeCombatUnitData();

    // Omega spawns wide rectangluar zone at the center of ally border. Zone moves to an enemy border
    {
        unit_data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
        auto& ability = unit_data.type_data.omega_abilities.AddAbility();
        ability.total_duration_ms = 50;
        auto& skill = ability.AddSkill();
        // Self targeting is the best for this case because it will allow to cast an ability even if there are no
        // targetable entities on the board at the moment
        skill.targeting.type = SkillTargetingType::kSelf;
        skill.deployment.type = SkillDeploymentType::kZone;
        skill.percentage_of_ability_duration = 100;
        skill.zone.duration_ms = 200'000;
        skill.zone.frequency_ms = 100;
        skill.zone.movement_speed_sub_units_per_time_step = 1500;
        skill.zone.shape = ZoneEffectShape::kRectangle;
        skill.zone.width_units = 150;
        skill.zone.height_units = 2;
        skill.zone.predefined_spawn_position = PredefinedGridPosition::kAllyBorderCenter;
        skill.zone.predefined_target_position = PredefinedGridPosition::kEnemyBorderCenter;
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(42_fp));
    }

    // Spawn red combat unit. It will be in infinite stun
    {  // hide variable in scope to prevent using it again by mistake
        const auto red = Spawn(Team::kRed, unit_data, {106, -73});
        ASSERT_NE(red, nullptr);
        Stun(*red);
        ASSERT_TRUE(EntityHelper::HasNegativeState(*red, EffectNegativeState::kStun));
    }

    // Spawn blue combat unit. It will do just one omega which spawns the zone
    const auto blue = Spawn(Team::kBlue, unit_data, {-108, 73});
    ASSERT_NE(blue, nullptr);

    EventHistory<EventType::kZoneCreated> zones_created(*world);
    ASSERT_TRUE(TimeStepUntilEvent<EventType::kZoneCreated>());  // Wait until blue entity spawns zone
    ASSERT_TRUE(TimeStepUntilEvent<EventType::kOnDamage>());     // Wait until red entity receives damage
}

}  // namespace simulation
