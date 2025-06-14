#include "augment_type_id.h"

#include "utility/struct_formatting_helper.h"

namespace simulation
{
void AugmentTypeID::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("name", name);
    h.Field("stage", stage);
    h.Field("variation", variation);
    h.Write("}}");
}
}  // namespace simulation
