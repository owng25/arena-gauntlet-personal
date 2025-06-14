#include "game_data_container.h"

namespace simulation
{

#define ILLUVIUM_REGISTER_DATA_TYPE(variable_name, name, key_type, data_type, container_type)                      \
    bool GameDataContainer::Has##name##Data(const key_type& type_id) const                                         \
    {                                                                                                              \
        return variable_name.Contains(type_id);                                                                    \
    }                                                                                                              \
    std::shared_ptr<const data_type> GameDataContainer::Get##name##Data(const key_type& type_id) const             \
    {                                                                                                              \
        return variable_name.Find(type_id);                                                                        \
    }                                                                                                              \
    bool GameDataContainer::Add##name##Data(const key_type& type_id, const std::shared_ptr<const data_type>& data) \
    {                                                                                                              \
        return variable_name.Add(type_id, data, *logger_);                                                         \
    }                                                                                                              \
    const container_type& GameDataContainer::Get##container_type() const                                           \
    {                                                                                                              \
        return variable_name;                                                                                      \
    }

ILLUVIUM_REGISTER_DATA_TYPE(augments_, Augment, AugmentTypeID, AugmentData, AugmentsDataContainer);
ILLUVIUM_REGISTER_DATA_TYPE(
    drone_augments_,
    DroneAugment,
    DroneAugmentTypeID,
    DroneAugmentData,
    DroneAugmentsDataContainer);
ILLUVIUM_REGISTER_DATA_TYPE(
    combat_affinity_synergies_,
    CombatAffinitySynergy,
    CombatAffinity,
    SynergyData,
    CombatAffinitySynergiesDataContainer);
ILLUVIUM_REGISTER_DATA_TYPE(
    combat_classes_synergies_,
    CombatClassSynergy,
    CombatClass,
    SynergyData,
    CombatClassSynergiesDataContainer);
ILLUVIUM_REGISTER_DATA_TYPE(combat_units_, CombatUnit, CombatUnitTypeID, CombatUnitData, CombatUnitsDataContainer);
ILLUVIUM_REGISTER_DATA_TYPE(consumables_, Consumable, ConsumableTypeID, ConsumableData, ConsumablesDataContainer);
ILLUVIUM_REGISTER_DATA_TYPE(
    encounter_mods_,
    EncounterMod,
    EncounterModTypeID,
    EncounterModData,
    EncounterModsDataContainer);
ILLUVIUM_REGISTER_DATA_TYPE(suits_, Suit, CombatSuitTypeID, CombatUnitSuitData, SuitsDataContainer);
ILLUVIUM_REGISTER_DATA_TYPE(weapons_, Weapon, CombatWeaponTypeID, CombatUnitWeaponData, WeaponsDataContainer);
#undef ILLUVIUM_REGISTER_DATA_TYPE

GameDataContainer::GameDataContainer(std::shared_ptr<Logger> logger) : logger_(std::move(logger))
{
    hyper_data_ = std::make_unique<HyperData>();
    effects_config_ = std::make_unique<WorldEffectsConfig>();
}

std::shared_ptr<const GameDataContainer> GameDataContainer::CreateDeepCopy() const
{
    // Logged is shared
    auto copy_container = std::make_shared<GameDataContainer>(logger_);
    copy_container->augments_ = AugmentsDataContainer{augments_.CreateDeepCopy()};
    copy_container->drone_augments_ = DroneAugmentsDataContainer{drone_augments_.CreateDeepCopy()};
    copy_container->combat_affinity_synergies_ =
        CombatAffinitySynergiesDataContainer{combat_affinity_synergies_.CreateDeepCopy()};
    copy_container->combat_classes_synergies_ =
        CombatClassSynergiesDataContainer{combat_classes_synergies_.CreateDeepCopy()};
    copy_container->combat_units_ = CombatUnitsDataContainer{combat_units_.CreateDeepCopy()};
    copy_container->consumables_ = ConsumablesDataContainer{consumables_.CreateDeepCopy()};
    copy_container->encounter_mods_ = EncounterModsDataContainer{encounter_mods_.CreateDeepCopy()};
    copy_container->suits_ = SuitsDataContainer{suits_.CreateDeepCopy()};
    copy_container->weapons_ = WeaponsDataContainer{weapons_.CreateDeepCopy()};

    copy_container->hyper_data_ = hyper_data_->CreateDeepCopy();
    copy_container->hyper_config_ = hyper_config_;
    copy_container->effects_config_ = effects_config_->CreateDeepCopy();

    return copy_container;
}

void GameDataContainer::SetHyperData(std::unique_ptr<HyperData> hyper_data)
{
    if (hyper_data == nullptr)
    {
        hyper_data = std::make_unique<HyperData>();
    }
    hyper_data_ = std::move(hyper_data);
}

void GameDataContainer::SetHyperConfig(const HyperConfig& hyper_config)
{
    hyper_config_ = hyper_config;
}

void GameDataContainer::SetWorldEffectsConfig(std::unique_ptr<WorldEffectsConfig> effects_config)
{
    if (effects_config == nullptr)
    {
        effects_config = std::make_unique<WorldEffectsConfig>();
    }
    effects_config_ = std::move(effects_config);
}

}  // namespace simulation
