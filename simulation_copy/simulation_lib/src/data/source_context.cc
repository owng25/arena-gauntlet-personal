#include "source_context.h"

#include "utility/enum.h"
#include "utility/string.h"
#include "utility/struct_formatting_helper.h"

namespace simulation
{
void SourceContextData::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.FieldIf(!sources.empty(), "sources", sources);
    h.CustomFieldIf(!ability_name.empty(), "ability_name = `{}`", ability_name);
    h.CustomFieldIf(!skill_name.empty(), "skill_name = `{}`", skill_name);
    h.EnumFieldIfNotNone("combat_unit_ability_type", combat_unit_ability_type);
    h.FieldIf(!combat_synergy.IsEmpty(), "combat_synergy", combat_synergy);
    h.Write("}}");
}
}  // namespace simulation
