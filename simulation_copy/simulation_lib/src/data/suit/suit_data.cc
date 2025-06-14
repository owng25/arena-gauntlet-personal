#include "suit_data.h"

#include "utility/enum.h"
#include "utility/string.h"
#include "utility/struct_formatting_helper.h"

namespace simulation
{

void CombatUnitSuitData::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("type_id", type_id);
    h.Write("}}");
}

}  // namespace simulation
