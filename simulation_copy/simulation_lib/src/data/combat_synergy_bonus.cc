#include "combat_synergy_bonus.h"

#include "utility/enum.h"

namespace simulation
{
void CombatSynergyBonus::FormatTo(fmt::format_context& ctx) const
{
    if (IsCombatAffinity())
    {
        fmt::format_to(ctx.out(), "{}", GetAffinity());
    }
    else
    {
        fmt::format_to(ctx.out(), "{}", GetClass());
    }
}
}  // namespace simulation
