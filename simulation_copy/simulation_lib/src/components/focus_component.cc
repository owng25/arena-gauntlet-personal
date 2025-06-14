#include "focus_component.h"

#include "ecs/entity.h"

namespace simulation
{
std::shared_ptr<Component> FocusComponent::CreateDeepCopyFromInitialState() const
{
    auto new_component = std::make_shared<FocusComponent>(*this);
    new_component->focus_.reset();
    new_component->ResetFocus();
    return new_component;
}

std::shared_ptr<Entity> FocusComponent::GetFocus() const
{
    return focus_.lock();
}

void FocusComponent::SetFocus(const std::shared_ptr<Entity>& focus)
{
    if (focus != focus_.lock())
    {
        focus_ = focus;
    }
}

EntityID FocusComponent::GetFocusID() const
{
    return IsFocusSet() ? GetFocus()->GetID() : kInvalidEntityID;
}

bool FocusComponent::IsFocusActive() const
{
    return IsFocusSet() ? GetFocus()->IsActive() : false;
}

}  // namespace simulation
