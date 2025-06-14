#include "utility/hex_grid_position.h"

#include "utility/struct_formatting_helper.h"

namespace simulation
{
void HexGridPosition::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("q", q);
    h.Field("r", r);
    h.Write("}}");
}

std::string HexGridPosition::ToString() const
{
    return fmt::format("{}", *this);
}

nlohmann::json HexGridPosition::ToJSONObject() const
{
    nlohmann::json json_object = nlohmann::json::object();
    json_object["Q"] = q;
    json_object["R"] = r;
    return json_object;
}
}  // namespace simulation
