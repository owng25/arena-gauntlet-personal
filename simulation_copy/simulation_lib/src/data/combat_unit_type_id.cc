#include "combat_unit_type_id.h"

#include "utility/enum.h"
#include "utility/string.h"

namespace simulation
{

void CombatUnitTypeID::FormatTo(fmt::format_context& ctx) const
{
    if (!line_name.empty())
    {
        fmt::format_to(ctx.out(), "{}", line_name);
        if (type != CombatUnitType::kRanger)
        {
            fmt::format_to(ctx.out(), "{}", stage);
        }
    }
}

}  // namespace simulation
