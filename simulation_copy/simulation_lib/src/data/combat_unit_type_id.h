#pragma once

#include <string>

#include "utility/custom_formatter.h"

namespace simulation
{
// The type of combat unit
enum class CombatUnitType : int
{
    kNone = 0,

    // An illuvial
    kIlluvial,

    // Ranger combat unit
    // See https://illuvium.atlassian.net/wiki/spaces/GD/pages/100303158/Ranger+Combat+Unit
    kRanger,

    // Pet combat unit
    // See https://illuvium.atlassian.net/wiki/spaces/AB/pages/124584457/Pet+Combat+Unit
    kPet,

    // A Crate, not actually spawned inside the simulation
    kCrate,

    // -new values can be added above this line
    kNum,
};

/* -------------------------------------------------------------------------------------------------------
 * CombatUnitTypeID
 *
 * Unique identifier for an CombatUnit type.
 * --------------------------------------------------------------------------------------------------------
 */
class CombatUnitTypeID
{
public:
    CombatUnitTypeID() = default;
    CombatUnitTypeID(const std::string& in_line_name, const int in_stage) : line_name{in_line_name}, stage{in_stage} {}

    bool operator==(const CombatUnitTypeID& other) const
    {
        // Rangers
        if (type == CombatUnitType::kRanger)
        {
            return line_name == other.line_name;
        }

        // Illuvial
        return line_name == other.line_name && stage == other.stage && type == other.type && path == other.path &&
               variation == other.variation;
    }

    bool operator!=(const CombatUnitTypeID& other) const
    {
        return !(*this == other);
    }

    // Is this the default one.
    bool IsEmpty() const
    {
        return line_name.empty() && stage == 0;
    }

    bool IsValid() const
    {
        return !IsEmpty();
    }

    void FormatTo(fmt::format_context& ctx) const;

    struct HashFunction
    {
        template <typename T>
        static size_t GetHashFor(const T& value)
        {
            return std::hash<T>()(value);
        }

        static constexpr size_t HashCombine(const size_t a, const size_t b)
        {
            return (a ^ b) >> 1;
        }

        size_t operator()(const CombatUnitTypeID& type_id) const
        {
            size_t hash = GetHashFor(type_id.line_name);
            hash = HashCombine(hash, GetHashFor(type_id.path) << 1);
            hash = HashCombine(hash, GetHashFor(type_id.variation) << 1);
            hash = HashCombine(hash, GetHashFor(type_id.stage) << 1);
            hash ^= GetHashFor(type_id.type) << 1;
            return hash;
        }
    };

    // Name of this combat unit line eg: Axolotl, FemaleRanger
    std::string line_name{};

    // Stage of growth for this illuvial
    // NOTE: Only illuvials have stages
    int stage = 0;

    // The type of this unit
    CombatUnitType type = CombatUnitType::kIlluvial;

    // Determines the specific fusion branch of the Illuvial.
    // https://illuvium.atlassian.net/wiki/spaces/GD/pages/667550016/Path
    std::string path{};

    // This refers to a subgroup, all of the same Type, where each member is still different.
    // https://illuvium.atlassian.net/wiki/spaces/GD/pages/273711105/Variation
    std::string variation{};
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(CombatUnitTypeID, FormatTo);
