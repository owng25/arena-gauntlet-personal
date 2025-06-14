#include "fixed_point.h"

namespace simulation
{
std::string FixedPoint::ToString() const
{
    return fmt::format("{}", *this);
}
}  // namespace simulation
