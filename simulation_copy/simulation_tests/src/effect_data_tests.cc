#include "data/effect_data.h"
#include "gtest/gtest.h"
#include "test_data_loader.h"

namespace simulation
{
class EffectExpressionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        sender_stats.base.Set(StatType::kMoveSpeedSubUnits, GetSenderCounter());
        sender_stats.base.Set(StatType::kAttackRangeUnits, GetSenderCounter());
        // 102
        sender_stats.base.Set(StatType::kAttackSpeed, GetSenderCounter());
        sender_stats.base.Set(StatType::kHitChancePercentage, GetSenderCounter());
        sender_stats.base.Set(StatType::kAttackDodgeChancePercentage, GetSenderCounter());
        // 105
        sender_stats.base.Set(StatType::kMaxHealth, GetSenderCounter());
        sender_stats.base.Set(StatType::kCurrentHealth, GetSenderCounter());
        sender_stats.base.Set(StatType::kEnergyCost, GetSenderCounter());
        sender_stats.base.Set(StatType::kStartingEnergy, GetSenderCounter());
        sender_stats.base.Set(StatType::kCurrentEnergy, GetSenderCounter());
        sender_stats.base.Set(StatType::kPhysicalResist, GetSenderCounter());
        sender_stats.base.Set(StatType::kEnergyResist, GetSenderCounter());
        sender_stats.base.Set(StatType::kTenacityPercentage, GetSenderCounter());
        sender_stats.base.Set(StatType::kWillpowerPercentage, GetSenderCounter());
        sender_stats.base.Set(StatType::kGrit, GetSenderCounter());
        sender_stats.base.Set(StatType::kResolve, GetSenderCounter());
        sender_stats.base.Set(StatType::kCritChancePercentage, GetSenderCounter());
        sender_stats.base.Set(StatType::kCritAmplificationPercentage, GetSenderCounter());
        sender_stats.base.Set(StatType::kOmegaPowerPercentage, GetSenderCounter());
        sender_stats.base.Set(StatType::kEnergyGainEfficiencyPercentage, GetSenderCounter());
        sender_stats.base.Set(StatType::kHealthGainEfficiencyPercentage, GetSenderCounter());
        sender_stats.base.Set(StatType::kPhysicalVampPercentage, GetSenderCounter());
        sender_stats.base.Set(StatType::kEnergyVampPercentage, GetSenderCounter());
        sender_stats.base.Set(StatType::kPureVampPercentage, GetSenderCounter());
        sender_stats.base.Set(StatType::kCritReductionPercentage, GetSenderCounter());

        receiver_stats.base.Set(StatType::kMoveSpeedSubUnits, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kAttackRangeUnits, GetReceiverCounter());
        // 202
        receiver_stats.base.Set(StatType::kAttackSpeed, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kHitChancePercentage, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kAttackDodgeChancePercentage, GetReceiverCounter());
        // 205
        receiver_stats.base.Set(StatType::kMaxHealth, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kCurrentHealth, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kEnergyCost, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kStartingEnergy, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kCurrentEnergy, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kPhysicalResist, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kEnergyResist, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kTenacityPercentage, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kWillpowerPercentage, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kGrit, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kResolve, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kCritChancePercentage, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kCritAmplificationPercentage, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kOmegaPowerPercentage, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kEnergyGainEfficiencyPercentage, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kHealthGainEfficiencyPercentage, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kPhysicalVampPercentage, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kEnergyVampPercentage, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kPureVampPercentage, GetReceiverCounter());
        receiver_stats.base.Set(StatType::kCritReductionPercentage, GetReceiverCounter());

        sender_stats.live = sender_stats.base;
        receiver_stats.live = receiver_stats.base;
    }

    FixedPoint GetSenderCounter()
    {
        const int counter = combat_class_from_counter;
        combat_class_from_counter += 1;
        return FixedPoint::FromInt(counter);
    }

    FixedPoint GetReceiverCounter()
    {
        const int counter = combat_class_target_counter;
        combat_class_target_counter += 1;
        return FixedPoint::FromInt(counter);
    }

    ExpressionStatsSource MakeDefaultDataSource() const
    {
        ExpressionStatsSource stats_source;
        stats_source.Set(ExpressionDataSourceType::kSender, sender_stats, nullptr);
        stats_source.Set(ExpressionDataSourceType::kReceiver, receiver_stats, nullptr);
        return stats_source;
    }

    FixedPoint Evaluate(const EffectExpression& expression)
    {
        ExpressionStatsSource stats_source;
        stats_source.Set(ExpressionDataSourceType::kSender, sender_stats, nullptr);
        stats_source.Set(ExpressionDataSourceType::kReceiver, receiver_stats, nullptr);
        return expression.Evaluate(ExpressionEvaluationContext{}, stats_source);
    }

    int combat_class_from_counter = 100;
    int combat_class_target_counter = 200;

    FullStatsData sender_stats;
    FullStatsData receiver_stats;
};

TEST_F(EffectExpressionTest, Amount)
{
    {
        const auto expression = EffectExpression::FromValue(-10_fp);
        EXPECT_EQ(Evaluate(expression), -10_fp);
    }
    {
        const auto expression = EffectExpression::FromValue(0_fp);
        EXPECT_EQ(Evaluate(expression), 0_fp);
    }
    {
        const auto expression = EffectExpression::FromValue(10_fp);
        EXPECT_EQ(Evaluate(expression), 10_fp);
    }
}

TEST_F(EffectExpressionTest, Stat)
{
    // Sender
    {
        // MoveSpeed (Sender)
        const auto expression = EffectExpression::FromStat(
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kSender);
        EXPECT_EQ(Evaluate(expression), 100_fp);
    }

    // Receiver
    {
        // MoveSpeed (Receiver)
        const auto expression = EffectExpression::FromStat(
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kReceiver);
        EXPECT_EQ(Evaluate(expression), 200_fp);
    }
}

TEST_F(EffectExpressionTest, StatAllTypeGroups)
{
    constexpr auto sender_bonus = 10_fp;
    constexpr auto receiver_bonus = 20_fp;
    sender_stats.live.Add(StatType::kMoveSpeedSubUnits, sender_bonus);
    receiver_stats.live.Add(StatType::kMoveSpeedSubUnits, receiver_bonus);

    //
    // MoveSpeed (Sender)
    //

    // Base
    {
        const auto expression = EffectExpression::FromStat(
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kBase,
            ExpressionDataSourceType::kSender);
        EXPECT_EQ(Evaluate(expression), 100_fp);
    }

    // Live
    {
        const auto expression = EffectExpression::FromStat(
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kSender);
        EXPECT_EQ(Evaluate(expression), 100_fp + sender_bonus);
    }

    // Bonus
    {
        const auto expression = EffectExpression::FromStat(
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kBonus,
            ExpressionDataSourceType::kSender);
        EXPECT_EQ(Evaluate(expression), sender_bonus);
    }

    //
    // MoveSpeed (Receiver)
    //

    // Base
    {
        const auto expression = EffectExpression::FromStat(
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kBase,
            ExpressionDataSourceType::kReceiver);
        EXPECT_EQ(Evaluate(expression), 200_fp);
    }

    // Live
    {
        const auto expression = EffectExpression::FromStat(
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kReceiver);
        EXPECT_EQ(Evaluate(expression), 200_fp + receiver_bonus);
    }

    // Bonus
    {
        const auto expression = EffectExpression::FromStat(
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kBonus,
            ExpressionDataSourceType::kReceiver);
        EXPECT_EQ(Evaluate(expression), receiver_bonus);
    }
}

TEST_F(EffectExpressionTest, StatPercentage)
{
    // Sender
    {
        // 10% MoveSpeed (Sender)
        const auto expression = EffectExpression::FromStatPercentage(
            10_fp,
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kSender);
        EXPECT_EQ(Evaluate(expression), 10_fp);
    }

    // Receiver
    {
        // 10% MoveSpeed (Receiver))
        const auto expression = EffectExpression::FromStatPercentage(
            10_fp,
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kReceiver);
        EXPECT_EQ(Evaluate(expression), 20_fp);
    }
}

TEST_F(EffectExpressionTest, StatPercentageAllTypeGroups)
{
    constexpr auto sender_bonus = 150_fp;
    constexpr auto receiver_bonus = 270_fp;
    constexpr auto percentage = 10_fp;
    sender_stats.live.Add(StatType::kMoveSpeedSubUnits, sender_bonus);
    receiver_stats.live.Add(StatType::kMoveSpeedSubUnits, receiver_bonus);

    //
    // 10% MoveSpeed (Sender)
    //

    // Base
    {
        const auto expression = EffectExpression::FromStatPercentage(
            percentage,
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kBase,
            ExpressionDataSourceType::kSender);
        EXPECT_EQ(Evaluate(expression), 10_fp);
    }

    // Live
    {
        const auto expression = EffectExpression::FromStatPercentage(
            percentage,
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kSender);
        EXPECT_EQ(Evaluate(expression), 25_fp);
    }

    // Bonus
    {
        const auto expression = EffectExpression::FromStatPercentage(
            percentage,
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kBonus,
            ExpressionDataSourceType::kSender);
        EXPECT_EQ(Evaluate(expression), 15_fp);
    }

    //
    // 10% MoveSpeed (Receiver)
    //

    // Base
    {
        const auto expression = EffectExpression::FromStatPercentage(
            percentage,
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kBase,
            ExpressionDataSourceType::kReceiver);
        EXPECT_EQ(Evaluate(expression), 20_fp);
    }

    // Live
    {
        const auto expression = EffectExpression::FromStatPercentage(
            percentage,
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kReceiver);
        EXPECT_EQ(Evaluate(expression), 47_fp);
    }

    // Bonus
    {
        const auto expression = EffectExpression::FromStatPercentage(
            percentage,
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kBonus,
            ExpressionDataSourceType::kReceiver);
        EXPECT_EQ(Evaluate(expression), 27_fp);
    }
}

TEST_F(EffectExpressionTest, PercentageMultiplyWith)
{
    // Sender
    {
        // 10% MoveSpeed (Sender) * MaxHealth (Sender) = 10 * 105 = 1050
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromStat(
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
        };
        EXPECT_EQ(Evaluate(expression), 1050_fp);
    }

    // Receiver
    {
        // 10% MoveSpeed (Sender) * MaxHealth (Receiver) = 10 * 205 = 2050
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromStat(
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
        };
        EXPECT_EQ(Evaluate(expression), 2050_fp);
    }

    // Receiver, invalid multiplay stat, should just ignore it
    {
        // 10% MoveSpeed (Sender) * kNone (Receiver) = 0
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromStat(StatType::kNone, StatEvaluationType::kLive, ExpressionDataSourceType::kReceiver),
        };
        EXPECT_EQ(Evaluate(expression), 0_fp);
    }
}

TEST_F(EffectExpressionTest, MinMax)
{
    EffectExpression expression;
    expression.operation_type = EffectOperationType::kMax;
    expression.operands = {
        EffectExpression::FromStat(
            StatType::kMoveSpeedSubUnits,
            StatEvaluationType::kLive,
            ExpressionDataSourceType::kSender),
        EffectExpression::FromStat(StatType::kMaxHealth, StatEvaluationType::kLive, ExpressionDataSourceType::kSender),
    };

    EXPECT_EQ(Evaluate(expression), 105_fp);
    EXPECT_EQ(Evaluate(expression), sender_stats.Get(StatType::kMaxHealth, StatEvaluationType::kLive));

    expression.operation_type = EffectOperationType::kMin;
    EXPECT_EQ(Evaluate(expression), 100_fp);
    EXPECT_EQ(Evaluate(expression), sender_stats.Get(StatType::kMoveSpeedSubUnits, StatEvaluationType::kLive));
}

TEST_F(EffectExpressionTest, Floor)
{
    auto loader = TestingDataLoader(Logger::Create());

    {
        const auto opt_expr = loader.ParseAndLoadExpression(
            R"({
        "Operation": "//",
        "Operands": [
            {
                "Stat": "CurrentHealth",
                "StatSource": "Sender",
                "StatEvaluationType": "Live"
            },
            {
                "Stat": "MaxHealth",
                "StatSource": "Sender",
                "StatEvaluationType": "Live"
            }
        ]
    })");

        ASSERT_TRUE(opt_expr);

        const auto& expr = *opt_expr;
        sender_stats.live.Set(StatType::kCurrentHealth, 100_fp);
        sender_stats.live.Set(StatType::kMaxHealth, 100_fp);
        EXPECT_EQ(Evaluate(expr), 1_fp);

        sender_stats.live.Set(StatType::kCurrentHealth, 75_fp);
        sender_stats.live.Set(StatType::kMaxHealth, 100_fp);
        EXPECT_EQ(Evaluate(expr), 0_fp);
    }

    {
        const auto opt_expr = loader.ParseAndLoadExpression(
            R"({
            "Operation": "//",
            "Operands": [
                {
                    "Operation": "-",
                    "Operands": [
                        {
                            "Operation": "/",
                            "Operands": [ 3, 2 ]
                        },
                        {
                            "Operation": "/",
                            "Operands": [
                                {
                                    "Stat": "CurrentHealth",
                                    "StatSource": "Sender",
                                    "StatEvaluationType": "Live"
                                },
                                {
                                    "Stat": "MaxHealth",
                                    "StatSource": "Sender",
                                    "StatEvaluationType": "Live"
                                }
                            ]
                        }
                    ]
                },
                1
            ]
        })");

        ASSERT_TRUE(opt_expr);
        sender_stats.live.Set(StatType::kMaxHealth, 100_fp);

        const auto& expr = *opt_expr;
        sender_stats.live.Set(StatType::kCurrentHealth, 100_fp);
        EXPECT_EQ(Evaluate(expr), 0_fp);

        sender_stats.live.Set(StatType::kCurrentHealth, 75_fp);
        EXPECT_EQ(Evaluate(expr), 0_fp);

        sender_stats.live.Set(StatType::kCurrentHealth, 50_fp);
        EXPECT_EQ(Evaluate(expr), 1_fp);

        sender_stats.live.Set(StatType::kCurrentHealth, 25_fp);
        EXPECT_EQ(Evaluate(expr), 1_fp);
    }
}

TEST_F(EffectExpressionTest, PercentageOf)
{
    // Just values
    {
        // 75% 1000 = 750
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kPercentageOf;
        expression.operands = {EffectExpression::FromValue(75_fp), EffectExpression::FromValue(1000_fp)};
        EXPECT_EQ(Evaluate(expression), 750_fp);
    }

    // Sender
    {
        // 10% MaxHealth (Sender) = 10 % 105 = 10
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kPercentageOf;
        expression.operands = {
            EffectExpression::FromValue(10_fp),
            EffectExpression::FromStat(
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
        };
        EXPECT_EQ(Evaluate(expression), "10.5"_fp);
    }

    // Receiver
    {
        // 10% MaxHealth (Receiver) = 25 % 205 = 51.25
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kPercentageOf;
        expression.operands = {
            EffectExpression::FromValue(25_fp),
            EffectExpression::FromStat(
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
        };
        EXPECT_EQ(Evaluate(expression), "51.25"_fp);
    }
}

TEST_F(EffectExpressionTest, PercentageAddValues)
{
    // Sender
    {
        // 10% MoveSpeed (Sender) + 5 * MaxHealth (Sender) = 10 + 5*105 = 535
        EffectExpression expression_right;
        expression_right.operation_type = EffectOperationType::kMultiply;
        expression_right.operands = {
            EffectExpression::FromValue(5_fp),
            EffectExpression::FromStat(
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender)};

        EffectExpression expression;
        expression.operation_type = EffectOperationType::kAdd;
        expression.operands = {
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            expression_right};

        EXPECT_EQ(Evaluate(expression), 535_fp);
    }

    // Receiver
    {
        // 10% MoveSpeed (Receiver) + 5 * MaxHealth (Receiver) = 20 + 5*205 = 1045
        EffectExpression expression_right;
        expression_right.operation_type = EffectOperationType::kMultiply;
        expression_right.operands = {
            EffectExpression::FromValue(5_fp),
            EffectExpression::FromStat(
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver)};

        EffectExpression expression;
        expression.operation_type = EffectOperationType::kAdd;
        expression.operands = {
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
            expression_right};

        EXPECT_EQ(Evaluate(expression), 1045_fp);
    }

    // Receiver, no stat multiply for add
    {
        // 10% MoveSpeed (Sender) + 5 = 10 + 5 = 15
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kAdd;
        expression.operands = {
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromValue(5_fp)};

        EXPECT_EQ(Evaluate(expression), 15_fp);
    }

    // Receiver, multiple add values
    {
        // 10% MoveSpeed (Sender) + 5 * MaxHealth (Receiver) + 10 * MaxHealth (Sender) = 10 + 5*205
        // + 10 * 105 = 2085

        // 5 * MaxHealth (Receiver)
        EffectExpression expression_right1;
        expression_right1.operation_type = EffectOperationType::kMultiply;
        expression_right1.operands = {
            EffectExpression::FromValue(5_fp),
            EffectExpression::FromStat(
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver)};

        // 10 * MaxHealth (Sender)
        EffectExpression expression_right2;
        expression_right2.operation_type = EffectOperationType::kMultiply;
        expression_right2.operands = {
            EffectExpression::FromValue(10_fp),
            EffectExpression::FromStat(
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender)};

        // Add all of them
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kAdd;
        expression.operands = {
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            expression_right1,
            expression_right2};

        EXPECT_EQ(Evaluate(expression), 2085_fp);
    }
}

TEST_F(EffectExpressionTest, PercentageAll)
{
    // Sender
    {
        // 10% MoveSpeed (Sender) * AttackRange (Sender) + 5 * MaxHealth (Receiver) = 10 * 101 +
        // 5*205 = 535

        // 10% MoveSpeed (Sender) * AttackRange (Sender)
        EffectExpression expression_left;
        expression_left.operation_type = EffectOperationType::kMultiply;
        expression_left.operands = {
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromStat(
                StatType::kAttackRangeUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender)};

        //  5 * MaxHealth (Receiver)
        EffectExpression expression_right;
        expression_right.operation_type = EffectOperationType::kMultiply;
        expression_right.operands = {
            EffectExpression::FromValue(5_fp),
            EffectExpression::FromStat(
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver)};

        EffectExpression expression;
        expression.operation_type = EffectOperationType::kAdd;
        expression.operands = {expression_left, expression_right};

        EXPECT_EQ(Evaluate(expression), 2035_fp);
    }

    // Receiver
    {
        // 10% MoveSpeed (Receiver) * AttackRange (Receiver) + 5 * MaxHealth (Sender) = 20 * 201 +
        // 5*105 = 4565

        // 10% MoveSpeed (Receiver) * AttackRange (Receiver)
        EffectExpression expression_left;
        expression_left.operation_type = EffectOperationType::kMultiply;
        expression_left.operands = {
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
            EffectExpression::FromStat(
                StatType::kAttackRangeUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver)};

        //  5 * MaxHealth (Sender)
        EffectExpression expression_right;
        expression_right.operation_type = EffectOperationType::kMultiply;
        expression_right.operands = {
            EffectExpression::FromValue(5_fp),
            EffectExpression::FromStat(
                StatType::kMaxHealth,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender)};

        EffectExpression expression;
        expression.operation_type = EffectOperationType::kAdd;
        expression.operands = {expression_left, expression_right};

        EXPECT_EQ(Evaluate(expression), 4545_fp);
    }
}

// Because OmegaPower is an percentage type we do a special multiplication
// As we can't use floats
TEST_F(EffectExpressionTest, OmegaPowerMultiplyWithValue)
{
    // Sender
    sender_stats.live.Set(StatType::kOmegaPowerPercentage, 50_fp);
    {
        // 100 * OmegaPower (Sender) = 100 * 0.5 = 50
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromValue(100_fp),
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
        };
        EXPECT_EQ(Evaluate(expression), 50_fp);
    }

    // Sender reverse
    sender_stats.live.Set(StatType::kOmegaPowerPercentage, 50_fp);
    {
        // OmegaPower (Sender) * 100 = 100 * 0.5 = 50
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromValue(100_fp),
        };
        EXPECT_EQ(Evaluate(expression), 50_fp);
    }

    // Receiver
    receiver_stats.live.Set(StatType::kOmegaPowerPercentage, 250_fp);
    {
        // 100 * OmegaPower (Receiver) = 100 * 2.5 = 250
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromValue(100_fp),
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
        };
        EXPECT_EQ(Evaluate(expression), 250_fp);
    }

    // Receiver reverse
    receiver_stats.live.Set(StatType::kOmegaPowerPercentage, 250_fp);
    {
        // OmegaPower (Receiver) * 100  = 100 * 2.5 = 250
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
            EffectExpression::FromValue(100_fp),
        };
        EXPECT_EQ(Evaluate(expression), 250_fp);
    }
}

TEST_F(EffectExpressionTest, OmegaPowerMultiplyWithStatPercentage)
{
    // Sender
    sender_stats.live.Set(StatType::kOmegaPowerPercentage, 50_fp);
    {
        // 10% MoveSpeed (Sender) * OmagePower (Sender) = 10 * 0.5 = 5
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
        };
        EXPECT_EQ(Evaluate(expression), 5_fp);
    }

    // Sender reverse
    sender_stats.live.Set(StatType::kOmegaPowerPercentage, 50_fp);
    {
        // OmagePower (Sender) * 10% MoveSpeed (Sender) = 10 * 0.5 = 5
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
        };
        EXPECT_EQ(Evaluate(expression), 5_fp);
    }

    // Receiver
    receiver_stats.live.Set(StatType::kOmegaPowerPercentage, 250_fp);
    {
        // 10% MoveSpeed (Sender) * SkilPower (Receiver) = 10 * 2.5 = 25
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
        };
        EXPECT_EQ(Evaluate(expression), 25_fp);
    }

    // Receiver reverse
    receiver_stats.live.Set(StatType::kOmegaPowerPercentage, 250_fp);
    {
        // OmagePower (Receiver) * 10% MoveSpeed (Sender) = 10 * 2.5 = 25
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
        };
        EXPECT_EQ(Evaluate(expression), 25_fp);
    }
}

TEST_F(EffectExpressionTest, OmagePowerMultiplyWithMultiple)
{
    // Sender
    sender_stats.live.Set(StatType::kOmegaPowerPercentage, 50_fp);
    {
        // OmagePower (Sender) * 10% MoveSpeed (Sender) * 200  = 200 * 0.5 * 10 = 1000
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromValue(200_fp),
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
        };
        EXPECT_EQ(Evaluate(expression), 1000_fp);
    }

    // Sender
    receiver_stats.live.Set(StatType::kOmegaPowerPercentage, 250_fp);
    {
        // OmagePower (Receiver) * 200 * 10% MoveSpeed (Sender) = 200 * 2.5 * 10 = 5000
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
            EffectExpression::FromValue(200_fp),
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
        };
        EXPECT_EQ(Evaluate(expression), 5000_fp);
    }
}

TEST_F(EffectExpressionTest, OmagePowerMultiplyWithOmegaPower)
{
    // Sender and target
    sender_stats.live.Set(StatType::kOmegaPowerPercentage, 50_fp);
    receiver_stats.live.Set(StatType::kOmegaPowerPercentage, 250_fp);
    {
        // OmagePower (Sender) * OmagePower (Receiver) * 200 * 10% MoveSpeed (Sender) = 0.5 * 2.5 *
        // 200 * 10 = 2500
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
            EffectExpression::FromValue(200_fp),
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
        };
        EXPECT_EQ(Evaluate(expression), 2500_fp);
    }

    // Sender and target
    sender_stats.live.Set(StatType::kOmegaPowerPercentage, 50_fp);
    receiver_stats.live.Set(StatType::kOmegaPowerPercentage, 250_fp);
    {
        // OmagePower (Sender) * OmagePower (Receiver) * 200 * 10% MoveSpeed (Sender) = 0.5 * 2.5 *
        // 200 * 10 = 2500
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromValue(200_fp),
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
        };
        EXPECT_EQ(Evaluate(expression), 2500_fp);
    }

    // Sender and target
    sender_stats.live.Set(StatType::kOmegaPowerPercentage, 50_fp);
    receiver_stats.live.Set(StatType::kOmegaPowerPercentage, 250_fp);
    {
        // OmagePower (Sender) * OmagePower (Receiver) * 200 * 10% MoveSpeed (Sender) = 0.5 * 2.5 *
        // 200 * 10 = 2500
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromValue(200_fp),
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
        };
        EXPECT_EQ(Evaluate(expression), 2500_fp);
    }

    // Only percentages
    sender_stats.live.Set(StatType::kOmegaPowerPercentage, 50_fp);
    receiver_stats.live.Set(StatType::kOmegaPowerPercentage, 250_fp);
    {
        // OmagePower (Sender) * OmagePower (Receiver) = 0.5 * 2.5
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kMultiply;
        expression.operands = {
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
        };
        EXPECT_EQ(Evaluate(expression), "1.25"_fp);
    }
}

TEST_F(EffectExpressionTest, DivideRecursively)
{
    // Simple
    {
        EffectExpression expression = EffectExpression::FromValue(50_fp);

        EXPECT_EQ(Evaluate(expression), 50_fp);
        expression /= 2_fp;
        EXPECT_EQ(Evaluate(expression), 25_fp);
    }

    // More complicated
    {
        EffectExpression expression;
        expression.operation_type = EffectOperationType::kAdd;
        expression.operands = {
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
            EffectExpression::FromValue(200_fp),
            EffectExpression::FromStat(
                StatType::kOmegaPowerPercentage,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kReceiver),
            EffectExpression::FromStatPercentage(
                10_fp,
                StatType::kMoveSpeedSubUnits,
                StatEvaluationType::kLive,
                ExpressionDataSourceType::kSender),
        };

        expression /= 2_fp;

        EXPECT_EQ(expression.base_value.value, 0_fp);
        ExpressionStatsSource stats_source;
        stats_source.Set(ExpressionDataSourceType::kSender, FullStatsData{}, nullptr);
        stats_source.Set(ExpressionDataSourceType::kReceiver, FullStatsData{}, nullptr);
        EXPECT_EQ(expression.Evaluate(ExpressionEvaluationContext{}, stats_source), 100_fp)
            << fmt::format("{}", expression);
    }
}

}  // namespace simulation
