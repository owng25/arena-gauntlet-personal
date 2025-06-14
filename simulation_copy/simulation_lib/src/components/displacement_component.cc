#include "displacement_component.h"

#include "components/attached_effects_component.h"

namespace simulation
{
void DisplacementComponent::SetAttachedEffectState(const AttachedEffectState& effect_state)
{
    displacement_effect_state_ = std::static_pointer_cast<const AttachedEffectState>(effect_state.shared_from_this());
}

std::shared_ptr<const AttachedEffectState> DisplacementComponent::GetAttachedEffectState() const
{
    return displacement_effect_state_.lock();
}

void DisplacementComponent::ClearAttachedEffectState()
{
    displacement_effect_state_.reset();
}
}  // namespace simulation
