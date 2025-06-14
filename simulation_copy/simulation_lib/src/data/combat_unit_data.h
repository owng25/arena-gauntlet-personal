#pragma once

#include "combat_unit_instance_data.h"
#include "data/ability_data.h"
#include "data/combat_unit_type_id.h"
#include "data/stats_data.h"

namespace simulation
{
// Holds the data of combat unit type
class CombatUnitTypeData
{
public:
    CombatUnitTypeData CreateDeepCopy() const
    {
        // Copy everything
        CombatUnitTypeData copy{*this};

        // Create a copy of the abilities
        copy.attack_abilities = attack_abilities.CreateDeepCopy();
        copy.omega_abilities = omega_abilities.CreateDeepCopy();
        copy.innate_abilities = innate_abilities.CreateDeepCopy();

        return copy;
    }

    const AbilitiesData* GetAbilitiesDataForAbilityType(const AbilityType ability_type) const
    {
        switch (ability_type)
        {
        case AbilityType::kAttack:
            return &attack_abilities;
        case AbilityType::kOmega:
            return &omega_abilities;
        case AbilityType::kInnate:
            return &innate_abilities;

        default:
            assert(false && "Invalid AbilityType for CombatUnitTypeData");
            return nullptr;
        }
    }

    // Combat class of this combat unit
    CombatClass combat_class = CombatClass::kNone;

    // Combat affinity of this combat unit
    CombatAffinity combat_affinity = CombatAffinity::kNone;

    // NOTE: Only used if evolution enabled
    // If set, this is the preferred dominant of all illuvials of the same line.
    // Used to generate dominant_combat_class.
    CombatClass preferred_line_dominant_combat_class = CombatClass::kNone;

    // NOTE: Only used if evolution enabled
    // If set, this is the preferred dominant of all illuvials of the same line
    // Used to generate dominant_combat_affinity.
    CombatAffinity preferred_line_dominant_combat_affinity = CombatAffinity::kNone;

    // Generated depending on the stage/line etc
    CombatClass dominant_combat_class = CombatClass::kNone;

    // Generated depending on the stage/line etc.
    CombatAffinity dominant_combat_affinity = CombatAffinity::kNone;

    // Template Stats for this combat unit
    StatsData stats{};

    // The tier of this unit
    int tier = 0;

    // The attack abilities aka attack abilities of this combat unit
    AbilitiesData attack_abilities{};

    // Omega abilities of this combat unit
    AbilitiesData omega_abilities{};

    // Innate abilities of this combat unit
    AbilitiesData innate_abilities{};
};

// Holds all the type data for a combat unit (illuvial/ranger)
class CombatUnitData
{
public:
    // Initialize with sane defaults
    CombatUnitData() = default;

    std::shared_ptr<CombatUnitData> CreateDeepCopy() const
    {
        // Copy everything
        auto copy = std::make_shared<CombatUnitData>(*this);

        // Create a copy of the type_data
        copy->type_data = type_data.CreateDeepCopy();

        return copy;
    }

    // Unique identifier for this combat unit type
    CombatUnitTypeID type_id{};

    // Size in units of this combat unit
    int radius_units = 0;

    // Type data of this combat unit
    CombatUnitTypeData type_data{};
};

// Holds the CombatUnitData + the CombatUnitInstanceData
struct FullCombatUnitData
{
    const CombatUnitTypeID& GetCombatUnitTypeID() const
    {
        return data.type_id;
    }
    const CombatWeaponTypeID& GetEquippedWeaponTypeID() const
    {
        return instance.equipped_weapon.type_id;
    }
    const CombatSuitTypeID& GetEquippedSuitTypeID() const
    {
        return instance.equipped_suit.type_id;
    }

    int GetLevel() const
    {
        return instance.level;
    }

    CombatUnitData data;
    CombatUnitInstanceData instance;
};

}  // namespace simulation
