#pragma once

#include "data/enums.h"
#include "ecs/component.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * CombatSynergyComponent
 *
 * Hold the combat class and affinity for an entity
 * --------------------------------------------------------------------------------------------------------
 */
class CombatSynergyComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<CombatSynergyComponent>(*this);
    }

    CombatAffinity GetCombatAffinity() const
    {
        return combat_affinity_;
    }
    CombatAffinity GetDefaultTypeDataCombatAffinity() const
    {
        return default_type_data_combat_affinity_;
    }
    CombatAffinity GetDominantCombatAffinity() const
    {
        return dominant_combat_affinity_;
    }

    CombatClass GetCombatClass() const
    {
        return combat_class_;
    }
    CombatClass GetDefaultTypeDataCombatClass() const
    {
        return default_type_data_combat_class_;
    }
    CombatClass GetDominantCombatClass() const
    {
        return dominant_combat_class_;
    }

    void SetCombatAffinity(const CombatAffinity combat_affinity)
    {
        combat_affinity_ = combat_affinity;
    }
    void SetDefaultTypeDataCombatAffinity(const CombatAffinity combat_affinity)
    {
        default_type_data_combat_affinity_ = combat_affinity;
    }
    void SetDominantCombatAffinity(const CombatAffinity combat_affinity)
    {
        dominant_combat_affinity_ = combat_affinity;
    }

    void SetCombatClass(const CombatClass combat_class)
    {
        combat_class_ = combat_class;
    }
    void SetDefaultTypeDataCombatClass(const CombatClass combat_class)
    {
        default_type_data_combat_class_ = combat_class;
    }
    void SetDominantCombatClass(const CombatClass combat_class)
    {
        dominant_combat_class_ = combat_class;
    }

    void SetIsSetFromBond(const bool from_bond)
    {
        is_set_from_bond_ = from_bond;
    }

    bool IsSetFromBond() const
    {
        return is_set_from_bond_;
    }

private:
    // Current entity combat class
    CombatClass combat_class_ = CombatClass::kNone;

    // Current entity combat affinity
    CombatAffinity combat_affinity_ = CombatAffinity::kNone;

    // The default type data combat class for this
    // NOTE: if bonding is not involved this has the same value as combat_class_
    CombatClass default_type_data_combat_class_ = CombatClass::kNone;

    // The default type data combat affinity for this.
    // NOTE: if bonding is not involved this has the same value as combat_affinity_
    CombatAffinity default_type_data_combat_affinity_ = CombatAffinity::kNone;

    // Was the combat class and class affinity set from bonding?
    bool is_set_from_bond_ = false;

    // Dominant combat affinity
    // https://illuvium.atlassian.net/wiki/spaces/GD/pages/228425729/Dominant+Synergy
    CombatAffinity dominant_combat_affinity_ = CombatAffinity::kNone;

    // Dominant combat class
    // https://illuvium.atlassian.net/wiki/spaces/GD/pages/228425729/Dominant+Synergy
    CombatClass dominant_combat_class_ = CombatClass::kNone;
};

/* -------------------------------------------------------------------------------------------------------
 * SynergyEntityComponent
 *
 * Used to identify synergy entity that is used for holding and executing team abilities.
 * --------------------------------------------------------------------------------------------------------
 */
class SynergyEntityComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<SynergyEntityComponent>(*this);
    }
};

}  // namespace simulation
