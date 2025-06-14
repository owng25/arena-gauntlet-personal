#pragma once

#include "data/ability_data.h"
#include "data/enums.h"
#include "data/stats_data.h"
#include "utility/custom_formatter.h"
#include "weapon_type_id.h"

namespace simulation
{
// Holds the data for a combat unit weapon
// Each weapon in the game converts the Ranger to a particular combat class, except for the default
// which is a pistol that gives no combat class. Each Weapon can also be infused by a Gemstone which
// gives off one of the 5 Base CombatAffinities.
class CombatUnitWeaponData
{
public:
    std::shared_ptr<CombatUnitWeaponData> CreateDeepCopy() const
    {
        // Copy everything
        auto copy = std::make_shared<CombatUnitWeaponData>(*this);

        // Create a copy of the abilities
        copy->attack_abilities = attack_abilities.CreateDeepCopy();
        copy->omega_abilities = omega_abilities.CreateDeepCopy();
        copy->innate_abilities = innate_abilities.CreateDeepCopy();

        return copy;
    }

    // Is this item considered valid
    bool IsValid() const
    {
        return type_id.IsValid();
    }

    void FormatTo(fmt::format_context& ctx) const;

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
            assert(false && "Invalid AbilityType for  CombatUnitWeaponData");
            return nullptr;
        }
    }

    // Unique identifier for this combat weapon
    CombatWeaponTypeID type_id{};

    // The type of weapon
    WeaponType weapon_type = WeaponType::kNone;

    // The tier of the item
    int tier = 0;

    // Only used if weapon_type is an amplifier. Tells us for what weapon is this amplifier targeting
    CombatWeaponTypeID amplifier_for_weapon_type_id{};

    // Combat class of this weapon
    CombatClass combat_class = CombatClass::kNone;

    // Combat affinity of this weapon
    CombatAffinity combat_affinity = CombatAffinity::kNone;

    // Dominant combat affinity
    // https://illuvium.atlassian.net/wiki/spaces/GD/pages/228425729/Dominant+Synergy
    CombatAffinity dominant_combat_affinity = CombatAffinity::kNone;

    // Dominant combat class
    // https://illuvium.atlassian.net/wiki/spaces/GD/pages/228425729/Dominant+Synergy
    CombatClass dominant_combat_class = CombatClass::kNone;

    // Stats for equipped weapon
    StatsData stats{};

    // The attack abilities aka attack abilities of this weapon
    AbilitiesData attack_abilities{};

    // Omega abilities of this weapon
    AbilitiesData omega_abilities{};

    // Innate abilities of this combat unit
    AbilitiesData innate_abilities{};

    // Contains all the replacements defined in the weapon amplifiers
    std::shared_ptr<EffectDataReplacements> effect_data_replacements;
};
}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(CombatUnitWeaponData, FormatTo);
