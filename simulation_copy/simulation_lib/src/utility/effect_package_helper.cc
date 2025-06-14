#include "effect_package_helper.h"

#include "components/attached_entity_component.h"
#include "ecs/world.h"
#include "utility/attached_effects_helper.h"

namespace simulation
{

EffectPackageHelper::EffectPackageHelper(World* world) : world_(world) {}

std::shared_ptr<Logger> EffectPackageHelper::GetLogger() const
{
    return world_->GetLogger();
}

void EffectPackageHelper::BuildLogPrefixFor(const EntityID id, std::string* out) const
{
    world_->BuildLogPrefixFor(id, out);
}

int EffectPackageHelper::GetTimeStepCount() const
{
    return world_->GetTimeStepCount();
}

void EffectPackageHelper::GetEmpowerEffectPackageForAbility(
    const Entity& entity,
    const AbilityType ability_type,
    const bool is_critical,
    EffectPackage* out_effect_package,
    std::unordered_set<AttachedEffectState*>* out_used_consumable_empowers) const
{
    const EntityID entity_id = entity.GetID();

    // Get empowers/disempowers from attached effects
    // NOTE: Order is important here, this should be before we apply the skill effects
    std::vector<AttachedEffectStatePtr> all_empowers;
    std::vector<AttachedEffectStatePtr> all_disempowers;

    if (entity.Has<AttachedEffectsComponent>())
    {
        const auto& attached_effects_component = entity.Get<AttachedEffectsComponent>();
        all_empowers = attached_effects_component.GetEmpowers();
        all_disempowers = attached_effects_component.GetDisempowers();
    }

    // Get empowers from shields
    if (entity.Has<AttachedEntityComponent>())
    {
        const auto& shield_component = entity.Get<AttachedEntityComponent>();
        const auto& shield_empowers = shield_component.GetEmpowers();
        for (const auto& empower : shield_empowers)
        {
            all_empowers.push_back(empower);
        }

        const auto& shield_disempowers = shield_component.GetDisempowers();
        for (const auto& disempower : shield_disempowers)
        {
            all_disempowers.push_back(disempower);
        }
    }

    // No empowers/disempowers
    if (all_empowers.empty() && all_disempowers.empty())
    {
        return;
    }

    LogDebug(
        entity_id,
        "| Empowers effects size = {}, Disempowers effects size = {}",
        all_empowers.size(),
        all_disempowers.size());

    // Empowers
    const AttachedEffectsHelper& attached_effects_helper = world_->GetAttachedEffectsHelper();
    for (const AttachedEffectStatePtr& empower_attached_effect : all_empowers)
    {
        if (!attached_effects_helper
                 .CanUseEmpowerOrDisempower(entity_id, ability_type, empower_attached_effect, true, is_critical))
        {
            continue;
        }

        if (out_used_consumable_empowers && empower_attached_effect->IsConsumable())
        {
            out_used_consumable_empowers->insert(empower_attached_effect.get());
        }

        // Send effects from empower
        const EffectData& empower_effect_data = empower_attached_effect->effect_data;
        if (empower_effect_data.is_used_as_global_effect_attribute)
        {
            // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44205000/Empower#Adding-an-Effect-Attribute
            LogDebug(entity_id, "| Adding empower - global effect attribute");
            out_effect_package->effects.push_back(empower_effect_data);
        }
        else
        {
            LogDebug(
                entity_id,
                "| Adding empowers - attached_effects size = {}",
                empower_effect_data.attached_effects.size());
            for (const auto& effect_data : empower_effect_data.attached_effects)
            {
                out_effect_package->effects.push_back(effect_data);
            }
        }

        // Add the empower effect package attributes
        out_effect_package->attributes += empower_effect_data.attached_effect_package_attributes;

        // If empower creates propagation, save source context to propagation
        if (!out_effect_package->attributes.propagation.IsEmpty())
        {
            out_effect_package->attributes.propagation.source_context =
                empower_attached_effect->effect_state.source_context;
        }
    }

    // Disempowers only effect package attributes
    for (const AttachedEffectStatePtr& disempower_attached_effect : all_disempowers)
    {
        if (!attached_effects_helper
                 .CanUseEmpowerOrDisempower(entity_id, ability_type, disempower_attached_effect, false, is_critical))
        {
            continue;
        }
        const EffectData& disempower_effect_data = disempower_attached_effect->effect_data;

        // Subtract the empower effect package attributes
        out_effect_package->attributes -= disempower_effect_data.attached_effect_package_attributes;
    }
}

std::shared_ptr<SkillData> EffectPackageHelper::CreateSkillFromEmpowerEffectPackage(
    const std::shared_ptr<SkillData>& from_data,
    const EffectPackage& empower_effect_package) const
{
    // Create a copy of the skill
    auto new_skill_data = from_data->CreateDeepCopy();

    // Merge the effect package
    MergeEmpowerEffectPackage(empower_effect_package, &new_skill_data->effect_package);

    return new_skill_data;
}

void EffectPackageHelper::MergeEmpowerEffectPackage(
    const EffectPackage& empower_effect_package,
    EffectPackage* out_effect_package) const
{
    assert(out_effect_package);

    // Empower effect package is empty, nothing to do
    if (empower_effect_package.IsEmpty())
    {
        return;
    }

    LogDebug(
        "| MergeEmpowerEffectPackage - empower_effect_package.effects.size() = {}",
        empower_effect_package.effects.size());

    // Merge attributes
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44205000/Empower#Adding-an-Effect-Package-Attribute
    out_effect_package->attributes += empower_effect_package.attributes;

    // Add the new effects
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44205000/Empower#Adding-an-Effect
    std::vector<EffectData> global_effect_attributes;
    for (const EffectData& empower_effect : empower_effect_package.effects)
    {
        if (empower_effect.is_used_as_global_effect_attribute)
        {
            global_effect_attributes.push_back(empower_effect);
        }
        else
        {
            // Add new empower
            out_effect_package->AddEffect(empower_effect);
        }
    }

    // Handle effect attributes
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44205000/Empower#Adding-an-Effect-Attribute
    for (EffectData& effect : out_effect_package->effects)
    {
        // Walk each global effect attribute and see if it matches the current effect type
        for (const EffectData& empower_effect_attribute : global_effect_attributes)
        {
            // Effect attribute from empower is not for this type
            if (effect.type_id != empower_effect_attribute.empower_for_effect_type_id)
            {
                continue;
            }

            // Merge Effect attributes
            effect.attributes += empower_effect_attribute.attributes;
        }
    }
}

bool EffectPackageHelper::DoesAbilityTypesMatchActivatedAbility(
    const std::vector<AbilityType>& abilities_types,
    const AbilityType activated_ability) const
{
    // Invalid case
    if (activated_ability == AbilityType::kNone)
    {
        return false;
    }

    for (const AbilityType ability_type : abilities_types)
    {
        // ability type matches exactly
        if (ability_type == activated_ability)
        {
            return true;
        }
    }

    return false;
}

}  // namespace simulation
