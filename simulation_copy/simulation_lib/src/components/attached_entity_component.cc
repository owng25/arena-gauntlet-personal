#include "attached_entity_component.h"

#include "ecs/entity.h"
#include "ecs/world.h"
#include "utility/vector_helper.h"

namespace simulation
{
void AttachedEntityComponent::RemoveAttachedEntity(const EntityID entity_id)
{
    VectorHelper::EraseValue(attached_entities_, entity_id);
}

std::vector<AttachedEffectStatePtr> AttachedEntityComponent::GetEmpowers() const
{
    const auto world = GetOwnerWorld();
    std::vector<AttachedEffectStatePtr> entity_empowers;
    for (const auto& attached_entity : attached_entities_)
    {
        const auto& entity = world->GetByID(attached_entity.id);

        const auto& entity_effects_component = entity.Get<AttachedEffectsComponent>();
        const auto& empowers = entity_effects_component.GetEmpowers();
        for (const auto& empower : empowers)
        {
            entity_empowers.push_back(empower);
        }
    }

    return entity_empowers;
}

std::vector<AttachedEffectStatePtr> AttachedEntityComponent::GetDisempowers() const
{
    const auto world = GetOwnerWorld();
    std::vector<AttachedEffectStatePtr> entity_disempowers;
    for (const auto& attached_entity : attached_entities_)
    {
        const auto& entity = world->GetByID(attached_entity.id);

        const auto& entity_effects_component = entity.Get<AttachedEffectsComponent>();
        const auto& disempowers = entity_effects_component.GetDisempowers();
        for (const auto& disempower : disempowers)
        {
            entity_disempowers.push_back(disempower);
        }
    }

    return entity_disempowers;
}

}  // namespace simulation
