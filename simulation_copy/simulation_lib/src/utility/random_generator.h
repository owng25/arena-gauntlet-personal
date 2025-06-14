#pragma once

#include <cassert>
#include <cstdint>
#include <limits>

namespace simulation
{
// Generates a deterministic set of pseudo-random numbers with a reasonable distribution.
// Original source: https://github.com/Bunny83/Utilities/blob/master/XorShift64.cs
// This should be an uniform distribution.
// We can't use std::uniform_int_distribution as it is not deterministic on every platform
// (see for example https://github.com/mc-imperial/jfs/issues/16).
class RandomGenerator
{
public:
    RandomGenerator() = default;
    explicit RandomGenerator(const uint64_t state) : state_{state}, is_initialised_{true}, original_state_(state) {}

    // Init the random generator with a seed
    void Init(const uint64_t seed)
    {
        is_initialised_ = true;
        state_ = seed ^ kSeedOffset;
        if (state_ == 0) state_ = kSeedOffset;

        original_state_ = state_;
    }

    // Gets the seed this random generator was initialized with
    uint64_t GetSeed() const
    {
        return original_state_ ^ kSeedOffset;
    }

    // Generates numbers in the range [min, max)
    uint64_t Range(const uint64_t min, const uint64_t max)
    {
        assert(is_initialised_);
        return FairRange(min, max);
    }

    // Gets the current state of the random generator
    uint64_t GetState() const
    {
        return state_;
    }

private:
    uint64_t Next()
    {
        // https://en.wikipedia.org/wiki/Xorshift#xorshift.2A
        state_ ^= state_ >> 12;
        state_ ^= state_ << 25;
        state_ ^= state_ >> 27;
        return state_ * 2685821657736338717ULL;  // 0x2545F4914F6CDD1DUL
    }

    uint64_t FairRange(const uint64_t min, const uint64_t max)
    {
        return min + FairRange(max - min);
    }

    uint64_t FairRange(const uint64_t range)
    {
        const uint64_t diff = kMaxUInt64 % range;

        const uint64_t divider = range / 4;

        // If aligned or range too big, just pick a number
        if (diff == 0 || divider == 0 || kMaxUInt64 / divider < 2) return Next() % range;

        uint64_t v = Next();

        // Avoid the last incomplete set
        while (kMaxUInt64 - v < diff) v = Next();

        return v % range;
    }

    uint64_t state_ = 0;
    bool is_initialised_ = false;

    // Keep track the original state right after Init() was called
    uint64_t original_state_ = 0;

    // Seed offset
    static constexpr uint64_t kSeedOffset = 13726359678912485784ULL;

    // Max uint64 number
    static constexpr uint64_t kMaxUInt64 = (std::numeric_limits<uint64_t>::max)();
};

}  // namespace simulation
