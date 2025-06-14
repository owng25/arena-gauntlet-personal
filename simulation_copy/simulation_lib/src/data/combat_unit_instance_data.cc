#include "combat_unit_instance_data.h"

#include "fmt/ranges.h"
#include "utility/enum.h"
#include "utility/string.h"
#include "utility/struct_formatting_helper.h"

namespace simulation
{
void CombatUnitInstanceData::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("id", id);
    h.Field("team", team);
    h.Field("position", position);
    h.Field("level", level);
    h.Field("bonded_id", bonded_id);

    h.EnumFieldIfNotNone("dominant_combat_affinity", dominant_combat_affinity);
    h.EnumFieldIfNotNone("dominant_combat_class", dominant_combat_class);

    if (equipped_weapon.IsValid())
    {
        h.Field("equipped_weapon", equipped_weapon);
    }
    if (equipped_suit.IsValid())
    {
        h.Field("equipped_suit", equipped_suit);
    }

    if (!equipped_augments.empty())
    {
        h.Field("equipped_augments", equipped_augments);
    }

    if (!equipped_consumables.empty())
    {
        h.Field("equipped_consumable", equipped_consumables);
    }

    h.FieldIf(!stats_overrides.empty(), "stats_overrides", stats_overrides);
    h.FieldIf(!finish.empty(), "finish", finish);

    h.Write("}}");
}

}  // namespace simulation
