#include "base_json_test.h"
#include "data/encounter_mod_type_id.h"
#include "data/enums.h"
#include "data/loaders/base_data_loader.h"

namespace simulation
{

class EncounterModJSONTests : public BaseJSONTest
{
public:
    using Super = BaseJSONTest;

protected:
    static constexpr std::string_view kPlaceholderAbilitiesArray = R"([{
        "Name": "Gain 10% of their Max Energy and Cleanse all of their negative effects.",
        "ActivationTrigger": {
            "TriggerType": "OnActivateXAbilities",
            "AbilityTypes": ["Omega"],
            "NumberOfAbilitiesActivated": 1
        },
        "TotalDurationMs": 0,
        "Skills": [{
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
        }]
    }])";
};

TEST_F(EncounterModJSONTests, LoadEncounterModTypeID)
{
    // Regualr case
    {
        constexpr std::string_view json_text = R"({
            "Name": "EncounterMod",
            "Stage": 2
        })";

        auto maybe_type_id = loader->ParseAndLoadEncounterModTypeID(json_text);
        ASSERT_TRUE(maybe_type_id.has_value());

        const EncounterModTypeID& type_id = *maybe_type_id;
        ASSERT_EQ(type_id.name, "EncounterMod");
        ASSERT_EQ(type_id.stage, 2);
    }

    // Name is empty. Must not fail
    {
        constexpr std::string_view json_text = R"({
            "Name": "",
            "Stage": 0
        })";

        auto maybe_type_id = loader->ParseAndLoadEncounterModTypeID(json_text);
        ASSERT_TRUE(maybe_type_id.has_value());

        auto& type_id = *maybe_type_id;

        // But id itself is not valid
        ASSERT_FALSE(type_id.IsValid());
    }

    // Error: negative stage
    {
        constexpr std::string_view json_text = R"({
            "Name": "EncounterMod",
            "Stage": -2
        })";

        auto maybe_type_id = silent_loader->ParseAndLoadEncounterModTypeID(json_text);
        ASSERT_FALSE(maybe_type_id.has_value());
    }
}

TEST_F(EncounterModJSONTests, LoadEncounterModData)
{
    // Normal case
    {
        const std::string json_text = fmt::format(
            R"({{
            "Name": "EncounterMod",
            "Stage": 2,
            "Abilities": {}
        }})",
            kPlaceholderAbilitiesArray);

        auto maybe_data = loader->ParseAndLoadEncounterModData(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        // ensure that ability is there. Testing ability (or encounter mod type id) loading is a purpose of another test
        auto& data = *maybe_data;
        ASSERT_EQ(data.innate_abilities.size(), 1);
    }
}

TEST_F(EncounterModJSONTests, LoadTeamsEncounterMods)
{
    // Two teams
    {
        constexpr std::string_view json_text = R"({
            "Red": [{
                "TypeID": {
                    "Name": "EncounterModRed",
                    "Stage": 1
                }
            }],
            "Blue": [{
                "TypeID": {
                    "Name": "EncounterModBlue",
                    "Stage": 2
                }
            }]
        })";

        auto maybe_data = loader->ParseAndLoadTeamsEncounterMods(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.size(), 2);
        ASSERT_EQ(data.count(Team::kRed), 1);
        auto& red_instances = data[Team::kRed];
        ASSERT_EQ(red_instances.size(), 1);
        ASSERT_EQ(red_instances[0].type_id.name, "EncounterModRed");
        ASSERT_EQ(red_instances[0].type_id.stage, 1);
        ASSERT_EQ(data.count(Team::kBlue), 1);
        auto& blue_instances = data[Team::kBlue];
        ASSERT_EQ(blue_instances.size(), 1);
        ASSERT_EQ(blue_instances[0].type_id.name, "EncounterModBlue");
        ASSERT_EQ(blue_instances[0].type_id.stage, 2);
    }

    // Two teams unreal format
    {
        constexpr std::string_view json_text = R"({
    "Red": {
        "Instances": [
            {
                "TypeID": {
                    "Name":"EncounterModRed",
                    "Stage":1
                }
            }
        ]
    },
    "Blue": {
        "Instances": [
            {
                "TypeID": {
                    "Name":"EncounterModBlue",
                    "Stage":2
                }
            }
        ]
    }
})";
        auto maybe_data = loader->ParseAndLoadTeamsEncounterMods(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.size(), 2);
        ASSERT_EQ(data.count(Team::kRed), 1);
        auto& red_instances = data[Team::kRed];
        ASSERT_EQ(red_instances.size(), 1);
        ASSERT_EQ(red_instances[0].type_id.name, "EncounterModRed");
        ASSERT_EQ(red_instances[0].type_id.stage, 1);
        ASSERT_EQ(data.count(Team::kBlue), 1);
        auto& blue_instances = data[Team::kBlue];
        ASSERT_EQ(blue_instances.size(), 1);
        ASSERT_EQ(blue_instances[0].type_id.name, "EncounterModBlue");
        ASSERT_EQ(blue_instances[0].type_id.stage, 2);
    }

    // Only Red
    {
        constexpr std::string_view json_text = R"({
            "Red": [{
                "ID": "ijkfebvnekj",
                "TypeID": {
                    "Name": "EncounterModRed",
                    "Stage": 1
                }
            }]
        })";

        auto maybe_data = loader->ParseAndLoadTeamsEncounterMods(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.size(), 1);
        ASSERT_EQ(data.count(Team::kRed), 1);
        auto& red_instances = data[Team::kRed];
        ASSERT_EQ(red_instances.size(), 1);
        ASSERT_EQ(red_instances[0].type_id.name, "EncounterModRed");
        ASSERT_EQ(red_instances[0].type_id.stage, 1);
    }

    // Only Blue
    {
        constexpr std::string_view json_text = R"({
            "Blue": [{
                "TypeID": {
                    "Name": "EncounterModBlue",
                    "Stage": 2
                }
            }]
        })";

        auto maybe_data = loader->ParseAndLoadTeamsEncounterMods(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.count(Team::kBlue), 1);
        auto& blue_instances = data[Team::kBlue];
        ASSERT_EQ(blue_instances.size(), 1);
        ASSERT_EQ(blue_instances[0].type_id.name, "EncounterModBlue");
        ASSERT_EQ(blue_instances[0].type_id.stage, 2);
    }

    // Empty object
    {
        constexpr std::string_view json_text = R"({})";

        auto maybe_data = loader->ParseAndLoadTeamsEncounterMods(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.size(), 0);
    }

    // Invalid key in object
    {
        constexpr std::string_view json_text = R"({
            "Blues": [{
                "TypeID": {
                    "Name": "EncounterModBlue",
                    "Stage": 2
                }
            }]
        })";

        auto maybe_data = silent_loader->ParseAndLoadTeamsEncounterMods(json_text);
        ASSERT_FALSE(maybe_data.has_value());
    }
}

}  // namespace simulation
