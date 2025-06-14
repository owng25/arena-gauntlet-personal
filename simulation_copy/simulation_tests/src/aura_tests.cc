#include "base_json_test.h"
#include "components/aura_component.h"
#include "components/position_component.h"
#include "utility/entity_helper.h"

namespace simulation
{

class AuraTest : public BaseJSONTest
{
public:
    static FullCombatUnitData MakeFullData()
    {
        FullCombatUnitData full_data{};
        full_data.data.type_id.line_name = "default";
        full_data.data.radius_units = 1;
        full_data.data.type_data.combat_affinity = CombatAffinity::kFire;
        full_data.data.type_data.combat_class = CombatClass::kBehemoth;

        // Stats
        {
            auto& stats = full_data.data.type_data.stats;
            stats.Set(StatType::kEnergyResist, 0_fp);
            stats.Set(StatType::kPhysicalResist, 0_fp);
            stats.Set(StatType::kOmegaPowerPercentage, 300_fp);
            stats.Set(StatType::kCritAmplificationPercentage, 5_fp);
            stats.Set(StatType::kCritChancePercentage, 0_fp);
            stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);
            stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
            stats.Set(StatType::kAttackSpeed, 100_fp);
            stats.Set(StatType::kResolve, 0_fp);
            stats.Set(StatType::kGrit, 0_fp);
            stats.Set(StatType::kWillpowerPercentage, 0_fp);
            stats.Set(StatType::kTenacityPercentage, 0_fp);
            stats.Set(StatType::kAttackRangeUnits, 15_fp);
            stats.Set(StatType::kOmegaRangeUnits, 25_fp);
            stats.Set(StatType::kStartingEnergy, 0_fp);
            stats.Set(StatType::kEnergyCost, 1000_fp);
            stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP);
            stats.Set(StatType::kMaxHealth, 1000_fp);
            stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
        }

        return full_data;
    }

    static constexpr std::string_view SimpleAttackAbility()
    {
        return R"({
            "Name": "Attack",
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": { "Type": "CurrentFocus" },
                    "Deployment": { "Type": "Direct" },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 10
                            }
                        ]
                    }
                }
            ]
        })";
    }
};

TEST_F(AuraTest, ParseFromJSON)
{
    const auto maybe_data = loader->ParseAndLoadEffect(R"({
        "Type": "Aura",
        "AttachedEffects": [
            {
                "Type": "Buff",
                "DurationMs": -1,
                "Stat": "Grit",
                "Expression": 10,
                "RadiusUnits": 20
            }
        ]
    })");

    ASSERT_TRUE(maybe_data.has_value());
    const auto& data = maybe_data.value();
    ASSERT_EQ(data.type_id.type, EffectType::kAura);
    ASSERT_EQ(data.radius_units, 0);
    ASSERT_EQ(data.lifetime.duration_time_ms, kTimeInfinite);
    ASSERT_EQ(data.attached_effects.size(), 1);
    ASSERT_EQ(data.attached_effects.front().type_id.type, EffectType::kBuff);
    ASSERT_EQ(data.attached_effects.front().type_id.stat_type, StatType::kGrit);
    ASSERT_EQ(data.attached_effects.front().lifetime.duration_time_ms, kTimeInfinite);
    ASSERT_EQ(data.attached_effects.front().GetExpression().base_value.value, 10_fp);
    ASSERT_EQ(data.attached_effects.front().radius_units, 20);
}

TEST_F(AuraTest, AuraOnSelfWithBuffForAllies)
{
    // VISUALIZE_TEST_STEPS;

    constexpr HexGridPosition initial_red_position{0, -5};
    std::vector<EntityID> red_entities;
    for (int i = 0; i != 10; ++i)
    {
        Entity* entity = nullptr;
        auto full_data = MakeFullData();
        full_data.instance.team = Team::kRed;
        full_data.instance.id = GenerateRandomUniqueID();
        full_data.instance.position = initial_red_position + HexGridPosition{3, -2} * (i + 1);
        SpawnCombatUnit(full_data, entity);
        ASSERT_NE(entity, nullptr);

        red_entities.push_back(entity->GetID());
    }

    // Add ability with aura for the first blue entity
    auto add_innate = [this](FullCombatUnitData& full_data)
    {
        auto& innate = full_data.data.type_data.innate_abilities.AddAbility();
        constexpr std::string_view ability_json = R"({
            "Name": "Aura with grit for each ally in 15 hexes",
            "ActivationTrigger": { "TriggerType": "OnBattleStart" },
            "TotalDurationMs": 0,
            "Skills": [
                {
                    "Name": "Grit aura",
                    "Deployment": { "Type": "Direct" },
                    "Targeting": { "Type": "Self" },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "Buff",
                                "Stat": "Resolve",
                                "DurationMs": 1500,
                                "Expression": 24
                            },
                            {
                                "Type": "Aura",
                                "AttachedEffects": [
                                    {
                                        "Type": "Buff",
                                        "Stat": "Grit",
                                        "OverlapProcessType": "Sum",
                                        "DurationMs": 1500,
                                        "Expression": 42,
                                        "RadiusUnits": 15
                                    }
                                ]
                            }
                        ]
                    }
                }
            ]
        })";
        ASSERT_TRUE(loader->ParseAndLoadAbility(ability_json, AbilityType::kInnate, &innate));
    };

    auto add_attack = [this](FullCombatUnitData& full_data)
    {
        ASSERT_TRUE(loader->ParseAndLoadAbility(
            SimpleAttackAbility(),
            AbilityType::kAttack,
            &full_data.data.type_data.innate_abilities.AddAbility()));
    };

    std::vector<EntityID> blue_entities;

    for (size_t i = 0; i != 10; ++i)
    {
        auto full_data = MakeFullData();
        if (i == 5)
        {
            add_innate(full_data);
            add_attack(full_data);
            full_data.data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 1000_fp);
            full_data.data.type_data.stats.Set(StatType::kAttackRangeUnits, 3_fp);
        }

        Entity* entity = nullptr;
        full_data.instance.team = Team::kBlue;
        full_data.instance.id = GenerateRandomUniqueID();
        full_data.instance.position = Reflect(world->GetByID(red_entities[i]).Get<PositionComponent>().GetPosition());
        SpawnCombatUnit(full_data, entity);
        ASSERT_NE(entity, nullptr);

        blue_entities.push_back(entity->GetID());
    }

    // Auras will be applied on the next aura system tick
    for (EntityID entity_id : blue_entities)
    {
        auto stats = world->GetLiveStats(entity_id);
        ASSERT_EQ(stats.Get(StatType::kGrit), 0_fp);
    }

    EventHistory<EventType::kAuraCreated> aura_created(*world);
    EventHistory<EventType::kAuraDestroyed> aura_destroyed(*world);
    EntityID blue_entity_with_aura_id = kInvalidEntityID;

    for (size_t i = 0; i != 30; ++i)
    {
        world->TimeStep();
        ASSERT_EQ(aura_created.Size(), 1);

        if (i == 0)
        {
            const EntityID aura_id = aura_created.events.front().entity_id;
            ASSERT_TRUE(EntityHelper::IsAnAura(*world, aura_id));
            const auto& aura = world->GetByID(aura_id);
            const auto& aura_data = aura.Get<AuraComponent>().GetComponentData();
            blue_entity_with_aura_id = aura_data.receiver_id;
            ASSERT_EQ(blue_entity_with_aura_id, blue_entities[5]);
            ASSERT_TRUE(world->HasEntity(blue_entity_with_aura_id));
        }

        const Entity& blue_entity_with_aura = world->GetByID(blue_entity_with_aura_id);
        const FixedPoint aura_holder_resolve = world->GetLiveStats(blue_entity_with_aura).Get(StatType::kResolve);

        if (!aura_destroyed.IsEmpty())
        {
            ASSERT_EQ(aura_destroyed.Size(), 1);
            ASSERT_EQ(aura_holder_resolve, 0_fp);
            // The buff should not exist anymore
            for (EntityID entity_id : blue_entities)
            {
                const auto& entity = world->GetByID(entity_id);
                ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), 0_fp);
            }
        }
        else
        {
            ASSERT_EQ(aura_destroyed.Size(), 0);
            ASSERT_EQ(aura_holder_resolve, 24_fp);

            const HexGridPosition aura_position = blue_entity_with_aura.Get<PositionComponent>().GetPosition();

            for (EntityID entity_id : blue_entities)
            {
                const auto& entity = world->GetByID(entity_id);
                const HexGridPosition entity_position = entity.Get<PositionComponent>().GetPosition();
                const auto distance = (aura_position - entity_position).Length();
                const bool should_have_buff = distance <= 15;
                ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), should_have_buff ? 42_fp : 0_fp)
                    << fmt::format(
                           "i = {}, entity_id = {}. Aura position: {}, entity_position: {}. Distance: {}",
                           i,
                           entity_id,
                           aura_position,
                           entity_position,
                           distance);
            }
        }

        // Red entities should not get any grit
        for (EntityID entity_id : red_entities)
        {
            const auto& entity = world->GetByID(entity_id);
            ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), 0_fp);
        }
    }
}

TEST_F(AuraTest, AuraOnAllyWithBuffForAllies)
{
    // VISUALIZE_TEST_STEPS;

    constexpr HexGridPosition initial_red_position{0, -5};
    std::vector<EntityID> red_entities;
    for (int i = 0; i != 10; ++i)
    {
        Entity* entity = nullptr;
        auto full_data = MakeFullData();
        full_data.instance.team = Team::kRed;
        full_data.instance.id = GenerateRandomUniqueID();
        full_data.instance.position = initial_red_position + HexGridPosition{3, -2} * (i + 1);
        SpawnCombatUnit(full_data, entity);
        ASSERT_NE(entity, nullptr);

        red_entities.push_back(entity->GetID());
    }

    // Add ability with aura for the first blue entity
    auto add_innate = [this](FullCombatUnitData& full_data)
    {
        auto& innate = full_data.data.type_data.innate_abilities.AddAbility();
        constexpr std::string_view ability_json = R"({
            "Name": "Aura with grit for each ally in 15 hexes",
            "ActivationTrigger": { "TriggerType": "OnBattleStart" },
            "TotalDurationMs": 0,
            "Skills": [
                {
                    "Name": "Grit aura",
                    "Deployment": { "Type": "Direct" },
                    "Targeting": {
                        "Type": "DistanceCheck",
                        "Group": "Ally",
                        "Lowest": true,
                        "Num": 1
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "Buff",
                                "Stat": "Resolve",
                                "DurationMs": 1500,
                                "Expression": 24
                            },
                            {
                                "Type": "Aura",
                                "AttachedEffects": [
                                    {
                                        "Type": "Buff",
                                        "Stat": "Grit",
                                        "DurationMs": 1500,
                                        "Expression": 42,
                                        "RadiusUnits": 15
                                    }
                                ]
                            }
                        ]
                    }
                }
            ]
        })";
        ASSERT_TRUE(loader->ParseAndLoadAbility(ability_json, AbilityType::kInnate, &innate));
    };

    auto add_attack = [this](FullCombatUnitData& full_data)
    {
        ASSERT_TRUE(loader->ParseAndLoadAbility(
            SimpleAttackAbility(),
            AbilityType::kAttack,
            &full_data.data.type_data.innate_abilities.AddAbility()));
    };

    std::vector<EntityID> blue_entities;

    for (size_t i = 0; i != 10; ++i)
    {
        auto full_data = MakeFullData();
        if (i == 5)
        {
            add_innate(full_data);
            add_attack(full_data);
            full_data.data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 1000_fp);
            full_data.data.type_data.stats.Set(StatType::kAttackRangeUnits, 3_fp);
        }

        Entity* entity = nullptr;
        full_data.instance.team = Team::kBlue;
        full_data.instance.id = GenerateRandomUniqueID();
        full_data.instance.position = Reflect(world->GetByID(red_entities[i]).Get<PositionComponent>().GetPosition());
        SpawnCombatUnit(full_data, entity);
        ASSERT_NE(entity, nullptr);

        blue_entities.push_back(entity->GetID());
    }

    // Auras will be applied on the next aura system tick
    for (EntityID entity_id : blue_entities)
    {
        auto stats = world->GetLiveStats(entity_id);
        ASSERT_EQ(stats.Get(StatType::kGrit), 0_fp);
    }

    EventHistory<EventType::kAuraCreated> aura_created(*world);
    EventHistory<EventType::kAuraDestroyed> aura_destroyed(*world);
    EntityID blue_entity_with_aura_id = kInvalidEntityID;

    for (size_t i = 0; i != 30; ++i)
    {
        world->TimeStep();
        ASSERT_EQ(aura_created.Size(), 1);

        if (i == 0)
        {
            const EntityID aura_id = aura_created.events.front().entity_id;
            ASSERT_TRUE(EntityHelper::IsAnAura(*world, aura_id));
            const auto& aura = world->GetByID(aura_id);
            const auto& aura_data = aura.Get<AuraComponent>().GetComponentData();
            blue_entity_with_aura_id = aura_data.receiver_id;
            ASSERT_TRUE(world->HasEntity(blue_entity_with_aura_id));
        }

        const Entity& blue_entity_with_aura = world->GetByID(blue_entity_with_aura_id);
        const FixedPoint aura_holder_resolve = world->GetLiveStats(blue_entity_with_aura).Get(StatType::kResolve);

        if (!aura_destroyed.IsEmpty())
        {
            ASSERT_EQ(aura_destroyed.Size(), 1);
            ASSERT_EQ(aura_holder_resolve, 0_fp);
            // The buff should not exist anymore
            for (EntityID entity_id : blue_entities)
            {
                const auto& entity = world->GetByID(entity_id);
                ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), 0_fp);
            }
        }
        else
        {
            ASSERT_EQ(aura_destroyed.Size(), 0);
            ASSERT_EQ(aura_holder_resolve, 24_fp);

            const HexGridPosition aura_position = blue_entity_with_aura.Get<PositionComponent>().GetPosition();

            for (EntityID entity_id : blue_entities)
            {
                const auto& entity = world->GetByID(entity_id);
                const HexGridPosition entity_position = entity.Get<PositionComponent>().GetPosition();
                const auto distance = (aura_position - entity_position).Length();
                const bool should_have_buff = distance <= 15;
                ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), should_have_buff ? 42_fp : 0_fp)
                    << fmt::format(
                           "i = {}, entity_id = {}. Aura position: {}, entity_position: {}. Distance: {}",
                           i,
                           entity_id,
                           aura_position,
                           entity_position,
                           distance);
            }
        }

        // Red entities should not get any grit
        for (EntityID entity_id : red_entities)
        {
            const auto& entity = world->GetByID(entity_id);
            ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), 0_fp);
        }
    }
}

TEST_F(AuraTest, AuraOnEnemyWithBuffForAllies)
{
    // VISUALIZE_TEST_STEPS;

    constexpr HexGridPosition initial_red_position{0, -5};
    std::vector<EntityID> red_entities;
    for (int i = 0; i != 10; ++i)
    {
        Entity* entity = nullptr;
        auto full_data = MakeFullData();
        full_data.instance.team = Team::kRed;
        full_data.instance.id = GenerateRandomUniqueID();
        full_data.instance.position = initial_red_position + HexGridPosition{3, -2} * (i + 1);
        full_data.data.type_data.stats.Set(StatType::kResolve, 48_fp);
        SpawnCombatUnit(full_data, entity);
        ASSERT_NE(entity, nullptr);

        red_entities.push_back(entity->GetID());
    }

    // Add ability with aura for the first blue entity
    auto add_innate = [this](FullCombatUnitData& full_data)
    {
        auto& innate = full_data.data.type_data.innate_abilities.AddAbility();
        constexpr std::string_view ability_json = R"({
            "Name": "Aura with grit for each ally in 15 hexes",
            "ActivationTrigger": { "TriggerType": "OnBattleStart" },
            "TotalDurationMs": 0,
            "Skills": [
                {
                    "Name": "Grit aura",
                    "Deployment": { "Type": "Direct" },
                    "Targeting": {
                        "Type": "DistanceCheck",
                        "Group": "Enemy",
                        "Lowest": true,
                        "Num": 1
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "Debuff",
                                "Stat": "Resolve",
                                "DurationMs": 1500,
                                "Expression": 24
                            },
                            {
                                "Type": "Aura",
                                "AttachedEffects": [
                                    {
                                        "Type": "Buff",
                                        "Stat": "Grit",
                                        "DurationMs": 1500,
                                        "Expression": 42,
                                        "RadiusUnits": 15
                                    }
                                ]
                            }
                        ]
                    }
                }
            ]
        })";
        ASSERT_TRUE(loader->ParseAndLoadAbility(ability_json, AbilityType::kInnate, &innate));
    };

    auto add_attack = [this](FullCombatUnitData& full_data)
    {
        ASSERT_TRUE(loader->ParseAndLoadAbility(
            SimpleAttackAbility(),
            AbilityType::kAttack,
            &full_data.data.type_data.innate_abilities.AddAbility()));
    };

    std::vector<EntityID> blue_entities;

    for (size_t i = 0; i != 10; ++i)
    {
        auto full_data = MakeFullData();
        if (i == 5)
        {
            add_innate(full_data);
            add_attack(full_data);
            full_data.data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 1000_fp);
            full_data.data.type_data.stats.Set(StatType::kAttackRangeUnits, 3_fp);
        }

        Entity* entity = nullptr;
        full_data.instance.team = Team::kBlue;
        full_data.instance.id = GenerateRandomUniqueID();
        full_data.instance.position = Reflect(world->GetByID(red_entities[i]).Get<PositionComponent>().GetPosition());
        SpawnCombatUnit(full_data, entity);
        ASSERT_NE(entity, nullptr);

        blue_entities.push_back(entity->GetID());
    }

    // Auras will be applied on the next aura system tick
    for (EntityID entity_id : blue_entities)
    {
        auto stats = world->GetLiveStats(entity_id);
        ASSERT_EQ(stats.Get(StatType::kGrit), 0_fp);
    }

    EventHistory<EventType::kAuraCreated> aura_created(*world);
    EventHistory<EventType::kAuraDestroyed> aura_destroyed(*world);
    EntityID red_entity_with_aura_id = kInvalidEntityID;

    for (size_t i = 0; i != 30; ++i)
    {
        world->TimeStep();
        ASSERT_EQ(aura_created.Size(), 1);

        if (i == 0)
        {
            const EntityID aura_id = aura_created.events.front().entity_id;
            ASSERT_TRUE(EntityHelper::IsAnAura(*world, aura_id));
            const auto& aura = world->GetByID(aura_id);
            const auto& aura_data = aura.Get<AuraComponent>().GetComponentData();
            red_entity_with_aura_id = aura_data.receiver_id;
            ASSERT_TRUE(world->HasEntity(red_entity_with_aura_id));
        }

        const Entity& red_entity_with_aura = world->GetByID(red_entity_with_aura_id);
        ASSERT_EQ(red_entity_with_aura.GetTeam(), Team::kRed);
        const FixedPoint aura_holder_resolve = world->GetLiveStats(red_entity_with_aura).Get(StatType::kResolve);

        if (!aura_destroyed.IsEmpty())
        {
            ASSERT_EQ(aura_destroyed.Size(), 1);
            ASSERT_EQ(aura_holder_resolve, 48_fp);
            // The buff should not exist anymore
            for (EntityID entity_id : blue_entities)
            {
                const auto& entity = world->GetByID(entity_id);
                ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), 0_fp);
            }
        }
        else
        {
            ASSERT_EQ(aura_destroyed.Size(), 0);
            ASSERT_EQ(aura_holder_resolve, 24_fp);

            const HexGridPosition aura_position = red_entity_with_aura.Get<PositionComponent>().GetPosition();

            for (EntityID entity_id : blue_entities)
            {
                const auto& entity = world->GetByID(entity_id);
                const HexGridPosition entity_position = entity.Get<PositionComponent>().GetPosition();
                const auto distance = (aura_position - entity_position).Length();
                const bool should_have_buff = distance <= 15;
                ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), should_have_buff ? 42_fp : 0_fp)
                    << fmt::format(
                           "i = {}, entity_id = {}. Aura position: {}, entity_position: {}. Distance: {}",
                           i,
                           entity_id,
                           aura_position,
                           entity_position,
                           distance);
            }
        }

        // Red entities should not get any grit
        for (EntityID entity_id : red_entities)
        {
            const auto& entity = world->GetByID(entity_id);
            ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), 0_fp);
        }
    }
}

TEST_F(AuraTest, AuraOnSelfWithDebuffForEnemies)
{
    // VISUALIZE_TEST_STEPS;

    constexpr HexGridPosition initial_red_position{0, -5};
    std::vector<EntityID> red_entities;
    for (int i = 0; i != 10; ++i)
    {
        Entity* entity = nullptr;
        auto full_data = MakeFullData();
        full_data.instance.team = Team::kRed;
        full_data.instance.id = GenerateRandomUniqueID();
        full_data.instance.position = initial_red_position + HexGridPosition{3, -2} * (i + 1);
        full_data.data.type_data.stats.Set(StatType::kGrit, 73_fp);
        SpawnCombatUnit(full_data, entity);
        ASSERT_NE(entity, nullptr);

        red_entities.push_back(entity->GetID());
    }

    auto add_innate = [this](FullCombatUnitData& full_data)
    {
        auto& innate = full_data.data.type_data.innate_abilities.AddAbility();
        constexpr std::string_view ability_json = R"({
            "Name": "Aura with grit debuff for each enemy in 15 hexes",
            "ActivationTrigger": { "TriggerType": "OnBattleStart" },
            "TotalDurationMs": 0,
            "Skills": [
                {
                    "Name": "Grit aura",
                    "Deployment": { "Type": "Direct" },
                    "Targeting": { "Type": "Self" },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "Buff",
                                "Stat": "Resolve",
                                "DurationMs": 1500,
                                "Expression": 24
                            },
                            {
                                "Type": "Aura",
                                "AttachedEffects": [
                                    {
                                        "Type": "Debuff",
                                        "Stat": "Grit",
                                        "DurationMs": 1500,
                                        "Expression": 42,
                                        "RadiusUnits": 15
                                    }
                                ]
                            }
                        ]
                    }
                }
            ]
        })";
        ASSERT_TRUE(loader->ParseAndLoadAbility(ability_json, AbilityType::kInnate, &innate));
    };

    auto add_attack = [this](FullCombatUnitData& full_data)
    {
        ASSERT_TRUE(loader->ParseAndLoadAbility(
            SimpleAttackAbility(),
            AbilityType::kAttack,
            &full_data.data.type_data.innate_abilities.AddAbility()));
    };

    std::vector<EntityID> blue_entities;

    for (size_t i = 0; i != 10; ++i)
    {
        auto full_data = MakeFullData();
        full_data.data.type_data.stats.Set(StatType::kGrit, 37_fp);
        if (i == 5)
        {
            add_innate(full_data);
            add_attack(full_data);
            full_data.data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 1000_fp);
            full_data.data.type_data.stats.Set(StatType::kAttackRangeUnits, 3_fp);
        }

        Entity* entity = nullptr;
        full_data.instance.team = Team::kBlue;
        full_data.instance.id = GenerateRandomUniqueID();
        full_data.instance.position = Reflect(world->GetByID(red_entities[i]).Get<PositionComponent>().GetPosition());
        SpawnCombatUnit(full_data, entity);
        ASSERT_NE(entity, nullptr);

        blue_entities.push_back(entity->GetID());
    }

    // Auras will be applied on the next aura system tick
    for (EntityID entity_id : red_entities)
    {
        auto stats = world->GetLiveStats(entity_id);
        ASSERT_EQ(stats.Get(StatType::kGrit), 73_fp);
    }

    EventHistory<EventType::kAuraCreated> aura_created(*world);
    EventHistory<EventType::kAuraDestroyed> aura_destroyed(*world);
    EntityID blue_entity_with_aura_id = kInvalidEntityID;

    for (size_t i = 0; i != 30; ++i)
    {
        world->TimeStep();
        ASSERT_EQ(aura_created.Size(), 1);

        if (i == 0)
        {
            const EntityID aura_id = aura_created.events.front().entity_id;
            ASSERT_TRUE(EntityHelper::IsAnAura(*world, aura_id));
            const auto& aura = world->GetByID(aura_id);
            const auto& aura_data = aura.Get<AuraComponent>().GetComponentData();
            blue_entity_with_aura_id = aura_data.receiver_id;
            ASSERT_EQ(blue_entity_with_aura_id, blue_entities[5]);
            ASSERT_TRUE(world->HasEntity(blue_entity_with_aura_id));
        }

        const Entity& blue_entity_with_aura = world->GetByID(blue_entity_with_aura_id);
        const FixedPoint aura_holder_resolve = world->GetLiveStats(blue_entity_with_aura).Get(StatType::kResolve);

        if (!aura_destroyed.IsEmpty())
        {
            ASSERT_EQ(aura_destroyed.Size(), 1);
            ASSERT_EQ(aura_holder_resolve, 0_fp);
            // The debuff should not exist anymore
            for (EntityID entity_id : red_entities)
            {
                const auto& entity = world->GetByID(entity_id);
                ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), 73_fp);
            }
        }
        else
        {
            ASSERT_EQ(aura_destroyed.Size(), 0);
            ASSERT_EQ(aura_holder_resolve, 24_fp);

            const HexGridPosition aura_position = blue_entity_with_aura.Get<PositionComponent>().GetPosition();

            for (EntityID entity_id : red_entities)
            {
                const auto& entity = world->GetByID(entity_id);
                const HexGridPosition entity_position = entity.Get<PositionComponent>().GetPosition();
                const auto distance = (aura_position - entity_position).Length();
                const bool in_range = distance <= 15;
                ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), in_range ? 31_fp : 73_fp) << fmt::format(
                    "i = {}, entity_id = {}. Aura position: {}, entity_position: {}. Distance: {}",
                    i,
                    entity_id,
                    aura_position,
                    entity_position,
                    distance);
            }
        }

        // Blue entities should not lose any grit
        for (EntityID entity_id : blue_entities)
        {
            const auto& entity = world->GetByID(entity_id);
            ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), 37_fp);
        }
    }
}

TEST_F(AuraTest, AuraOnAllyWithDebuffForEnemies)
{
    constexpr HexGridPosition initial_red_position{0, -5};
    std::vector<EntityID> red_entities;
    for (int i = 0; i != 10; ++i)
    {
        Entity* entity = nullptr;
        auto full_data = MakeFullData();
        full_data.instance.team = Team::kRed;
        full_data.instance.id = GenerateRandomUniqueID();
        full_data.instance.position = initial_red_position + HexGridPosition{3, -2} * (i + 1);
        full_data.data.type_data.stats.Set(StatType::kGrit, 73_fp);
        SpawnCombatUnit(full_data, entity);
        ASSERT_NE(entity, nullptr);

        red_entities.push_back(entity->GetID());
    }

    auto add_innate = [this](FullCombatUnitData& full_data)
    {
        auto& innate = full_data.data.type_data.innate_abilities.AddAbility();
        constexpr std::string_view ability_json = R"({
            "Name": "Aura with grit debuff for each enemy in 15 hexes",
            "ActivationTrigger": { "TriggerType": "OnBattleStart" },
            "TotalDurationMs": 0,
            "Skills": [
                {
                    "Name": "Grit aura",
                    "Deployment": { "Type": "Direct" },
                    "Targeting": {
                        "Type": "DistanceCheck",
                        "Group": "Ally",
                        "Lowest": true,
                        "Num": 1
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "Buff",
                                "Stat": "Resolve",
                                "DurationMs": 1500,
                                "Expression": 24
                            },
                            {
                                "Type": "Aura",
                                "AttachedEffects": [
                                    {
                                        "Type": "Debuff",
                                        "Stat": "Grit",
                                        "DurationMs": 1500,
                                        "Expression": 42,
                                        "RadiusUnits": 15
                                    }
                                ]
                            }
                        ]
                    }
                }
            ]
        })";
        ASSERT_TRUE(loader->ParseAndLoadAbility(ability_json, AbilityType::kInnate, &innate));
    };

    auto add_attack = [this](FullCombatUnitData& full_data)
    {
        ASSERT_TRUE(loader->ParseAndLoadAbility(
            SimpleAttackAbility(),
            AbilityType::kAttack,
            &full_data.data.type_data.innate_abilities.AddAbility()));
    };

    std::vector<EntityID> blue_entities;

    for (size_t i = 0; i != 10; ++i)
    {
        auto full_data = MakeFullData();
        full_data.data.type_data.stats.Set(StatType::kGrit, 37_fp);
        if (i == 5)
        {
            add_innate(full_data);
            add_attack(full_data);
            full_data.data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 1000_fp);
            full_data.data.type_data.stats.Set(StatType::kAttackRangeUnits, 3_fp);
        }

        Entity* entity = nullptr;
        full_data.instance.team = Team::kBlue;
        full_data.instance.id = GenerateRandomUniqueID();
        full_data.instance.position = Reflect(world->GetByID(red_entities[i]).Get<PositionComponent>().GetPosition());
        SpawnCombatUnit(full_data, entity);
        ASSERT_NE(entity, nullptr);

        blue_entities.push_back(entity->GetID());
    }

    // Auras will be applied on the next aura system tick
    for (EntityID entity_id : red_entities)
    {
        auto stats = world->GetLiveStats(entity_id);
        ASSERT_EQ(stats.Get(StatType::kGrit), 73_fp);
    }

    EventHistory<EventType::kAuraCreated> aura_created(*world);
    EventHistory<EventType::kAuraDestroyed> aura_destroyed(*world);
    EntityID blue_entity_with_aura_id = kInvalidEntityID;

    for (size_t i = 0; i != 30; ++i)
    {
        world->TimeStep();
        ASSERT_EQ(aura_created.Size(), 1);

        if (i == 0)
        {
            const EntityID aura_id = aura_created.events.front().entity_id;
            ASSERT_TRUE(EntityHelper::IsAnAura(*world, aura_id));
            const auto& aura = world->GetByID(aura_id);
            const auto& aura_data = aura.Get<AuraComponent>().GetComponentData();
            blue_entity_with_aura_id = aura_data.receiver_id;
            ASSERT_TRUE(world->HasEntity(blue_entity_with_aura_id));
        }

        const Entity& blue_entity_with_aura = world->GetByID(blue_entity_with_aura_id);
        const FixedPoint aura_holder_resolve = world->GetLiveStats(blue_entity_with_aura).Get(StatType::kResolve);

        if (!aura_destroyed.IsEmpty())
        {
            ASSERT_EQ(aura_destroyed.Size(), 1);
            ASSERT_EQ(aura_holder_resolve, 0_fp);
            // The debuff should not exist anymore
            for (EntityID entity_id : red_entities)
            {
                const auto& entity = world->GetByID(entity_id);
                ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), 73_fp);
            }
        }
        else
        {
            ASSERT_EQ(aura_destroyed.Size(), 0);
            ASSERT_EQ(aura_holder_resolve, 24_fp);

            const HexGridPosition aura_position = blue_entity_with_aura.Get<PositionComponent>().GetPosition();

            for (EntityID entity_id : red_entities)
            {
                const auto& entity = world->GetByID(entity_id);
                const HexGridPosition entity_position = entity.Get<PositionComponent>().GetPosition();
                const auto distance = (aura_position - entity_position).Length();
                const bool in_range = distance <= 15;
                ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), in_range ? 31_fp : 73_fp) << fmt::format(
                    "i = {}, entity_id = {}. Aura position: {}, entity_position: {}. Distance: {}",
                    i,
                    entity_id,
                    aura_position,
                    entity_position,
                    distance);
            }
        }

        // Blue entities should not lose any grit
        for (EntityID entity_id : blue_entities)
        {
            const auto& entity = world->GetByID(entity_id);
            ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), 37_fp);
        }
    }
}

TEST_F(AuraTest, AuraOnEnemyWithDebuffForEnemies)
{
    constexpr HexGridPosition initial_red_position{0, -5};
    std::vector<EntityID> red_entities;
    for (int i = 0; i != 10; ++i)
    {
        Entity* entity = nullptr;
        auto full_data = MakeFullData();
        full_data.instance.team = Team::kRed;
        full_data.instance.id = GenerateRandomUniqueID();
        full_data.instance.position = initial_red_position + HexGridPosition{3, -2} * (i + 1);
        full_data.data.type_data.stats.Set(StatType::kGrit, 73_fp);
        full_data.data.type_data.stats.Set(StatType::kResolve, 31_fp);
        SpawnCombatUnit(full_data, entity);
        ASSERT_NE(entity, nullptr);

        red_entities.push_back(entity->GetID());
    }

    auto add_innate = [this](FullCombatUnitData& full_data)
    {
        auto& innate = full_data.data.type_data.innate_abilities.AddAbility();
        constexpr std::string_view ability_json = R"({
            "Name": "Aura with grit debuff for each enemy in 15 hexes",
            "ActivationTrigger": { "TriggerType": "OnBattleStart" },
            "TotalDurationMs": 0,
            "Skills": [
                {
                    "Name": "Grit aura",
                    "Deployment": { "Type": "Direct" },
                    "Targeting": {
                        "Type": "DistanceCheck",
                        "Group": "Enemy",
                        "Lowest": true,
                        "Num": 1
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "Debuff",
                                "Stat": "Resolve",
                                "DurationMs": 1500,
                                "Expression": 24
                            },
                            {
                                "Type": "Aura",
                                "AttachedEffects": [
                                    {
                                        "Type": "Debuff",
                                        "Stat": "Grit",
                                        "DurationMs": 1500,
                                        "Expression": 42,
                                        "RadiusUnits": 15
                                    }
                                ]
                            }
                        ]
                    }
                }
            ]
        })";
        ASSERT_TRUE(loader->ParseAndLoadAbility(ability_json, AbilityType::kInnate, &innate));
    };

    auto add_attack = [this](FullCombatUnitData& full_data)
    {
        ASSERT_TRUE(loader->ParseAndLoadAbility(
            SimpleAttackAbility(),
            AbilityType::kAttack,
            &full_data.data.type_data.innate_abilities.AddAbility()));
    };

    std::vector<EntityID> blue_entities;

    for (size_t i = 0; i != 10; ++i)
    {
        auto full_data = MakeFullData();
        full_data.data.type_data.stats.Set(StatType::kResolve, 13_fp);
        full_data.data.type_data.stats.Set(StatType::kGrit, 37_fp);
        if (i == 5)
        {
            add_innate(full_data);
            add_attack(full_data);
            full_data.data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 1000_fp);
            full_data.data.type_data.stats.Set(StatType::kAttackRangeUnits, 3_fp);
        }

        Entity* entity = nullptr;
        full_data.instance.team = Team::kBlue;
        full_data.instance.id = GenerateRandomUniqueID();
        full_data.instance.position = Reflect(world->GetByID(red_entities[i]).Get<PositionComponent>().GetPosition());
        SpawnCombatUnit(full_data, entity);
        ASSERT_NE(entity, nullptr);

        blue_entities.push_back(entity->GetID());
    }

    // Auras will be applied on the next aura system tick
    for (EntityID entity_id : red_entities)
    {
        auto stats = world->GetLiveStats(entity_id);
        ASSERT_EQ(stats.Get(StatType::kGrit), 73_fp);
        ASSERT_EQ(stats.Get(StatType::kResolve), 31_fp);
    }
    for (EntityID entity_id : blue_entities)
    {
        auto stats = world->GetLiveStats(entity_id);
        ASSERT_EQ(stats.Get(StatType::kGrit), 37_fp);
        ASSERT_EQ(stats.Get(StatType::kResolve), 13_fp);
    }

    EventHistory<EventType::kAuraCreated> aura_created(*world);
    EventHistory<EventType::kAuraDestroyed> aura_destroyed(*world);
    EntityID blue_entity_with_aura_id = kInvalidEntityID;

    for (size_t i = 0; i != 30; ++i)
    {
        world->TimeStep();
        ASSERT_EQ(aura_created.Size(), 1);

        if (i == 0)
        {
            const EntityID aura_id = aura_created.events.front().entity_id;
            ASSERT_TRUE(EntityHelper::IsAnAura(*world, aura_id));
            const auto& aura = world->GetByID(aura_id);
            const auto& aura_data = aura.Get<AuraComponent>().GetComponentData();
            blue_entity_with_aura_id = aura_data.receiver_id;
            ASSERT_TRUE(world->HasEntity(blue_entity_with_aura_id));
        }

        const Entity& blue_entity_with_aura = world->GetByID(blue_entity_with_aura_id);
        const FixedPoint aura_holder_resolve = world->GetLiveStats(blue_entity_with_aura).Get(StatType::kResolve);

        if (!aura_destroyed.IsEmpty())
        {
            ASSERT_EQ(aura_destroyed.Size(), 1);
            ASSERT_EQ(aura_holder_resolve, 31_fp);
            // The debuff should not exist anymore
            for (EntityID entity_id : red_entities)
            {
                const auto& entity = world->GetByID(entity_id);
                ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), 73_fp);
            }
        }
        else
        {
            ASSERT_EQ(aura_destroyed.Size(), 0);
            ASSERT_EQ(aura_holder_resolve, 7_fp);

            const HexGridPosition aura_position = blue_entity_with_aura.Get<PositionComponent>().GetPosition();

            for (EntityID entity_id : red_entities)
            {
                const auto& entity = world->GetByID(entity_id);
                const HexGridPosition entity_position = entity.Get<PositionComponent>().GetPosition();
                const auto distance = (aura_position - entity_position).Length();
                const bool in_range = distance <= 15;
                ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), in_range ? 31_fp : 73_fp) << fmt::format(
                    "i = {}, entity_id = {}. Aura position: {}, entity_position: {}. Distance: {}",
                    i,
                    entity_id,
                    aura_position,
                    entity_position,
                    distance);
            }
        }

        // Blue entities should not lose any grit or resolve
        for (EntityID entity_id : blue_entities)
        {
            const auto& entity = world->GetByID(entity_id);
            ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kGrit), 37_fp);
            ASSERT_EQ(world->GetLiveStats(entity).Get(StatType::kResolve), 13_fp);
        }
    }
}

}  // namespace simulation
