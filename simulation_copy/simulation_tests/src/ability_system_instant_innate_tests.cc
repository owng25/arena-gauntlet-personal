#include <span>
#include <variant>

#include "base_test_fixtures.h"
#include "test_data_loader.h"
#include "utility/entity_helper.h"

namespace simulation
{

class AbilitySystemInstantInnateTest : public BaseTest
{
public:
    struct SenderId
    {
        int sender_id = kInvalidEntityID;
    };

    struct Sender
    {
        const Entity* sender = nullptr;
    };

    struct AbilityName
    {
        std::string_view name;
    };

    using FilterParam = std::variant<AbilityType, SenderId>;

    static bool CheckFilter(const event_data::AbilityActivated& event, const AbilityType& ability_type)
    {
        return event.ability_type == ability_type;
    }

    static bool CheckFilter(const event_data::AbilityActivated& event, const SenderId& sender)
    {
        return event.sender_id == sender.sender_id;
    }

    static bool CheckFilter(const event_data::AbilityActivated& event, const Sender& sender)
    {
        return event.sender_id == sender.sender->GetID();
    }

    static bool CheckFilter(const event_data::AbilityActivated& event, const AbilityName& name)
    {
        return event.ability_name == name.name;
    }

    template <typename... FilterArg>
    static size_t CountAbilities(std::span<const event_data::AbilityActivated> abilities, const FilterArg&... filters)
    {
        return static_cast<size_t>(std::count_if(
            abilities.begin(),
            abilities.end(),
            [&](const event_data::AbilityActivated& event)
            {
                return (true && ... && CheckFilter(event, filters));
            }));
    }

    template <typename... FilterArg>
    static size_t CountAbilities(const EventHistory<EventType::kAbilityActivated>& history, const FilterArg&... filters)
    {
        return CountAbilities(history.events, filters...);
    }

    static constexpr std::string_view kDummyStaticTemplateJSON = R"({
        "UnitType": "Illuvial",
        "Line": "DummyStatic",
        "Stage": 1,
        "Path": "",
        "Variation": "",
        "Tier": 0,
        "CombatAffinity": "Water",
        "CombatClass": "Fighter",
        "DominantCombatAffinity": "Water",
        "DominantCombatClass": "Fighter",
        "SizeUnits": 2,
        "Stats": {
            "MaxHealth": 1000000,
            "StartingEnergy": 0,
            "EnergyCost": 200000,
            "PhysicalResist": 0,
            "EnergyResist": 0,
            "TenacityPercentage": 0,
            "WillpowerPercentage": 0,
            "Grit": 0,
            "Resolve": 0,
            "AttackPhysicalDamage": 0,
            "AttackEnergyDamage": 0,
            "AttackPureDamage": 0,
            "AttackSpeed": 100,
            "MoveSpeedSubUnits": 0,
            "HitChancePercentage": 100,
            "AttackDodgeChancePercentage": 0,
            "CritChancePercentage": 0,
            "CritAmplificationPercentage": 150,
            "OmegaPowerPercentage": 100,
            "AttackRangeUnits": 125,
            "OmegaRangeUnits": 0,
            "HealthRegeneration": 0,
            "EnergyRegeneration": 0,
            "EnergyGainEfficiencyPercentage": 100,
            "OnActivationEnergy": 0,
            "VulnerabilityPercentage": 100,
            "EnergyPiercingPercentage": 0,
            "PhysicalPiercingPercentage": 0,
            "HealthGainEfficiencyPercentage": 100,
            "PhysicalVampPercentage": 0,
            "EnergyVampPercentage": 0,
            "PureVampPercentage": 0,
            "Thorns": 0,
            "StartingShield": 2000000000,
            "CritReductionPercentage": 0
        },
        "AttackAbilitiesSelection": "Cycle",
        "AttackAbilities": [],
        "OmegaAbilitiesSelection": "Cycle",
        "OmegaAbilities": []
    })";

    static constexpr std::string_view kInstantDamageAbilityJSON = R"({
        "Name": "Damage",
        "ActivationTrigger": { "TriggerType": "OnBattleStart" },
        "TotalDurationMs": 0,
        "Skills": [
            {
                "Name": "Deal 100 damage to current focus",
                "Deployment": { "Type": "Direct" },
                "Targeting": { "Type": "CurrentFocus" },
                "EffectPackage": {
                    "Effects": [
                        {
                            "Type": "InstantDamage",
                            "DamageType": "Pure",
                            "Expression": 100000
                        }
                    ]
                }
            }
        ]
    })";

    static constexpr std::string_view kInstantHealAbilityJSON = R"({
        "Name": "Heal",
        "ActivationTrigger": { "TriggerType": "OnBattleStart" },
        "TotalDurationMs": 0,
        "Skills": [
            {
                "Name": "Self heal 100 hp",
                "Deployment": { "Type": "Direct" },
                "Targeting": { "Type": "Self" },
                "EffectPackage": {
                    "Effects": [
                        {
                            "Type": "InstantHeal",
                            "HealType": "Normal",
                            "Expression": 100000
                        }
                    ]
                }
            }
        ]
    })";

    static constexpr std::string_view kDelayedInstantDamageAbilityJSON = R"({
        "Name": "Damage",
        "ActivationTrigger": {
            "TriggerType": "OnBattleStart",
            "NotBeforeMs": 500
        },
        "TotalDurationMs": 0,
        "Skills": [
            {
                "Name": "Deal 100 damage to current focus",
                "Deployment": { "Type": "Direct" },
                "Targeting": { "Type": "CurrentFocus" },
                "EffectPackage": {
                    "Effects": [
                        {
                            "Type": "InstantDamage",
                            "DamageType": "Pure",
                            "Expression": 100000
                        }
                    ]
                }
            }
        ]
    })";

    static constexpr std::string_view kDelayedInstantHealAbilityJSON = R"({
        "Name": "Heal",
        "ActivationTrigger": {
            "TriggerType": "OnBattleStart",
            "NotBeforeMs": 500
        },
        "TotalDurationMs": 0,
        "Skills": [
            {
                "Name": "Self heal 100 hp",
                "Deployment": { "Type": "Direct" },
                "Targeting": { "Type": "Self" },
                "EffectPackage": {
                    "Effects": [
                        {
                            "Type": "InstantHeal",
                            "HealType": "Normal",
                            "Expression": 100000
                        }
                    ]
                }
            }
        ]
    })";

    static constexpr std::string_view kAttackAbility = R"({
        "Name": "Attack",
        "Skills": [
            {
                "Name": "Deal 100 damage to current focus",
                "Deployment": {
                    "Type": "Direct",
                    "PreDeploymentDelayPercentage": 0
                },
                "Targeting": { "Type": "CurrentFocus" },
                "EffectPackage": {
                    "Effects": [
                        {
                            "Type": "InstantDamage",
                            "DamageType": "Pure",
                            "Expression": 100000
                        }
                    ]
                }
            }
        ]
    })";

    bool AddAbility(CombatUnitData& data, AbilityType type, TestingDataLoader& loader, std::string_view text) const
    {
        AbilitiesData* abilities = nullptr;
        ILLUVIUM_ENSURE_ENUM_SIZE(AbilityType, 4);
        switch (type)
        {
        case AbilityType::kAttack:
            abilities = &data.type_data.attack_abilities;
            break;
        case AbilityType::kOmega:
            abilities = &data.type_data.omega_abilities;
            break;
        case AbilityType::kInnate:
            abilities = &data.type_data.innate_abilities;
            break;
        default:
            assert(false);
            return false;
        }

        AbilityData* ability_data = &abilities->AddAbility();
        if (!loader.ParseAndLoadAbility(text, type, ability_data))
        {
            abilities->abilities.pop_back();
            return false;
        }

        return true;
    }
};

TEST_F(AbilitySystemInstantInnateTest, OneInnateOnBattleStart)
{
    TestingDataLoader loader(world->GetLogger());

    auto unit_json = loader.ParseJSON(kDummyStaticTemplateJSON);
    ASSERT_TRUE(unit_json);

    CombatUnitData unit_data{};
    ASSERT_TRUE(loader.LoadCombatUnitData(*unit_json, &unit_data));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kInnate, loader, kInstantDamageAbilityJSON));

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {0, -3}, unit_data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, Reflect({0, -3}), unit_data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    EventHistory<EventType::kAbilityActivated> activated_abilities(*world);
    TimeStepForNTimeSteps(2);
    ASSERT_EQ(activated_abilities.Size(), 2);
    ASSERT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Damage"}, Sender{red_entity}), 1);
    ASSERT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Damage"}, Sender{blue_entity}), 1);
}

TEST_F(AbilitySystemInstantInnateTest, OneInnateDuringAttack)
{
    TestingDataLoader loader(world->GetLogger());

    auto unit_json = loader.ParseJSON(kDummyStaticTemplateJSON);
    ASSERT_TRUE(unit_json);

    CombatUnitData unit_data{};
    ASSERT_TRUE(loader.LoadCombatUnitData(*unit_json, &unit_data));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kAttack, loader, kAttackAbility));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kInnate, loader, kInstantDamageAbilityJSON));

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {0, -3}, unit_data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, Reflect({0, -3}), unit_data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    EventHistory<EventType::kAbilityActivated> activated_abilities(*world);
    TimeStepForNTimeSteps(1);
    ASSERT_EQ(activated_abilities.Size(), 4);
    ASSERT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Damage"}, Sender{red_entity}), 1);
    ASSERT_EQ(CountAbilities(activated_abilities, AbilityType::kAttack, AbilityName{"Attack"}, Sender{red_entity}), 1);
    ASSERT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Damage"}, Sender{blue_entity}), 1);
    ASSERT_EQ(CountAbilities(activated_abilities, AbilityType::kAttack, AbilityName{"Attack"}, Sender{blue_entity}), 1);

    TimeStepForNTimeSteps(1);
    ASSERT_EQ(activated_abilities.Size(), 4);
}

TEST_F(AbilitySystemInstantInnateTest, OneDelayedInnateOnBattleStart)
{
    TestingDataLoader loader(world->GetLogger());

    auto unit_json = loader.ParseJSON(kDummyStaticTemplateJSON);
    ASSERT_TRUE(unit_json);

    CombatUnitData unit_data{};
    ASSERT_TRUE(loader.LoadCombatUnitData(*unit_json, &unit_data));
    auto damage_ability = AddAbility(unit_data, AbilityType::kInnate, loader, kDelayedInstantDamageAbilityJSON);
    ASSERT_TRUE(damage_ability);

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {0, -3}, unit_data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, Reflect({0, -3}), unit_data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    EventHistory<EventType::kAbilityActivated> activated_abilities(*world);
    TimeStepForNTimeSteps(4);
    ASSERT_EQ(activated_abilities.Size(), 0);
    TimeStepForNTimeSteps(1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Damage"}, Sender{red_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Damage"}, Sender{blue_entity}), 1);
    ASSERT_EQ(activated_abilities.Size(), 2);
}

TEST_F(AbilitySystemInstantInnateTest, TwoInnatesOnBattleStart)
{
    TestingDataLoader loader(world->GetLogger());

    auto unit_json = loader.ParseJSON(kDummyStaticTemplateJSON);
    ASSERT_TRUE(unit_json);

    CombatUnitData unit_data{};
    ASSERT_TRUE(loader.LoadCombatUnitData(*unit_json, &unit_data));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kInnate, loader, kInstantDamageAbilityJSON));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kInnate, loader, kInstantHealAbilityJSON));

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {0, -3}, unit_data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, Reflect({0, -3}), unit_data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    EventHistory<EventType::kAbilityActivated> activated_abilities(*world);
    TimeStepForNTimeSteps(2);

    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Damage"}, Sender{blue_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Heal"}, Sender{blue_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Damage"}, Sender{red_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Heal"}, Sender{red_entity}), 1);
    ASSERT_EQ(activated_abilities.Size(), 4);
}

TEST_F(AbilitySystemInstantInnateTest, TwoDelayedInnatesOnBattleStart)
{
    TestingDataLoader loader(world->GetLogger());

    auto unit_json = loader.ParseJSON(kDummyStaticTemplateJSON);
    ASSERT_TRUE(unit_json);

    CombatUnitData unit_data{};
    ASSERT_TRUE(loader.LoadCombatUnitData(*unit_json, &unit_data));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kInnate, loader, kDelayedInstantDamageAbilityJSON));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kInnate, loader, kDelayedInstantHealAbilityJSON));

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {0, -3}, unit_data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, Reflect({0, -3}), unit_data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    EventHistory<EventType::kAbilityActivated> activated_abilities(*world);
    TimeStepForNTimeSteps(4);
    ASSERT_EQ(activated_abilities.Size(), 0);
    TimeStepForNTimeSteps(10);

    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Damage"}, Sender{blue_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Heal"}, Sender{blue_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Damage"}, Sender{red_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Heal"}, Sender{red_entity}), 1);
    ASSERT_EQ(activated_abilities.Size(), 4);
}

TEST_F(AbilitySystemInstantInnateTest, TwoDelayedInnatesDuringAttack)
{
    TestingDataLoader loader(world->GetLogger());

    auto unit_json = loader.ParseJSON(kDummyStaticTemplateJSON);
    ASSERT_TRUE(unit_json);

    CombatUnitData unit_data{};
    ASSERT_TRUE(loader.LoadCombatUnitData(*unit_json, &unit_data));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kAttack, loader, kAttackAbility));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kInnate, loader, kDelayedInstantDamageAbilityJSON));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kInnate, loader, kDelayedInstantHealAbilityJSON));

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {0, -3}, unit_data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, Reflect({0, -3}), unit_data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    EventHistory<EventType::kAbilityActivated> activated_abilities(*world);
    TimeStepForNTimeSteps(1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kAttack, AbilityName{"Attack"}, Sender{blue_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kAttack, AbilityName{"Attack"}, Sender{red_entity}), 1);
    ASSERT_EQ(activated_abilities.Size(), 2);

    TimeStepForNTimeSteps(3);
    ASSERT_EQ(activated_abilities.Size(), 2);
    TimeStepForNTimeSteps(1);

    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kAttack, AbilityName{"Attack"}, Sender{blue_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Damage"}, Sender{blue_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Heal"}, Sender{blue_entity}), 1);

    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kAttack, AbilityName{"Attack"}, Sender{red_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Damage"}, Sender{red_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, AbilityName{"Heal"}, Sender{red_entity}), 1);

    ASSERT_EQ(activated_abilities.Size(), 6);
}

TEST_F(AbilitySystemInstantInnateTest, TwoInfinitePositiveStatesFromDifferentAbilities)
{
    TestingDataLoader loader(world->GetLogger());

    auto unit_json = loader.ParseJSON(kDummyStaticTemplateJSON);
    ASSERT_TRUE(unit_json);

    constexpr std::string_view innate_format = R"({
        "Name": "+++",
        "ActivationTrigger": {
            "TriggerType": "OnBattleStart"
        },
        "TotalDurationMs": 0,
        "Skills": [
            {
                "Name": "True Sight and Attack Damage",
                "Deployment": {
                    "Type": "Direct"
                },
                "Targeting": {
                    "Type": "Self"
                },
                "EffectPackage": {
                    "Effects": [
                        {
                            "Type": "PositiveState",
                            "PositiveState": "Truesight",
                            "DurationMs": -1
                        }
                    ]
                }
            }
        ]
    })";

    const std::string innate_a_json = FindAndReplace(innate_format, "+++", "Innate A");
    const std::string innate_b_json = FindAndReplace(innate_format, "+++", "Innate B");

    CombatUnitData unit_data{};
    ASSERT_TRUE(loader.LoadCombatUnitData(*unit_json, &unit_data));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kAttack, loader, kAttackAbility));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kInnate, loader, innate_a_json));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kInnate, loader, innate_b_json));

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {0, -3}, unit_data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, Reflect({0, -3}), unit_data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    EventHistory<EventType::kAbilityActivated> activated_abilities(*world);
    TimeStepForNTimeSteps(1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kAttack, AbilityName{"Attack"}, Sender{blue_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kAttack, AbilityName{"Attack"}, Sender{red_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, Sender{blue_entity}), 2);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, Sender{red_entity}), 2);
    ASSERT_EQ(activated_abilities.Size(), 6);

    ASSERT_TRUE(EntityHelper::HasPositiveState(*red_entity, EffectPositiveState::kTruesight));
    ASSERT_TRUE(EntityHelper::HasPositiveState(*blue_entity, EffectPositiveState::kTruesight));

    for (size_t i = 0; i != 3; ++i)
    {
        TimeStepForNTimeSteps(1);
        ASSERT_TRUE(EntityHelper::HasPositiveState(*red_entity, EffectPositiveState::kTruesight));
        ASSERT_TRUE(EntityHelper::HasPositiveState(*blue_entity, EffectPositiveState::kTruesight));
    }
}

TEST_F(AbilitySystemInstantInnateTest, TwoInfinitePositiveStatesFromSameAbility)
{
    TestingDataLoader loader(world->GetLogger());

    auto unit_json = loader.ParseJSON(kDummyStaticTemplateJSON);
    ASSERT_TRUE(unit_json);

    constexpr std::string_view innate_format = R"({
        "Name": "+++",
        "ActivationTrigger": {
            "TriggerType": "OnBattleStart"
        },
        "TotalDurationMs": 0,
        "Skills": [
            {
                "Name": "True Sight and Attack Damage",
                "Deployment": {
                    "Type": "Direct"
                },
                "Targeting": {
                    "Type": "Self"
                },
                "EffectPackage": {
                    "Effects": [
                        {
                            "Type": "PositiveState",
                            "PositiveState": "Truesight",
                            "DurationMs": -1
                        }
                    ]
                }
            }
        ]
    })";

    const std::string innate_json = FindAndReplace(innate_format, "+++", "Innate A");

    CombatUnitData unit_data{};
    ASSERT_TRUE(loader.LoadCombatUnitData(*unit_json, &unit_data));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kAttack, loader, kAttackAbility));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kInnate, loader, innate_json));
    ASSERT_TRUE(AddAbility(unit_data, AbilityType::kInnate, loader, innate_json));

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {0, -3}, unit_data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, Reflect({0, -3}), unit_data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    EventHistory<EventType::kAbilityActivated> activated_abilities(*world);
    TimeStepForNTimeSteps(1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kAttack, AbilityName{"Attack"}, Sender{blue_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kAttack, AbilityName{"Attack"}, Sender{red_entity}), 1);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, Sender{blue_entity}), 2);
    EXPECT_EQ(CountAbilities(activated_abilities, AbilityType::kInnate, Sender{red_entity}), 2);
    ASSERT_EQ(activated_abilities.Size(), 6);

    ASSERT_TRUE(EntityHelper::HasPositiveState(*red_entity, EffectPositiveState::kTruesight));
    ASSERT_TRUE(EntityHelper::HasPositiveState(*blue_entity, EffectPositiveState::kTruesight));

    for (size_t i = 0; i != 3; ++i)
    {
        TimeStepForNTimeSteps(1);
        ASSERT_TRUE(EntityHelper::HasPositiveState(*red_entity, EffectPositiveState::kTruesight));
        ASSERT_TRUE(EntityHelper::HasPositiveState(*blue_entity, EffectPositiveState::kTruesight));
    }
}

}  // namespace simulation
