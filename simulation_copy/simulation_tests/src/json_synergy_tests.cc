#include "base_test_fixtures.h"
#include "data/loaders/base_data_loader.h"
#include "gtest/gtest.h"

namespace simulation
{
class JSONSynergyTest : public BaseTest
{
    bool IsEnabledLogsForCalculateLiveStats() const override
    {
        return false;
    }
};

TEST_F(JSONSynergyTest, LoadCombatSynergy)
{
    const char* synergy_json_raw = R"(
{
    "CombatClass": "Berserker",
    "CombatClassComponents": [
        "Fighter",
        "Fighter"
    ],
    "SynergyThresholds": [
        2,
        3,
        4
    ],
    "SynergyThresholdsAbilities": {},
    "IntrinsicAbilities": []
}
    )";

    SynergyData data;
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadSynergyData(nlohmann::json::parse(synergy_json_raw), &data);

    EXPECT_EQ(data.GetCombatClass(), CombatClass::kBerserker);
    const std::vector<CombatClass> expected_components{CombatClass::kFighter, CombatClass::kFighter};
    EXPECT_EQ(data.GetComponentsAsCombatClasses(), expected_components);
    EXPECT_TRUE(data.IsCombatClass());
    EXPECT_FALSE(data.IsCombatAffinity());
    EXPECT_FALSE(data.combat_synergy.IsEmpty());
}

TEST_F(JSONSynergyTest, LoadAffinitySynergy)
{
    const char* synergy_json_raw = R"(
{
    "CombatAffinity": "Water",
    "CombatAffinityComponents": [],
    "SynergyThresholds": [
        3,
        6,
        9
    ],
    "SynergyThresholdsAbilities": {
        "Variables": [
            {
                "Name": "x",
                "Values": [3, 7, 12]
            },
            {
                "Name": "y",
                "Values": [5000, 7500, 10000]
            }
        ],
        "UnitAbilities": [
            {
                "Name": "Water Illuvials gain additional Energy Regen.",
                "ActivationTrigger": {
                    "TriggerType": "OnBattleStart"
                },
                "TotalDurationMs": 0,
                "Skills": [
                    {
                        "Name": "Skill Name",
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
                                    "DurationMs": "{y}",
                                    "Stat": "EnergyRegeneration",
                                    "Expression": "{x}"
                                }
                            ]
                        }
                    }
                ]
            }
        ],
        "TeamAbilities": []
    },
    "DisableIntrinsicAbilitiesOnFirstThreshold": true,
    "IntrinsicAbilities": [
        {
            "Name": "SynergyCount Ability Name",
            "ActivationTrigger": {
                "TriggerType": "OnBattleStart"
            },
            "TotalDurationMs": 0,
                "Skills": [
                    {
                        "Name": "SynergyCount Skill Name",
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
                                    "Stat": "EnergyRegeneration",
                                    "DurationMs": -1,
                                    "Expression": {
                                        "Operation": "*",
                                        "Operands": [
                                            2,
                                            {
                                                "Type": "SynergyCount",
                                                "CombatAffinity": "Water"
                                            },
                                            {
                                                "Type": "SynergyCount",
                                                "CombatClass": "Fighter",
                                                "StatSource": "Receiver"
                                            }
                                        ]
                                    }
                                }
                            ]
                        }
                    }
                ]
        }
    ]
}
    )";

    SynergyData data;
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadSynergyData(nlohmann::json::parse(synergy_json_raw), &data);

    EXPECT_EQ(data.GetCombatAffinity(), CombatAffinity::kWater);
    const std::vector<CombatAffinity> expected_components;
    EXPECT_EQ(data.GetComponentsAsCombatAffinities(), expected_components);
    EXPECT_FALSE(data.IsCombatClass());
    EXPECT_TRUE(data.IsCombatAffinity());
    EXPECT_FALSE(data.combat_synergy.IsEmpty());

    EXPECT_TRUE(data.disable_intrinsic_abilities_on_first_threshold);
    ASSERT_EQ(data.synergy_thresholds.size(), 3);
    EXPECT_EQ(data.synergy_thresholds[0], 3);
    EXPECT_EQ(data.synergy_thresholds[1], 6);
    EXPECT_EQ(data.synergy_thresholds[2], 9);

    // An ability for each treshold should have been createad
    ASSERT_EQ(data.unit_threshold_abilities.size(), 3);

    // Ensure each ability has the expected values
    const std::array<int, 3> expected_duration_ms{5000, 7500, 10000};
    const std::array<FixedPoint, 3> expected_expression_value{3_fp, 7_fp, 12_fp};
    for (size_t i = 0; i < data.synergy_thresholds.size(); i++)
    {
        const int threshold_value = data.synergy_thresholds[i];

        // Should exist in the ability
        ASSERT_TRUE(data.unit_threshold_abilities.count(threshold_value) > 0);

        const std::vector<std::shared_ptr<const AbilityData>>& abilities_data =
            data.unit_threshold_abilities.at(threshold_value);

        ASSERT_EQ(abilities_data.size(), 1);

        // Check ability properties
        const auto& ability_data = *abilities_data.at(0);
        EXPECT_EQ(ability_data.name, "Water Illuvials gain additional Energy Regen.");
        EXPECT_EQ(ability_data.activation_trigger_data.trigger_type, ActivationTriggerType::kOnBattleStart);
        EXPECT_EQ(ability_data.total_duration_ms, 0);

        // Check skill properties
        ASSERT_EQ(ability_data.skills.size(), 1);
        const auto& skill_data = *ability_data.skills.at(0);
        EXPECT_EQ(skill_data.name, "Skill Name");
        EXPECT_EQ(skill_data.deployment.type, SkillDeploymentType::kDirect);
        EXPECT_EQ(skill_data.targeting.type, SkillTargetingType::kSelf);

        // Check effects
        ASSERT_EQ(skill_data.effect_package.effects.size(), 1);
        const auto& first_effect_data = skill_data.effect_package.effects.at(0);
        EXPECT_EQ(first_effect_data.type_id.type, EffectType::kBuff);
        EXPECT_EQ(first_effect_data.type_id.stat_type, StatType::kEnergyRegeneration);

        // Should be different depending on the current threshold value
        EXPECT_EQ(first_effect_data.lifetime.duration_time_ms, expected_duration_ms[i]);
        EXPECT_EQ(first_effect_data.GetExpression().base_value.value, expected_expression_value[i]);
    }

    // Check IntrinsicAbilities
    ASSERT_EQ(data.intrinsic_abilities.size(), 1);

    {
        const auto& ability_data = *data.intrinsic_abilities.at(0);
        EXPECT_EQ(ability_data.name, "SynergyCount Ability Name");
        EXPECT_EQ(ability_data.activation_trigger_data.trigger_type, ActivationTriggerType::kOnBattleStart);
        EXPECT_EQ(ability_data.total_duration_ms, 0);

        // Check skill properties
        ASSERT_EQ(ability_data.skills.size(), 1);
        const auto& skill_data = *ability_data.skills.at(0);
        EXPECT_EQ(skill_data.name, "SynergyCount Skill Name");
        EXPECT_EQ(skill_data.deployment.type, SkillDeploymentType::kDirect);
        EXPECT_EQ(skill_data.targeting.type, SkillTargetingType::kSelf);

        // Check effects
        ASSERT_EQ(skill_data.effect_package.effects.size(), 1);
        const auto& first_effect_data = skill_data.effect_package.effects.at(0);
        EXPECT_EQ(first_effect_data.type_id.type, EffectType::kBuff);
        EXPECT_EQ(first_effect_data.type_id.stat_type, StatType::kEnergyRegeneration);
        EXPECT_EQ(first_effect_data.lifetime.duration_time_ms, kTimeInfinite);

        // Check Expression
        const auto& first_effect_expression = first_effect_data.GetExpression();
        EXPECT_EQ(first_effect_expression.operation_type, EffectOperationType::kMultiply);
        ASSERT_EQ(first_effect_expression.operands.size(), 3);

        EXPECT_EQ(first_effect_expression.operands[0].base_value.value, 2_fp);
        EXPECT_EQ(first_effect_expression.operands[0].base_value.type, EffectValueType::kValue);

        EXPECT_EQ(first_effect_expression.operands[1].base_value.type, EffectValueType::kSynergyCount);
        EXPECT_EQ(first_effect_expression.operands[1].base_value.combat_synergy, CombatAffinity::kWater);
        EXPECT_EQ(first_effect_expression.operands[1].base_value.data_source_type, ExpressionDataSourceType::kSender);

        EXPECT_EQ(first_effect_expression.operands[2].base_value.type, EffectValueType::kSynergyCount);
        EXPECT_EQ(first_effect_expression.operands[2].base_value.combat_synergy, CombatClass::kFighter);
        EXPECT_EQ(first_effect_expression.operands[2].base_value.data_source_type, ExpressionDataSourceType::kReceiver);
    }
}

}  // namespace simulation
