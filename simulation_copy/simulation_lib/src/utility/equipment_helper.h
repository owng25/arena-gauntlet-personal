#pragma once

#include "data/combat_synergy_bonus.h"
#include "data/effect_data.h"
#include "data/enums.h"
#include "data/stats_data.h"
#include "data/weapon/weapon_instance_data.h"
#include "utility/logger_consumer.h"

namespace simulation
{

struct AbilitiesData;
struct FullCombatUnitData;
class GameDataContainer;
class SuitsDataContainer;
class CombatWeaponTypeID;
class World;
class CombatUnitWeaponData;

/**
 * A helper class for managing equipment-related functionality and data, such as
 * retrieving combat-related stats and abilities, and verifying equipability of
 * weapons and amplifiers. Implements the LoggerConsumer interface for logging purposes.
 */
class EquipmentHelper final : public LoggerConsumer
{
public:
    EquipmentHelper() = default;
    explicit EquipmentHelper(World* world);

    //
    // LoggerConsumer interface
    //

    std::shared_ptr<Logger> GetLogger() const override;
    void BuildLogPrefixFor(const EntityID id, std::string* out_string) const override;
    int GetTimeStepCount() const override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kAugment;
    }

    //
    // Own functions
    //

    // Helper getters to get the data directly irrespective if it is an illuvial or ranger
    std::tuple<CombatClass, CombatAffinity> GetWeaponDominants(const CombatWeaponTypeID& type_id) const;

    static CombatClass GetDefaultTypeDataCombatClass(
        const GameDataContainer& game_data_container,
        const FullCombatUnitData& full_data);
    CombatClass GetDefaultTypeDataCombatClass(const FullCombatUnitData& full_data) const;

    static CombatAffinity GetDefaultTypeDataCombatAffinity(
        const GameDataContainer& game_data_container,
        const FullCombatUnitData& full_data);
    CombatAffinity GetDefaultTypeDataCombatAffinity(const FullCombatUnitData& full_data) const;

    static StatsData GetDefaultTypeDataStatsData(
        const GameDataContainer& game_data_container,
        const FullCombatUnitData& full_data);
    StatsData GetDefaultTypeDataStatsData(const FullCombatUnitData& full_data) const;

    static FixedPoint GetDefaultTypeDataStatValueForType(
        const GameDataContainer& game_data_container,
        const FullCombatUnitData& full_data,
        const StatType stat_type);
    FixedPoint GetDefaultTypeDataStatValueForType(const FullCombatUnitData& full_data, const StatType stat_type) const;

    // Calculates the total stats of the weapon summed with the stats from its amplifiers.
    // NOTE: this does not do any validity checks, just sums up the stats.
    static StatsData GetTotalWeaponInstanceStats(
        const GameDataContainer& game_data_container,
        const CombatWeaponInstanceData& weapon_instance);
    StatsData GetTotalWeaponInstanceStats(const CombatWeaponInstanceData& weapon_instance) const;

    // Can equip the weapon with the weapon_type_id on any ranger
    bool CanEquipNormalWeapon(const CombatWeaponTypeID& weapon_type_id, std::string* out_error_message = nullptr) const;

    // Can equip the amplifier_type_id on the weapon_type_id
    bool CanEquipAmplifierToWeaponInstance(
        const CombatWeaponInstanceData& weapon_instance,
        const CombatWeaponTypeID& amplifier_type_id,
        std::string* out_error_message = nullptr) const;
    bool CanEquipAmplifierToWeaponTypeID(
        const CombatWeaponTypeID& weapon_type_id,
        const CombatWeaponTypeID& amplifier_type_id,
        std::string* out_error_message = nullptr) const;

    /**
     * Retrieves and merges the abilities data for a specified ability type based on
     * the provided combat unit data. The merging process includes default ability data
     * and, if applicable, specific weapon amplifier abilities.
     *
     *  NOTE: Suits are not handled here they are added later with AddDataInnateAbilities
     *
     * @param full_data The complete combat unit data from which the ability data is merged.
     * @param ability_type The specific type of ability (e.g., attack, omega, innate) for which
     *        abilities data is retrieved and merged.
     * @param out_abilities_data Pointer to the abilities data structure where the consolidated
     *        abilities data will be stored. This must not be null.
     * @param out_error Pointer to a string for storing a potential error message in case of
     *        failure. This can be null if no error message is required.
     * @return Returns true if the abilities data is successfully retrieved and merged;
     *         otherwise, false if an error occurs.
     */
    bool GetMergedAbilitiesDataForAbilityType(
        const FullCombatUnitData& full_data,
        const AbilityType ability_type,
        AbilitiesData* out_abilities_data,
        std::string* out_error = nullptr) const;

private:
    /**
     * Merges abilities data from a weapon amplifier into the specified output abilities data.
     * Abilities from the weapon amplifier are either added or replace existing abilities based on
     * their update type. For abilities marked with `kAdd`, they are appended to the output data.
     * For abilities marked with `kReplace`, they replace existing abilities in the output data by name.
     *
     * @param weapon_amplifier_abilities The abilities data associated with the weapon amplifier that
     *        is to be merged into the output abilities data.
     * @param out_abilities_data Pointer to the abilities data where the merged results will be stored.
     *        This pointer must be valid and should not be null.
     */
    void MergeWeaponAmplifierAbilitiesData(
        const AbilitiesData& weapon_amplifier_abilities,
        AbilitiesData* out_abilities_data) const;

    /**
     * Collects and stores effect data replacements from the given weapon data into the
     * current instance's effect data replacements container. This method ensures that
     * duplicate replacements are not included and logs any conflicts or new entries.
     *
     * @param weapon_data A shared pointer to the weapon data that contains effect
     *                    data replacements to be collected. If the provided weapon
     *                    data does not have replacements, the method returns early.
     */
    void CollectEffectDataReplacements(const std::shared_ptr<const CombatUnitWeaponData>& weapon_data) const;

    /**
     * Applies effect data replacements to the abilities data for a given ability type.
     * This method checks the abilities within the provided `AbilitiesData` object to determine
     * if any effects require replacements, and then applies the necessary modifications.
     *
     * @param ability_type The type of ability for which the effect data replacements are being applied.
     * @param out_abilities_data A pointer to the abilities data structure that will be modified
     *                           with the applied effect data replacements.
     */
    void ApplyEffectDataReplacements(const AbilityType ability_type, AbilitiesData* out_abilities_data) const;

    bool DoesContainAnyEffectForReplacement(const std::vector<EffectData>& effects_data) const;
    bool DoesEffectPackageContainsAnyEffectForReplacement(const EffectPackage& effect_package) const;
    bool DoesAbilityDataContainsAnyEffectForReplacement(const std::shared_ptr<const AbilityData>& ability_data) const;

    void ReplaceAllEffectsInsideEffectData(const std::string& context, EffectData& out_effect_data) const;
    void ReplaceAllEffectsInsideEffectPackage(const std::string& context, EffectPackage& out_effect_package) const;
    void ReplaceAllEffectsDataInsideAbilityData(
        const std::string& context,
        std::shared_ptr<AbilityData>& out_ability_data) const;

    /**
     * A mutable shared pointer to an instance of `EffectDataReplacements`, used to
     * store and manage replacement mappings for effect data within the current instance.
     * This member variable is intended for internal modifications, such as collecting new
     * replacements from weapon data or applying them to abilities data dynamically during
     * operation. Ensures thread-safe access with shared ownership of the replacement data.
     */
    mutable std::shared_ptr<EffectDataReplacements> effect_data_replacements_;

    // Owner world of this helper
    World* world_ = nullptr;
};
}  // namespace simulation
