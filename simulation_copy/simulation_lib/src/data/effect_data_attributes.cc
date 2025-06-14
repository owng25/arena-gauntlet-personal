#include "effect_data_attributes.h"

#include <sstream>

#include "utility/enum.h"
#include "utility/math.h"
#include "utility/string.h"
#include "utility/struct_formatting_helper.h"

namespace simulation
{

void EffectDataAttributes::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.FieldIf("excess_heal_to_shield", excess_heal_to_shield);
    h.FieldIf("missing_health_percentage_to_health", missing_health_percentage_to_health);
    h.FieldIf("to_shield_percentage", to_shield_percentage);
    h.FieldIf("to_shield_duration_ms", to_shield_duration_ms);
    h.FieldIf("max_health_percentage_to_health", max_health_percentage_to_health);
    h.FieldIf("cleanse_negative_states", cleanse_negative_states);
    h.FieldIf("cleanse_conditions", cleanse_conditions);
    h.FieldIf("cleanse_bots", cleanse_bots);
    h.FieldIf("cleanse_dots", cleanse_dots);
    h.FieldIf("cleanse_debuffs", cleanse_debuffs);
    h.FieldIf("shield_bypass", shield_bypass);
    h.Write("}}");
}

}  // namespace simulation
