#pragma once

#include <string>

#include "data/beam_data.h"
#include "data/chain_data.h"
#include "data/dash_data.h"
#include "data/mark_data.h"
#include "data/projectile_data.h"
#include "data/shield_data.h"
#include "data/splash_data.h"
#include "data/zone_data.h"
#include "ecs/world.h"
#include "utility/hex_grid_position.h"

namespace simulation
{
class DroneAugmentData;
class DroneAugmentTypeID;

class AuraData;

/* -------------------------------------------------------------------------------------------------------
 * EntityFactory
 *
 * This class constructs an Entity based on its data.
 * --------------------------------------------------------------------------------------------------------
 */
class EntityFactory
{
public:
    // Spawns a combat unit
    // Set out_error_message to a valid string if you want to also find out why this errors out
    static Entity* SpawnCombatUnit(
        World& world,
        const FullCombatUnitData& full_data,
        const EntityID parent_id,
        std::string* out_error_message = nullptr);

    // Spawns a Chain Propagation, this can spawn multiple chains
    static std::vector<Entity*> SpawnChainPropagation(World& world, const Team team, const ChainData& chain_data);

    static Entity* SpawnProjectile(
        World& world,
        const Team team,
        const HexGridPosition& position,
        const HexGridPosition& target,
        const ProjectileData& projectile_data);

    static Entity* SpawnZone(World& world, const Team team, const HexGridPosition& position, ZoneData& zone_data);

    static Entity* SpawnSplash(
        World& world,
        const Team team,
        const EntityID receiver_id,
        const HexGridPosition& position,
        const SplashData& splash_data);

    static Entity* SpawnBeam(
        World& world,
        const Team team,
        const HexGridPosition& position,
        const HexGridPosition& target,
        const BeamData& beam_data);

    static Entity* SpawnDash(World& world, const Team team, const DashData& dash_data, const int skill_duration_ms);

    // Spawns a shield and attaches it to the receiver
    static Entity* SpawnShieldAndAttach(World& world, const Team team, const ShieldData& shield_data);

    // Spawns an aura and attaches it to the receiver
    static Entity* SpawnAuraAndAttach(World& world, AuraData shield_data);

    // Spawns a mark and attaches it to the receiver
    static Entity* SpawnMarkAndAttach(World& world, const Team team, const MarkData& mark_data);

    // Spawns a synergy entity for given team
    static Entity* SpawnSynergyEntity(World& world, const Team team);

    // Spawns a synergy entity for given team
    static Entity* SpawnDroneAugmentEntity(World& world, const DroneAugmentData& drone_augment_data, const Team team);

    /** Finds intersection between the ray and rectangle
     * Ray source sould not be equal to ray target.
     * @param ray_origin - ray origin. Must be inside of rectangle
     * @param ray_direction - ray direction. Must not be zero
     * @param rect_min - minimum rect corner
     * @param rect_max - maximum rect corner
     * @return - returns intersection of the ray and rectangle
     */
    static IVector2D FindRayRectIntersection(
        const IVector2D ray_origin,
        const IVector2D ray_direction,
        const IVector2D rect_min,
        const IVector2D rect_max);

private:
    // Internal method that does spawn a single chain that targets the receiver_id
    static Entity* SpawnChainToReceiver(
        World& world,
        const Team team,
        std::shared_ptr<SkillData> skill_data,
        const ChainData& chain_data,
        const EntityID receiver_id);

    // Helper method that checks if an entity can pe spawned
    static bool CanSpawnFromSender(
        const std::string_view method_context,
        const World& world,
        const HexGridPosition& position,
        const EntityID sender_id,
        EntityID* out_combat_unit_sender_id);
    static bool CanSpawnFromSenderToReceiver(
        const std::string_view method_context,
        const World& world,
        const HexGridPosition& position,
        const EntityID sender_id,
        const EntityID receiver_id,
        EntityID* out_combat_unit_sender_id);

    static bool CanSpawnFromSender(
        const std::string_view method_context,
        const World& world,
        const EntityID sender_id,
        EntityID* out_combat_unit_sender_id);
    static bool CanSpawnFromSenderToReceiver(
        const std::string_view method_context,
        const World& world,
        const EntityID sender_id,
        const EntityID receiver_id,
        EntityID* out_combat_unit_sender_id);

    // Helper to get the StatsData for a spawned entity
    static StatsData GetStatsDataForSpawnedEntity(const World& world, const EntityID combat_unit_id);

    // Helper method to set the parent of entity stats
    static void UpdateCombatUnitParentLiveStats(const World& world, const Entity& entity);

    // Helper function that tells us if we can spawn from it
    static bool
    CanSpawnFromCombatUnit(const std::string_view method_context, const World& world, const EntityID combat_unit_id);

    static int CalculateZoneSpawnDirection(
        const World& world,
        const ZoneEffectShape zone_shape,
        const HexGridPosition& zone_spawn_position,
        const Entity& combat_unit_sender);
};

}  // namespace simulation
