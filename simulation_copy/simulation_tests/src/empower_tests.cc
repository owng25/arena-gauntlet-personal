#include "ability_system_data_fixtures.h"
#include "components/position_component.h"
#include "data/enums.h"
#include "gtest/gtest.h"
#include "systems/effect_system.h"
#include "test_data_loader.h"

namespace simulation
{
class EmpowerTest : public BaseTest
{
protected:
    static CombatUnitData MakeUnitDataWithDefaultStats()
    {
        CombatUnitData data = CreateCombatUnitData();

        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kEnergyResist, 0_fp);
        data.type_data.stats.Set(StatType::kPhysicalResist, 0_fp);
        data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 300_fp);
        data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 5_fp);
        data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);         // No critical attacks
        data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);  // No Dodge
        data.type_data.stats.Set(StatType::kHitChancePercentage, 0_fp);          // No Miss
        data.type_data.stats.Set(StatType::kResolve, 0_fp);
        data.type_data.stats.Set(StatType::kGrit, 0_fp);
        data.type_data.stats.Set(StatType::kWillpowerPercentage, 0_fp);
        data.type_data.stats.Set(StatType::kTenacityPercentage, 0_fp);
        data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 10000_fp);
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);

        // Health
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

        // Omega
        data.type_data.stats.Set(StatType::kOmegaRangeUnits, 150_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);

        // Attack
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 150_fp);
        data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);

        return data;
    }

    void AddAttachedEffectToSelf(const Entity& entity, const std::string_view effect_json_text)
    {
        TestingDataLoader loader(world->GetLogger());
        const auto effect_data = loader.ParseAndLoadEffect(effect_json_text);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(entity.GetID());
        GetAttachedEffectsHelper().AddAttachedEffect(entity, entity.GetID(), *effect_data, effect_state);
    }

    void ParseAndLoadAbility(
        const std::string_view ability_json_text,
        const AbilityType ability_type,
        AbilityData& out_ability_data)
    {
        TestingDataLoader loader(world->GetLogger());
        const auto ability_json = loader.ParseJSON(ability_json_text);
        ASSERT_TRUE(ability_json.has_value());
        ASSERT_TRUE(loader.LoadAbility(*ability_json, ability_type, &out_ability_data));
    }
};

TEST_F(EmpowerTest, ActivationsUntilExpiryOnBattleStart)
{
    auto data = MakeUnitDataWithDefaultStats();

    constexpr std::array<HexGridPosition, 1> red_positions{{
        {-4, 6},
    }};
    std::array<Entity*, red_positions.size()> red_entities{};
    for (size_t index = 0; index != red_entities.size(); ++index)
    {
        SpawnCombatUnit(Team::kRed, red_positions[index], data, red_entities[index]);
        ASSERT_NE(red_entities[index], nullptr);
    }

    static constexpr std::string_view attack_ability_text = R"({
        "Name": "Ability",
        "TotalDurationMs": 0,
        "Skills": [
            {
                "Name": "Execute",
                "Targeting": {
                    "Type": "CurrentFocus"
                },
                "Deployment": {
                    "Type": "Direct"
                },
                "PercentageOfAbilityDuration": 100,
                "EffectPackage": {
                    "Effects": [
                        {
                            "Type": "InstantDamage",
                            "DamageType": "Physical",
                            "Expression": 50
                        }
                    ]
                }
            }
        ]
    })";
    ParseAndLoadAbility(attack_ability_text, AbilityType::kAttack, data.type_data.attack_abilities.AddAbility());
    data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    static constexpr std::string_view innate_ability_text = R"({
        "Name": "Innate with empower",
        "ActivationTrigger": {
            "TriggerType": "OnBattleStart"
        },
        "TotalDurationMs": 0,
        "Skills": [
            {
                "Name": "Wound",
                "Deployment": {
                    "Type": "Direct"
                },
                "Targeting": {
                    "Type": "Self"
                },
                "EffectPackage": {
                    "Effects": [
                        {
                            "Type": "Empower",
                            "IsConsumable": true,
                            "ActivationsUntilExpiry": 2,
                            "ActivatedBy": "Attack",
                            "AttachedEffects": [
                                {
                                    "Type": "InstantDamage",
                                    "DamageType": "Pure",
                                    "Expression": 100
                                }
                            ]
                        }
                    ]
                }
            }
        ]
    })";
    ParseAndLoadAbility(innate_ability_text, AbilityType::kInnate, data.type_data.innate_abilities.AddAbility());

    constexpr std::array<HexGridPosition, 1> blue_positions{{
        {0, -2},
    }};
    std::array<Entity*, blue_positions.size()> blue_entities{};
    for (size_t index = 0; index != blue_entities.size(); ++index)
    {
        SpawnCombatUnit(Team::kBlue, blue_positions[index], data, blue_entities[index]);
        ASSERT_NE(blue_entities[index], nullptr);
    }

    Entity& red = *red_entities.front();
    Entity& blue = *blue_entities.front();

    EventHistory<EventType::kAbilityActivated> on_activated(*world);
    EventHistory<EventType::kOnDamage> on_damage(*world);

    constexpr size_t activations_count = 2;

    for (size_t i = 1; i <= activations_count + 1; ++i)
    {
        world->TimeStep();

        // Activated ability: expect blue to attack every time step
        {
            if (i == 1)
            {
                ASSERT_EQ(on_activated.Size(), 2);
                const auto& e = on_activated.events.front();
                ASSERT_EQ(e.ability_type, AbilityType::kInnate);
                ASSERT_EQ(e.sender_id, blue.GetID());
            }
            else
            {
                ASSERT_EQ(on_activated.Size(), 1) << "i = " << i;
            }

            {
                const auto& e = on_activated.events.back();
                ASSERT_EQ(e.ability_type, AbilityType::kAttack);
                ASSERT_EQ(e.sender_id, blue.GetID());
            }
        }

        // Test damage events.
        // On the first time step expect two events: one with physical damage
        // and another with pure damage from empower.
        // On the last time step expect only one damage event from attack ability
        {
            const bool expect_attack_with_empower = i <= activations_count;
            const size_t expected_damage_events = expect_attack_with_empower ? 2 : 1;
            ASSERT_EQ(on_damage.Size(), expected_damage_events) << "i = " << i;

            {
                const auto& e = on_damage.events[0];
                ASSERT_EQ(e.sender_id, blue.GetID());
                ASSERT_EQ(e.receiver_id, red.GetID());
                ASSERT_EQ(e.damage_type, EffectDamageType::kPhysical);
                ASSERT_EQ(e.damage_value, 50_fp);
            }

            if (expect_attack_with_empower)
            {
                const auto& e = on_damage.events[1];
                ASSERT_EQ(e.sender_id, blue.GetID());
                ASSERT_EQ(e.receiver_id, red.GetID());
                ASSERT_EQ(e.damage_type, EffectDamageType::kPure);
                ASSERT_EQ(e.damage_value, 100_fp);
            }
        }

        {
            const auto& attached_effects_component = blue.Get<AttachedEffectsComponent>();
            const auto empowers = attached_effects_component.GetEmpowers();

            if (i <= activations_count)
            {
                ASSERT_EQ(empowers.size(), 1);
                const auto& empower = empowers.front();
                ASSERT_EQ(empower->total_activations_lifetime, i - 1);
                ASSERT_EQ(empower->IsExpired(), i - 1 == activations_count);
            }
            else
            {
                ASSERT_EQ(empowers.size(), 0);
            }
        }

        on_activated.Clear();
        on_damage.Clear();
    }
}

TEST_F(EmpowerTest, ActivationsUntilExpiryOnVanquish)
{
    const auto red_data = MakeUnitDataWithDefaultStats();

    // Red entities are "dummies", so they do not have any abilities
    constexpr std::array<HexGridPosition, 2> red_positions{{
        {-4, 6},
        {5, 6},
    }};
    std::array<Entity*, red_positions.size()> red_entities{};
    for (size_t index = 0; index != red_entities.size(); ++index)
    {
        SpawnCombatUnit(Team::kRed, red_positions[index], red_data, red_entities[index]);
        ASSERT_NE(red_entities[index], nullptr);
    }

    auto blue_data = MakeUnitDataWithDefaultStats();

    static constexpr std::string_view attack_ability_text = R"({
        "Name": "Ability",
        "TotalDurationMs": 0,
        "Skills": [
            {
                "Name": "Execute",
                "Targeting": {
                    "Type": "CurrentFocus"
                },
                "Deployment": {
                    "Type": "Direct"
                },
                "PercentageOfAbilityDuration": 100,
                "EffectPackage": {
                    "Effects": [
                        {
                            "Type": "InstantDamage",
                            "DamageType": "Physical",
                            "Expression": 50
                        }
                    ]
                }
            }
        ]
    })";
    ParseAndLoadAbility(attack_ability_text, AbilityType::kAttack, blue_data.type_data.attack_abilities.AddAbility());
    blue_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    static constexpr std::string_view innate_ability_text = R"({
        "Name": "Innate with empower",
        "ActivationTrigger": {
            "TriggerType": "OnVanquish",
            "Allegiance": "Self"
        },
        "TotalDurationMs": 0,
        "Skills": [
            {
                "Name": "Wound",
                "Deployment": {
                    "Type": "Direct"
                },
                "Targeting": {
                    "Type": "Self"
                },
                "EffectPackage": {
                    "Effects": [
                        {
                            "Type": "Empower",
                            "IsConsumable": true,
                            "ActivationsUntilExpiry": 1,
                            "ActivatedBy": "Attack",
                            "AttachedEffects": [
                                {
                                    "Type": "InstantDamage",
                                    "DamageType": "Pure",
                                    "Expression": 100
                                }
                            ]
                        }
                    ]
                }
            }
        ]
    })";
    ParseAndLoadAbility(innate_ability_text, AbilityType::kInnate, blue_data.type_data.innate_abilities.AddAbility());

    constexpr std::array<HexGridPosition, 1> blue_positions{{
        {0, -2},
    }};
    std::array<Entity*, blue_positions.size()> blue_entities{};
    for (size_t index = 0; index != blue_entities.size(); ++index)
    {
        SpawnCombatUnit(Team::kBlue, blue_positions[index], blue_data, blue_entities[index]);
        ASSERT_NE(blue_entities[index], nullptr);
    }

    Entity& closest_red = *red_entities[0];
    closest_red.Get<StatsComponent>().GetMutableTemplateStats().Set(StatType::kCurrentHealth, 50_fp);

    Entity& far_red = *red_entities[1];
    Entity& blue = *blue_entities.front();

    const auto has_active_empower = [](const Entity& entity) -> bool
    {
        auto& attached_effects = entity.Get<AttachedEffectsComponent>();
        const auto& empowers = attached_effects.GetEmpowers();
        if (empowers.empty())
        {
            return false;
        }

        const auto& empower = empowers.front();
        return !empower->IsExpired();
    };

    EventHistory<EventType::kOnDamage> on_damage(*world);
    EventHistory<EventType::kAbilityActivated> on_activated(*world);
    EventHistory<EventType::kFainted> on_fainted(*world);

    {
        // Time step 1:
        // Blue kills the closest red entity by dealing 50 damage
        // Which must trigger an innate ability which gives blue an empower
        world->TimeStep();

        ASSERT_EQ(on_damage.Size(), 1);
        {
            const auto& e = on_damage[0];
            ASSERT_EQ(e.sender_id, blue.GetID());
            ASSERT_EQ(e.receiver_id, closest_red.GetID());
            ASSERT_EQ(e.damage_type, EffectDamageType::kPhysical);
            ASSERT_EQ(e.damage_value, 50_fp);
        }

        ASSERT_EQ(on_fainted.Size(), 1);
        {
            const auto& e = on_fainted[0];
            ASSERT_EQ(e.entity_id, closest_red.GetID());
            ASSERT_EQ(e.vanquisher_id, blue.GetID());
        }

        ASSERT_TRUE(has_active_empower(blue));

        on_damage.Clear();
        on_fainted.Clear();
        on_activated.Clear();
    }

    {
        // Time step 2:
        // Blue entity deactivates ability since target is dead.
        // Empower must remain active.
        world->TimeStep();
        ASSERT_EQ(on_damage.Size(), 0);
        ASSERT_TRUE(on_activated.IsEmpty());
        ASSERT_TRUE(has_active_empower(blue));
    }

    {
        // Time step 2:
        // Blue entity attacks far right with empower
        // Empower must remain active (will be removed with deactivation on the next time step)
        world->TimeStep();

        ASSERT_EQ(on_damage.Size(), 2);
        {
            const auto& e = on_damage[0];
            ASSERT_EQ(e.sender_id, blue.GetID());
            ASSERT_EQ(e.receiver_id, far_red.GetID());
            ASSERT_EQ(e.damage_type, EffectDamageType::kPhysical);
            ASSERT_EQ(e.damage_value, 50_fp);
        }
        {
            const auto& e = on_damage[1];
            ASSERT_EQ(e.sender_id, blue.GetID());
            ASSERT_EQ(e.receiver_id, far_red.GetID());
            ASSERT_EQ(e.damage_type, EffectDamageType::kPure);
            ASSERT_EQ(e.damage_value, 100_fp);
        }

        ASSERT_EQ(on_activated.Size(), 1);
        ASSERT_TRUE(has_active_empower(blue));

        on_damage.Clear();
        on_activated.Clear();
    }

    {
        // Time step 3:
        // Blue entity attacks far right without empower
        world->TimeStep();
        ASSERT_EQ(on_damage.Size(), 1);
        {
            const auto& e = on_damage[0];
            ASSERT_EQ(e.sender_id, blue.GetID());
            ASSERT_EQ(e.receiver_id, far_red.GetID());
            ASSERT_EQ(e.damage_type, EffectDamageType::kPhysical);
            ASSERT_EQ(e.damage_value, 50_fp);
        }

        ASSERT_EQ(on_activated.Size(), 1);
        ASSERT_FALSE(has_active_empower(blue));

        on_damage.Clear();
        on_activated.Clear();
    }
}

}  // namespace simulation
