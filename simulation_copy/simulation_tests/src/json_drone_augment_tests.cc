#include "base_json_test.h"

namespace simulation
{
class DroneAugmentTestJSON : public BaseJSONTest
{
public:
    using Super = BaseJSONTest;
};

TEST_F(DroneAugmentTestJSON, DroneAugmentTypeID)
{
    const char* json_text = R"({
        "Name": "Sus",
        "Stage": 2
    })";

    const auto maybe_data = loader->ParseAndLoadDroneAugmentTypeID(json_text);
    ASSERT_TRUE(maybe_data.has_value());

    const auto& data = *maybe_data;
    ASSERT_EQ(data.name, "Sus");
    ASSERT_EQ(data.stage, 2);
}

TEST_F(DroneAugmentTestJSON, ValidGameAugment)
{
    const char* drone_augment_data_json_raw = R"({
            "Name": "Augment name",
            "Stage": 2,
            "Type": "Game"
        })";

    const auto maybe_drone_augment_data = loader->ParseAndLoadDroneAugment(drone_augment_data_json_raw);
    ASSERT_TRUE(maybe_drone_augment_data.has_value());

    const DroneAugmentData& data = *maybe_drone_augment_data;
    EXPECT_EQ(data.drone_augment_type, DroneAugmentType::kGame);

    const auto& type_id = data.type_id;
    EXPECT_EQ(type_id.name, "Augment name");
    EXPECT_EQ(type_id.stage, 2);

    ASSERT_EQ(data.innate_abilities.abilities.size(), 0);
}

TEST_F(DroneAugmentTestJSON, ValidBackendAndGameAugment)
{
    const char* drone_augment_data_json_raw = R"( {
            "Name": "ValidBackendAndGameAugment",
            "Stage": 3,
            "Type": "ServerAndGame"
        })";

    auto maybe_drone_augment_data = loader->ParseAndLoadDroneAugment(drone_augment_data_json_raw);
    ASSERT_TRUE(maybe_drone_augment_data.has_value());

    const DroneAugmentData& data = *maybe_drone_augment_data;
    EXPECT_EQ(data.drone_augment_type, DroneAugmentType::kServerAndGame);

    const auto& type_id = data.type_id;
    EXPECT_EQ(type_id.name, "ValidBackendAndGameAugment");
    EXPECT_EQ(type_id.stage, 3);
    ASSERT_EQ(data.innate_abilities.abilities.size(), 0);
}

TEST_F(DroneAugmentTestJSON, ValidSimulationAugment)
{
    const char* drone_augment_data_json_raw = R"( {
        "Name": "ValidSimulationAugment",
        "Stage": 2,
        "Type": "Simulation",
        "Abilities": [
            {
                "Name": "Ability Name",
                "TotalDurationMs": 0,
                "ActivationTrigger": {
                    "TriggerType": "OnBattleStart"
                },
                "Skills": [
                    {
                        "Name": "Skill Name",
                        "Deployment": {
                            "Type": "Direct",
                            "Guidance": [
                                "Ground",
                                "Underground",
                                "Airborne"
                            ]
                        },
                        "Targeting": {
                            "Type": "Synergy",
                            "Guidance": [
                                "Ground",
                                "Underground",
                                "Airborne"
                            ],
                            "Group": "Ally",
                            "CombatAffinity": "Air"
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
            }
        ]
    })";

    auto maybe_drone_augment_data = loader->ParseAndLoadDroneAugment(drone_augment_data_json_raw);
    ASSERT_TRUE(maybe_drone_augment_data.has_value());

    const DroneAugmentData& data = *maybe_drone_augment_data;
    EXPECT_EQ(data.drone_augment_type, DroneAugmentType::kSimulation);

    const auto& type_id = data.type_id;
    EXPECT_EQ(type_id.name, "ValidSimulationAugment");
    EXPECT_EQ(type_id.stage, 2);
    ASSERT_EQ(data.innate_abilities.abilities.size(), 1);

    const auto& ability_data = *data.innate_abilities.abilities.at(0);
    EXPECT_EQ(ability_data.name, "Ability Name");

    // Check trigger
    const auto& trigger = ability_data.activation_trigger_data;
    EXPECT_EQ(trigger.trigger_type, ActivationTriggerType::kOnBattleStart);

    // Check skills
    ASSERT_EQ(ability_data.skills.size(), 1);
    const auto& skill_data = *ability_data.skills.at(0);

    EXPECT_EQ(skill_data.name, "Skill Name");

    // Check deployment
    EXPECT_EQ(skill_data.deployment.type, SkillDeploymentType::kDirect);
    const auto all_guidance = MakeEnumSet(GuidanceType::kGround, GuidanceType::kAirborne, GuidanceType::kUnderground);
    EXPECT_EQ(skill_data.deployment.guidance, all_guidance);

    // Check targeting
    EXPECT_EQ(skill_data.targeting.type, SkillTargetingType::kSynergy);
    EXPECT_EQ(skill_data.targeting.guidance, all_guidance);
    EXPECT_EQ(skill_data.targeting.combat_synergy, CombatAffinity::kAir);
    EXPECT_EQ(skill_data.targeting.group, AllegianceType::kAlly);

    // Check effects
    ASSERT_EQ(skill_data.effect_package.effects.size(), 1);

    const auto& first_effect_data = skill_data.effect_package.effects.at(0);
    EXPECT_EQ(first_effect_data.type_id.type, EffectType::kBuff);
    EXPECT_EQ(first_effect_data.type_id.stat_type, StatType::kOmegaPowerPercentage);

    // Should be different depending on the current threshold value
    EXPECT_EQ(first_effect_data.lifetime.duration_time_ms, -1);
    EXPECT_EQ(first_effect_data.GetExpression().base_value.value, 10_fp);
}

}  // namespace simulation