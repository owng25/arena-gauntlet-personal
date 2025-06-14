#include "utility/ivector2d.h"

#include "utility/struct_formatting_helper.h"

namespace simulation
{

void IVector2D::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("x", x);
    h.Field("y", y);
    h.Write("}}");
}

}  // namespace simulation
