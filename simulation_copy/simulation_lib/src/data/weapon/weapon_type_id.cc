#include "weapon_type_id.h"

#include "utility/enum.h"
#include "utility/struct_formatting_helper.h"

namespace simulation
{
void CombatWeaponTypeID::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("name", name);
    h.Field("stage", stage);
    h.Field("combat_affinity", combat_affinity);
    h.Field("variation", variation);
    h.Write("}}");
}
}  // namespace simulation
