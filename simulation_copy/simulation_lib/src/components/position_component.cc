#include "position_component.h"

#include "ecs/world.h"
#include "utility/grid_helper.h"

namespace simulation
{
std::vector<HexGridPosition> PositionComponent::GetTakenPositions() const
{
    const HexGridPosition& center = GetPosition();
    const int radius = GetRadius();
    return GridHelper::GetHexagonTakenPositions(center, radius);
}

int PositionComponent::AngleToPosition(const PositionComponent& other_position) const
{
    // Calls a helper function that uses world position to get the angle between two arbitrary hex
    // positions.
    return AngleToPosition(other_position.GetPosition());
}

int PositionComponent::AngleToPosition(const HexGridPosition& other_position) const
{
    // Use world position to get the angle between two arbitrary hex positions
    const IVector2D world_position_self = GetOwnerWorld()->ToWorldPosition(GetPosition());
    const IVector2D world_position_other = GetOwnerWorld()->ToWorldPosition(other_position);

    return world_position_self.AngleToPosition(world_position_other);
}

bool PositionComponent::IsPositionsIntersecting(const HexGridPosition& other_center, const int other_radius) const
{
    return GridHelper::DoHexagonsIntersect(GetPosition(), GetRadius(), other_center, other_radius);
}

}  // namespace simulation
