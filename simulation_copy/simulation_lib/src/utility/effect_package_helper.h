#pragma once

#include <unordered_set>

#include "data/effect_package.h"
#include "data/enums.h"
#include "data/skill_data.h"
#include "utility/logger_consumer.h"

namespace simulation
{
class World;
class Entity;
class AttachedEffectState;

/* -------------------------------------------------------------------------------------------------------
 * EffectPackageHelper
 *
 * Helper methods for an an effect package and handling empowers
 * --------------------------------------------------------------------------------------------------------
 */
class EffectPackageHelper final : public LoggerConsumer
{
public:
    EffectPackageHelper() = default;
    explicit EffectPackageHelper(World* world);

    //
    // LoggerConsumer interface
    //

    std::shared_ptr<Logger> GetLogger() const override;
    void BuildLogPrefixFor(const EntityID id, std::string* out_string) const override;
    int GetTimeStepCount() const override;
    std::string_view GetLoggerCategoryName() const override
    {
        return LogCategory::kEffect;
    }

    //
    // Own functions
    //

    // Helper function to get the empower effect package for an ability
    void GetEmpowerEffectPackageForAbility(
        const Entity& entity,
        const AbilityType ability_type,
        const bool is_critical,
        EffectPackage* out_effect_package,
        std::unordered_set<AttachedEffectState*>* out_used_consumable_empowers) const;

    // Create a new SkillData if empower_effects is not empty
    std::shared_ptr<SkillData> CreateSkillFromEmpowerEffectPackage(
        const std::shared_ptr<SkillData>& from_data,
        const EffectPackage& empower_effect_package) const;

    // Merges the empower_effect_package into the out_effect_package
    // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44205000/Empower
    void MergeEmpowerEffectPackage(const EffectPackage& empower_effect_package, EffectPackage* out_effect_package)
        const;

    // Helper method to determine if the activated_ability is matched by any value inside ability_types
    bool DoesAbilityTypesMatchActivatedAbility(
        const std::vector<AbilityType>& ability_types,
        const AbilityType activated_ability) const;

private:
    // Owner world of this helper
    World* world_ = nullptr;
};

}  // namespace simulation
