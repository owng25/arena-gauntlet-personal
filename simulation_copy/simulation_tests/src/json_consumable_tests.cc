#include "base_json_test.h"

namespace simulation
{

class ConsumableTestJSON : public BaseJSONTest
{
public:
    using Super = BaseJSONTest;
};

TEST_F(ConsumableTestJSON, Normal)
{
    const char* consumable_json_raw = R"({
        "Name": "BasketFruit",
        "Stage": 15,
        "Tier": 3,
        "Abilities": [
            {
                "Name": "Consumable - Floraball",
                "ActivationTrigger": {
                    "TriggerType": "OnBattleStart"
                },
                "TotalDurationMs": 0,
                "Skills": [
                    {
                        "Name": "Max Health and Energy/Physical Resist buff",
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
                                    "Stat": "MaxHealth",
                                    "Expression": 400,
                                    "DurationMs": -1
                                }
                            ]
                        }
                    }
                ]
            }
        ]
    })";

    auto maybe_data = loader->ParseAndLoadConsumable(consumable_json_raw);
    ASSERT_TRUE(maybe_data.has_value());

    auto& data = *maybe_data;

    {
        EXPECT_EQ(data.tier, 3);
        const auto& type_id = data.type_id;
        EXPECT_EQ(type_id.stage, 15);
        EXPECT_EQ(type_id.name, "BasketFruit");
    }

    // Just check something was read. Test ability parsing is out of scope of this test
    EXPECT_EQ(data.innate_abilities.size(), 1);
}

TEST_F(ConsumableTestJSON, TypeID)
{
    // Valid case
    {
        const char* json_raw = R"({
            "Name": "BasketFruit",
            "Stage": 15
        })";

        auto maybe_data = loader->ParseAndLoadConsumableTypeID(json_raw);
        ASSERT_TRUE(maybe_data.has_value());

        const auto& type_id = *maybe_data;
        EXPECT_EQ(type_id.name, "BasketFruit");
        EXPECT_EQ(type_id.stage, 15);
    }

    // Name must be present
    {
        const char* json_raw = R"({
            "Stage": 15
        })";

        auto maybe_data = silent_loader->ParseAndLoadConsumableTypeID(json_raw);
        ASSERT_FALSE(maybe_data.has_value());
    }

    // Stage must be present
    {
        const char* json_raw = R"({
            "Name": "BasketFruit"
        })";

        auto maybe_data = silent_loader->ParseAndLoadConsumableTypeID(json_raw);
        ASSERT_FALSE(maybe_data.has_value());
    }
}

TEST_F(ConsumableTestJSON, ConsumablesConfig)
{
    // Regular
    {
        constexpr std::string_view json_text = R"({
            "MaxConsumables": 42
        })";

        auto maybe_data = loader->ParseAndLoadConsumablesConfig(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.max_consumables, 42);
    }

    // Missing "MaxConsumables" property
    {
        constexpr std::string_view json_text = R"({})";
        auto maybe_data = silent_loader->ParseAndLoadConsumablesConfig(json_text);
        ASSERT_FALSE(maybe_data.has_value());
    }
}

}  // namespace simulation
