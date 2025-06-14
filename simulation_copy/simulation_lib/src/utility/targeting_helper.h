#pragma once

#include <unordered_set>
#include <vector>

#include "data/constants.h"
#include "data/skill_data.h"
#include "data/stats_data.h"
#include "utility/logger_consumer.h"

namespace simulation
{
class World;
class Entity;
class AbilityState;

/* -------------------------------------------------------------------------------------------------------
 * TargetingHelper
 *
 * This defines world targeting helper functions
 * --------------------------------------------------------------------------------------------------------
 */
class TargetingHelper final : public LoggerConsumer
{
public:
    // Struct that specifies the result of the GetEntitiesOfSkillTarget method
    struct SkillTargetFindResult
    {
        std::vector<EntityID> receiver_ids;
        EntityID true_sender_id = kInvalidEntityID;
    };

    TargetingHelper() = default;
    explicit TargetingHelper(World* world);

    //
    // LoggerConsumer interface
    //

    std::shared_ptr<Logger> GetLogger() const override;
    void BuildLogPrefixFor(const EntityID id, std::string* out_string) const override;
    int GetTimeStepCount() const override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kAbility;
    }

    //
    // Own functions
    //

    // Helper method to get all entities of a skill target
    // This internally calls the other methods from this helper depending on the target type
    // - GetEntitiesWithingRange
    // - GetEntitiesDistanceCheck
    // - GetEntitiesCombatStatCheck
    // - FilterEntitiesForGuidance
    // NOTE: ignored_targets is only used if the sender_id does not have a
    // FilteredTargetingComponent
    void GetEntitiesOfSkillTarget(
        const EntityID sender_id,
        const AbilityState* ability_state,
        const SkillData& skill_data,
        const std::unordered_set<EntityID>& ignored_targets,
        SkillTargetFindResult* out_result) const;

    // Checks whether an entity matches the specified guidance
    bool DoesEntityMatchesGuidance(
        const EnumSet<GuidanceType>& guidance_types,
        const EntityID sender_id,
        const EntityID receiver_id) const;

    // Returns targeting guidance for the entity, active or most restrictive based on attack ability only
    EnumSet<GuidanceType> GetTargetingGuidanceForEntity(const Entity& entity) const;

    std::vector<EntityID> GetEntitiesWithinRange(
        const EntityID sender_id,
        const AllegianceType allegiance_type,
        const int targeting_radius_units,
        const bool targeting_self,
        const bool only_current_focusers,
        const std::unordered_set<EntityID>& ignored_targets) const
    {
        std::vector<EntityID> ids;
        GetEntitiesWithinRange(
            sender_id,
            allegiance_type,
            targeting_radius_units,
            targeting_self,
            only_current_focusers,
            ignored_targets,
            &ids);
        return ids;
    }

    // Helper method to get all the entities in a range around the sender_id
    void GetEntitiesWithinRange(
        const EntityID sender_id,
        const AllegianceType allegiance_type,
        const int targeting_radius_units,
        const bool targeting_self,
        const bool only_current_focusers,
        const std::unordered_set<EntityID>& ignored_targets,
        std::vector<EntityID>* out_entities) const;

    // Filter entities for specified guidance
    std::vector<EntityID> FilterEntitiesForGuidance(
        const EnumSet<GuidanceType>& targeting_guidance,
        const EntityID sender_id,
        const std::vector<EntityID>& target_entities) const;

    // Helper method to get the furthest entity from the sender_id
    EntityID GetFurthestEnemyEntity(EntityID sender_id, const std::unordered_set<EntityID>& ignored_targets) const;

    // Helper method to get the position that overlaps the highest number of enemies
    HexGridPosition FindMaxEnemyOverlapPosition(
        const EntityID sender_id,
        const int free_radius,
        const int overlap_radius,
        const std::unordered_set<EntityID>& ignored_targets) const;

private:
    // Helper method to get the closest/farthest allies/enemies from the entity sender_id
    std::vector<EntityID> GetEntitiesDistanceCheck(
        const EntityID sender_id,
        const AllegianceType allegiance_type,
        const bool targeting_lowest,
        const size_t targeting_num,
        const bool targeting_self,
        const std::unordered_set<EntityID>& ignored_targets) const;

    // Helper method to get the lowest/highest stat entities from the entity sender_id
    std::vector<EntityID> GetEntitiesCombatStatCheck(
        const EntityID sender_id,
        const AllegianceType allegiance_type,
        const bool targeting_lowest,
        const StatType targeting_stat_type,
        const size_t targeting_num,
        const bool targeting_self,
        const std::unordered_set<EntityID>& ignored_targets) const;

    // Helper method to get the entities based on an expression
    std::vector<EntityID> GetEntitiesExpressionCheck(
        const EntityID sender_id,
        const AllegianceType allegiance_type,
        const bool targeting_lowest,
        const EffectExpression& expression,
        const size_t targeting_num,
        const bool targeting_self,
        const std::unordered_set<EntityID>& ignored_targets) const;

    // Helper method to get all entities of selected allegiance type
    std::vector<EntityID> GetEntitiesOfAllegianceType(
        const EntityID sender_id,
        const AllegianceType allegiance_type,
        const bool targeting_self,
        const std::unordered_set<EntityID>& ignored_targets) const;

    // Helper method to get all entities of selected synergy
    std::vector<EntityID> GetEntitiesOfCombatSynergy(
        const EntityID sender_id,
        const AllegianceType allegiance_type,
        const CombatSynergyBonus combat_synergy,
        const CombatSynergyBonus not_combat_synergy,
        const bool targeting_self,
        const std::unordered_set<EntityID>& ignored_targets) const;

    // Helper method to get all entities of selected tier
    std::vector<EntityID> GetEntitiesOfTier(
        const EntityID sender_id,
        const AllegianceType allegiance_type,
        const int tier,
        const size_t targeting_num,
        const std::unordered_set<EntityID>& ignored_targets) const;

    // Get all entities that belong to that group
    std::vector<EntityID> GetEntitiesOfGroup(
        const EntityID sender_id,
        const AllegianceType allegiance_type,
        const bool targeting_self,
        const std::function<bool(EntityID)>& is_ignored = nullptr) const;

    // Helper method to get all targets of previous skills
    std::vector<EntityID> GetEntitiesOfPreviousSkillTarget(
        const EntityID sender_id,
        const std::unordered_set<EntityID>& ignored_targets) const;

    // Get entities wtih best density around them
    std::vector<EntityID> GetEntitiesWithBestDensity(
        const EntityID sender_id,
        const SkillData& skill_data,
        const std::unordered_set<EntityID>& ignored_targets) const;

    std::vector<EntityID> GetPetEntities(
        const EntityID sender_id,
        const AllegianceType group,
        const std::unordered_set<EntityID>& ignored_targets) const;

    // Filter the receiver_ids to correspond with the current filters
    std::vector<EntityID> FilterTargetEntities(
        const EntityID sender_id,
        const std::vector<EntityID>& all_target_entities,
        const size_t targeting_num,
        const std::unordered_set<EntityID>& ignored_targets) const
    {
        std::vector<EntityID> ids;
        FilterTargetEntities(sender_id, all_target_entities, targeting_num, ignored_targets, &ids);
        return ids;
    }

    void FilterTargetEntities(
        const EntityID sender_id,
        const std::vector<EntityID>& all_target_entities,
        const size_t targeting_num,
        const std::unordered_set<EntityID>& ignored_targets,
        std::vector<EntityID>* out_result) const;

    std::vector<EntityID> SelectRandomEntities(const std::vector<EntityID>& entities, const size_t max_num) const;

private:
    // Owner world of this helper
    World* world_ = nullptr;
};

}  // namespace simulation
