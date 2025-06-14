#include "ability_system_data_fixtures.h"
#include "components/position_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "gtest/gtest.h"

namespace simulation
{
class DashSystemTest : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    void InitStats() override
    {
        data = CreateCombatUnitData();
        dummy_data = CreateCombatUnitData();

        // Stats
        data.radius_units = 2;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        // 30^2 == 900
        // square distance between blue and red is 800
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 30_fp);
        data.type_data.stats.Set(StatType::kStartingEnergy, 100_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 200_fp);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);
        data.type_data.stats.Set(StatType::kCritChancePercentage, 100_fp);

        // Dummy Stats
        dummy_data.radius_units = 1;
        dummy_data.type_data.combat_affinity = CombatAffinity::kFire;
        dummy_data.type_data.combat_class = CombatClass::kBehemoth;
        dummy_data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        dummy_data.type_data.stats.Set(StatType::kAttackRangeUnits, 1_fp);
        dummy_data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        dummy_data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        dummy_data.type_data.stats.Set(StatType::kMaxHealth, 200_fp);
        dummy_data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);
        dummy_data.type_data.stats.Set(StatType::kCritChancePercentage, 100_fp);
    }

    void InitAttackAbilities() override
    {
        // Add dash omega
        {
            data.type_data.omega_abilities.abilities.clear();

            auto& ability = data.type_data.omega_abilities.AddAbility();
            ability.name = "Dash Ability";
            ability.total_duration_ms = 200;
            auto& skill1 = ability.AddSkill();
            skill1.name = "Dash Attack";
            skill1.targeting.type = SkillTargetingType::kDistanceCheck;
            skill1.targeting.group = AllegianceType::kEnemy;
            skill1.targeting.lowest = false;
            skill1.targeting.num = 1;
            skill1.deployment.type = SkillDeploymentType::kDash;
            skill1.deployment.pre_deployment_delay_percentage = 0;
            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(25_fp));
            skill1.effect_package.attributes.can_crit = true;
            skill1.effect_package.attributes.always_crit = true;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    void SetUp() override
    {
        Super::SetUp();

        // Listen to spawned dashes
        world->SubscribeToEvent(
            EventType::kDashCreated,
            [this](const Event& event)
            {
                events_dash_created.emplace_back(event.Get<event_data::DashCreated>());
            });

        // Listen to move events
        world->SubscribeToEvent(
            EventType::kMoved,
            [this](const Event& event)
            {
                const auto& data = event.Get<event_data::Moved>();
                if (EntityHelper::IsADash(*world, data.entity_id))
                {
                    events_dash_moved.emplace_back(data);
                }
            });

        // Listen to destroyed dashes
        world->SubscribeToEvent(
            EventType::kDashDestroyed,
            [this](const Event& event)
            {
                events_dash_destroyed.emplace_back(event.Get<event_data::DashDestroyed>());
            });
    }

    void SpawnCombatUnits() override
    {
        // No combat units by default
    }

public:
    CombatUnitData dummy_data;

    std::vector<event_data::DashCreated> events_dash_created;
    std::vector<event_data::DashDestroyed> events_dash_destroyed;
    std::vector<event_data::Moved> events_dash_moved;
};

TEST_F(DashSystemTest, DashApplyToAll)
{
    // Apply to all
    data.type_data.omega_abilities.abilities[0]->skills[0]->dash.apply_to_all = true;

    constexpr int r = 3;
    SpawnCombatUnit(Team::kRed, {0, r}, data, red_entity);

    Entity* blue_entity2 = nullptr;
    Entity* blue_entity3 = nullptr;
    Entity* blue_entity4 = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, r}, dummy_data, blue_entity);
    SpawnCombatUnit(Team::kBlue, {19, r}, dummy_data, blue_entity2);
    SpawnCombatUnit(Team::kBlue, {25, r}, dummy_data, blue_entity3);
    SpawnCombatUnit(Team::kBlue, {7, r + 5}, dummy_data, blue_entity4);  // out of line

    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& target_position_component = blue_entity3->Get<PositionComponent>();

    // 1. Activate omega + spawn dash
    world->TimeStep();

    // Verify dash creation
    ASSERT_EQ(events_ability_activated.size(), 1) << "One ability should be activated";
    ASSERT_EQ(events_ability_activated[0].sender_id, red_entity->GetID()) << "The ability activation by red";
    ASSERT_EQ(events_dash_created.size(), 1) << "One dash should be created";
    ASSERT_EQ(events_dash_created[0].sender_id, red_entity->GetID()) << "The dash should be created by red";
    ASSERT_TRUE(world->HasEntity(events_dash_created[0].entity_id));

    ASSERT_EQ(events_dash_moved.size(), 0) << "Not moved at creation first step";

    // 2. Reach target position, initiate dash destruction
    world->TimeStep();

    // Validate dash movement
    ASSERT_EQ(events_dash_moved.size(), 1) << "One dash should move";
    ASSERT_EQ(events_dash_moved[0].position, HexGridPosition(15, 3)) << "Dash should be in immediate position";
    events_dash_moved.clear();

    ASSERT_EQ(events_effect_package_received.size(), 1) << "one entity should be damaged";
    ASSERT_EQ(events_effect_package_received[0].receiver_id, blue_entity->GetID());
    events_effect_package_received.clear();

    // 3. Reach target position, initiate dash destruction
    world->TimeStep();

    // Validate dash movement
    ASSERT_EQ(events_dash_moved.size(), 1) << "One dash should be moved";
    const auto target_position =
        target_position_component.GetPosition() +
        HexGridPosition(target_position_component.GetRadius() + red_position_component.GetRadius() + 1, 0);
    ASSERT_EQ(events_dash_moved[0].position, target_position) << "Dash should move behind the target entity";
    ASSERT_EQ(red_position_component.GetPosition(), target_position)
        << "Dash should move sender to the target position";

    // Validate dash destruction event
    ASSERT_EQ(events_dash_destroyed.size(), 1) << "One dash should be destroyed";
    ASSERT_TRUE(world->HasEntity(events_dash_destroyed[0].entity_id)) << "Entity is still in a world for one tick";

    EXPECT_EQ(events_effect_package_received.size(), 2) << "Last targets should be damaged";
    ASSERT_EQ(events_effect_package_received[0].receiver_id, blue_entity2->GetID());
    ASSERT_EQ(events_effect_package_received[1].receiver_id, blue_entity3->GetID());
    EXPECT_EQ(events_ability_deactivated.size(), 4) << "All abilities should be ended";

    // 4. Check destruction of dash
    world->TimeStep();
    ASSERT_FALSE(world->HasEntity(events_dash_destroyed[0].entity_id));
}

TEST_F(DashSystemTest, DashApplyToTargetOnly)
{
    // Do not apply to all
    data.type_data.omega_abilities.abilities[0]->skills[0]->dash.apply_to_all = false;

    int r = 3;
    SpawnCombatUnit(Team::kRed, {0, r}, data, red_entity);

    Entity* blue_entity2 = nullptr;
    Entity* blue_entity3 = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, r}, dummy_data, blue_entity);
    SpawnCombatUnit(Team::kBlue, {19, r}, dummy_data, blue_entity2);
    SpawnCombatUnit(Team::kBlue, {25, r}, dummy_data, blue_entity3);

    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& target_position_component = blue_entity3->Get<PositionComponent>();

    // 1. Activate omega + spawn dash
    world->TimeStep();

    // Verify dash creation
    ASSERT_EQ(events_ability_activated.size(), 1) << "One ability should be activated";
    ASSERT_EQ(events_ability_activated[0].sender_id, red_entity->GetID()) << "The ability activation by red";
    ASSERT_EQ(events_dash_created.size(), 1) << "One dash should be created";
    ASSERT_EQ(events_dash_created[0].sender_id, red_entity->GetID()) << "The dash should be created by red";
    ASSERT_TRUE(world->HasEntity(events_dash_created[0].entity_id));

    ASSERT_EQ(events_dash_moved.size(), 0) << "Not moved at creation first step";

    // 2. Reach target position, initiate dash destruction
    world->TimeStep();

    // Validate dash movement
    ASSERT_EQ(events_dash_moved.size(), 1) << "One dash should move";
    ASSERT_EQ(events_dash_moved[0].position, HexGridPosition(15, r)) << "Dash should be in immediate position";
    events_dash_moved.clear();

    ASSERT_EQ(events_effect_package_received.size(), 0) << "No entities should be damaged at this timestep";

    // 3. Reach target position, initiate dash destruction
    world->TimeStep();

    // Validate dash movement
    ASSERT_EQ(events_dash_moved.size(), 1) << "One dash should be created";
    const auto target_position =
        target_position_component.GetPosition() +
        HexGridPosition(target_position_component.GetRadius() + red_position_component.GetRadius() + 1, 0);
    ASSERT_EQ(events_dash_moved[0].position, target_position) << "Dash should move behind the target entity";
    ASSERT_EQ(red_position_component.GetPosition(), target_position)
        << "Dash should move sender to the target position";

    // Validate dash destruction event
    ASSERT_EQ(events_dash_destroyed.size(), 1) << "One dash should be destroyed";
    ASSERT_TRUE(world->HasEntity(events_dash_destroyed[0].entity_id)) << "Entity is still in a world for one tick";

    ASSERT_EQ(events_effect_package_received.size(), 1) << "One enemy should be damaged";
    ASSERT_EQ(events_effect_package_received[0].receiver_id, blue_entity3->GetID()) << "Third entity should be damaged";

    ASSERT_EQ(events_ability_deactivated.size(), 2) << "All abilities should be ended";
}

TEST_F(DashSystemTest, DashLandBehindDisabled)
{
    // Apply to all
    data.type_data.omega_abilities.abilities[0]->skills[0]->dash.land_behind = false;

    constexpr int r = 3;
    SpawnCombatUnit(Team::kRed, {0, r}, data, red_entity);

    Entity* blue_entity2 = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, r}, dummy_data, blue_entity);
    SpawnCombatUnit(Team::kBlue, {25, r}, dummy_data, blue_entity2);

    // 1. Activate omega + spawn dash
    world->TimeStep();

    // Verify dash creation
    ASSERT_EQ(events_ability_activated.size(), 1) << "One ability should be activated";
    ASSERT_EQ(events_ability_activated[0].sender_id, red_entity->GetID()) << "The ability activation by red";
    ASSERT_EQ(events_dash_created.size(), 1) << "One dash should be created";
    ASSERT_EQ(events_dash_created[0].sender_id, red_entity->GetID()) << "The dash should be created by red";
    ASSERT_TRUE(world->HasEntity(events_dash_created[0].entity_id));

    ASSERT_EQ(events_dash_moved.size(), 0) << "Not moved at creation first step";

    // 2. Reach target position, initiate dash destruction
    world->TimeStep();

    // Validate dash movement
    ASSERT_EQ(events_dash_moved.size(), 1) << "One dash should move";
    ASSERT_EQ(events_dash_moved[0].position, HexGridPosition(13, 1)) << "Dash should be in immediate position";
    events_dash_moved.clear();

    ASSERT_EQ(events_effect_package_received.size(), 1) << "One entity should be damaged";
    ASSERT_EQ(events_effect_package_received[0].receiver_id, blue_entity->GetID());
    events_effect_package_received.clear();

    // 3. Reach target position, initiate dash destruction
    world->TimeStep();

    // Validate dash movement
    ASSERT_EQ(events_dash_moved.size(), 1) << "One dash should be created";
    ASSERT_EQ(events_dash_moved[0].position, HexGridPosition(25, -1)) << "Dash should move behind the target entity";

    // Validate dash destruction event
    ASSERT_EQ(events_dash_destroyed.size(), 1) << "One dash should be destroyed";
    ASSERT_TRUE(world->HasEntity(events_dash_destroyed[0].entity_id)) << "Entity is still in a world for one tick";

    ASSERT_EQ(events_effect_package_received.size(), 1) << "Last target should be damaged";
    ASSERT_EQ(events_ability_deactivated.size(), 3) << "All abilities should be ended";

    // 4. Check destruction of dash
    world->TimeStep();
    ASSERT_FALSE(world->HasEntity(events_dash_destroyed[0].entity_id));
}

}  // namespace simulation
