#pragma once

#include "data/enums.h"
#include "utility/custom_formatter.h"

namespace simulation
{

// Helper struct that can hold either a CombatAffinity or a CombatClass
struct CombatSynergyBonus final
{
    // Copyable or Movable
    constexpr CombatSynergyBonus() = default;
    constexpr CombatSynergyBonus(const CombatSynergyBonus&) = default;
    constexpr CombatSynergyBonus& operator=(const CombatSynergyBonus&) = default;
    constexpr CombatSynergyBonus(CombatSynergyBonus&&) = default;
    constexpr CombatSynergyBonus& operator=(CombatSynergyBonus&&) = default;
    ~CombatSynergyBonus() = default;

    // Implicit constructors
    constexpr CombatSynergyBonus(const CombatClass in_class) : combat_class_(in_class) {}
    constexpr CombatSynergyBonus(const CombatAffinity in_affinity) : combat_affinity_(in_affinity) {}

    // Assign a Combat Class
    CombatSynergyBonus& operator=(const CombatClass in_class)
    {
        combat_class_ = in_class;
        return *this;
    }

    // Assign a Combat Affinity
    CombatSynergyBonus& operator=(const CombatAffinity in_affinity)
    {
        combat_affinity_ = in_affinity;
        return *this;
    }

    // Check Equality with a combat Class & Affinity
    constexpr bool operator==(const CombatSynergyBonus& other) const
    {
        return combat_class_ == other.combat_class_ && combat_affinity_ == other.combat_affinity_;
    }
    constexpr bool operator==(const CombatClass other) const
    {
        return IsCombatClass() && combat_class_ == other;
    }
    constexpr bool operator==(const CombatAffinity other) const
    {
        return IsCombatAffinity() && combat_affinity_ == other;
    }
    constexpr bool operator!=(const CombatSynergyBonus& other) const
    {
        return !(*this == other);
    }
    constexpr bool operator!=(const CombatClass other) const
    {
        return !(*this == other);
    }
    constexpr bool operator!=(const CombatAffinity other) const
    {
        return !(*this == other);
    }

    // Is this value a combat class or a combat affinity?
    constexpr bool IsCombatClass() const
    {
        return combat_class_ != CombatClass::kNone;
    }
    constexpr bool IsCombatAffinity() const
    {
        return combat_affinity_ != CombatAffinity::kNone;
    }

    // Has any value?
    constexpr bool IsEmpty() const
    {
        return !IsCombatClass() && !IsCombatAffinity();
    }

    // Int value of this combat synergy
    constexpr int GetIntValue() const
    {
        if (IsCombatClass())
        {
            return static_cast<int>(GetClass());
        }

        return static_cast<int>(GetAffinity());
    }

    // Gets the int value used for sorting a container of CombatSynergyBonus
    constexpr int GetIntSortAbsoluteValue() const
    {
        if (IsCombatClass())
        {
            return static_cast<int>(GetClass());
        }

        // For affinities we add 1000 so that it can be different to any int value from the CombatClassEnum
        return static_cast<int>(GetAffinity()) + 1000;
    }

    // Public getters
    constexpr CombatClass GetClass() const
    {
        return combat_class_;
    }
    constexpr CombatAffinity GetAffinity() const
    {
        return combat_affinity_;
    }
    void FormatTo(fmt::format_context& ctx) const;

private:
    CombatClass combat_class_ = CombatClass::kNone;
    CombatAffinity combat_affinity_ = CombatAffinity::kNone;
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(CombatSynergyBonus, FormatTo);
