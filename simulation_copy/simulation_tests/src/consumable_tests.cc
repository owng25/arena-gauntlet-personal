#include "base_test_fixtures.h"
#include "components/abilities_component.h"
#include "components/consumable_component.h"

namespace simulation
{
class ConsumableBonusSystemTest : public BaseTest
{
    typedef BaseTest Super;

protected:
    void SetUp() override
    {
        InitConsumableInstances();
        Super::SetUp();

        SpawnEntities();
    }

    static AbilitiesData CreateAbilities()
    {
        AbilitiesData abilities_data;
        auto& ability = abilities_data.AddAbility();
        ability.name = "Bar";
        ability.activation_trigger_data.activation_time_limit_ms = 200;

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.name = "After Omega";
        skill1.targeting.type = SkillTargetingType::kCurrentFocus;
        skill1.deployment.type = SkillDeploymentType::kDirect;
        skill1.deployment.pre_deployment_delay_percentage = 0;
        skill1.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(100_fp));

        return abilities_data;
    }

    virtual void InitConsumableInstances()
    {
        consumable_instance_.type_id = ConsumableTypeID{"Heal", 1};
    }

    void FillConsumablesData(GameDataContainer& game_data) const override
    {
        auto consumable = std::make_shared<ConsumableData>();
        consumable->type_id = consumable_instance_.type_id;
        consumable->innate_abilities = CreateAbilities().abilities;
        game_data.AddConsumableData(consumable->type_id, consumable);
    }

    void SpawnEntities()
    {
        auto combat_unit_data = CreateCombatUnitData();
        combat_unit_data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);

        SpawnCombatUnit(Team::kBlue, HexGridPosition{25, 50}, combat_unit_data, blue_entity_);
        assert(blue_entity_);
    }

protected:
    Entity* blue_entity_ = nullptr;
    CombatUnitData combat_unit_data_{};

    ConsumableInstanceData consumable_instance_;
};

TEST_F(ConsumableBonusSystemTest, AddConsumable)
{
    const auto& consumable_component = blue_entity_->Get<ConsumableComponent>();

    // Add consumable to entity
    const bool is_consumable_added = world->AddConsumableBeforeBattleStarted(*blue_entity_, consumable_instance_);
    ASSERT_TRUE(is_consumable_added);
    ASSERT_EQ(consumable_component.GetEquippedConsumablesSize(), 1);
}

TEST_F(ConsumableBonusSystemTest, RemoveConsumable)
{
    const auto& consumable_component = blue_entity_->Get<ConsumableComponent>();

    // Add consumable to entity
    const bool is_add_consumable = world->AddConsumableBeforeBattleStarted(*blue_entity_, consumable_instance_);
    ASSERT_TRUE(is_add_consumable);
    ASSERT_EQ(consumable_component.GetEquippedConsumablesSize(), 1);

    // Remove consumable from entity
    const bool is_consumable_removed = world->RemoveConsumableBeforeBattleStarted(*blue_entity_, consumable_instance_);
    ASSERT_TRUE(is_consumable_removed);
    ASSERT_EQ(consumable_component.GetEquippedConsumablesSize(), 0);
}

TEST_F(ConsumableBonusSystemTest, AbilityFromConsumable)
{
    const auto& abilities_component = blue_entity_->Get<AbilitiesComponent>();

    // Add augment to entity
    const bool is_consumable_added = world->AddConsumableBeforeBattleStarted(*blue_entity_, consumable_instance_);
    ASSERT_TRUE(is_consumable_added);

    // Before game started
    ASSERT_EQ(abilities_component.HasAnyAbility(AbilityType::kInnate), false);

    TimeStepForMs(100);
    TimeStepForMs(100);

    // After game started
    ASSERT_EQ(abilities_component.HasAnyAbility(AbilityType::kInnate), true);
}

}  // namespace simulation
