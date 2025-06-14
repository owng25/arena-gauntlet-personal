#include "base_json_test.h"

namespace simulation
{

class WorldEffectsConfigTestJSON : public BaseJSONTest
{
public:
    using Super = BaseJSONTest;
};

TEST_F(WorldEffectsConfigTestJSON, AllEntriesPresentAndValid)
{
    constexpr std::string_view json_text = R"({
        "Burn": {
            "DurationMs": 1000,
            "FrequencyTimeMs": 2000,
            "MaxStacks": 1,
            "CleanseOnMaxStacks": false,
            "DotDamageType": "Physical",
            "DotHighPrecisionPercentage": 3000,
            "DebuffStatType": "AttackPhysicalDamage",
            "DebuffPercentage": 4000
        },
        "Frost": {
            "DurationMs": 1001,
            "FrequencyTimeMs": 2001,
            "MaxStacks": 2,
            "CleanseOnMaxStacks": true,
            "DotDamageType": "Pure",
            "DotHighPrecisionPercentage": 3001,
            "DebuffStatType": "AttackPureDamage",
            "DebuffPercentage": 4001
        },
        "Poison": {
            "DurationMs": 1002,
            "FrequencyTimeMs": 2002,
            "MaxStacks": 3,
            "CleanseOnMaxStacks": false,
            "DotDamageType": "Energy",
            "DotHighPrecisionPercentage": 3002,
            "DebuffStatType": "AttackEnergyDamage",
            "DebuffPercentage": 4002
        },
        "Wound": {
            "DurationMs": 1003,
            "FrequencyTimeMs": 2003,
            "MaxStacks": 4,
            "CleanseOnMaxStacks": true,
            "DotDamageType": "Purest",
            "DotHighPrecisionPercentage": 3003,
            "DebuffStatType": "MaxHealth",
            "DebuffPercentage": 4003
        }
    })";

    auto opt_data = loader->ParseAndLoadWorldEffectsConfig(json_text);
    ASSERT_TRUE(opt_data.has_value());

    const auto& data = *opt_data;

    const auto& burn = data.GetConditionType(EffectConditionType::kBurn);
    EXPECT_EQ(burn.duration_ms, 1000);
    EXPECT_EQ(burn.frequency_time_ms, 2000);
    EXPECT_EQ(burn.max_stacks, 1);
    EXPECT_EQ(burn.cleanse_on_max_stacks, false);
    EXPECT_EQ(burn.dot_damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(burn.dot_high_precision_percentage, 3000_fp);
    EXPECT_EQ(burn.debuff_stat_type, StatType::kAttackPhysicalDamage);
    EXPECT_EQ(burn.debuff_percentage, 4000_fp);

    const auto& frost = data.GetConditionType(EffectConditionType::kFrost);
    EXPECT_EQ(frost.duration_ms, 1001);
    EXPECT_EQ(frost.frequency_time_ms, 2001);
    EXPECT_EQ(frost.max_stacks, 2);
    EXPECT_EQ(frost.cleanse_on_max_stacks, true);
    EXPECT_EQ(frost.dot_damage_type, EffectDamageType::kPure);
    EXPECT_EQ(frost.dot_high_precision_percentage, 3001_fp);
    EXPECT_EQ(frost.debuff_stat_type, StatType::kAttackPureDamage);
    EXPECT_EQ(frost.debuff_percentage, 4001_fp);

    const auto& poison = data.GetConditionType(EffectConditionType::kPoison);
    EXPECT_EQ(poison.duration_ms, 1002);
    EXPECT_EQ(poison.frequency_time_ms, 2002);
    EXPECT_EQ(poison.max_stacks, 3);
    EXPECT_EQ(poison.cleanse_on_max_stacks, false);
    EXPECT_EQ(poison.dot_damage_type, EffectDamageType::kEnergy);
    EXPECT_EQ(poison.dot_high_precision_percentage, 3002_fp);
    EXPECT_EQ(poison.debuff_stat_type, StatType::kAttackEnergyDamage);
    EXPECT_EQ(poison.debuff_percentage, 4002_fp);

    const auto& wound = data.GetConditionType(EffectConditionType::kWound);
    EXPECT_EQ(wound.duration_ms, 1003);
    EXPECT_EQ(wound.frequency_time_ms, 2003);
    EXPECT_EQ(wound.max_stacks, 4);
    EXPECT_EQ(wound.cleanse_on_max_stacks, true);
    EXPECT_EQ(wound.dot_damage_type, EffectDamageType::kPurest);
    EXPECT_EQ(wound.dot_high_precision_percentage, 3003_fp);
    EXPECT_EQ(wound.debuff_stat_type, StatType::kMaxHealth);
    EXPECT_EQ(wound.debuff_percentage, 4003_fp);
}

TEST_F(WorldEffectsConfigTestJSON, DurationIsRequired)
{
    constexpr std::string_view json_text = R"({
        "FrequencyTimeMs": 2000,
        "MaxStacks": 1,
        "CleanseOnMaxStacks": false,
        "DotDamageType": "Physical",
        "DotHighPrecisionPercentage": 3000,
        "DebuffStatType": "AttackPhysicalDamage",
        "DebuffPercentage": 4000
    })";

    const auto opt_data = silent_loader->ParseAndLoadWorldEffectConditionConfig(json_text);
    ASSERT_FALSE(opt_data.has_value());
}

TEST_F(WorldEffectsConfigTestJSON, FrequencyTimeMsIsRequired)
{
    constexpr std::string_view json_text = R"({
        "DurationMs": 1000,
        "MaxStacks": 1,
        "CleanseOnMaxStacks": false,
        "DotDamageType": "Physical",
        "DotHighPrecisionPercentage": 3000,
        "DebuffStatType": "AttackPhysicalDamage",
        "DebuffPercentage": 4000
    })";

    const auto opt_data = silent_loader->ParseAndLoadWorldEffectConditionConfig(json_text);
    ASSERT_FALSE(opt_data.has_value());
}

TEST_F(WorldEffectsConfigTestJSON, MaxStacksIsRequired)
{
    constexpr std::string_view json_text = R"({
        "DurationMs": 1000,
        "FrequencyTimeMs": 2000,
        "CleanseOnMaxStacks": false,
        "DotDamageType": "Physical",
        "DotHighPrecisionPercentage": 3000,
        "DebuffStatType": "AttackPhysicalDamage",
        "DebuffPercentage": 4000
    })";

    const auto opt_data = silent_loader->ParseAndLoadWorldEffectConditionConfig(json_text);
    ASSERT_FALSE(opt_data.has_value());
}

TEST_F(WorldEffectsConfigTestJSON, CleanseOnMaxIsRequired)
{
    constexpr std::string_view json_text = R"({
        "DurationMs": 1000,
        "FrequencyTimeMs": 2000,
        "MaxStacks": 1,
        "DotDamageType": "Physical",
        "DotHighPrecisionPercentage": 3000,
        "DebuffStatType": "AttackPhysicalDamage",
        "DebuffPercentage": 4000
    })";

    const auto opt_data = silent_loader->ParseAndLoadWorldEffectConditionConfig(json_text);
    ASSERT_FALSE(opt_data.has_value());
}

TEST_F(WorldEffectsConfigTestJSON, DotInfoIsOptional)
{
    constexpr std::string_view json_text = R"({
        "DurationMs": 1000,
        "FrequencyTimeMs": 2000,
        "MaxStacks": 1,
        "CleanseOnMaxStacks": false,
        "DebuffStatType": "AttackPhysicalDamage",
        "DebuffPercentage": 4000
    })";

    const auto opt_data = silent_loader->ParseAndLoadWorldEffectConditionConfig(json_text);
    ASSERT_TRUE(opt_data.has_value());

    const auto& data = *opt_data;
    EXPECT_EQ(data.dot_damage_type, EffectDamageType::kNone);
    EXPECT_EQ(data.dot_high_precision_percentage, 0_fp);
}

TEST_F(WorldEffectsConfigTestJSON, DotInfoRequiresAllFields)
{
    ASSERT_FALSE(silent_loader->ParseAndLoadWorldEffectConditionConfig(R"({
        "DurationMs": 1000,
        "FrequencyTimeMs": 2000,
        "MaxStacks": 1,
        "CleanseOnMaxStacks": false,
        "DotHighPrecisionPercentage": 3000,
        "DebuffStatType": "AttackPhysicalDamage",
        "DebuffPercentage": 4000
    })"));

    ASSERT_FALSE(silent_loader->ParseAndLoadWorldEffectConditionConfig(R"({
        "DurationMs": 1000,
        "FrequencyTimeMs": 2000,
        "MaxStacks": 1,
        "CleanseOnMaxStacks": false,
        "DotDamageType": "Physical",
        "DebuffStatType": "AttackPhysicalDamage",
        "DebuffPercentage": 4000
    })"));
}

TEST_F(WorldEffectsConfigTestJSON, DebuffInfoIsOptional)
{
    constexpr std::string_view json_text = R"({
        "DurationMs": 1000,
        "FrequencyTimeMs": 2000,
        "MaxStacks": 1,
        "CleanseOnMaxStacks": false,
        "DotDamageType": "Physical",
        "DotHighPrecisionPercentage": 3000
    })";

    const auto opt_data = silent_loader->ParseAndLoadWorldEffectConditionConfig(json_text);
    ASSERT_TRUE(opt_data.has_value());

    const auto& data = *opt_data;
    EXPECT_EQ(data.debuff_stat_type, StatType::kNone);
    EXPECT_EQ(data.debuff_percentage, 0_fp);
}

TEST_F(WorldEffectsConfigTestJSON, DebuffInfoRequiresAllFields)
{
    ASSERT_FALSE(silent_loader->ParseAndLoadWorldEffectConditionConfig(R"({
        "DurationMs": 1000,
        "FrequencyTimeMs": 2000,
        "MaxStacks": 1,
        "CleanseOnMaxStacks": false,
        "DebuffPercentage": 4000
    })"));

    ASSERT_FALSE(silent_loader->ParseAndLoadWorldEffectConditionConfig(R"({
        "DurationMs": 1000,
        "FrequencyTimeMs": 2000,
        "MaxStacks": 1,
        "CleanseOnMaxStacks": false,
        "DebuffStatType": "AttackPhysicalDamage"
    })"));
}
}  // namespace simulation
