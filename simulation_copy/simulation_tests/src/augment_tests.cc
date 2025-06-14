#include "base_test_fixtures.h"
#include "components/abilities_component.h"
#include "components/augment_component.h"

namespace simulation
{
class AugmentTest : public BaseTest
{
    typedef BaseTest Super;

protected:
    virtual void FillAugmentAbilities(std::vector<std::shared_ptr<const AbilityData>>& abilities_data) const
    {
        auto add_skill = [](AbilityData& ability)
        {
            auto& skill = ability.AddSkill();
            skill.name = "Skill";
            skill.targeting.type = SkillTargetingType::kCurrentFocus;
            skill.deployment.type = SkillDeploymentType::kDirect;
            skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(100_fp));
        };

        {
            auto ability = AbilityData::Create();
            ability->name = "AbilityA";
            add_skill(*ability);
            abilities_data.push_back(std::move(ability));
        }

        {
            auto ability = AbilityData::Create();
            ability->name = "AbilityB";
            add_skill(*ability);
            abilities_data.push_back(std::move(ability));
        }
    }

    static StatsData CreateStats()
    {
        StatsData stats_data;
        stats_data.Set(StatType::kAttackSpeed, 10_fp);
        stats_data.Set(StatType::kMaxHealth, 20_fp);
        return stats_data;
    }

    void FillAugmentsData(GameDataContainer& game_data) const override
    {
        auto augment = std::make_shared<AugmentData>();
        augment->augment_type = AugmentType::kNormal;
        augment->type_id = AugmentTypeID{"TwoAbilities", 1, ""};
        FillAugmentAbilities(augment->innate_abilities);
        game_data.AddAugmentData(augment->type_id, augment);
    }
};

TEST_F(AugmentTest, AddAndRemove)
{
    auto make_instance = []()
    {
        return AugmentInstanceData{"", {"TwoAbilities", 1, ""}};
    };

    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, HexGridPosition{25, 50}, CreateCombatUnitData(), blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    const auto& abilities_component = blue_entity->Get<AbilitiesComponent>();
    const auto& augment_component = blue_entity->Get<AugmentComponent>();
    auto& innates = abilities_component.GetDataInnateAbilities().abilities;

    // Add augment to entity
    ASSERT_TRUE(world->AddAugmentBeforeBattleStarted(*blue_entity, make_instance()));
    ASSERT_EQ(augment_component.GetEquippedAugmentsSize(), 1);
    ASSERT_EQ(innates.size(), 0);

    // Add augment with another selected effect to an entity
    ASSERT_TRUE(world->AddAugmentBeforeBattleStarted(*blue_entity, make_instance()));
    ASSERT_EQ(augment_component.GetEquippedAugmentsSize(), 2);

    // Remove augment from entity
    ASSERT_TRUE(world->RemoveAugmentBeforeBattleStarted(*blue_entity, make_instance()));
    ASSERT_EQ(augment_component.GetEquippedAugmentsSize(), 0);
    ASSERT_EQ(innates.size(), 0);

    world->TimeStep();

    ASSERT_EQ(augment_component.GetEquippedAugmentsSize(), 0);
    ASSERT_EQ(innates.size(), 0);
}

class AugmentWithBlink : public AugmentTest
{
public:
    void FillAugmentsData(GameDataContainer& game_data) const override
    {
        for (int stage = 1; stage != 3; ++stage)
        {
            auto augment = std::make_shared<AugmentData>();
            augment->augment_type = AugmentType::kNormal;
            augment->type_id = AugmentTypeID{"Blink augment", stage, ""};
            FillAugmentAbilities(augment->innate_abilities);
            game_data.AddAugmentData(augment->type_id, augment);
        }
    }

    void FillAugmentAbilities(std::vector<std::shared_ptr<const AbilityData>>& abilities) const override
    {
        auto ability = AbilityData::Create();
        ability->name = "Blink ability";
        ability->total_duration_ms = 0;
        ability->activation_trigger_data.trigger_type = ActivationTriggerType::kOnBattleStart;

        auto& skill = ability->AddSkill();
        skill.name = "Blink skill";
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.targeting.type = SkillTargetingType::kCurrentFocus;
        skill.AddEffect(EffectData::CreateBlink(ReservedPositionType::kBehindReceiver, 0, 0));
        abilities.push_back(std::move(ability));
    }
};

TEST_F(AugmentWithBlink, MultipleBlinks)
{
    auto make_instance = [](const int stage)
    {
        return AugmentInstanceData{"", {"Blink augment", stage, ""}};
    };

    // Blue entity with 2 augments and each augment brings blink ability
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, HexGridPosition{25, 50}, CreateCombatUnitData(), blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    const auto& abilities_component = blue_entity->Get<AbilitiesComponent>();
    const auto& augment_component = blue_entity->Get<AugmentComponent>();
    auto& innates = abilities_component.GetDataInnateAbilities().abilities;

    // Add 2 augments to an entity
    for (int i = 1; i != 3; ++i)
    {
        ASSERT_TRUE(world->AddAugmentBeforeBattleStarted(*blue_entity, make_instance(i)));
        ASSERT_EQ(augment_component.GetEquippedAugmentsSize(), i);
        ASSERT_EQ(innates.size(), 0);
    }

    // Red entity with infinity stun
    Entity* red_entity = nullptr;
    SpawnCombatUnit(Team::kRed, HexGridPosition{25, -50}, CreateCombatUnitData(), red_entity);
    ASSERT_NE(red_entity, nullptr);
    Stun(*red_entity);

    world->TimeStep();
    ASSERT_EQ(innates.size(), 1);
}
}  // namespace simulation
