#include "base_json_test.h"

namespace simulation
{

class OverloadConfigTestJSON : public BaseJSONTest
{
public:
    using Super = BaseJSONTest;
};

TEST_F(OverloadConfigTestJSON, OverloadConfig)
{
    // Regular
    {
        constexpr std::string_view json_text = R"({
            "EnableOverloadSystem": true,
            "IncreaseOverloadDamagePercentage": 42,
            "StartSecondsApplyOverloadDamage": 24
        })";

        auto maybe_data = loader->ParseAndLoadOverloadConfig(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.enable_overload_system, true);
        ASSERT_EQ(data.increase_overload_damage_percentage, 42_fp);
        ASSERT_EQ(data.start_seconds_apply_overload_damage, 24);
    }

    // Missing "EnableOverloadSystem" property
    {
        constexpr std::string_view json_text = R"({
            "IncreaseOverloadDamagePercentage": 42,
            "StartSecondsApplyOverloadDamage": 24
        })";
        auto maybe_data = silent_loader->ParseAndLoadOverloadConfig(json_text);
        ASSERT_FALSE(maybe_data.has_value());
    }

    // Missing "IncreaseOverloadDamagePercentage" property
    {
        constexpr std::string_view json_text = R"({
            "EnableOverloadSystem": true,
            "StartSecondsApplyOverloadDamage": 24
        })";
        auto maybe_data = silent_loader->ParseAndLoadOverloadConfig(json_text);
        ASSERT_FALSE(maybe_data.has_value());
    }

    // Missing "StartSecondsApplyOverloadDamage" property
    {
        constexpr std::string_view json_text = R"({
            "EnableOverloadSystem": true,
            "IncreaseOverloadDamagePercentage": 42
        })";
        auto maybe_data = silent_loader->ParseAndLoadOverloadConfig(json_text);
        ASSERT_FALSE(maybe_data.has_value());
    }

    // Do not require all files if system is disabled
    {
        constexpr std::string_view json_text = R"({
            "EnableOverloadSystem": false
        })";

        auto maybe_data = loader->ParseAndLoadOverloadConfig(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.enable_overload_system, false);
    }
}

}  // namespace simulation
