#include "base_test_fixtures.h"
#include "components/abilities_component.h"
#include "components/attached_entity_component.h"
#include "components/stats_component.h"
#include "utility/entity_helper.h"

namespace simulation
{
class MarkSystemTest : public BaseTest
{
    typedef BaseTest Super;

protected:
    virtual void InitCombatUnitData()
    {
        data = CreateCombatUnitData();

        // Stats
        data.radius_units = 1;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kEnergyResist, 0_fp);
        data.type_data.stats.Set(StatType::kPhysicalResist, 0_fp);
        data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 300_fp);
        data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 5_fp);
        // No critical attacks
        data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);
        // No Dodge
        data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);
        // No Miss
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);

        // Once every second
        data.type_data.stats.Set(StatType::kAttackSpeed, 100_fp);

        data.type_data.stats.Set(StatType::kResolve, 0_fp);
        data.type_data.stats.Set(StatType::kGrit, 0_fp);
        data.type_data.stats.Set(StatType::kWillpowerPercentage, 0_fp);
        data.type_data.stats.Set(StatType::kTenacityPercentage, 0_fp);
        // 15^2 == 225
        // square distance between blue and red is 200
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 15_fp);
        data.type_data.stats.Set(StatType::kOmegaRangeUnits, 25_fp);
        data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 1000_fp);
        data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);

        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Foo";
            ability.total_duration_ms = 100;

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Attack";
            // 200 ms
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kDirect;
            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));

            skill1.deployment.pre_deployment_delay_percentage = 0;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void SetUpEventListeners()
    {
        // Subscribe to all the events
        world->SubscribeToEvent(
            EventType::kMarkCreated,
            [this](const Event& event)
            {
                events_created_mark.emplace_back(event.Get<event_data::MarkCreated>());
                marks_created++;
            });
        world->SubscribeToEvent(
            EventType::kMarkDestroyed,
            [this](const Event& event)
            {
                events_destroyed_mark.emplace_back(event.Get<event_data::MarkDestroyed>());
                marks_destroyed++;
            });

        // Listen to events
        world->SubscribeToEvent(
            EventType::kAbilityActivated,
            [this](const Event& event)
            {
                events_ability_activated.emplace_back(event.Get<const event_data::AbilityActivated&>());
            });
        world->SubscribeToEvent(
            EventType::kAbilityDeactivated,
            [this](const Event& event)
            {
                events_ability_deactivated.emplace_back(event.Get<event_data::AbilityDeactivated>());
            });
    }

    void SetUp() override
    {
        Super::SetUp();

        InitCombatUnitData();
        SetUpEventListeners();

        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {5, 5}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {7, 7}, data, red_entity);
        SpawnCombatUnit(Team::kRed, {9, 9}, data, red_entity2);
    }

    void AddMark(
        const Entity& sender_entity,
        const Entity& receiver_entity,
        const ActivationTriggerType trigger_type,
        const AllegianceType sender_allegiance,
        const AllegianceType receiver_allegiance,
        const int mark_effect_duration_ms)
    {
        // Ability
        auto ability_data = AbilityData::Create();
        {
            ability_data->name = "Innate Ability";
            ability_data->activation_trigger_data.trigger_type = trigger_type;
            ability_data->activation_trigger_data.max_activations = 1;
            ability_data->activation_trigger_data.sender_allegiance = sender_allegiance;
            ability_data->activation_trigger_data.receiver_allegiance = receiver_allegiance;
            ability_data->total_duration_ms = 0;

            // Skills
            auto& skill_data = ability_data->AddSkill();
            skill_data.name = "Innate Attack";
            skill_data.targeting.type = SkillTargetingType::kActivator;
            skill_data.deployment.type = SkillDeploymentType::kDirect;
            skill_data.percentage_of_ability_duration = 0;
            skill_data.deployment.pre_deployment_delay_percentage = 0;

            // Healing effect
            const auto effect_data =
                EffectData::CreateHeal(EffectHealType::kNormal, EffectExpression::FromValue(50_fp));

            // Add effect to skill
            skill_data.AddEffect(effect_data);
        }

        // Apply SpawnMark effect
        EffectState effect_state;
        effect_state.sender_stats = world->GetFullStats(sender_entity.GetID());
        effect_state.source_context.combat_unit_ability_type = AbilityType::kOmega;
        EffectData effect_data = EffectData::CreateSpawnMark(mark_effect_duration_ms);
        effect_data.attached_abilities.push_back(ability_data);
        effect_data.should_destroy_attached_entity_on_sender_death = ShouldDestroyOnSenderDeath();

        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            sender_entity.GetID(),
            receiver_entity.GetID(),
            effect_data,
            effect_state);
    }

    virtual bool ShouldDestroyOnSenderDeath() const
    {
        return false;
    }

    Entity* blue_entity = nullptr;
    Entity* red_entity = nullptr;
    Entity* red_entity2 = nullptr;
    CombatUnitData data{};

    // Listen to mark events
    std::vector<event_data::MarkCreated> events_created_mark;
    std::vector<event_data::MarkDestroyed> events_destroyed_mark;

    // Listen to ability events
    std::vector<event_data::AbilityActivated> events_ability_activated;
    std::vector<event_data::AbilityDeactivated> events_ability_deactivated;

    int marks_created = 0;
    int marks_destroyed = 0;
};

class MarkSystemTestWithDestroyOnSenderDeath : public MarkSystemTest
{
    typedef MarkSystemTest Super;
    typedef MarkSystemTestWithDestroyOnSenderDeath Self;

protected:
    bool ShouldDestroyOnSenderDeath() const override
    {
        return true;
    }
};

TEST_F(MarkSystemTest, MarkEntity)
{
    const auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    const auto& red_stats_component = red_entity->Get<StatsComponent>();
    const auto& red_abilities_component = red_entity->Get<AbilitiesComponent>();
    const auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Reduce health of the other red entity
    auto& red2_stats_component = red_entity2->Get<StatsComponent>();
    red2_stats_component.SetCurrentHealth(300_fp);

    // Add mark from red to blue
    AddMark(
        *red_entity,
        *blue_entity,
        ActivationTriggerType::kOnHit,
        AllegianceType::kEnemy,
        AllegianceType::kSelf,
        1000);

    // Check for innate abilities
    EXPECT_TRUE(blue_abilities_component.HasAnyAbility(AbilityType::kInnate));
    EXPECT_FALSE(red_abilities_component.HasAnyAbility(AbilityType::kInnate));

    // Make sure mark effect was attached.
    const auto& attached_entity_component = blue_entity->Get<AttachedEntityComponent>();
    auto& attached_entities = attached_entity_component.GetAttachedEntities();
    ASSERT_EQ(attached_entities.size(), 1);
    ASSERT_EQ(attached_entities[0].type, AttachedEntityType::kMark);

    // Blue entity should be marked with OnHit Ability activation trigger
    ASSERT_EQ(events_created_mark.size(), 1);
    ASSERT_EQ(events_ability_activated.size(), 0);

    // Check for current health
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 1000_fp);
    ASSERT_EQ(red_stats_component.GetCurrentHealth(), 1000_fp);

    events_created_mark.clear();

    // Entities attack each other
    ASSERT_TRUE(TimeStepUntilEventAbilityActivated());

    ASSERT_EQ(attached_entities.size(), 1);
    ASSERT_EQ(events_created_mark.size(), 0);

    // -100 damage for blue and red.
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 800_fp)
        << "Blue health should be 800 (it was hit by red and red2)";
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 950_fp) << "Red health should be 950";
    EXPECT_EQ(red2_stats_component.GetCurrentHealth(), 300_fp) << "Red2 should be unaffected";

    ASSERT_EQ(events_ability_activated.size(), 4);
    EXPECT_EQ(events_ability_activated[0].ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated[1].ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated[2].ability_type, AbilityType::kInnate);
    EXPECT_EQ(events_ability_activated[3].ability_type, AbilityType::kAttack);

    events_ability_activated.clear();

    // Innate ability should trigger and red should gain 25 health.
    // Mark should remove from unit
    ASSERT_TRUE(TimeStepUntilEventAbilityActivated());

    ASSERT_EQ(events_ability_activated.size(), 3);
    EXPECT_EQ(events_ability_activated[0].ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated[1].ability_type, AbilityType::kAttack);
    EXPECT_EQ(events_ability_activated[2].ability_type, AbilityType::kAttack);

    // -100 damage
    EXPECT_EQ(blue_stats_component.GetCurrentHealth(), 600_fp) << "Blue health should be 600";

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 850_fp) << "Red health should be 850";
    EXPECT_EQ(red2_stats_component.GetCurrentHealth(), 300_fp) << "Red2 health should be 300";

    // Mark effect was detached from red entity.
    EXPECT_EQ(attached_entities.size(), 0);

    // Mark effect was destroyed
    EXPECT_EQ(events_destroyed_mark.size(), 1);

    // Shouldn't have any innate ability after destroying mark
    EXPECT_FALSE(blue_abilities_component.HasAnyAbility(AbilityType::kInnate));
    EXPECT_FALSE(red_abilities_component.HasAnyAbility(AbilityType::kInnate));

    events_ability_activated.clear();
    events_destroyed_mark.clear();
}

TEST_F(MarkSystemTestWithDestroyOnSenderDeath, Test)
{
    const auto& red_abilities_component = red_entity->Get<AbilitiesComponent>();
    const auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    // Reduce health of the other red entity
    auto& red2_stats_component = red_entity2->Get<StatsComponent>();
    red2_stats_component.SetCurrentHealth(300_fp);

    // Add mark from red to blue
    AddMark(
        *red_entity,
        *blue_entity,
        ActivationTriggerType::kOnHit,
        AllegianceType::kEnemy,
        AllegianceType::kSelf,
        kTimeInfinite);

    // Check for innate abilities
    EXPECT_TRUE(blue_abilities_component.HasAnyAbility(AbilityType::kInnate));
    EXPECT_FALSE(red_abilities_component.HasAnyAbility(AbilityType::kInnate));

    // Make sure mark effect was attached.
    const auto& attached_entity_component = blue_entity->Get<AttachedEntityComponent>();
    auto& attached_entities = attached_entity_component.GetAttachedEntities();
    ASSERT_EQ(attached_entities.size(), 1);
    ASSERT_EQ(attached_entities[0].type, AttachedEntityType::kMark);

    // Blue entity should be marked with OnHit Ability activation trigger
    ASSERT_EQ(events_created_mark.size(), 1);
    ASSERT_EQ(events_ability_activated.size(), 0);

    // Faint the sender entity
    {
        EntityHelper::Kill(*red_entity);

        event_data::Fainted event_data;
        event_data.entity_id = red_entity->GetID();
        event_data.vanquisher_id = red_entity->GetID();
        world->EmitEvent<EventType::kFainted>(event_data);
    }

    TimeStepForNTimeSteps(2);

    // Mark effect was detached from red entity.
    EXPECT_EQ(attached_entities.size(), 0);

    // Mark effect was destroyed
    EXPECT_EQ(events_destroyed_mark.size(), 1);

    // Shouldn't have any innate ability after destroying mark
    EXPECT_FALSE(blue_abilities_component.HasAnyAbility(AbilityType::kInnate));
    EXPECT_FALSE(red_abilities_component.HasAnyAbility(AbilityType::kInnate));

    events_ability_activated.clear();
    events_destroyed_mark.clear();
}

}  // namespace simulation
