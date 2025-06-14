#pragma once

#include <vector>

#include "ecs/component.h"
#include "utility/hex_grid_position.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * PositionComponent
 *
 * This class provides a positioning component, representing the center location of the entity on
 * the grid. In our representation q is the distance 45 degrees clockwise to the horizontal and r
 * is the vertical distance from the center of the grid.
 *
 * Each grid hex position (q, r) is composed of subunit positions (sub_q, sub_r)
 * sub_q and sub_r which represent smaller hexes inside the position, although during operations
 * the offsets could place the position inside another hex unit.
 *
 * NOTE: That the position of the combat units form a hexagon
 *
 * Docs for more details:
 * https://illuvium.atlassian.net/wiki/spaces/TECHNOLOGY/pages/206013000/Hex+System
 * --------------------------------------------------------------------------------------------------------
 */
class PositionComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<PositionComponent>(*this);
    }

    // Component initialisation to valid data
    void InitFromData(const HexGridPosition& position, const int radius, const bool taking_space)
    {
        SetPosition(position);
        SetRadius(radius);
        SetTakingSpace(taking_space);
    }

    // Component initialisation to null data
    void Init() override
    {
        // Set position out of bounds on initialisation
        SetPosition(kInvalidHexHexGridPosition);
    }

    // Returns the Q, R, S unit coordinate
    constexpr const HexGridPosition& GetPosition() const
    {
        return position_;
    }
    constexpr int GetQ() const
    {
        return position_.q;
    }
    constexpr int GetR() const
    {
        return position_.r;
    }
    constexpr int GetS() const
    {
        return position_.s();
    }

    // Returns the Q, R subunit coordinate
    constexpr const HexGridPosition& GetSubUnitPosition() const
    {
        return subunit_position_;
    }

    // Sets the Q coordinate
    constexpr void SetQ(const int q)
    {
        SetQ(q, 0);
    }
    constexpr void SetQ(const int q, const int subunit_q)
    {
        position_.q = q;
        subunit_position_.q = subunit_q;
    }

    // Sets the R coordinate
    constexpr void SetR(const int r)
    {
        SetR(r, 0);
    }
    constexpr void SetR(const int r, const int subunit_r)
    {
        position_.r = r;
        subunit_position_.r = subunit_r;
    }

    // Sets the Q, R  unit coordinates pairs
    constexpr void SetPosition(const HexGridPosition& position)
    {
        SetPosition(position.q, position.r);
    }
    constexpr void SetPosition(const int q, const int r)
    {
        SetQ(q);
        SetR(r);
    }

    // Sets the Q and R coordinate pairs
    // These coordinates refers to the center position of the combat unit
    constexpr void SetPosition(const HexGridPosition& position, const HexGridPosition& subunit)
    {
        SetPosition(position.q, position.r, subunit.q, subunit.r);
    }
    constexpr void SetPosition(const int q, const int r, const int subunit_q, const int subunit_r)
    {
        SetQ(q, subunit_q);
        SetR(r, subunit_r);
    }

    // Return true if has a reserved position
    constexpr bool HasReservedPosition() const
    {
        return reserved_position_ != kInvalidHexHexGridPosition;
    }

    // Gets a reserved position
    constexpr const HexGridPosition& GetReservedPosition() const
    {
        return reserved_position_;
    }

    // Set a reserved position
    constexpr void SetReservedPosition(const HexGridPosition& reserved_position)
    {
        reserved_position_ = reserved_position;
    }

    // Clear reserved position
    constexpr void ClearReservedPosition()
    {
        reserved_position_ = kInvalidHexHexGridPosition;
    }

    // Gets the distance from center for this position component
    constexpr int GetRadius() const
    {
        return radius_units_;
    }

    // Sets the radius of the entity (aka radius from the center)
    constexpr void SetRadius(const int radius_units)
    {
        radius_units_ = radius_units;
    }

    // Does this take space on the grid, useful to know if pathfinding should avoid us.
    // If true this will be marked as obstacle by other entities
    // If false this will be ignored by other combat units
    constexpr bool IsTakingSpace() const
    {
        return is_taking_space_;
    }

    // Set whether this entity is taking space
    constexpr void SetTakingSpace(bool value)
    {
        is_taking_space_ = value;
    }

    // Can this be overlapped by other entities
    // Pathfinding will not use it as obstacle,
    // but other system still can check collision with it.
    // If true this will not be marked as obstacle on grid
    // If false this will behave normally
    constexpr bool IsOverlapable() const
    {
        return is_overlapable_;
    }

    // Set whether this entity is can be overlapped
    constexpr void SetOverlapable(bool value)
    {
        is_overlapable_ = value;
    }

    // Is the position unit defined by (q, r) on the grid take by this component?
    // You can't compare positions directly because of the radius
    constexpr bool IsPositionTaken(const HexGridPosition& position) const
    {
        // Position matches the center
        const HexGridPosition& center = GetPosition();
        if (position == center)
        {
            return true;
        }

        // Otherwise check the radius
        // |v - c| <= r
        const HexGridPosition vector_to_position = position - center;
        return vector_to_position.Length() <= radius_units_;
    }
    constexpr bool IsPositionTaken(const int q, const int r) const
    {
        return IsPositionTaken({q, r});
    }

    // Checks for overlap between two positions
    bool IsPositionsIntersecting(const HexGridPosition& other_center, const int other_radius) const;
    bool IsPositionsIntersecting(const PositionComponent& other_position_component) const
    {
        return IsPositionsIntersecting(other_position_component.GetPosition(), other_position_component.GetRadius());
    }
    bool IsPositionsIntersecting(const int q, const int r, const int other_radius) const
    {
        return IsPositionsIntersecting({q, r}, other_radius);
    }

    // Gets a list of all the taken positions for this Position and this radius
    std::vector<HexGridPosition> GetTakenPositions() const;

    // Convert this PositionComponent to subunits
    // This is similar to the Position::ToSubUnits() but this takes into account also
    // this component subunit position
    // Aka: GetPosition().ToSubUnits() + GetSubUnitPosition();
    constexpr HexGridPosition ToSubUnits() const
    {
        return GetPosition().ToSubUnits() + GetSubUnitPosition();
    }

    // Get the angle from this position to another, going counter-clockwise from where x > 0 and
    // y = 0
    // Return degrees in the range [0, 360)
    int AngleToPosition(const PositionComponent& other_position) const;
    int AngleToPosition(const HexGridPosition& other_position) const;

    // Checks whether another position is within range of this one
    constexpr bool IsInRange(const PositionComponent& other_position, const int range_units) const
    {
        // Get vector between positions
        const HexGridPosition vector_to_other = other_position.GetPosition() - GetPosition();

        // Accommodate CombatUnit sizes
        // Add 1 to the radius because range=0 means right next to each other.
        // See: MovementSystem::FindNavigatingWaypoints
        const int sum_dist_from_center = radius_units_ + other_position.radius_units_ + 1;
        const int distance = vector_to_other.Length() - sum_dist_from_center;

        // Check if in range or not
        return distance <= range_units;
    }

private:
    // Center q, r position unit withing the grid
    // Range for this is
    // q - [-HexGridPosition::QLimit(), HexGridPosition::QLimit()]
    // r - [-kGridExtent, kGridExtent]
    HexGridPosition position_ = kInvalidHexHexGridPosition;

    // Subunit Q, R position within a grid hex (q, r)
    // Range for each (-kHalfOfSubUnitsPerUnit, kHalfOfSubUnitsPerUnit]
    HexGridPosition subunit_position_ = kInvalidHexHexGridPosition;

    // An additional position reserved for this unit
    HexGridPosition reserved_position_ = kInvalidHexHexGridPosition;

    // How many hex units from the center (position_) does this entity extend.
    int radius_units_ = 0;

    // Whether entity takes space on the grid
    bool is_taking_space_ = false;

    // Whether entity can be overlapped by other entities
    bool is_overlapable_ = false;
};

}  // namespace simulation
