#include "ability_data.h"

#include <sstream>

#include "utility/custom_formatter.h"
#include "utility/enum.h"
#include "utility/string.h"
#include "utility/struct_formatting_helper.h"

namespace simulation
{

void AbilitiesData::MergeAbilities(const AbilitiesData& other)
{
    // No abilities
    if (other.abilities.empty())
    {
        return;
    }

    // Copy abilities
    VectorHelper::AppendVector(abilities, other.abilities);

    // Copy other data if different from the default
    // For ints the default is 0 and for enums the default is kNone
    // clang-format off
    if (selection_type != AbilitySelectionType::kNone &&
        other.selection_type != AbilitySelectionType::kNone &&
        selection_type != other.selection_type)
    {
        selection_type = other.selection_type;
    }
    if (activation_cadence != 0 &&
        other.activation_cadence != 0 &&
        activation_cadence != other.activation_cadence)
    {
        activation_cadence = other.activation_cadence;
    }
    if (activation_check_value != 0_fp &&
        other.activation_check_value != 0_fp &&
        activation_check_value != other.activation_check_value)
    {
        activation_check_value = other.activation_check_value;
    }
    if (activation_check_stat_type != StatType::kNone &&
        other.activation_check_stat_type != StatType::kNone &&
        activation_check_stat_type != other.activation_check_stat_type)
    {
        activation_check_stat_type = other.activation_check_stat_type;
    }
    // clang-format on
}

bool AbilityActivationTriggerData::IsEmpty() const
{
    return trigger_type == ActivationTriggerType::kNone && max_activations == 0 && activation_time_limit_ms == 0 &&
           activate_every_time_ms == 0 && health_lower_limit_percentage == 0_fp && number_of_abilities_activated == 1 &&
           activation_radius_units == 0 && damage_threshold == 0_fp && number_of_effect_packages_received == 1 &&
           number_of_skills_deployed == 1 && comparison_type == ComparisonType::kEqual &&
           number_of_effect_packages_received_modulo == 0 && trigger_value != 1 &&
           required_receiver_conditions.IsEmpty() && required_receiver_conditions.IsEmpty() && damage_types.IsEmpty() &&
           !every_x;
}

void AbilityActivationTriggerData::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.EnumFieldIfNotNone("trigger_type", trigger_type);
    h.FieldIf("max_activations", max_activations);
    h.FieldIf("activation_time_limit_ms", activation_time_limit_ms);
    h.FieldIf("activate_every_time_ms", activate_every_time_ms);
    h.FieldIf("health_lower_limit_percentage", health_lower_limit_percentage);
    h.FieldIf("number_of_abilities_activated", number_of_abilities_activated);
    h.FieldIf("every_x", every_x);
    h.FieldIf("activation_radius_units", activation_radius_units);
    h.FieldIf(!required_sender_conditions.IsEmpty(), "required_sender_conditions", required_sender_conditions);
    h.FieldIf(!required_receiver_conditions.IsEmpty(), "required_receiver_conditions", required_receiver_conditions);
    h.FieldIf(!damage_types.IsEmpty(), "damage_types", damage_types);
    h.FieldIf("damage_threshold", damage_threshold);
    h.FieldIf(
        number_of_effect_packages_received != 1,
        "number_of_effect_packages_received",
        number_of_effect_packages_received);
    h.FieldIf(comparison_type != simulation::ComparisonType::kEqual, "comparison_type", comparison_type);
    h.FieldIf(trigger_value != 1, "trigger_value", trigger_value);
    h.FieldIf(", number_of_skills_deployed", number_of_skills_deployed);
    h.FieldIf("number_of_effect_packages_received_modulo", number_of_effect_packages_received_modulo);

    h.Write("}}");
}

void AbilityData::FormatTo(fmt::format_context& ctx) const
{
    StructFormattingHelper h(ctx);
    h.Write("{{");
    h.CustomField("name = `{}`", name);
    h.Field("total_duration_ms", total_duration_ms);
    h.Field("skills size", skills.size());
    h.FieldIf(!movement_lock, "movement locked ", movement_lock);
    h.FieldIf("is_use_once", is_use_once);
    h.FieldIf(!activation_trigger_data.IsEmpty(), "activation_trigger_data", activation_trigger_data);
    h.Write("}}");
}

}  // namespace simulation
