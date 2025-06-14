#pragma once

#include "ability_system_data_fixtures.h"
#include "data/effect_data.h"
#include "ecs/event.h"
#include "utility/attached_effects_helper.h"

namespace simulation
{
// Variant where the attack abilities can start at the beginning of the game every second with an
// innate on attack ability
class AbilitySystemInnateTest : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return true;
    }

    virtual int GetActivationTimeLimitMs() const
    {
        return 0;
    }
    virtual int GetMaxActivations() const
    {
        return 0;
    }
    virtual int GetInnateTotalDurationMs() const
    {
        return 0;
    }

    virtual int GetPreDeploymentDelayPercentage() const
    {
        return 0;
    }

    virtual ActivationTriggerType GetActivationTrigger() const
    {
        return ActivationTriggerType::kNone;
    }

    virtual bool TriggerOnlyOnFocus() const
    {
        return false;
    }

    virtual AllegianceType GetTriggerSenderAllegiance() const
    {
        return AllegianceType::kAll;
    }

    virtual AllegianceType GetTriggerReceiverAllegiance() const
    {
        return AllegianceType::kAll;
    }

    // Empty on purpose
    void InitOmegaAbilities() override {}

    void InitInnateAbilities() override
    {
        auto& ability = data.type_data.innate_abilities.AddAbility();
        ability.name = "Bar";
        ability.activation_trigger_data.trigger_type = GetActivationTrigger();
        ability.activation_trigger_data.damage_types = GetTriggerDamageTypes();
        ability.activation_trigger_data.max_activations = GetMaxActivations();
        ability.activation_trigger_data.activation_time_limit_ms = GetActivationTimeLimitMs();
        ability.activation_trigger_data.sender_allegiance = GetTriggerSenderAllegiance();
        ability.activation_trigger_data.receiver_allegiance = GetTriggerReceiverAllegiance();
        ability.activation_trigger_data.only_focus = TriggerOnlyOnFocus();
        ability.activation_trigger_data.damage_threshold = GetTriggerDamageThreshold();
        ability.activation_trigger_data.health_lower_limit_percentage = GetHealthLowerLimitPercentage();
        PatchInnateAbilityData(ability);

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.name = "Innate Attack";
        skill1.targeting.type = GetInnateTargeting();
        skill1.deployment.type = GetInnateDeploymentType();
        // More than the shield so we affect the health too
        skill1.AddEffect(CreateInnateEffect());

        ability.total_duration_ms = GetInnateTotalDurationMs();
        skill1.percentage_of_ability_duration = 100;
        skill1.deployment.pre_deployment_delay_percentage = GetPreDeploymentDelayPercentage();

        data.type_data.innate_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual void PatchInnateAbilityData(AbilityData&) const {}

    virtual FixedPoint GetHealthLowerLimitPercentage() const
    {
        return 0_fp;
    }

    virtual SkillDeploymentType GetInnateDeploymentType() const
    {
        return SkillDeploymentType::kDirect;
    }

    virtual EffectData CreateInnateEffect() const
    {
        return EffectData::CreateDamage(EffectDamageType::kPure, EffectExpression::FromValue(100_fp));
    }

    virtual EnumSet<EffectDamageType> GetTriggerDamageTypes() const
    {
        return {};
    }

    virtual FixedPoint GetTriggerDamageThreshold() const
    {
        return 0_fp;
    }

    virtual SkillTargetingType GetInnateTargeting() const
    {
        return SkillTargetingType::kCurrentFocus;
    }

    void InitStats() override
    {
        Super::InitStats();
        // Once every 200 ms
        data.type_data.stats.Set(StatType::kAttackSpeed, 500_fp);

        data.type_data.stats.Set(StatType::kMaxHealth, 100000_fp);  // make entities not die
        // Ignore movement
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
    }

    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {10, 20}, data, red_entity);
    }

    size_t CountActivatedAbilityTypesForSenderId(
        const size_t before_index,
        const AbilityType ability_type,
        const EntityID combat_unit_sender_id) const
    {
        size_t count = 0;
        for (size_t i = 0; i < before_index && i < events_ability_activated.size(); i++)
        {
            const auto& event_data = events_ability_activated.at(i);
            if (event_data.ability_type == ability_type && event_data.combat_unit_sender_id == combat_unit_sender_id)
            {
                count++;
            }
        }

        return count;
    }

    size_t GetActivatedAbilitiesCount(AbilityType ability_type, EntityID sender = kInvalidEntityID) const
    {
        return static_cast<size_t>(std::count_if(
            events_ability_activated.begin(),
            events_ability_activated.end(),
            [&](const event_data::AbilityActivated& event)
            {
                if (event.ability_type == ability_type)
                {
                    return sender == kInvalidEntityID || sender == event.sender_id;
                }

                return false;
            }));
    }
};

// Variant With kOnMiss + kEnemy sender allegiance
class AbilitySystemInnateTest_WithOnEnemyMiss : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnMiss;
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kEnemy;
    }
};

// Variant With kOnHit (sender is enemy, receiver is ally)
class AbilitySystemInnateTest_OnAllyHitByEnemy : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    Entity* red_entity_2 = nullptr;
    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnHit;
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kEnemy;
    }

    AllegianceType GetTriggerReceiverAllegiance() const override
    {
        return AllegianceType::kAlly;
    }

    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {10, 20}, data, red_entity);
        SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity_2);
    }
};

// Variant With kOnHit (sender is self, receiver is enemy)
class AbilitySystemInnateTest_OnEnemyHitBySelf : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    Entity* red_entity_2 = nullptr;
    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnHit;
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kSelf;
    }

    AllegianceType GetTriggerReceiverAllegiance() const override
    {
        return AllegianceType::kEnemy;
    }

    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);

        // Spawn red without innate
        auto red_data = data;
        red_data.type_data.innate_abilities.abilities.clear();
        SpawnCombatUnit(Team::kRed, {10, 20}, red_data, red_entity);
    }
};

// Variant With kOnHit (sender is enemy, receiver is self)
class AbilitySystemInnateTest_OnSelfHitByEnemy : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnHit;
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kEnemy;
    }

    AllegianceType GetTriggerReceiverAllegiance() const override
    {
        return AllegianceType::kSelf;
    }
};

// Variant With kOnHit and time limit
class AbilitySystemInnateTest_WithOnHitWithTimeLimit : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    int GetActivationTimeLimitMs() const override
    {
        return 200;
    }
    int GetInnateTotalDurationMs() const override
    {
        return 100;
    }
    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnHit;
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kSelf;
    }

    AllegianceType GetTriggerReceiverAllegiance() const override
    {
        return AllegianceType::kEnemy;
    }
};

// Variant With kOnMiss
class AbilitySystemInnateTest_WithOnMissStart : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnMiss;
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kSelf;
    }
};

// Variant With kOnDamage
class AbilitySystemInnateTest_OnReceiveDamage : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnDamage;
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kAll;
    }

    AllegianceType GetTriggerReceiverAllegiance() const override
    {
        return AllegianceType::kSelf;
    }
};

// Variant With kOnDamage which triggers on physical damage only
class AbilitySystemInnateTest_OnDealPhysDamage : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnDamage;
    }

    EnumSet<EffectDamageType> GetTriggerDamageTypes() const override
    {
        return MakeEnumSet(EffectDamageType::kPhysical);
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kSelf;
    }

    AllegianceType GetTriggerReceiverAllegiance() const override
    {
        return AllegianceType::kEnemy;
    }
};

// Variant With kOnDamage which triggers on physical damage only
class AbilitySystemInnateTest_OnDealDamageAndThreshold : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnDamage;
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kSelf;
    }

    AllegianceType GetTriggerReceiverAllegiance() const override
    {
        return AllegianceType::kEnemy;
    }

    EnumSet<EffectDamageType> GetTriggerDamageTypes() const override
    {
        return MakeEnumSet(EffectDamageType::kPhysical);
    }

    FixedPoint GetTriggerDamageThreshold() const override
    {
        return 101_fp;
    }
};

// Variant With kOnDamage which triggers on energy damage dealt by self to enemy
class AbilitySystemInnateTest_WithOnDealEnergyDamage : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnDamage;
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kSelf;
    }

    AllegianceType GetTriggerReceiverAllegiance() const override
    {
        return AllegianceType::kEnemy;
    }

    EnumSet<EffectDamageType> GetTriggerDamageTypes() const override
    {
        return MakeEnumSet(EffectDamageType::kEnergy);
    }
};

// Variant With kOnDealCrit
class AbilitySystemInnateTest_WithOnDealCrit : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnDealCrit;
    }

    void InitInnateAbilities() override
    {
        Super::InitInnateAbilities();

        // Always crit
        data.type_data.stats.Set(StatType::kCritChancePercentage, kMaxPercentageFP);
        // No Dodge
        data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, kMinPercentageFP);
        // No Miss
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        // Once every second
        data.type_data.stats.Set(StatType::kAttackSpeed, 100_fp);
    }
};

// Variant With kOnActivateXAbilities
class AbilitySystemInnateTest_WithOnAbilityActivated : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    int GetMaxActivations() const override
    {
        return 3;
    }

    void InitInnateAbilities() override
    {
        auto& ability = data.type_data.innate_abilities.AddAbility();
        ability.name = "Bar";
        ability.activation_trigger_data.trigger_type = GetActivationTrigger();
        ability.activation_trigger_data.damage_types = GetTriggerDamageTypes();
        ability.activation_trigger_data.max_activations = GetMaxActivations();
        ability.activation_trigger_data.activation_time_limit_ms = GetActivationTimeLimitMs();
        ability.activation_trigger_data.sender_allegiance = GetTriggerSenderAllegiance();
        ability.activation_trigger_data.receiver_allegiance = GetTriggerReceiverAllegiance();
        ability.activation_trigger_data.only_focus = TriggerOnlyOnFocus();
        ability.activation_trigger_data.damage_threshold = GetTriggerDamageThreshold();
        ability.activation_trigger_data.every_x = true;
        ability.activation_trigger_data.number_of_abilities_activated = 1;
        ability.activation_trigger_data.ability_types = MakeEnumSet(AbilityType::kAttack);

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.name = "Innate Attack";
        skill1.targeting.type = GetInnateTargeting();
        skill1.deployment.type = GetInnateDeploymentType();
        // More than the shield so we affect the health too
        skill1.AddEffect(CreateInnateEffect());

        ability.total_duration_ms = GetInnateTotalDurationMs();
        skill1.percentage_of_ability_duration = 100;
        skill1.deployment.pre_deployment_delay_percentage = GetPreDeploymentDelayPercentage();

        data.type_data.innate_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnActivateXAbilities;
    }
};

// Variant With kOnDodge
class AbilitySystemInnateTest_WithOnDodge : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnDodge;
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kEnemy;
    }
};

// Variant With kOnDodge only_focus
class AbilitySystemInnateTest_WithOnDodgeCurrentFocus : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    Entity* red_entity_2 = nullptr;

    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {10, 20}, data, red_entity);
        SpawnCombatUnit(Team::kRed, {5, 20}, data, red_entity_2);
    }

    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnDodge;
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kEnemy;
    }

    bool TriggerOnlyOnFocus() const override
    {
        return true;
    }
};

// Variant With kOnDodge trigger and kSelf allegiance
class AbilitySystemInnateTest_WithOnDodgeApplyToSelf : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {10, 20}, data, red_entity);
    }

    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnDodge;
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kSelf;
    }

    EffectData CreateInnateEffect() const override
    {
        return EffectData::CreatePositiveState(EffectPositiveState::kInvulnerable, 5000);
    }

    SkillTargetingType GetInnateTargeting() const override
    {
        return SkillTargetingType::kSelf;
    }
};

// Variant With kHealthInRange
class AbilitySystemInnateTest_WithHealthInRange : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    FixedPoint GetHealthLowerLimitPercentage() const override
    {
        return 50_fp;
    }

    void InitStats() override
    {
        Super::InitStats();

        data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
        data.type_data.combat_affinity = CombatAffinity::kVerdant;
    }

    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kHealthInRange;
    }
};

// Variant With kOnActivateXAbilities
class AbilitySystemInnateTest_WithEveryXAttack : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    void PatchInnateAbilityData(AbilityData& ability_data) const override
    {
        ability_data.activation_trigger_data.number_of_abilities_activated = 2;
        ability_data.activation_trigger_data.every_x = true;
        ability_data.activation_trigger_data.ability_types = MakeEnumSet(AbilityType::kAttack);
    }

    void InitStats() override
    {
        Super::InitStats();

        data.type_data.combat_class = CombatClass::kBerserker;
    }

    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnActivateXAbilities;
    }
};

// Variant With kInRange
class AbilitySystemInnateTest_WithInRange : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    void SetUp() override
    {
        Super::SetUp();

        // Add an effect Wrap it in an attached effect
        const auto effect_data1 =
            EffectData::CreateCondition(EffectConditionType::kPoison, kDefaultAttachedEffectsFrequencyMs);

        EffectState effect_state1{};
        effect_state1.sender_stats = world->GetFullStats(blue_entity->GetID());

        // Add the effect
        GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity->GetID(), effect_data1, effect_state1);

        // Add an effect Wrap it in an attached effect
        const auto effect_data2 =
            EffectData::CreateCondition(EffectConditionType::kPoison, kDefaultAttachedEffectsFrequencyMs);

        EffectState effect_state2{};
        effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());

        // Add the effect
        GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, red_entity->GetID(), effect_data2, effect_state2);
    }

    void PatchInnateAbilityData(AbilityData& ability_data) const override
    {
        ability_data.activation_trigger_data.activation_radius_units = 20;
        ability_data.activation_trigger_data.required_sender_conditions = MakeEnumSet(ConditionType::kPoisoned);
        ability_data.activation_trigger_data.required_receiver_conditions = MakeEnumSet(ConditionType::kPoisoned);
    }

    void InitStats() override
    {
        Super::InitStats();

        // blue_entity is kPoison type
        data.type_data.combat_affinity = CombatAffinity::kToxic;
    }

    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kInRange;
    }
};

// Variant With kOnVanquish
class AbilitySystemInnateTest_WithOnVanquish : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    void InitInnateAbilities() override
    {
        auto& ability = data.type_data.innate_abilities.AddAbility();
        ability.activation_trigger_data.trigger_type = ActivationTriggerType::kOnVanquish;
        ability.activation_trigger_data.ability_types = MakeEnumSet(AbilityType::kAttack);
        ability.activation_trigger_data.sender_allegiance = AllegianceType::kSelf;

        // Skills
        auto& skill = ability.AddSkill();
        skill.name = "Innate Attack";
        skill.targeting.type = SkillTargetingType::kSelf;
        skill.deployment.type = SkillDeploymentType::kDirect;

        skill.AddEffect(EffectData::CreateHeal(EffectHealType::kNormal, EffectExpression::FromValue(10_fp)));

        data.type_data.innate_abilities.selection_type = AbilitySelectionType::kCycle;
    }
};

// Variant With kOnVanquish
class AbilitySystemInnateTest_OnVanquish : public AbilitySystemInnateTest
{
    using Super = AbilitySystemInnateTest;

protected:
    void InitStats() override
    {
        Super::InitStats();

        // One attack every time step
        data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
    }

    template <typename F>
    CombatUnitData ModifyInnate(F&& modify_callback) const
    {
        CombatUnitData copy = data;
        copy.type_data.innate_abilities.abilities.clear();
        modify_callback(CreateDefaultInnate(copy));
        return copy;
    }

    static AbilityData& CreateDefaultInnate(CombatUnitData& data)
    {
        auto& ability = data.type_data.innate_abilities.AddAbility();
        ability.activation_trigger_data.trigger_type = ActivationTriggerType::kOnVanquish;
        ability.activation_trigger_data.ability_types = MakeEnumSet(AbilityType::kAttack);
        ability.activation_trigger_data.every_x = true;

        // Skills
        auto& skill = ability.AddSkill();
        skill.name = "Innate Attack";
        skill.targeting.type = SkillTargetingType::kSelf;
        skill.deployment.type = SkillDeploymentType::kDirect;

        skill.AddEffect(EffectData::CreateHeal(EffectHealType::kNormal, EffectExpression::FromValue(10_fp)));
        data.type_data.innate_abilities.selection_type = AbilitySelectionType::kCycle;

        return ability;
    }

    void InitInnateAbilities() override
    {
        CreateDefaultInnate(data);
    }

    void SpawnBlue(const HexGridPosition& position, const CombatUnitData& combat_unit_data)
    {
        Entity* entity = nullptr;
        SpawnCombatUnit(Team::kBlue, position, combat_unit_data, entity);
        blue_entities_ids.push_back(entity->GetID());

        if (blue_entities_ids.size() == 1)
        {
            blue_entity = entity;
        }
    }

    void SpawnRed(const HexGridPosition& position, const CombatUnitData& combat_unit_data)
    {
        Entity* entity = nullptr;
        SpawnCombatUnit(Team::kRed, position, combat_unit_data, entity);
        red_entities_ids.push_back(entity->GetID());

        if (red_entities_ids.size() == 1)
        {
            red_entity = entity;
        }
    }

    void SpawnCombatUnits() override {}

    Entity& GetBlueEntity(size_t index)
    {
        return world->GetByID(blue_entities_ids.at(index));
    }

    Entity& GetRedEntity(size_t index)
    {
        return world->GetByID(red_entities_ids.at(index));
    }

    std::vector<EntityID> blue_entities_ids;
    std::vector<EntityID> red_entities_ids;
};

// Variant With kOnAllyFaint
class AbilitySystemInnateTest_WithOnAllyFaint : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    void SpawnCombatUnits() override
    {
        Super::SpawnCombatUnits();

        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {-10, -20}, data2, blue_entity2);
    }

    void InitStats() override
    {
        Super::InitStats();
        data2 = data;

        data2.type_data.stats.Set(StatType::kAttackRangeUnits, 100_fp);
    }

    void InitInnateAbilities() override
    {
        Super::InitInnateAbilities();

        auto& ability = data2.type_data.innate_abilities.AddAbility();
        ability.activation_trigger_data.trigger_type = GetActivationTrigger();
        ability.activation_trigger_data.max_activations = GetMaxActivations();
        ability.activation_trigger_data.activation_time_limit_ms = GetActivationTimeLimitMs();
        ability.activation_trigger_data.sender_allegiance = GetTriggerSenderAllegiance();
        ability.activation_trigger_data.receiver_allegiance = GetTriggerReceiverAllegiance();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.lowest = false;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.num = 1;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kEnergy, EffectExpression::FromValue(50_fp));

        ability.total_duration_ms = GetInnateTotalDurationMs();
        skill1.deployment.pre_deployment_delay_percentage = GetPreDeploymentDelayPercentage();

        data2.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void SetUp() override
    {
        Super::SetUp();
    }

    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnFaint;
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kAlly;
    }

    Entity* blue_entity2 = nullptr;
    CombatUnitData data2;
};

// Variant With kOnFaint
class AbilitySystemInnateTest_WithOnFaint : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    void SpawnCombatUnits() override
    {
        Super::SpawnCombatUnits();

        SpawnCombatUnit(Team::kBlue, {-10, -20}, data, blue_entity2);
    }

    void InitStats() override
    {
        Super::InitStats();
    }

    SkillTargetingType GetInnateTargeting() const override
    {
        return SkillTargetingType::kVanquisher;
    }

    EffectData CreateInnateEffect() const override
    {
        return EffectData::CreateDamage(EffectDamageType::kEnergy, EffectExpression::FromValue(50_fp));
    }

    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnFaint;
    }

    AllegianceType GetTriggerSenderAllegiance() const override
    {
        return AllegianceType::kSelf;
    }

    // Fake entity, not used
    Entity* blue_entity2 = nullptr;
};

// Variant With kOnAssist
class AbilitySystemInnateTest_WithOnAssist : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    void SpawnCombatUnits() override
    {
        SpawnCombatUnit(Team::kBlue, {0, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity2);
        SpawnCombatUnit(Team::kRed, {0, 15}, data, red_entity);
        SpawnCombatUnit(Team::kRed, {-10, -10}, data_no_range, red_entity2);
    }

    void InitStats() override
    {
        Super::InitStats();
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 100_fp);

        data_no_range = data;
        data_no_range.type_data.stats.Set(StatType::kAttackRangeUnits, 1_fp);
    }

    void InitInnateAbilities() override
    {
        auto& ability = data.type_data.innate_abilities.AddAbility();
        ability.activation_trigger_data.trigger_type = GetActivationTrigger();
        ability.activation_trigger_data.max_activations = GetMaxActivations();
        ability.activation_trigger_data.activation_time_limit_ms = GetActivationTimeLimitMs();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.lowest = true;
        skill1.targeting.num = 1;
        skill1.targeting.group = AllegianceType::kEnemy;

        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddDamageEffect(EffectDamageType::kEnergy, EffectExpression::FromValue(50_fp));

        ability.total_duration_ms = GetInnateTotalDurationMs();
        skill1.deployment.pre_deployment_delay_percentage = GetPreDeploymentDelayPercentage();

        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void SetUp() override
    {
        Super::SetUp();
    }

    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnAssist;
    }

    // Fake entity, not used
    Entity* blue_entity2 = nullptr;
    Entity* red_entity2 = nullptr;

    CombatUnitData data_no_range;
};

class AbilitySystemInnateTest_WithOnBattleStart_Blink : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    void SpawnCombatUnits() override
    {
        Super::SpawnCombatUnits();
    }

    void InitStats() override
    {
        Super::InitStats();
    }

    void InitInnateAbilities() override
    {
        auto& ability = data.type_data.innate_abilities.AddAbility();
        ability.activation_trigger_data.trigger_type = GetActivationTrigger();
        ability.activation_trigger_data.max_activations = GetMaxActivations();
        ability.activation_trigger_data.activation_time_limit_ms = GetActivationTimeLimitMs();
        ability.total_duration_ms = GetInnateTotalDurationMs();

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.targeting.type = SkillTargetingType::kSelf;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.AddEffect(EffectData::CreateBlink(ReservedPositionType::kAcross, 0, 0));

        skill1.deployment.pre_deployment_delay_percentage = GetPreDeploymentDelayPercentage();
    }

    void SetUp() override
    {
        Super::SetUp();

        // Listen to events
        world->SubscribeToEvent(
            EventType::kBlinked,
            [this](const Event& event)
            {
                events_blinked.emplace_back(event.Get<event_data::Blinked>());
            });

        world->SubscribeToEvent(
            EventType::kSkillDeploying,
            [this](const Event& event)
            {
                events_skill_deploying.emplace_back(event.Get<event_data::Skill>());
            });

        world->SubscribeToEvent(
            EventType::kOnEffectApplied,
            [this](const Event& event)
            {
                events_effect_applied.emplace_back(event.Get<event_data::Effect>());
            });
    }

    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnBattleStart;
    }

    int GetInnateTotalDurationMs() const override
    {
        return 1000;
    }

    int GetPreDeploymentDelayPercentage() const override
    {
        return 50;
    }

    std::vector<event_data::Blinked> events_blinked;
    std::vector<event_data::Skill> events_skill_deploying;
    std::vector<event_data::Effect> events_effect_applied;
};

class AbilitySystemInnateTest_WithOnBattleStart_Blink_And_NegativeState
    : public AbilitySystemInnateTest_WithOnBattleStart_Blink
{
    typedef AbilitySystemInnateTest_WithOnBattleStart_Blink Super;

protected:
    void InitInnateAbilities() override
    {
        auto& ability = data.type_data.innate_abilities.AddAbility();
        ability.activation_trigger_data.trigger_type = GetActivationTrigger();
        ability.activation_trigger_data.max_activations = GetMaxActivations();
        ability.activation_trigger_data.activation_time_limit_ms = GetActivationTimeLimitMs();
        ability.total_duration_ms = GetInnateTotalDurationMs();

        // Skills
        {
            auto& skill1 = ability.AddSkill();
            skill1.targeting.type = SkillTargetingType::kSelf;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddEffect(EffectData::CreateBlink(ReservedPositionType::kAcross, 0, 0));

            skill1.deployment.pre_deployment_delay_percentage = 20;
            skill1.percentage_of_ability_duration = 50;
        }
        {
            auto& skill2 = ability.AddSkill();
            skill2.targeting.type = SkillTargetingType::kDistanceCheck;
            skill2.targeting.lowest = true;
            skill2.targeting.group = AllegianceType::kEnemy;
            skill2.targeting.num = 1;
            skill2.deployment.type = SkillDeploymentType::kDirect;
            skill2.AddEffect(EffectData::CreateNegativeState(EffectNegativeState::kRoot, 1000));

            skill2.deployment.pre_deployment_delay_percentage = 50;
            skill2.percentage_of_ability_duration = 50;
        }
    }

    int GetInnateTotalDurationMs() const override
    {
        return 1000;
    }
};

// Variant With kOnEnergyFull
class AbilitySystemInnateTest_WithOnEnergyFull : public AbilitySystemInnateTest
{
    typedef AbilitySystemInnateTest Super;

protected:
    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnEnergyFull;
    }

    int GetMaxActivations() const override
    {
        return 1;
    }
};

// Variant With kOnReceiveXEffectPackages
class AbilitySystemInnateTest_WithOnEveryXEffectPackage : public AbilitySystemInnateTest
{
    using Super = AbilitySystemInnateTest;

protected:
    void PatchInnateAbilityData(AbilityData& ability_data) const override
    {
        AbilityActivationTriggerData& trigger_data = ability_data.activation_trigger_data;
        trigger_data.ability_types = MakeEnumSet(AbilityType::kAttack);
        trigger_data.number_of_effect_packages_received_modulo = 5;
        trigger_data.comparison_type = ComparisonType::kEqual;
        trigger_data.number_of_effect_packages_received = 3;
    }

    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnReceiveXEffectPackages;
    }

    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);

        auto no_innate_data = data;
        no_innate_data.type_data.innate_abilities.abilities.clear();
        SpawnCombatUnit(Team::kRed, {10, 20}, no_innate_data, red_entity);
    }
};

// Variant With kOnDeployXSkills
class AbilitySystemInnateTest_WithOnDeployXSkills : public AbilitySystemInnateTest
{
    using Super = AbilitySystemInnateTest;

protected:
    static constexpr int GetInnatePeriod()
    {
        return 5;
    }

    void PatchInnateAbilityData(AbilityData& ability_data) const override
    {
        AbilityActivationTriggerData& trigger_data = ability_data.activation_trigger_data;
        trigger_data.ability_types = MakeEnumSet(AbilityType::kAttack);
        trigger_data.number_of_skills_deployed = GetInnatePeriod();
    }

    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnDeployXSkills;
    }

    void InitAttackAbilities() override
    {
        Super::InitAttackAbilities();
    }

    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);

        CombatUnitData no_innate_data = data;
        no_innate_data.type_data.innate_abilities.abilities.clear();
        SpawnCombatUnit(Team::kRed, {10, 20}, no_innate_data, red_entity);
    }
};

// Variant With kOnReceiveXEffectPackages with zone ability
class AbilitySystemInnateTest_WithOnEveryXEffectPackage_Zone : public AbilitySystemInnateTest
{
    using Super = AbilitySystemInnateTest;

protected:
    void PatchInnateAbilityData(AbilityData& ability_data) const override
    {
        AbilityActivationTriggerData& trigger_data = ability_data.activation_trigger_data;
        trigger_data.ability_types = MakeEnumSet(AbilityType::kAttack);
        trigger_data.number_of_effect_packages_received_modulo = 2;
        trigger_data.comparison_type = ComparisonType::kEqual;
        trigger_data.number_of_effect_packages_received = 0;
    }

    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnReceiveXEffectPackages;
    }

    void InitAttackAbilities() override
    {
        // Add zone attack ability
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Zone Ability";

            // Activation trigger
            // need this attack only once so set big cooldown here
            ability.activation_trigger_data.activation_cooldown_ms = 20000;

            // Skills
            auto& skill = ability.AddSkill();
            skill.name = "Zone Attack";
            skill.targeting.type = SkillTargetingType::kCurrentFocus;
            skill.deployment.type = SkillDeploymentType::kZone;
            skill.effect_package.attributes.always_crit = false;

            // Set the zone data
            skill.zone.shape = ZoneEffectShape::kHexagon;
            skill.zone.radius_units = 2;
            skill.zone.max_radius_units = 20;
            skill.zone.width_units = 4;
            skill.zone.height_units = 4;
            skill.zone.duration_ms = 600;
            skill.zone.frequency_ms = 100;

            skill.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(10_fp));
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);

        // Red should not have innate
        auto red_data = data;
        red_data.type_data.innate_abilities.abilities.clear();
        SpawnCombatUnit(Team::kRed, {10, 20}, red_data, red_entity);
    }
};

// Variant With kOnReceiveXEffectPackages with damage over time attack ability
class AbilitySystemInnateTest_WithOnEveryXEffectPackage_Dot : public AbilitySystemInnateTest
{
    using Super = AbilitySystemInnateTest;

protected:
    void PatchInnateAbilityData(AbilityData& ability_data) const override
    {
        AbilityActivationTriggerData& trigger_data = ability_data.activation_trigger_data;
        trigger_data.ability_types = MakeEnumSet(AbilityType::kAttack);
        trigger_data.number_of_effect_packages_received_modulo = 2;
        trigger_data.comparison_type = ComparisonType::kEqual;
        trigger_data.number_of_effect_packages_received = 0;
    }

    ActivationTriggerType GetActivationTrigger() const override
    {
        return ActivationTriggerType::kOnReceiveXEffectPackages;
    }

    void InitAttackAbilities() override
    {
        // Add zone attack ability
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Zone Ability";

            // Activation trigger
            // need this attack only once so set big cooldown here
            ability.activation_trigger_data.activation_cooldown_ms = 20000;

            // Skills
            auto& skill = ability.AddSkill();
            skill.name = "Zone Attack";
            skill.targeting.type = SkillTargetingType::kCurrentFocus;
            skill.deployment.type = SkillDeploymentType::kDirect;
            skill.effect_package.attributes.always_crit = false;

            const int dot_effect_duration = 500;
            const int dot_effect_period = 200;
            auto dot_effect = EffectData::CreateDamageOverTime(
                EffectDamageType::kPhysical,
                EffectExpression::FromValue(5_fp),
                dot_effect_duration,
                dot_effect_period);
            skill.AddEffect(dot_effect);
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 10}, data, blue_entity);

        // Red should not have innate
        auto red_data = data;
        red_data.type_data.innate_abilities.abilities.clear();
        SpawnCombatUnit(Team::kRed, {10, 20}, red_data, red_entity);
    }
};

}  // namespace simulation
