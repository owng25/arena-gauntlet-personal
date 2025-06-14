// utility class

#include "utility/math.h"

// Check some assumptions
// This should basically never fail, see table here:
// https://en.cppreference.com/w/cpp/language/types
static_assert(sizeof(int) == 4, "int type is not 4 bytes on your platform");

namespace simulation
{
/**
 * Below code for Q15 trig adapted from
 * https://lab.dobergroup.org.ua/radiobase/portapack/portapack-eried/-/blob/182059b3c6c06a041d414993fcb7567702807b51/firmware/baseband/fxpt_atan2.cpp
 * https://github.com/evilpete/insteonrf/blob/master/Src/fxpt_atan2.c
 * https://github.com/l3VGV/fxpt_atan2/blob/master/main.c
 *
 */
uint16_t Math::ATan2(const int16_t y, const int16_t x)
{
    static constexpr int16_t k1 = 2847;
    static constexpr int16_t k2 = 11039;

    if (x == y)
    {  // x/y or y/x would return -1 since 1 isn't representable
        if (y > 0)
        {  // 1/8
            return 8192;
        }
        if (y < 0)
        {  // 5/8
            return 40960;
        }

        // x = y = 0
        return 0;
    }

    const int16_t nabs_y = S16NAbs(y), nabs_x = S16NAbs(x);
    if (nabs_x < nabs_y)
    {  // octants 1, 4, 5, 8
        const int16_t y_over_x = Q15Div(y, x);
        const int16_t correction = Q15Mul(k1, S16NAbs(y_over_x));
        const int16_t unrotated = Q15Mul(static_cast<int16_t>(k2 + correction), y_over_x);
        if (x > 0)
        {
            // octants 1, 8
            return static_cast<uint16_t>(unrotated);
        }

        // octants 4, 5
        return static_cast<uint16_t>(32768 + unrotated);
    }

    // octants 2, 3, 6, 7
    const int16_t x_over_y = Q15Div(x, y);
    const int16_t correction = Q15Mul(k1, S16NAbs(x_over_y));
    const int16_t unrotated = Q15Mul(static_cast<int16_t>(k2 + correction), x_over_y);
    if (y > 0)
    {
        // octants 2, 3
        return static_cast<uint16_t>(16384 - unrotated);
    }

    // octants 6, 7
    return static_cast<uint16_t>(49152 - unrotated);
}

int16_t Math::S16NAbs(const int16_t j)
{
#if (((int16_t) - 1) >> 1) == ((int16_t) - 1)
    // signed right shift sign-extends (arithmetic)
    const int16_t negSign = static_cast<int16_t>(~(j >> 15));  // splat sign bit into all 16 and complement
    // if j is positive (negSign is -1), xor will invert j and sub will add 1
    // otherwise j is unchanged
    return static_cast<int16_t>((j ^ negSign) - negSign);
#else
    return (j < 0 ? j : -j);
#endif
}

/**
 * Q15 (1.0.15 fixed point) multiplication. Various common rounding modes are in
 * the function definition for reference (and preference).
 *
 * @param j 16-bit signed integer representing -1 to (1 - (2**-15)), inclusive
 * @param k same format as j
 * @return product of j and k, in same format
 */
int16_t Math::Q15Mul(const int16_t j, const int16_t k)
{
    const int32_t intermediate = j * static_cast<int32_t>(k);
    return static_cast<int16_t>((intermediate + ((intermediate & 0x7FFF) == 0x4000 ? 0 : 0x4000)) >> 15);
}

/**
 * Q15 (1.0.15 fixed point) division (non-saturating). Be careful when using
 * this function, as it does not behave well when the result is out-of-range.
 *
 * Value is not defined if numerator is greater than or equal to denominator.
 *
 * @param numer 16-bit signed integer representing -1 to (1 - (2**-15))
 * @param denom same format as numer; must be greater than numerator
 * @return numer / denom in same format as numer and denom
 */
int16_t Math::Q15Div(const int16_t numer, const int16_t denom)
{
    return static_cast<int16_t>((static_cast<int32_t>(numer) << 15) / denom);
}

}  // namespace simulation
