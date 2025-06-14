#include "encounter_mod_type_id.h"

#include "utility/struct_formatting_helper.h"

namespace simulation
{
void EncounterModTypeID::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("name", name);
    h.Field("stage", stage);
    h.Write("}}");
}
}  // namespace simulation
