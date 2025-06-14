#pragma once

#include <string>

#include "augment_type_id.h"
#include "data/constants.h"
#include "utility/custom_formatter.h"

namespace simulation
{

// Instance data for a AugmentData
class AugmentInstanceData
{
public:
    bool operator==(const AugmentTypeID& other_type_id) const
    {
        return type_id == other_type_id;
    }

    bool operator!=(const AugmentTypeID& other_type_id) const
    {
        return !(*this == other_type_id);
    }

    bool operator==(const AugmentInstanceData& other) const
    {
        return id == other.id && type_id == other.type_id;
    }

    bool operator!=(const AugmentInstanceData& other) const
    {
        return !(*this == other);
    }

    void FormatTo(fmt::format_context& ctx) const;

    // Unique ID
    std::string id;

    // Type ID
    AugmentTypeID type_id;
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(AugmentInstanceData, FormatTo);
