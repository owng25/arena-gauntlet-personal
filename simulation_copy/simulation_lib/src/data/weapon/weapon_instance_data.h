#pragma once

#include "weapon_type_id.h"

namespace simulation
{

// Base instance data for any combat weapon or amplifier
class CombatWeaponBaseInstanceData
{
public:
    // Unique ID
    std::string id;

    // Type ID
    CombatWeaponTypeID type_id;

    // Is this item considered valid
    bool IsValid() const
    {
        return type_id.IsValid();
    }

    // Overload comparison
    bool operator==(const CombatWeaponBaseInstanceData& other) const
    {
        return type_id == other.type_id && id == other.id;
    }
    bool operator!=(const CombatWeaponBaseInstanceData& other) const
    {
        return !(*this == other);
    }

    void FormatTo(fmt::format_context& ctx) const;
};

// Instance data for a CombatWeaponTypeID of type Amplifier
class CombatWeaponAmplifierInstanceData : public CombatWeaponBaseInstanceData
{
    using Super = CombatWeaponBaseInstanceData;

public:
    // Overload comparison
    bool operator==(const CombatWeaponAmplifierInstanceData& other) const
    {
        return static_cast<const Super&>(*this) == other;
    }
    bool operator!=(const CombatWeaponAmplifierInstanceData& other) const
    {
        return !(*this == other);
    }
};

// Instance data for a CombatWeaponTypeID
class CombatWeaponInstanceData : public CombatWeaponBaseInstanceData
{
    using Super = CombatWeaponBaseInstanceData;

public:
    // Equipped amplifiers on this weapon
    std::vector<CombatWeaponAmplifierInstanceData> equipped_amplifiers;

    // Overload comparison
    bool operator==(const CombatWeaponInstanceData& other) const
    {
        return static_cast<const Super&>(*this) == other && equipped_amplifiers == other.equipped_amplifiers;
    }
    bool operator!=(const CombatWeaponInstanceData& other) const
    {
        return !(*this == other);
    }

    void FormatTo(fmt::format_context& ctx) const;
};
}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(CombatWeaponBaseInstanceData, FormatTo);
ILLUVIUM_MAKE_STRUCT_FORMATTER(CombatWeaponAmplifierInstanceData, FormatTo);
ILLUVIUM_MAKE_STRUCT_FORMATTER(CombatWeaponInstanceData, FormatTo);
