#pragma once

#include <string>

#include "utility/custom_formatter.h"

namespace simulation
{
// Contains data about the type of augment
class DroneAugmentTypeID
{
public:
    // The name of the drone augment
    std::string name{};

    // The stage of the drone augment
    int stage = 0;

    bool operator==(const DroneAugmentTypeID& other) const
    {
        return name == other.name && stage == other.stage;
    }
    bool operator!=(const DroneAugmentTypeID& other) const
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

        size_t operator()(const DroneAugmentTypeID& type_id) const
        {
            const size_t name_hash = GetHash(type_id.name);
            const size_t stage_hash = GetHash(type_id.stage);
            return (name_hash ^ (stage_hash << 1)) >> 1;
        }
    };

    void FormatTo(fmt::format_context& ctx) const;
};
}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(DroneAugmentTypeID, FormatTo);
