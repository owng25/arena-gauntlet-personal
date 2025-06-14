#include <optional>

#include "base_json_test.h"
#include "test_data_loader.h"

namespace simulation
{

class SkillTargetingDataTestJSON : public BaseJSONTest
{
public:
    using Super = BaseJSONTest;
};

TEST_F(SkillTargetingDataTestJSON, InZoneTargeting)
{
    {
        const char* json_raw = R"({
            "Type": "InZone",
            "Group": "Ally",
            "RadiusUnits": 10
        })";

        std::optional<SkillTargetingData> maybe_data = loader->ParseAndLoadSkillTargetingData(json_raw);
        ASSERT_TRUE(maybe_data.has_value());

        const auto& data = *maybe_data;
        EXPECT_EQ(data.type, SkillTargetingType::kInZone);
        EXPECT_EQ(data.group, AllegianceType::kAlly);
        EXPECT_EQ(data.radius_units, 10);
    }

    {
        const char* json_raw = R"({
            "Type": "InZone",
            "Group": "Ally",
            "RadiusUnits": 10,
            "OnlyCurrentFocusers": true
        })";

        std::optional<SkillTargetingData> maybe_data = loader->ParseAndLoadSkillTargetingData(json_raw);
        ASSERT_TRUE(maybe_data.has_value());

        const auto& data = *maybe_data;
        EXPECT_EQ(data.type, SkillTargetingType::kInZone);
        EXPECT_EQ(data.group, AllegianceType::kAlly);
        EXPECT_EQ(data.radius_units, 10);
        EXPECT_EQ(data.only_current_focusers, true);
    }
}

}  // namespace simulation
