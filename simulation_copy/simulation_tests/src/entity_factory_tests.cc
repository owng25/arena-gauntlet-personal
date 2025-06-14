#include "base_test_fixtures.h"
#include "components/abilities_component.h"
#include "components/attached_effects_component.h"
#include "components/combat_synergy_component.h"
#include "components/decision_component.h"
#include "components/drone_augment_component.h"
#include "components/duration_component.h"
#include "components/focus_component.h"
#include "components/mark_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/projectile_component.h"
#include "components/shield_component.h"
#include "components/stats_component.h"
#include "factories/entity_factory.h"
#include "gtest/gtest.h"
#include "test_data_loader.h"

namespace simulation
{
class EntityFactoryTest : public BaseTest
{
};

class DroneAugmentTest : public EntityFactoryTest
{
    void FillDroneAugmentsData(GameDataContainer& game_data) const override
    {
        auto drone_augment_data = DroneAugmentData::Create();
        drone_augment_data->drone_augment_type = DroneAugmentType::kSimulation;
        drone_augment_data->type_id.name = "TestName";
        drone_augment_data->type_id.stage = 1;

        game_data.AddDroneAugmentData(drone_augment_data->type_id, drone_augment_data);
    }
};

TEST_F(EntityFactoryTest, SpawnCombatUnitEmpty)
{
    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    ASSERT_TRUE(entity->Has<PositionComponent>());
    ASSERT_TRUE(entity->Has<MovementComponent>());
    ASSERT_TRUE(entity->Has<FocusComponent>());
    ASSERT_TRUE(entity->Has<StatsComponent>());
    ASSERT_TRUE(entity->Has<AbilitiesComponent>());
    ASSERT_TRUE(entity->Has<DecisionComponent>());
    ASSERT_TRUE(entity->Has<CombatSynergyComponent>());
    ASSERT_TRUE(entity->Has<AttachedEffectsComponent>());
    ASSERT_FALSE(entity->Has<ProjectileComponent>());

    // Get the components
    auto& position_component = entity->Get<PositionComponent>();
    auto& movement_component = entity->Get<MovementComponent>();
    auto& focus_component = entity->Get<FocusComponent>();
    auto& stats_component = entity->Get<StatsComponent>();
    auto& combat_synergy_component = entity->Get<CombatSynergyComponent>();
    auto& abilities_component = entity->Get<AbilitiesComponent>();

    // Ensure stats are empty
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyResist), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kPhysicalResist), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackSpeed), kDefaultAttackSpeed);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kOmegaPowerPercentage), kDefaultOmegaPowerPercentage);
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kCritAmplificationPercentage),
        kDefaultCritAmplificationPercentage);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kCritChancePercentage), kDefaultCritChancePercentage);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackDodgeChancePercentage), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kHitChancePercentage), kDefaultHitChancePercentage);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackRangeUnits), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kResolve), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kVulnerabilityPercentage), 100_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kGrit), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kWillpowerPercentage), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kTenacityPercentage), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kStartingEnergy), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyCost), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kMaxHealth), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kStartingShield), 0_fp);

    // Check combat class, combat affinity and size
    EXPECT_EQ(position_component.GetRadius(), 0);
    EXPECT_EQ(combat_synergy_component.GetCombatClass(), CombatClass::kFighter);
    EXPECT_EQ(combat_synergy_component.GetCombatAffinity(), CombatAffinity::kWater);

    EXPECT_EQ(focus_component.GetSelectionType(), FocusComponent::SelectionType::kClosestEnemy);
    EXPECT_EQ(focus_component.GetRefocusType(), FocusComponent::RefocusType::kAlways);
    EXPECT_EQ(movement_component.GetMovementType(), MovementComponent::MovementType::kNavigation);

    EXPECT_EQ(abilities_component.GetDataAttackAbilities().abilities.size(), 0);
    EXPECT_EQ(abilities_component.GetDataOmegaAbilities().abilities.size(), 0);
}

TEST_F(EntityFactoryTest, SpawnProjectileEmpty)
{
    // Set up a dummy CombatUnit type
    CombatUnitData data_combat_unit = CreateCombatUnitData();
    Entity* combat_unit_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data_combat_unit, combat_unit_entity);

    // Set up a dummy CombatUnit type
    ProjectileData data;
    data.sender_id = combat_unit_entity->GetID();

    // Spawn the CombatUnit
    Entity* entity = EntityFactory::SpawnProjectile(*world, Team::kBlue, {10, 20}, {20, 20}, data);

    ASSERT_TRUE(entity->Has<PositionComponent>());
    ASSERT_TRUE(entity->Has<MovementComponent>());
    ASSERT_TRUE(entity->Has<FocusComponent>());
    ASSERT_TRUE(entity->Has<StatsComponent>());
    ASSERT_TRUE(entity->Has<AbilitiesComponent>());
    ASSERT_TRUE(entity->Has<ProjectileComponent>());
    ASSERT_TRUE(entity->Has<DecisionComponent>());
    ASSERT_FALSE(entity->Has<CombatSynergyComponent>());
    ASSERT_FALSE(entity->Has<AttachedEffectsComponent>());

    // Get the components
    auto& position_component = entity->Get<PositionComponent>();
    auto& movement_component = entity->Get<MovementComponent>();
    auto& focus_component = entity->Get<FocusComponent>();
    auto& stats_component = entity->Get<StatsComponent>();
    auto& abilities_component = entity->Get<AbilitiesComponent>();

    // Ensure stats to be in the right state
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyResist), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyPiercingPercentage), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kPhysicalResist), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackSpeed), kDefaultAttackSpeed);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kOmegaPowerPercentage), kDefaultOmegaPowerPercentage);
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kCritAmplificationPercentage),
        kDefaultCritAmplificationPercentage);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kCritChancePercentage), kDefaultCritChancePercentage);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackDodgeChancePercentage), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kHitChancePercentage), kMaxPercentageFP);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackRangeUnits), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kResolve), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kGrit), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kWillpowerPercentage), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kTenacityPercentage), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kStartingEnergy), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyCost), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kMaxHealth), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kStartingShield), 0_fp);

    EXPECT_EQ(position_component.GetRadius(), 0);
    EXPECT_FALSE(position_component.IsTakingSpace());
    EXPECT_EQ(focus_component.GetSelectionType(), FocusComponent::SelectionType::kPredefined);
    EXPECT_EQ(focus_component.GetRefocusType(), FocusComponent::RefocusType::kNever);
    EXPECT_EQ(movement_component.GetMovementType(), MovementComponent::MovementType::kDirectPosition);

    EXPECT_EQ(abilities_component.GetDataAttackAbilities().abilities.size(), 1);
    EXPECT_TRUE(abilities_component.HasAnyAbility(AbilityType::kAttack));
    EXPECT_EQ(abilities_component.GetDataOmegaAbilities().abilities.size(), 0);
}

TEST_F(EntityFactoryTest, SpawnCombatUnitAttack)
{
    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kEnergyResist, 1_fp);
    data.type_data.stats.Set(StatType::kEnergyPiercingPercentage, 0_fp);
    data.type_data.stats.Set(StatType::kPhysicalResist, 2_fp);
    data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 300_fp);
    data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 5_fp);
    data.type_data.stats.Set(StatType::kCritChancePercentage, 6_fp);
    data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 7_fp);
    data.type_data.stats.Set(StatType::kHitChancePercentage, 8_fp);
    data.type_data.stats.Set(StatType::kResolve, 9_fp);
    data.type_data.stats.Set(StatType::kGrit, 10_fp);
    data.type_data.stats.Set(StatType::kWillpowerPercentage, 11_fp);
    data.type_data.stats.Set(StatType::kTenacityPercentage, 12_fp);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 14_fp);
    data.type_data.stats.Set(StatType::kStartingEnergy, 16_fp);
    data.type_data.stats.Set(StatType::kEnergyCost, 18_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 19_fp);
    data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);
    data.type_data.stats.Set(StatType::kStartingShield, 10_fp);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Get the components
    const auto& position_component = entity->Get<PositionComponent>();
    const auto& movement_component = entity->Get<MovementComponent>();
    const auto& stats_component = entity->Get<StatsComponent>();
    const auto& combat_synergy_component = entity->Get<CombatSynergyComponent>();

    // Test values
    // Ensure position correct
    EXPECT_EQ(position_component.GetQ(), 10);
    EXPECT_EQ(position_component.GetR(), 20);

    // Ensure movement attributes correct
    EXPECT_EQ(movement_component.GetMovementSpeedSubUnits(), data.type_data.stats.Get(StatType::kMoveSpeedSubUnits));
    EXPECT_EQ(movement_component.GetMovementType(), MovementComponent::MovementType::kNavigation);

    // Ensure stats correct
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kEnergyResist),
        data.type_data.stats.Get(StatType::kEnergyResist));
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kEnergyPiercingPercentage),
        data.type_data.stats.Get(StatType::kEnergyPiercingPercentage));
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kPhysicalResist),
        data.type_data.stats.Get(StatType::kPhysicalResist));
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kOmegaPowerPercentage),
        data.type_data.stats.Get(StatType::kOmegaPowerPercentage));
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kCritAmplificationPercentage),
        data.type_data.stats.Get(StatType::kCritAmplificationPercentage));
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kCritChancePercentage),
        data.type_data.stats.Get(StatType::kCritChancePercentage));
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kAttackDodgeChancePercentage),
        data.type_data.stats.Get(StatType::kAttackDodgeChancePercentage));
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kHitChancePercentage),
        data.type_data.stats.Get(StatType::kHitChancePercentage));
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kAttackRangeUnits),
        data.type_data.stats.Get(StatType::kAttackRangeUnits));
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kResolve), data.type_data.stats.Get(StatType::kResolve));
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kVulnerabilityPercentage),
        data.type_data.stats.Get(StatType::kVulnerabilityPercentage));
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kGrit), data.type_data.stats.Get(StatType::kGrit));
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kWillpowerPercentage),
        data.type_data.stats.Get(StatType::kWillpowerPercentage));
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kTenacityPercentage),
        data.type_data.stats.Get(StatType::kTenacityPercentage));
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kStartingEnergy),
        data.type_data.stats.Get(StatType::kStartingEnergy));
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kEnergyCost),
        data.type_data.stats.Get(StatType::kEnergyCost));
    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kMaxHealth),
        data.type_data.stats.Get(StatType::kMaxHealth));

    EXPECT_EQ(
        stats_component.GetBaseValueForType(StatType::kStartingShield),
        data.type_data.stats.Get(StatType::kStartingShield));

    // Ensure current values correct
    EXPECT_EQ(stats_component.GetCurrentEnergy(), 16_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 19_fp);

    // Check combat class, combat affinity and size
    EXPECT_EQ(position_component.GetRadius(), data.radius_units);
    EXPECT_TRUE(position_component.IsTakingSpace());
    EXPECT_EQ(combat_synergy_component.GetCombatClass(), data.type_data.combat_class);
    EXPECT_EQ(combat_synergy_component.GetCombatAffinity(), data.type_data.combat_affinity);
}

TEST_F(EntityFactoryTest, SpawnCombatUnitAbilities)
{
    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();

    // Add abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();
        ability.name = "Foo";
        ability.total_duration_ms = 1200;

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.name = "Attack";
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.deployment.pre_deployment_delay_percentage = 25;
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(80_fp));
    }
    {
        auto& ability = data.type_data.omega_abilities.AddAbility();
        ability.name = "Foo";
        ability.total_duration_ms = 3600;

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.name = "Omega Attack Chain";
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.deployment.type = SkillDeploymentType::kBeam;
        skill1.deployment.pre_deployment_delay_percentage = 10;
        skill1.percentage_of_ability_duration = 60;
        skill1.AddEffect(
            EffectData::CreateBuff(StatType::kMaxHealth, EffectExpression::FromValue(120_fp), kTimeInfinite));

        auto& skill2 = ability.AddSkill();
        skill2.name = "Omega Attack Damage";
        skill2.targeting.type = SkillTargetingType::kDistanceCheck;
        skill2.deployment.type = SkillDeploymentType::kDirect;
        skill2.deployment.pre_deployment_delay_percentage = 10;
        skill1.percentage_of_ability_duration = 40;
        skill2.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(500_fp));
    }
    data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Get the components
    auto& abilities_component = entity->Get<AbilitiesComponent>();

    // Check
    auto& attack_abilities = abilities_component.GetDataAttackAbilities();
    auto& omega_abilities = abilities_component.GetDataOmegaAbilities();

    // Attack
    ASSERT_EQ(data.type_data.attack_abilities.selection_type, attack_abilities.selection_type);
    for (size_t ability_index = 0; ability_index < attack_abilities.abilities.size(); ability_index++)
    {
        const auto& entity_ability = attack_abilities.abilities[ability_index];
        const auto& data_ability = data.type_data.attack_abilities.abilities[ability_index];

        ASSERT_EQ(data_ability->name, entity_ability->name) << "data does not match | Attack Index = " << ability_index;
        ASSERT_EQ(data_ability->total_duration_ms, entity_ability->total_duration_ms)
            << "data does not match | Attack Index = " << ability_index;

        // Skills
        ASSERT_EQ(data_ability->skills.size(), entity_ability->skills.size())
            << "Skills size does not match | Attack Index = " << ability_index;
        for (size_t skill_index = 0; skill_index < entity_ability->skills.size(); skill_index++)
        {
            const auto& entity_skill = entity_ability->skills.at(skill_index);
            const auto& data_skill = data_ability->skills.at(skill_index);

            ASSERT_EQ(data_skill->name, entity_skill->name)
                << "data does not match | Attack Index = " << ability_index << " | Skill Index = " << skill_index;
            ASSERT_EQ(data_skill->targeting.type, entity_skill->targeting.type)
                << "data does not match | Attack Index = " << ability_index << " | Skill Index = " << skill_index;
            ASSERT_EQ(data_skill->deployment.type, entity_skill->deployment.type)
                << "data does not match | Attack Index = " << ability_index << " | Skill Index = " << skill_index;
            ASSERT_EQ(data_skill->percentage_of_ability_duration, entity_skill->percentage_of_ability_duration)
                << "data does not match | Attack Index = " << ability_index << " | Skill Index = " << skill_index;
            ASSERT_EQ(
                data_skill->deployment.pre_deployment_delay_percentage,
                entity_skill->deployment.pre_deployment_delay_percentage)
                << "data does not match | Attack Index = " << ability_index << " | Skill Index = " << skill_index;

            // Effects
            ASSERT_EQ(data_skill->effect_package.effects.size(), entity_skill->effect_package.effects.size())
                << "Effect size do not match | Attack Index = " << ability_index << " | Skill Index = " << skill_index;
            for (size_t effect_index = 0; effect_index < entity_skill->effect_package.effects.size(); effect_index++)
            {
                const auto& entity_effect = entity_skill->effect_package.effects.at(effect_index);
                const auto& data_effect = data_skill->effect_package.effects.at(effect_index);

                ASSERT_EQ(data_effect.type_id.type, entity_effect.type_id.type)
                    << "Effect types do not match | Attack Index = " << ability_index
                    << " | Skill Index = " << skill_index << " | Effect Index = " << effect_index;

                ASSERT_EQ(data_effect.type_id.stat_type, entity_effect.type_id.stat_type)
                    << "Effect stat do not match | Attack Index = " << ability_index
                    << " | Skill Index = " << skill_index << " | Effect Index = " << effect_index;

                ExpressionStatsSource stats_source;
                stats_source.Set(ExpressionDataSourceType::kSender, FullStatsData{}, nullptr);
                ASSERT_EQ(
                    data_effect.GetExpression().Evaluate(ExpressionEvaluationContext{}, stats_source),
                    entity_effect.GetExpression().Evaluate(ExpressionEvaluationContext{}, stats_source))
                    << "Effect value do not match | Attack Index = " << ability_index
                    << " | Skill Index = " << skill_index << " | Effect Index = " << effect_index;
            }
        }
    }

    // Omega ability
    ASSERT_EQ(data.type_data.omega_abilities.selection_type, omega_abilities.selection_type);
    for (size_t ability_index = 0; ability_index < omega_abilities.abilities.size(); ability_index++)
    {
        const auto& entity_ability = omega_abilities.abilities[ability_index];
        const auto& data_ability = data.type_data.omega_abilities.abilities[ability_index];

        ASSERT_EQ(data_ability->name, entity_ability->name)
            << "data does not match | Omega Ability Index = " << ability_index;
        ASSERT_EQ(data_ability->total_duration_ms, entity_ability->total_duration_ms)
            << "data does not match | Omega Ability Index = " << ability_index;

        // Skills
        ASSERT_EQ(data_ability->skills.size(), entity_ability->skills.size())
            << "Skills size does not match | Omega Ability Index = " << ability_index;
        for (size_t skill_index = 0; skill_index < entity_ability->skills.size(); skill_index++)
        {
            const auto& entity_skill = entity_ability->skills[skill_index];
            const auto& data_skill = data_ability->skills[skill_index];

            ASSERT_EQ(data_skill->name, entity_skill->name)
                << "data does not match | Omega Ability Index = " << ability_index
                << " | Skill Index = " << skill_index;
            ASSERT_EQ(data_skill->targeting.type, entity_skill->targeting.type)
                << "data does not match | Omega Ability Index = " << ability_index
                << " | Skill Index = " << skill_index;
            ASSERT_EQ(data_skill->deployment.type, entity_skill->deployment.type)
                << "data does not match | Omega Ability Index = " << ability_index
                << " | Skill Index = " << skill_index;
            ASSERT_EQ(data_skill->percentage_of_ability_duration, entity_skill->percentage_of_ability_duration)
                << "data does not match | Attack Index = " << ability_index << " | Skill Index = " << skill_index;
            ASSERT_EQ(
                data_skill->deployment.pre_deployment_delay_percentage,
                entity_skill->deployment.pre_deployment_delay_percentage)
                << "data does not match | Attack Index = " << ability_index << " | Skill Index = " << skill_index;

            // Effects
            ASSERT_EQ(data_skill->effect_package.effects.size(), entity_skill->effect_package.effects.size())
                << "Effect size do not match | Attack Index = " << ability_index << " | Skill Index = " << skill_index;
            for (size_t effect_index = 0; effect_index < entity_skill->effect_package.effects.size(); effect_index++)
            {
                const auto& entity_effect = entity_skill->effect_package.effects.at(effect_index);
                const auto& data_effect = data_skill->effect_package.effects.at(effect_index);

                ASSERT_EQ(data_effect.type_id.type, entity_effect.type_id.type)
                    << "Effect types do not match | Attack Index = " << ability_index
                    << " | Skill Index = " << skill_index << " | Effect Index = " << effect_index;

                ASSERT_EQ(data_effect.type_id.stat_type, entity_effect.type_id.stat_type)
                    << "Effect stat do not match | Attack Index = " << ability_index
                    << " | Skill Index = " << skill_index << " | Effect Index = " << effect_index;

                ExpressionStatsSource stats_source;
                stats_source.Set(ExpressionDataSourceType::kSender, FullStatsData{}, nullptr);
                ASSERT_EQ(
                    data_effect.GetExpression().Evaluate(ExpressionEvaluationContext{}, stats_source),
                    entity_effect.GetExpression().Evaluate(ExpressionEvaluationContext{}, stats_source))
                    << "Effect value do not match | Attack Index = " << ability_index
                    << " | Skill Index = " << skill_index << " | Effect Index = " << effect_index;
            }
        }
    }
}

TEST_F(EntityFactoryTest, SpawnCombatUnitAttackAbilityOutOfScope)
{
    Entity* entity_ptr = nullptr;

    // Intentionally go out of scope
    {
        // Set up a dummy CombatUnit type
        CombatUnitData data = CreateCombatUnitData();

        // Add abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Foo";
            ability.total_duration_ms = 1000;

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Attack";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(72_fp));
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

        // Spawn the CombatUnit
        SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity_ptr);
    }  // intentionally go out of scope with the data
    auto& entity = *entity_ptr;

    // Get the components
    auto& abilities_component = entity.Get<AbilitiesComponent>();

    // Check
    auto& attack_abilities = abilities_component.GetDataAttackAbilities();

    // Attack
    ASSERT_EQ(AbilitySelectionType::kCycle, attack_abilities.selection_type);

    // Ability1
    {
        const size_t ability_index = 0;
        const auto& ability = *attack_abilities.abilities[0];

        ASSERT_EQ("Foo", ability.name) << "data does not match | Attack Index = " << ability_index;
        ASSERT_EQ(1000, ability.total_duration_ms) << "data does not match | Attack Index = " << ability_index;

        // Skills
        const size_t skill_index = 0;
        const auto& skill1 = *ability.skills[skill_index];

        ASSERT_EQ("Attack", skill1.name) << "data does not match | Attack Index = " << ability_index
                                         << " | Skill Index = " << skill_index;
        ASSERT_EQ(SkillTargetingType::kCurrentFocus, skill1.targeting.type)
            << "data does not match | Attack Index = " << ability_index << " | Skill Index = " << skill_index;
        ASSERT_EQ(SkillDeploymentType::kDirect, skill1.deployment.type)
            << "data does not match | Attack Index = " << ability_index << " | Skill Index = " << skill_index;
        ASSERT_EQ(EffectDamageType::kPhysical, skill1.effect_package.effects[0].type_id.damage_type)
            << "data does not match | Attack Index = " << ability_index << " | Skill Index = " << skill_index;

        ExpressionStatsSource stats_source;
        stats_source.Set(ExpressionDataSourceType::kSender, FullStatsData{}, nullptr);
        ASSERT_EQ(
            72_fp,
            skill1.effect_package.effects[0].GetExpression().Evaluate(ExpressionEvaluationContext{}, stats_source))
            << "Value does not match"
            << " | Attack Index = " << ability_index << " | Skill Index = " << skill_index;
    }
}

TEST_F(EntityFactoryTest, SpawnShieldEmpty)
{
    // Set up a dummy CombatUnit type
    CombatUnitData data_combat_unit = CreateCombatUnitData();
    Entity* combat_unit_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data_combat_unit, combat_unit_entity);

    // Set up a dummy shield type
    ShieldData data;
    data.sender_id = combat_unit_entity->GetID();
    data.receiver_id = combat_unit_entity->GetID();

    // Spawn the shield
    Entity* entity = EntityFactory::SpawnShieldAndAttach(*world, Team::kBlue, data);

    ASSERT_FALSE(entity->Has<PositionComponent>());
    ASSERT_FALSE(entity->Has<MovementComponent>());
    ASSERT_FALSE(entity->Has<StatsComponent>());
    ASSERT_FALSE(entity->Has<ProjectileComponent>());
    ASSERT_FALSE(entity->Has<CombatSynergyComponent>());
    ASSERT_TRUE(entity->Has<DecisionComponent>());
    ASSERT_TRUE(entity->Has<AbilitiesComponent>());
    ASSERT_TRUE(entity->Has<ShieldComponent>());
    ASSERT_TRUE(entity->Has<FocusComponent>());
    ASSERT_TRUE(entity->Has<AttachedEffectsComponent>());

    // Get the components
    const auto& focus_component = entity->Get<FocusComponent>();
    const auto& shield_component = entity->Get<ShieldComponent>();
    const auto& abilities_component = entity->Get<AbilitiesComponent>();
    const auto& duration_component = entity->Get<DurationComponent>();
    auto& attached_effects_component = entity->Get<AttachedEffectsComponent>();

    // Ensure stats to be in the right state
    EXPECT_EQ(focus_component.GetSelectionType(), FocusComponent::SelectionType::kClosestEnemy);
    EXPECT_EQ(focus_component.GetRefocusType(), FocusComponent::RefocusType::kAlways);

    EXPECT_EQ(shield_component.GetValue(), 0_fp);
    EXPECT_EQ(duration_component.GetDurationMs(), kTimeInfinite);

    EXPECT_EQ(abilities_component.GetDataAttackAbilities().abilities.size(), 0);
    EXPECT_FALSE(abilities_component.HasAnyAbility(AbilityType::kAttack));
    EXPECT_EQ(abilities_component.GetDataOmegaAbilities().abilities.size(), 0);

    EXPECT_EQ(attached_effects_component.GetAttachedEffects().size(), 0);
}

TEST_F(EntityFactoryTest, SpawnMarkEmpty)
{
    // Set up a dummy CombatUnit type
    const CombatUnitData data_combat_unit = CreateCombatUnitData();
    Entity* combat_unit_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 10}, data_combat_unit, combat_unit_entity);

    // Set up a dummy mark type
    MarkData data;
    data.sender_id = combat_unit_entity->GetID();
    data.receiver_id = combat_unit_entity->GetID();
    data.abilities_data.push_back(AbilityData::Create());

    // Spawn the mark
    const Entity* entity = EntityFactory::SpawnMarkAndAttach(*world, Team::kBlue, data);

    ASSERT_TRUE(entity->Has<MarkComponent>());
    ASSERT_TRUE(entity->Has<AttachedEffectsComponent>());
    ASSERT_TRUE(entity->Has<AbilitiesComponent>());
    ASSERT_TRUE(entity->Has<DurationComponent>());
}

TEST_F(EntityFactoryTest, RemoveCombatUnitBeforeBattleStarted)
{
    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kEnergyResist, 1_fp);
    data.type_data.stats.Set(StatType::kEnergyPiercingPercentage, 0_fp);
    data.type_data.stats.Set(StatType::kPhysicalResist, 2_fp);
    data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 300_fp);
    data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 5_fp);
    data.type_data.stats.Set(StatType::kCritChancePercentage, 6_fp);
    data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 7_fp);
    data.type_data.stats.Set(StatType::kHitChancePercentage, 8_fp);
    data.type_data.stats.Set(StatType::kResolve, 9_fp);
    data.type_data.stats.Set(StatType::kGrit, 10_fp);
    data.type_data.stats.Set(StatType::kWillpowerPercentage, 11_fp);
    data.type_data.stats.Set(StatType::kTenacityPercentage, 12_fp);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 14_fp);
    data.type_data.stats.Set(StatType::kStartingEnergy, 16_fp);
    data.type_data.stats.Set(StatType::kEnergyCost, 18_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 19_fp);
    data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);
    data.type_data.stats.Set(StatType::kStartingShield, 10_fp);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);
    EXPECT_EQ(world->GetAll().size(), 1);
    EXPECT_EQ(world->GetTeams().size(), 1);

    world->RemoveCombatUnitBeforeBattleStarted(entity->GetID());

    ASSERT_EQ(world->GetTimeStepCount(), 0);
    EXPECT_EQ(world->GetAll().size(), 0);
    EXPECT_EQ(world->GetTeams().size(), 0);
    EXPECT_EQ(world->GetLastAddedEntityID(), kInvalidEntityID);
}

TEST_F(EntityFactoryTest, RemoveTwoCombatUnitsInARow)
{
    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;

    CombatUnitData data2 = CreateCombatUnitData();
    data2.radius_units = 1;
    data2.type_data.combat_affinity = CombatAffinity::kEarth;
    data2.type_data.combat_class = CombatClass::kEnchanter;
    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);
    Entity* entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 40}, data2, entity2);

    EXPECT_EQ(world->GetAll().size(), 2);
    EXPECT_EQ(world->GetTeams().size(), 2);

    world->RemoveCombatUnitBeforeBattleStarted(entity->GetID());

    ASSERT_EQ(world->GetTimeStepCount(), 0);
    EXPECT_EQ(world->GetAll().size(), 1);
    EXPECT_EQ(world->GetTeams().size(), 1);

    world->RemoveCombatUnitBeforeBattleStarted(entity2->GetID());

    ASSERT_EQ(world->GetTimeStepCount(), 0);
    EXPECT_EQ(world->GetAll().size(), 0);
    EXPECT_EQ(world->GetTeams().size(), 0);
    EXPECT_EQ(world->GetLastAddedEntityID(), kInvalidEntityID);
}

TEST_F(EntityFactoryTest, RemoveCombatUnitBeforeBattleStartedsSameOwner)
{
    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;

    CombatUnitData data2 = CreateCombatUnitData();
    data2.radius_units = 1;
    data2.type_data.combat_affinity = CombatAffinity::kEarth;
    data2.type_data.combat_class = CombatClass::kEnchanter;
    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);
    Entity* entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {20, 40}, data2, entity2);

    EXPECT_EQ(world->GetAll().size(), 2);
    EXPECT_EQ(world->GetTeams().size(), 1);

    world->RemoveCombatUnitBeforeBattleStarted(entity->GetID());

    ASSERT_EQ(world->GetTimeStepCount(), 0);
    EXPECT_EQ(world->GetAll().size(), 1);
    // expect owner to have one more combat unit, so it shouldn't be deleted
    EXPECT_EQ(world->GetTeams().size(), 1);

    world->RemoveCombatUnitBeforeBattleStarted(entity2->GetID());

    ASSERT_EQ(world->GetTimeStepCount(), 0);
    EXPECT_EQ(world->GetAll().size(), 0);
    EXPECT_EQ(world->GetTeams().size(), 0);
    EXPECT_EQ(world->GetLastAddedEntityID(), kInvalidEntityID);
}

TEST_F(EntityFactoryTest, FindRayRectIntersection)
{
    constexpr IVector2D zero{0, 0};
    constexpr IVector2D square_min{-100, -100};
    constexpr IVector2D square_max{100, 100};
    constexpr IVector2D x_wide_min{-200, -100};
    constexpr IVector2D x_wide_max{200, 100};
    constexpr IVector2D y_wide_min{-100, -200};
    constexpr IVector2D y_wide_max{100, 200};
    constexpr IVector2D right{1, 0};
    constexpr IVector2D left{-1, 0};
    constexpr IVector2D top{0, 1};
    constexpr IVector2D bot{0, -1};

    // Top
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, top, square_min, square_max), IVector2D(0, 100));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, top, x_wide_min, x_wide_max), IVector2D(0, 100));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, top, y_wide_min, y_wide_max), IVector2D(0, 200));

    // Top-Right
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, top + right, square_min, square_max), IVector2D(100, 100));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, top + right, x_wide_min, x_wide_max), IVector2D(100, 100));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, top + right, y_wide_min, y_wide_max), IVector2D(100, 100));

    // Right
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, right, square_min, square_max), IVector2D(100, 0));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, right, x_wide_min, x_wide_max), IVector2D(200, 0));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, right, y_wide_min, y_wide_max), IVector2D(100, 0));

    // Bottom-Right
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, bot + right, square_min, square_max), IVector2D(100, -100));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, bot + right, x_wide_min, x_wide_max), IVector2D(100, -100));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, bot + right, y_wide_min, y_wide_max), IVector2D(100, -100));

    // Bottom
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, bot, square_min, square_max), IVector2D(0, -100));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, bot, x_wide_min, x_wide_max), IVector2D(0, -100));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, bot, y_wide_min, y_wide_max), IVector2D(0, -200));

    // Bottom-Left
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, bot + left, square_min, square_max), IVector2D(-100, -100));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, bot + left, x_wide_min, x_wide_max), IVector2D(-100, -100));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, bot + left, y_wide_min, y_wide_max), IVector2D(-100, -100));

    // Left
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, left, square_min, square_max), IVector2D(-100, 0));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, left, x_wide_min, x_wide_max), IVector2D(-200, 0));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, left, y_wide_min, y_wide_max), IVector2D(-100, 0));

    // Top-Left
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, top + left, square_min, square_max), IVector2D(-100, 100));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, top + left, x_wide_min, x_wide_max), IVector2D(-100, 100));
    EXPECT_EQ(EntityFactory::FindRayRectIntersection(zero, top + left, y_wide_min, y_wide_max), IVector2D(-100, 100));
}

TEST_F(EntityFactoryTest, RemoveAddMoreCombatUnits)
{
    // Set up a dummy CombatUnit type
    CombatUnitData data = CreateCombatUnitData();
    data.radius_units = 1;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;

    CombatUnitData data2 = CreateCombatUnitData();
    data2.radius_units = 1;
    data2.type_data.combat_affinity = CombatAffinity::kEarth;
    data2.type_data.combat_class = CombatClass::kEnchanter;
    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);
    Entity* entity2 = nullptr;
    SpawnCombatUnit(Team::kRed, {20, 40}, data2, entity2);

    EXPECT_EQ(world->GetAll().size(), 2);
    EXPECT_EQ(world->GetTeams().size(), 2);

    world->RemoveCombatUnitBeforeBattleStarted(entity->GetID());

    ASSERT_EQ(world->GetTimeStepCount(), 0);
    EXPECT_EQ(world->GetAll().size(), 1);
    EXPECT_EQ(world->GetTeams().size(), 1);

    // add another combat unit
    CombatUnitData data3 = CreateCombatUnitData();
    data3.radius_units = 1;
    data3.type_data.combat_affinity = CombatAffinity::kBloom;
    data3.type_data.combat_class = CombatClass::kFighter;

    Entity* entity3 = nullptr;
    SpawnCombatUnit(Team::kRed, {10, 20}, data3, entity3);

    ASSERT_EQ(world->GetTimeStepCount(), 0);
    EXPECT_EQ(world->GetAll().size(), 2);
    EXPECT_EQ(world->GetTeams().size(), 1);

    world->RemoveCombatUnitBeforeBattleStarted(entity3->GetID());

    ASSERT_EQ(world->GetTimeStepCount(), 0);
    EXPECT_EQ(world->GetAll().size(), 1);
    EXPECT_EQ(world->GetTeams().size(), 1);

    // add another combat unit
    CombatUnitData data4 = CreateCombatUnitData();
    data4.radius_units = 1;
    data4.type_data.combat_affinity = CombatAffinity::kDust;
    data4.type_data.combat_class = CombatClass::kBulwark;

    Entity* entity4 = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data4, entity4);

    ASSERT_EQ(world->GetTimeStepCount(), 0);
    EXPECT_EQ(world->GetAll().size(), 2);
    EXPECT_EQ(world->GetTeams().size(), 2);

    world->RemoveCombatUnitBeforeBattleStarted(entity2->GetID());

    ASSERT_EQ(world->GetTimeStepCount(), 0);
    EXPECT_EQ(world->GetAll().size(), 1);
    EXPECT_EQ(world->GetTeams().size(), 1);

    world->RemoveCombatUnitBeforeBattleStarted(entity4->GetID());

    ASSERT_EQ(world->GetTimeStepCount(), 0);
    EXPECT_EQ(world->GetAll().size(), 0);
    EXPECT_EQ(world->GetTeams().size(), 0);
    EXPECT_EQ(world->GetLastAddedEntityID(), kInvalidEntityID);
}

TEST_F(EntityFactoryTest, SpawnDroneAugmentEmpty)
{
    // Set up a dummy CombatUnit type
    DroneAugmentData drone_augment_data;
    drone_augment_data.drone_augment_type = DroneAugmentType::kSimulation;
    drone_augment_data.type_id.name = "TestName";
    drone_augment_data.type_id.stage = 1;

    // Spawn the CombatUnit
    Entity* drone_augment_entity = EntityFactory::SpawnDroneAugmentEntity(*world, drone_augment_data, Team::kBlue);
    ASSERT_NE(drone_augment_entity, nullptr);

    ASSERT_TRUE(drone_augment_entity->Has<DroneAugmentEntityComponent>());
    ASSERT_TRUE(drone_augment_entity->Has<AbilitiesComponent>());

    auto& component = drone_augment_entity->Get<DroneAugmentEntityComponent>();
    ASSERT_EQ(component.GetDroneAugmentTypeID(), drone_augment_data.type_id);
}

TEST_F(DroneAugmentTest, SpawnDroneAugmentEmptyFromWorld)
{
    DroneAugmentTypeID drone_augment_type_id;
    drone_augment_type_id.name = "TestName";
    drone_augment_type_id.stage = 1;

    // Spawn the CombatUnit
    ASSERT_TRUE(world->AddDroneAugmentBeforeBattleStarted(Team::kBlue, drone_augment_type_id));

    Entity& drone_augment_entity = world->GetByID(0);

    ASSERT_TRUE(drone_augment_entity.Has<DroneAugmentEntityComponent>());
    ASSERT_TRUE(drone_augment_entity.Has<AbilitiesComponent>());

    auto& component = drone_augment_entity.Get<DroneAugmentEntityComponent>();
    ASSERT_EQ(component.GetDroneAugmentTypeID(), drone_augment_type_id);
}

class CombatUnitRangerTest : public EntityFactoryTest
{
public:
    using Super = EntityFactoryTest;

public:
    void PostInitWorld() override
    {
        Super::PostInitWorld();
        loader = std::make_unique<TestingDataLoader>(world->GetLogger());
    }

    void SetUp() override
    {
        Super::SetUp();
    }

    void FillEquipmentData(GameDataContainer& game_data_container) const override
    {
        const char* weapon_flamewardenstaff_json = R"(
{
	"Name": "FlamewardenStaff",
	"Stage": 0,
	"Tier": 0,
	"Variation": "Original",
	"CombatAffinity": "Fire",
	"CombatClass": "Empath",
	"DominantCombatAffinity": "Fire",
	"DominantCombatClass": "Empath",
	"Type": "Normal",
	"Stats": {
		"MaxHealth": 700000,
		"StartingEnergy": 50000,
		"EnergyCost": 160000,
		"PhysicalResist": 20,
		"EnergyResist": 20,
		"TenacityPercentage": 0,
		"WillpowerPercentage": 0,
		"Grit": 0,
		"Resolve": 0,
		"AttackPhysicalDamage": 80000,
		"AttackEnergyDamage": 0,
		"AttackPureDamage": 0,
		"AttackSpeed": 70,
		"MoveSpeedSubUnits": 0,
		"HitChancePercentage": 100,
		"AttackDodgeChancePercentage": 0,
		"CritChancePercentage": 25,
		"CritAmplificationPercentage": 150,
		"OmegaPowerPercentage": 100,
		"AttackRangeUnits": 50,
		"OmegaRangeUnits": 50,
		"HealthRegeneration": 0,
		"EnergyRegeneration": 0,
		"EnergyGainEfficiencyPercentage": 100,
		"OnActivationEnergy": 0,
		"VulnerabilityPercentage": 100,
		"EnergyPiercingPercentage": 0,
		"PhysicalPiercingPercentage": 0,
		"HealthGainEfficiencyPercentage": 100,
		"PhysicalVampPercentage": 0,
		"EnergyVampPercentage": 0,
		"PureVampPercentage": 0,
		"Thorns": 0,
		"StartingShield": 0,
		"CritReductionPercentage": 0
	},
	"AttackAbilitiesSelection": "Cycle",
	"AttackAbilities": [
		{
			"Name": "Staff Attack I",
			"Skills": [
				{
					"Name": "Attack",
					"Targeting": {
						"Type": "CurrentFocus",
						"Guidance": [
							"Ground",
							"Airborne"
						]
					},
					"Deployment": {
						"Type": "Projectile",
						"Guidance": [
							"Ground",
							"Airborne"
						],
						"PreDeploymentDelayPercentage": 50
					},
					"Projectile": {
						"SizeUnits": 0,
						"IsHoming": true,
						"SpeedSubUnits": 10000,
						"IsBlockable": false,
						"ApplyToAll": false
					},
					"EffectPackage": {
						"Effects": [
							{
								"Type": "InstantDamage",
								"DamageType": "Physical",
								"Expression": {
									"Stat": "AttackPhysicalDamage",
									"StatSource": "Sender"
								}
							},
							{
								"Type": "InstantDamage",
								"DamageType": "Energy",
								"Expression": {
									"Stat": "AttackEnergyDamage",
									"StatSource": "Sender"
								}
							},
							{
								"Type": "InstantDamage",
								"DamageType": "Pure",
								"Expression": {
									"Stat": "AttackPureDamage",
									"StatSource": "Sender"
								}
							}
						]
					}
				}
			]
		}
	],
	"OmegaAbilitiesSelection": "Cycle",
	"OmegaAbilities": [
		{
			"Name": "Infernal Convergence I",
			"TotalDurationMs": 2000,
			"Skills": [
				{
					"Name": "Spinning Projectile",
					"Targeting": {
						"Type": "CurrentFocus"
					},
					"Deployment": {
						"Type": "Projectile",
						"Guidance": [
							"Ground",
							"Airborne"
						],
						"PreDeploymentDelayPercentage": 45
					},
					"Projectile": {
						"SizeUnits": 3,
						"IsHoming": true,
						"SpeedSubUnits": 8000,
						"IsBlockable": false,
						"ApplyToAll": false
					},
					"PercentageOfAbilityDuration": 100,
					"EffectPackage": {
						"Attributes": {
							"Propagation": {
								"PropagationType": "Splash",
								"SplashRadiusUnits": 20,
								"DeploymentGuidance": [
									"Ground",
									"Airborne",
									"Underground"
								],
								"EffectPackage": {
									"Effects": [
										{
											"Type": "InstantDamage",
											"DamageType": "Energy",
											"Expression": {
												"Operation": "+",
												"Operands": [
													{
														"Operation": "*",
														"Operands": [
															600000,
															{
																"Stat": "OmegaPowerPercentage",
																"StatSource": "Sender"
															}
														]
													}
												]
											}
										}
									]
								}
							}
						},
						"Effects": []
					}
				}
			]
		}
	],
	"InnateAbilities": [
		{
			"Name": "VineboundCleaver_Ability1",
			"ActivationTrigger": {
				"TriggerType": "OnBattleStart"
			},
			"TotalDurationMs": 0,
			"Skills": [
				{
					"Name": "VineboundCleaver Attack Speed",
					"Deployment": {
						"Type": "Direct"
					},
					"Targeting": {
						"Type": "Self"
					},
					"EffectPackage": {
						"Attributes": {
							"Propagation": {
								"PropagationType": "Splash",
								"SplashRadiusUnits": 15,
								"DeploymentGuidance": [
									"Ground",
									"Airborne"
								],
								"EffectPackage": {
									"Effects": [
										{
                                            "ID": "IDAttributesPropagationEffectReplace",
											"Type": "InstantDamage",
											"DamageType": "Energy",
											"Expression": 7000
										}
									]
								}
							}
						},
						"Effects": [
							{
								"ID": "IDInnateSimpleEffectReplace",
								"Type": "Buff",
								"Stat": "AttackSpeed",
								"OverlapProcessType": "Stacking",
								"DurationMs": -1,
								"FrequencyMs": 500,
								"Expression": 1000
							},
							{
								"Type": "Empower",
								"ActivatedBy": "Attack",
								"IsConsumable": true,
								"ActivationsUntilExpiry": 2,
								"AttachedEffects": [
									{
								        "ID": "IDAttachedEffectInsideEmpower",
										"Type": "Debuff",
										"DurationMs": 2000,
										"Stat": "AttackPhysicalDamage",
										"OverlapProcessType": "Sum",
										"Expression": 1234
									}
								]
							},
							{
								"Type": "Empower",
								"ActivatedBy": "Attack",
								"DurationMs": 6000,
								"AttachedEffectPackageAttributes": {
									"Propagation": {
										"PropagationType": "Splash",
										"IgnoreFirstPropagationReceiver": true,
										"SplashRadiusUnits": 17,
										"EffectPackage": {
											"Effects": [
									            {
                                                    "ID": "IDAttachedEffectPackageAttributesPropagationEffectReplace",
											        "Type": "InstantDamage",
											        "DamageType": "Pure",
											        "Expression": 23500
										        }
											]
										}
									}
								}
							}
						]
					}
				}
			]
		}
	],
	"DisplayName": "Flamewarden Staff",
	"DisplayDescription": "The Ranger releases a whirling projectile from their staff, dealing Energy Damage equal to 600*OP to the target and all enemies within a 20-radius.",
	"Description": {
		"Format": "The Ranger releases a whirling projectile from their staff, dealing Energy Damage equal to 600*OP to the target and all enemies within a 20-radius.",
		"Parameters": {}
	},
	"DisplayDescriptionNormalized": "The Ranger releases a whirling projectile from their staff, dealing Energy Damage equal to 600*OP to the target and all enemies within a 20-radius."
}
)";
        AddWeaponData(game_data_container, weapon_flamewardenstaff_json);

        const char* weapon_amplifier_flamewarden_zonebreakear_json = R"(
{
	"Name": "FlamewardenStaffZonebreaker",
	"Stage": 0,
	"Tier": 0,
	"Variation": "Original",
	"Type": "Amplifier",
	"AmplifierForWeapon": {
		"Name": "FlamewardenStaff",
		"Stage": 0,
		"Variation": "Original",
		"CombatAffinity": "Fire"
	},
	"Stats": {
		"MaxHealth": 400000,
		"StartingEnergy": 0,
		"EnergyCost": 0,
		"PhysicalResist": 0,
		"EnergyResist": 0,
		"TenacityPercentage": 0,
		"WillpowerPercentage": 0,
		"Grit": 0,
		"Resolve": 0,
		"AttackPhysicalDamage": 0,
		"AttackEnergyDamage": 0,
		"AttackPureDamage": 0,
		"AttackSpeed": 0,
		"MoveSpeedSubUnits": 0,
		"HitChancePercentage": 0,
		"AttackDodgeChancePercentage": 0,
		"CritChancePercentage": 0,
		"CritAmplificationPercentage": 0,
		"OmegaPowerPercentage": 0,
		"AttackRangeUnits": 0,
		"OmegaRangeUnits": 0,
		"HealthRegeneration": 0,
		"EnergyRegeneration": 0,
		"EnergyGainEfficiencyPercentage": 0,
		"OnActivationEnergy": 0,
		"VulnerabilityPercentage": 0,
		"EnergyPiercingPercentage": 0,
		"PhysicalPiercingPercentage": 0,
		"HealthGainEfficiencyPercentage": 0,
		"PhysicalVampPercentage": 0,
		"EnergyVampPercentage": 0,
		"PureVampPercentage": 0,
		"Thorns": 0,
		"StartingShield": 0,
		"CritReductionPercentage": 0
	},
	"AttackAbilitiesSelection": "Cycle",
	"AttackAbilities": [],
	"OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
		{
			"Name": "Infernal Convergence I",
			"TotalDurationMs": 5000,
			"Skills": [
				{
					"Name": "Spinning Projectile",
					"Targeting": {
						"Type": "CurrentFocus"
					},
					"Deployment": {
						"Type": "Projectile",
						"Guidance": [
							"Ground",
							"Airborne"
						],
						"PreDeploymentDelayPercentage": 45
					},
					"Projectile": {
						"SizeUnits": 3,
						"IsHoming": true,
						"SpeedSubUnits": 8000,
						"IsBlockable": false,
						"ApplyToAll": false
					},
					"PercentageOfAbilityDuration": 100,
					"EffectPackage": {
						"Attributes": {
							"Propagation": {
								"PropagationType": "Splash",
								"SplashRadiusUnits": 20,
								"DeploymentGuidance": [
									"Ground",
									"Airborne",
									"Underground"
								],
								"EffectPackage": {
									"Effects": [
										{
											"Type": "InstantDamage",
											"DamageType": "Energy",
											"Expression": {
												"Operation": "+",
												"Operands": [
													{
														"Operation": "*",
														"Operands": [
															600000,
															{
																"Stat": "OmegaPowerPercentage",
																"StatSource": "Sender"
															}
														]
													}
												]
											}
										}
									]
								}
							}
						},
						"Effects": []
					}
				}
			]
		}
	],
	"InnateAbilities": [
		{
			"Name": "ZoneBreaker_Ability1",
			"ActivationTrigger": {
				"TriggerType": "OnBattleStart"
			},
			"TotalDurationMs": 0,
			"Skills": [
				{
					"Name": "ZoneBreaker - DoT",
					"Deployment": {
						"Type": "Direct"
					},
					"Targeting": {
						"Type": "Self"
					},
					"EffectPackage": {
						"Effects": [
							{
								"Type": "Empower",
								"IsConsumable": false,
								"DurationMs": -1,
								"ActivatedBy": "Omega",
								"AttachedEffects": [
									{
										"Type": "DOT",
										"DamageType": "Pure",
										"OverlapProcessType": "Merge",
										"DurationMs": 5000,
										"FrequencyMs": 1000,
										"Expression": {
											"Percentage": 5,
											"Stat": "MaxHealth",
											"StatSource": "Receiver"
										}
									}
								]
							}
						]
					}
				}
			]
		}
	],
	"DisplayName": "Zonebreaker",
	"DisplayDescription": "Explosion deals an additional 5% Max Health over 5 seconds.",
	"Description": {
		"Format": "Explosion deals an additional 5% Max Health over 5 seconds.",
		"Parameters": {}
	},
	"DisplayDescriptionNormalized": "Explosion deals an additional 5% Max Health over 5 seconds."
}
)";
        AddWeaponData(game_data_container, weapon_amplifier_flamewarden_zonebreakear_json);

        const char* weapon_amplifier_flamewarden_overloadimpact_json = R"(
{
	"Name": "FlamewardenStaffOverloadImpact",
	"Stage": 0,
	"Tier": 0,
	"Variation": "Original",
	"Type": "Amplifier",
	"AmplifierForWeapon": {
		"Name": "FlamewardenStaff",
		"Stage": 0,
		"Variation": "Original",
		"CombatAffinity": "Fire"
	},
	"Stats": {
		"MaxHealth": 100000,
		"StartingEnergy": 0,
		"EnergyCost": 0,
		"PhysicalResist": 0,
		"EnergyResist": 0,
		"TenacityPercentage": 0,
		"WillpowerPercentage": 0,
		"Grit": 0,
		"Resolve": 0,
		"AttackPhysicalDamage": 0,
		"AttackEnergyDamage": 0,
		"AttackPureDamage": 0,
		"AttackSpeed": 0,
		"MoveSpeedSubUnits": 0,
		"HitChancePercentage": 0,
		"AttackDodgeChancePercentage": 0,
		"CritChancePercentage": 0,
		"CritAmplificationPercentage": 0,
		"OmegaPowerPercentage": 0,
		"AttackRangeUnits": 0,
		"OmegaRangeUnits": 0,
		"HealthRegeneration": 0,
		"EnergyRegeneration": 0,
		"EnergyGainEfficiencyPercentage": 0,
		"OnActivationEnergy": 0,
		"VulnerabilityPercentage": 0,
		"EnergyPiercingPercentage": 0,
		"PhysicalPiercingPercentage": 0,
		"HealthGainEfficiencyPercentage": 0,
		"PhysicalVampPercentage": 0,
		"EnergyVampPercentage": 0,
		"PureVampPercentage": 0,
		"Thorns": 0,
		"StartingShield": 0,
		"CritReductionPercentage": 0
	},
	"AttackAbilitiesSelection": "Cycle",
	"AttackAbilities": [],
	"OmegaAbilitiesSelection": "Cycle",
	"OmegaAbilities": [],
	"InnateAbilities": [
		{
			"Name": "Executiuon Surge",
			"ActivationTrigger": {
				"TriggerType": "OnActivateXAbilities",
				"AbilityTypes": [
					"Omega"
				],
				"EveryX": true,
				"NumberOfAbilitiesActivated": 1
			},
			"TotalDurationMs": 0,
			"Skills": [
				{
					"Name": "ExecutiuonSurge_Ability",
					"Deployment": {
						"Type": "Direct",
						"PreDeploymentDelayPercentage": 0
					},
					"Targeting": {
						"Type": "CurrentFocus"
					},
					"PercentageOfAbilityDuration": 0,
					"EffectPackage": {
						"Attributes": {
							"Propagation": {
								"PropagationType": "Splash",
								"SplashRadiusUnits": 20,
								"DeploymentGuidance": [
									"Ground",
									"Airborne",
									"Underground"
								],
								"EffectPackage": {
									"Effects": [
										{
											"Type": "InstantDamage",
											"DamageType": "Energy",
											"Expression": {
												"Operation": "+",
												"Operands": [
													{
														"Operation": "*",
														"Operands": [
															400000,
															{
																"Stat": "OmegaPowerPercentage",
																"StatSource": "Sender"
															}
														]
													}
												]
											}
										}
									]
								}
							}
						},
						"Effects": []
					}
				}
			]
		}
	],
	"EffectsReplacements": [
        {
			"ID": "IDInnateSimpleEffectReplace",
			"Type": "Buff",
			"Stat": "AttackSpeed",
			"OverlapProcessType": "Stacking",
			"DurationMs": -1,
			"FrequencyMs": 500,
			"Expression": 3000
		},
		{
		    "ID": "IDAttachedEffectInsideEmpower",
			"Type": "Debuff",
			"DurationMs": 9000,
			"Stat": "AttackPhysicalDamage",
			"OverlapProcessType": "Sum",
			"Expression": 2234
		},
	    {
            "ID": "IDAttributesPropagationEffectReplace",
		    "Type": "InstantDamage",
			"DamageType": "Energy",
			"Expression": 17000
	    },
        {
            "ID": "IDAttachedEffectPackageAttributesPropagationEffectReplace",
            "Type": "InstantDamage",
            "DamageType": "Pure",
            "Expression": 33500
        }
	],
	"DisplayName": "Overload Impact",
	"DisplayDescription": "Omega deals an additional 400*OP damage on impact.",
	"Description": {
		"Format": "Omega deals an additional 400*OP damage on impact.",
		"Parameters": {}
	},
	"DisplayDescriptionNormalized": "Omega deals an additional 400*OP damage on impact."
}
)";
        AddWeaponData(game_data_container, weapon_amplifier_flamewarden_overloadimpact_json);

        const char* suit_jumpsuit_json = R"(
{
	"Name": "Jumpsuit",
	"Stage": 0,
	"Tier": 0,
	"Variation": "Original",
	"Type": "Normal",
	"Stats": {
		"PhysicalResist": 0,
		"EnergyResist": 0,
		"MaxHealth": 100,
		"Grit": 0,
		"Resolve": 0,
		"AttackPhysicalDamage": 0,
		"AttackEnergyDamage": 0,
		"AttackPureDamage": 0,
		"AttackSpeed": 0,
		"AttackDodgeChancePercentage": 0,
		"MoveSpeedSubUnits": 0,
		"OmegaPowerPercentage": 0,
		"HealthRegeneration": 0,
		"EnergyRegeneration": 0,
		"StartingEnergy": 0,
		"StartingShield": 0,
		"CritChancePercentage": 0,
		"CritAmplificationPercentage": 0,
		"PhysicalVampPercentage": 0,
		"EnergyVampPercentage": 0,
		"PureVampPercentage": 0
	},
	"Abilities": [],
	"DisplayName": "Jumpsuit",
	"DisplayDescription": "<Italic>The Jumpsuit can be enhanced in Round 15 by selecting an Amplifier.</>",
	"Description": {
		"Format": "<Italic>The Jumpsuit can be enhanced in Round 15 by selecting an Amplifier.</>",
		"Parameters": {}
	},
	"DisplayDescriptionNormalized": "The Jumpsuit can be enhanced in Round 15 by selecting an Amplifier."
}
)";
        AdddSuitData(game_data_container, suit_jumpsuit_json);
    }

    void AddWeaponData(GameDataContainer& game_data_container, const std::string_view json_data) const
    {
        auto maybe_data = loader->ParseAndLoadWeapon(json_data);
        const CombatUnitWeaponData& weapon_data = maybe_data.value();
        game_data_container.AddWeaponData(weapon_data.type_id, std::make_shared<CombatUnitWeaponData>(weapon_data));
    }

    void AdddSuitData(GameDataContainer& game_data_container, const std::string_view json_data) const
    {
        auto maybe_data = loader->ParseAndLoadSuitData(json_data);
        const CombatUnitSuitData& suit_data = maybe_data.value();
        game_data_container.AddSuitData(suit_data.type_id, std::make_shared<CombatUnitSuitData>(suit_data));
    }

protected:
    std::unique_ptr<TestingDataLoader> loader;
};

TEST_F(CombatUnitRangerTest, SpawnRangerDefault)
{
    const FixedPoint ranger_combat_unit_data_default_max_health = 400000_fp;
    const FixedPoint expected_equipped_weapon_max_health = 700000_fp;
    const FixedPoint expected_equipped_suit_max_health = 100_fp;

    const CombatUnitData ranger_data = CreateRangerCombatUnitData(ranger_combat_unit_data_default_max_health);
    CombatUnitInstanceData ranger_instance_data = CreateCombatUnitInstanceData();
    {
        ranger_instance_data.team = Team::kBlue;
        ranger_instance_data.position = {10, 20};

        CombatWeaponTypeID& weapon_type_id = ranger_instance_data.equipped_weapon.type_id;
        weapon_type_id.name = "FlamewardenStaff";
        weapon_type_id.variation = "Original";
        weapon_type_id.combat_affinity = CombatAffinity::kFire;

        CombatSuitTypeID& suit_type_id = ranger_instance_data.equipped_suit.type_id;
        suit_type_id.name = "Jumpsuit";
        suit_type_id.variation = "Original";
    }

    Entity* female_ranger = nullptr;
    SpawnCombatUnit(ranger_instance_data, ranger_data, female_ranger);

    ASSERT_NE(female_ranger, nullptr);

    Entity& ranger_unit = world->GetByID(0);
    const auto& stats_component = ranger_unit.Get<StatsComponent>();

    // Base ranger + weapon + suit
    EXPECT_EQ(
        stats_component.GetCurrentHealth(),
        ranger_combat_unit_data_default_max_health + expected_equipped_weapon_max_health +
            expected_equipped_suit_max_health);

    // Check if omega is the same from the weapon
    const auto& abilities_component = ranger_unit.Get<AbilitiesComponent>();
    const auto& omega_abilities = abilities_component.GetAbilities(AbilityType::kOmega).data.abilities;
    ASSERT_EQ(omega_abilities.size(), 1);

    const auto& first_omega_ability = omega_abilities.front();
    ASSERT_EQ(first_omega_ability->name, "Infernal Convergence I");
    ASSERT_EQ(first_omega_ability->total_duration_ms, 2000);
}

TEST_F(CombatUnitRangerTest, SpawnRangerWithOneAmplifier)
{
    const FixedPoint ranger_combat_unit_data_default_max_health = 400000_fp;
    const FixedPoint expected_equipped_weapon_max_health = 700000_fp;
    const FixedPoint expected_equipped_suit_max_health = 100_fp;

    const CombatUnitData ranger_data = CreateRangerCombatUnitData(ranger_combat_unit_data_default_max_health);
    CombatUnitInstanceData ranger_instance_data = CreateCombatUnitInstanceData();
    {
        ranger_instance_data.team = Team::kBlue;
        ranger_instance_data.position = {10, 20};

        CombatWeaponTypeID& weapon_type_id = ranger_instance_data.equipped_weapon.type_id;
        weapon_type_id.name = "FlamewardenStaff";
        weapon_type_id.variation = "Original";
        weapon_type_id.combat_affinity = CombatAffinity::kFire;

        {
            CombatWeaponAmplifierInstanceData amplifier_instance_data;
            amplifier_instance_data.type_id.name = "FlamewardenStaffZonebreaker";
            amplifier_instance_data.type_id.variation = "Original";
            ranger_instance_data.equipped_weapon.equipped_amplifiers.push_back(amplifier_instance_data);
        }

        CombatSuitTypeID& suit_type_id = ranger_instance_data.equipped_suit.type_id;
        suit_type_id.name = "Jumpsuit";
        suit_type_id.variation = "Original";
    }

    const FixedPoint expected_first_amplifier_max_health = 400000_fp;

    Entity* female_ranger = nullptr;
    SpawnCombatUnit(ranger_instance_data, ranger_data, female_ranger);

    ASSERT_NE(female_ranger, nullptr);

    Entity& ranger_unit = world->GetByID(0);
    const auto& stats_component = ranger_unit.Get<StatsComponent>();

    // Base ranger + weapon + suit + amplifiers
    EXPECT_EQ(
        stats_component.GetCurrentHealth(),
        ranger_combat_unit_data_default_max_health + expected_equipped_weapon_max_health +
            expected_equipped_suit_max_health + expected_first_amplifier_max_health);

    // Check if omega was replaced
    const auto& abilities_component = ranger_unit.Get<AbilitiesComponent>();

    {
        const auto& omega_abilities = abilities_component.GetAbilities(AbilityType::kOmega).data.abilities;
        ASSERT_EQ(omega_abilities.size(), 1);

        const auto& first_omega_ability = omega_abilities.front();
        ASSERT_EQ(first_omega_ability->name, "Infernal Convergence I");
        ASSERT_EQ(first_omega_ability->total_duration_ms, 5000);
    }

    // Check if it has innates from amplifiers
    const auto& innate_abilities = abilities_component.GetAbilities(AbilityType::kInnate).data.abilities;
    ASSERT_EQ(innate_abilities.size(), 2);

    ASSERT_EQ(innate_abilities[0]->name, "VineboundCleaver_Ability1");
    ASSERT_EQ(innate_abilities[1]->name, "ZoneBreaker_Ability1");

    // Check no effect replacements have been done
    {
        const auto& first_innate_ability = innate_abilities[0];
        const auto& first_innate_effect_package = first_innate_ability->skills[0]->effect_package;

        const FixedPoint first_effect_propagation_expression_value =
            first_innate_effect_package.attributes.propagation.effect_package->effects[0].GetExpression().Evaluate({});
        EXPECT_EQ(first_effect_propagation_expression_value, 7000_fp);

        const FixedPoint first_effect_expression_value =
            first_innate_effect_package.effects[0].GetExpression().Evaluate({});
        EXPECT_EQ(first_effect_expression_value, 1000_fp);

        const int second_effect_duration_ms =
            first_innate_effect_package.effects[1].attached_effects[0].lifetime.duration_time_ms;
        EXPECT_EQ(second_effect_duration_ms, 2000);

        const FixedPoint third_effect_expression_value =
            first_innate_effect_package.effects[2]
                .attached_effect_package_attributes.propagation.effect_package->effects[0]
                .GetExpression()
                .Evaluate({});
        EXPECT_EQ(third_effect_expression_value, 23500_fp);
    }
}

TEST_F(CombatUnitRangerTest, SpawnRangerWithTwoAmplifier)
{
    const FixedPoint ranger_combat_unit_data_default_max_health = 400000_fp;
    const FixedPoint expected_equipped_weapon_max_health = 700000_fp;
    const FixedPoint expected_equipped_suit_max_health = 100_fp;

    const CombatUnitData ranger_data = CreateRangerCombatUnitData(ranger_combat_unit_data_default_max_health);
    CombatUnitInstanceData ranger_instance_data = CreateCombatUnitInstanceData();
    {
        ranger_instance_data.team = Team::kBlue;
        ranger_instance_data.position = {10, 20};

        CombatWeaponTypeID& weapon_type_id = ranger_instance_data.equipped_weapon.type_id;
        weapon_type_id.name = "FlamewardenStaff";
        weapon_type_id.variation = "Original";
        weapon_type_id.combat_affinity = CombatAffinity::kFire;

        {
            CombatWeaponAmplifierInstanceData amplifier_instance_data;
            amplifier_instance_data.type_id.name = "FlamewardenStaffZonebreaker";
            amplifier_instance_data.type_id.variation = "Original";
            ranger_instance_data.equipped_weapon.equipped_amplifiers.push_back(amplifier_instance_data);
        }
        {
            CombatWeaponAmplifierInstanceData amplifier_instance_data;
            amplifier_instance_data.type_id.name = "FlamewardenStaffOverloadImpact";
            amplifier_instance_data.type_id.variation = "Original";
            ranger_instance_data.equipped_weapon.equipped_amplifiers.push_back(amplifier_instance_data);
        }

        CombatSuitTypeID& suit_type_id = ranger_instance_data.equipped_suit.type_id;
        suit_type_id.name = "Jumpsuit";
        suit_type_id.variation = "Original";
    }

    const FixedPoint expected_first_amplifier_max_health = 400000_fp;
    const FixedPoint expected_second_amplifier_max_health = 100000_fp;

    Entity* female_ranger = nullptr;
    SpawnCombatUnit(ranger_instance_data, ranger_data, female_ranger);

    ASSERT_NE(female_ranger, nullptr);

    Entity& ranger_unit = world->GetByID(0);
    const auto& stats_component = ranger_unit.Get<StatsComponent>();

    // Base ranger + weapon + suit + amplifiers
    EXPECT_EQ(
        stats_component.GetCurrentHealth(),
        ranger_combat_unit_data_default_max_health + expected_equipped_weapon_max_health +
            expected_equipped_suit_max_health + expected_first_amplifier_max_health +
            expected_second_amplifier_max_health);

    // Check if omega was replaced
    const auto& abilities_component = ranger_unit.Get<AbilitiesComponent>();

    {
        const auto& omega_abilities = abilities_component.GetAbilities(AbilityType::kOmega).data.abilities;
        ASSERT_EQ(omega_abilities.size(), 1);

        const auto& first_omega_ability = omega_abilities.front();
        ASSERT_EQ(first_omega_ability->name, "Infernal Convergence I");
        ASSERT_EQ(first_omega_ability->total_duration_ms, 5000);
    }

    // Check if it has innates from amplifiers
    const auto& innate_abilities = abilities_component.GetAbilities(AbilityType::kInnate).data.abilities;
    ASSERT_EQ(innate_abilities.size(), 3);

    ASSERT_EQ(innate_abilities[0]->name, "VineboundCleaver_Ability1");
    ASSERT_EQ(innate_abilities[1]->name, "ZoneBreaker_Ability1");
    ASSERT_EQ(innate_abilities[2]->name, "Executiuon Surge");

    // Check effect replacements have been done
    {
        const auto& first_innate_ability = innate_abilities[0];
        const auto& first_innate_effect_package = first_innate_ability->skills[0]->effect_package;

        const FixedPoint first_effect_propagation_expression_value =
            first_innate_effect_package.attributes.propagation.effect_package->effects[0].GetExpression().Evaluate({});
        EXPECT_EQ(first_effect_propagation_expression_value, 17000_fp);

        const FixedPoint first_effect_expression_value =
            first_innate_effect_package.effects[0].GetExpression().Evaluate({});
        EXPECT_EQ(first_effect_expression_value, 3000_fp);

        const int second_effect_duration_ms =
            first_innate_effect_package.effects[1].attached_effects[0].lifetime.duration_time_ms;
        EXPECT_EQ(second_effect_duration_ms, 9000);

        const FixedPoint third_effect_expression_value =
            first_innate_effect_package.effects[2]
                .attached_effect_package_attributes.propagation.effect_package->effects[0]
                .GetExpression()
                .Evaluate({});
        EXPECT_EQ(third_effect_expression_value, 33500_fp);
    }
}

}  // namespace simulation
