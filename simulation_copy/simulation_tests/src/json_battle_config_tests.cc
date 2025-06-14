#include "base_json_test.h"

namespace simulation
{

class BattleConfigJSONTests : public BaseJSONTest
{
public:
    using Super = BaseJSONTest;
};

TEST_F(BattleConfigJSONTests, AllFields)
{
    constexpr std::string_view json_text = R"({
            "RandomSeed": 1234,
            "GridHeight": 150,
            "GridWidth": 151,
            "GridScale": 10,
            "MiddleLineWidth": 5,
            "SortByUniqueID": false,
            "UseMaxStagePlacementRadius": true,
            "MaxWeaponAmplifiers": 3,
            "AugmentsConfig" : {
                "MaxAugments": 10
            },
            "ConsumablesConfig" : {
                "MaxConsumables" : 10
            },
            "OverloadConfig": {
                "EnableOverloadSystem": false
            },
            "LevelingConfig": {
                "EnableLevelingSystem": true,
                "LevelingStatGrowthPercentage": 31
            },
            "TeamsEncounterMods": {}
        })";

    auto opt_data = loader->ParseAndLoadBattleConfig(json_text);
    ASSERT_TRUE(opt_data.has_value());

    const BattleConfig& data = *opt_data;
    EXPECT_EQ(data.random_seed, 1234);
    EXPECT_EQ(data.grid_height, 150);
    EXPECT_EQ(data.grid_width, 151);
    EXPECT_EQ(data.grid_scale, 10);
    EXPECT_EQ(data.middle_line_width, 5);
    EXPECT_EQ(data.sort_by_unique_id, false);
    EXPECT_EQ(data.use_max_stage_placement_radius, true);
    EXPECT_EQ(data.leveling_config.enable_leveling_system, true);
    EXPECT_EQ(data.leveling_config.leveling_grow_rate_percentage, 31_fp);
    EXPECT_EQ(data.augments_config.max_augments, 10);
    EXPECT_EQ(data.consumables_config.max_consumables, 10);
    EXPECT_EQ(data.max_weapon_amplifiers, 3);
}

TEST_F(BattleConfigJSONTests, DefaultsTest)
{
    constexpr std::string_view json_text = R"({
            "MiddleLineWidth": 5,
            "SortByUniqueID": false,
            "UseMaxStagePlacementRadius": true,
            "AugmentsConfig" : {
                "MaxAugments": 10
            },
            "ConsumablesConfig" : {
                "MaxConsumables" : 10
            },
            "OverloadConfig": {
                "EnableOverloadSystem": false
            },
            "LevelingConfig": {
                "EnableLevelingSystem": true,
                "LevelingStatGrowthPercentage": 31
            },
            "TeamsEncounterMods": {}
        })";

    auto opt_data = loader->ParseAndLoadBattleConfig(json_text);
    ASSERT_TRUE(opt_data.has_value());

    const BattleConfig& data = *opt_data;
    EXPECT_EQ(data.grid_height, 151);
    EXPECT_EQ(data.grid_width, 151);
    EXPECT_EQ(data.grid_scale, 10);
    EXPECT_EQ(data.max_weapon_amplifiers, 2);
}

TEST_F(BattleConfigJSONTests, RandomSeedIsOptional)
{
    constexpr std::string_view json_text = R"({
            "GridHeight": 150,
            "GridWidth": 151,
            "GridScale": 10,
            "MiddleLineWidth": 0,
            "SortByUniqueID": true,
            "EnableEvolution": true,
            "UseMaxStagePlacementRadius": true,
            "AugmentsConfig" : {
                "MaxAugments": 10
            },
            "ConsumablesConfig" : {
                "MaxConsumables" : 10
            },
            "OverloadConfig": {
                "EnableOverloadSystem": false
            },
            "TeamsEncounterMods": {}
        })";

    auto opt_data = loader->ParseAndLoadBattleConfig(json_text);
    ASSERT_TRUE(opt_data.has_value());
}

TEST_F(BattleConfigJSONTests, GridHeightIsOptional)
{
    constexpr std::string_view json_text = R"({
            "RandomSeed": 1234,
            "GridWidth": 151,
            "GridScale": 10,
            "MiddleLineWidth": 0,
            "SortByUniqueID": true,
            "EnableEvolution": true,
            "UseMaxStagePlacementRadius": true,
            "AugmentsConfig" : {
                "MaxAugments": 10
            },
            "ConsumablesConfig" : {
                "MaxConsumables" : 10
            },
            "OverloadConfig": {
                "EnableOverloadSystem": false
            },
            "TeamsEncounterMods": {}
        })";

    auto opt_data = loader->ParseAndLoadBattleConfig(json_text);
    ASSERT_TRUE(opt_data.has_value());
}

TEST_F(BattleConfigJSONTests, GridWidthIsOptional)
{
    constexpr std::string_view json_text = R"({
            "RandomSeed": 1234,
            "GridHeight": 150,
            "GridScale": 10,
            "MiddleLineWidth": 0,
            "SortByUniqueID": true,
            "EnableEvolution": true,
            "UseMaxStagePlacementRadius": true,
            "AugmentsConfig" : {
                "MaxAugments": 10
            },
            "ConsumablesConfig" : {
                "MaxConsumables" : 10
            },
            "OverloadConfig": {
                "EnableOverloadSystem": false
            },
            "TeamsEncounterMods": {}
        })";

    auto opt_data = loader->ParseAndLoadBattleConfig(json_text);
    ASSERT_TRUE(opt_data.has_value());
}

TEST_F(BattleConfigJSONTests, GridScaleIsOptional)
{
    constexpr std::string_view json_text = R"({
            "RandomSeed": 1234,
            "GridHeight": 150,
            "GridWidth": 151,
            "MiddleLineWidth": 0,
            "SortByUniqueID": true,
            "EnableEvolution": true,
            "UseMaxStagePlacementRadius": true,
            "AugmentsConfig" : {
                "MaxAugments": 10
            },
            "ConsumablesConfig" : {
                "MaxConsumables" : 10
            },
            "OverloadConfig": {
                "EnableOverloadSystem": false
            },
            "TeamsEncounterMods": {}
        })";

    auto opt_data = loader->ParseAndLoadBattleConfig(json_text);
    ASSERT_TRUE(opt_data.has_value());
}

TEST_F(BattleConfigJSONTests, MiddleLineWidthIsOptional)
{
    constexpr std::string_view json_text = R"({
            "RandomSeed": 1234,
            "GridHeight": 150,
            "GridWidth": 151,
            "GridScale": 10,
            "SortByUniqueID": true,
            "EnableEvolution": true,
            "UseMaxStagePlacementRadius": true,
            "AugmentsConfig" : {
                "MaxAugments": 10
            },
            "ConsumablesConfig" : {
                "MaxConsumables" : 10
            },
            "OverloadConfig": {
                "EnableOverloadSystem": false
            },
            "TeamsEncounterMods": {}
        })";

    auto opt_data = loader->ParseAndLoadBattleConfig(json_text);
    ASSERT_TRUE(opt_data.has_value());
}

TEST_F(BattleConfigJSONTests, SortByUniqueIDIsOptional)
{
    constexpr std::string_view json_text = R"({
            "RandomSeed": 1234,
            "GridHeight": 150,
            "GridWidth": 151,
            "GridScale": 10,
            "MiddleLineWidth": 0,
            "EnableEvolution": true,
            "UseMaxStagePlacementRadius": true,
            "AugmentsConfig" : {
                "MaxAugments": 10
            },
            "ConsumablesConfig" : {
                "MaxConsumables" : 10
            },
            "OverloadConfig": {
                "EnableOverloadSystem": false
            },
            "TeamsEncounterMods": {}
        })";

    auto opt_data = loader->ParseAndLoadBattleConfig(json_text);
    ASSERT_TRUE(opt_data.has_value());
}

TEST_F(BattleConfigJSONTests, EnableEvolutionIsOptional)
{
    constexpr std::string_view json_text = R"({
            "RandomSeed": 1234,
            "GridHeight": 150,
            "GridWidth": 151,
            "GridScale": 10,
            "MiddleLineWidth": 0,
            "SortByUniqueID": true,
            "UseMaxStagePlacementRadius": true,
            "AugmentsConfig" : {
                "MaxAugments": 10
            },
            "ConsumablesConfig" : {
                "MaxConsumables" : 10
            },
            "OverloadConfig": {
                "EnableOverloadSystem": false
            },
            "TeamsEncounterMods": {}
        })";

    ASSERT_TRUE(loader->ParseAndLoadBattleConfig(json_text).has_value());
}

TEST_F(BattleConfigJSONTests, UseMaxStagePlacementRadiusIsOptional)
{
    constexpr std::string_view json_text = R"({
            "RandomSeed": 1234,
            "GridHeight": 150,
            "GridWidth": 151,
            "GridScale": 10,
            "MiddleLineWidth": 0,
            "SortByUniqueID": true,
            "EnableEvolution": true,
            "AugmentsConfig" : {
                "MaxAugments": 10
            },
            "ConsumablesConfig" : {
                "MaxConsumables" : 10
            },
            "OverloadConfig": {
                "EnableOverloadSystem": false
            },
            "TeamsEncounterMods": {}
        })";

    ASSERT_TRUE(loader->ParseAndLoadBattleConfig(json_text).has_value());
}

TEST_F(BattleConfigJSONTests, AugmentsConfigIsOptional)
{
    constexpr std::string_view json_text = R"({
            "RandomSeed": 1234,
            "GridHeight": 150,
            "GridWidth": 151,
            "GridScale": 10,
            "MiddleLineWidth": 0,
            "SortByUniqueID": true,
            "EnableEvolution": true,
            "UseMaxStagePlacementRadius": true,
            "ConsumablesConfig" : {
                "MaxConsumables" : 10
            },
            "OverloadConfig": {
                "EnableOverloadSystem": false
            },
            "TeamsEncounterMods": {}
        })";

    ASSERT_TRUE(loader->ParseAndLoadBattleConfig(json_text).has_value());
}

TEST_F(BattleConfigJSONTests, ConsumablesConfigIsOptional)
{
    constexpr std::string_view json_text = R"({
            "RandomSeed": 1234,
            "GridHeight": 150,
            "GridWidth": 151,
            "GridScale": 10,
            "MiddleLineWidth": 0,
            "SortByUniqueID": true,
            "EnableEvolution": true,
            "UseMaxStagePlacementRadius": true,
            "AugmentsConfig" : {
                "MaxAugments": 10
            },
            "OverloadConfig": {
                "EnableOverloadSystem": false
            },
            "TeamsEncounterMods": {}
        })";

    ASSERT_TRUE(loader->ParseAndLoadBattleConfig(json_text).has_value());
}

TEST_F(BattleConfigJSONTests, OverloadConfigIsOptional)
{
    constexpr std::string_view json_text = R"({
            "RandomSeed": 1234,
            "GridHeight": 150,
            "GridWidth": 151,
            "GridScale": 10,
            "MiddleLineWidth": 0,
            "SortByUniqueID": true,
            "EnableEvolution": true,
            "UseMaxStagePlacementRadius": true,
            "AugmentsConfig" : {
                "MaxAugments": 10
            },
            "ConsumablesConfig" : {
                "MaxConsumables" : 10
            },
            "TeamsEncounterMods": {}
        })";

    ASSERT_TRUE(loader->ParseAndLoadBattleConfig(json_text).has_value());
}

TEST_F(BattleConfigJSONTests, TeamsEncounterModsIsOptional)
{
    constexpr std::string_view json_text = R"({
            "RandomSeed": 1234,
            "GridHeight": 150,
            "GridWidth": 151,
            "GridScale": 10,
            "MiddleLineWidth": 0,
            "SortByUniqueID": true,
            "EnableEvolution": true,
            "UseMaxStagePlacementRadius": true,
            "AugmentsConfig" : {
                "MaxAugments": 10
            },
            "ConsumablesConfig" : {
                "MaxConsumables" : 10
            },
            "OverloadConfig": {
                "EnableOverloadSystem": false
            }
        })";

    ASSERT_TRUE(loader->ParseAndLoadBattleConfig(json_text).has_value());
}

}  // namespace simulation
