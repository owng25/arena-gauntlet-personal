#include "augment_instance_data.h"

#include "utility/enum.h"
#include "utility/struct_formatting_helper.h"

namespace simulation
{
void AugmentInstanceData::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("id", id);
    h.Field("type_id", type_id);
    h.Write("}}");
}
}  // namespace simulation
