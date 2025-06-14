#include "effect_data.h"

#include <sstream>

#include "data/ability_data.h"
#include "data/effect_enums.h"
#include "data/skill_data.h"
#include "utility/custom_formatter.h"
#include "utility/enum.h"
#include "utility/math.h"
#include "utility/string.h"
#include "utility/struct_formatting_helper.h"

namespace simulation
{

EffectData EffectData::CreateDeepCopy() const
{
    EffectData copy = *this;

    // Copy attached_effects
    copy.attached_effects.clear();
    for (const auto& attached_effect : attached_effects)
    {
        copy.attached_effects.push_back(attached_effect.CreateDeepCopy());
    }

    // Copy event_skills
    copy.event_skills.clear();
    for (const auto& [event_type, skill_data] : event_skills)
    {
        copy.event_skills[event_type] = skill_data->CreateDeepCopy();
    }

    // Copy attached_abilities
    copy.attached_abilities.clear();
    for (const auto& attached_ability : attached_abilities)
    {
        copy.attached_abilities.push_back(attached_ability->CreateDeepCopy());
    }

    copy.attached_effect_package_attributes = attached_effect_package_attributes.CreateDeepCopy();

    return copy;
}

void EffectLifetime::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");

    h.Field("duration_time_ms", duration_time_ms);
    h.Field("frequency_time_ms", frequency_time_ms);
    h.Field("max_stacks", max_stacks);
    h.Field("deactivate_if_validation_list_not_valid", deactivate_if_validation_list_not_valid);
    h.EnumFieldIfNotNone("overlap_process_type", overlap_process_type);

    if (is_consumable)
    {
        h.Field("is_consumable", is_consumable);
        h.Field("activations_until_expiry", activations_until_expiry);
        h.Field("activated_by", activated_by);
        h.Field("consumable_activation_frequency", consumable_activation_frequency);
    }

    if (blocks_until_expiry != simulation::kTimeInfinite)
    {
        h.Field("blocks_until_expiry", blocks_until_expiry);
    }
    h.Write("}}");
}

void EffectState::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("is_critical", is_critical);
    h.Field("source_context", source_context);
    h.Write("}}");
}

bool EffectDataReplacements::ContainsID(const std::string& id) const
{
    return VectorHelper::ContainsValuePredicate(
        replacements,
        [&](const EffectData& value_in_vector)
        {
            return value_in_vector.id == id;
        });
}

const EffectData* EffectDataReplacements::GetEffectDataForID(const std::string& id) const
{
    for (const EffectData& effect_data : replacements)
    {
        if (effect_data.id == id)
        {
            return &effect_data;
        }
    }

    return nullptr;
}

void EffectData::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.Field("type_id", type_id);
    h.FieldIf(!id.empty(), "id", id);
    h.FieldIf(!event_skills.empty(), "event_skills.size()", event_skills.size());
    h.FieldIf(!required_conditions.IsEmpty(), "required_conditions", required_conditions);
    h.FieldIf(UsesDurationTime() || lifetime.duration_time_ms, "lifetime", lifetime);
    h.FieldIf(UsesExpression(), "expression", GetExpression());
    h.FieldIf(!attached_effects.empty(), "attached_effects.size()", attached_effects.size());
    h.FieldIf(
        !attached_effect_package_attributes.IsEmpty(),
        "attached_effect_package_attributes",
        attached_effect_package_attributes);
    h.FieldIf(!attached_abilities.empty(), "attached_abilities.size()", attached_abilities.size());

    // Fields depend on effect type
    switch (type_id.type)
    {
    case EffectType::kEmpower:
    case EffectType::kDisempower:
        h.Field("activated_by", lifetime.activated_by);
        h.Field("is_used_as_global_effect_attribute", is_used_as_global_effect_attribute);
        h.FieldIf(is_used_as_global_effect_attribute, "empower_for_effect_type_id", empower_for_effect_type_id);
        break;

    case EffectType::kDisplacement:
        h.Field("displacement_distance_sub_units", displacement_distance_sub_units);
        break;

    case EffectType::kExecute:
        h.Field("ability_types", ability_types);
        break;

    case EffectType::kBlink:
        h.Field("blink_target", blink_target);
        h.Field("blink_delay_ms", blink_delay_ms);
        break;

    default:
        break;
    }

    // Add optional attributes
    h.FieldIf(!attributes.IsEmpty(), "attributes", attributes);

    // TODO(vampy): Add to string for the other types inside the effect data
    h.Write("}}");
}

}  // namespace simulation
