#pragma once

#include "data/combat_unit_type_id.h"
#include "data/suit/suit_instance_data.h"
#include "data/weapon/weapon_instance_data.h"
#include "ecs/component.h"

namespace simulation
{
/* -------------------------------------------------------------------------------------------------------
 * CombatUnitComponent
 *
 * This class holds the details of the type of combat unit spawned in the world
 * --------------------------------------------------------------------------------------------------------
 */
class CombatUnitComponent : public Component
{
public:
    std::shared_ptr<Component> CreateDeepCopyFromInitialState() const override
    {
        return std::make_shared<CombatUnitComponent>(*this);
    }

    int GetChildrenSpawnedCount() const
    {
        return children_spawned_count_;
    }
    void IncrementChildrenSpawnedCount()
    {
        children_spawned_count_++;
    }

    void SetUniqueID(const std::string& unique_id)
    {
        unique_id_ = unique_id;
    }
    void SetTypeID(const CombatUnitTypeID& type_id)
    {
        type_id_ = type_id;
    }
    void SetEquippedWeapon(const CombatWeaponInstanceData& new_weapon)
    {
        equipped_weapon_ = new_weapon;
    }
    void SetEquippedSuit(const CombatSuitInstanceData& new_suit)
    {
        equipped_suit_ = new_suit;
    }
    void SetBondedUniqueID(const std::string& bonded_id)
    {
        bonded_unique_id_ = bonded_id;
    }
    void SetFinish(const std::string& finish)
    {
        finish_ = finish;
    }

    const std::string& GetUniqueID() const
    {
        return unique_id_;
    }
    const CombatUnitTypeID& GetTypeID() const
    {
        return type_id_;
    }

    const CombatWeaponInstanceData& GetEquippedWeapon() const
    {
        return equipped_weapon_;
    }
    const std::vector<CombatWeaponAmplifierInstanceData>& GetEquippedWeaponAmplifiers() const
    {
        return GetEquippedWeapon().equipped_amplifiers;
    }
    const CombatSuitInstanceData& GetEquippedSuit() const
    {
        return equipped_suit_;
    }

    const std::string& GetBondedUniqueID() const
    {
        return bonded_unique_id_;
    }
    const std::string& GetFinish() const
    {
        return finish_;
    }

    bool IsAIlluvial() const
    {
        return type_id_.type == CombatUnitType::kIlluvial;
    }
    bool IsARanger() const
    {
        return type_id_.type == CombatUnitType::kRanger;
    }
    bool IsAPet() const
    {
        return type_id_.type == CombatUnitType::kPet;
    }

private:
    // Unique ID of this unit
    std::string unique_id_;

    // The Type id of this unit: Axoltl, Turtle, etc
    CombatUnitTypeID type_id_{};

    // The equipped weapon if any
    CombatWeaponInstanceData equipped_weapon_{};

    // The equipped suit  if any
    CombatSuitInstanceData equipped_suit_{};

    // To which combat unit is this unit bonded with.
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/3440924/Bond
    std::string bonded_unique_id_;

    // Number of children that this combat unit spawned
    int children_spawned_count_ = 0;

    // Used mostly for overworld
    // https://illuvium.atlassian.net/wiki/spaces/AVATAR/pages/418545665/Illuvitar+Finish
    std::string finish_;
};

}  // namespace simulation
