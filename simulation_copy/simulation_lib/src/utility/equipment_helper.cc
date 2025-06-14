#include "equipment_helper.h"

#include "data/combat_unit_data.h"
#include "data/containers/game_data_container.h"
#include "ecs/world.h"
#include "string.h"
#include "utility/enum.h"

namespace simulation
{

EquipmentHelper::EquipmentHelper(World* world) : world_(world) {}

std::shared_ptr<Logger> EquipmentHelper::GetLogger() const
{
    return world_->GetLogger();
}

void EquipmentHelper::BuildLogPrefixFor(const EntityID id, std::string* out) const
{
    world_->BuildLogPrefixFor(id, out);
}

int EquipmentHelper::GetTimeStepCount() const
{
    return world_->GetTimeStepCount();
}

std::tuple<CombatClass, CombatAffinity> EquipmentHelper::GetWeaponDominants(const CombatWeaponTypeID& type_id) const
{
    const auto& game_data_container = world_->GetGameDataContainer();
    if (const auto weapon_data = game_data_container.GetWeaponData(type_id))
    {
        return {weapon_data->dominant_combat_class, weapon_data->dominant_combat_affinity};
    }

    return {CombatClass::kNone, CombatAffinity::kNone};
}

CombatClass EquipmentHelper::GetDefaultTypeDataCombatClass(
    const GameDataContainer& game_data_container,
    const FullCombatUnitData& full_data)
{
    if (full_data.GetCombatUnitTypeID().type == CombatUnitType::kRanger)
    {
        CombatClass combat_class = CombatClass::kNone;
        if (const auto weapon_data = game_data_container.GetWeaponData(full_data.GetEquippedWeaponTypeID()))
        {
            combat_class = weapon_data->combat_class;
        }

        return combat_class;
    }

    return full_data.data.type_data.combat_class;
}

CombatClass EquipmentHelper::GetDefaultTypeDataCombatClass(const FullCombatUnitData& full_data) const
{
    return GetDefaultTypeDataCombatClass(world_->GetGameDataContainer(), full_data);
}

CombatAffinity EquipmentHelper::GetDefaultTypeDataCombatAffinity(
    const GameDataContainer& game_data_container,
    const FullCombatUnitData& full_data)
{
    if (full_data.GetCombatUnitTypeID().type == CombatUnitType::kRanger)
    {
        CombatAffinity combat_affinity = CombatAffinity::kNone;
        if (const auto weapon_data = game_data_container.GetWeaponData(full_data.GetEquippedWeaponTypeID()))
        {
            combat_affinity = weapon_data->combat_affinity;
        }

        return combat_affinity;
    }

    return full_data.data.type_data.combat_affinity;
}

CombatAffinity EquipmentHelper::GetDefaultTypeDataCombatAffinity(const FullCombatUnitData& full_data) const
{
    return GetDefaultTypeDataCombatAffinity(world_->GetGameDataContainer(), full_data);
}

// For ranger add some extra fields like max health from the ranger combat unit data
static constexpr std::array<StatType, 1> kRangerStatTypesToAddToDefault{
    StatType::kMaxHealth,
};

StatsData EquipmentHelper::GetDefaultTypeDataStatsData(
    const GameDataContainer& game_data_container,
    const FullCombatUnitData& full_data)
{
    if (full_data.GetCombatUnitTypeID().type == CombatUnitType::kRanger)
    {
        // Add total from weapons
        StatsData result = GetTotalWeaponInstanceStats(game_data_container, full_data.instance.equipped_weapon);

        // Plus Suits
        if (const auto suit_data = game_data_container.GetSuitData(full_data.GetEquippedSuitTypeID()))
        {
            result += suit_data->stats;
        }

        for (const StatType stat_type : kRangerStatTypesToAddToDefault)
        {
            result.Add(stat_type, full_data.data.type_data.stats.Get(stat_type));
        }

        return result;
    }

    // Non ranger
    return full_data.data.type_data.stats;
}

StatsData EquipmentHelper::GetDefaultTypeDataStatsData(const FullCombatUnitData& full_data) const
{
    return GetDefaultTypeDataStatsData(world_->GetGameDataContainer(), full_data);
}

FixedPoint EquipmentHelper::GetDefaultTypeDataStatValueForType(
    const GameDataContainer& game_data_container,
    const FullCombatUnitData& full_data,
    const StatType stat_type)
{
    if (full_data.GetCombatUnitTypeID().type == CombatUnitType::kRanger)
    {
        FixedPoint result = 0_fp;

        const CombatWeaponInstanceData& weapon_instance = full_data.instance.equipped_weapon;
        if (const auto weapon_data = game_data_container.GetWeaponData(weapon_instance.type_id))
        {
            result += weapon_data->stats.Get(stat_type);
        }

        // Amplifiers
        for (const CombatWeaponAmplifierInstanceData& equipped_amplifier : weapon_instance.equipped_amplifiers)
        {
            if (const auto amplifier_weapon_data = game_data_container.GetWeaponData(equipped_amplifier.type_id))
            {
                result += amplifier_weapon_data->stats.Get(stat_type);
            }
        }

        if (const auto suit_data = game_data_container.GetSuitData(full_data.GetEquippedSuitTypeID()))
        {
            result += suit_data->stats.Get(stat_type);
        }

        // Any extra stat to add?
        for (const StatType ranger_stat_type : kRangerStatTypesToAddToDefault)
        {
            if (ranger_stat_type == stat_type)
            {
                result += full_data.data.type_data.stats.Get(stat_type);
                break;
            }
        }

        return result;
    }

    return full_data.data.type_data.stats.Get(stat_type);
}

FixedPoint EquipmentHelper::GetDefaultTypeDataStatValueForType(
    const FullCombatUnitData& full_data,
    const StatType stat_type) const
{
    return GetDefaultTypeDataStatValueForType(world_->GetGameDataContainer(), full_data, stat_type);
}

StatsData EquipmentHelper::GetTotalWeaponInstanceStats(
    const GameDataContainer& game_data_container,
    const CombatWeaponInstanceData& weapon_instance)
{
    // Normal weapon
    StatsData total_weapon_stats;
    if (const auto weapon_data = game_data_container.GetWeaponData(weapon_instance.type_id))
    {
        total_weapon_stats = weapon_data->stats;
    }

    // Amplifiers
    for (const CombatWeaponAmplifierInstanceData& equipped_amplifier : weapon_instance.equipped_amplifiers)
    {
        if (const auto amplifier_weapon_data = game_data_container.GetWeaponData(equipped_amplifier.type_id))
        {
            total_weapon_stats += amplifier_weapon_data->stats;
        }
    }

    return total_weapon_stats;
}

StatsData EquipmentHelper::GetTotalWeaponInstanceStats(const CombatWeaponInstanceData& weapon_instance) const
{
    return GetTotalWeaponInstanceStats(world_->GetGameDataContainer(), weapon_instance);
}

// Gets the default abilities data for the corresponding unit type and ability type
// NOTE: This does not retrieve any ability types
static const AbilitiesData* GetDefaultAbilitiesDataForCombatUnitTypeInternal(
    const GameDataContainer& game_data_container,
    const FullCombatUnitData& full_data,
    const AbilityType ability_type)
{
    // Ranger
    if (full_data.GetCombatUnitTypeID().type == CombatUnitType::kRanger)
    {
        const auto weapon_data = game_data_container.GetWeaponData(full_data.GetEquippedWeaponTypeID());
        if (!weapon_data)
        {
            return nullptr;  // No weapon data found
        }

        return weapon_data->GetAbilitiesDataForAbilityType(ability_type);
    }

    // Not a Ranger
    const auto& type_data = full_data.data.type_data;
    return type_data.GetAbilitiesDataForAbilityType(ability_type);
}

bool EquipmentHelper::CanEquipNormalWeapon(const CombatWeaponTypeID& weapon_type_id, std::string* out_error_message)
    const
{
    const auto weapon_data = world_->GetGameDataContainer().GetWeaponData(weapon_type_id);
    if (!weapon_data)
    {
        if (out_error_message)
        {
            *out_error_message = fmt::format("Weapon data for type_id: {}, DOES NOT EXIST!", weapon_type_id);
        }

        return false;
    }

    // Check if it's a normal weapon
    if (weapon_data->weapon_type != WeaponType::kNormal)
    {
        if (out_error_message)
        {
            *out_error_message = fmt::format(
                "Can't equip weapon_type = {}, type_id: {}. Only Normal weapons can be equipped",
                weapon_data->weapon_type,
                weapon_type_id);
        }

        return false;
    }

    return true;
}

bool EquipmentHelper::CanEquipAmplifierToWeaponInstance(
    const CombatWeaponInstanceData& weapon_instance,
    const CombatWeaponTypeID& amplifier_type_id,
    std::string* out_error_message) const
{
    const size_t max_weapon_amplifiers = world_->GetBattleConfig().max_weapon_amplifiers;
    if (weapon_instance.equipped_amplifiers.size() > max_weapon_amplifiers)
    {
        *out_error_message = fmt::format(
            "Can't equip amplifier_type_id: {} ON weapon_instance: {}. Already at maximum equipped amplifiers, "
            "max_weapon_amplifiers = {}",
            amplifier_type_id,
            weapon_instance,
            max_weapon_amplifiers);

        return false;
    }

    return CanEquipAmplifierToWeaponTypeID(weapon_instance.type_id, amplifier_type_id, out_error_message);
}

bool EquipmentHelper::CanEquipAmplifierToWeaponTypeID(
    const CombatWeaponTypeID& weapon_type_id,
    const CombatWeaponTypeID& amplifier_type_id,
    std::string* out_error_message) const
{
    const std::vector<CombatWeaponTypeID>& whitelist_allowed_amplifiers =
        world_->GetGameDataContainer().GetWeaponsDataContainer().GetAllAmplifiersTypeIDsForWeapon(weapon_type_id);
    for (const CombatWeaponTypeID& amplifier_id : whitelist_allowed_amplifiers)
    {
        if (amplifier_id == amplifier_type_id)
        {
            return true;
        }
    }

    if (out_error_message)
    {
        *out_error_message = fmt::format(
            "Can't equip amplifier_type_id: {} ON weapon_type_id: {}. This amplifier type is not for this particular "
            "weapon",
            amplifier_type_id,
            weapon_type_id);
    }
    return false;
}

bool EquipmentHelper::GetMergedAbilitiesDataForAbilityType(
    const FullCombatUnitData& full_data,
    const AbilityType ability_type,
    AbilitiesData* out_abilities_data,
    std::string* out_error) const
{
    assert(out_abilities_data);
    if (!out_abilities_data)
    {
        if (out_error)
        {
            *out_error = "Invalid out_abilities_data param.";
        }
        return false;
    }
    const GameDataContainer& game_data_container = world_->GetGameDataContainer();

    // Get default pre merging
    const AbilitiesData* default_abilities_data = nullptr;

    ILLUVIUM_ENSURE_ENUM_SIZE(AbilityType, 4);
    switch (ability_type)
    {
    case AbilityType::kAttack:
    case AbilityType::kOmega:
    case AbilityType::kInnate:
    {
        // Create a copy we
        default_abilities_data =
            GetDefaultAbilitiesDataForCombatUnitTypeInternal(game_data_container, full_data, ability_type);
        break;
    }
    default:
        if (out_error)
        {
            *out_error = "Invalid AbilityType provided.";
        }
        assert(false && "Invalid AbilityType passed to EquipmentHelper::GetMergedAbilitiesDataForAbilityType");
        return false;
    }

    // Should never reach here
    assert(default_abilities_data);
    if (!default_abilities_data)
    {
        if (out_error)
        {
            *out_error = "Invalid default_abilities_data.";
        }
        return false;
    }

    // Copy default abilities
    *out_abilities_data = *default_abilities_data;

    // Merge weapon amplifier abilities
    if (full_data.GetCombatUnitTypeID().type == CombatUnitType::kRanger)
    {
        const auto equipped_normal_weapon_data = game_data_container.GetWeaponData(full_data.GetEquippedWeaponTypeID());
        effect_data_replacements_ = EffectDataReplacements::Create();
        CollectEffectDataReplacements(equipped_normal_weapon_data);

        for (const CombatWeaponAmplifierInstanceData& weapon_amplifier_instance :
             full_data.instance.equipped_weapon.equipped_amplifiers)
        {
            const auto amplifier_weapon_data = game_data_container.GetWeaponData(weapon_amplifier_instance.type_id);
            if (!amplifier_weapon_data)
            {
                continue;
            }

            const AbilitiesData* weapon_amplifier_abilities_data =
                amplifier_weapon_data->GetAbilitiesDataForAbilityType(ability_type);
            if (!weapon_amplifier_abilities_data)
            {
                continue;
            }

            LogDebug(
                "EquipmentHelper::GetMergedAbilitiesDataForAbilityType - Merging abilities (ability_type = {}) of "
                "combat "
                "unit: {} - WITH abilities from weapon amplifier: {}",
                ability_type,
                full_data.instance,
                weapon_amplifier_instance);

            // Collect for each weapon what replecements it has to ensure uniqnuess
            CollectEffectDataReplacements(amplifier_weapon_data);

            // Merge the abilities first
            MergeWeaponAmplifierAbilitiesData(*weapon_amplifier_abilities_data, out_abilities_data);
        }

        // Apply the effect data replacements if any
        ApplyEffectDataReplacements(ability_type, out_abilities_data);
    }

    effect_data_replacements_ = nullptr;
    return true;
}

void EquipmentHelper::MergeWeaponAmplifierAbilitiesData(
    const AbilitiesData& weapon_amplifier_abilities,
    AbilitiesData* out_abilities_data) const
{
    assert(out_abilities_data);
    static constexpr std::string_view method_name = "EquipmentHelper::MergeWeaponAmplifierAbilitiesData";

    for (const std::shared_ptr<const AbilityData>& weapon_ability_ptr : weapon_amplifier_abilities.abilities)
    {
        if (weapon_ability_ptr->update_type == AbilityUpdateType::kAdd)
        {
            LogDebug("{} - Adding weapon amplifier ability with name = {}", method_name, weapon_ability_ptr->name);
            out_abilities_data->AddAbility(weapon_ability_ptr);
        }
        else if (weapon_ability_ptr->update_type == AbilityUpdateType::kReplace)
        {
            // Find weapon
            LogDebug("{} - Trying to find replacement ability with name = {}", method_name, weapon_ability_ptr->name);

            std::vector<std::shared_ptr<const AbilityData>>& abilities_vector = out_abilities_data->abilities;
            auto found_iterator = std::find_if(
                abilities_vector.begin(),
                abilities_vector.end(),
                [&](const std::shared_ptr<const AbilityData>& ability_ptr)
                {
                    return ability_ptr->name == weapon_ability_ptr->name;
                });

            if (found_iterator != abilities_vector.end())
            {
                LogDebug("{} - Found ability with name to replace: {}", method_name, weapon_ability_ptr->name);
                *found_iterator = weapon_ability_ptr;
            }
            else
            {
                LogErr("{} - Ability to replace NOT FOUND: {}", method_name, weapon_ability_ptr->name);
            }
        }
    }
}

void EquipmentHelper::CollectEffectDataReplacements(
    const std::shared_ptr<const CombatUnitWeaponData>& weapon_data) const
{
    assert(effect_data_replacements_);
    static constexpr std::string_view method_name = "EquipmentHelper::CollectEffectDataReplacements";

    // Does not have one
    if (!weapon_data->effect_data_replacements)
    {
        return;
    }

    for (const EffectData& new_effect_data : weapon_data->effect_data_replacements->replacements)
    {
        if (effect_data_replacements_->ContainsID(new_effect_data.id))
        {
            LogErr("{} - Effect Replacement with ID = {} is already defined", method_name, new_effect_data.id);
            continue;
        }

        LogDebug(
            "{} - Found Effect Data replacement with id = {}, type_id: {}",
            method_name,
            new_effect_data.id,
            new_effect_data.type_id);
        effect_data_replacements_->replacements.push_back(new_effect_data);
    }
}

void EquipmentHelper::ApplyEffectDataReplacements(const AbilityType ability_type, AbilitiesData* out_abilities_data)
    const
{
    assert(out_abilities_data);
    static constexpr std::string_view method_name = "EquipmentHelper::ApplyEffectDataReplacements";

    if (effect_data_replacements_->replacements.empty())
    {
        LogDebug("{} - No effect data replacements", method_name);
        return;
    }

    // Key: Index in abilities array
    // Value: Original ability array
    std::unordered_map<size_t, std::shared_ptr<const AbilityData>> abilities_to_replace_map;

    // Figure out what abilities need replacing
    for (size_t index = 0; index < out_abilities_data->abilities.size(); index++)
    {
        const std::shared_ptr<const AbilityData>& ability_data = out_abilities_data->abilities[index];

        if (DoesAbilityDataContainsAnyEffectForReplacement(ability_data))
        {
            LogDebug("{} - Found effect for replacement in ability_name = {}", method_name, ability_data->name);
            abilities_to_replace_map[index] = ability_data;
        }
    }

    // Recreate those abilities
    for (const auto& [index, original_ability] : abilities_to_replace_map)
    {
        const std::string context = fmt::format(
            "Root AbilityType = {}, AbilityName = {}, ExtraContextPath = ",
            ability_type,
            original_ability->name);
        auto copy_ability = original_ability->CreateDeepCopy();

        ReplaceAllEffectsDataInsideAbilityData(context, copy_ability);

        // Ovewrite ability
        out_abilities_data->abilities[index] = copy_ability;
    }
}

bool EquipmentHelper::DoesContainAnyEffectForReplacement(const std::vector<EffectData>& effects_data) const
{
    for (const EffectData& effect_data : effects_data)
    {
        // Effect itself
        if (effect_data_replacements_->ContainsID(effect_data.id))
        {
            return true;
        }

        // Attached Effect Package attributes with propagation effect package
        if (const auto& propagation_effect_package =
                effect_data.attached_effect_package_attributes.propagation.effect_package)
        {
            if (DoesEffectPackageContainsAnyEffectForReplacement(*propagation_effect_package))
            {
                return true;
            }
        }

        // Attached effects
        if (DoesContainAnyEffectForReplacement(effect_data.attached_effects))
        {
            return true;
        }

        // Attached abilities
        for (const auto& attached_ability : effect_data.attached_abilities)
        {
            if (DoesAbilityDataContainsAnyEffectForReplacement(attached_ability))
            {
                return true;
            }
        }
    }

    return false;
}

bool EquipmentHelper::DoesEffectPackageContainsAnyEffectForReplacement(const EffectPackage& effect_package) const
{
    // Propagation effect package
    if (const auto& propagation_effect_package = effect_package.attributes.propagation.effect_package)
    {
        if (DoesEffectPackageContainsAnyEffectForReplacement(*propagation_effect_package))
        {
            return true;
        }
    }

    // Search effects
    if (DoesContainAnyEffectForReplacement(effect_package.effects))
    {
        return true;
    }

    return false;
}

bool EquipmentHelper::DoesAbilityDataContainsAnyEffectForReplacement(
    const std::shared_ptr<const AbilityData>& ability_data) const
{
    for (const std::shared_ptr<SkillData>& skill_data : ability_data->skills)
    {
        if (DoesEffectPackageContainsAnyEffectForReplacement(skill_data->effect_package))
        {
            return true;
        }
    }

    return false;
}

void EquipmentHelper::ReplaceAllEffectsInsideEffectData(const std::string& context, EffectData& out_effect_data) const
{
    // Matches DoesContainAnyEffectForReplacement
    static constexpr std::string_view method_name = "EquipmentHelper::ReplaceAllEffectsInsideEffectData";

    // Effect itself
    if (const EffectData* replacement_effect_data = effect_data_replacements_->GetEffectDataForID(out_effect_data.id))
    {
        LogDebug(
            "{} - context = {} - Found effect replacement for id = {}, original effect type: {}, replacement effect "
            "type: {}",
            method_name,
            context + ".Effect",
            out_effect_data.id,
            out_effect_data.type_id,
            replacement_effect_data->type_id);

        out_effect_data = *replacement_effect_data;
        return;
    }

    // Attached Effect Package attributes with propagation effect package
    if (auto& propagation_effect_package =
            out_effect_data.attached_effect_package_attributes.propagation.effect_package)
    {
        ReplaceAllEffectsInsideEffectPackage(
            context + ".AttachedEffectPackageAttributesPropagation",
            *propagation_effect_package);
    }

    // Attached effects
    for (EffectData& attached_effect_data : out_effect_data.attached_effects)
    {
        ReplaceAllEffectsInsideEffectData(context + ".AttachedEffect", attached_effect_data);
    }

    // Attached abilities
    for (auto& attached_ability : out_effect_data.attached_abilities)
    {
        auto copy_attached_ability = attached_ability->CreateDeepCopy();
        ReplaceAllEffectsDataInsideAbilityData(context + " .AttachedAbility", copy_attached_ability);

        // Copy new data
        attached_ability = copy_attached_ability;
    }
}

void EquipmentHelper::ReplaceAllEffectsInsideEffectPackage(
    const std::string& context,
    EffectPackage& out_effect_package) const
{
    // Matches DoesEffectPackageContainsAnyEffectForReplacement
    // static constexpr std::string_view method_name = "EquipmentHelper::ReplaceAllEffectsInsideEffectPackage";

    // Propagation effect package
    if (auto& propagation_effect_package = out_effect_package.attributes.propagation.effect_package)
    {
        ReplaceAllEffectsInsideEffectPackage(context + ".AttributesPropagation", *propagation_effect_package);
    }

    // Search effects
    for (EffectData& effect_data : out_effect_package.effects)
    {
        ReplaceAllEffectsInsideEffectData(context + ".EffectPackage", effect_data);
    }
}

void EquipmentHelper::ReplaceAllEffectsDataInsideAbilityData(
    const std::string& context,
    std::shared_ptr<AbilityData>& out_ability_data) const
{
    // static constexpr std::string_view method_name = "EquipmentHelper::ReplaceAllEffectsDataInsideAbilityData";

    // Matches DoesAbilityDataContainsAnyEffectForReplacement
    for (const std::shared_ptr<SkillData>& skill_data : out_ability_data->skills)
    {
        ReplaceAllEffectsInsideEffectPackage(context + " .Skill", skill_data->effect_package);
    }
}

}  // namespace simulation
