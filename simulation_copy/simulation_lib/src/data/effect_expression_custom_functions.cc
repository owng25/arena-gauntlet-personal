#include "effect_expression_custom_functions.h"

#include "components/shield_component.h"
#include "ecs/world.h"
#include "utility/entity_helper.h"

namespace simulation
{

FixedPoint EffectExpressionCustomFunctions::GetShieldAmount(
    const ExpressionDataSourceType data_srouce_type,
    const ExpressionEvaluationContext& context,
    const ExpressionStatsSource&)
{
    ILLUVIUM_ENSURE_ENUM_SIZE(ExpressionDataSourceType, 4);
    EntityID data_source_entity_id = kInvalidEntityID;
    switch (data_srouce_type)
    {
    case ExpressionDataSourceType::kReceiver:
        data_source_entity_id = context.GetReceiverID();
        break;
    case ExpressionDataSourceType::kSender:
        data_source_entity_id = context.GetSenderID();
        break;
    default:
        break;
    }

    const World& world = *context.GetWorld();
    if (data_source_entity_id == kInvalidEntityID || !world.HasEntity(data_source_entity_id))
    {
        return 0_fp;
    }

    const auto& entity = world.GetByID(data_source_entity_id);

    if (EntityHelper::IsAShield(entity))
    {
        const ShieldComponent& shield_component = entity.Get<ShieldComponent>();
        return shield_component.GetValue();
    }

    return world.GetShieldTotal(entity);
}

}  // namespace simulation
