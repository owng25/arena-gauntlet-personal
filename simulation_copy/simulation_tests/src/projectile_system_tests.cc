#include "ability_system_data_fixtures.h"
#include "components/focus_component.h"
#include "components/position_component.h"
#include "components/projectile_component.h"
#include "components/stats_component.h"
#include "ecs/event.h"
#include "ecs/event_types_data.h"
#include "gtest/gtest.h"
#include "systems/ability_system.h"
#include "utility/attached_effects_helper.h"

namespace simulation
{
// Variant where we attack with projectile ranged attacks
class ProjectileSystemTestWithStartHomingProjectile : public AbilitySystemTest
{
    typedef AbilitySystemTest Super;

protected:
    bool UseInstantTimers() const override
    {
        return false;
    }

    void InitStats() override
    {
        data = CreateCombatUnitData();

        // Stats
        data.radius_units = 2;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 50_fp);
        data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data.type_data.stats.Set(StatType::kMaxHealth, 200_fp);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);
    }

    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Projectile Ability";
            // Timers are instant 0

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.SetDefaults(AbilityType::kAttack);
            skill1.name = "Projectile Attack";
            skill1.targeting.type = SkillTargetingType::kCurrentFocus;
            skill1.deployment.type = SkillDeploymentType::kProjectile;
            skill1.deployment.guidance = DeploymentGuidance();

            // Set the projectile data
            skill1.projectile.size_units = ProjectileSize();
            skill1.projectile.speed_sub_units = 3000;
            skill1.projectile.is_homing = IsHoming();
            skill1.projectile.apply_to_all = ApplyToAll();
            skill1.projectile.is_blockable = IsBlockable();

            // More than the shield so we affect the health too
            skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));

            ability.total_duration_ms = UseInstantTimers() ? 0 : 1000;
            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    virtual int ProjectileSize() const
    {
        return 0;
    }
    virtual bool IsHoming() const
    {
        return true;
    }
    virtual bool IsBlockable() const
    {
        return false;
    }
    virtual bool ApplyToAll() const
    {
        return false;
    }
    virtual EnumSet<GuidanceType> DeploymentGuidance() const
    {
        return MakeEnumSet(GuidanceType::kGround, GuidanceType::kUnderground, GuidanceType::kAirborne);
    }

    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {0, -3}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {0, 12}, data, red_entity);
    }

    // Projectile events fire test implementation
    void ProjectileEventsFire()
    {
        auto& blue_stats_component = blue_entity->Get<StatsComponent>();
        auto& red_stats_component = red_entity->Get<StatsComponent>();

        // Listen to spawned projectiles
        std::vector<event_data::ProjectileCreated> events_projectile_created;
        world->SubscribeToEvent(
            EventType::kProjectileCreated,
            [&events_projectile_created](const Event& event)
            {
                events_projectile_created.emplace_back(event.Get<event_data::ProjectileCreated>());
            });

        // Listen to spawned projectiles
        std::vector<event_data::Moved> events_projectiles_moved;
        world->SubscribeToEvent(
            EventType::kMoved,
            [&events_projectiles_moved, this](const Event& event)
            {
                const auto& data = event.Get<event_data::Moved>();
                if (EntityHelper::IsAProjectile(*world, data.entity_id))
                {
                    events_projectiles_moved.emplace_back(data);
                }
            });

        // Listen to energy changed
        std::vector<event_data::StatChanged> events_energy_changed;
        world->SubscribeToEvent(
            EventType::kEnergyChanged,
            [&events_energy_changed](const Event& event)
            {
                events_energy_changed.emplace_back(event.Get<event_data::StatChanged>());
            });

        // Deploy projectile
        ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

        ASSERT_EQ(2, events_energy_changed.size())
            << "Energy changed events should have fired for 2 attack abilities (projectiles)";
        events_energy_changed.clear();

        // Check events contains what is needed
        ASSERT_EQ(events_projectile_created.size(), 2) << "Projectiles were not created";
        ASSERT_EQ(events_projectile_created[0].sender_id, blue_entity->GetID())
            << "First projectile did not originate from blue";
        ASSERT_EQ(events_projectile_created[1].sender_id, red_entity->GetID())
            << "Second projectile did not originate from red";
        ASSERT_EQ(events_projectile_created[0].receiver_id, red_entity->GetID())
            << "First projectile target from blue is not the red";
        ASSERT_EQ(events_projectile_created[1].receiver_id, blue_entity->GetID())
            << "Second projectile target from red is not blue";
        ASSERT_TRUE(world->HasEntity(events_projectile_created[0].entity_id));
        ASSERT_TRUE(world->HasEntity(events_projectile_created[1].entity_id));

        // Moving projectile to target
        ASSERT_EQ(0, events_projectiles_moved.size()) << "No projectile should have moved";

        const EntityID first_projectile_sender_blue_id = events_projectile_created[0].entity_id;
        const EntityID first_projectile_sender_red_id = events_projectile_created[1].entity_id;
        events_projectile_created.clear();

        auto& projectile_sender_blue = world->GetByID(first_projectile_sender_blue_id);
        auto& projectile_blue_component = projectile_sender_blue.Get<ProjectileComponent>();
        auto& projectile_blue_focus = projectile_sender_blue.Get<FocusComponent>();

        auto& projectile_sender_red = world->GetByID(first_projectile_sender_red_id);
        auto& projectile_red_component = projectile_sender_red.Get<ProjectileComponent>();
        auto& projectile_red_focus = projectile_sender_red.Get<FocusComponent>();

        ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
            << "Projectile from blue does not have the target set to the red";
        ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
            << "Projectile from red does not have the target set to the blue";

        // Simulate other combat units moved near our entities
        // Entity* fake_red_entity = nullptr;
        // SpawnCombatUnit(Team::kRed, {3, 5}, data, fake_red_entity);
        // Entity* fake_blue_entity = nullptr;
        // SpawnCombatUnit(Team::kBlue, {10, 12}, data, fake_blue_entity);

        ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
            << "Projectile from blue does not have the target set to the red";
        ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
            << "Projectile from red does not have the target set to the blue";

        world->TimeStep();
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();
        world->TimeStep();
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();
        world->TimeStep();
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();

        // Listen to health changed
        std::vector<event_data::StatChanged> events_health_changed;
        world->SubscribeToEvent(
            EventType::kHealthChanged,
            [&events_health_changed](const Event& event)
            {
                events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
            });

        // Listen to destroyed projectiles
        std::vector<event_data::ProjectileDestroyed> events_projectile_destroyed;
        world->SubscribeToEvent(
            EventType::kProjectileDestroyed,
            [&events_projectile_destroyed](const Event& event)
            {
                events_projectile_destroyed.emplace_back(event.Get<event_data::ProjectileDestroyed>());
            });

        // Reached target
        if (ProjectileSize() < 2)
        {
            world->TimeStep();
        }

        // Start abilities sequence for the projectiles
        events_energy_changed.clear();
        world->TimeStep();  // 11, kAttackAbility, kExecuting, kFinished
        ASSERT_EQ(2, events_effect_package_received.size()) << "Projectiles should have hit targets";

        // Energy should have only applied to target. 2 events from each attack abilities,
        // pre-mitigated damages, and stat modified events.
        ASSERT_EQ(2, events_energy_changed.size()) << "Energy changed should have only fired from 2 "
                                                      "premitigated damage got from the projetiles";

        // Check if projectiles hit the right target
        ASSERT_EQ(projectile_sender_blue.GetID(), events_effect_package_received.at(0).sender_id)
            << "First hit event did not originate from our blue projectile";
        ASSERT_EQ(projectile_blue_component.GetReceiverID(), events_effect_package_received.at(0).receiver_id)
            << "Target for blue hit event does not match entity projectile component data";
        ASSERT_EQ(red_entity->GetID(), events_effect_package_received.at(0).receiver_id)
            << "Blue projectile did not hit red";

        ASSERT_EQ(projectile_sender_red.GetID(), events_effect_package_received.at(1).sender_id)
            << "First hit event did not originate from our red projectile";
        ASSERT_EQ(projectile_red_component.GetReceiverID(), events_effect_package_received.at(1).receiver_id)
            << "Target for red hit event does not match entity projectile component data";
        ASSERT_EQ(blue_entity->GetID(), events_effect_package_received.at(1).receiver_id)
            << "Red projectile did not hit blue";

        // Damage should have been done
        ASSERT_EQ(2, events_health_changed.size()) << "Health should have changed on blue and red";

        // Should have hit red
        ASSERT_EQ(red_entity->GetID(), events_health_changed.at(0).entity_id) << "Red health did not change";
        ASSERT_LT(red_stats_component.GetCurrentHealth(), data.type_data.stats.Get(StatType::kMaxHealth))
            << "Health should have changed on red";
        ASSERT_LT(events_health_changed.at(0).delta, 0_fp) << "Health should have changed on red";

        // Should have hit blue
        ASSERT_EQ(blue_entity->GetID(), events_health_changed.at(1).entity_id) << "Blue health did not change";
        ASSERT_LT(blue_stats_component.GetCurrentHealth(), data.type_data.stats.Get(StatType::kMaxHealth))
            << "Health should have changed on blue";
        ASSERT_LT(events_health_changed.at(1).delta, 0_fp) << "Health should have changed on blue";

        // Check if right projectiles events fired
        ASSERT_EQ(events_projectile_destroyed.size(), 2);
        ASSERT_EQ(events_projectile_destroyed[0].entity_id, first_projectile_sender_blue_id);
        ASSERT_EQ(events_projectile_destroyed[1].entity_id, first_projectile_sender_red_id);

        // 12. Projectiles should be destroyed
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_blue_id));
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_red_id));
        world->TimeStep();

        ASSERT_FALSE(world->HasEntity(first_projectile_sender_blue_id))
            << "Projectile from blue should have been destroyed";
        ASSERT_FALSE(world->HasEntity(first_projectile_sender_red_id))
            << "Projectile from red should have been destroyed";
    }

    // Projectile events fire test implementation
    void ProjectileEventsFireDodge()
    {
        // Listen to spawned projectiles
        std::vector<event_data::ProjectileCreated> events_projectile_created;
        world->SubscribeToEvent(
            EventType::kProjectileCreated,
            [&events_projectile_created](const Event& event)
            {
                events_projectile_created.emplace_back(event.Get<event_data::ProjectileCreated>());
            });

        // Listen to spawned projectiles
        std::vector<event_data::Moved> events_projectiles_moved;
        world->SubscribeToEvent(
            EventType::kMoved,
            [&events_projectiles_moved, this](const Event& event)
            {
                const auto& data = event.Get<event_data::Moved>();
                if (EntityHelper::IsAProjectile(*world, data.entity_id))
                {
                    events_projectiles_moved.emplace_back(data);
                }
            });

        // Deploy projectile
        ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

        // Check events contains what is needed
        ASSERT_EQ(events_projectile_created.size(), 2) << "Projectiles were not created";
        ASSERT_EQ(events_projectile_created[0].sender_id, blue_entity->GetID())
            << "First projectile did not originate from blue";
        ASSERT_EQ(events_projectile_created[1].sender_id, red_entity->GetID())
            << "Second projectile did not originate from red";
        ASSERT_EQ(events_projectile_created[0].receiver_id, red_entity->GetID())
            << "First projectile target from blue is not the red";
        ASSERT_EQ(events_projectile_created[1].receiver_id, blue_entity->GetID())
            << "Second projectile target from red is not blue";
        ASSERT_TRUE(world->HasEntity(events_projectile_created[0].entity_id));
        ASSERT_TRUE(world->HasEntity(events_projectile_created[1].entity_id));

        // Moving projectile to target
        ASSERT_EQ(0, events_projectiles_moved.size()) << "No projectile should have moved";

        const EntityID first_projectile_sender_blue_id = events_projectile_created[0].entity_id;
        const EntityID first_projectile_sender_red_id = events_projectile_created[1].entity_id;
        events_projectile_created.clear();

        auto& projectile_sender_blue = world->GetByID(first_projectile_sender_blue_id);
        auto& projectile_blue_focus = projectile_sender_blue.Get<FocusComponent>();

        auto& projectile_sender_red = world->GetByID(first_projectile_sender_red_id);
        auto& projectile_red_focus = projectile_sender_red.Get<FocusComponent>();

        ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
            << "Projectile from blue does not have the target set to the red";
        ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
            << "Projectile from red does not have the target set to the blue";

        ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
            << "Projectile from blue does not have the target set to the red";
        ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
            << "Projectile from red does not have the target set to the blue";

        world->TimeStep();
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();
        world->TimeStep();
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();
        world->TimeStep();
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();

        // Listen to health changed
        std::vector<event_data::StatChanged> events_health_changed;
        world->SubscribeToEvent(
            EventType::kHealthChanged,
            [&events_health_changed](const Event& event)
            {
                events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
            });

        // Listen to destroyed projectiles
        std::vector<event_data::ProjectileDestroyed> events_projectile_destroyed;
        world->SubscribeToEvent(
            EventType::kProjectileDestroyed,
            [&events_projectile_destroyed](const Event& event)
            {
                events_projectile_destroyed.emplace_back(event.Get<event_data::ProjectileDestroyed>());
            });

        // Listen to destroyed projectiles
        std::vector<event_data::EffectPackage> events_projectile_dodged;
        world->SubscribeToEvent(
            EventType::kEffectPackageDodged,
            [&events_projectile_dodged](const Event& event)
            {
                events_projectile_dodged.emplace_back(event.Get<event_data::EffectPackage>());
            });

        // Reached target
        if (ProjectileSize() < 2)
        {
            world->TimeStep();
        }

        // Start abilities sequence for the projectiles
        events_projectile_dodged.clear();
        world->TimeStep();  // 11, kAttackAbility, kExecuting, kFinished
        ASSERT_EQ(0, events_effect_package_received.size()) << "Projectiles should not hit targets";

        // Dodged red and blue
        ASSERT_EQ(2, events_projectile_dodged.size()) << "blue entity should have dodged because of max dodge chance";
        ASSERT_EQ(red_entity->GetID(), events_projectile_dodged.at(0).receiver_id) << "red entity should have dodged";
        ASSERT_EQ(blue_entity->GetID(), events_projectile_dodged.at(1).receiver_id) << "blue entity should have dodged";

        // Damage shouldn't have been done
        ASSERT_EQ(0, events_health_changed.size()) << "Health should not have changed on blue and red";

        // Check if right projectiles events fired
        ASSERT_EQ(events_projectile_destroyed.size(), 2);
        ASSERT_EQ(events_projectile_destroyed[0].entity_id, first_projectile_sender_blue_id);
        ASSERT_EQ(events_projectile_destroyed[1].entity_id, first_projectile_sender_red_id);

        // 12. Projectiles should be destroyed
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_blue_id));
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_red_id));
        world->TimeStep();

        ASSERT_FALSE(world->HasEntity(first_projectile_sender_blue_id))
            << "Projectile from blue should have been destroyed";
        ASSERT_FALSE(world->HasEntity(first_projectile_sender_red_id))
            << "Projectile from red should have been destroyed";
    }

    void ProjectileEventsFireBlockable()
    {
        auto& blue_stats_component = blue_entity->Get<StatsComponent>();
        auto& red_stats_component = red_entity->Get<StatsComponent>();

        EXPECT_EQ(red_stats_component.GetCurrentHealth(), 200_fp);

        // Always crit
        blue_stats_component.GetMutableTemplateStats().Set(StatType::kCritChancePercentage, kMaxPercentageFP);
        red_stats_component.GetMutableTemplateStats().Set(StatType::kCritChancePercentage, kMaxPercentageFP);

        blue_stats_component.SetCritChanceOverflowPercentage(105_fp);
        red_stats_component.SetCritChanceOverflowPercentage(105_fp);

        // Listen to spawned projectiles
        std::vector<event_data::ProjectileCreated> events_projectile_created;
        world->SubscribeToEvent(
            EventType::kProjectileCreated,
            [&events_projectile_created](const Event& event)
            {
                events_projectile_created.emplace_back(event.Get<event_data::ProjectileCreated>());
            });

        // Listen to spawned projectiles
        std::vector<event_data::Moved> events_projectiles_moved;
        world->SubscribeToEvent(
            EventType::kMoved,
            [&events_projectiles_moved, this](const Event& event)
            {
                const auto& data = event.Get<event_data::Moved>();
                if (EntityHelper::IsAProjectile(*world, data.entity_id))
                {
                    events_projectiles_moved.emplace_back(data);
                }
            });

        // Deploy projectile
        ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

        // Check events contains what is needed
        ASSERT_EQ(events_projectile_created.size(), 2) << "Projectiles were not created";
        ASSERT_EQ(events_projectile_created[0].sender_id, blue_entity->GetID())
            << "First projectile did not originate from blue";
        ASSERT_EQ(events_projectile_created[1].sender_id, red_entity->GetID())
            << "Second projectile did not originate from red";
        ASSERT_EQ(events_projectile_created[0].receiver_id, red_entity->GetID())
            << "First projectile target from blue is not the red";
        ASSERT_EQ(events_projectile_created[1].receiver_id, blue_entity->GetID())
            << "Second projectile target from red is not blue";
        ASSERT_TRUE(world->HasEntity(events_projectile_created[0].entity_id));
        ASSERT_TRUE(world->HasEntity(events_projectile_created[1].entity_id));

        const EntityID first_projectile_sender_blue_id = events_projectile_created[0].entity_id;
        const EntityID first_projectile_sender_red_id = events_projectile_created[1].entity_id;
        events_projectile_created.clear();

        auto& projectile_sender_blue = world->GetByID(first_projectile_sender_blue_id);
        auto& projectile_blue_component = projectile_sender_blue.Get<ProjectileComponent>();
        auto& projectile_blue_focus = projectile_sender_blue.Get<FocusComponent>();

        auto& projectile_sender_red = world->GetByID(first_projectile_sender_red_id);
        auto& projectile_red_component = projectile_sender_red.Get<ProjectileComponent>();
        auto& projectile_red_focus = projectile_sender_red.Get<FocusComponent>();

        ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
            << "Projectile from blue does not have the target set to the red";
        ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
            << "Projectile from red does not have the target set to the blue";

        // Moving projectile to target
        world->TimeStep();
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();

        world->TimeStep();
        ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
            << "Projectile from blue does not have the target set to the red";
        ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
            << "Projectile from red does not have the target set to the blue";
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();

        world->TimeStep();
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();

        // Reached target
        if (ProjectileSize() < 2)
        {
            world->TimeStep();
        }

        // Listen to health changed
        std::vector<event_data::StatChanged> events_health_changed;
        world->SubscribeToEvent(
            EventType::kHealthChanged,
            [&events_health_changed](const Event& event)
            {
                events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
            });

        // Listen to destroyed projectiles
        std::vector<event_data::ProjectileDestroyed> events_projectile_destroyed;
        world->SubscribeToEvent(
            EventType::kProjectileDestroyed,
            [&events_projectile_destroyed](const Event& event)
            {
                events_projectile_destroyed.emplace_back(event.Get<event_data::ProjectileDestroyed>());
            });

        // Start abilities sequence for the projectiles
        world->TimeStep();
        ASSERT_EQ(2, events_effect_package_received.size()) << "Projectiles should have hit targets";

        // Check if projectiles hit the right target
        ASSERT_EQ(projectile_sender_blue.GetID(), events_effect_package_received.at(0).sender_id)
            << "First hit event did not originate from our blue projectile";
        ASSERT_EQ(projectile_blue_component.GetReceiverID(), events_effect_package_received.at(0).receiver_id)
            << "Target for blue hit event does not match entity projectile component data";
        ASSERT_EQ(red_entity->GetID(), events_effect_package_received.at(0).receiver_id)
            << "Blue projectile did not hit red";

        ASSERT_EQ(projectile_sender_red.GetID(), events_effect_package_received.at(1).sender_id)
            << "First hit event did not originate from our red projectile";
        ASSERT_EQ(projectile_red_component.GetReceiverID(), events_effect_package_received.at(1).receiver_id)
            << "Target for red hit event does not match entity projectile component data";
        ASSERT_EQ(blue_entity->GetID(), events_effect_package_received.at(1).receiver_id)
            << "Red projectile did not hit blue";

        // Damage should have been done
        ASSERT_EQ(2, events_health_changed.size()) << "Health should have changed on blue and red";

        // Should have hit red
        ASSERT_EQ(red_entity->GetID(), events_health_changed.at(0).entity_id) << "Red health did not change";
        ASSERT_LT(red_stats_component.GetCurrentHealth(), data.type_data.stats.Get(StatType::kMaxHealth))
            << "Health should have changed on red";
        ASSERT_LT(events_health_changed.at(0).delta, 0_fp) << "Health should have changed on red";

        // Should have hit blue
        ASSERT_EQ(blue_entity->GetID(), events_health_changed.at(1).entity_id) << "Blue health did not change";
        ASSERT_LT(blue_stats_component.GetCurrentHealth(), data.type_data.stats.Get(StatType::kMaxHealth))
            << "Health should have changed on blue";
        ASSERT_LT(events_health_changed.at(1).delta, 0_fp) << "Health should have changed on blue";

        // Check if right projectiles events fired
        ASSERT_EQ(events_projectile_destroyed.size(), 2);
        ASSERT_EQ(events_projectile_destroyed[0].entity_id, first_projectile_sender_blue_id);
        ASSERT_EQ(events_projectile_destroyed[1].entity_id, first_projectile_sender_red_id);

        // Projectiles should be destroyed
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_blue_id));
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_red_id));
        world->TimeStep();

        ASSERT_FALSE(world->HasEntity(first_projectile_sender_blue_id))
            << "Projectile from blue should have been destroyed";
        ASSERT_FALSE(world->HasEntity(first_projectile_sender_red_id))
            << "Projectile from red should have been destroyed";

        EXPECT_EQ(red_stats_component.GetCurrentHealth(), 50_fp);
    }

    void ProjectilesAreBlocked()
    {
        // Listen to spawned projectiles
        std::vector<event_data::ProjectileCreated> events_projectile_created;
        world->SubscribeToEvent(
            EventType::kProjectileCreated,
            [&events_projectile_created](const Event& event)
            {
                events_projectile_created.emplace_back(event.Get<event_data::ProjectileCreated>());
            });

        // Listen to spawned projectiles
        std::vector<event_data::Moved> events_projectiles_moved;
        world->SubscribeToEvent(
            EventType::kMoved,
            [&events_projectiles_moved, this](const Event& event)
            {
                const auto& data = event.Get<event_data::Moved>();
                if (EntityHelper::IsAProjectile(*world, data.entity_id))
                {
                    events_projectiles_moved.emplace_back(data);
                }
            });

        // Deploy projectile
        ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

        const EntityID first_projectile_sender_blue_id = events_projectile_created[0].entity_id;
        const EntityID first_projectile_sender_red_id = events_projectile_created[1].entity_id;
        events_projectile_created.clear();

        auto& projectile_sender_blue = world->GetByID(first_projectile_sender_blue_id);
        auto& projectile_blue_component = projectile_sender_blue.Get<ProjectileComponent>();
        auto& projectile_blue_position = projectile_sender_blue.Get<PositionComponent>();
        auto& projectile_blue_focus = projectile_sender_blue.Get<FocusComponent>();

        auto& projectile_sender_red = world->GetByID(first_projectile_sender_red_id);
        auto& projectile_red_focus = projectile_sender_red.Get<FocusComponent>();

        ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
            << "Projectile from blue does not have the target set to the red";
        ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
            << "Projectile from red does not have the target set to the blue";

        // Moving projectile to target
        world->TimeStep();
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();

        world->TimeStep();
        ASSERT_EQ(HexGridPosition(0, 3), projectile_blue_position.GetPosition())
            << "Projectile is not in the right position";
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
            << "Projectile from blue does not have the target set to the red";
        ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
            << "Projectile from red does not have the target set to the blue";
        events_projectiles_moved.clear();

        // Simulate other combat units moved near our entities
        // Should be blocked
        Entity* fake_red_entity = nullptr;
        SpawnCombatUnit(Team::kRed, {0, 7}, data, fake_red_entity);
        const auto& fake_red_stats_component = fake_red_entity->Get<StatsComponent>();

        // Listen to health changed
        std::vector<event_data::StatChanged> events_health_changed;
        world->SubscribeToEvent(
            EventType::kHealthChanged,
            [&events_health_changed](const Event& event)
            {
                events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
            });

        // Listen to destroyed projectiles
        std::vector<event_data::ProjectileDestroyed> events_projectile_destroyed;
        world->SubscribeToEvent(
            EventType::kProjectileDestroyed,
            [&events_projectile_destroyed](const Event& event)
            {
                events_projectile_destroyed.emplace_back(event.Get<event_data::ProjectileDestroyed>());
            });

        // Projectile should have been blocked by our fake_red_entity
        world->TimeStep();
        // Projectiles should have still called the moved event
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectile should have moved";
        events_projectiles_moved.clear();

        // Check if right projectiles events fired
        ASSERT_EQ(events_projectile_destroyed.size(), 1) << "Only the blue projectile should have destroyed";
        ASSERT_EQ(events_projectile_destroyed[0].entity_id, first_projectile_sender_blue_id)
            << "Blue projectile is not destroyed";
        ASSERT_EQ(1, events_effect_package_received.size())
            << "Projectile should have been blocked by our fake red targets";

        // Check if projectiles hit the right target
        ASSERT_EQ(projectile_sender_blue.GetID(), events_effect_package_received.at(0).sender_id)
            << "First hit event did not originate from our blue projectile";
        ASSERT_NE(projectile_blue_component.GetReceiverID(), events_effect_package_received.at(0).receiver_id)
            << "Target for blue hit event SHOULD not match entity projectile component data as "
               "the "
               "projectile was blocked by something else other than our target";
        ASSERT_EQ(fake_red_entity->GetID(), events_effect_package_received.at(0).receiver_id)
            << "Blue projectile did not hit faker red entity";

        // Damage should have been done
        ASSERT_EQ(1, events_health_changed.size()) << "Health should have changed on fake red entity";

        // Should have hit fake_red_entity
        ASSERT_EQ(fake_red_entity->GetID(), events_health_changed.at(0).entity_id) << "Fake Red health did not change";
        ASSERT_LT(fake_red_stats_component.GetCurrentHealth(), data.type_data.stats.Get(StatType::kMaxHealth))
            << "Health should have changed on fake red";
        ASSERT_LT(events_health_changed.at(0).delta, 0_fp) << "Health should have changed on fake red";

        // Projectiles should be destroyed from next time step
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_blue_id));
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_red_id));
        world->TimeStep();
        ASSERT_FALSE(world->HasEntity(first_projectile_sender_blue_id))
            << "Projectile from blue should have been destroyed";
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_red_id))
            << "Projectile from red should have NOT been destroyed";
    }

    void ProjectileReachingTargetIfTargetIsDead(bool bSecondProjectileOnStep9 = false)
    {
        auto& blue_stats_component = blue_entity->Get<StatsComponent>();

        // Listen to spawned projectiles
        std::vector<event_data::ProjectileCreated> events_projectile_created;
        world->SubscribeToEvent(
            EventType::kProjectileCreated,
            [&events_projectile_created](const Event& event)
            {
                events_projectile_created.emplace_back(event.Get<event_data::ProjectileCreated>());
            });

        // Listen to spawned projectiles
        std::vector<event_data::Moved> events_projectiles_moved;
        world->SubscribeToEvent(
            EventType::kMoved,
            [&events_projectiles_moved, this](const Event& event)
            {
                const auto& data = event.Get<event_data::Moved>();
                if (EntityHelper::IsAProjectile(*world, data.entity_id))
                {
                    events_projectiles_moved.emplace_back(data);
                }
            });

        // Listen to destroyed projectiles
        std::vector<event_data::ProjectileDestroyed> events_projectile_destroyed;
        world->SubscribeToEvent(
            EventType::kProjectileDestroyed,
            [&events_projectile_destroyed](const Event& event)
            {
                events_projectile_destroyed.emplace_back(event.Get<event_data::ProjectileDestroyed>());
            });

        // Listen to health changed
        std::vector<event_data::StatChanged> events_health_changed;
        world->SubscribeToEvent(
            EventType::kHealthChanged,
            [&events_health_changed](const Event& event)
            {
                events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
            });

        // Deploy projectile
        ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

        // Check created projectiles
        ASSERT_EQ(events_projectile_created.size(), 2) << "Projectiles were not created";
        ASSERT_EQ(events_projectile_created[0].sender_id, blue_entity->GetID())
            << "First projectile did not originate from blue";
        ASSERT_EQ(events_projectile_created[1].sender_id, red_entity->GetID())
            << "Second projectile did not originate from red";
        ASSERT_EQ(events_projectile_created[0].receiver_id, red_entity->GetID())
            << "First projectile target from blue is not the red";
        ASSERT_EQ(events_projectile_created[1].receiver_id, blue_entity->GetID())
            << "Second projectile target from red is not blue";

        // Get created projectiles IDs and validate on world
        const EntityID first_projectile_blue_id = events_projectile_created[0].entity_id;
        const EntityID first_projectile_red_id = events_projectile_created[1].entity_id;
        ASSERT_TRUE(world->HasEntity(first_projectile_blue_id));
        ASSERT_TRUE(world->HasEntity(first_projectile_red_id));

        const auto& projectile_blue = world->GetByID(first_projectile_blue_id);
        const auto& projectile_red = world->GetByID(first_projectile_red_id);
        const auto& projectile_blue_focus = projectile_blue.Get<FocusComponent>();
        const auto& projectile_red_focus = projectile_red.Get<FocusComponent>();
        const auto& projectile_blue_component = projectile_blue.Get<ProjectileComponent>();

        events_projectile_created.clear();

        // Check the target focuses
        ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
            << "Projectile from blue does not have the target set to the red";
        ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
            << "Projectile from red does not have the target set to the blue";

        // Simulate target is dead
        red_entity->Deactivate();

        // Should not destroy projectile from blue
        ASSERT_TRUE(world->HasEntity(first_projectile_blue_id));
        ASSERT_TRUE(world->HasEntity(first_projectile_red_id));

        // Moving projectile to target
        world->TimeStep();
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();

        // Blue projectile becoming Not-Homing because target is dead.
        world->TimeStep();

        // Projectile still on world
        ASSERT_TRUE(world->HasEntity(first_projectile_blue_id));
        ASSERT_TRUE(world->HasEntity(first_projectile_red_id));

        // Check for Homing projectile
        ASSERT_FALSE(projectile_blue_component.IsHoming());

        ASSERT_EQ(events_projectile_destroyed.size(), 0) << "Projectile from blue should not have been destroyed";

        // Moving projectile to target
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();

        if (ProjectileSize() < 2)
        {
            world->TimeStep();

            ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
            events_projectiles_moved.clear();

            ASSERT_TRUE(world->HasEntity(first_projectile_blue_id));
            ASSERT_TRUE(world->HasEntity(first_projectile_red_id));
        }

        world->TimeStep();

        ASSERT_TRUE(world->HasEntity(first_projectile_blue_id));
        ASSERT_TRUE(world->HasEntity(first_projectile_red_id));

        // Moving projectile to target
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();

        // Damage blue team and destroy red projectile
        // Blue projectile should reach the deactivated target position and get destroyed.
        world->TimeStep();

        // Damage should have been done to blue team cause red entity is deactivated
        ASSERT_EQ(events_health_changed.size(), 1) << "Health should have changed on blue";

        // Should have hit blue
        ASSERT_EQ(blue_entity->GetID(), events_health_changed.at(0).entity_id) << "Blue health did not change";
        ASSERT_LT(blue_stats_component.GetCurrentHealth(), data.type_data.stats.Get(StatType::kMaxHealth))
            << "Health should have changed on blue";
        ASSERT_LT(events_health_changed.at(0).delta, 0_fp) << "Health should have changed on blue";
        events_health_changed.clear();

        ASSERT_TRUE(world->HasEntity(first_projectile_blue_id));
        ASSERT_TRUE(world->HasEntity(first_projectile_red_id));

        if (bSecondProjectileOnStep9)
        {
            // Red projectile should have destroyed
            ASSERT_EQ(events_projectile_destroyed.size(), 1) << "Projectile from red should have been destroyed";
            events_projectile_destroyed.clear();
        }
        else
        {
            // Blue and red projectile should have destroyed
            ASSERT_EQ(events_projectile_destroyed.size(), 2)
                << "Projectile from blue and red should have been destroyed";
            events_projectile_destroyed.clear();
        }

        world->TimeStep();

        if (bSecondProjectileOnStep9)
        {
            ASSERT_FALSE(world->HasEntity(first_projectile_red_id));

            // Blue and red projectile should have destroyed
            ASSERT_EQ(events_projectile_destroyed.size(), 1)
                << "Projectile from blue and red should have been destroyed";
            events_projectile_destroyed.clear();

            world->TimeStep();

            ASSERT_FALSE(world->HasEntity(first_projectile_blue_id));
        }
        else
        {
            ASSERT_FALSE(world->HasEntity(first_projectile_red_id));
            ASSERT_FALSE(world->HasEntity(first_projectile_blue_id));
        }
    }

    void ProjectileEventsFireApplyToAll()
    {
        auto& blue_stats_component = blue_entity->Get<StatsComponent>();
        auto& red_stats_component = red_entity->Get<StatsComponent>();

        // Listen to spawned projectiles
        std::vector<event_data::ProjectileCreated> fire_all_events_projectile_created;
        world->SubscribeToEvent(
            EventType::kProjectileCreated,
            [&fire_all_events_projectile_created](const Event& event)
            {
                fire_all_events_projectile_created.emplace_back(event.Get<event_data::ProjectileCreated>());
            });

        // Listen to spawned projectiles
        std::vector<event_data::Moved> events_projectiles_moved;
        world->SubscribeToEvent(
            EventType::kMoved,
            [&events_projectiles_moved, this](const Event& event)
            {
                const auto& data = event.Get<event_data::Moved>();
                if (EntityHelper::IsAProjectile(*world, data.entity_id))
                {
                    events_projectiles_moved.emplace_back(data);
                }
            });

        // Deploy projectile
        ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

        // Check events contains what is needed
        ASSERT_EQ(fire_all_events_projectile_created.size(), 2) << "Projectiles were not created";
        ASSERT_EQ(fire_all_events_projectile_created[0].sender_id, blue_entity->GetID())
            << "First projectile did not originate from blue";
        ASSERT_EQ(fire_all_events_projectile_created[1].sender_id, red_entity->GetID())
            << "Second projectile did not originate from red";
        ASSERT_EQ(fire_all_events_projectile_created[0].receiver_id, red_entity->GetID())
            << "First projectile target from blue is not the red";
        ASSERT_EQ(fire_all_events_projectile_created[1].receiver_id, blue_entity->GetID())
            << "Second projectile target from red is not blue";
        ASSERT_TRUE(world->HasEntity(fire_all_events_projectile_created[0].entity_id));
        ASSERT_TRUE(world->HasEntity(fire_all_events_projectile_created[1].entity_id));

        const EntityID first_projectile_sender_blue_id = fire_all_events_projectile_created[0].entity_id;
        const EntityID first_projectile_sender_red_id = fire_all_events_projectile_created[1].entity_id;
        fire_all_events_projectile_created.clear();

        auto& projectile_sender_blue = world->GetByID(first_projectile_sender_blue_id);
        auto& projectile_blue_component = projectile_sender_blue.Get<ProjectileComponent>();
        auto& projectile_blue_focus = projectile_sender_blue.Get<FocusComponent>();

        auto& projectile_sender_red = world->GetByID(first_projectile_sender_red_id);
        auto& projectile_red_component = projectile_sender_red.Get<ProjectileComponent>();
        auto& projectile_red_focus = projectile_sender_red.Get<FocusComponent>();

        ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
            << "Projectile from blue does not have the target set to the red";
        ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
            << "Projectile from red does not have the target set to the blue";

        // Moving projectile to target
        world->TimeStep();
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();

        world->TimeStep();
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
            << "Projectile from blue does not have the target set to the red";
        ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
            << "Projectile from red does not have the target set to the blue";
        events_projectiles_moved.clear();

        world->TimeStep();
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();

        // Reached target
        if (ProjectileSize() < 2)
        {
            world->TimeStep();
        }

        // Listen to health changed
        std::vector<event_data::StatChanged> events_health_changed;
        world->SubscribeToEvent(
            EventType::kHealthChanged,
            [&events_health_changed](const Event& event)
            {
                events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
            });

        // Listen to destroyed projectiles
        std::vector<event_data::ProjectileDestroyed> events_projectile_destroyed;
        world->SubscribeToEvent(
            EventType::kProjectileDestroyed,
            [&events_projectile_destroyed](const Event& event)
            {
                events_projectile_destroyed.emplace_back(event.Get<event_data::ProjectileDestroyed>());
            });

        // Start abilities sequence for the projectiles
        world->TimeStep();
        ASSERT_EQ(2, events_effect_package_received.size()) << "Projectiles should have hit targets";

        // Check if projectiles hit the right target
        ASSERT_EQ(projectile_sender_blue.GetID(), events_effect_package_received.at(0).sender_id)
            << "First hit event did not originate from our blue projectile";
        ASSERT_EQ(projectile_blue_component.GetReceiverID(), events_effect_package_received.at(0).receiver_id)
            << "Target for blue hit event does not match entity projectile component data";
        ASSERT_EQ(red_entity->GetID(), events_effect_package_received.at(0).receiver_id)
            << "Blue projectile did not hit red";

        ASSERT_EQ(projectile_sender_red.GetID(), events_effect_package_received.at(1).sender_id)
            << "First hit event did not originate from our red projectile";
        ASSERT_EQ(projectile_red_component.GetReceiverID(), events_effect_package_received.at(1).receiver_id)
            << "Target for red hit event does not match entity projectile component data";
        ASSERT_EQ(blue_entity->GetID(), events_effect_package_received.at(1).receiver_id)
            << "Red projectile did not hit blue";

        // Damage should have been done
        ASSERT_EQ(2, events_health_changed.size()) << "Health should have changed on blue and red";

        // Should have hit red
        ASSERT_EQ(red_entity->GetID(), events_health_changed.at(0).entity_id) << "Red health did not change";
        ASSERT_LT(red_stats_component.GetCurrentHealth(), data.type_data.stats.Get(StatType::kMaxHealth))
            << "Health should have changed on red";
        ASSERT_LT(events_health_changed.at(0).delta, 0_fp) << "Health should have changed on red";

        // Should have hit blue
        ASSERT_EQ(blue_entity->GetID(), events_health_changed.at(1).entity_id) << "Blue health did not change";
        ASSERT_LT(blue_stats_component.GetCurrentHealth(), data.type_data.stats.Get(StatType::kMaxHealth))
            << "Health should have changed on blue";
        ASSERT_LT(events_health_changed.at(1).delta, 0_fp) << "Health should have changed on blue";

        // Check if right projectiles events fired
        ASSERT_EQ(events_projectile_destroyed.size(), 2);
        ASSERT_EQ(events_projectile_destroyed[0].entity_id, first_projectile_sender_blue_id);
        ASSERT_EQ(events_projectile_destroyed[1].entity_id, first_projectile_sender_red_id);

        // Projectiles should be destroyed
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_blue_id));
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_red_id));
        world->TimeStep();

        ASSERT_FALSE(world->HasEntity(first_projectile_sender_blue_id))
            << "Projectile from blue should have been destroyed";
        ASSERT_FALSE(world->HasEntity(first_projectile_sender_red_id))
            << "Projectile from red should have been destroyed";
    }

    void ProjectilesHitEveryone()
    {
        auto& blue_stats_component = blue_entity->Get<StatsComponent>();
        auto& red_stats_component = red_entity->Get<StatsComponent>();
        // Listen to spawned projectiles
        std::vector<event_data::ProjectileCreated> events_projectile_created;
        world->SubscribeToEvent(
            EventType::kProjectileCreated,
            [&events_projectile_created](const Event& event)
            {
                events_projectile_created.emplace_back(event.Get<event_data::ProjectileCreated>());
            });

        // Listen to spawned projectiles
        std::vector<event_data::Moved> events_projectiles_moved;
        world->SubscribeToEvent(
            EventType::kMoved,
            [&events_projectiles_moved, this](const Event& event)
            {
                const auto& data = event.Get<event_data::Moved>();
                if (EntityHelper::IsAProjectile(*world, data.entity_id))
                {
                    events_projectiles_moved.emplace_back(data);
                }
            });

        // Deploy projectile
        ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

        const EntityID first_projectile_sender_blue_id = events_projectile_created[0].entity_id;
        const EntityID first_projectile_sender_red_id = events_projectile_created[1].entity_id;
        events_projectile_created.clear();

        auto& projectile_sender_blue = world->GetByID(first_projectile_sender_blue_id);
        auto& projectile_blue_component = projectile_sender_blue.Get<ProjectileComponent>();
        auto& projectile_blue_position = projectile_sender_blue.Get<PositionComponent>();
        auto& projectile_blue_focus = projectile_sender_blue.Get<FocusComponent>();

        auto& projectile_sender_red = world->GetByID(first_projectile_sender_red_id);
        auto& projectile_red_component = projectile_sender_red.Get<ProjectileComponent>();
        auto& projectile_red_focus = projectile_sender_red.Get<FocusComponent>();

        ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
            << "Projectile from blue does not have the target set to the red";
        ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
            << "Projectile from red does not have the target set to the blue";

        // Moving projectile to target
        ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
            << "Projectile from blue does not have the target set to the red";
        ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
            << "Projectile from red does not have the target set to the blue";

        world->TimeStep();
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        events_projectiles_moved.clear();

        world->TimeStep();
        ASSERT_EQ(HexGridPosition(0, 3), projectile_blue_position.GetPosition())
            << "Projectile is not in the right position";
        ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
        ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
            << "Projectile from blue does not have the target set to the red";
        ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
            << "Projectile from red does not have the target set to the blue";
        events_projectiles_moved.clear();

        // Simulate other combat units moved near our entities
        // Should also be hit
        Entity* fake_red_entity = nullptr;
        SpawnCombatUnit(Team::kRed, {1, 7}, data, fake_red_entity);
        const auto& fake_red_stats_component = fake_red_entity->Get<StatsComponent>();

        // Listen to health changed
        std::vector<event_data::StatChanged> events_health_changed;
        world->SubscribeToEvent(
            EventType::kHealthChanged,
            [&events_health_changed](const Event& event)
            {
                events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
            });

        // Listen to destroyed projectiles
        std::vector<event_data::ProjectileDestroyed> events_projectile_destroyed;
        world->SubscribeToEvent(
            EventType::kProjectileDestroyed,
            [&events_projectile_destroyed](const Event& event)
            {
                events_projectile_destroyed.emplace_back(event.Get<event_data::ProjectileDestroyed>());
            });

        // Projectile should have hit our fake_red_entity
        world->TimeStep();

        // Projectiles should have still called the moved event
        EXPECT_EQ(2, events_projectiles_moved.size()) << "2 projectile should have moved";
        events_projectiles_moved.clear();

        // Check if right projectiles events fired
        EXPECT_EQ(events_projectile_destroyed.size(), 0)
            << "Projectiles should have not yet been destroyed because apply_all is enabled";
        ASSERT_EQ(1, events_effect_package_received.size()) << "Projectile should have hit our fake red targets";

        // Check if projectiles hit the right target
        ASSERT_EQ(projectile_sender_blue.GetID(), events_effect_package_received.at(0).sender_id)
            << "First hit event did not originate from our blue projectile";
        ASSERT_NE(projectile_blue_component.GetReceiverID(), events_effect_package_received.at(0).receiver_id)
            << "Target for blue hit event SHOULD not match entity projectile component data as "
               "the "
               "projectile was blocked by something else other than our target";
        ASSERT_EQ(fake_red_entity->GetID(), events_effect_package_received.at(0).receiver_id)
            << "Blue projectile did not hit faker red entity";

        // Damage should have been done
        ASSERT_EQ(1, events_health_changed.size()) << "Health should have changed on fake red entity";

        // Should have hit fake_red_entity
        ASSERT_EQ(fake_red_entity->GetID(), events_health_changed.at(0).entity_id) << "Fake Red health did not change";
        ASSERT_LT(fake_red_stats_component.GetCurrentHealth(), data.type_data.stats.Get(StatType::kMaxHealth))
            << "Health should have changed on fake red";
        ASSERT_LT(events_health_changed.at(0).delta, 0_fp) << "Health should have changed on fake red";

        // Projectiles should NOT BE destroyed from next time step
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_blue_id))
            << "first_projectile_sender_blue_id = " << first_projectile_sender_blue_id;
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_red_id))
            << "first_projectile_sender_red_id = " << first_projectile_sender_red_id;
        world->TimeStep();
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_blue_id))
            << "first_projectile_sender_blue_id = " << first_projectile_sender_blue_id;
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_red_id))
            << "first_projectile_sender_red_id = " << first_projectile_sender_red_id;

        //
        // Should have hit normal targets
        //
        events_effect_package_received.clear();
        events_health_changed.clear();
        events_projectile_destroyed.clear();

        // Start abilities sequence for the projectiles
        world->TimeStep();
        ASSERT_EQ(2, events_effect_package_received.size()) << "Projectiles should have hit targets";

        // Check if projectiles hit the right target
        ASSERT_EQ(projectile_sender_blue.GetID(), events_effect_package_received.at(0).sender_id)
            << "First hit event did not originate from our blue projectile";
        ASSERT_EQ(projectile_blue_component.GetReceiverID(), events_effect_package_received.at(0).receiver_id)
            << "Target for blue hit event does not match entity projectile component data";
        ASSERT_EQ(red_entity->GetID(), events_effect_package_received.at(0).receiver_id)
            << "Blue projectile did not hit red";

        ASSERT_EQ(projectile_sender_red.GetID(), events_effect_package_received.at(1).sender_id)
            << "First hit event did not originate from our red projectile";
        ASSERT_EQ(projectile_red_component.GetReceiverID(), events_effect_package_received.at(1).receiver_id)
            << "Target for red hit event does not match entity projectile component data";
        ASSERT_EQ(blue_entity->GetID(), events_effect_package_received.at(1).receiver_id)
            << "Red projectile did not hit blue";

        // Damage should have been done
        ASSERT_EQ(2, events_health_changed.size()) << "Health should have changed on blue and red";

        // Should have hit red
        ASSERT_EQ(red_entity->GetID(), events_health_changed.at(0).entity_id) << "Red health did not change";
        ASSERT_LT(red_stats_component.GetCurrentHealth(), data.type_data.stats.Get(StatType::kMaxHealth))
            << "Health should have changed on red";
        ASSERT_LT(events_health_changed.at(0).delta, 0_fp) << "Health should have changed on red";

        // Should have hit blue
        ASSERT_EQ(blue_entity->GetID(), events_health_changed.at(1).entity_id) << "Blue health did not change";
        ASSERT_LT(blue_stats_component.GetCurrentHealth(), data.type_data.stats.Get(StatType::kMaxHealth))
            << "Health should have changed on blue";
        ASSERT_LT(events_health_changed.at(1).delta, 0_fp) << "Health should have changed on blue";

        // Check if right projectiles events fired
        ASSERT_EQ(events_projectile_destroyed.size(), 2);
        ASSERT_EQ(events_projectile_destroyed[0].entity_id, first_projectile_sender_blue_id);
        ASSERT_EQ(events_projectile_destroyed[1].entity_id, first_projectile_sender_red_id);

        // Projectiles should be destroyed
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_blue_id));
        ASSERT_TRUE(world->HasEntity(first_projectile_sender_red_id));
        world->TimeStep();

        ASSERT_FALSE(world->HasEntity(first_projectile_sender_blue_id))
            << "Projectile from blue should have been destroyed";
        ASSERT_FALSE(world->HasEntity(first_projectile_sender_red_id))
            << "Projectile from red should have been destroyed";
    }
};
class ProjectileSystemTestWithShield : public ProjectileSystemTestWithStartHomingProjectile
{
    typedef ProjectileSystemTestWithStartHomingProjectile Super;

protected:
    void InitAttackAbilities() override
    {
        // Add attack abilities
        {
            auto& ability = data.type_data.attack_abilities.AddAbility();
            ability.name = "Projectile Ability";
            // Timers are instant 0

            // Skills
            auto& skill1 = ability.AddSkill();
            skill1.name = "Projectile Attack";
            skill1.targeting.type = SkillTargetingType::kAllegiance;
            skill1.targeting.group = AllegianceType::kAlly;
            skill1.deployment.type = SkillDeploymentType::kProjectile;
            skill1.deployment.guidance = DeploymentGuidance();

            // Set the projectile data
            skill1.projectile.size_units = ProjectileSize();
            skill1.projectile.speed_sub_units = 3000;
            skill1.projectile.is_homing = IsHoming();
            skill1.projectile.apply_to_all = ApplyToAll();
            skill1.projectile.is_blockable = IsBlockable();

            auto shield_effect_data = EffectData::Create(EffectType::kSpawnShield, EffectExpression::FromValue(100_fp));

            skill1.AddEffect(shield_effect_data);

            ability.total_duration_ms = UseInstantTimers() ? 0 : 1000;
            skill1.deployment.pre_deployment_delay_percentage = UseInstantTimers() ? 0 : 20;
        }
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    }

    int ProjectileSize() const override
    {
        return 2;
    }

    void SpawnCombatUnits() override
    {
        // Spawn two combat units
        SpawnCombatUnit(Team::kBlue, {0, -3}, data, blue_entity);
        SpawnCombatUnit(Team::kBlue, {0, 12}, data, blue_entity2);
    }

    Entity* blue_entity2 = nullptr;
};

class ProjectileSystemTestWithStartHomingProjectileDodge : public ProjectileSystemTestWithStartHomingProjectile
{
    typedef ProjectileSystemTestWithStartHomingProjectile Super;

protected:
    void InitStats() override
    {
        Super::InitStats();

        data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, kMaxPercentageFP);
    }
};

class ProjectileSystemTestWithStartHomingProjectileWithSize : public ProjectileSystemTestWithStartHomingProjectile
{
    typedef ProjectileSystemTestWithStartHomingProjectile Super;

protected:
    int ProjectileSize() const override
    {
        return 3;
    }
};

class ProjectileSystemTestWithStartNonHomingProjectile : public ProjectileSystemTestWithStartHomingProjectile
{
    typedef ProjectileSystemTestWithStartHomingProjectile Super;

protected:
    bool IsHoming() const override
    {
        return false;
    }
};

class ProjectileSystemTestWithStartNonHomingProjectileWithSize : public ProjectileSystemTestWithStartNonHomingProjectile
{
    typedef ProjectileSystemTestWithStartNonHomingProjectile Super;

protected:
    int ProjectileSize() const override
    {
        return 3;
    }
};

// Variant of ranged attacks with homing and blockable enabled
class ProjectileSystemTestWithStartHomingBlockableProjectile : public ProjectileSystemTestWithStartHomingProjectile
{
protected:
    bool IsHoming() const override
    {
        return true;
    }
    bool IsBlockable() const override
    {
        return true;
    }
    bool ApplyToAll() const override
    {
        return false;
    }
};

class ProjectileSystemTestWithStartHomingBlockableProjectileWithSize
    : public ProjectileSystemTestWithStartHomingBlockableProjectile
{
    typedef ProjectileSystemTestWithStartHomingBlockableProjectile Super;

protected:
    int ProjectileSize() const override
    {
        return 3;
    }
};

// Variant of ranged attacks with homing disabled and blockable enabled
class ProjectileSystemTestWithStartNonHomingBlockableProjectile
    : public ProjectileSystemTestWithStartNonHomingProjectile
{
protected:
    bool IsHoming() const override
    {
        return false;
    }
    bool IsBlockable() const override
    {
        return true;
    }
    bool ApplyToAll() const override
    {
        return false;
    }
};

class ProjectileSystemTestWithStartNonHomingBlockableProjectileWithSize
    : public ProjectileSystemTestWithStartNonHomingBlockableProjectile
{
    typedef ProjectileSystemTestWithStartNonHomingBlockableProjectile Super;

protected:
    int ProjectileSize() const override
    {
        return 3;
    }
};

// Variant with homing that is not blockable and has apply all modifier
class ProjectileSystemTestWithStartHomingApplyToAllProjectile : public ProjectileSystemTestWithStartHomingProjectile
{
protected:
    bool IsHoming() const override
    {
        return true;
    }
    bool IsBlockable() const override
    {
        return false;
    }
    bool ApplyToAll() const override
    {
        return true;
    }
};

class ProjectileSystemTestWithStartHomingApplyToAllProjectileWithSize
    : public ProjectileSystemTestWithStartHomingApplyToAllProjectile
{
    typedef ProjectileSystemTestWithStartHomingApplyToAllProjectile Super;

protected:
    int ProjectileSize() const override
    {
        return 3;
    }
};

// Variant without homing that is not blockable and has apply all modifier
class ProjectileSystemTestWithStartNonHomingApplyToAllProjectile
    : public ProjectileSystemTestWithStartNonHomingProjectile
{
protected:
    bool IsHoming() const override
    {
        return false;
    }
    bool IsBlockable() const override
    {
        return false;
    }
    bool ApplyToAll() const override
    {
        return true;
    }
};

class ProjectileSystemTestWithStartNonHomingApplyToAllProjectileWithSize
    : public ProjectileSystemTestWithStartNonHomingApplyToAllProjectile
{
    typedef ProjectileSystemTestWithStartNonHomingApplyToAllProjectile Super;

protected:
    int ProjectileSize() const override
    {
        return 3;
    }
};

class ProjectileSystemTestBlockedBySize : public ProjectileSystemTestWithStartNonHomingBlockableProjectile
{
    typedef ProjectileSystemTestWithStartNonHomingBlockableProjectile Super;

protected:
    void SpawnCombatUnits() override
    {
        // Spawn three combat units
        SpawnCombatUnit(Team::kBlue, {0, -17}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {0, -3}, data, red_entity);

        // CombatUnit getting in the way
        Entity* third_entity = nullptr;
        SpawnCombatUnit(Team::kRed, {0, -8}, data, third_entity);
    }

    int ProjectileSize() const override
    {
        return 3;
    }
};

class ProjectileSystemTestWithGuidance : public ProjectileSystemTestWithStartNonHomingBlockableProjectile
{
    typedef ProjectileSystemTestWithStartNonHomingBlockableProjectile Super;

protected:
    void SpawnCombatUnits() override
    {
        // Spawn three combat units
        SpawnCombatUnit(Team::kBlue, {0, -17}, data, blue_entity);
        SpawnCombatUnit(Team::kRed, {0, -3}, data, red_entity);

        // CombatUnit getting in the way
        Entity* third_entity = nullptr;
        SpawnCombatUnit(Team::kRed, {0, -8}, data, third_entity);

        // Obstacle is airborne
        const auto effect_data = EffectData::CreatePlaneChange(EffectPlaneChange::kAirborne, 1000);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(third_entity->GetID());
        GetAttachedEffectsHelper().AddAttachedEffect(*third_entity, third_entity->GetID(), effect_data, effect_state);
    }

    int ProjectileSize() const override
    {
        return 3;
    }

    EnumSet<GuidanceType> DeploymentGuidance() const override
    {
        return MakeEnumSet(GuidanceType::kGround);
    }
};

class ProjectileSystemTestWithStartHomingSlowProjectile : public ProjectileSystemTestWithStartHomingProjectile
{
    typedef ProjectileSystemTestWithStartHomingProjectile Super;

    void InitAttackAbilities() override
    {
        Super::InitAttackAbilities();

        data.type_data.attack_abilities.abilities[0]->skills[0]->projectile.speed_sub_units = 2000;
    }
};

class ProjectileSystemTestWithShieldWithGroundGuidance : public ProjectileSystemTestWithShield
{
public:
    EnumSet<GuidanceType> DeploymentGuidance() const override
    {
        return MakeEnumSet(GuidanceType::kGround);
    }
};

TEST_F(ProjectileSystemTestWithStartHomingProjectile, ProjectilesEventsFire)
{
    ProjectileEventsFire();
}
TEST_F(ProjectileSystemTestWithStartNonHomingProjectile, ProjectilesEventsFire)
{
    ProjectileEventsFire();
}

TEST_F(ProjectileSystemTestWithStartHomingProjectileDodge, DodgeProjectile)
{
    ProjectileEventsFireDodge();
}

TEST_F(ProjectileSystemTestWithStartHomingProjectile, ProjectileReachingTargetIfTargetIsDead)
{
    ProjectileReachingTargetIfTargetIsDead();
}

TEST_F(ProjectileSystemTestWithStartNonHomingProjectile, ProjectileReachingTargetIfTargetIsDead)
{
    ProjectileReachingTargetIfTargetIsDead();
}

TEST_F(ProjectileSystemTestWithStartHomingBlockableProjectile, ProjectilesEventsFire)
{
    ProjectileEventsFireBlockable();
}

TEST_F(ProjectileSystemTestWithStartNonHomingBlockableProjectile, ProjectilesEventsFire)
{
    ProjectileEventsFireBlockable();
}

TEST_F(ProjectileSystemTestWithStartHomingBlockableProjectile, ProjectilesAreBlocked)
{
    ProjectilesAreBlocked();
}

TEST_F(ProjectileSystemTestWithStartHomingApplyToAllProjectile, ProjectilesEventsFire)
{
    ProjectileEventsFireApplyToAll();
}

TEST_F(ProjectileSystemTestWithStartNonHomingApplyToAllProjectile, ProjectilesEventsFire)
{
    ProjectileEventsFireApplyToAll();
}

TEST_F(ProjectileSystemTestWithStartHomingApplyToAllProjectile, ProjectilesHitEveryone)
{
    ProjectilesHitEveryone();
}

TEST_F(ProjectileSystemTestWithStartHomingProjectileWithSize, ProjectilesEventsFire)
{
    ProjectileEventsFire();
}

TEST_F(ProjectileSystemTestWithStartNonHomingProjectileWithSize, ProjectilesEventsFire)
{
    ProjectileEventsFire();
}

TEST_F(ProjectileSystemTestWithStartHomingProjectileWithSize, ProjectileReachingTargetIfTargetIsDead)
{
    ProjectileReachingTargetIfTargetIsDead(true);
}

TEST_F(ProjectileSystemTestWithStartNonHomingProjectileWithSize, ProjectileReachingTargetIfTargetIsDead)
{
    ProjectileReachingTargetIfTargetIsDead(true);
}

TEST_F(ProjectileSystemTestWithStartHomingBlockableProjectileWithSize, ProjectilesEventsFire)
{
    ProjectileEventsFireBlockable();
}

TEST_F(ProjectileSystemTestWithStartNonHomingBlockableProjectileWithSize, ProjectilesEventsFire)
{
    ProjectileEventsFireBlockable();
}

TEST_F(ProjectileSystemTestWithStartHomingBlockableProjectileWithSize, ProjectilesAreBlocked)
{
    ProjectilesAreBlocked();
}

TEST_F(ProjectileSystemTestWithStartHomingApplyToAllProjectileWithSize, ProjectilesEventsFire)
{
    ProjectileEventsFireApplyToAll();
}

TEST_F(ProjectileSystemTestWithStartNonHomingApplyToAllProjectileWithSize, ProjectilesEventsFire)
{
    ProjectileEventsFireApplyToAll();
}

TEST_F(ProjectileSystemTestBlockedBySize, HitOtherTarget)
{
    // Force focus of blue to entity ID 1
    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    blue_focus_component.SetFocus(world->GetByIDPtr(1));

    // Listen to spawned projectiles
    std::vector<event_data::ProjectileCreated> events_projectile_created;
    world->SubscribeToEvent(
        EventType::kProjectileCreated,
        [&events_projectile_created](const Event& event)
        {
            events_projectile_created.emplace_back(event.Get<event_data::ProjectileCreated>());
        });

    // Deploy projectile
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // Gather and verify data
    EXPECT_EQ(events_projectile_created.size(), 3);
    const EntityID first_projectile_sender_blue_id = events_projectile_created[0].entity_id;
    const auto& projectile_sender_blue = world->GetByID(first_projectile_sender_blue_id);
    const auto& projectile_blue_component = projectile_sender_blue.Get<ProjectileComponent>();

    // Start abilities sequence for the first projectile (slightly ahead of others due to focus)
    world->TimeStep();
    world->TimeStep();
    ASSERT_EQ(2, events_effect_package_received.size()) << "First projectile should have hit target";

    // Check if projectile hit the right target - it should not
    ASSERT_EQ(first_projectile_sender_blue_id, events_effect_package_received.at(0).sender_id)
        << "First hit event did not originate from our blue projectile";
    ASSERT_NE(projectile_blue_component.GetReceiverID(), events_effect_package_received.at(0).receiver_id)
        << "Target for blue hit event matches entity projectile component data";
    ASSERT_NE(red_entity->GetID(), events_effect_package_received.at(0).receiver_id) << "Blue projectile hit red";
}

TEST_F(ProjectileSystemTestWithGuidance, PassAirborneTarget)
{
    // Force focus of blue to entity ID 1
    auto& blue_focus_component = blue_entity->Get<FocusComponent>();
    blue_focus_component.SetFocus(world->GetByIDPtr(1));

    // Listen to spawned projectiles
    std::vector<event_data::ProjectileCreated> events_projectile_created;
    world->SubscribeToEvent(
        EventType::kProjectileCreated,
        [&events_projectile_created](const Event& event)
        {
            events_projectile_created.emplace_back(event.Get<event_data::ProjectileCreated>());
        });

    // Deploy projectile
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // Gather and verify data
    EXPECT_EQ(events_projectile_created.size(), 3);
    const EntityID first_projectile_sender_blue_id = events_projectile_created[0].entity_id;
    const auto& projectile_sender_blue = world->GetByID(first_projectile_sender_blue_id);
    const auto& projectile_blue_component = projectile_sender_blue.Get<ProjectileComponent>();

    // Start abilities sequence for the first projectile (slightly ahead of others due to focus)
    world->TimeStep();
    world->TimeStep();
    world->TimeStep();
    ASSERT_EQ(3, events_effect_package_received.size()) << "3 projectiles should have hit targets";

    // Check if projectile hit the right target
    ASSERT_EQ(first_projectile_sender_blue_id, events_effect_package_received.at(1).sender_id)
        << "Second hit event did not originate from our blue projectile";
    ASSERT_EQ(projectile_blue_component.GetReceiverID(), events_effect_package_received.at(1).receiver_id)
        << "Target for blue hit event does not match entity projectile component data";
    ASSERT_EQ(red_entity->GetID(), events_effect_package_received.at(1).receiver_id)
        << "Blue projectile did not hit red";

    // Check that nothing else hit the airborne entity
    ASSERT_NE(2, events_effect_package_received.at(0).receiver_id) << "First hit event reached airborne entity";
    ASSERT_NE(2, events_effect_package_received.at(2).receiver_id) << "Third hit event reached airborne entity";
}

TEST_F(ProjectileSystemTestWithStartHomingProjectile, ProjectilesShouldDamageUnrargetable)
{
    // Listen to spawned projectiles
    std::vector<event_data::ProjectileCreated> events_projectile_created;
    world->SubscribeToEvent(
        EventType::kProjectileCreated,
        [&events_projectile_created](const Event& event)
        {
            events_projectile_created.emplace_back(event.Get<event_data::ProjectileCreated>());
        });

    // Listen to projectile movement
    std::vector<event_data::Moved> events_projectiles_moved;
    world->SubscribeToEvent(
        EventType::kMoved,
        [&events_projectiles_moved, this](const Event& event)
        {
            const auto& data = event.Get<event_data::Moved>();
            if (EntityHelper::IsAProjectile(*world, data.entity_id))
            {
                events_projectiles_moved.emplace_back(data);
            }
        });

    // Deploy projectile
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // Check events contains what is needed
    ASSERT_EQ(events_projectile_created.size(), 2) << "Projectiles were not created";
    ASSERT_EQ(events_projectile_created[0].sender_id, blue_entity->GetID())
        << "First projectile did not originate from blue";
    ASSERT_EQ(events_projectile_created[1].sender_id, red_entity->GetID())
        << "Second projectile did not originate from red";
    ASSERT_EQ(events_projectile_created[0].receiver_id, red_entity->GetID())
        << "First projectile target from blue is not the red";
    ASSERT_EQ(events_projectile_created[1].receiver_id, blue_entity->GetID())
        << "Second projectile target from red is not blue";
    ASSERT_TRUE(world->HasEntity(events_projectile_created[0].entity_id));
    ASSERT_TRUE(world->HasEntity(events_projectile_created[1].entity_id));

    ASSERT_EQ(0, events_projectiles_moved.size()) << "No projectile should have moved";

    const EntityID first_projectile_sender_blue_id = events_projectile_created[0].entity_id;
    const EntityID first_projectile_sender_red_id = events_projectile_created[1].entity_id;
    events_projectile_created.clear();

    auto& projectile_sender_blue = world->GetByID(first_projectile_sender_blue_id);
    auto& projectile_blue_focus = projectile_sender_blue.Get<FocusComponent>();

    auto& projectile_sender_red = world->GetByID(first_projectile_sender_red_id);
    auto& projectile_red_focus = projectile_sender_red.Get<FocusComponent>();

    ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
        << "Projectile from blue does not have the target set to the red";
    ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
        << "Projectile from red does not have the target set to the blue";

    ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
        << "Projectile from blue does not have the target set to the red";
    ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
        << "Projectile from red does not have the target set to the blue";

    world->TimeStep();
    ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
    events_projectiles_moved.clear();
    world->TimeStep();
    ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
    events_projectiles_moved.clear();
    world->TimeStep();
    ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
    events_projectiles_moved.clear();

    // Listen to health changed
    std::vector<event_data::StatChanged> events_health_changed;
    world->SubscribeToEvent(
        EventType::kHealthChanged,
        [&events_health_changed](const Event& event)
        {
            events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
        });

    // Listen to destroyed projectiles
    std::vector<event_data::ProjectileDestroyed> events_projectile_destroyed;
    world->SubscribeToEvent(
        EventType::kProjectileDestroyed,
        [&events_projectile_destroyed](const Event& event)
        {
            events_projectile_destroyed.emplace_back(event.Get<event_data::ProjectileDestroyed>());
        });

    // Reached target
    if (ProjectileSize() < 2)
    {
        world->TimeStep();
    }

    ASSERT_EQ(0, events_effect_package_received.size()) << "Projectiles should not have hit targets";

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsTargetable(*blue_entity));
    ASSERT_TRUE(EntityHelper::IsTargetable(*red_entity));

    const auto effect_data = EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, 1000);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

    ASSERT_FALSE(EntityHelper::IsTargetable(*blue_entity));

    // Start abilities sequence for the projectiles
    world->TimeStep();  // kAttackAbility, kExecuting, kFinished
    ASSERT_EQ(2, events_effect_package_received.size()) << "Projectile should have hit both entities";

    // Check if first projectile hit the right target
    ASSERT_EQ(projectile_sender_blue.GetID(), events_effect_package_received.at(0).sender_id);
    ASSERT_EQ(red_entity->GetID(), events_effect_package_received.at(0).receiver_id);

    // Check if second projectile hit the right target
    ASSERT_EQ(projectile_sender_red.GetID(), events_effect_package_received.at(1).sender_id);
    ASSERT_EQ(blue_entity->GetID(), events_effect_package_received.at(1).receiver_id);

    // Damage should have been done to both entities
    ASSERT_EQ(2, events_health_changed.size()) << "Health should have changed only on both entities";

    // Check if two projectiles are going to be destroyed
    ASSERT_EQ(events_projectile_destroyed.size(), 2);
    ASSERT_EQ(events_projectile_destroyed[0].entity_id, first_projectile_sender_blue_id);
    ASSERT_EQ(events_projectile_destroyed[1].entity_id, first_projectile_sender_red_id);

    // Projectiles should be destroyed
    ASSERT_TRUE(world->HasEntity(first_projectile_sender_blue_id));
    ASSERT_TRUE(world->HasEntity(first_projectile_sender_red_id));
    world->TimeStep();

    ASSERT_FALSE(world->HasEntity(first_projectile_sender_blue_id))
        << "Projectile from blue should have been destroyed";
    ASSERT_FALSE(world->HasEntity(first_projectile_sender_red_id)) << "Projectile from red should have been destroyed";
}

TEST_F(ProjectileSystemTestWithStartHomingSlowProjectile, ProjectilesShouldNotFollowUntargetable)
{
    // Listen to spawned projectiles
    std::vector<event_data::ProjectileCreated> events_projectile_created;
    world->SubscribeToEvent(
        EventType::kProjectileCreated,
        [&events_projectile_created](const Event& event)
        {
            events_projectile_created.emplace_back(event.Get<event_data::ProjectileCreated>());
        });

    // Listen to projectile movement
    std::vector<event_data::Moved> events_projectiles_moved;
    world->SubscribeToEvent(
        EventType::kMoved,
        [&events_projectiles_moved, this](const Event& event)
        {
            const auto& data = event.Get<event_data::Moved>();
            if (EntityHelper::IsAProjectile(*world, data.entity_id))
            {
                events_projectiles_moved.emplace_back(data);
            }
        });

    // Deploy projectile
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    // Check events contains what is needed
    ASSERT_EQ(events_projectile_created.size(), 2) << "Projectiles were not created";
    ASSERT_EQ(events_projectile_created[0].sender_id, blue_entity->GetID())
        << "First projectile did not originate from blue";
    ASSERT_EQ(events_projectile_created[1].sender_id, red_entity->GetID())
        << "Second projectile did not originate from red";
    ASSERT_EQ(events_projectile_created[0].receiver_id, red_entity->GetID())
        << "First projectile target from blue is not the red";
    ASSERT_EQ(events_projectile_created[1].receiver_id, blue_entity->GetID())
        << "Second projectile target from red is not blue";
    ASSERT_TRUE(world->HasEntity(events_projectile_created[0].entity_id));
    ASSERT_TRUE(world->HasEntity(events_projectile_created[1].entity_id));

    ASSERT_EQ(0, events_projectiles_moved.size()) << "No projectile should have moved";

    const EntityID first_projectile_sender_blue_id = events_projectile_created[0].entity_id;
    const EntityID first_projectile_sender_red_id = events_projectile_created[1].entity_id;
    events_projectile_created.clear();

    auto& projectile_sender_blue = world->GetByID(first_projectile_sender_blue_id);
    auto& projectile_blue_focus = projectile_sender_blue.Get<FocusComponent>();

    auto& projectile_sender_red = world->GetByID(first_projectile_sender_red_id);
    auto& projectile_red_focus = projectile_sender_red.Get<FocusComponent>();

    ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
        << "Projectile from blue does not have the target set to the red";
    ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
        << "Projectile from red does not have the target set to the blue";

    ASSERT_EQ(red_entity->GetID(), projectile_blue_focus.GetFocusID())
        << "Projectile from blue does not have the target set to the red";
    ASSERT_EQ(blue_entity->GetID(), projectile_red_focus.GetFocusID())
        << "Projectile from red does not have the target set to the blue";

    world->TimeStep();
    ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
    events_projectiles_moved.clear();
    world->TimeStep();
    ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
    events_projectiles_moved.clear();
    world->TimeStep();
    ASSERT_EQ(2, events_projectiles_moved.size()) << "2 projectiles should have moved";
    events_projectiles_moved.clear();

    // Listen to health changed
    std::vector<event_data::StatChanged> events_health_changed;
    world->SubscribeToEvent(
        EventType::kHealthChanged,
        [&events_health_changed](const Event& event)
        {
            events_health_changed.emplace_back(event.Get<event_data::StatChanged>());
        });

    // Listen to destroyed projectiles
    std::vector<event_data::ProjectileDestroyed> events_projectile_destroyed;
    world->SubscribeToEvent(
        EventType::kProjectileDestroyed,
        [&events_projectile_destroyed](const Event& event)
        {
            events_projectile_destroyed.emplace_back(event.Get<event_data::ProjectileDestroyed>());
        });

    // Reached target
    if (ProjectileSize() < 2)
    {
        world->TimeStep();
    }

    ASSERT_EQ(0, events_effect_package_received.size()) << "Projectiles should not have hit targets";

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsTargetable(*blue_entity));
    ASSERT_TRUE(EntityHelper::IsTargetable(*red_entity));

    const auto effect_data = EffectData::CreatePositiveState(EffectPositiveState::kUntargetable, 1000);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity->GetID());
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity->GetID(), effect_data, effect_state);

    ASSERT_FALSE(EntityHelper::IsTargetable(*blue_entity));

    // After one time step projectiles don't reach targets
    world->TimeStep();
    ASSERT_EQ(0, events_effect_package_received.size());

    // Move blue entity from its current position
    auto& blue_position_component = blue_entity->Get<PositionComponent>();
    const HexGridPosition& current_position = blue_position_component.GetPosition();
    blue_position_component.SetPosition(current_position + HexGridPosition(10, 10));

    // After this time step projectiles don't reach targets
    world->TimeStep();
    ASSERT_EQ(0, events_effect_package_received.size());

    // Projectile from blue sender reaches target, second one - still not because of radius of target entity
    world->TimeStep();  // kAttackAbility, kExecuting, kFinished
    ASSERT_EQ(1, events_effect_package_received.size()) << "Projectile should have hit only red";

    // Check if first projectile hit the right target
    ASSERT_EQ(projectile_sender_blue.GetID(), events_effect_package_received.at(0).sender_id);
    ASSERT_EQ(red_entity->GetID(), events_effect_package_received.at(0).receiver_id);

    // Damage should have been done to one entity
    ASSERT_EQ(1, events_health_changed.size()) << "Health should have changed only on both entities";
    events_health_changed.clear();

    // Check if blue projectile is going to be destroyed
    ASSERT_EQ(events_projectile_destroyed.size(), 1);
    ASSERT_EQ(events_projectile_destroyed[0].entity_id, first_projectile_sender_blue_id);

    events_projectile_destroyed.clear();

    // Projectiles should not be destroyed
    ASSERT_TRUE(world->HasEntity(first_projectile_sender_blue_id));
    ASSERT_TRUE(world->HasEntity(first_projectile_sender_red_id));

    // Projectile from red sender reaches target location
    // But doesn't cause any damage
    world->TimeStep();

    // Damage should have been done to one entity
    ASSERT_EQ(0, events_health_changed.size()) << "Health should not have changed to any of entities";

    // Check if red projectile is going to be destroyed
    ASSERT_EQ(events_projectile_destroyed.size(), 1);
    ASSERT_EQ(events_projectile_destroyed[0].entity_id, first_projectile_sender_red_id);
    events_projectile_destroyed.clear();

    ASSERT_FALSE(world->HasEntity(first_projectile_sender_blue_id))
        << "Projectile from blue should have been destroyed";

    world->TimeStep();

    ASSERT_FALSE(world->HasEntity(first_projectile_sender_red_id)) << "Projectile from red should have been destroyed";
}

TEST_F(ProjectileSystemTestWithStartHomingProjectile, ThornsHitOnProjectileHit)
{
    auto& red_stats_component = red_entity->Get<StatsComponent>();
    auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 200_fp);

    blue_stats_component.GetMutableTemplateStats().Set(StatType::kThorns, 25_fp);

    // Deploy projectile
    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());

    world->TimeStep();
    world->TimeStep();
    world->TimeStep();
    world->TimeStep();
    world->TimeStep();

    // Projectile does 150 damage, thorns does an additional 25 damage
    EXPECT_EQ(red_stats_component.GetCurrentHealth(), 25_fp);
}

TEST_F(ProjectileSystemTestWithShield, ProjectileSendShield)
{
    auto& blue2_stats_component = blue_entity2->Get<StatsComponent>();

    EXPECT_EQ(blue2_stats_component.GetCurrentHealth(), 200_fp);
    EXPECT_EQ(world->GetShieldTotal(*blue_entity2), 0_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());
    world->TimeStep();

    // Projectile should provide blue_entity2 100 shield
    EXPECT_EQ(world->GetShieldTotal(*blue_entity2), 100_fp);
}

TEST_F(ProjectileSystemTestWithShieldWithGroundGuidance, ProjectileSendShieldAirborne)
{
    auto& blue2_stats_component = blue_entity2->Get<StatsComponent>();

    // Also make the second blue entity airborne.
    {
        const auto effect_data = EffectData::CreatePlaneChange(EffectPlaneChange::kAirborne, kTimeInfinite);
        Entity* target_entity = blue_entity2;
        const EntityID blue_entity_id = target_entity->GetID();
        {
            EffectState effect_state{};
            effect_state.sender_stats = world->GetFullStats(blue_entity_id);
            GetAttachedEffectsHelper().AddAttachedEffect(*target_entity, blue_entity_id, effect_data, effect_state);
        }
    }

    EXPECT_EQ(blue2_stats_component.GetCurrentHealth(), 200_fp);
    EXPECT_EQ(world->GetShieldTotal(*blue_entity2), 0_fp);

    // Manually init system
    auto ability_system = AbilitySystem();
    ability_system.Init(world.get());
    auto& blue_abilities_component = blue_entity->Get<AbilitiesComponent>();

    auto& attack_ability = blue_abilities_component.GetStateAttackAbilities().at(0);
    ForcedActivateAbility(*blue_entity, ability_system, attack_ability);

    ASSERT_TRUE(TimeStepUntilEventSkillDeploying());
    world->TimeStep();

    // Projectile should provide blue_entity2 100 shield even though guidance type does not match entity plane:
    // When an Ally undergoes a Plane Change it does not make a difference to its team. An Ally always remains
    // targetable, and always intersects with entities sent by allies no matter what plane they are in.
    EXPECT_EQ(world->GetShieldTotal(*blue_entity2), 100_fp);
}

// Variant of ranged attacks with non-homing and non-blockable projectiles
TEST_F(BaseTest, ProjectileSystemTestWithStartNonHomingNonBlockableProjectile)
{
    CombatUnitData data = CreateCombatUnitData();

    // Stats
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 300_fp);
    data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 5_fp);
    // No critical attacks
    data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);
    // No Dodge
    data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);
    // No Miss
    data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);

    // Once every tick
    data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);

    // 15^2 == 225
    // square distance between blue and red is 200
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 15_fp);
    data.type_data.stats.Set(StatType::kOmegaRangeUnits, 25_fp);
    data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);
    data.type_data.stats.Set(StatType::kEnergyCost, 1000_fp);
    data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP);
    data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

    // don't move
    data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);

    // Stats
    data.radius_units = 2;
    data.type_data.combat_affinity = CombatAffinity::kFire;
    data.type_data.combat_class = CombatClass::kBehemoth;
    data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 50_fp);
    data.type_data.stats.Set(StatType::kStartingEnergy, 50_fp);
    data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 200_fp);
    data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 2000_fp);

    // Add attack abilities
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();
        ability.name = "Projectile Ability";

        // Skills
        auto& skill1 = ability.AddSkill();
        skill1.SetDefaults(AbilityType::kAttack);
        skill1.name = "Projectile Attack";
        skill1.targeting.type = SkillTargetingType::kDistanceCheck;
        skill1.targeting.group = AllegianceType::kEnemy;
        skill1.targeting.lowest = false;
        skill1.targeting.num = 1;
        skill1.deployment.type = SkillDeploymentType::kProjectile;
        skill1.deployment.guidance =
            MakeEnumSet(GuidanceType::kGround, GuidanceType::kUnderground, GuidanceType::kAirborne);

        // Set the projectile data
        skill1.projectile.size_units = 1;
        skill1.projectile.speed_sub_units = 3000;
        skill1.projectile.is_homing = false;
        skill1.projectile.apply_to_all = true;
        skill1.projectile.is_blockable = false;

        // More than the shield so we affect the health too
        skill1.AddDamageEffect(EffectDamageType::kPhysical, EffectExpression::FromValue(100_fp));

        ability.total_duration_ms = 1000;
        skill1.deployment.pre_deployment_delay_percentage = 20;
    }

    data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    // Spawn 4 blue and 4 red combats units.
    // Place them in one row

    std::vector<Entity*> blue_entities;
    for (int i = 0; i != 4; ++i)
    {
        Entity* entity = nullptr;
        SpawnCombatUnit(Team::kBlue, {3 * (i), -3 * (2 * i + 1)}, data, entity);
        blue_entities.push_back(entity);

        // first blue entity will not be stunned
        if (i > 0)
        {
            Stun(*entity);
        }
    }

    std::vector<Entity*> red_entities;
    for (int i = 0; i != 4; ++i)
    {
        Entity* entity = nullptr;
        SpawnCombatUnit(Team::kRed, {-3 * (i + 1), 3 * (2 * i + 1)}, data, entity);
        red_entities.push_back(entity);
        Stun(*entity);
    }

    EventHistory<EventType::kAbilityActivated> activated_abilities(*world);
    EventHistory<EventType::kProjectileCreated> spawned_projectiles(*world);
    EventHistory<EventType::kProjectileDestroyed> projectiles_destroyed(*world);

    // Activate an ability with projectile
    world->TimeStep();
    ASSERT_EQ(activated_abilities.Size(), 1);

    {
        const event_data::AbilityActivated& ability = activated_abilities[0];
        ASSERT_EQ(ability.predicted_receiver_ids.size(), 1);
        EXPECT_EQ(ability.predicted_receiver_ids[0], red_entities.back()->GetID());
    }

    EventHistory<EventType::kOnEffectApplied> effects_applied(*world);

    // Spawn projectile to the furthest enemy
    world->TimeStep();
    ASSERT_EQ(activated_abilities.Size(), 1);
    ASSERT_EQ(spawned_projectiles.Size(), 1);
    ASSERT_EQ(effects_applied.Size(), 0);
    ASSERT_EQ(projectiles_destroyed.Size(), 0);
    Stun(*blue_entities[0]);  // don't need new projectiles

    {
        const auto& projectile_event = spawned_projectiles[0];
        ASSERT_TRUE(projectile_event.apply_to_all);
        ASSERT_FALSE(projectile_event.is_blockable);
        ASSERT_FALSE(projectile_event.is_homing);
        ASSERT_EQ(projectile_event.receiver_id, red_entities.back()->GetID());
        ASSERT_EQ(projectile_event.receiver_position, red_entities.back()->Get<PositionComponent>().GetPosition());
    }

    TimeStepForNTimeSteps(10);
    ASSERT_EQ(effects_applied.Size(), 4);
    EXPECT_EQ(effects_applied[0].receiver_id, red_entities[0]->GetID());
    EXPECT_EQ(effects_applied[1].receiver_id, red_entities[1]->GetID());
    EXPECT_EQ(effects_applied[2].receiver_id, red_entities[2]->GetID());
    EXPECT_EQ(effects_applied[3].receiver_id, red_entities[3]->GetID());
    ASSERT_EQ(projectiles_destroyed.Size(), 1);
}

}  // namespace simulation
