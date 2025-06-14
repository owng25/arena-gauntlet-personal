#include "ability_system_data_fixtures.h"
#include "gtest/gtest.h"

namespace simulation
{
class ChannelTimeTest : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    void SetUp() override
    {
        Super::SetUp();

        world->SubscribeToEvent(
            EventType::kSkillDeploying,
            [this](const Event& event)
            {
                events_skill_deploying.emplace_back(event.Get<event_data::Skill>());
            });

        world->SubscribeToEvent(
            EventType::kSkillChanneling,
            [this](const Event& event)
            {
                events_skill_channeling.emplace_back(event.Get<event_data::Skill>());
            });

        world->SubscribeToEvent(
            EventType::kSkillFinished,
            [this](const Event& event)
            {
                events_skill_finished.emplace_back(event.Get<event_data::Skill>());
            });
    }

    bool UseInstantTimers() const override
    {
        return true;
    }

    void InitStats() override
    {
        data = CreateCombatUnitData();

        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 20_fp);
        data.type_data.stats.Set(StatType::kStartingEnergy, 100_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
    }

    void InitAttackAbilities() override {}

    void InitOmegaAbilities() override
    {
        // Add omega abilities
        {
            auto& ability = data.type_data.omega_abilities.AddAbility();
            ability.name = "Omega Ability";
            ability.total_duration_ms = 1000;

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Omega Attack 1";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.channel_time_ms = 700;

            // This skill lasts 800 ms but the channel time is just 700 ms
            skill1.percentage_of_ability_duration = 80;
            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(10_fp));

            auto& skill2 = ability.AddSkill();
            skill2.name = "Omega Attack 2";
            skill2.targeting.type = SkillTargetingType::kCurrentFocus;
            skill2.deployment.type = SkillDeploymentType::kDirect;
            skill2.channel_time_ms = 0;
            // Use anything that's left
            skill2.percentage_of_ability_duration = 100;
            skill2.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(10_fp));
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {10, 10}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {20, 20}, data, red_entity);
    }

    std::vector<event_data::Skill> events_skill_deploying;
    std::vector<event_data::Skill> events_skill_channeling;
    std::vector<event_data::Skill> events_skill_finished;
};

TEST_F(ChannelTimeTest, TimingTest)
{
    int skill1_active_count = 0;
    int skill2_active_count = 0;
    int channeling_count = 0;
    int deploying_count = 0;

    // Run for 11 time steps - 10 time steps for the ability
    // + extra a time step to check bounds of ability
    for (int time_step = 0; time_step < 11; time_step++)
    {
        // TimeStep
        world->TimeStep();

        // Get ability
        const auto& abilities_component = blue_entity->Get<AbilitiesComponent>();
        const auto ability = abilities_component.GetActiveAbility();
        if (ability == nullptr)
        {
            continue;
        }

        // Get skill
        const auto skill = abilities_component.GetActiveSkill();
        if (skill == nullptr)
        {
            continue;
        }

        // Gather data for active skill
        switch (skill->index)
        {
        case 0:
            skill1_active_count++;
            break;
        case 1:
            skill2_active_count++;
            break;
        default:
            assert(false);
            break;
        }

        // Gather data for states
        switch (skill->state)
        {
        case SkillStateType::kDeploying:
            deploying_count++;
            break;
        case SkillStateType::kChanneling:
            channeling_count++;
            break;
        default:
            break;
        }
    }

    // Channeling skill active for 5 time steps
    EXPECT_EQ(skill1_active_count, 8);

    // Standard skill active for remainder of 10 time steps
    EXPECT_EQ(skill2_active_count, 2);

    // 2 entities
    const int entities_num = 2;
    EXPECT_EQ(events_skill_finished.size(), 2 * entities_num);
    EXPECT_EQ(events_skill_deploying.size(), 2 * entities_num);
    EXPECT_EQ(events_skill_channeling.size(), 1 * entities_num);
    EXPECT_EQ(deploying_count, 4);
    EXPECT_EQ(channeling_count, 6);
}

}  // namespace simulation
