#pragma once

#include "components/attached_effects_component.h"
#include "data/constants.h"
#include "data/enums.h"
#include "ecs/component.h"

namespace simulation
{

struct AttachedEntity
{
    AttachedEntityType type = AttachedEntityType::kNone;
    EntityID id = kInvalidEntityID;

    bool operator<(const AttachedEntity& other)
    {
        return id < other.id;
    }
    bool operator<(const EntityID& other_id)
    {
        return id < other_id;
    }
    bool operator==(const AttachedEntity& other) const
    {
        return id == other.id;
    }
    bool operator==(const EntityID& other) const
    {
        return id == other;
    }
};

/* -------------------------------------------------------------------------------------------------------
 * AttachedEntityComponent
 *
 * This class keeps track of the attached entities for an entity
 * --------------------------------------------------------------------------------------------------------
 */
class AttachedEntityComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<AttachedEntityComponent>(*this);
    }

    // Component initialisation
    void Init() override {}

    const std::vector<AttachedEntity>& GetAttachedEntities() const
    {
        return attached_entities_;
    }

    void AddAttachedEntity(const AttachedEntity& entity)
    {
        attached_entities_.push_back(entity);
    }
    void RemoveAttachedEntity(const AttachedEntity& entity)
    {
        RemoveAttachedEntity(entity.id);
    }
    void RemoveAttachedEntity(const EntityID entity_id);

    // Gets all the active empowers
    std::vector<AttachedEffectStatePtr> GetEmpowers() const;

    // Gets all the active disempowers
    std::vector<AttachedEffectStatePtr> GetDisempowers() const;

private:
    std::vector<AttachedEntity> attached_entities_;
};

}  // namespace simulation
