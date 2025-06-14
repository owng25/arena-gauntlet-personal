#include "utility/targeting_helper.h"

#include "components/abilities_component.h"
#include "components/combat_unit_component.h"
#include "components/filtering_component.h"
#include "components/focus_component.h"
#include "components/position_component.h"
#include "components/shield_component.h"
#include "data/containers/game_data_container.h"
#include "ecs/world.h"
#include "utility/entity_helper.h"
#include "utility/enum.h"
#include "utility/intersection_helper.h"

namespace simulation
{
TargetingHelper::TargetingHelper(World* world) : world_(world) {}

std::shared_ptr<Logger> TargetingHelper::GetLogger() const
{
    return world_->GetLogger();
}

void TargetingHelper::BuildLogPrefixFor(const EntityID id, std::string* out) const
{
    world_->BuildLogPrefixFor(id, out);
}

int TargetingHelper::GetTimeStepCount() const
{
    return world_->GetTimeStepCount();
}

void TargetingHelper::GetEntitiesOfSkillTarget(
    const EntityID sender_id,
    const AbilityState* ability_state,
    const SkillData& skill_data,
    const std::unordered_set<EntityID>& ignored_targets,
    SkillTargetFindResult* out_result) const
{
    assert(out_result);

    // Clear to empty
    out_result->receiver_ids.clear();
    out_result->true_sender_id = sender_id;

    // Sender id does not exist
    if (!world_->HasEntity(sender_id))
    {
        assert(false);
        return;
    }

    // Get sender entity and focus
    const auto& sender_entity = world_->GetByID(sender_id);

    // Get target relative to owner of shield, not shield itself
    if (EntityHelper::IsAShield(*world_, sender_id))
    {
        const auto& shield_state_component = sender_entity.Get<ShieldComponent>();
        out_result->true_sender_id = shield_state_component.GetSenderID();
    }

    // Different depending on the SkillTargetingType
    switch (skill_data.targeting.type)
    {
    case SkillTargetingType::kCurrentFocus:
    {
        if (!sender_entity.Has<FocusComponent>())
        {
            assert(false);
            return;
        }
        const auto& focus_component = sender_entity.Get<FocusComponent>();
        if (focus_component.IsFocusActive())
        {
            // Can only ever be a single target
            const EntityID focus_id = focus_component.GetFocus()->GetID();
            if (ignored_targets.count(focus_id) == 0)
            {
                out_result->receiver_ids.push_back(focus_id);
            }
        }
        break;
    }

    case SkillTargetingType::kSelf:
    {
        // Single target of self
        if (ignored_targets.count(out_result->true_sender_id) == 0)
        {
            out_result->receiver_ids.push_back(out_result->true_sender_id);
        }
        break;
    }

    case SkillTargetingType::kInZone:
        out_result->receiver_ids = GetEntitiesWithinRange(
            out_result->true_sender_id,
            skill_data.targeting.group,
            skill_data.targeting.radius_units,
            skill_data.targeting.self,
            skill_data.targeting.only_current_focusers,
            ignored_targets);
        break;

    case SkillTargetingType::kDistanceCheck:
        out_result->receiver_ids = GetEntitiesDistanceCheck(
            out_result->true_sender_id,
            skill_data.targeting.group,
            skill_data.targeting.lowest,
            skill_data.targeting.num,
            skill_data.targeting.self,
            ignored_targets);
        break;

    case SkillTargetingType::kCombatStatCheck:
        out_result->receiver_ids = GetEntitiesCombatStatCheck(
            out_result->true_sender_id,
            skill_data.targeting.group,
            skill_data.targeting.lowest,
            skill_data.targeting.stat_type,
            skill_data.targeting.num,
            skill_data.targeting.self,
            ignored_targets);
        break;

    case SkillTargetingType::kExpressionCheck:
    {
        out_result->receiver_ids = GetEntitiesExpressionCheck(
            out_result->true_sender_id,
            skill_data.targeting.group,
            skill_data.targeting.lowest,
            skill_data.targeting.expression,
            skill_data.targeting.num,
            skill_data.targeting.self,
            ignored_targets);
        break;
    }

    case SkillTargetingType::kAllegiance:
        out_result->receiver_ids = GetEntitiesOfAllegianceType(
            out_result->true_sender_id,
            skill_data.targeting.group,
            skill_data.targeting.self,
            ignored_targets);
        break;

    case SkillTargetingType::kSynergy:
        out_result->receiver_ids = GetEntitiesOfCombatSynergy(
            out_result->true_sender_id,
            skill_data.targeting.group,
            skill_data.targeting.combat_synergy,
            skill_data.targeting.not_combat_synergy,
            skill_data.targeting.self,
            ignored_targets);
        break;

    case SkillTargetingType::kTier:
        out_result->receiver_ids = GetEntitiesOfTier(
            out_result->true_sender_id,
            skill_data.targeting.group,
            skill_data.targeting.tier,
            skill_data.targeting.num,
            ignored_targets);
        break;

    case SkillTargetingType::kVanquisher:
    {
        const EntityID vanquisher_id = world_->GetCombatUnitVanquisherID(out_result->true_sender_id);
        if (vanquisher_id == kInvalidEntityID)
        {
            LogErr(
                sender_id,
                "| GetEntitiesOfSkillTarget - kVanquisher doesn't NOT HAVE ANY VANQUISHER. TARGETING IS NOT USED "
                "CORRECTLY.");
            break;
        }

        LogDebug(sender_id, "| GetEntitiesOfSkillTarget - vanquisher_id = {}", vanquisher_id);
        out_result->receiver_ids = {vanquisher_id};
        break;
    }

    case SkillTargetingType::kPreviousTargetList:
    {
        out_result->receiver_ids = GetEntitiesOfPreviousSkillTarget(out_result->true_sender_id, ignored_targets);
        break;
    }

    case SkillTargetingType::kActivator:
    {
        if (!ability_state)
        {
            LogErr(
                sender_id,
                "| GetEntitiesOfSkillTarget - should not be called for kActivator targeting without ability");
            assert(false);
            break;
        }

        LogDebug(sender_id, "| GetEntitiesOfSkillTarget - activator_contexts = {}", ability_state->activator_contexts);

        out_result->receiver_ids.clear();

        if (ability_state->data->activation_trigger_data.trigger_type == ActivationTriggerType::kOnDodge)
        {
            if (!ability_state->activator_contexts.empty())
            {
                // NOTE: For dodge we use receiver of the last activator context
                out_result->receiver_ids.push_back(
                    ability_state->activator_contexts.back().receiver_combat_unit_entity_id);
            }
            else
            {
                LogErr(
                    sender_id,
                    "| GetEntitiesOfSkillTarget - activator_contexts is empty when trying to use it for trigger_type = "
                    "Dodge");
            }
        }
        else
        {
            for (const auto& context : ability_state->activator_contexts)
            {
                out_result->receiver_ids.push_back(context.sender_combat_unit_entity_id);
            }
        }
        break;
    }

    case SkillTargetingType::kDensity:
        out_result->receiver_ids = GetEntitiesWithBestDensity(out_result->true_sender_id, skill_data, ignored_targets);
        break;

    case SkillTargetingType::kPets:
        out_result->receiver_ids =
            GetPetEntities(out_result->true_sender_id, skill_data.targeting.group, ignored_targets);
        break;

    default:
        // We should always have targeting type set in skills
        LogErr(sender_id, "| GetEntitiesOfSkillTarget - Targeting type is not set");
        assert(false);
        break;
    }

    // Remove any receivers that do not match the targeting guidance
    if (EntityHelper::IsACombatUnit(*world_, sender_id))
    {
        out_result->receiver_ids =
            FilterEntitiesForGuidance(skill_data.targeting.guidance, sender_id, out_result->receiver_ids);
    }
}

std::vector<EntityID> TargetingHelper::GetEntitiesOfAllegianceType(
    const EntityID sender_id,
    const AllegianceType allegiance_type,
    const bool targeting_self,
    const std::unordered_set<EntityID>& ignored_targets) const
{
    const std::vector<EntityID> found_entities = GetEntitiesOfGroup(sender_id, allegiance_type, targeting_self);

    // Filter
    std::vector<EntityID> receiver_ids = FilterTargetEntities(sender_id, found_entities, 0, ignored_targets);

    LogDebug(sender_id, "| GetEntitiesOfAllegianceType  - found targets = [{}]", receiver_ids);

    return receiver_ids;
}

std::vector<EntityID> TargetingHelper::GetEntitiesOfCombatSynergy(
    const EntityID sender_id,
    const AllegianceType allegiance_type,
    const CombatSynergyBonus combat_synergy,
    const CombatSynergyBonus not_combat_synergy,
    const bool targeting_self,
    const std::unordered_set<EntityID>& ignored_targets) const
{
    const SynergiesHelper& synergies_helper = world_->GetSynergiesHelper();
    auto is_ignored = [&](const EntityID entity_id) -> bool
    {
        if (!EntityHelper::IsACombatUnit(*world_, entity_id))
        {
            return true;
        }

        // Found this entity contains this combat synergy
        const auto& entity = world_->GetByID(entity_id);
        if (!combat_synergy.IsEmpty() && !synergies_helper.HasCombatSynergy(entity, combat_synergy))
        {
            return true;
        }

        // Found this entity does not contain unwanted combat synergy
        if (!not_combat_synergy.IsEmpty() && synergies_helper.HasCombatSynergy(entity, not_combat_synergy))
        {
            return true;
        }

        return false;
    };

    const std::vector<EntityID> found_entities =
        GetEntitiesOfGroup(sender_id, allegiance_type, targeting_self, is_ignored);

    // Filter
    std::vector<EntityID> receiver_ids = FilterTargetEntities(sender_id, found_entities, 0, ignored_targets);

    LogDebug(sender_id, "| GetEntitiesOfCombatSynergy  - found targets = [{}]", receiver_ids);

    return receiver_ids;
}

std::vector<EntityID> TargetingHelper::GetEntitiesOfTier(
    const EntityID sender_id,
    const AllegianceType allegiance_type,
    const int tier,
    const size_t targeting_num,
    const std::unordered_set<EntityID>& ignored_targets) const
{
    constexpr bool targeting_self = true;
    const std::vector<EntityID> found_entities = GetEntitiesOfGroup(
        sender_id,
        allegiance_type,
        targeting_self,
        [&](const EntityID entity_id) -> bool
        {
            if (ignored_targets.contains(entity_id))
            {
                return true;
            }

            const Entity& entity = world_->GetByID(entity_id);
            if (!entity.Has<CombatUnitComponent>())
            {
                return true;
            }

            const auto& combat_unit_component = entity.Get<CombatUnitComponent>();
            if (combat_unit_component.IsARanger())
            {
                return true;  // ignore rangers
            }

            const CombatUnitTypeID& type_id = combat_unit_component.GetTypeID();

            const auto& game_data_container = world_->GetGameDataContainer();

            // TODO Moved copy tier to CombatUnitComponent ?
            const std::shared_ptr<const CombatUnitData> combat_unit_data_ptr =
                game_data_container.GetCombatUnitData(type_id);
            if (!combat_unit_data_ptr)
            {
                return true;
            }

            // ignore everything except equal tier
            return combat_unit_data_ptr->type_data.tier != tier;
        });

    // Filter
    std::vector<EntityID> receiver_ids = FilterTargetEntities(sender_id, found_entities, 0, ignored_targets);

    if (targeting_num != 0)
    {
        receiver_ids = SelectRandomEntities(receiver_ids, targeting_num);
    }

    LogDebug(sender_id, "| GetEntitiesOfTier  - found targets = [{}]", receiver_ids);

    return receiver_ids;
}

static HexGridPosition GetEntityPosition(const Entity& entity)
{
    if (!entity.Has<PositionComponent>())
    {
        return {0, 0};
    }

    const auto& position_component = entity.Get<PositionComponent>();
    return position_component.GetPosition();
}

static HexGridPosition GetEntityPosition(const World& world, EntityID entity_id)
{
    return GetEntityPosition(world.GetByID(entity_id));
}

void TargetingHelper::GetEntitiesWithinRange(
    const EntityID sender_id,
    const AllegianceType allegiance_type,
    const int targeting_radius_units,
    const bool targeting_self,
    const bool only_current_focusers,
    const std::unordered_set<EntityID>& ignored_targets,
    std::vector<EntityID>* out_entities) const
{
    static constexpr std::string_view method_name = "TargetingHelper::GetEntitiesWithinRange";
    const HexGridPosition sender_position = GetEntityPosition(*world_, sender_id);

    EntityID must_be_focused = kInvalidEntityID;
    if (only_current_focusers)
    {
        auto entity = world_->GetByIDPtr(sender_id);

        // Want to take parent if the sender entity is a pet or not a combat unit
        while (entity && (!EntityHelper::IsACombatUnit(*entity) || EntityHelper::IsAPetCombatUnit(*entity)))
        {
            entity = world_->GetByIDPtr(entity->GetParentID());
        }

        // Unlikely but it is better to handle this case
        if (!entity)
        {
            LogDebug(
                sender_id,
                "{} - could not find an illuvial parent, which is required for current focusers targeting filter",
                method_name);
            return;
        }

        must_be_focused = entity->GetID();
    }

    // Ignore entities that too far away
    const auto is_ignored_by_distance = [&](const Entity& entity) -> bool
    {
        const HexGridPosition entity_position = GetEntityPosition(entity);
        const int distance = (sender_position - entity_position).Length();
        return distance > targeting_radius_units;
    };

    const auto is_ignored_by_focusing = [&](const Entity& entity) -> bool
    {
        return entity.Has<FocusComponent>() && entity.Get<FocusComponent>().GetFocusID() != must_be_focused;
    };

    // Add extra check to filtering function only if this check is required
    std::function<bool(EntityID)> is_ignored{};
    if (only_current_focusers)
    {
        is_ignored = [&](const EntityID entity_id)
        {
            const Entity& entity = world_->GetByID(entity_id);
            return is_ignored_by_distance(entity) || is_ignored_by_focusing(entity);
        };
    }
    else
    {
        is_ignored = [&](const EntityID entity_id)
        {
            const Entity& entity = world_->GetByID(entity_id);
            return is_ignored_by_distance(entity);
        };
    }

    const std::vector<EntityID> found_entities =
        GetEntitiesOfGroup(sender_id, allegiance_type, targeting_self, is_ignored);

    // Filter by targets
    FilterTargetEntities(sender_id, found_entities, 0, ignored_targets, out_entities);

    LogDebug(sender_id, "| GetEntitiesWithingRange - found targets = [{}]", *out_entities);
}

// This utility sorts entities by a value returned by sort_value_getter
// This is supposed to be used when sort_value_getter is heavy-weight function
// which we don't want to call n log(n) times during sort.
template <typename SortValueGetter>
static void SortEntitiesBy(std::vector<EntityID>& entities, const bool ascending, SortValueGetter&& sort_value_getter)
{
    // Use return value type of sort_value_getter as value type of map
    using MapValueType = std::decay_t<decltype(sort_value_getter(EntityID{}))>;

    // Save all values associated with entities
    std::unordered_map<EntityID, MapValueType> associated_values;
    associated_values.reserve(entities.size());
    for (const EntityID entity_id : entities)
    {
        associated_values[entity_id] = sort_value_getter(entity_id);
    }

    if (ascending)
    {
        // Sort the list from lowest to highest associated value
        std::sort(
            entities.begin(),
            entities.end(),
            [&](const EntityID a, const EntityID b)
            {
                const auto a_value = associated_values[a];
                const auto b_value = associated_values[b];
                if (a_value == b_value)
                {
                    return a < b;
                }

                return a_value < b_value;
            });
    }
    else
    {
        // Sort the list from highest to lowest associated value
        std::sort(
            entities.begin(),
            entities.end(),
            [&](const EntityID a, const EntityID b)
            {
                const auto a_value = associated_values[a];
                const auto b_value = associated_values[b];
                if (a_value == b_value)
                {
                    return a > b;
                }

                return a_value > b_value;
            });
    }
}

std::vector<EntityID> TargetingHelper::GetEntitiesDistanceCheck(
    const EntityID sender_id,
    const AllegianceType allegiance_type,
    const bool targeting_lowest,
    const size_t targeting_num,
    const bool targeting_self,
    const std::unordered_set<EntityID>& ignored_targets) const
{
    std::vector<EntityID> found_entities = GetEntitiesOfGroup(sender_id, allegiance_type, targeting_self);

    // Sort found entities by the distance to sender
    const HexGridPosition sender_position = GetEntityPosition(*world_, sender_id);
    SortEntitiesBy(
        found_entities,
        targeting_lowest,
        [&](const EntityID entity_id)
        {
            const HexGridPosition entity_position = GetEntityPosition(*world_, entity_id);
            const int distance = (sender_position - entity_position).Length();
            return distance;
        });

    // Filter by targets ids
    std::vector<EntityID> receiver_ids =
        FilterTargetEntities(sender_id, found_entities, targeting_num, ignored_targets);

    LogDebug(sender_id, "| GetEntitiesDistanceCheck - found targets = [{}]", receiver_ids);

    return receiver_ids;
}

std::vector<EntityID> TargetingHelper::GetEntitiesCombatStatCheck(
    const EntityID sender_id,
    const AllegianceType allegiance_type,
    const bool targeting_lowest,
    const StatType targeting_stat_type,
    const size_t targeting_num,
    const bool targeting_self,
    const std::unordered_set<EntityID>& ignored_targets) const
{
    std::vector<EntityID> found_entities = GetEntitiesOfGroup(sender_id, allegiance_type, targeting_self);

    // Sort found entities by live stat value
    SortEntitiesBy(
        found_entities,
        targeting_lowest,
        [&](const EntityID entity_id)
        {
            return world_->GetLiveStats(entity_id).Get(targeting_stat_type);
        });

    // Filter
    std::vector<EntityID> receiver_ids =
        FilterTargetEntities(sender_id, found_entities, targeting_num, ignored_targets);

    LogDebug(sender_id, "| GetEntitiesCombatStatCheck - found targets = [{}]", receiver_ids);

    return receiver_ids;
}

std::vector<EntityID> TargetingHelper::GetEntitiesExpressionCheck(
    const EntityID sender_id,
    const AllegianceType allegiance_type,
    const bool targeting_lowest,
    const EffectExpression& expression,
    const size_t targeting_num,
    const bool targeting_self,
    const std::unordered_set<EntityID>& ignored_targets) const
{
    std::vector<EntityID> found_entities = GetEntitiesOfGroup(sender_id, allegiance_type, targeting_self);

    // Sort found entities by expression result
    SortEntitiesBy(
        found_entities,
        targeting_lowest,
        [&](const EntityID entity_id)
        {
            const ExpressionEvaluationContext expression_context(world_, entity_id, entity_id);
            return expression.Evaluate(expression_context);
        });

    // Filter
    std::vector<EntityID> receiver_ids =
        FilterTargetEntities(sender_id, found_entities, targeting_num, ignored_targets);

    LogDebug(sender_id, "| GetEntitiesExpressionCheck - found targets = [{}]", receiver_ids);

    return receiver_ids;
}

std::vector<EntityID> TargetingHelper::FilterEntitiesForGuidance(
    const EnumSet<GuidanceType>& targeting_guidance,
    const EntityID sender_id,
    const std::vector<EntityID>& target_entities) const
{
    std::vector<EntityID> receiver_ids;
    for (const auto& entity : target_entities)
    {
        if (DoesEntityMatchesGuidance(targeting_guidance, sender_id, entity))
        {
            // Matches specified guidance, add to output
            receiver_ids.push_back(entity);
        }
    }

    return receiver_ids;
}

EntityID TargetingHelper::GetFurthestEnemyEntity(
    EntityID sender_id,
    const std::unordered_set<EntityID>& ignored_targets) const
{
    constexpr bool closest = false;
    constexpr int max_targets = 1;
    constexpr bool targeting_self = false;
    const std::vector<EntityID> result = GetEntitiesDistanceCheck(
        sender_id,
        AllegianceType::kEnemy,
        closest,
        max_targets,
        targeting_self,
        ignored_targets);

    assert(result.size() <= 1);
    return result.size() == 1 ? result.front() : kInvalidEntityID;
}

bool TargetingHelper::DoesEntityMatchesGuidance(
    const EnumSet<GuidanceType>& guidance_types,
    const EntityID sender_id,
    const EntityID receiver_id) const
{
    if (sender_id == receiver_id)
    {
        // Self always matches
        return true;
    }

    if (!world_->HasEntity(sender_id))
    {
        LogWarn(sender_id, "| DoesEntityMatchesGuidance - sender entity does not exist");
        return false;
    }

    const auto& sender_entity = world_->GetByID(sender_id);

    if (!world_->HasEntity(receiver_id))
    {
        // No such entity
        LogWarn(receiver_id, "| DoesEntityMatchesGuidance - receiver entity does not exist");
        return false;
    }

    const auto& receiver_entity = world_->GetByID(receiver_id);

    // Ignore plane change with allies
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/277610967/Plane+Change#Enemies-Only
    if (sender_entity.IsAlliedWith(receiver_entity))
    {
        return true;
    }

    if (!receiver_entity.Has<AttachedEffectsComponent>())
    {
        // Can only be grounded
        return guidance_types.Contains(GuidanceType::kGround);
    }

    const auto& attached_effects_component = receiver_entity.Get<AttachedEffectsComponent>();
    if (attached_effects_component.HasPlaneChange(EffectPlaneChange::kAirborne))
    {
        // Airborne matches
        return guidance_types.Contains(GuidanceType::kAirborne);
    }
    if (attached_effects_component.HasPlaneChange(EffectPlaneChange::kUnderground))
    {
        // Underground matches
        return guidance_types.Contains(GuidanceType::kUnderground);
    }

    // No plane change effect, matches ground
    return guidance_types.Contains(GuidanceType::kGround);
}

EnumSet<GuidanceType> TargetingHelper::GetTargetingGuidanceForEntity(const Entity& entity) const
{
    if (!entity.Has<AbilitiesComponent>())
    {
        // Just return default here
        LogWarn(entity.GetID(), "| GetTargetingGuidanceForEntityFocus - entity does not have abilities");
        return MakeEnumSet(GuidanceType::kGround);
    }

    const auto& abilities_component = entity.Get<AbilitiesComponent>();
    const auto& attack_abilities = abilities_component.GetDataAttackAbilities().abilities;
    const auto& active_ability = abilities_component.GetActiveAbility();

    // If we have an active ability, use that
    if (active_ability != nullptr)
    {
        const auto* skill_data = active_ability->data->GetSkillData(0);
        if (skill_data)
        {
            return skill_data->targeting.guidance;
        }
    }

    // Else iterate the attack abilities to find guidances shared by all
    const auto all_guidances = MakeEnumSet(GuidanceType::kGround, GuidanceType::kAirborne, GuidanceType::kUnderground);
    EnumSet<GuidanceType> result = all_guidances;
    for (const auto& attack_ability : attack_abilities)
    {
        const auto* skill_data = attack_ability->GetSkillData(0);
        result.Remove(all_guidances - skill_data->targeting.guidance);
    }

    // Defaults to ground
    if (result.IsEmpty())
    {
        LogWarn(entity.GetID(), "| GetTargetingGuidanceForEntityFocus - no common guidance for attack abilities");
        return MakeEnumSet(GuidanceType::kGround);
    }
    return result;
}

void TargetingHelper::FilterTargetEntities(
    const EntityID sender_id,
    const std::vector<EntityID>& all_target_entities,
    const size_t targeting_num,
    const std::unordered_set<EntityID>& targets_to_ignore,
    std::vector<EntityID>* out_result) const
{
    if (!world_->HasEntity(sender_id))
    {
        return;
    }

    const Entity& sender_entity = world_->GetByID(sender_id);
    const FilteringComponent* filter =
        sender_entity.Has<FilteringComponent>() ? &sender_entity.Get<FilteringComponent>() : nullptr;

    std::vector<EntityID>& receiver_ids = *out_result;
    receiver_ids.clear();
    receiver_ids.reserve(targeting_num);

    std::vector<EntityID> low_priority_targets;

    for (const EntityID& receiver_id : all_target_entities)
    {
        // Found already all the targets we want
        if (targeting_num != 0 && receiver_ids.size() == targeting_num)
        {
            break;
        }

        // Check filter
        if (filter)
        {
            if (filter->HasOldTarget(receiver_id))
            {
                // Ignore if old target
                if (filter->GetOnlyNewTargets())
                {
                    continue;
                }

                // Ignore if old target but also keep track of duplicates
                if (filter->GetPrioritizeNewTargets())
                {
                    low_priority_targets.push_back(receiver_id);
                    continue;
                }
            }
        }
        else if (targets_to_ignore.count(receiver_id) > 0)
        {
            continue;
        }

        // Add to the found list
        receiver_ids.push_back(receiver_id);
    }

    // Add low priority targets to receiver ids until it reaches desired count
    for (size_t i = 0; i != low_priority_targets.size() && receiver_ids.size() < targeting_num; ++i)
    {
        receiver_ids.push_back(low_priority_targets[i]);
    }
}

std::vector<EntityID> TargetingHelper::SelectRandomEntities(
    const std::vector<EntityID>& all_target_entities,
    const size_t max_num) const
{
    if (max_num == 0 || all_target_entities.size() <= max_num)
    {
        return all_target_entities;
    }

    std::vector<EntityID> selected_entities;
    selected_entities.reserve(max_num);

    std::unordered_set<int> chosen_indices;
    while (selected_entities.size() < max_num)
    {
        const int random_index = world_->RandomRange(0, static_cast<int>(all_target_entities.size()));
        if (chosen_indices.insert(random_index).second)
        {
            const size_t index = static_cast<size_t>(random_index);
            selected_entities.push_back(all_target_entities[index]);
        }
    }

    return selected_entities;
}

std::vector<EntityID> TargetingHelper::GetEntitiesOfGroup(
    const EntityID sender_id,
    const AllegianceType allegiance_type,
    const bool targeting_self,
    const std::function<bool(EntityID)>& is_ignored) const
{
    if (!world_->HasEntity(sender_id))
    {
        return {};
    }

    std::vector<EntityID> found_entities;

    // Get all entities on the board
    const auto& sender_entity = world_->GetByID(sender_id);
    const auto& all_entities = world_->GetAll();

    auto add_found_entity = [&](const Entity& found_entity)
    {
        auto targetable_allegiance = AllegianceType::kEnemy;
        if (found_entity.IsAlliedWith(sender_entity))
        {
            targetable_allegiance = AllegianceType::kAlly;
        }

        // Confirm we only select targetable entities
        if (!EntityHelper::IsTargetable(found_entity, targetable_allegiance))
        {
            return;
        }

        // Is this entity ignored?
        if (is_ignored && is_ignored(found_entity.GetID()))
        {
            return;
        }

        found_entities.push_back(found_entity.GetID());
    };

    // Just add self and return early
    if (allegiance_type == AllegianceType::kSelf)
    {
        add_found_entity(sender_entity);
        return found_entities;
    }

    for (const auto& other_entity : all_entities)
    {
        const EntityID other_id = other_entity->GetID();

        // Avoid targeting self
        if (sender_id == other_id && !targeting_self)
        {
            continue;
        }

        // Add entities to a list within based on agency
        switch (allegiance_type)
        {
        case AllegianceType::kAll:
        {
            add_found_entity(*other_entity);
            break;
        }
        case AllegianceType::kAlly:
        {
            if (other_entity->IsAlliedWith(sender_entity))
            {
                add_found_entity(*other_entity);
            }
            break;
        }
        case AllegianceType::kEnemy:
        {
            if (!other_entity->IsAlliedWith(sender_entity))
            {
                add_found_entity(*other_entity);
            }
            break;
        }
        default:
            break;
        }
    }

    return found_entities;
}

std::vector<EntityID> TargetingHelper::GetEntitiesOfPreviousSkillTarget(
    const EntityID sender_id,
    const std::unordered_set<EntityID>& ignored_targets) const
{
    if (!world_->HasEntity(sender_id))
    {
        return {};
    }

    const auto& sender_entity = world_->GetByID(sender_id);
    const auto& abilities_component = sender_entity.Get<AbilitiesComponent>();
    const AbilityStatePtr& active_ability = abilities_component.GetActiveAbility();
    if (!active_ability)
    {
        LogErr(sender_id, "| GetEntitiesOfPreviousSkillTarget - function misused, entity doesn't have active ability");
        return {};
    }

    const size_t skill_index = active_ability->GetCurrentSkillIndex();

    // NOTE: If this is the first Skill we default to SELF.
    // However, this should NEVER be used for this.
    if (skill_index == 0)
    {
        return {sender_id};
    }
    const size_t previous_skill_index = skill_index - 1;
    const auto& previous_skill_state = active_ability->skills.at(previous_skill_index);

    std::vector<EntityID> receiver_ids;
    for (EntityID receiver_id : previous_skill_state.targeting_state.available_targets)
    {
        if (world_->IsCombatUnitAlive(receiver_id) && ignored_targets.count(receiver_id) == 0)
        {
            receiver_ids.push_back(receiver_id);
        }
    }

    return receiver_ids;
}

static bool WouldHexZoneIntersectPosition(
    const World&,
    const SkillData& skill_data,
    const HexGridPosition /*sender_position*/,
    const int /*sender_radius_units*/,
    const HexGridPosition zone_position,
    const Entity& intersect_entity)
{
    if (!intersect_entity.Has<PositionComponent>())
    {
        return false;
    }

    const HexGridPosition other_entity_position = intersect_entity.Get<PositionComponent>().GetPosition();
    return IntersectionHelper::DoesHexZoneIntersectEntity(
        skill_data.zone.radius_units,
        zone_position,
        other_entity_position);
}

static bool WouldRectZoneIntersectPosition(
    const World&,
    const SkillData& skill_data,
    const HexGridPosition /*sender_position*/,
    const int /*sender_radius_units*/,
    const HexGridPosition zone_position,
    const Entity& intersect_entity)
{
    if (!intersect_entity.Has<PositionComponent>())
    {
        return false;
    }

    const HexGridPosition other_entity_position = intersect_entity.Get<PositionComponent>().GetPosition();
    return IntersectionHelper::DoesRectangleZoneIntersectEntity(
        zone_position.ToSubUnits(),
        other_entity_position.ToSubUnits(),
        Math::UnitsToSubUnits(skill_data.zone.width_units),
        Math::UnitsToSubUnits(skill_data.zone.height_units));
}

static bool WouldTriangleZoneIntersectPosition(
    const World& world,
    const SkillData& skill_data,
    const HexGridPosition sender_position,
    const int /*sender_radius_units*/,
    const HexGridPosition zone_position,
    const Entity& intersect_entity)
{
    if (!intersect_entity.Has<PositionComponent>())
    {
        return false;
    }

    // Angle the zone is facing relative to its position
    const IVector2D world_zone_position_units = world.ToWorldPosition(zone_position);
    const int spawn_angle = world.ToWorldPosition(sender_position).AngleToPosition(world_zone_position_units);
    const int direction_degrees = Math::AngleLimitTo360(spawn_angle + skill_data.zone.direction_degrees);

    return IntersectionHelper::DoesTriangleZoneIntersectEntity(
        world,
        zone_position,
        direction_degrees,
        skill_data.zone.radius_units,
        intersect_entity.Get<PositionComponent>().GetPosition());
}

static bool WouldBeamIntersectPosition(
    const World& world,
    const SkillData& skill_data,
    const HexGridPosition beam_position,
    const int /*sender_radius_units*/,
    const HexGridPosition target_position,
    const Entity& other_entity)
{
    if (!other_entity.Has<PositionComponent>())
    {
        return false;
    }

    const IVector2D world_target_position = world.ToWorldPosition(target_position);
    const int direction = world.ToWorldPosition(beam_position).AngleToPosition(world_target_position);

    const HexGridPosition sub_units_vector = (beam_position - target_position).ToSubUnits();
    const int world_length_sub_units = world.ToWorldPosition(sub_units_vector).Length();

    const PositionComponent& other_entity_position_component = other_entity.Get<PositionComponent>();

    return IntersectionHelper::DoesBeamIntersectEntity(
        world,
        beam_position,
        direction,
        Math::UnitsToSubUnits(skill_data.beam.width_units),
        world_length_sub_units,
        other_entity_position_component.GetPosition(),
        other_entity_position_component.GetRadius());
}

static bool WouldDashIntersectPosition(
    const World& world,
    const SkillData& skill_data,
    const HexGridPosition sender_position,
    const int sender_radius_units,
    const HexGridPosition target_position,
    const Entity& other_entity)
{
    // This is not really precise algorithm.
    // Just assume dash acts in the same way as beam with entity diameter for now.
    if (!other_entity.Has<PositionComponent>())
    {
        return false;
    }

    HexGridPosition open_position;

    const GridHelper& grid_helper = world.GetGridHelper();
    grid_helper.BuildObstacles(sender_radius_units);
    // Get open spot - we know the target location is not usable for at least the sum of the radius units
    // Evaluate position criteria
    if (skill_data.dash.land_behind)
    {
        open_position = grid_helper.GetOpenPositionBehind(sender_position, target_position, sender_radius_units);
    }
    else
    {
        open_position = grid_helper.GetOpenPositionNearby(target_position, sender_radius_units);
    }

    if (open_position == kInvalidHexHexGridPosition)
    {
        return false;
    }

    const IVector2D world_target_position = world.ToWorldPosition(open_position);
    const int direction = world.ToWorldPosition(sender_position).AngleToPosition(world_target_position);

    const HexGridPosition sub_units_vector = (sender_position - open_position).ToSubUnits();
    const int world_length_sub_units = world.ToWorldPosition(sub_units_vector).Length();

    const PositionComponent& other_entity_position_component = other_entity.Get<PositionComponent>();

    return IntersectionHelper::DoesBeamIntersectEntity(
        world,
        sender_position,
        direction,
        Math::UnitsToSubUnits(1 + 2 * sender_radius_units),
        world_length_sub_units,
        other_entity_position_component.GetPosition(),
        other_entity_position_component.GetRadius());
}

static bool WouldProjectileIntersectPosition(
    const World& world,
    const SkillData& skill_data,
    const HexGridPosition sender_position,
    const int /*sender_radius_units*/,
    const HexGridPosition target_position,
    const Entity& other_entity)
{
    // This is not really precise algorithm.
    // Just assume dash acts in the same way as beam with entity diameter for now.
    if (!other_entity.Has<PositionComponent>())
    {
        return false;
    }

    const IVector2D world_target_position = world.ToWorldPosition(target_position);
    const int direction = world.ToWorldPosition(sender_position).AngleToPosition(world_target_position);

    const HexGridPosition sub_units_vector = (sender_position - target_position).ToSubUnits();
    const int world_length_sub_units = world.ToWorldPosition(sub_units_vector).Length();

    const PositionComponent& other_entity_position_component = other_entity.Get<PositionComponent>();

    return IntersectionHelper::DoesBeamIntersectEntity(
        world,
        sender_position,
        direction,
        Math::UnitsToSubUnits(1 + 2 * skill_data.projectile.size_units),
        world_length_sub_units,
        other_entity_position_component.GetPosition(),
        other_entity_position_component.GetRadius());
}

using IntersectionFunction = bool (*)(
    const World& world,
    const SkillData& skill_data,
    const HexGridPosition sender_position,
    const int sender_radius_units,
    const HexGridPosition target_position,
    const Entity& intersect_entity);

static IntersectionFunction SelectZoneIntersectionFunction(const SkillData& skill_data)
{
    assert(skill_data.deployment.type == SkillDeploymentType::kZone);

    switch (skill_data.zone.shape)
    {
    case ZoneEffectShape::kHexagon:
        return WouldHexZoneIntersectPosition;

    case ZoneEffectShape::kRectangle:
        return WouldRectZoneIntersectPosition;

    case ZoneEffectShape::kTriangle:
        return WouldTriangleZoneIntersectPosition;

    default:
        assert(false);
        break;
    }

    return nullptr;
}

static IntersectionFunction SelectIntersectionFunction(const SkillData& skill_data)
{
    switch (skill_data.deployment.type)
    {
    case SkillDeploymentType::kZone:
        return SelectZoneIntersectionFunction(skill_data);
    case SkillDeploymentType::kBeam:
        return WouldBeamIntersectPosition;
    case SkillDeploymentType::kDash:
        return WouldDashIntersectPosition;
    case SkillDeploymentType::kProjectile:
        return WouldProjectileIntersectPosition;
    default:
        break;
    }

    return nullptr;
}

// Get entities wtih best density around them
std::vector<EntityID> TargetingHelper::GetEntitiesWithBestDensity(
    const EntityID sender_id,
    const SkillData& skill_data,
    const std::unordered_set<EntityID>& ignored_targets) const
{
    std::vector<EntityID> found_entities =
        GetEntitiesOfGroup(sender_id, skill_data.targeting.group, skill_data.targeting.self);

    const IntersectionFunction intersection_checker = SelectIntersectionFunction(skill_data);

    HexGridPosition sender_position{0, 0};
    int sender_radius_units = 1;

    if (world_->HasEntity(sender_id))
    {
        const Entity& sender_entity = world_->GetByID(sender_id);
        if (sender_entity.Has<PositionComponent>())
        {
            const PositionComponent& sender_position_component = sender_entity.Get<PositionComponent>();
            sender_position = sender_position_component.GetPosition();
            sender_radius_units = sender_position_component.GetRadius();
        }
    }

    // Sort by the number of possible overlaps in case of application
    SortEntitiesBy(
        found_entities,
        skill_data.targeting.lowest,
        [&](const EntityID target_entity_id)
        {
            const Entity& target_entity = world_->GetByID(target_entity_id);
            const HexGridPosition target_position = target_entity.Get<PositionComponent>().GetPosition();

            size_t intersections_count = 0;
            for (const EntityID potential_intersection_id : found_entities)
            {
                if (target_entity_id == potential_intersection_id)
                {
                    intersections_count += 1;
                    continue;
                }

                const Entity& other_entity = world_->GetByID(potential_intersection_id);

                if (!other_entity.IsActive())
                {
                    continue;
                }

                if (intersection_checker(
                        *world_,
                        skill_data,
                        sender_position,
                        sender_radius_units,
                        target_position,
                        other_entity))
                {
                    intersections_count += 1;
                }
            }

            return intersections_count;
        });

    // Filter ignored targets and take targeting.num into account
    std::vector<EntityID> receiver_ids =
        FilterTargetEntities(sender_id, found_entities, skill_data.targeting.num, ignored_targets);

    LogDebug(sender_id, "| GetEntitiesCombatStatCheck - found targets = [{}]", receiver_ids);

    return receiver_ids;
}

std::vector<EntityID> TargetingHelper::GetPetEntities(
    const EntityID sender_id,
    const AllegianceType group,
    const std::unordered_set<EntityID>& ignored_targets) const
{
    static constexpr std::string_view method_name = "TargetingHelper::GetPetEntities";

    if (!world_->HasEntity(sender_id))
    {
        return {};
    }

    std::vector<EntityID> found_entities;

    const auto& sender_entity = world_->GetByID(sender_id);

    const auto try_add_entity = [&](const Entity& found_entity)
    {
        if (!found_entity.Has<CombatUnitComponent>())
        {
            return;
        }

        const auto& combat_unit_component = found_entity.Get<CombatUnitComponent>();
        if (!combat_unit_component.IsAPet())
        {
            return;
        }

        const auto targetable_allegiance =
            found_entity.IsAlliedWith(sender_entity) ? AllegianceType::kAlly : AllegianceType::kEnemy;

        // Confirm we only select targetable entities
        if (!EntityHelper::IsTargetable(found_entity, targetable_allegiance))
        {
            return;
        }

        // Is this entity ignored?
        if (ignored_targets.count(found_entity.GetID()))
        {
            return;
        }

        found_entities.push_back(found_entity.GetID());
    };

    for (const auto& other_entity : world_->GetAll())
    {
        switch (group)
        {
        case AllegianceType::kSelf:
        {
            if (other_entity->GetParentID() == sender_id)
            {
                try_add_entity(*other_entity);
            }
            break;
        }
        case AllegianceType::kAll:
            try_add_entity(*other_entity);
            break;

        case AllegianceType::kAlly:
            if (other_entity->IsAlliedWith(sender_entity))
            {
                try_add_entity(*other_entity);
            }
            break;

        case AllegianceType::kEnemy:
            if (!other_entity->IsAlliedWith(sender_entity))
            {
                try_add_entity(*other_entity);
            }
            break;

        default:
            break;
        }
    }

    // Filter ignored targets
    std::vector<EntityID> receiver_ids = FilterTargetEntities(sender_id, found_entities, 0, {});
    LogDebug(sender_id, "| {} - found targets = [{}]", method_name, receiver_ids);
    return receiver_ids;
}

HexGridPosition TargetingHelper::FindMaxEnemyOverlapPosition(
    const EntityID sender_id,
    const int free_radius,
    const int overlap_radius,
    const std::unordered_set<EntityID>& ignored_targets) const
{
    if (overlap_radius < free_radius)
    {
        // Wrong input: overlap radius must be bigger than free radius
        assert(false);
        return kInvalidHexHexGridPosition;
    }

    const std::vector<EntityID> enemies = GetEntitiesOfGroup(
        sender_id,
        AllegianceType::kEnemy,
        false,
        [&](const EntityID entity) -> bool
        {
            return ignored_targets.count(entity);
        });

    using CachedEntityInfo = std::tuple<const Entity*, const PositionComponent*>;
    std::vector<CachedEntityInfo> enemies_info;
    enemies_info.reserve(enemies.size());

    for (const EntityID entity_id : enemies)
    {
        const auto& entity = world_->GetByID(entity_id);
        enemies_info.emplace_back(&entity, &entity.Get<PositionComponent>());
    }

    const auto& grid_helper = world_->GetGridHelper();
    HexGridPosition best_position = kInvalidHexHexGridPosition;
    size_t best_overlap_count = 0;
    std::vector<HexGridPosition> ring_positions;

    for (const auto& [pivot_entity, pivot_entity_position_component] : enemies_info)
    {
        // Try to pick position around pivot entity
        const auto& pivot_entity_position = pivot_entity_position_component->GetPosition();
        const int pivot_entity_radius = pivot_entity_position_component->GetRadius();
        const int min_scan_radius = pivot_entity_radius + free_radius + 1;
        const int max_scan_radius = pivot_entity_radius + overlap_radius + 1;
        for (int ring_radius = min_scan_radius; ring_radius <= max_scan_radius; ++ring_radius)
        {
            ring_positions.clear();
            grid_helper.GetSingleRingPositions(pivot_entity_position, ring_radius, &ring_positions);
            for (const HexGridPosition& position : ring_positions)
            {
                if (grid_helper.HasObstacleAt(position))
                {
                    continue;
                }

                // Ensure position could be taken
                if (grid_helper.IsHexagonPositionTaken(position, free_radius))
                {
                    continue;
                }

                // Position is OK. Count how many enemies it overlaps with overlap radius
                size_t overlap_count = 0;
                for (const auto& [other_entity, other_entity_position_component] : enemies_info)
                {
                    const bool found_intersection = grid_helper.DoHexagonsIntersect(
                        position,
                        overlap_radius,
                        other_entity_position_component->GetPosition(),
                        other_entity_position_component->GetRadius());
                    if (found_intersection)
                    {
                        overlap_count += 1;
                    }
                }

                if (overlap_count >= best_overlap_count)
                {
                    best_overlap_count = overlap_count;
                    best_position = position;
                }
            }
        }
    }

    return best_position;
}
}  // namespace simulation
