#include "base_json_test.h"
#include "data/augment/augment_data.h"

namespace simulation
{

class AugmentTestJSON : public BaseJSONTest
{
public:
    using Super = BaseJSONTest;
};

TEST_F(AugmentTestJSON, Normal)
{
    // Regular valid scenario
    {
        const char* augment_data_json_raw = R"( {
            "Name": "Augment name",
            "Stage": 2,
            "Tier": 1,
            "Type": "Normal",
            "Abilities": [{
                "Name": "sdasda",
                "TotalDurationMs": 0,
                "ActivationTrigger": {
                    "TriggerType": "OnDamage",
                    "SenderAllegiance": "Enemy",
                    "ReceiverAllegiance": "Self",
                    "AbilityTypes": ["Omega"]
                },
                "Skills": [
                    {
                        "Name": "After Omega",
                        "Deployment": {
                            "Type": "Direct"
                        },
                        "Targeting": {
                            "Type": "Self"
                        },
                        "EffectPackage": {
                            "Effects": [
                                {
                                    "Type": "Buff",
                                    "Stat": "OmegaPowerPercentage",
                                    "DurationMs": -1,
                                    "Expression": 10
                                }
                            ]
                        }
                    }
                ]
            }]
        })";

        auto maybe_augment_data = loader->ParseAndLoadAugment(augment_data_json_raw);
        ASSERT_TRUE(maybe_augment_data.has_value());

        const AugmentData& data = *maybe_augment_data;
        EXPECT_EQ(data.tier, 1);

        auto& type_id = data.type_id;
        EXPECT_EQ(type_id.name, "Augment name");
        EXPECT_EQ(type_id.stage, 2);

        // Check abilities
        {
            ASSERT_EQ(data.innate_abilities.size(), 1);

            const auto& ability_data = *data.innate_abilities.at(0);
            ASSERT_EQ(ability_data.name, "sdasda");

            // Check skills
            ASSERT_EQ(ability_data.skills.size(), 1);
            const auto& skill_data = *ability_data.skills.at(0);

            ASSERT_EQ(skill_data.name, "After Omega");
            ASSERT_EQ(skill_data.deployment.type, SkillDeploymentType::kDirect);
            ASSERT_EQ(skill_data.targeting.type, SkillTargetingType::kSelf);

            // Check effects
            ASSERT_EQ(skill_data.effect_package.effects.size(), 1);

            const auto& first_effect_data = skill_data.effect_package.effects.at(0);
            EXPECT_EQ(first_effect_data.type_id.type, EffectType::kBuff);
            EXPECT_EQ(first_effect_data.type_id.stat_type, StatType::kOmegaPowerPercentage);

            // Should be different depending on the current threshold value
            EXPECT_EQ(first_effect_data.lifetime.duration_time_ms, -1);
            EXPECT_EQ(first_effect_data.GetExpression().base_value.value, 10_fp);

            const auto& trigger = ability_data.activation_trigger_data;
            ASSERT_EQ(trigger.trigger_type, ActivationTriggerType::kOnDamage);
            ASSERT_EQ(trigger.sender_allegiance, AllegianceType::kEnemy);
            ASSERT_EQ(trigger.receiver_allegiance, AllegianceType::kSelf);
            ASSERT_EQ(trigger.ability_types, MakeEnumSet(AbilityType::kOmega));
        }
    }

    // Wrong: Contains shared activation trigger
    {
        const char* json_text = R"( {
            "Name": "Augment name",
            "Stage": 2,
            "Tier": 1,
            "Type": "NewNormal",
            "ActivationTrigger": {
                    "TriggerType": "OnDamage",
                    "SenderAllegiance": "Enemy",
                    "ReceiverAllegiance": "Self",
                    "AbilityTypes": ["Omega"]
                },
            "Abilities": [{
                "Name": "sdasda",
                "TotalDurationMs": 0,
                "ActivationTrigger": {
                    "TriggerType": "OnDamage",
                    "SenderAllegiance": "Enemy",
                    "ReceiverAllegiance": "Self",
                    "AbilityTypes": ["Omega"]
                },
                "Skills": [
                    {
                        "Name": "After Omega",
                        "Deployment": {
                            "Type": "Direct"
                        },
                        "Targeting": {
                            "Type": "Self"
                        },
                        "EffectPackage": {
                            "Effects": [
                                {
                                    "Type": "Buff",
                                    "Stat": "OmegaPowerPercentage",
                                    "DurationMs": -1,
                                    "Expression": 10
                                }
                            ]
                        }
                    }
                ]
            }]
        })";

        auto maybe_augment = silent_loader->ParseAndLoadAugment(json_text);
        ASSERT_FALSE(maybe_augment.has_value());
    }

    // Wrong: No abilities
    {
        const char* json_text = R"( {
            "Name": "Augment name",
            "Stage": 2,
            "Tier": 1,
            "Type": "NewNormal",
            "LegendaryMasteryPointsCost": 42,
            "Abilities": []
        })";

        auto maybe_augment = silent_loader->ParseAndLoadAugment(json_text);
        ASSERT_FALSE(maybe_augment.has_value());
    }
}

TEST_F(AugmentTestJSON, GreaterPower)
{
    // Regular valid scenario
    {
        const char* augment_data_json_raw = R"( {
            "Name": "Augment name",
            "Stage": 2,
            "Tier": 1,
            "Type": "GreaterPower",
            "LegendaryMasteryPointsCost": 42,
            "Abilities": [{
                "Name": "sdasda",
                "TotalDurationMs": 0,
                "ActivationTrigger": {
                    "TriggerType": "OnDamage",
                    "SenderAllegiance": "Enemy",
                    "ReceiverAllegiance": "Self",
                    "AbilityTypes": ["Omega"]
                },
                "Skills": [
                    {
                        "Name": "After Omega",
                        "Deployment": {
                            "Type": "Direct"
                        },
                        "Targeting": {
                            "Type": "Self"
                        },
                        "EffectPackage": {
                            "Effects": [
                                {
                                    "Type": "Buff",
                                    "Stat": "OmegaPowerPercentage",
                                    "DurationMs": -1,
                                    "Expression": 10
                                }
                            ]
                        }
                    }
                ]
            }]
        })";

        auto maybe_augment_data = loader->ParseAndLoadAugment(augment_data_json_raw);
        ASSERT_TRUE(maybe_augment_data.has_value());

        const AugmentData& data = *maybe_augment_data;
        EXPECT_EQ(data.tier, 1);

        auto& type_id = data.type_id;
        EXPECT_EQ(type_id.name, "Augment name");
        EXPECT_EQ(type_id.stage, 2);

        // Check abilities
        {
            ASSERT_EQ(data.innate_abilities.size(), 1);

            const auto& ability_data = *data.innate_abilities.at(0);
            ASSERT_EQ(ability_data.name, "sdasda");

            // Check skills
            ASSERT_EQ(ability_data.skills.size(), 1);
            const auto& skill_data = *ability_data.skills.at(0);

            ASSERT_EQ(skill_data.name, "After Omega");
            ASSERT_EQ(skill_data.deployment.type, SkillDeploymentType::kDirect);
            ASSERT_EQ(skill_data.targeting.type, SkillTargetingType::kSelf);

            // Check effects
            ASSERT_EQ(skill_data.effect_package.effects.size(), 1);

            const auto& first_effect_data = skill_data.effect_package.effects.at(0);
            EXPECT_EQ(first_effect_data.type_id.type, EffectType::kBuff);
            EXPECT_EQ(first_effect_data.type_id.stat_type, StatType::kOmegaPowerPercentage);

            // Should be different depending on the current threshold value
            EXPECT_EQ(first_effect_data.lifetime.duration_time_ms, -1);
            EXPECT_EQ(first_effect_data.GetExpression().base_value.value, 10_fp);

            const auto& trigger = ability_data.activation_trigger_data;
            ASSERT_EQ(trigger.trigger_type, ActivationTriggerType::kOnDamage);
            ASSERT_EQ(trigger.sender_allegiance, AllegianceType::kEnemy);
            ASSERT_EQ(trigger.receiver_allegiance, AllegianceType::kSelf);
            ASSERT_EQ(trigger.ability_types, MakeEnumSet(AbilityType::kOmega));
        }
    }

    // Wrong: Contains shared activation trigger
    {
        const char* json_text = R"( {
            "Name": "Augment name",
            "Stage": 2,
            "Tier": 1,
            "Type": "GreaterPower",
            "LegendaryMasteryPointsCost": 42,
            "ActivationTrigger": {
                    "TriggerType": "OnDamage",
                    "SenderAllegiance": "Enemy",
                    "ReceiverAllegiance": "Self",
                    "AbilityTypes": ["Omega"]
                },
            "Abilities": [{
                "Name": "sdasda",
                "TotalDurationMs": 0,
                "ActivationTrigger": {
                    "TriggerType": "OnDamage",
                    "SenderAllegiance": "Enemy",
                    "ReceiverAllegiance": "Self",
                    "AbilityTypes": ["Omega"]
                },
                "Skills": [
                    {
                        "Name": "After Omega",
                        "Deployment": {
                            "Type": "Direct"
                        },
                        "Targeting": {
                            "Type": "Self"
                        },
                        "EffectPackage": {
                            "Effects": [
                                {
                                    "Type": "Buff",
                                    "Stat": "OmegaPowerPercentage",
                                    "DurationMs": -1,
                                    "Expression": 10
                                }
                            ]
                        }
                    }
                ]
            }]
        })";

        auto maybe_augment = silent_loader->ParseAndLoadAugment(json_text);
        ASSERT_FALSE(maybe_augment.has_value());
    }

    // Wrong: No abilities
    {
        const char* json_text = R"( {
            "Name": "Augment name",
            "Stage": 2,
            "Tier": 1,
            "Type": "GreaterPower",
            "LegendaryMasteryPointsCost": 42,
            "Abilities": []
        })";

        auto maybe_augment = silent_loader->ParseAndLoadAugment(json_text);
        ASSERT_FALSE(maybe_augment.has_value());
    }

    // Wrong: No mastery points cost
    {
        const char* json_text = R"( {
            "Name": "Augment name",
            "Stage": 2,
            "Tier": 1,
            "Type": "GreaterPower",
            "ActivationTrigger": {
                    "TriggerType": "OnDamage",
                    "SenderAllegiance": "Enemy",
                    "ReceiverAllegiance": "Self",
                    "AbilityTypes": ["Omega"]
                },
            "Abilities": [{
                "Name": "sdasda",
                "TotalDurationMs": 0,
                "ActivationTrigger": {
                    "TriggerType": "OnDamage",
                    "SenderAllegiance": "Enemy",
                    "ReceiverAllegiance": "Self",
                    "AbilityTypes": ["Omega"]
                },
                "Skills": [
                    {
                        "Name": "After Omega",
                        "Deployment": {
                            "Type": "Direct"
                        },
                        "Targeting": {
                            "Type": "Self"
                        },
                        "EffectPackage": {
                            "Effects": [
                                {
                                    "Type": "Buff",
                                    "Stat": "OmegaPowerPercentage",
                                    "DurationMs": -1,
                                    "Expression": 10
                                }
                            ]
                        }
                    }
                ]
            }]
        })";

        auto maybe_augment = silent_loader->ParseAndLoadAugment(json_text);
        ASSERT_FALSE(maybe_augment.has_value());
    }
}

TEST_F(AugmentTestJSON, SynergyBonus)
{
    // Simple case
    {
        const char* json_text = R"({
            "Name": "Test",
            "Stage": 1,
            "Tier": 2,
            "Type": "SynergyBonus",
            "LegendaryMasteryPointsCost": 88,
            "CombatAffinities": {
                "Air": 1
            },
            "CombatClasses": {
                "Fighter": 1
            }
        })";

        auto maybe_augment = loader->ParseAndLoadAugment(json_text);
        ASSERT_TRUE(maybe_augment.has_value());

        const AugmentData& augment = *maybe_augment;

        ASSERT_EQ(augment.combat_affinities.size(), 1);
        ASSERT_EQ(augment.combat_affinities.count(CombatAffinity::kAir), 1);
        EXPECT_EQ(augment.combat_affinities.at(CombatAffinity::kAir), 1);

        ASSERT_EQ(augment.combat_classes.size(), 1);
        ASSERT_EQ(augment.combat_classes.count(CombatClass::kFighter), 1);
        EXPECT_EQ(augment.combat_classes.at(CombatClass::kFighter), 1);
    }

    // Ensure that we can't put synergies into normal augment
    {
        const char* json_text = R"({
            "Name": "Test",
            "Stage": 1,
            "Tier": 2,
            "Type": "Normal",
            "LegendaryMasteryPointsCost": 88,
            "CombatAffinities": {
                "Air": 1
            },
            "CombatClasses": {
                "Fighter": 1
            }
        })";

        auto maybe_augment = silent_loader->ParseAndLoadAugment(json_text);
        ASSERT_FALSE(maybe_augment.has_value());
    }

    // Synergy bonus augments must not have abilities
    {
        const char* json_text = R"({
            "Name": "Test",
            "Stage": 1,
            "Tier": 2,
            "Type": "SynergyBonus",
            "LegendaryMasteryPointsCost": 88,
            "CombatAffinities": {
                "Air": 1
            },
            "CombatClasses": {
                "Fighter": 1
            },
            "Abilities": []
        })";

        auto maybe_augment = silent_loader->ParseAndLoadAugment(json_text);
        ASSERT_FALSE(maybe_augment.has_value());
    }

    {
        const char* json_text = R"({
            "Name": "Test",
            "Stage": 1,
            "Tier": 2,
            "Type": "SynergyBonus",
            "CombatAffinities": {
                "Air": 1
            },
            "CombatClasses": {
                "Fighter": 1
            }
        })";

        auto maybe_augment = silent_loader->ParseAndLoadAugment(json_text);
        ASSERT_TRUE(maybe_augment.has_value());
    }
}

TEST_F(AugmentTestJSON, AugmentInstanceData)
{
    // Regular
    {
        const char* json_text = R"({
            "ID": "123",
            "TypeID": {
                "Name": "Sus",
                "Stage": 2
            },
            "SelectedAbilityIndex": 42
        })";

        auto maybe_value = loader->ParseAndLoadAugmentInstance(json_text);
        ASSERT_TRUE(maybe_value.has_value());

        auto& value = *maybe_value;
        ASSERT_EQ(value.id, "123");
        ASSERT_EQ(value.type_id.name, "Sus");
        ASSERT_EQ(value.type_id.stage, 2);
    }

    // Selected ability index is optional
    {
        const char* json_text = R"({
            "ID": "123",
            "TypeID": {
                "Name": "Sus",
                "Stage": 2
            }
        })";

        auto maybe_value = loader->ParseAndLoadAugmentInstance(json_text);
        ASSERT_TRUE(maybe_value.has_value());

        auto& value = *maybe_value;
        ASSERT_EQ(value.id, "123");
        ASSERT_EQ(value.type_id.name, "Sus");
        ASSERT_EQ(value.type_id.stage, 2);
    }
}

TEST_F(AugmentTestJSON, AugmentTypeID)
{
    // Regular
    {
        const char* json_text = R"({
            "Name": "Sus",
            "Stage": 2,
            "Variation": "Something"
        })";

        auto maybe_data = loader->ParseAndLoadAugmentTypeID(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.name, "Sus");
        ASSERT_EQ(data.stage, 2);
        ASSERT_EQ(data.variation, "Something");
    }

    // Variation can be empty
    {
        const char* json_text = R"({
            "Name": "Sus",
            "Stage": 2,
            "Variation": ""
        })";

        auto maybe_data = loader->ParseAndLoadAugmentTypeID(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.name, "Sus");
        ASSERT_EQ(data.stage, 2);
        ASSERT_EQ(data.variation, "");
    }

    // Variation is optional and empty by default
    {
        const char* json_text = R"({
            "Name": "Sus",
            "Stage": 2
        })";

        auto maybe_data = loader->ParseAndLoadAugmentTypeID(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.name, "Sus");
        ASSERT_EQ(data.stage, 2);
        ASSERT_EQ(data.variation, "");
    }
}

TEST_F(AugmentTestJSON, AugmentsConfig)
{
    // Regular
    {
        constexpr std::string_view json_text = R"({
            "MaxAugments": 42
        })";

        auto maybe_data = loader->ParseAndLoadAugmentsConfig(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.max_augments, 42);
        ASSERT_EQ(data.max_drone_augments, 3);
    }

    // Regular
    {
        constexpr std::string_view json_text = R"({
            "MaxAugments": 42,
            "MaxDroneAugments": 4
        })";

        auto maybe_data = loader->ParseAndLoadAugmentsConfig(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.max_augments, 42);
        ASSERT_EQ(data.max_drone_augments, 4);
    }

    // Negative count
    {
        constexpr std::string_view json_text = R"({
            "MaxAugments": -2
        })";

        auto maybe_data = silent_loader->ParseAndLoadAugmentsConfig(json_text);
        ASSERT_FALSE(maybe_data.has_value());
    }

    // Missing "MaxAugments" property
    {
        constexpr std::string_view json_text = R"({
        })";

        auto maybe_data = silent_loader->ParseAndLoadAugmentsConfig(json_text);
        ASSERT_FALSE(maybe_data.has_value());
    }
}

}  // namespace simulation
