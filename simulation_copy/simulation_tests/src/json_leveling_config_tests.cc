#include "base_json_test.h"

namespace simulation
{

class LevelingConfigTestJSON : public BaseJSONTest
{
public:
    using Super = BaseJSONTest;
};

TEST_F(LevelingConfigTestJSON, OverloadConfig)
{
    // All non-default
    {
        constexpr std::string_view json_text = R"({
            "EnableLevelingSystem": false,
            "LevelingStatGrowthPercentage": 3
        })";

        auto maybe_data = loader->ParseAndLoadLevelingConfig(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.enable_leveling_system, false);
        ASSERT_EQ(data.leveling_grow_rate_percentage, 3_fp);
    }

    // Defaults
    {
        constexpr std::string_view json_text = R"({})";

        auto maybe_data = loader->ParseAndLoadLevelingConfig(json_text);
        ASSERT_TRUE(maybe_data.has_value());

        auto& data = *maybe_data;
        ASSERT_EQ(data.enable_leveling_system, true);
        ASSERT_EQ(data.leveling_grow_rate_percentage, 25_fp);
    }
}

}  // namespace simulation
