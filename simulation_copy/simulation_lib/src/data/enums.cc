#include "enums.h"

#include <sstream>

#include "utility/enum.h"
#include "utility/string.h"

namespace simulation
{

bool EvaluateComparison(const ComparisonType comparison_type, const FixedPoint left_value, const FixedPoint right_value)
{
    switch (comparison_type)
    {
    case ComparisonType::kGreater:
    {
        if (left_value > right_value)
        {
            return true;
        }
        break;
    }
    case ComparisonType::kLess:
    {
        if (left_value < right_value)
        {
            return true;
        }
        break;
    }
    case ComparisonType::kEqual:
    {
        if (left_value == right_value)
        {
            return true;
        }

        break;
    }
    default:
        assert(false);
        break;
    }

    return false;
}

}  // namespace simulation
