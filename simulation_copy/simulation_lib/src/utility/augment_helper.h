#pragma once

#include "ecs/entity.h"
#include "utility/enum_map.h"
#include "utility/logger_consumer.h"

namespace simulation
{
class CombatUnitTypeID;
}

namespace simulation
{
class AbilityData;
class AugmentInstanceData;
class AugmentTypeID;
class World;

/* -------------------------------------------------------------------------------------------------------
 * AugmentHelper
 *
 * This defines the helper functions for augments
 * --------------------------------------------------------------------------------------------------------
 */
class AugmentHelper final : public LoggerConsumer
{
public:
    AugmentHelper() = default;
    explicit AugmentHelper(World* world);

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

    // Can add any augment to this entity?
    bool CanAddMoreAugments(const EntityID entity_id) const;
    bool CanAddMoreAugments(const Entity& entity) const;
    bool CanAddMoreAugments(const size_t equipped_augments_size) const;

    // Can add this particular augment?
    bool CanAddAugment(const EntityID entity_id, const AugmentTypeID& augment_type_id) const;
    bool CanAddAugment(const Entity& entity, const AugmentTypeID& augment_type_id) const;
    bool CanAddAugmentToCombatUnit(
        const CombatUnitTypeID& unit_type_id,
        const std::vector<AugmentInstanceData>& equipped_augments,
        const AugmentTypeID& add_augment_type_id) const;

    // Can remove any augment from this entity?
    bool CanRemoveAugment(const EntityID entity_id) const;
    bool CanRemoveAugment(const Entity& entity) const;

    // Does the entity has this augment instance?
    bool HasAugment(const EntityID entity_id, const AugmentInstanceData& instance) const;
    bool HasAugment(const Entity& entity, const AugmentInstanceData& instance) const;

    // Apply an augment to an entity
    bool AddAugment(const Entity& entity, const AugmentInstanceData& instance) const;

    // Remove augment from an entity
    bool RemoveAugment(const Entity& entity, const AugmentInstanceData& instance) const;

    // Adds combat affinities stacks provided by augments equipped on the entity to map
    void GetAllAugmentsCombatAffinities(
        const Entity& entity,
        EnumMap<CombatAffinity, int>* out_combat_affinities_stacks) const;
    void GetAllAugmentsCombatAffinities(
        const std::vector<AugmentInstanceData>& equipped_augments,
        EnumMap<CombatAffinity, int>* out_combat_affinities_stacks) const;

    // Adds combat classes stacks provided by augments equipped on the entity to map
    void GetAllAugmentsCombatClasses(const Entity& entity, EnumMap<CombatClass, int>* out_combat_classes_stacks) const;
    void GetAllAugmentsCombatClasses(
        const std::vector<AugmentInstanceData>& equipped_augments,
        EnumMap<CombatClass, int>* out_combat_classes_stacks) const;

    // Is augment legendary
    AugmentType GetAugmentType(const AugmentTypeID& augment_type_id) const;
    bool IsAugmentTypeIDLegendary(const AugmentTypeID& augment_type_id) const
    {
        return IsAugmentTypeLegendary(GetAugmentType(augment_type_id));
    }
    static constexpr bool IsAugmentTypeLegendary(const AugmentType augment_type)
    {
        static_assert(static_cast<int>(AugmentType::kNum) == 5, "Update this function when enumeration changes");
        switch (augment_type)
        {
        case AugmentType::kGreaterPower:
        case AugmentType::kSynergyBonus:
            return true;
        default:
            return false;
        }
    }

private:
    // Owner world of this helper
    World* world_ = nullptr;
};

}  // namespace simulation
