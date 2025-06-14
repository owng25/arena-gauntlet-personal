#include "base_test_fixtures.h"
#include "data/loaders/base_data_loader.h"
#include "gtest/gtest.h"

namespace simulation
{
class JSONHyperTest : public BaseTest
{
    bool IsEnabledLogsForCalculateLiveStats() const override
    {
        return false;
    }
};

TEST_F(JSONHyperTest, LoadHyperData)
{
    const char* hyper_data_json_raw = R"(
{
    "CombatAffinityCounter": {
        "CombatAffinityTypes": [
            "Water",
            "Earth",
            "Fire",
            "Nature",
            "Air"
        ],
        "CombatAffinities": [
            {
                "Name": "Water",
                "Effectiveness": {
                    "Water": 0,
                    "Earth": 2,
                    "Fire": 1,
                    "Nature": -1,
                    "Air": -2
                }
            },
            {
                "Name": "Earth",
                "Effectiveness": {
                    "Water": -2,
                    "Earth": 0,
                    "Fire": 2,
                    "Nature": 1,
                    "Air": -1
                }
            },
            {
                "Name": "Fire",
                "Effectiveness": {
                    "Water": -1,
                    "Earth": -2,
                    "Fire": 0,
                    "Nature": 2,
                    "Air": 1
                }
            },
            {
                "Name": "Nature",
                "Effectiveness": {
                    "Water": 1,
                    "Earth": -1,
                    "Fire": -2,
                    "Nature": 0,
                    "Air": 2
                }
            },
            {
                "Name": "Air",
                "Effectiveness": {
                    "Water": 2,
                    "Earth": 1,
                    "Fire": -1,
                    "Nature": -2,
                    "Air": 0
                }
            }
        ]
    }
}
)";

    HyperData hyper_data;
    BaseDataLoader loader(world->GetLogger());
    loader.LoadHyperData(nlohmann::json::parse(hyper_data_json_raw), &hyper_data);

    ASSERT_EQ(hyper_data.combat_affinity_opponents.size(), 5);
    EXPECT_EQ(hyper_data.GetCombatAffinityEffectiveness(CombatAffinity::kWater, CombatAffinity::kWater), 0);
    EXPECT_EQ(hyper_data.GetCombatAffinityEffectiveness(CombatAffinity::kWater, CombatAffinity::kEarth), 2);
    EXPECT_EQ(hyper_data.GetCombatAffinityEffectiveness(CombatAffinity::kWater, CombatAffinity::kFire), 1);
    EXPECT_EQ(hyper_data.GetCombatAffinityEffectiveness(CombatAffinity::kWater, CombatAffinity::kNature), -1);
    EXPECT_EQ(hyper_data.GetCombatAffinityEffectiveness(CombatAffinity::kWater, CombatAffinity::kAir), -2);
    EXPECT_EQ(hyper_data.GetCombatAffinityEffectiveness(CombatAffinity::kEarth, CombatAffinity::kWater), -2);
}

TEST_F(JSONHyperTest, LoadHyperConfig)
{
    const char* hyper_data_json_raw = R"(
{
    "SubHyperScalePercentage": 10,
    "EnemiesRangeUnits": 40
}
)";

    HyperConfig hyper_config;
    BaseDataLoader loader(world->GetLogger());
    loader.LoadHyperConfig(nlohmann::json::parse(hyper_data_json_raw), &hyper_config);

    EXPECT_EQ(hyper_config.sub_hyper_scale_percentage, 10_fp);
    EXPECT_EQ(hyper_config.enemies_range_units, 40);
}

}  // namespace simulation
