#pragma once

#include "data/weapon/weapon_data.h"
#include "data/weapon/weapon_type_id.h"
#include "data_container_base.h"

namespace simulation
{
class WeaponsDataContainer
    : public DataContainerBase<CombatWeaponTypeID, CombatUnitWeaponData, CombatWeaponTypeID::HashFunction>
{
public:
    WeaponsDataContainer() = default;
    explicit WeaponsDataContainer(const DataContainerBase& base) : DataContainerBase(base) {}

    const std::vector<CombatWeaponTypeID>& GetAllAmplifiersTypeIDsForWeapon(const CombatWeaponTypeID& type_id) const
    {
        if (!amplifiers_for_weapons_map_.contains(type_id))
        {
            static const std::vector<CombatWeaponTypeID> empty_vector;
            return empty_vector;
        }

        return amplifiers_for_weapons_map_.at(type_id);
    }

protected:
    virtual void HandleOnAdd(const CombatWeaponTypeID& type_id) override
    {
        // If amplifier_for_weapon_type_id is valid then it means it has the amplifier type_id as an option.
        std::shared_ptr<const CombatUnitWeaponData> data_ptr = Find(type_id);
        if (data_ptr->weapon_type == WeaponType::kAmplifier && data_ptr->amplifier_for_weapon_type_id.IsValid())
        {
            amplifiers_for_weapons_map_[data_ptr->amplifier_for_weapon_type_id].push_back(type_id);
        }
    }

    // Helper map to keep track of all weapon amplifiers for each weapon.
    // Key: Weapon Type ID
    // Value: Vector of all amplifiers for this weapon type id
    std::unordered_map<CombatWeaponTypeID, std::vector<CombatWeaponTypeID>, CombatWeaponTypeID::HashFunction>
        amplifiers_for_weapons_map_{};
};

}  // namespace simulation
