#pragma once

#include <cmath>

#include "color.h"

namespace simulation::tool
{

namespace constants
{

// Colors
namespace color
{

static constexpr Color LightGray{200, 200, 200, 255};
static constexpr Color Gray{130, 130, 130, 255};
static constexpr Color DarkGray{80, 80, 80, 255};
static constexpr Color Yellow{253, 249, 0, 255};
static constexpr Color Gold{255, 203, 0, 255};
static constexpr Color Orange{255, 161, 0, 255};
static constexpr Color Pink{255, 109, 194, 255};
static constexpr Color Red{230, 41, 55, 255};
static constexpr Color Maroon{190, 33, 55, 255};
static constexpr Color Green{0, 228, 48, 255};
static constexpr Color Lime{0, 158, 47, 255};
static constexpr Color DarkGreen{0, 117, 44, 255};
static constexpr Color SkyBlue{102, 191, 255, 255};
static constexpr Color Blue{0, 121, 241, 255};
static constexpr Color DarkBlue{0, 82, 172, 255};
static constexpr Color Purple{200, 122, 255, 255};
static constexpr Color Violet{135, 60, 190, 255};
static constexpr Color DarkPurple{112, 31, 126, 255};
static constexpr Color Beige{211, 176, 131, 255};
static constexpr Color Brown{127, 106, 79, 255};
static constexpr Color DarkBrown{76, 63, 47, 255};
static constexpr Color White{255, 255, 255, 255};
static constexpr Color Black{0, 0, 0, 255};
static constexpr Color Blank{0, 0, 0, 0};
static constexpr Color Magneta{255, 0, 255, 255};
static constexpr Color RayWhite{245, 245, 245, 255};

struct TeamColorSchema
{
    Color hex_fill;
    Color hex_border;
    Color damage_info;
};

static constexpr TeamColorSchema kRedTeamColorSchema = []()
{
    TeamColorSchema schema{};
    schema.hex_fill = Yellow;
    schema.hex_border = Orange;
    schema.damage_info = Yellow;
    return schema;
}();

static constexpr TeamColorSchema kBlueTeamColorSchema = []()
{
    TeamColorSchema schema{};
    schema.hex_fill = SkyBlue;
    schema.hex_border = Blue;
    schema.damage_info = SkyBlue;
    return schema;
}();

static constexpr TeamColorSchema kUnknownColorSchema = []()
{
    TeamColorSchema schema{};
    schema.hex_fill = Gray;
    schema.hex_border = DarkGray;
    schema.damage_info = Gray;
    return schema;
}();
}  // namespace color

}  // namespace constants
}  // namespace simulation::tool
