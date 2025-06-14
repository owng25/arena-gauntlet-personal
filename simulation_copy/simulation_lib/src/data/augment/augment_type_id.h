#pragma once

#include <string>

#include "utility/custom_formatter.h"

namespace simulation
{
// Contains data about the type of augment
class AugmentTypeID
{
public:
    AugmentTypeID() = default;
    AugmentTypeID(const std::string& in_name, const int in_stage, const std::string& in_variation)
        : name{in_name},
          stage{in_stage},
          variation{in_variation}
    {
    }

    // The name of the item
    std::string name{};

    // The stage of the item
    int stage = 0;

    // The variation of the item
    std::string variation{};

    bool operator==(const AugmentTypeID& other) const
    {
        return name == other.name && stage == other.stage && variation == other.variation;
    }
    bool operator!=(const AugmentTypeID& other) const
    {
        return !(*this == other);
    }

    // Is this item considered valid
    bool IsValid() const
    {
        return !name.empty();
    }

    struct HashFunction
    {
        template <typename T>
        static size_t GetHash(const T& value)
        {
            return std::hash<T>{}(value);
        }

        size_t operator()(const AugmentTypeID& type_id) const
        {
            const size_t name_hash = GetHash(type_id.name);
            const size_t stage_hash = GetHash(type_id.stage);
            const size_t variation_hash = GetHash(type_id.variation);
            return ((name_hash ^ (stage_hash << 1)) >> 1) ^ variation_hash;
        }
    };

    void FormatTo(fmt::format_context& ctx) const;
};
}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(AugmentTypeID, FormatTo);
