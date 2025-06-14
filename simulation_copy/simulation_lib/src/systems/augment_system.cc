#include "augment_system.h"

#include "components/abilities_component.h"
#include "components/augment_component.h"
#include "components/stats_component.h"
#include "data/containers/game_data_container.h"
#include "ecs/world.h"
#include "utility/augment_helper.h"

namespace simulation
{
void AugmentSystem::Init(World* world)
{
    System::Init(world);
}

void AugmentSystem::TimeStep(const Entity& entity)
{
    ILLUVIUM_PROFILE_FUNCTION();

    AddAugmentDataToEntity(entity);
}

void AugmentSystem::PreBattleStarted(const Entity& entity)
{
    AddAugmentDataToEntity(entity);
}

void AugmentSystem::AddAugmentDataToEntity(const Entity& entity)
{
    static constexpr std::string_view method_name = "AugmentSystem::AddAugmentDataToEntity";

    // Not active
    if (!entity.IsActive())
    {
        return;
    }

    // Add innate abilities from augments
    if (!entity.Has<StatsComponent, AugmentComponent>())
    {
        return;
    }

    // Once per entity
    const EntityID entity_id = entity.GetID();
    if (entities_activated_.count(entity_id) > 0)
    {
        return;
    }
    entities_activated_.insert(entity_id);
    LogDebug(entity_id, "{}", method_name);

    auto& abilities_component = entity.Get<AbilitiesComponent>();
    const auto& augment_component = entity.Get<AugmentComponent>();

    const auto& equipped_augments = augment_component.GetEquippedAugments();
    const auto& augment_helper = world_->GetAugmentHelper();

    for (const auto& equipped_augment : equipped_augments)
    {
        ILLUVIUM_ENSURE_ENUM_SIZE(AugmentType, 5);

        const AugmentType augment_type = augment_helper.GetAugmentType(equipped_augment.type_id);
        switch (augment_type)
        {
        case AugmentType::kNormal:
        case AugmentType::kComponent:
            LogDebug(entity_id, "{} - Adding innate abilities for new augment: {}", method_name, equipped_augment);

            // Add all abilities
            if (const auto augment_data = world_->GetGameDataContainer().GetAugmentData(equipped_augment.type_id))
            {
                for (const auto& ability_data : augment_data->innate_abilities)
                {
                    abilities_component.AddDataInnateAbility(ability_data);
                }
            }
            break;

        case AugmentType::kGreaterPower:
            // Add all abilities
            LogDebug(
                entity_id,
                "{} - Adding innate abilities for legendary augment: {}",
                method_name,
                equipped_augment);
            if (const auto augment_data = world_->GetGameDataContainer().GetAugmentData(equipped_augment.type_id))
            {
                for (const auto& ability_data : augment_data->innate_abilities)
                {
                    abilities_component.AddDataInnateAbility(ability_data);
                }
            }
            else
            {
                LogErr(entity_id, "{} - Can't find data for legendary augment: {}", method_name, equipped_augment);
            }
            break;

        case AugmentType::kSynergyBonus:
            // Synergy augments can have any optional abilities
            if (const auto augment_data = world_->GetGameDataContainer().GetAugmentData(equipped_augment.type_id))
            {
                if (!augment_data->innate_abilities.empty())
                {
                    LogDebug(
                        entity_id,
                        "{} - Adding innate abilities for synergy legendary augment: {}",
                        method_name,
                        equipped_augment);
                    for (const auto& ability_data : augment_data->innate_abilities)
                    {
                        abilities_component.AddDataInnateAbility(ability_data);
                    }
                }
            }
            else
            {
                LogErr(entity_id, "{} - Can't find data for legendary augment: {}", method_name, equipped_augment);
            }

            LogDebug(
                entity_id,
                "{} - Synergy legendary augment = {} is not handled here. See SynergySystem::AddSynergiesFromAugments.",
                method_name,
                equipped_augment);
            break;

        case AugmentType::kNone:
        default:
            LogErr(entity_id, "{} - invalid augment_type for augment: {}", method_name, equipped_augment);
            break;
        }
    }
}

}  // namespace simulation
