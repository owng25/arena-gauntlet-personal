#include "base_test_fixtures.h"
#include "components/abilities_component.h"
#include "components/position_component.h"
#include "components/stats_component.h"
#include "gtest/gtest.h"
#include "utility/stats_helper.h"

namespace simulation
{
class AbilitiesComponentTest : public BaseTest
{
};

TEST_F(AbilitiesComponentTest, ChooseOmegaAbilityWithSelfAttributeCheckMultiple)
{
    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<AbilitiesComponent>();
    blue_entity.Add<StatsComponent>();

    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);

    auto& blue_stats_component = blue_entity.Get<StatsComponent>();
    StatsHelper::SetDefaults(blue_stats_component.GetMutableTemplateStats());
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kCritAmplificationPercentage, 100_fp);

    auto& blue_ability_component = blue_entity.Get<AbilitiesComponent>();

    // Create data
    AbilitiesData omega_abilities;
    omega_abilities.activation_check_value = 50_fp;
    omega_abilities.activation_check_stat_type = StatType::kCritAmplificationPercentage;

    auto& ability1 = omega_abilities.AddAbility();
    ability1.name = "Omega1";
    auto& ability2 = omega_abilities.AddAbility();
    ability2.name = "Omega2";
    omega_abilities.selection_type = AbilitySelectionType::kSelfAttributeCheck;

    ASSERT_FALSE(blue_ability_component.HasAnyAbility(AbilityType::kOmega));
    ASSERT_FALSE(blue_ability_component.HasAnyAbility(AbilityType::kAttack));
    blue_ability_component.SetAbilitiesData(omega_abilities, AbilityType::kOmega);
    ASSERT_TRUE(blue_ability_component.HasAnyAbility(AbilityType::kOmega));
    ASSERT_FALSE(blue_ability_component.HasAnyAbility(AbilityType::kAttack));

    // Should be zero
    ASSERT_EQ(0, blue_ability_component.ChooseOmegaAbility()->index);

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kCritAmplificationPercentage, 10_fp);
    // CritAmplification below 50(activation percentage), so index should be 1
    ASSERT_EQ(1, blue_ability_component.ChooseOmegaAbility()->index);
}

TEST_F(AbilitiesComponentTest, ChooseAttackAbilityWithSelfAttributeCheckMultiple)
{
    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<AbilitiesComponent>();
    blue_entity.Add<StatsComponent>();

    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);

    auto& blue_stats_component = blue_entity.Get<StatsComponent>();
    StatsHelper::SetDefaults(blue_stats_component.GetMutableTemplateStats());
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kCritAmplificationPercentage, 100_fp);

    auto& blue_ability_component = blue_entity.Get<AbilitiesComponent>();

    // Create data
    AbilitiesData attack_abilities;
    attack_abilities.activation_check_value = 50_fp;
    attack_abilities.activation_check_stat_type = StatType::kCritAmplificationPercentage;

    auto& ability1 = attack_abilities.AddAbility();
    ability1.name = "Attack1";
    auto& ability2 = attack_abilities.AddAbility();
    ability2.name = "Attack2";
    attack_abilities.selection_type = AbilitySelectionType::kSelfAttributeCheck;

    ASSERT_FALSE(blue_ability_component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_FALSE(blue_ability_component.HasAnyAbility(AbilityType::kOmega));
    blue_ability_component.SetAbilitiesData(attack_abilities, AbilityType::kAttack);
    ASSERT_TRUE(blue_ability_component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_FALSE(blue_ability_component.HasAnyAbility(AbilityType::kOmega));

    // Should be zero
    ASSERT_EQ(0, blue_ability_component.ChooseAttackAbility()->index);

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kCritAmplificationPercentage, 10_fp);
    // CritAmplification below 50(activation percentage), so index should be 1
    ASSERT_EQ(1, blue_ability_component.ChooseAttackAbility()->index);
}

TEST_F(AbilitiesComponentTest, ChooseOmegaAbilityWithSelfHealthCheckMultiple)
{
    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<AbilitiesComponent>();
    blue_entity.Add<StatsComponent>();

    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);

    auto& blue_stats_component = blue_entity.Get<StatsComponent>();
    StatsHelper::SetDefaults(blue_stats_component.GetMutableTemplateStats());
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, 100_fp);
    blue_stats_component.SetCurrentHealth(100_fp);

    auto& blue_ability_component = blue_entity.Get<AbilitiesComponent>();

    // Create data
    AbilitiesData omega_abilities;
    omega_abilities.activation_check_value = 50_fp;

    auto& ability1 = omega_abilities.AddAbility();
    ability1.name = "Omega1";
    auto& ability2 = omega_abilities.AddAbility();
    ability2.name = "Omega2";
    omega_abilities.selection_type = AbilitySelectionType::kSelfHealthCheck;

    ASSERT_FALSE(blue_ability_component.HasAnyAbility(AbilityType::kOmega));
    ASSERT_FALSE(blue_ability_component.HasAnyAbility(AbilityType::kAttack));
    blue_ability_component.SetAbilitiesData(omega_abilities, AbilityType::kOmega);
    ASSERT_TRUE(blue_ability_component.HasAnyAbility(AbilityType::kOmega));
    ASSERT_FALSE(blue_ability_component.HasAnyAbility(AbilityType::kAttack));

    // Should be zero
    ASSERT_EQ(0, blue_ability_component.ChooseOmegaAbility()->index);

    blue_stats_component.SetCurrentHealth(20_fp);
    // Health below 50(activation percentage), so index should be 1
    ASSERT_EQ(1, blue_ability_component.ChooseOmegaAbility()->index);
}

TEST_F(AbilitiesComponentTest, ChooseAttackAbilityWithSelfHealthCheckMultiple)
{
    // Add blue entity with components
    auto& blue_entity = world->AddEntity(Team::kBlue);
    blue_entity.Add<PositionComponent>();
    blue_entity.Add<AbilitiesComponent>();
    blue_entity.Add<StatsComponent>();

    auto& blue_position_component = blue_entity.Get<PositionComponent>();
    blue_position_component.SetPosition(10, 10);

    auto& blue_stats_component = blue_entity.Get<StatsComponent>();
    StatsHelper::SetDefaults(blue_stats_component.GetMutableTemplateStats());
    blue_stats_component.GetMutableTemplateStats().Set(StatType::kMaxHealth, 100_fp);
    blue_stats_component.SetCurrentHealth(100_fp);

    auto& blue_ability_component = blue_entity.Get<AbilitiesComponent>();

    // Create data
    AbilitiesData attack_abilities;
    attack_abilities.activation_check_value = 50_fp;

    auto& ability1 = attack_abilities.AddAbility();
    ability1.name = "Attack1";
    auto& ability2 = attack_abilities.AddAbility();
    ability2.name = "Attack2";
    attack_abilities.selection_type = AbilitySelectionType::kSelfHealthCheck;

    ASSERT_FALSE(blue_ability_component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_FALSE(blue_ability_component.HasAnyAbility(AbilityType::kOmega));
    blue_ability_component.SetAbilitiesData(attack_abilities, AbilityType::kAttack);
    ASSERT_TRUE(blue_ability_component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_FALSE(blue_ability_component.HasAnyAbility(AbilityType::kOmega));

    // Should be zero
    ASSERT_EQ(0, blue_ability_component.ChooseAttackAbility()->index);

    blue_stats_component.SetCurrentHealth(20_fp);
    // Health below 50(activation percentage), so index should be 1
    ASSERT_EQ(1, blue_ability_component.ChooseAttackAbility()->index);
}
TEST_F(AbilitiesComponentTest, ChooseAttackAbilityWithCycleSingle)
{
    // Create data
    AbilitiesData attack_abilities;
    auto& ability1 = attack_abilities.AddAbility();
    ability1.name = "Attack1";
    attack_abilities.selection_type = AbilitySelectionType::kCycle;

    Entity& entity = world->AddEntity(Team::kBlue);
    auto component = entity.Add<AbilitiesComponent>();
    ASSERT_FALSE(component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_FALSE(component.HasAnyAbility(AbilityType::kOmega));
    component.SetAbilitiesData(attack_abilities, AbilityType::kAttack);
    ASSERT_TRUE(component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_FALSE(component.HasAnyAbility(AbilityType::kOmega));

    // Should always be zero
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
}

TEST_F(AbilitiesComponentTest, ChooseAttackAbilityWithCycleMultiple)
{
    Entity& entity = world->AddEntity(Team::kBlue);
    auto component = entity.Add<AbilitiesComponent>();

    // Create data
    AbilitiesData attack_abilities;
    auto& ability1 = attack_abilities.AddAbility();
    ability1.name = "Attack1";
    auto& ability2 = attack_abilities.AddAbility();
    ability2.name = "Attack2";
    auto& ability3 = attack_abilities.AddAbility();
    ability3.name = "Attack3";
    attack_abilities.selection_type = AbilitySelectionType::kCycle;

    component.SetAbilitiesData(attack_abilities, AbilityType::kAttack);

    auto& current_ability_index = component.GetAbilities(AbilityType::kAttack).state.current_ability_index;

    // Should wrap around
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
    current_ability_index = 0;
    ASSERT_EQ(1, component.ChooseAttackAbility()->index);
    current_ability_index = 1;
    ASSERT_EQ(2, component.ChooseAttackAbility()->index);
    current_ability_index = 2;
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
    current_ability_index = 0;
    ASSERT_EQ(1, component.ChooseAttackAbility()->index);
    current_ability_index = 1;
    ASSERT_EQ(2, component.ChooseAttackAbility()->index);
    current_ability_index = 2;
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
    current_ability_index = 0;
}

TEST_F(AbilitiesComponentTest, ChooseOmegaAbilityWithCycleSingle)
{
    Entity& entity = world->AddEntity(Team::kBlue);
    auto component = entity.Add<AbilitiesComponent>();

    // Create data
    AbilitiesData omega_abilities;
    auto& ability1 = omega_abilities.AddAbility();
    ability1.name = "Omega1";
    omega_abilities.selection_type = AbilitySelectionType::kCycle;

    ASSERT_FALSE(component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_FALSE(component.HasAnyAbility(AbilityType::kOmega));
    component.SetAbilitiesData(omega_abilities, AbilityType::kOmega);
    ASSERT_FALSE(component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_TRUE(component.HasAnyAbility(AbilityType::kOmega));

    // Should always be zero
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
}

TEST_F(AbilitiesComponentTest, ChooseOmegaAbilityWithCycleMultiple)
{
    // Create data
    AbilitiesData omega_abilities;
    auto& ability1 = omega_abilities.AddAbility();
    ability1.name = "Omega1";
    auto& ability2 = omega_abilities.AddAbility();
    ability2.name = "Omega2";
    auto& ability3 = omega_abilities.AddAbility();
    ability3.name = "Omega3";
    omega_abilities.selection_type = AbilitySelectionType::kCycle;

    Entity& entity = world->AddEntity(Team::kBlue);
    auto& component = entity.Add<AbilitiesComponent>();
    component.SetAbilitiesData(omega_abilities, AbilityType::kOmega);
    auto& current_ability_index = component.GetAbilities(AbilityType::kOmega).state.current_ability_index;

    // Should wrap around
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
    current_ability_index = 0;
    ASSERT_EQ(1, component.ChooseOmegaAbility()->index);
    current_ability_index = 1;
    ASSERT_EQ(2, component.ChooseOmegaAbility()->index);
    current_ability_index = 2;
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
    current_ability_index = 0;
    ASSERT_EQ(1, component.ChooseOmegaAbility()->index);
    current_ability_index = 1;
    ASSERT_EQ(2, component.ChooseOmegaAbility()->index);
    current_ability_index = 2;
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
    current_ability_index = 0;
}

TEST_F(AbilitiesComponentTest, ChooseAllAbilitiesWithCycle)
{
    // Create attack
    AbilitiesData attack_abilities;
    {
        auto& ability1 = attack_abilities.AddAbility();
        ability1.name = "Attack1";
        auto& ability2 = attack_abilities.AddAbility();
        ability2.name = "Attack2";
    }
    attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Create omega abilities
    AbilitiesData omega_abilities;
    {
        auto& ability1 = omega_abilities.AddAbility();
        ability1.name = "Omega1";
        auto& ability2 = omega_abilities.AddAbility();
        ability2.name = "Omega2";
        auto& ability3 = omega_abilities.AddAbility();
        ability3.name = "Omega3";
        auto& ability4 = omega_abilities.AddAbility();
        ability4.name = "Omega4";
    }
    omega_abilities.selection_type = AbilitySelectionType::kCycle;

    Entity& entity = world->AddEntity(Team::kBlue);
    auto component = entity.Add<AbilitiesComponent>();
    component.SetAbilitiesData(attack_abilities, AbilityType::kAttack);
    component.SetAbilitiesData(omega_abilities, AbilityType::kOmega);
    auto& omega_index = component.GetAbilities(AbilityType::kOmega).state.current_ability_index;
    auto& attack_index = component.GetAbilities(AbilityType::kAttack).state.current_ability_index;

    // Should keep track separate
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
    attack_index = 0;
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
    omega_index = 0;
    ASSERT_EQ(1, component.ChooseAttackAbility()->index);
    attack_index = 1;
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
    attack_index = 0;
    ASSERT_EQ(1, component.ChooseOmegaAbility()->index);
    omega_index = 1;
    ASSERT_EQ(1, component.ChooseAttackAbility()->index);
    attack_index = 1;
    ASSERT_EQ(2, component.ChooseOmegaAbility()->index);
    omega_index = 2;
    ASSERT_EQ(3, component.ChooseOmegaAbility()->index);
    omega_index = 3;
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
    attack_index = 0;
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
}

TEST_F(AbilitiesComponentTest, ChooseAttackAbilityWithEveryXActivation)
{
    // Create data
    AbilitiesData attack_abilities;
    auto& ability1 = attack_abilities.AddAbility();
    ability1.name = "Attack1";
    auto& ability2 = attack_abilities.AddAbility();
    ability2.name = "Attack2";
    attack_abilities.selection_type = AbilitySelectionType::kEveryXActivations;
    attack_abilities.activation_cadence = 1;

    Entity& entity = world->AddEntity(Team::kBlue);
    auto component = entity.Add<AbilitiesComponent>();
    component.SetAbilitiesData(attack_abilities, AbilityType::kAttack);
    auto& ability_state = component.GetStateAttackAbilities();

    // Should wrap around, choosing ability changes index but activating does not
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
    component.OnAbilityActivated(ability_state.at(0), 0);
    ASSERT_EQ(1, component.ChooseAttackAbility()->index);

    ASSERT_EQ(1, component.ChooseAttackAbility()->index);
    component.OnAbilityActivated(ability_state.at(1), 0);
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);

    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
    component.OnAbilityActivated(ability_state.at(0), 0);
    ASSERT_EQ(1, component.ChooseAttackAbility()->index);

    ASSERT_EQ(1, component.ChooseAttackAbility()->index);
}

TEST_F(AbilitiesComponentTest, ChooseAttackAbilityWithEveryXActivations)
{
    // Create data
    AbilitiesData attack_abilities;
    auto& ability1 = attack_abilities.AddAbility();
    ability1.name = "Attack1";
    auto& ability2 = attack_abilities.AddAbility();
    ability2.name = "Attack2";
    attack_abilities.selection_type = AbilitySelectionType::kEveryXActivations;
    attack_abilities.activation_cadence = 2;

    Entity& entity = world->AddEntity(Team::kBlue);
    auto component = entity.Add<AbilitiesComponent>();
    component.SetAbilitiesData(attack_abilities, AbilityType::kAttack);
    auto& ability_state = component.GetStateAttackAbilities();

    // Should not wrap around
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);

    // Should wrap around
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);
    component.OnAbilityActivated(ability_state.at(component.ChooseAttackAbility()->index), 0);
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);

    component.OnAbilityActivated(ability_state.at(component.ChooseAttackAbility()->index), 0);
    ASSERT_EQ(1, component.ChooseAttackAbility()->index);

    component.OnAbilityActivated(ability_state.at(component.ChooseAttackAbility()->index), 0);
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);

    component.OnAbilityActivated(ability_state.at(component.ChooseAttackAbility()->index), 0);
    ASSERT_EQ(1, component.ChooseAttackAbility()->index);

    component.OnAbilityActivated(ability_state.at(component.ChooseAttackAbility()->index), 0);
    ASSERT_EQ(0, component.ChooseAttackAbility()->index);

    component.OnAbilityActivated(ability_state.at(component.ChooseAttackAbility()->index), 0);
    ASSERT_EQ(1, component.ChooseAttackAbility()->index);
}

TEST_F(AbilitiesComponentTest, ChooseOmegaAbilityWithEveryXActivation)
{
    // Create data
    AbilitiesData omega_abilities;
    auto& ability1 = omega_abilities.AddAbility();
    ability1.name = "Omega1";
    auto& ability2 = omega_abilities.AddAbility();
    ability2.name = "Omega2";
    omega_abilities.selection_type = AbilitySelectionType::kEveryXActivations;
    omega_abilities.activation_cadence = 1;

    Entity& entity = world->AddEntity(Team::kBlue);
    auto component = entity.Add<AbilitiesComponent>();
    component.SetAbilitiesData(omega_abilities, AbilityType::kOmega);
    auto& ability_state = component.GetStateOmegaAbilities();

    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
    component.OnAbilityActivated(ability_state.at(0), 0);
    ASSERT_EQ(1, component.ChooseOmegaAbility()->index);
    ASSERT_EQ(1, component.ChooseOmegaAbility()->index);
    component.OnAbilityActivated(ability_state.at(1), 0);
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
    component.OnAbilityActivated(ability_state.at(0), 0);
    ASSERT_EQ(1, component.ChooseOmegaAbility()->index);
    ASSERT_EQ(1, component.ChooseOmegaAbility()->index);
    component.OnAbilityActivated(ability_state.at(1), 0);
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
}

TEST_F(AbilitiesComponentTest, ChooseOmegaAbilityWithEveryXActivations)
{
    // Create data
    AbilitiesData omega_abilities;
    auto& ability1 = omega_abilities.AddAbility();
    ability1.name = "Omega1";
    auto& ability2 = omega_abilities.AddAbility();
    ability2.name = "Omega2";
    omega_abilities.selection_type = AbilitySelectionType::kEveryXActivations;
    omega_abilities.activation_cadence = 2;

    Entity& entity = world->AddEntity(Team::kBlue);
    auto component = entity.Add<AbilitiesComponent>();
    component.SetAbilitiesData(omega_abilities, AbilityType::kOmega);
    auto& ability_state = component.GetStateOmegaAbilities();

    // Should not wrap around
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);

    // Should wrap around
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);
    component.OnAbilityActivated(ability_state.at(component.ChooseOmegaAbility()->index), 0);
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);

    component.OnAbilityActivated(ability_state.at(component.ChooseOmegaAbility()->index), 0);
    ASSERT_EQ(1, component.ChooseOmegaAbility()->index);

    component.OnAbilityActivated(ability_state.at(component.ChooseOmegaAbility()->index), 0);
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);

    component.OnAbilityActivated(ability_state.at(component.ChooseOmegaAbility()->index), 0);
    ASSERT_EQ(1, component.ChooseOmegaAbility()->index);

    component.OnAbilityActivated(ability_state.at(component.ChooseOmegaAbility()->index), 0);
    ASSERT_EQ(0, component.ChooseOmegaAbility()->index);

    component.OnAbilityActivated(ability_state.at(component.ChooseOmegaAbility()->index), 0);
    ASSERT_EQ(1, component.ChooseOmegaAbility()->index);
}

}  // namespace simulation
