#include "hex_grid_config.h"

#include "utility/ensure_enum_size.h"

namespace simulation
{

static PredefinedGridPosition MirrorPredefinedPosition(const PredefinedGridPosition side)
{
    ILLUVIUM_ENSURE_ENUM_SIZE(PredefinedGridPosition, 3);
    switch (side)
    {
    case PredefinedGridPosition::kAllyBorderCenter:
        return PredefinedGridPosition::kEnemyBorderCenter;
    case PredefinedGridPosition::kEnemyBorderCenter:
        return PredefinedGridPosition::kAllyBorderCenter;
    default:
        assert(false);
        return PredefinedGridPosition::kNone;
    }
}

HexGridPosition HexGridConfig::ResolvePredefinedPosition(
    const Team ally_team,
    const PredefinedGridPosition predefined_position) const
{
    // Just to implement only from one team point of view
    ILLUVIUM_ENSURE_ENUM_SIZE(Team, 3);
    if (ally_team == Team::kBlue)
    {
        return ResolvePredefinedPosition(Team::kRed, MirrorPredefinedPosition(predefined_position));
    }

    // Always red team here
    int row = 0, column = 0;
    ILLUVIUM_ENSURE_ENUM_SIZE(PredefinedGridPosition, 3);
    switch (predefined_position)
    {
    case PredefinedGridPosition::kAllyBorderCenter:
        column = grid_width_ / 2;
        row = 0;
        break;
    case PredefinedGridPosition::kEnemyBorderCenter:
        column = grid_width_ / 2;
        row = grid_height_ - 1;
        break;
    default:
        assert(false);
        break;
    }

    return GetCoordinates(static_cast<size_t>(row * grid_width_ + column));
}

}  // namespace simulation
