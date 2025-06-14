#pragma once

#include "data/constants.h"
#include "data/effect_data.h"
#include "ecs/component.h"
#include "utility/hex_grid_position.h"

namespace simulation
{
class AttachedEffectState;

/* -------------------------------------------------------------------------------------------------------
 * DisplacementComponent
 *
 * This class handles displacements for an entity
 * --------------------------------------------------------------------------------------------------------
 */
class DisplacementComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<DisplacementComponent>(*this);
    }

    // Component initialisation
    void Init() override {}

    // Entity that sent the displacement
    void SetSenderID(const EntityID sender_id)
    {
        sender_id_ = sender_id;
    }
    EntityID GetSenderID() const
    {
        return sender_id_;
    }

    // Gets/sets the current displacement type
    EffectDisplacementType GetDisplacementType() const
    {
        return displacement_type_;
    }
    void SetDisplacementType(const EffectDisplacementType displacement_type)
    {
        displacement_type_ = displacement_type;
    }

    // Gets/sets the target position of the displacement
    const HexGridPosition& GetDisplacementTargetPosition() const
    {
        return displacement_target_position_;
    }
    void SetDisplacementTargetPosition(const HexGridPosition& displacement_target_position)
    {
        displacement_target_position_ = displacement_target_position;
    }

    // Gets/sets the end displacement skill
    const std::shared_ptr<SkillData>& GetDisplacementEndSkill() const
    {
        return displacement_end_skill_;
    }
    void SetDisplacementEndSkill(std::shared_ptr<SkillData> displacement_end_skill)
    {
        displacement_end_skill_ = std::move(displacement_end_skill);
    }
    void ClearDisplacementEndSkill()
    {
        displacement_end_skill_.reset();
    }

    void SetAttachedEffectState(const AttachedEffectState& effect_state);
    std::shared_ptr<const AttachedEffectState> GetAttachedEffectState() const;
    void ClearAttachedEffectState();

private:
    // Displacement sender
    EntityID sender_id_ = kInvalidEntityID;

    // Type of displacement
    EffectDisplacementType displacement_type_ = EffectDisplacementType::kNone;

    // Displacement target position, if applicable
    HexGridPosition displacement_target_position_ = kInvalidHexHexGridPosition;

    // Skills that trigger on displacement end event
    std::shared_ptr<SkillData> displacement_end_skill_;

    std::weak_ptr<const AttachedEffectState> displacement_effect_state_;
};

}  // namespace simulation
