#include "base_test_fixtures.h"
#include "components/focus_component.h"
#include "components/position_component.h"
#include "ecs/event_types_data.h"
#include "ecs/world.h"
#include "gtest/gtest.h"

namespace simulation
{
class ECSTest : public BaseTest
{
};

TEST_F(ECSTest, Create)
{
    WorldConfig config;
    world = CreateWorld(config, nullptr);

    // Create
    auto entity_ptr = Entity::Create(world, Team::kBlue, 0);
    auto& entity = *entity_ptr;

    ASSERT_EQ(entity.GetTeam(), Team::kBlue);
    ASSERT_EQ(entity.IsActive(), true);
}

TEST_F(ECSTest, AddComponents)
{
    // Create
    WorldConfig config;
    world = CreateWorld(config, nullptr);
    auto entity_ptr = Entity::Create(world, Team::kBlue, 0);
    auto& entity = *entity_ptr;

    ASSERT_EQ(entity.Has<PositionComponent>(), false) << "Should NOT have PositionComponent";
    ASSERT_EQ(entity.Has<FocusComponent>(), false) << "Should NOT have FocusComponent";
    auto& position_component_return = entity.Add<PositionComponent>();
    ASSERT_EQ(position_component_return.GetQ(), kInvalidPosition);
    ASSERT_EQ(entity.Has<PositionComponent>(), true) << "Should have PositionComponent";
    ASSERT_EQ(entity.Has<FocusComponent>(), false) << "Should NOT have FocusComponent";

    auto& position_component = entity.Get<PositionComponent>();
    ASSERT_EQ(position_component.GetQ(), kInvalidPosition);
    ASSERT_EQ(position_component.GetR(), kInvalidPosition);
}

TEST_F(ECSTest, WorldCreate)
{
    // Add entities
    ASSERT_EQ(world->GetAll().size(), 0);
    auto& entity = world->AddEntity(Team::kBlue);
    ASSERT_EQ(world->GetAll().size(), 1);

    ASSERT_EQ(world->GetAll()[0]->GetTeam(), Team::kBlue);
    ASSERT_EQ(world->GetAll()[0]->GetTeam(), entity.GetTeam());
}

TEST_F(ECSTest, ListenToCreateEvents)
{
    // Subscribe to events
    int callback_entities_created = 0;
    event_data::Created callback_entities_data;
    const auto& entity_created_callback = [&callback_entities_created, &callback_entities_data](const Event& event)
    {
        callback_entities_created++;
        callback_entities_data = event.Get<event_data::Created>();
    };

    ASSERT_EQ(Event::IsEventTypeValid(EventType::kCreated), true);
    world->SubscribeToEvent(EventType::kCreated, entity_created_callback);

    // Add entities and check callback
    ASSERT_EQ(callback_entities_created, 0);
    ASSERT_EQ(callback_entities_data.entity_id, kInvalidEntityID);

    ASSERT_EQ(world->GetAll().size(), callback_entities_created);
    auto& entity1 = world->AddEntity(Team::kBlue);
    ASSERT_EQ(entity1.GetID(), 0);
    ASSERT_EQ(world->GetAll().size(), 1);
    ASSERT_EQ(callback_entities_created, 1);
    ASSERT_EQ(world->GetAll()[0]->GetTeam(), Team::kBlue);

    ASSERT_TRUE(world->HasEntity(callback_entities_data.entity_id));
    {
        const auto& callback_entity = world->GetByID(callback_entities_data.entity_id);
        ASSERT_EQ(callback_entity.GetTeam(), Team::kBlue);
    }

    auto& entity2 = world->AddEntity(Team::kRed);
    ASSERT_EQ(world->GetAll().size(), 2);
    ASSERT_EQ(entity2.GetID(), 1);
    ASSERT_EQ(callback_entities_created, 2);
    ASSERT_EQ(world->GetAll()[1]->GetTeam(), Team::kRed);
    {
        const auto& callback_entity = world->GetByID(callback_entities_data.entity_id);
        ASSERT_EQ(callback_entity.GetTeam(), Team::kRed);
    }

    // Are allies

    ASSERT_FALSE(world->AreAllies(entity1.GetID(), entity2.GetID()));
}

}  // namespace simulation
