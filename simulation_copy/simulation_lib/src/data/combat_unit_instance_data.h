#pragma once

#include "data/augment/augment_instance_data.h"
#include "data/consumable/consumable_instance_data.h"
#include "data/stats_data.h"
#include "data/suit/suit_instance_data.h"
#include "data/weapon/weapon_instance_data.h"
#include "utility/custom_formatter.h"
#include "utility/hex_grid_position.h"

namespace simulation
{
// Instance data for a CombatUnitData
struct CombatUnitInstanceData
{
    void FormatTo(fmt::format_context& ctx) const;

    // Team this combat unit belongs to
    Team team = Team::kNone;

    // Unique ID of this unit
    std::string id;

    // To whom is this combat unit bonded with.
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/3440924/Bond
    std::string bonded_id;

    // Position of the combat unit
    HexGridPosition position;

    // The current level of this unit
    int level = kMinLevel;

    // Dominant combat affinity
    // https://illuvium.atlassian.net/wiki/spaces/GD/pages/228425729/Dominant+Synergy
    CombatAffinity dominant_combat_affinity = CombatAffinity::kNone;

    // Dominant combat class
    // https://illuvium.atlassian.net/wiki/spaces/GD/pages/228425729/Dominant+Synergy
    CombatClass dominant_combat_class = CombatClass::kNone;

    // Equipped weapon, only used for rangers
    CombatWeaponInstanceData equipped_weapon;

    // Equipped suit, only used for rangers
    CombatSuitInstanceData equipped_suit;

    // Consumable
    std::vector<ConsumableInstanceData> equipped_consumables;

    // List of all the equipped augments
    std::vector<AugmentInstanceData> equipped_augments;

    // Map of all not default unit stats
    std::unordered_map<StatType, FixedPoint> stats_overrides;

    // Random modifier stats
    StatsData random_percentage_modifier_stats;

    // Used mostly for overworld
    // https://illuvium.atlassian.net/wiki/spaces/AVATAR/pages/418545665/Illuvitar+Finish
    std::string finish;
};

}  // namespace simulation

ILLUVIUM_MAKE_STRUCT_FORMATTER(CombatUnitInstanceData, FormatTo);
