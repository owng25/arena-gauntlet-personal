#include "weapon_instance_data.h"

#include "utility/string.h"
#include "utility/struct_formatting_helper.h"

namespace simulation
{

void CombatWeaponBaseInstanceData::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("id", id);
    h.Field("type_id", type_id);
    h.Write("}}");
}

void CombatWeaponInstanceData::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("id", id);
    h.Field("type_id", type_id);

    h.CustomFieldIf(
        !equipped_amplifiers.empty(),
        "equipped_amplifiers = [{}]",
        String::JoinBy(equipped_amplifiers, ", "));

    h.Write("}}");
}
}  // namespace simulation
