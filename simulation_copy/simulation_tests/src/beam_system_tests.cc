#include "ability_system_data_fixtures.h"
#include "components/beam_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/stats_component.h"
#include "gtest/gtest.h"
#include "utility/attached_effects_helper.h"
#include "utility/entity_helper.h"

namespace simulation
{
class BeamSystemTest : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    void SetUp() override
    {
        Super::SetUp();

        world->SubscribeToEvent(
            EventType::kBeamCreated,
            [this](const Event& event)
            {
                const auto& beam_data = event.Get<event_data::BeamCreated>();
                events_beam_created.emplace_back(beam_data);
            });
        world->SubscribeToEvent(
            EventType::kBeamDestroyed,
            [this](const Event& event)
            {
                const auto& beam_data = event.Get<event_data::BeamDestroyed>();
                events_beam_destroyed.emplace_back(beam_data);
            });
        world->SubscribeToEvent(
            EventType::kBeamActivated,
            [this](const Event& event)
            {
                const auto& beam_data = event.Get<event_data::BeamActivated>();
                events_beam_activated.emplace_back(beam_data);
            });
        world->SubscribeToEvent(
            EventType::kBeamUpdated,
            [this](const Event& event)
            {
                const auto& beam_data = event.Get<event_data::BeamUpdated>();
                events_beam_updated.emplace_back(beam_data);
            });
    }

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
        data.type_data.stats.Set(StatType::kStartingEnergy, 100_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);

        // Target dummy data
        dummy_data = CreateCombatUnitData();
        dummy_data.radius_units = 1;
        dummy_data.type_data.combat_affinity = CombatAffinity::kFire;
        dummy_data.type_data.combat_class = CombatClass::kBehemoth;
        dummy_data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        dummy_data.type_data.stats.Set(StatType::kAttackRangeUnits, 500_fp);
        dummy_data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        dummy_data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        dummy_data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
        dummy_data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.omega_abilities.AddAbility();
            ability.name = "Omega Ability";
            ability.total_duration_ms = 1000;

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Omega Beam Attack";
            SetSkillTargeting(skill1);
            skill1.deployment.type = SkillDeploymentType::kBeam;
            skill1.channel_time_ms = 800;
            skill1.percentage_of_ability_duration = 100;

            // Set the beam data
            skill1.beam.width_units = GetBeamWidthUnits();
            skill1.beam.frequency_ms = GetFrequencyMs();
            skill1.beam.apply_once = IsApplyOnce();
            skill1.beam.is_homing = IsHoming();
            skill1.beam.is_blockable = IsBlockable();

            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(10_fp));
        }
        data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual std::unordered_set<EntityID> GetHomingMovableIDs() const
    {
        return {};
    }

    virtual void SetSkillTargeting(SkillData& skill)
    {
        skill.targeting.type = SkillTargetingType::kDistanceCheck;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = IsClosestEnemy();
        skill.targeting.num = 1;
    }

    virtual int GetBeamWidthUnits() const
    {
        return 20;
    }
    virtual int GetFrequencyMs() const
    {
        return 200;
    }
    virtual bool IsApplyOnce() const
    {
        return false;
    }
    virtual bool IsHoming() const
    {
        return false;
    }
    virtual bool IsBlockable() const
    {
        return false;
    }

    virtual bool IsClosestEnemy() const
    {
        return true;
    }

    void InitOmegaAbilities() override {}
    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {-32, -65}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {-27, -55}, data, red_entity);
    }

    void SpawnExtraRed(const HexGridPosition& position)
    {
        EntityID out_id = kInvalidEntityID;
        SpawnExtraRed(position, &out_id);
    }
    void SpawnExtraRed(const HexGridPosition& position, EntityID* out_id)
    {
        // Spawn an extra red Combat Unit
        Entity* extra_entity = nullptr;
        SpawnCombatUnit(Team::kRed, position, data, extra_entity);

        // Don't let it move
        auto& movement_component = extra_entity->Get<MovementComponent>();
        movement_component.SetMovementSpeedSubUnits(0_fp);

        *out_id = extra_entity->GetID();
    }
    void SpawnBlockingUnit()
    {
        SpawnCombatUnit(Team::kBlue, {-32, -60}, dummy_data, blue_entity2);
    }

    void OutOfRangeTest(const EntityID miss_entity_id)
    {
        AddShields();

        // Run for 10 time steps
        for (int time_step = 0; time_step < 10; time_step++)
        {
            // TimeStep
            world->TimeStep();
        }

        // Iterate the entities
        auto& entities = world->GetAll();
        for (auto& entity : entities)
        {
            if (!EntityHelper::IsACombatUnit(*entity)) continue;

            const auto& stats_component = entity->Get<StatsComponent>();

            // Nothing should change due to out of range
            if (miss_entity_id == entity->GetID())
            {
                EXPECT_EQ(world->GetShieldTotal(*entity), 10_fp) << "entity_id = " << entity->GetID();
                EXPECT_EQ(stats_component.GetCurrentHealth(), 100_fp) << "entity_id = " << entity->GetID();
            }
            else
            {
                // Should have done some damage
                EXPECT_EQ(world->GetShieldTotal(*entity), 0_fp) << "entity_id = " << entity->GetID();
                EXPECT_LT(stats_component.GetCurrentHealth(), 100_fp) << "entity_id = " << entity->GetID();
            }
        }
    }

    void HitTest(const int expected_beams_count)
    {
        AddShields();

        // Keep track of health and shield (by entity id)
        std::unordered_map<EntityID, FixedPoint> last_health;
        std::unordered_map<EntityID, FixedPoint> last_shield;
        std::unordered_map<EntityID, int> change_count;

        // Run for 9 time steps
        for (int time_step = 0; time_step <= 8; time_step++)
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

                // Move if homing
                if (IsHoming())
                {
                    // Only move Combat Units that are whitelisted
                    if (!EntityHelper::IsACombatUnit(*entity) || !GetHomingMovableIDs().count(entity_id))
                    {
                        continue;
                    }

                    // Move the Combat Units
                    auto& position_component = entity->Get<PositionComponent>();
                    position_component.SetR(position_component.GetR() + 5);
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
                if (!IsHoming() && entity_id == 2)
                {
                    // Should not be affected
                    EXPECT_EQ(stats_component.GetCurrentHealth(), 100_fp)
                        << "time step = " << time_step << ", entity_id = " << entity_id;
                    continue;
                }

                if (time_step == 4)
                {
                    // No damage this time_step
                    EXPECT_EQ(world->GetShieldTotal(*entity), last_shield[entity_id])
                        << "time_step = " << time_step << ", entity_id = " << entity_id;
                    EXPECT_EQ(stats_component.GetCurrentHealth(), last_health[entity_id])
                        << "time_step = " << time_step << ", entity_id = " << entity_id;
                }
                else if (time_step == 9)
                {
                    // Blockable does not hit entity 0
                    if (!(entity_id == 0 && IsBlockable()))
                    {
                        // Should have sustained some damage by now
                        EXPECT_EQ(world->GetShieldTotal(*entity), 0_fp)
                            << "time_step = " << time_step << ", entity_id = " << entity_id;

                        // Apply once doesn't get through the shields
                        if (!IsApplyOnce())
                        {
                            EXPECT_LT(stats_component.GetCurrentHealth(), 100_fp)
                                << "time_step = " << time_step << ", entity_id = " << entity_id;
                        }
                    }

                    // Beam should have finished channeling
                    if (entity_id == 4)
                    {
                        EXPECT_FALSE(entity->IsActive()) << ", entity_id = " << entity_id;
                    }
                }
                else
                {
                    // Either no or some damage this time_step
                    EXPECT_LE(world->GetShieldTotal(*entity), last_shield[entity_id])
                        << "time_step = " << time_step << ", entity_id = " << entity_id;
                    EXPECT_LE(stats_component.GetCurrentHealth(), last_health[entity_id])
                        << "time_step = " << time_step << ", entity_id = " << entity_id;
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

        // Destroy the beams on 10th time step
        world->TimeStep();

        EXPECT_EQ(events_beam_created.size(), expected_beams_count);
        EXPECT_EQ(events_beam_destroyed.size(), expected_beams_count);

        for (size_t i = 0; i < events_beam_created.size(); i++)
        {
            const auto& event_data = events_beam_created.at(i);
            const auto& beam_entity = world->GetByID(event_data.entity_id);
            const auto& beam_component = beam_entity.Get<BeamComponent>();

            // These values could have changed for a homing beam
            if (!IsHoming())
            {
                EXPECT_EQ(beam_component.GetDirectionDegrees(), event_data.beam_direction_degrees);
                ASSERT_NE(event_data.beam_direction_degrees, 0);
            }
        }

        // 4 activations per beam
        EXPECT_EQ(events_beam_activated.size(), expected_beams_count * 4);

        for (size_t i = 0; i < events_beam_activated.size(); i++)
        {
            const auto& event_data = events_beam_activated.at(i);
            const auto& beam_entity = world->GetByID(event_data.entity_id);
            const auto& beam_component = beam_entity.Get<BeamComponent>();

            // These values could have changed for a homing beam
            if (!IsHoming())
            {
                EXPECT_EQ(beam_component.GetDirectionDegrees(), event_data.beam_direction_degrees);
                ASSERT_NE(event_data.beam_direction_degrees, 0);
            }
        }

        if (IsHoming())
        {
            EXPECT_EQ(events_beam_updated.size(), 14);
        }
        else
        {
            EXPECT_EQ(events_beam_updated.size(), 0);
        }

        // Check changes
        const auto& entities = world->GetAll();
        for (const auto& entity : entities)
        {
            const EntityID entity_id = entity->GetID();
            if (!EntityHelper::IsACombatUnit(*entity)) continue;

            // If testing for apply once, make sure not changed more than once
            if (IsApplyOnce())
            {
                EXPECT_LE(change_count[entity_id], 1);
            }
            // else
            // {
            //     EXPECT_GE(change_count[entity_id], 1);
            // }
        }
    }

    CombatUnitData dummy_data;
    std::vector<event_data::BeamCreated> events_beam_created;
    std::vector<event_data::BeamActivated> events_beam_activated;
    std::vector<event_data::BeamUpdated> events_beam_updated;
    std::vector<event_data::BeamDestroyed> events_beam_destroyed;
    Entity* blue_entity2 = nullptr;
};

class BeamSystemNoDataTest : public BaseTest
{
};

class BeamSystemApplyOnceTest : public BeamSystemTest
{
    typedef BeamSystemTest Super;

protected:
    bool IsApplyOnce() const override
    {
        return true;
    }
};

class BeamSystemHomingTest : public BeamSystemTest
{
    typedef BeamSystemTest Super;

protected:
    std::unordered_set<EntityID> GetHomingMovableIDs() const override
    {
        // Entity with ID 1
        return {1};
    }

    bool IsHoming() const override
    {
        return true;
    }
};

class BeamSystemBlockableTest : public BeamSystemTest
{
    typedef BeamSystemTest Super;

protected:
    bool IsBlockable() const override
    {
        return true;
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.omega_abilities.AddAbility();
            ability.name = "Omega Ability";
            ability.total_duration_ms = 1000;

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Omega Beam Attack";
            SetSkillTargeting(skill1);
            skill1.deployment.type = SkillDeploymentType::kBeam;
            skill1.channel_time_ms = 800;
            skill1.percentage_of_ability_duration = 100;

            // Set the beam data
            skill1.beam.width_units = GetBeamWidthUnits();
            skill1.beam.frequency_ms = GetFrequencyMs();
            skill1.beam.apply_once = IsApplyOnce();
            skill1.beam.is_homing = IsHoming();
            skill1.beam.is_blockable = IsBlockable();
            skill1.beam.block_allegiance = AllegianceType::kEnemy;

            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(10_fp));
        }
        data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }
};

class BeamSystemTestOutOfRange : public BeamSystemTest
{
    typedef BeamSystemTest Super;

protected:
    int GetBeamWidthUnits() const override
    {
        return 8;
    }
};

class BeamMultiAngleTest : public BeamSystemTest
{
    typedef BeamSystemTest Super;

    void SetSkillTargeting(SkillData& skill) override
    {
        skill.targeting.type = SkillTargetingType::kCombatStatCheck;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = false;
        skill.targeting.stat_type = StatType::kEnergyCost;
        skill.targeting.num = 1;
    }

    void SpawnCombatUnits() override
    {
        // Graph: https://www.geogebra.org/calculator/hdeqczaq

        // Spawn attacking Combat Unit
        // Entity 0
        SpawnCombatUnit(Team::kBlue, {-32, -65}, data, blue_entity);

        // Spawn focus at 45 degrees
        // Entity 1
        SpawnCombatUnit(Team::kRed, {-27, -55}, dummy_data, red_entity);

        // Check success
        // Spawn extra targets (misses)
        // Entity 2 - At 306 degrees
        Entity* temp = nullptr;
        SpawnCombatUnit(Team::kRed, {13, -70}, dummy_data, temp);
        // Entity 3 - At 56 degrees
        SpawnCombatUnit(Team::kRed, {-12, -5}, dummy_data, temp);
        // Entity 4 - At 26 degrees
        SpawnCombatUnit(Team::kRed, {-12, -46}, dummy_data, temp);
        // Entity 5 - At 18 degrees
        SpawnCombatUnit(Team::kRed, {-17, -55}, dummy_data, temp);
        // Entity 6 - At 14 degrees
        SpawnCombatUnit(Team::kRed, {-12, -55}, dummy_data, temp);

        // Spawn extra targets (hits)
        // Entity 7 - At 51 degrees
        SpawnCombatUnit(Team::kRed, {-15, -23}, dummy_data, temp);

        // Entity 8 - At 48 degrees
        // The target of entity 0
        auto dummy_data_highest_energy_cost = dummy_data;
        dummy_data_highest_energy_cost.type_data.stats.Set(StatType::kEnergyCost, 1000_fp);
        SpawnCombatUnit(Team::kRed, {-12, -25}, dummy_data_highest_energy_cost, temp);

        // Entity 9 - At 41 degrees
        SpawnCombatUnit(Team::kRed, {-12, -30}, dummy_data, temp);

        ASSERT_EQ(world->GetAll().size(), 10);
    }

    int GetBeamWidthUnits() const override
    {
        return 10;
    }
};

class BeamHitSideTest : public BeamSystemTest
{
    typedef BeamSystemTest Super;

    bool IsClosestEnemy() const override
    {
        return false;
    }

    void SpawnCombatUnits() override
    {
        // Graph: https://www.geogebra.org/calculator/aeamq9jy

        // Modify data for test
        data.radius_units = 2;
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 500_fp);

        // Spawn attacking CombatUnit
        // Entity 0
        SpawnCombatUnit(Team::kBlue, {-25, -50}, data, blue_entity);

        // Spawn focus direct to the right, farthest enemy
        // Entity 1
        SpawnCombatUnit(Team::kRed, {13, -50}, dummy_data, red_entity);

        // Spawn extra target away from center line - just clip the closer edge
        // Entity 2
        Entity* red_dummy2 = nullptr;
        SpawnCombatUnit(Team::kRed, {-12, -40}, dummy_data, red_dummy2);

        // Spawn extra target further away from center line - just clip the further edge
        // Entity 3
        Entity* red_dummy3 = nullptr;
        SpawnCombatUnit(Team::kRed, {-12, -60}, dummy_data, red_dummy3);

        // Spawn extra target even further away from center line - go too far
        // Entity 4
        Entity* red_dummy4 = nullptr;
        SpawnCombatUnit(Team::kRed, {-12, -65}, dummy_data, red_dummy4);
    }
};

class BeamDoesntHitBehindTest : public BeamSystemHomingTest
{
    typedef BeamSystemHomingTest Super;

    bool IsClosestEnemy() const override
    {
        return false;
    }

    int GetBeamWidthUnits() const override
    {
        return 3;
    }

    void SpawnCombatUnits() override
    {
        // Spawn attacking CombatUnit
        // Entity 0
        SpawnCombatUnit(Team::kBlue, {-32, -65}, data, blue_entity);

        // Spawn farthest enemy
        // Entity 1
        SpawnCombatUnit(Team::kRed, {-27, -55}, dummy_data, red_entity);

        // Spawn entity just behind attacking
        // Entity 2
        Entity* red_dummy2 = nullptr;
        SpawnCombatUnit(Team::kRed, {-34, -67}, dummy_data, red_dummy2);
    }
};

TEST_F(BeamSystemNoDataTest, InvalidFrequencyNoCrash)
{
    // CombatUnit stats
    CombatUnitData data;
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 20_fp);
    data.type_data.stats.Set(StatType::kStartingEnergy, 100_fp);
    data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
    data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);

    // Add attack abilities
    {
        auto& ability = data.type_data.omega_abilities.AddAbility();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.lowest = true;
        skill1.targeting.num = 1;
        skill1.deployment.type = SkillDeploymentType::kBeam;
        skill1.channel_time_ms = 800;

        // Set the beam data
        skill1.beam.width_units = 20;
        skill1.beam.frequency_ms = 0;

        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(10_fp));
    }
    data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;

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

TEST_F(BeamSystemTestOutOfRange, SimpleBeamOutOfRange)
{
    // Right behind red
    EntityID extra_id = kInvalidEntityID;
    SpawnExtraRed(HexGridPosition{-25, -51}, &extra_id);
    OutOfRangeTest(extra_id);
}

TEST_F(BeamSystemTest, SimpleBeamHit)
{
    SpawnExtraRed(HexGridPosition{-25, -45});
    HitTest(3);
}

TEST_F(BeamSystemTest, SimpleBeamHit2)
{
    // SpawnExtraRed(HexGridPosition{-25, -45});
    HitTest(2);
}

TEST_F(BeamSystemHomingTest, HomingBeamHit)
{
    SpawnExtraRed(HexGridPosition{-25, -50});
    HitTest(3);
}

TEST_F(BeamSystemHomingTest, HomingBeamHit2)
{
    HitTest(2);
}

TEST_F(BeamSystemBlockableTest, BlockableBeamHit)
{
    SpawnExtraRed(HexGridPosition{-25, -50});
    SpawnBlockingUnit();
    HitTest(3);
}

TEST_F(BeamSystemApplyOnceTest, ApplyOnce)
{
    HitTest(2);
}

TEST_F(BeamSystemTest, BeamInterruption)
{
    // Keep track of health (by entity id)
    std::map<EntityID, FixedPoint> last_health;
    std::map<EntityID, int> active_change_count;
    std::map<EntityID, int> interrupted_change_count;

    // Run for 5 time steps to get things going
    for (int time_step = 0; time_step < 5; time_step++)
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

            // Init change count
            if (active_change_count.count(entity_id) == 0)
            {
                active_change_count[entity_id] = 0;
            }
        }

        world->TimeStep();

        // Iterate the entities (we don't care about new entities)
        for (const auto& entity : entities)
        {
            const EntityID entity_id = entity->GetID();
            if (!entity->IsActive()) continue;
            if (!EntityHelper::IsACombatUnit(*entity)) continue;

            const auto& stats_component = entity->Get<StatsComponent>();

            // Update change count if changed
            if (stats_component.GetCurrentHealth() != last_health[entity_id])
            {
                active_change_count[entity_id]++;
            }

            last_health[entity_id] = stats_component.GetCurrentHealth();
        }
    }

    // Interrupt the beams
    for (const auto& entity : world->GetAll())
    {
        const EntityID entity_id = entity->GetID();
        if (!EntityHelper::IsACombatUnit(*entity))
        {
            continue;
        }

        // Stun for 5 time steps
        const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kStun, 500);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(entity_id);
        GetAttachedEffectsHelper().AddAttachedEffect(*entity, entity_id, effect_data, effect_state);
    }

    // Run for 5 time steps
    for (int time_step = 5; time_step < 10; time_step++)
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

            // Init change count
            if (interrupted_change_count.count(entity_id) == 0)
            {
                interrupted_change_count[entity_id] = 0;
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

            // Update change count if changed
            if (stats_component.GetCurrentHealth() != last_health[entity_id])
            {
                interrupted_change_count[entity_id]++;
            }

            last_health[entity_id] = stats_component.GetCurrentHealth();
        }
    }

    // Iterate the entities to check changes and state
    for (const auto& entity : world->GetAll())
    {
        const EntityID entity_id = entity->GetID();

        // Check changes for Combat Units
        if (EntityHelper::IsACombatUnit(*entity))
        {
            EXPECT_EQ(active_change_count[entity_id], 2);
            EXPECT_EQ(interrupted_change_count[entity_id], 0);
        }

        // Check not active for beams
        if (EntityHelper::IsABeam(*world, entity_id))
        {
            EXPECT_FALSE(entity->IsActive());
        }
    }
}

TEST_F(BeamMultiAngleTest, MultiAngleTest)
{
    // Keep track of health (by entity id)
    std::map<EntityID, FixedPoint> last_health;
    std::map<EntityID, int> change_count;

    // Run for 5 time steps
    for (int time_step = 0; time_step < 5; time_step++)
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

            // Update change count if changed
            if (stats_component.GetCurrentHealth() != last_health[entity_id])
            {
                change_count[entity_id]++;
            }

            last_health[entity_id] = stats_component.GetCurrentHealth();
        }
    }

    // Check correct entities changed
    // Don't hit self
    EXPECT_EQ(change_count[0], 0);

    // Direct focus hit
    EXPECT_EQ(change_count[1], 2);

    // All misses
    EXPECT_EQ(change_count[2], 0);
    EXPECT_EQ(change_count[3], 0);
    EXPECT_EQ(change_count[4], 0);
    EXPECT_EQ(change_count[5], 0);
    EXPECT_EQ(change_count[6], 0);

    // Hits
    EXPECT_EQ(change_count[7], 2);
    EXPECT_EQ(change_count[8], 2);
    EXPECT_EQ(change_count[9], 2);
}

TEST_F(BeamHitSideTest, HitSideTest)
{
    // Keep track of health (by entity id)
    std::map<EntityID, FixedPoint> last_health;
    std::map<EntityID, int> change_count;

    // Run for 5 time steps
    for (int time_step = 0; time_step < 5; time_step++)
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

            // Update change count if changed
            if (stats_component.GetCurrentHealth() != last_health[entity_id])
            {
                change_count[entity_id]++;
            }

            last_health[entity_id] = stats_component.GetCurrentHealth();
        }
    }

    // Check correct entities changed
    // Don't hit self
    EXPECT_EQ(change_count[0], 0);

    // Direct focus hit
    EXPECT_EQ(change_count[1], 2);

    // Hit off side entities
    EXPECT_EQ(change_count[2], 2);
    EXPECT_EQ(change_count[3], 2);

    // Miss too far entity
    EXPECT_EQ(change_count[4], 0);
}

TEST_F(BeamDoesntHitBehindTest, DoesntHitTargetBehindTest)
{
    // Keep track of health (by entity id)
    std::map<EntityID, FixedPoint> last_health;
    std::map<EntityID, int> change_count;

    // Run for 5 time steps
    for (int time_step = 0; time_step < 5; time_step++)
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

            // Update change count if changed
            if (stats_component.GetCurrentHealth() != last_health[entity_id])
            {
                change_count[entity_id]++;
            }

            last_health[entity_id] = stats_component.GetCurrentHealth();
        }
    }

    // Check correct entities changed
    // Don't hit self
    EXPECT_EQ(change_count[0], 0);

    // Hit farthest entity
    EXPECT_EQ(change_count[1], 2);

    // Should not hit entity behind
    EXPECT_EQ(change_count[2], 0);
}

class BeamMinimalTest : public BaseTest
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

TEST_F(BeamMinimalTest, BlockingBeam_SmallDistance)
{
    // VISUALIZE_TEST_STEPS;

    auto unit_data = MakeCombatUnitData();

    // Omega ability with beam
    {
        unit_data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
        auto& ability = unit_data.type_data.omega_abilities.AddAbility();
        ability.total_duration_ms = 5000;

        auto& skill = ability.AddSkill();

        // select furthest enemy
        skill.targeting.type = SkillTargetingType::kDistanceCheck;
        skill.targeting.lowest = false;
        skill.targeting.num = 1;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.guidance = MakeEnumSet(GuidanceType::kGround);

        skill.deployment.type = SkillDeploymentType::kBeam;
        skill.percentage_of_ability_duration = 100;
        skill.channel_time_ms = 5000;
        skill.beam.is_blockable = true;
        skill.beam.block_allegiance = AllegianceType::kEnemy;
        skill.beam.width_units = 2;
        skill.beam.frequency_ms = 100;
        skill.beam.apply_once = true;
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(42_fp));
    }

    // Spawn red combat units. They will be in infinite stun
    const std::array<HexGridPosition, 2> red_positions{{{38, 7}, {29, 21}}};
    const std::array<int, 2> red_radiuses{4, 7};
    std::vector<Entity*> red_units;
    for (size_t red_index = 0; red_index != red_positions.size(); ++red_index)
    {
        const auto& red_position = red_positions[red_index];
        unit_data.radius_units = red_radiuses[red_index];
        const auto red = Spawn(Team::kRed, unit_data, red_position);
        ASSERT_NE(red, nullptr);
        Stun(*red);
        ASSERT_TRUE(EntityHelper::HasNegativeState(*red, EffectNegativeState::kStun));
        red_units.push_back(red);
    }

    // Spawn blue combat unit. It will do just one omega which spawns the zone
    const HexGridPosition blue_pos = {58, -24};
    const auto blue = Spawn(Team::kBlue, unit_data, blue_pos);
    ASSERT_NE(blue, nullptr);

    EventHistory<EventType::kOnDamage> damage_events(*world);
    EventHistory<EventType::kBeamCreated> beam_created_events(*world);
    ASSERT_TRUE(TimeStepUntilEvent<EventType::kBeamDestroyed>());

    // Check there is only one damage event and it comes from the beam
    ASSERT_EQ(beam_created_events.Size(), 1);
    ASSERT_EQ(damage_events.Size(), 1);
    const auto& damage_event = damage_events.events.front();
    ASSERT_EQ(damage_event.damage_value, 42_fp);
    ASSERT_EQ(damage_event.damage_type, EffectDamageType::kPure);
    ASSERT_EQ(damage_event.sender_id, beam_created_events.events.front().entity_id);

    const auto closest_red_entity = std::min_element(
        red_positions.begin(),
        red_positions.end(),
        [&](const HexGridPosition& a, const HexGridPosition& b)
        {
            return (a - blue_pos).Length() < (b - blue_pos).Length();
        });
    ASSERT_EQ(*closest_red_entity, world->GetByID(damage_event.receiver_id).Get<PositionComponent>().GetPosition());
}

}  // namespace simulation
