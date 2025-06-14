#include "base_json_test.h"

namespace simulation
{
class JSONTriggerTest : public BaseJSONTest
{
public:
};

TEST_F(JSONTriggerTest, LoadOnDeployXSkills)
{
    // Ability type is required
    EXPECT_FALSE(silent_loader->IsValidTrigger(R"({
        "TriggerType": "OnDeployXSkills"
    })"));

    // Default value for NumberOfAbilitiesDeployed
    {
        const char* json_text = R"({
            "TriggerType": "OnDeployXSkills",
            "AbilityTypes": ["Attack", "Omega", "Innate"]
        })";

        auto maybe_trigger = loader->ParseAndLoadTrigger(json_text);
        EXPECT_TRUE(maybe_trigger.has_value());

        auto& trigger_data = *maybe_trigger;
        EXPECT_EQ(trigger_data.trigger_type, ActivationTriggerType::kOnDeployXSkills);
        EXPECT_EQ(trigger_data.ability_types, EnumSet<AbilityType>::MakeFull());
        EXPECT_EQ(trigger_data.number_of_skills_deployed, 1);
    }

    // Common case
    {
        const char* json_text = R"({
            "TriggerType": "OnDeployXSkills",
            "AbilityTypes": ["Attack"],
            "NumberOfSkillsDeployed": 42
        })";

        auto maybe_trigger = loader->ParseAndLoadTrigger(json_text);
        EXPECT_TRUE(maybe_trigger.has_value());

        auto& trigger_data = *maybe_trigger;
        EXPECT_EQ(trigger_data.trigger_type, ActivationTriggerType::kOnDeployXSkills);
        EXPECT_EQ(trigger_data.ability_types, MakeEnumSet(AbilityType::kAttack));
        EXPECT_EQ(trigger_data.number_of_skills_deployed, 42);
    }
}

TEST_F(JSONTriggerTest, LoadOnVanquishTrigger)
{
    // Should fail because allegiance is required
    EXPECT_FALSE(silent_loader->IsValidTrigger(R"({
        "TriggerType": "OnVanquish"
    })"));

    // Should fail because ComparisonType and EveryX are mutually exclusive
    EXPECT_FALSE(silent_loader->IsValidTrigger(R"({
        "TriggerType": "OnVanquish",
        "Allegiance": "Enemy",
        "EveryX": true,
        "ComparisonType": "<",
        "TriggerValue": 5
    })"));

    // Check defaults
    {
        const char* json_text = R"({
            "TriggerType": "OnVanquish",
            "Allegiance": "Enemy"
        })";

        auto maybe_trigger = loader->ParseAndLoadTrigger(json_text);
        EXPECT_TRUE(maybe_trigger.has_value());

        auto& trigger_data = *maybe_trigger;
        EXPECT_EQ(trigger_data.trigger_type, ActivationTriggerType::kOnVanquish);
        EXPECT_EQ(trigger_data.sender_allegiance, AllegianceType::kEnemy);
        EXPECT_EQ(trigger_data.every_x, true);
        EXPECT_EQ(trigger_data.trigger_value, 1);
        EXPECT_FALSE(trigger_data.only_from_parent);
    }

    // every 2
    {
        const char* json_text = R"({
            "TriggerType": "OnVanquish",
            "Allegiance": "Enemy",
            "EveryX": true,
            "TriggerValue": 2,
            "OnlyFromParent": true
        })";

        auto maybe_trigger = loader->ParseAndLoadTrigger(json_text);
        EXPECT_TRUE(maybe_trigger.has_value());

        auto& trigger = *maybe_trigger;
        EXPECT_EQ(trigger.trigger_type, ActivationTriggerType::kOnVanquish);
        EXPECT_EQ(trigger.sender_allegiance, AllegianceType::kEnemy);
        EXPECT_EQ(trigger.every_x, true);
        EXPECT_EQ(trigger.trigger_value, 2);
        EXPECT_TRUE(trigger.only_from_parent);
    }

    // every 2 (without "EveryX")
    {
        const char* json_text = R"({
            "TriggerType": "OnVanquish",
            "Allegiance": "Enemy",
            "TriggerValue": 2
        })";

        auto maybe_trigger = loader->ParseAndLoadTrigger(json_text);
        EXPECT_TRUE(maybe_trigger.has_value());

        auto& trigger = *maybe_trigger;
        EXPECT_EQ(trigger.trigger_type, ActivationTriggerType::kOnVanquish);
        EXPECT_EQ(trigger.sender_allegiance, AllegianceType::kEnemy);
        EXPECT_EQ(trigger.every_x, true);
        EXPECT_EQ(trigger.trigger_value, 2);
    }

    {  // ComparisonType is specified
        const char* json_text = R"({
            "TriggerType": "OnVanquish",
            "Allegiance": "Enemy",
            "ComparisonType": "<",
            "TriggerValue": 5
        })";

        auto maybe_trigger = loader->ParseAndLoadTrigger(json_text);
        EXPECT_TRUE(maybe_trigger.has_value());

        auto& trigger = *maybe_trigger;
        EXPECT_EQ(trigger.trigger_type, ActivationTriggerType::kOnVanquish);
        EXPECT_EQ(trigger.sender_allegiance, AllegianceType::kEnemy);
        EXPECT_EQ(trigger.every_x, false);
        EXPECT_EQ(trigger.comparison_type, ComparisonType::kLess);
        EXPECT_EQ(trigger.trigger_value, 5);
    }
}

TEST_F(JSONTriggerTest, LoadOnReceiveXEffectPackagesTrigger)
{
    // Required keys
    EXPECT_FALSE(silent_loader->IsValidTrigger(R"({
        "TriggerType": "OnReceiveXEffectPackages"
    })"));

    {
        const char* json_text = R"({
            "TriggerType": "OnReceiveXEffectPackages",
            "AbilityTypes": ["Attack", "Omega", "Innate"],
            "ModuloValue": 5,
            "ComparisonType": "==",
            "CompareAgainst": 3
        })";

        auto maybe_trigger = loader->ParseAndLoadTrigger(json_text);
        EXPECT_TRUE(maybe_trigger.has_value());

        auto& trigger_data = *maybe_trigger;
        EXPECT_EQ(trigger_data.trigger_type, ActivationTriggerType::kOnReceiveXEffectPackages);
        EXPECT_EQ(trigger_data.ability_types, EnumSet<AbilityType>::MakeFull());
        EXPECT_EQ(trigger_data.number_of_effect_packages_received_modulo, 5);
        EXPECT_EQ(trigger_data.comparison_type, ComparisonType::kEqual);
        EXPECT_EQ(trigger_data.number_of_effect_packages_received, 3);
    }
}

TEST_F(JSONTriggerTest, LoadOnShieldHitTrigger)
{
    {
        const char* json_text = R"({
            "TriggerType": "OnShieldHit",
            "SenderAllegiance": "Enemy",
            "ReceiverAllegiance": "Ally"
        })";

        auto maybe_trigger = loader->ParseAndLoadTrigger(json_text);
        EXPECT_TRUE(maybe_trigger.has_value());

        auto& trigger_data = *maybe_trigger;
        EXPECT_EQ(trigger_data.trigger_type, ActivationTriggerType::kOnShieldHit);
        EXPECT_EQ(trigger_data.sender_allegiance, AllegianceType::kEnemy);
        EXPECT_EQ(trigger_data.receiver_allegiance, AllegianceType::kAlly);
    }
}

TEST_F(JSONTriggerTest, LoadOnFaintTrigger)
{
    {
        const char* json_text = R"({
            "TriggerType": "OnFaint",
            "Allegiance": "Enemy"
        })";

        auto maybe_trigger = loader->ParseAndLoadTrigger(json_text);
        EXPECT_TRUE(maybe_trigger.has_value());

        auto& trigger_data = *maybe_trigger;
        EXPECT_EQ(trigger_data.trigger_type, ActivationTriggerType::kOnFaint);
        EXPECT_EQ(trigger_data.sender_allegiance, AllegianceType::kEnemy);
        EXPECT_EQ(trigger_data.receiver_allegiance, AllegianceType::kAll);
    }

    {
        const char* json_text = R"({
            "TriggerType": "OnFaint",
            "Allegiance": "Ally"
        })";

        auto maybe_trigger = loader->ParseAndLoadTrigger(json_text);
        EXPECT_TRUE(maybe_trigger.has_value());

        auto& trigger_data = *maybe_trigger;
        EXPECT_EQ(trigger_data.trigger_type, ActivationTriggerType::kOnFaint);
        EXPECT_EQ(trigger_data.sender_allegiance, AllegianceType::kAlly);
        EXPECT_EQ(trigger_data.receiver_allegiance, AllegianceType::kAll);
    }

    {
        const char* json_text = R"({
            "TriggerType": "OnFaint",
            "Allegiance": "Self"
        })";

        auto maybe_trigger = loader->ParseAndLoadTrigger(json_text);
        EXPECT_TRUE(maybe_trigger.has_value());

        auto& trigger_data = *maybe_trigger;
        EXPECT_EQ(trigger_data.trigger_type, ActivationTriggerType::kOnFaint);
        EXPECT_EQ(trigger_data.sender_allegiance, AllegianceType::kSelf);
        EXPECT_EQ(trigger_data.receiver_allegiance, AllegianceType::kAll);
    }

    // Must because both allegiances must be specified
    EXPECT_FALSE(silent_loader->IsValidTrigger(R"({
        "TriggerType": "OnFaint"
    })"));
}
}  // namespace simulation
