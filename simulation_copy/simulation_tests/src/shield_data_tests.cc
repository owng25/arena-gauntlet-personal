#include "data/shield_data.h"
#include "gtest/gtest.h"

namespace simulation
{
class ShieldDataTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        data.value = GetCounter();
        data.duration_ms = GetCounter().AsInt();
    }

    FixedPoint GetCounter()
    {
        const auto counter = combat_class_counter;
        combat_class_counter += 1_fp;
        return counter;
    }

    FixedPoint combat_class_counter = 1_fp;
    ShieldData data;
};

TEST_F(ShieldDataTest, GetValues)
{
    int counter = 1;
    EXPECT_EQ(data.value.AsInt(), counter);
    counter += 1;
    EXPECT_EQ(data.duration_ms, counter);
}

}  // namespace simulation
