#include <optional>

#include "base_json_test.h"
#include "test_data_loader.h"

namespace simulation
{

class SuitDataTestJSON : public BaseJSONTest
{
public:
    static std::string AddTestAbility(const std::string_view format)
    {
        return FindAndReplace(format, "+++", kPlaceholderSuitAbility);
    }

    static constexpr std::string_view kPlaceholderSuitAbility = R"({
    "Name": "InfernoSynergy_Empower",
    "Description": "Some ability",
    "ActivationTrigger": {
        "TriggerType": "OnBattleStart"
    },
    "TotalDurationMs": 0,
    "Skills": [
        {
            "Name": "Precision Percentage (Crit Chance) buff",
            "Deployment": { "Type": "Direct" },
            "Targeting": { "Type": "Self" },
            "EffectPackage": {
                "Effects": [{
                    "Type": "InstantHeal",
                    "HealType": "Normal",
                    "Expression": 10
                }]
            }
        }
    ]
})";
};

TEST_F(SuitDataTestJSON, TypeID)
{
    // Valid case
    {
        const char* weapon_json_raw = R"({
            "Name": "CoolSuit",
            "Stage": 1,
            "Variation": "Something"
        })";

        auto maybe_data = loader->ParseAndLoadSuitTypeID(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());

        const auto& type_id = *maybe_data;
        EXPECT_EQ(type_id.name, "CoolSuit");
        EXPECT_EQ(type_id.stage, 1);
        EXPECT_EQ(type_id.variation, "Something");
    }

    // Name must be present
    {
        const char* weapon_json_raw = R"({
            "Stage": 1,
            "Variation": "Something"
        })";

        auto maybe_data = silent_loader->ParseAndLoadSuitTypeID(weapon_json_raw);
        ASSERT_FALSE(maybe_data.has_value());
    }

    // Stage must be present
    {
        const char* weapon_json_raw = R"({
            "Name": "CoolSuit",
            "Variation": "Something"
        })";

        auto maybe_data = silent_loader->ParseAndLoadSuitTypeID(weapon_json_raw);
        ASSERT_FALSE(maybe_data.has_value());
    }

    // Variation can be empty
    {
        const char* weapon_json_raw = R"({
            "Name": "CoolSuit",
            "Stage": 1,
            "Variation": ""
        })";

        auto maybe_data = loader->ParseAndLoadSuitTypeID(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());

        const auto& type_id = *maybe_data;
        EXPECT_EQ(type_id.name, "CoolSuit");
        EXPECT_EQ(type_id.stage, 1);
        EXPECT_EQ(type_id.variation, "");
    }

    // Variation is optional field
    {
        const char* weapon_json_raw = R"({
            "Name": "CoolSuit",
            "Stage": 1
        })";

        auto maybe_data = loader->ParseAndLoadSuitTypeID(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());

        const auto& type_id = *maybe_data;
        EXPECT_EQ(type_id.name, "CoolSuit");
        EXPECT_EQ(type_id.stage, 1);
        EXPECT_EQ(type_id.variation, "");
    }
}

TEST_F(SuitDataTestJSON, Regular)
{
    const auto json_raw = AddTestAbility(
        R"({
        "Name": "ArcaneCuirass",
        "Stage": 1,
        "Tier": 4,
        "Variation": "Original",
        "Type": "Normal",
        "Stats": {
            "PhysicalResist": 1,
            "EnergyResist": 2,
            "MaxHealth": 3,
            "Grit": 4,
            "Resolve": 5
        },
        "Abilities": [
            +++
        ]
    })");

    const std::optional<CombatUnitSuitData> maybe_data = loader->ParseAndLoadSuitData(json_raw);
    ASSERT_TRUE(maybe_data.has_value());

    const auto& data = *maybe_data;

    // Check type id and stats values only once here.
    // They must have a separate serialization unit tests
    EXPECT_EQ(data.type_id.name, "ArcaneCuirass");
    EXPECT_EQ(data.type_id.stage, 1);
    EXPECT_EQ(data.type_id.variation, "Original");
    EXPECT_EQ(data.tier, 4);
    EXPECT_EQ(data.stats.Get(StatType::kPhysicalResist), 1_fp);
    EXPECT_EQ(data.stats.Get(StatType::kEnergyResist), 2_fp);
    EXPECT_EQ(data.stats.Get(StatType::kMaxHealth), 3_fp);
    EXPECT_EQ(data.stats.Get(StatType::kGrit), 4_fp);
    EXPECT_EQ(data.stats.Get(StatType::kResolve), 5_fp);
}

TEST_F(SuitDataTestJSON, RequireStats)
{
    const auto json_raw = AddTestAbility(
        R"({
        "Name": "ArcaneCuirass",
        "Stage": 1,
        "Tier": 4,
        "Variation": "Original",
        "Abilities": [
            +++
        ]
    })");

    ASSERT_FALSE(silent_loader->ParseAndLoadSuitData(json_raw));
}

TEST_F(SuitDataTestJSON, RequireTier)
{
    const auto json_raw = AddTestAbility(
        R"({
        "Name": "ArcaneCuirass",
        "Stage": 1,
        "Variation": "Original",
        "Stats": {
            "PhysicalResist": 1,
            "EnergyResist": 2,
            "MaxHealth": 3,
            "Grit": 4,
            "Resolve": 5
        },
        "Abilities": [
            +++
        ]
    })");

    ASSERT_FALSE(silent_loader->ParseAndLoadSuitData(json_raw));
}

TEST_F(SuitDataTestJSON, RequireTypeID)
{
    const auto json_raw = AddTestAbility(
        R"({
        "Tier": 4,
        "Stats": {
            "PhysicalResist": 1,
            "EnergyResist": 2,
            "MaxHealth": 3,
            "Grit": 4,
            "Resolve": 5
        },
        "Abilities": [
            +++
        ]
    })");

    ASSERT_FALSE(silent_loader->ParseAndLoadSuitData(json_raw));
}

TEST_F(SuitDataTestJSON, AbilitiesArrayIsOptional)
{
    const auto json_raw = AddTestAbility(
        R"({
        "Name": "ArcaneCuirass",
        "Stage": 1,
        "Tier": 4,
        "Variation": "Original",
        "Type": "Normal",
        "Stats": {
            "PhysicalResist": 1,
            "EnergyResist": 2,
            "MaxHealth": 3,
            "Grit": 4,
            "Resolve": 5
        }
    })");

    ASSERT_TRUE(loader->ParseAndLoadSuitData(json_raw));
}

TEST_F(SuitDataTestJSON, AbilitiesArrayCanBeEmpty)
{
    const auto json_raw = AddTestAbility(
        R"({
        "Name": "ArcaneCuirass",
        "Stage": 1,
        "Tier": 4,
        "Variation": "Original",
        "Type": "Normal",
        "Stats": {
            "PhysicalResist": 1,
            "EnergyResist": 2,
            "MaxHealth": 3,
            "Grit": 4,
            "Resolve": 5
        },
        "Abilities": []
    })");

    ASSERT_TRUE(loader->ParseAndLoadSuitData(json_raw));
}

TEST_F(SuitDataTestJSON, EnsureFailIfNecessaryStatMissing)
{
    static constexpr std::array<StatType, 5> necessary_stats{
        StatType::kEnergyResist,
        StatType::kPhysicalResist,
        StatType::kMaxHealth,
        StatType::kGrit,
        StatType::kResolve,
    };

    nlohmann::json json = nlohmann::json::parse(AddTestAbility(
        R"({
        "Name": "ArcaneCuirass",
        "Stage": 1,
        "Tier": 4,
        "Variation": "Original",
        "Abilities": [
            +++
        ]
    })"));

    CombatUnitSuitData suit;
    EXPECT_FALSE(silent_loader->LoadCombatSuitData(json, &suit));

    for (const StatType excluded_stat : necessary_stats)
    {
        json["Stats"] = nlohmann::json::object();
        auto& stats = json["Stats"];

        // add all stats except ith
        for (const StatType stat : necessary_stats)
        {
            if (stat != excluded_stat)
            {
                stats[Enum::StatTypeToString(stat)] = 22;
                EXPECT_FALSE(silent_loader->LoadCombatSuitData(json, &suit));
            }
        }
    }
}

}  // namespace simulation
