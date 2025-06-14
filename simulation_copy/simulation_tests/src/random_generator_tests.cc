#include <array>
#include <climits>

#include "gtest/gtest.h"
#include "utility/random_generator.h"

namespace simulation
{
TEST(RandomGenerator, Create)
{
    auto rg = RandomGenerator();
    ASSERT_EQ(0, rg.GetState());

    const uint64_t seed = 42;
    rg.Init(seed);
    ASSERT_EQ(seed, rg.GetSeed());
    ASSERT_EQ(13726359678912485810ULL, rg.GetState());

    // Create another random generator, that are initialized with the same seed
    auto rg2 = RandomGenerator();
    rg2.Init(seed);
    ASSERT_EQ(rg2.GetState(), rg.GetState());

    ASSERT_EQ(47, rg2.Range(1, 100));
    // rg2 should have a different state now
    ASSERT_NE(rg2.GetState(), rg.GetState());
    ASSERT_NE(rg.Range(1, 100), rg2.Range(1, 100));
    rg.Range(1, 100);  // make rg advance to the state of rg2
    ASSERT_EQ(rg.Range(1, 100), rg2.Range(1, 100));

    ASSERT_EQ(seed, rg.GetSeed());
    ASSERT_EQ(seed, rg2.GetSeed());
}

TEST(RandomGenerator, DifferentRanges)
{
    const uint64_t seed = 42;
    auto rg1 = RandomGenerator();
    auto rg2 = RandomGenerator();
    rg1.Init(seed);
    rg2.Init(seed);
    ASSERT_EQ(rg2.GetState(), rg1.GetState());
    ASSERT_EQ(47, rg1.Range(1, 100));
    ASSERT_EQ(0, rg2.Range(0, 10));
    ASSERT_EQ(3, rg2.Range(0, 10));
    ASSERT_EQ(45, rg1.Range(1, 100));
    // State should be the same
    ASSERT_EQ(rg2.GetState(), rg1.GetState());

    // rg2 should have a different state now
    // ASSERT_NE(rg2.GetState(), rg1.GetState());
}

TEST(RandomGenerator, CheckDistribution)
{
    constexpr uint64_t min = 0;
    constexpr uint64_t max = 100;
    constexpr int num_iterations = 10000;
    constexpr int perfect_frequency = num_iterations / static_cast<int>(max);
    constexpr int acceptable_frequency = static_cast<int>(static_cast<float>(perfect_frequency) / 1.6f);

    // Seed and generate numbers
    for (uint64_t seed = 0; seed < 100; seed++)
    {
        // Maps from key: Range() to value: Number of times seen
        std::array<int, max> seen_numbers{};

        auto rg = RandomGenerator();
        rg.Init(seed);
        for (int i = 0; i < num_iterations; i++)
        {
            seen_numbers[rg.Range(min, max)]++;
        }

        // Min
        ASSERT_GT(seen_numbers[min], 0);

        // Max - 1
        ASSERT_GT(seen_numbers[max - 1], 0);

        // Check
        // std::cout << "seed = " << seed << std::endl;
        for (size_t nr = 0; nr < seen_numbers.size(); nr++)
        {
            const int frequency = seen_numbers[nr];
            // std::cout << "nr = " << nr << ",  % = " << static_cast<float>(frequency) /
            // num_iterations * 100 << std::endl;
            ASSERT_GE(frequency, acceptable_frequency) << "frequency for nr is not in range: " << nr;
        }
        // std::cout << std::endl << std::endl;
    }
}

}  // namespace simulation
