#pragma once

#include <memory>

#include "augments_data_container.h"
#include "combat_affinity_synergies_data_container.h"
#include "combat_class_synergies_data_container.h"
#include "combat_units_data_container.h"
#include "consumables_data_container.h"
#include "data/hyper_config.h"
#include "data/hyper_data.h"
#include "data/world_effects_config.h"
#include "drone_augments_data_container.h"
#include "encounter_mods_data_container.h"
#include "suits_data_container.h"
#include "weapons_data_container.h"

namespace simulation
{

class GameDataContainer final
{
public:
    explicit GameDataContainer(std::shared_ptr<Logger> logger);
    ~GameDataContainer() = default;

    std::shared_ptr<const GameDataContainer> CreateDeepCopy() const;

    bool HasAugmentData(const AugmentTypeID& type_id) const;
    bool HasDroneAugmentData(const DroneAugmentTypeID& type_id) const;
    bool HasCombatClassSynergyData(const CombatClass& type_id) const;
    bool HasCombatAffinitySynergyData(const CombatAffinity& type_id) const;
    bool HasCombatUnitData(const CombatUnitTypeID& type_id) const;
    bool HasConsumableData(const ConsumableTypeID& type_id) const;
    bool HasEncounterModData(const EncounterModTypeID& type_id) const;
    bool HasSuitData(const CombatSuitTypeID& type_id) const;
    bool HasWeaponData(const CombatWeaponTypeID& type_id) const;

    std::shared_ptr<const AugmentData> GetAugmentData(const AugmentTypeID& type_id) const;
    std::shared_ptr<const DroneAugmentData> GetDroneAugmentData(const DroneAugmentTypeID& type_id) const;
    std::shared_ptr<const SynergyData> GetCombatClassSynergyData(const CombatClass& type_id) const;
    std::shared_ptr<const SynergyData> GetCombatAffinitySynergyData(const CombatAffinity& type_id) const;
    std::shared_ptr<const CombatUnitData> GetCombatUnitData(const CombatUnitTypeID& unit_id) const;
    std::shared_ptr<const ConsumableData> GetConsumableData(const ConsumableTypeID& type_id) const;
    std::shared_ptr<const EncounterModData> GetEncounterModData(const EncounterModTypeID& type_id) const;
    std::shared_ptr<const CombatUnitSuitData> GetSuitData(const CombatSuitTypeID& type_id) const;
    std::shared_ptr<const CombatUnitWeaponData> GetWeaponData(const CombatWeaponTypeID& type_id) const;

    bool AddAugmentData(const AugmentTypeID& type_id, const std::shared_ptr<const AugmentData>& data);
    bool AddDroneAugmentData(const DroneAugmentTypeID& type_id, const std::shared_ptr<const DroneAugmentData>& data);
    bool AddCombatClassSynergyData(const CombatClass& type_id, const std::shared_ptr<const SynergyData>& data);
    bool AddCombatAffinitySynergyData(const CombatAffinity& type_id, const std::shared_ptr<const SynergyData>& data);
    bool AddCombatUnitData(const CombatUnitTypeID& unit_id, const std::shared_ptr<const CombatUnitData>& data);
    bool AddConsumableData(const ConsumableTypeID& type_id, const std::shared_ptr<const ConsumableData>& data);
    bool AddEncounterModData(const EncounterModTypeID& type_id, const std::shared_ptr<const EncounterModData>& data);
    bool AddSuitData(const CombatSuitTypeID& type_id, const std::shared_ptr<const CombatUnitSuitData>& data);
    bool AddWeaponData(const CombatWeaponTypeID& type_id, const std::shared_ptr<const CombatUnitWeaponData>& data);

    const AugmentsDataContainer& GetAugmentsDataContainer() const;
    const DroneAugmentsDataContainer& GetDroneAugmentsDataContainer() const;
    const CombatClassSynergiesDataContainer& GetCombatClassSynergiesDataContainer() const;
    const CombatAffinitySynergiesDataContainer& GetCombatAffinitySynergiesDataContainer() const;
    const CombatUnitsDataContainer& GetCombatUnitsDataContainer() const;
    const ConsumablesDataContainer& GetConsumablesDataContainer() const;
    const EncounterModsDataContainer& GetEncounterModsDataContainer() const;
    const SuitsDataContainer& GetSuitsDataContainer() const;
    const WeaponsDataContainer& GetWeaponsDataContainer() const;

    void SetHyperData(std::unique_ptr<HyperData> hyper_data);
    void SetHyperConfig(const HyperConfig& hyper_config);
    void SetWorldEffectsConfig(std::unique_ptr<WorldEffectsConfig> effects_config);

    const HyperData& GetHyperData() const
    {
        return *hyper_data_;
    }
    const HyperConfig& GetHyperConfig() const
    {
        return hyper_config_;
    }
    const WorldEffectsConfig& GetWorldEffectsConfig() const
    {
        return *effects_config_;
    }

private:
    std::shared_ptr<Logger> logger_;

    AugmentsDataContainer augments_;
    DroneAugmentsDataContainer drone_augments_;
    CombatAffinitySynergiesDataContainer combat_affinity_synergies_;
    CombatClassSynergiesDataContainer combat_classes_synergies_;
    CombatUnitsDataContainer combat_units_;
    ConsumablesDataContainer consumables_;
    EncounterModsDataContainer encounter_mods_;
    SuitsDataContainer suits_;
    WeaponsDataContainer weapons_;

    std::unique_ptr<HyperData> hyper_data_;
    std::unique_ptr<WorldEffectsConfig> effects_config_;
    HyperConfig hyper_config_;
};
}  // namespace simulation
