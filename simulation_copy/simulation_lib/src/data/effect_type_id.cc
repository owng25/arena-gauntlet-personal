#include "effect_type_id.h"

#include <sstream>

#include "utility/enum.h"
#include "utility/math.h"
#include "utility/string.h"
#include "utility/struct_formatting_helper.h"

namespace simulation
{

void EffectTypeID::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("type", type);
    h.EnumFieldIfNotNone("positive_state", positive_state);
    h.EnumFieldIfNotNone("negative_state", negative_state);
    h.EnumFieldIfNotNone("condition_type", condition_type);
    h.EnumFieldIfNotNone("displacement_type", displacement_type);
    h.EnumFieldIfNotNone("propagation_type", propagation_type);
    h.EnumFieldIfNotNone("heal_type", heal_type);
    h.FieldIf(UsesDamageType(), "damage_type", damage_type);
    h.FieldIf(UsesStatType(), "stat_type", stat_type);
    h.Write("}}");
}

}  // namespace simulation
