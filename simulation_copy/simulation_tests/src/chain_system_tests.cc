#include <memory>

#include "base_test_fixtures.h"
#include "components/attached_effects_component.h"
#include "components/stats_component.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "gtest/gtest.h"
#include "utility/attached_effects_helper.h"
#include "utility/entity_helper.h"

namespace simulation
{
class ChainSystemTest : public BaseTest
{
    typedef BaseTest Super;

protected:
    virtual void InitCombatUnitData()
    {
        data_no_chain = CreateCombatUnitData();
        data_with_chain = CreateCombatUnitData();

        InitStats();
        InitAttackAbilities();
        // InitOmegaAbilities();
    }

    virtual void InitStats()
    {
        // CombatUnit stats
        // No abilities

        data_no_chain.radius_units = 1;
        data_no_chain.type_data.combat_affinity = CombatAffinity::kFire;
        data_no_chain.type_data.combat_class = CombatClass::kBehemoth;
        data_no_chain.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data_no_chain.type_data.stats.Set(StatType::kAttackRangeUnits, 100_fp);
        data_no_chain.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        data_no_chain.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data_no_chain.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
        data_no_chain.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);

        // Chain Combat Units
        data_with_chain.radius_units = 1;
        data_with_chain.type_data.combat_affinity = CombatAffinity::kFire;
        data_with_chain.type_data.combat_class = CombatClass::kBehemoth;
        data_with_chain.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data_with_chain.type_data.stats.Set(StatType::kAttackRangeUnits, 100_fp);
        data_with_chain.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        data_with_chain.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data_with_chain.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
        data_with_chain.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
        data_with_chain.type_data.stats.Set(StatType::kAttackSpeed, FixedPoint::FromInt(GetAttackSpeed()));
    }

    virtual void InitAttackAbilities()
    {
        // Add attack abilities
        {
            auto& ability = data_with_chain.type_data.attack_abilities.AddAbility();
            ability.name = "Chain Ability";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Chain Attack";
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.targeting.type = SkillTargetingType::kDistanceCheck;
            skill1.targeting.group = AllegianceType::kEnemy;
            skill1.targeting.lowest = true;
            skill1.targeting.num = 1;

            AddEffectChainPropagation(skill1);
            AddEffectsExtra(skill1);
        }

        data_no_chain.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
        data_with_chain.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual void AddEffectChainPropagation(SkillData& skill)
    {
        auto& propagation = skill.effect_package.attributes.propagation;
        propagation.type = EffectPropagationType::kChain;
        propagation.chain_number = GetChainNumber();
        propagation.chain_delay_ms = GetChainDelayMs();
        propagation.only_new_targets = GetChainOnlyNewTargets();
        propagation.prioritize_new_targets = GetChainPrioritizeNewTargets();
        propagation.add_original_effect_package = GetChainAddOriginalEffectPackage();
        propagation.effect_package = std::make_shared<EffectPackage>();
        propagation.effect_package->AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(10_fp));
    }

    virtual void AddEffectsExtra(SkillData& skill)
    {
        const auto negative_effect = EffectData::CreateNegativeState(EffectNegativeState::kRoot, kTimeInfinite);
        skill.effect_package.effects.emplace_back(negative_effect);
    }

    virtual void SpawnCombatUnits()
    {
        // Add blue Combat Units
        // 0.
        SpawnCombatUnit(Team::kBlue, HexGridPosition(-25, -50), data_no_chain, blue1);
        // 1.
        SpawnCombatUnit(Team::kBlue, HexGridPosition(-12, -50), data_no_chain, blue2);

        // Spaced off to the side so it doesn't get hit
        // 2.
        SpawnCombatUnit(Team::kBlue, HexGridPosition(23, -50), data_no_chain, blue3);
        // 3.
        SpawnCombatUnit(Team::kBlue, HexGridPosition(-22, -25), data_with_chain, blue4);

        // Add red Combat Units
        // 4.
        SpawnCombatUnit(Team::kRed, HexGridPosition(-25, 25), data_no_chain, red1);
        // 5.
        SpawnCombatUnit(Team::kRed, HexGridPosition(-12, 25), data_no_chain, red2);

        // Spaced off to the side so it doesn't get hit
        // 6.
        SpawnCombatUnit(Team::kRed, HexGridPosition(13, 25), data_no_chain, red3);
        // 7.
        SpawnCombatUnit(Team::kRed, HexGridPosition(-12, 2), data_with_chain, red4);
    }

    virtual int GetAttackSpeed() const
    {
        return 100;
    }

    virtual int GetChainDelayMs() const
    {
        return 0;
    }
    virtual int GetChainNumber() const
    {
        return 3;
    }
    virtual bool GetChainOnlyNewTargets() const
    {
        return true;
    }
    virtual bool GetChainPrioritizeNewTargets() const
    {
        return true;
    }
    virtual bool GetChainAddOriginalEffectPackage() const
    {
        return true;
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();

        world->SubscribeToEvent(
            EventType::kChainCreated,
            [this](const Event& event)
            {
                const auto& chain_data = event.Get<event_data::ChainCreated>();
                chain_created_time_steps[chain_data.entity_id] = world->GetTimeStepCount();
                events_chain_created.emplace_back(chain_data);
            });
        world->SubscribeToEvent(
            EventType::kChainDestroyed,
            [this](const Event& event)
            {
                const auto& chain_data = event.Get<event_data::ChainDestroyed>();
                chain_destroyed_time_steps[chain_data.entity_id] = world->GetTimeStepCount();
                if (chain_data.is_last_chain)
                {
                    count_chain_destroyed_is_last_chain++;
                }
                events_chain_destroyed.emplace_back(chain_data);
            });

        world->SubscribeToEvent(
            EventType::kTryToApplyEffect,
            [this](const Event& event)
            {
                const auto& event_data = event.Get<event_data::Effect>();
                events_apply_effect.emplace_back(event_data);

                // Only count chains
                if (!EntityHelper::IsAChain(*world, event_data.sender_id))
                {
                    return;
                }

                if (event_data.data.type_id.type == EffectType::kInstantDamage)
                {
                    count_apply_damage_from_chains++;
                }
                else if (event_data.data.type_id.type == EffectType::kNegativeState)
                {
                    count_apply_negative_state_from_chains++;
                }
            });

        SpawnCombatUnits();
    }

    void CalculateEntitiesDamaged()
    {
        entities_damaged_count = 0;
        entities_damaged.clear();

        // Iterate the entities (we don't care about new entities)
        const auto& entities = world->GetAll();
        for (const auto& entity : entities)
        {
            const EntityID entity_id = entity->GetID();
            if (!entity->IsActive()) continue;
            if (!EntityHelper::IsACombatUnit(*entity)) continue;

            const auto& stats_component = entity->Get<StatsComponent>();
            if (stats_component.GetCurrentHealth() < 100_fp)
            {
                entities_damaged[entity_id] = true;
                entities_damaged_count++;
            }
        }
    }

    void CheckChainsTotalDuration(const int expected_total_time_steps)
    {
        ASSERT_EQ(chain_created_time_steps.size(), chain_destroyed_time_steps.size());

        for (const auto& [chain_id, created_time_steps] : chain_created_time_steps)
        {
            ASSERT_TRUE(chain_destroyed_time_steps.count(chain_id) > 0)
                << "chain_id = " << chain_id << "only has a created_time_steps =" << created_time_steps
                << ", but not a destroyed_time_steps";

            const int destroyed_time_steps = chain_destroyed_time_steps.at(chain_id);
            const int actual_total_duration = destroyed_time_steps - created_time_steps;
            EXPECT_EQ(actual_total_duration, expected_total_time_steps)
                << "chain_id = " << chain_id << ", created_time_steps = " << created_time_steps
                << ", destroyed_time_steps = " << destroyed_time_steps
                << ", does not have the total lifetime duration of the chain";
        }
    }

    CombatUnitData data_no_chain;
    CombatUnitData data_with_chain;
    Entity* blue1 = nullptr;
    Entity* blue2 = nullptr;
    Entity* blue3 = nullptr;
    Entity* blue4 = nullptr;
    Entity* red1 = nullptr;
    Entity* red2 = nullptr;
    Entity* red3 = nullptr;
    Entity* red4 = nullptr;

    int entities_damaged_count = 0;
    std::map<EntityID, bool> entities_damaged;
    int count_apply_damage_from_chains = 0;
    int count_apply_negative_state_from_chains = 0;

    std::vector<event_data::Effect> events_apply_effect;
    std::vector<event_data::ChainCreated> events_chain_created;
    std::vector<event_data::ChainDestroyed> events_chain_destroyed;
    int count_chain_destroyed_is_last_chain = 0;

    // Maps from chain id to bounces time steps
    std::unordered_map<EntityID, std::vector<int>> chain_bounces_time_steps;

    // Maps from chain id to time step it was created
    std::unordered_map<EntityID, int> chain_created_time_steps;

    // Maps from chain id to time step it was destroyed
    std::unordered_map<EntityID, int> chain_destroyed_time_steps;
};

class ChainSystemOmegaTest : public ChainSystemTest
{
    typedef ChainSystemTest Super;

protected:
    void InitCombatUnitData() override
    {
        data_no_chain = CreateCombatUnitData();
        data_with_chain = CreateCombatUnitData();

        InitStats();
        InitOmegaAbilities();
        InitAttackAbilities();
    }

    void InitStats() override
    {
        // CombatUnit stats
        // No abilities

        data_no_chain.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data_no_chain.type_data.stats.Set(StatType::kAttackRangeUnits, 100_fp);
        data_no_chain.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        data_no_chain.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data_no_chain.type_data.stats.Set(StatType::kMaxHealth, 100_fp);

        // Chain Combat Units
        data_with_chain.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data_with_chain.type_data.stats.Set(StatType::kAttackRangeUnits, 100_fp);
        data_with_chain.type_data.stats.Set(StatType::kEnergyCost, 0_fp);
        data_with_chain.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
        data_with_chain.type_data.stats.Set(StatType::kCritChancePercentage, 100_fp);
        data_with_chain.type_data.stats.Set(StatType::kAttackSpeed, FixedPoint::FromInt(GetAttackSpeed()));
    }

    virtual void InitOmegaAbilities()
    {
        // Add attack abilities
        {
            auto& ability = data_with_chain.type_data.omega_abilities.AddAbility();
            ability.name = "Chain Ability";

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Chain Attack";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.targeting.group = AllegianceType::kEnemy;
            skill1.targeting.lowest = true;

            AddEffectChainPropagation(skill1);
            AddEffectsExtra(skill1);
        }

        data_no_chain.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
        data_with_chain.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void SetUp() override
    {
        Super::SetUp();
        InitCombatUnitData();
    }
};

class ChainSystemTestWith1Chain : public ChainSystemTest
{
    typedef ChainSystemTest Super;

protected:
    int GetChainNumber() const override
    {
        return 1;
    }
};

class ChainSystemTestSlow : public ChainSystemTest
{
    typedef ChainSystemTest Super;

protected:
    // Does an attack every 5 seconds
    int GetAttackSpeed() const override
    {
        return 20;
    }
    int GetChainDelayMs() const override
    {
        return 500;
    }
};

// Slow chain system where there are not enough targets
class ChainSystemTestSlowNotEnoughTargets : public ChainSystemTestSlow
{
    typedef ChainSystemTestSlow Super;

protected:
    void SpawnCombatUnits() override
    {
        // Add blue Combat Units
        Entity* blue_dummy1 = nullptr;
        SpawnCombatUnit(Team::kBlue, HexGridPosition(-12, -25), data_with_chain, blue_dummy1);

        // Add red Combat Units
        Entity* red_dummy1 = nullptr;
        SpawnCombatUnit(Team::kRed, HexGridPosition(-25, 25), data_no_chain, red_dummy1);
    }
};

TEST_F(ChainSystemTest, SimpleChain)
{
    world->TimeStep();

    TimeStepForNTimeSteps(4);

    // Should be chain number times 2 entities
    ASSERT_EQ(events_chain_created.size(), GetChainNumber() * 2);

    // on the first application skill applies negative state to the target and
    // starts chain, each application of which will apply 2 effects - damage and
    // negative effect. So (3 jumps) * 2 = 6 effect applications during propagation
    // plus one initial negative effect. That's 7 effects applications triggered by
    // one entity with propagation ability. This setup contains two such entities so
    // total number of effects applications is 14
    ASSERT_EQ(events_apply_effect.size(), 14);

    // 3 apply damage from each chain
    // 3 negative state from each chain
    // And there are two chains in the world
    // Negative state are in the chain propagation because of add_original_effect_package
    ASSERT_EQ(count_apply_damage_from_chains, 6);
    ASSERT_EQ(count_apply_negative_state_from_chains, 6);

    // Should remove the last chains
    TimeStepForNTimeSteps(1);

    // Destroyed as many as there were created
    ASSERT_EQ(events_chain_destroyed.size(), GetChainNumber() * 2);
    ASSERT_EQ(count_chain_destroyed_is_last_chain, 2) << "2 entities should have been activated";

    CalculateEntitiesDamaged();

    // 6 of the 8 entities should have taken damage
    EXPECT_EQ(entities_damaged_count, 6);

    // These specific entities (based on positions specified earlier)
    // Entities 2 and 6 are off to the side
    EXPECT_TRUE(entities_damaged[0]);
    EXPECT_TRUE(entities_damaged[1]);
    EXPECT_FALSE(entities_damaged[2]);
    EXPECT_TRUE(entities_damaged[3]);
    EXPECT_TRUE(entities_damaged[4]);
    EXPECT_TRUE(entities_damaged[5]);
    EXPECT_FALSE(entities_damaged[6]);
    EXPECT_TRUE(entities_damaged[7]);
}

TEST_F(ChainSystemTestWith1Chain, OneChain)
{
    TimeStepForNTimeSteps(2);
    ASSERT_EQ(events_chain_created.size(), GetChainNumber() * 2);

    // 1 from the original effect + 2 from 1 chain bounces = 3
    // we have 2 entities so = 6
    ASSERT_EQ(events_apply_effect.size(), 6);

    // 1 apply damage from each chain
    // 1 negative state from each chain
    // And there are two chains in the world
    // Negative state are in the chain propagation because of add_original_effect_package
    ASSERT_EQ(count_apply_damage_from_chains, 2);
    ASSERT_EQ(count_apply_negative_state_from_chains, 2);

    // Should remove the chains
    TimeStepForNTimeSteps(1);

    // Destroyed as many as there were created
    ASSERT_EQ(events_chain_destroyed.size(), GetChainNumber() * 2);
    ASSERT_EQ(count_chain_destroyed_is_last_chain, 2);

    CalculateEntitiesDamaged();

    // 2 of the 8 entities should have taken damage
    EXPECT_EQ(entities_damaged_count, 2);

    // These specific entities (based on positions specified earlier)
    EXPECT_TRUE(entities_damaged[3]);
    EXPECT_TRUE(entities_damaged[7]);
}

TEST_F(ChainSystemTestSlow, SlowChain)
{
    // Run for 8 seconds - enough time for 2 activations and destruction
    const int expected_chain_activations = 2;
    TimeStepForSeconds(8);
    ASSERT_EQ(events_chain_created.size(), GetChainNumber() * 2 * expected_chain_activations);
    ASSERT_EQ(events_chain_destroyed.size(), GetChainNumber() * 2 * expected_chain_activations);
    ASSERT_EQ(count_chain_destroyed_is_last_chain, 4);

    CalculateEntitiesDamaged();

    // Chains should last GetChainDelayMs() + 1 (destroy time)
    const int expected_total_duration_time_steps = Time::MsToTimeSteps(GetChainDelayMs()) + 1;
    CheckChainsTotalDuration(expected_total_duration_time_steps);

    // 6 of the entities should have taken damage because of the 2 activations
    EXPECT_EQ(entities_damaged_count, 6);
    EXPECT_EQ(entities_damaged.size(), 6);

    // These specific entities (based on positions specified earlier)
    // Entities 2 and 6 are off to the side
    EXPECT_TRUE(entities_damaged[0]);
    EXPECT_TRUE(entities_damaged[1]);
    EXPECT_FALSE(entities_damaged[2]);
    EXPECT_TRUE(entities_damaged[3]);
    EXPECT_TRUE(entities_damaged[4]);
    EXPECT_TRUE(entities_damaged[5]);
    EXPECT_FALSE(entities_damaged[6]);
    EXPECT_TRUE(entities_damaged[7]);
}

TEST_F(ChainSystemTestSlowNotEnoughTargets, SlowChain)
{
    // Run for 6 seconds
    // But only 2 chains created because there are not enough targets
    TimeStepForSeconds(6);
    ASSERT_EQ(events_chain_created.size(), 2);

    // Should be destroyed as there are no more targets
    ASSERT_EQ(events_chain_destroyed.size(), 2);
    ASSERT_EQ(count_chain_destroyed_is_last_chain, 2);

    CalculateEntitiesDamaged();

    // Chains should last GetChainDelayMs() + 1 (destroy time)
    const int expected_total_duration_time_steps = Time::MsToTimeSteps(GetChainDelayMs()) + 1;
    CheckChainsTotalDuration(expected_total_duration_time_steps);

    // Just one entity should be damaged
    EXPECT_EQ(entities_damaged_count, 1);
    EXPECT_EQ(entities_damaged.size(), 1);

    // These specific entities (based on positions specified earlier)
    // Entities 2 and 6 are off to the side
    EXPECT_TRUE(entities_damaged[1]);
}

TEST_F(ChainSystemTest, ChainUntargetableFocus)
{
    // Get chain sender entities
    const EntityID blue_chain_sender_id = 3;
    const EntityID red_chain_sender_id = 7;
    const auto& blue_chain_sender = world->GetByID(blue_chain_sender_id);
    const auto& red_chain_sender = world->GetByID(red_chain_sender_id);

    // Make entities untargetable
    const auto effect_data = EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, 1000);
    EffectState effect_state_blue{}, effect_state_red{};
    effect_state_blue.sender_stats = world->GetFullStats(blue_chain_sender_id);
    effect_state_red.sender_stats = world->GetFullStats(red_chain_sender_id);
    GetAttachedEffectsHelper()
        .AddAttachedEffect(blue_chain_sender, blue_chain_sender_id, effect_data, effect_state_blue);
    GetAttachedEffectsHelper().AddAttachedEffect(red_chain_sender, red_chain_sender_id, effect_data, effect_state_red);

    TimeStepForNTimeSteps(4);

    // Should be chain number times 2 entities
    ASSERT_EQ(events_chain_created.size(), GetChainNumber() * 2);

    // 1 from the original effect + 6 from 3 chain bounces = 7
    // we have 2 entities so = 14
    ASSERT_EQ(events_apply_effect.size(), 14);

    // 3 apply damage from each chain
    // 3 negative state from each chain
    // And there are two chains in the world
    // Negative state are in the chain propagation because of add_original_effect_package
    ASSERT_EQ(count_apply_damage_from_chains, 6);
    ASSERT_EQ(count_apply_negative_state_from_chains, 6);

    // Should remove the last chains
    TimeStepForNTimeSteps(1);

    // Destroyed as many as there were created
    ASSERT_EQ(events_chain_destroyed.size(), GetChainNumber() * 2);
    ASSERT_EQ(count_chain_destroyed_is_last_chain, 2) << "2 entities should have been activated";

    CalculateEntitiesDamaged();

    // 6 of the 8 entities should have taken damage
    EXPECT_EQ(entities_damaged_count, 6);

    // These specific entities (based on positions specified earlier)
    // Entities 2 and 6 are off to the side, but 3 and 7 are untargetable
    EXPECT_TRUE(entities_damaged[0]);
    EXPECT_TRUE(entities_damaged[1]);
    EXPECT_TRUE(entities_damaged[2]);
    EXPECT_FALSE(entities_damaged[3]);
    EXPECT_TRUE(entities_damaged[4]);
    EXPECT_TRUE(entities_damaged[5]);
    EXPECT_TRUE(entities_damaged[6]);
    EXPECT_FALSE(entities_damaged[7]);
}

TEST_F(ChainSystemOmegaTest, SimpleChainEmpower)
{
    auto& blue4_stats_component = blue4->Get<StatsComponent>();
    auto& blue4_attached_efects_component = blue4->Get<AttachedEffectsComponent>();
    blue4_stats_component.SetCritChanceOverflowPercentage(100_fp);

    auto& blue2_stats_component = blue2->Get<StatsComponent>();
    auto& red1_stats_component = red1->Get<StatsComponent>();

    // Add empower with critical strike enabled for blue 4
    {
        // Add empower
        EffectData effect_data;
        effect_data.type_id.type = EffectType::kEmpower;
        effect_data.lifetime.is_consumable = false;
        effect_data.lifetime.duration_time_ms = kTimeInfinite;
        effect_data.lifetime.activated_by = AbilityType::kOmega;
        effect_data.attached_effect_package_attributes.can_crit = true;

        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(blue1->GetID());

        effect_data.attached_effect_package_attributes.can_crit = true;

        GetAttachedEffectsHelper().AddAttachedEffect(*blue4, blue4->GetID(), effect_data, effect_state);
    }

    EXPECT_EQ(blue2_stats_component.GetCurrentHealth(), 100_fp);
    EXPECT_EQ(red1_stats_component.GetCurrentHealth(), 100_fp);
    ASSERT_EQ(blue4_attached_efects_component.GetEmpowers().size(), 1);
    TimeStepForNTimeSteps(3);

    ASSERT_EQ(blue4_attached_efects_component.GetEmpowers().size(), 1);
    EXPECT_EQ(blue2_stats_component.GetCurrentHealth(), 90_fp);
    EXPECT_EQ(red1_stats_component.GetCurrentHealth(), 85_fp);
    // Should be chain number times 2 entities
    ASSERT_EQ(events_chain_created.size(), 12);

    // Should remove the last chains
    TimeStepForNTimeSteps(1);
    EXPECT_EQ(blue2_stats_component.GetCurrentHealth(), 80_fp);
    EXPECT_EQ(red1_stats_component.GetCurrentHealth(), 70_fp);

    CalculateEntitiesDamaged();
    TimeStepForNTimeSteps(1);
    EXPECT_EQ(blue2_stats_component.GetCurrentHealth(), 70_fp);
    EXPECT_EQ(red1_stats_component.GetCurrentHealth(), 55_fp);

    // 6 of the 8 entities should have taken damage
    EXPECT_EQ(entities_damaged_count, 6);

    // These specific entities (based on positions specified earlier)
    // Entities 2 and 6 are off to the side
    EXPECT_TRUE(entities_damaged[0]);
    EXPECT_TRUE(entities_damaged[1]);
    EXPECT_FALSE(entities_damaged[2]);
    EXPECT_TRUE(entities_damaged[3]);
    EXPECT_TRUE(entities_damaged[4]);
    EXPECT_TRUE(entities_damaged[5]);
    EXPECT_FALSE(entities_damaged[6]);
    EXPECT_TRUE(entities_damaged[7]);
}

class MinimalTest : public BaseTest
{
public:
    virtual CombatUnitData MakeCombatUnitData()
    {
        CombatUnitData unit_data = CreateCombatUnitData();
        unit_data.type_id.line_name = "default";
        unit_data.radius_units = 2;
        unit_data.type_data.combat_affinity = CombatAffinity::kFire;
        unit_data.type_data.combat_class = CombatClass::kBehemoth;
        unit_data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 300_fp);
        unit_data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 5_fp);
        unit_data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);             // No critical attacks
        unit_data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);      // No Dodge
        unit_data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);  // No Miss
        unit_data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);                   // Once every tick
        unit_data.type_data.stats.Set(
            StatType::kAttackRangeUnits,
            1000_fp);                                                      // Very big attack radius so no need to move
        unit_data.type_data.stats.Set(StatType::kOmegaRangeUnits, 50_fp);  // Very big omega radius so no need to move
        unit_data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);
        unit_data.type_data.stats.Set(StatType::kEnergyCost, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP);
        unit_data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);  // don't move
        return unit_data;
    }

    Entity* Spawn(const Team team_name, const CombatUnitData& unit_data, const HexGridPosition position)
    {
        Entity* entity = nullptr;
        SpawnCombatUnit(team_name, position, unit_data, entity);
        return entity;
    }
};

TEST_F(MinimalTest, ComplexChainTest)
{
    CombatUnitData unit_data = MakeCombatUnitData();

    unit_data.type_data.stats.Set(StatType::kMaxHealth, 2000_fp);

    unit_data.type_data.stats.Set(StatType::kStartingEnergy, unit_data.type_data.stats.Get(StatType::kEnergyCost));

    // Add attack abilities
    // Here attack deals 100 pure damage to currently focused entity
    {
        auto& ability = unit_data.type_data.attack_abilities.AddAbility();
        ability.name = "Attack ability";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Attack Skill";
        skill.targeting.type = SkillTargetingType::kCurrentFocus;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = false;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.deployment.guidance = EnumSet<GuidanceType>::MakeFull();
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(100_fp));
        skill.deployment.pre_deployment_delay_percentage = 0;

        ability.total_duration_ms = 1000;

        unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    // Add omega abilities
    // Here omega deals 200 pure damage to currently focused entity
    {
        auto& ability = unit_data.type_data.omega_abilities.AddAbility();
        ability.name = "Omega Ability";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kOmega);
        skill.name = "Omega Skill";
        skill.targeting.type = SkillTargetingType::kCurrentFocus;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.deployment.guidance = EnumSet<GuidanceType>::MakeFull();
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(200_fp));
        skill.deployment.pre_deployment_delay_percentage = 0;

        ability.total_duration_ms = 0;

        unit_data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    // Add innate ability
    {
        auto& ability = unit_data.type_data.innate_abilities.AddAbility();
        ability.name = "Innate Ability";
        ability.activation_trigger_data.trigger_type = ActivationTriggerType::kOnActivateXAbilities;
        ability.activation_trigger_data.ability_types = MakeEnumSet(AbilityType::kOmega);
        ability.activation_trigger_data.every_x = true;
        ability.activation_trigger_data.number_of_abilities_activated = 1;
        ability.activation_trigger_data.sender_allegiance = AllegianceType::kSelf;
        ability.activation_trigger_data.receiver_allegiance = AllegianceType::kAll;
        ability.total_duration_ms = 0;

        // Skills
        auto& skill = ability.AddSkill();
        skill.name = "Debuff attack";
        skill.targeting.type = SkillTargetingType::kSelf;
        skill.deployment.type = SkillDeploymentType::kDirect;

        EffectData& empower_effect = skill.effect_package.effects.emplace_back();
        empower_effect.type_id.type = EffectType::kEmpower;
        empower_effect.lifetime.activated_by = AbilityType::kAttack;
        empower_effect.lifetime.is_consumable = true;
        empower_effect.lifetime.activations_until_expiry = 1;
        empower_effect.attached_effect_package_attributes.can_crit = false;

        auto& propagation = empower_effect.attached_effect_package_attributes.propagation;
        propagation.type = EffectPropagationType::kChain;
        propagation.targeting_group = AllegianceType::kEnemy;
        propagation.chain_delay_ms = 2000;
        propagation.chain_number = 3;
        propagation.prioritize_new_targets = false;
        propagation.only_new_targets = true;
        propagation.ignore_first_propagation_receiver = false;
        propagation.targeting_guidance = MakeEnumSet(GuidanceType::kGround);
        propagation.effect_package = std::make_shared<EffectPackage>();
        propagation.effect_package->attributes.vampiric_percentage = EffectExpression::FromValue(10_fp);

        auto& dot_effect = propagation.effect_package->effects.emplace_back();
        dot_effect = EffectData::CreateDamageOverTime(
            EffectDamageType::kEnergy,
            EffectExpression::FromValue(4200_fp),
            1000,
            100);

        unit_data.type_data.innate_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    Entity* blue_entity = Spawn(Team::kBlue, unit_data, {0, -3});
    ASSERT_NE(blue_entity, nullptr);
    const EntityID blue_entity_id = blue_entity->GetID();
    blue_entity->Get<StatsComponent>().SetCurrentHealth(1000_fp);

    Entity* red_entity = Spawn(Team::kRed, unit_data, {-18, 37});
    ASSERT_NE(red_entity, nullptr);
    Stun(*red_entity);
    const EntityID red_entity_id = red_entity->GetID();

    EventHistory<EventType::kAbilityActivated> abilities_activated(*world);
    EventHistory<EventType::kEffectPackageReceived> effect_packages_received(*world);
    EventHistory<EventType::kChainCreated> chains_created(*world);
    EventHistory<EventType::kOnEffectApplied> effects_applied(*world);
    EventHistory<EventType::kOnHealthGain> health_gained(*world);

    auto clear_events_and_time_step = [&]()
    {
        abilities_activated.Clear();
        effect_packages_received.Clear();
        effects_applied.Clear();
        chains_created.Clear();
        health_gained.Clear();
        world->TimeStep();
    };

    clear_events_and_time_step();

    /////////////////////////////////////////////// Time step 0

    // Omega activates and triggers innate
    ASSERT_EQ(abilities_activated.Size(), 2);
    ASSERT_EQ(abilities_activated[0].predicted_receiver_ids.size(), 1);
    ASSERT_EQ(abilities_activated[0].predicted_receiver_ids[0], blue_entity_id);
    ASSERT_EQ(abilities_activated[0].ability_type, AbilityType::kInnate);
    ASSERT_EQ(abilities_activated[1].predicted_receiver_ids.size(), 1);
    ASSERT_EQ(abilities_activated[1].predicted_receiver_ids[0], red_entity_id);
    ASSERT_EQ(abilities_activated[1].ability_type, AbilityType::kOmega);

    // Effect from omega and innate
    ASSERT_EQ(effect_packages_received.Size(), 2);
    ASSERT_EQ(effect_packages_received[0].ability_type, AbilityType::kInnate);
    ASSERT_EQ(effect_packages_received[0].sender_id, blue_entity_id);
    ASSERT_EQ(effect_packages_received[0].receiver_id, blue_entity_id);
    ASSERT_EQ(effect_packages_received[1].ability_type, AbilityType::kOmega);
    ASSERT_EQ(effect_packages_received[1].sender_id, blue_entity_id);
    ASSERT_EQ(effect_packages_received[1].receiver_id, red_entity_id);

    // Empower and damage from omega
    ASSERT_EQ(effects_applied.Size(), 2);
    EXPECT_EQ(effects_applied[0].data.type_id.type, EffectType::kEmpower);
    EXPECT_EQ(effects_applied[1].data.type_id.type, EffectType::kInstantDamage);

    ASSERT_EQ(chains_created.Size(), 0);
    ASSERT_EQ(health_gained.Size(), 0);

    clear_events_and_time_step();

    /////////////////////////////////////////////// Time step 1

    // after the next attack empower skill should spawn a zone
    ASSERT_EQ(abilities_activated.Size(), 1);
    EXPECT_EQ(abilities_activated[0].ability_type, AbilityType::kAttack);
    EXPECT_EQ(abilities_activated[0].sender_id, blue_entity_id);
    ASSERT_EQ(abilities_activated[0].predicted_receiver_ids.size(), 1);
    EXPECT_EQ(abilities_activated[0].predicted_receiver_ids[0], red_entity_id);

    ASSERT_EQ(effects_applied.Size(), 1);
    EXPECT_EQ(effects_applied[0].data.type_id.type, EffectType::kInstantDamage);

    ASSERT_EQ(chains_created.Size(), 1);
    EXPECT_EQ(chains_created[0].combat_unit_sender_id, blue_entity_id);
    EXPECT_EQ(chains_created[0].receiver_id, red_entity_id);
    const EntityID chain_entity_id = chains_created[0].entity_id;

    ASSERT_EQ(health_gained.Size(), 0);

    Stun(*blue_entity);

    clear_events_and_time_step();

    /////////////////////////////////////////////// Time step 2

    // Chain entity activates ability
    ASSERT_EQ(abilities_activated.Size(), 1);
    EXPECT_EQ(abilities_activated[0].ability_type, AbilityType::kAttack);
    EXPECT_EQ(abilities_activated[0].sender_id, chain_entity_id);

    // chain entity attack applies effect package
    ASSERT_EQ(effect_packages_received.Size(), 1);
    ASSERT_EQ(effect_packages_received[0].ability_type, AbilityType::kAttack);
    ASSERT_EQ(effect_packages_received[0].sender_id, chain_entity_id);

    // effect package brings effect
    ASSERT_EQ(effects_applied.Size(), 1);
    ASSERT_EQ(effects_applied[0].sender_id, chain_entity_id);
    ASSERT_EQ(effects_applied[0].receiver_id, red_entity_id);
    ASSERT_EQ(effects_applied[0].data.type_id.type, EffectType::kDamageOverTime);
    ASSERT_EQ(effects_applied[0].data.type_id.damage_type, EffectDamageType::kEnergy);

    ASSERT_EQ(chains_created.Size(), 0);
    ASSERT_EQ(health_gained.Size(), 0);

    clear_events_and_time_step();

    /////////////////////////////////////////////// Time step 3
    ASSERT_EQ(effects_applied.Size(), 1);

    EXPECT_EQ(effects_applied[0].receiver_id, red_entity_id);
    EXPECT_EQ(effects_applied[0].sender_id, blue_entity_id);
    EXPECT_EQ(effects_applied[0].data.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(effects_applied[0].data.type_id.damage_type, EffectDamageType::kEnergy);

    EXPECT_EQ(EvaluateNoStats(effects_applied[0].data.GetExpression()), 420_fp);

    ASSERT_EQ(abilities_activated.Size(), 0);
    ASSERT_EQ(effect_packages_received.Size(), 0);
    ASSERT_EQ(chains_created.Size(), 0);

    ASSERT_EQ(health_gained.Size(), 1);
    EXPECT_EQ(health_gained[0].gain, 42_fp);
}

TEST_F(MinimalTest, ChainTestIgnoreFirst)
{
    CombatUnitData unit_data = MakeCombatUnitData();

    unit_data.type_data.stats.Set(StatType::kMaxHealth, 2000_fp);

    // Add attack abilities
    {
        auto& ability = unit_data.type_data.attack_abilities.AddAbility();
        ability.name = "Attack ability";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Attack Skill";
        skill.targeting.type = SkillTargetingType::kCurrentFocus;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = false;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.deployment.pre_deployment_delay_percentage = 0;

        ability.total_duration_ms = 100;

        auto& propagation = skill.effect_package.attributes.propagation;
        propagation.type = EffectPropagationType::kChain;
        propagation.targeting_group = AllegianceType::kEnemy;
        propagation.chain_delay_ms = 100;
        propagation.chain_number = 3;
        propagation.prioritize_new_targets = false;
        propagation.only_new_targets = false;
        propagation.ignore_first_propagation_receiver = true;
        propagation.effect_package = std::make_shared<EffectPackage>();
        propagation.effect_package->AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(42_fp));

        unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    // Blue entity attacks and makes chain
    Entity* blue_entity = Spawn(Team::kBlue, unit_data, {0, -3});
    ASSERT_NE(blue_entity, nullptr);
    const EntityID blue_entity_id = blue_entity->GetID();
    blue_entity->Get<StatsComponent>().SetCurrentHealth(1000_fp);

    // Red entities does nothing (stun)
    constexpr std::array<HexGridPosition, 2> red_team_positions = {{{-4, 5}, {-7, 11}}};

    std::vector<Entity*> red_entities;
    for (const HexGridPosition red_pos : red_team_positions)
    {
        red_entities.emplace_back(Spawn(Team::kRed, unit_data, red_pos));
        ASSERT_NE(red_entities.back(), nullptr);
        Stun(*red_entities.back());
    }

    EventHistory<EventType::kAbilityActivated> abilities_activated(*world);
    EventHistory<EventType::kEffectPackageReceived> effect_packages_received(*world);
    EventHistory<EventType::kChainCreated> chains_created(*world);
    EventHistory<EventType::kOnEffectApplied> effects_applied(*world);
    EventHistory<EventType::kOnDamage> damage_received(*world);

    auto clear_events_and_time_step = [&]()
    {
        abilities_activated.Clear();
        effect_packages_received.Clear();
        effects_applied.Clear();
        chains_created.Clear();
        damage_received.Clear();
        world->TimeStep();
    };

    /////////////////////////////////////////////// Time step 0
    clear_events_and_time_step();

    // Ability applied to the first spaned entity
    ASSERT_EQ(abilities_activated.Size(), 1);
    ASSERT_EQ(abilities_activated[0].sender_id, blue_entity_id);
    ASSERT_EQ(abilities_activated[0].ability_type, AbilityType::kAttack);
    ASSERT_EQ(abilities_activated[0].predicted_receiver_ids.size(), 1);
    ASSERT_EQ(abilities_activated[0].predicted_receiver_ids[0], red_entities.front()->GetID());
    Stun(*blue_entity);  // Do not need more attacks

    ASSERT_EQ(damage_received.Size(), 0);

    // But first chain is spawned on the next target
    ASSERT_EQ(chains_created.Size(), 1);
    ASSERT_EQ(chains_created[0].sender_id, blue_entity_id);
    ASSERT_EQ(chains_created[0].receiver_id, red_entities.back()->GetID());
    std::vector<EntityID> chains;
    chains.emplace_back(chains_created[0].entity_id);
}

}  // namespace simulation
