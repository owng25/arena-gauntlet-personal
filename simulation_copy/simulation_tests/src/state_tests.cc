#include "base_test_fixtures.h"
#include "components/state_component.h"
#include "ecs/world.h"
#include "gtest/gtest.h"

namespace simulation
{
class StateTest : public BaseTest
{
};

TEST_F(StateTest, FaintDisappear)
{
    // Track event counts
    int fainted_count = 0;
    int disappear_count = 0;

    // Listen to state change events
    std::vector<event_data::CombatUnitStateChanged> events_state_changed;
    world->SubscribeToEvent(
        EventType::kCombatUnitStateChanged,
        [&events_state_changed, &fainted_count, &disappear_count](const Event& event)
        {
            // Get and store the event
            const auto event_data = event.Get<event_data::CombatUnitStateChanged>();
            events_state_changed.emplace_back(event_data);

            // Update relevant counter
            switch (event_data.state)
            {
            case CombatUnitState::kFainted:
                fainted_count++;
                break;
            case CombatUnitState::kDisappeared:
                disappear_count++;
                break;
            default:
                break;
            }
        });

    // Add blue entity
    Entity* blue_entity = nullptr;
    SpawnCombatUnit(Team::kBlue, HexGridPosition{25, 50}, CreateCombatUnitData(), blue_entity);
    auto& state_component = blue_entity->Get<StateComponent>();

    // Make sure initial state is alive
    ASSERT_EQ(state_component.GetState(), CombatUnitState::kAlive);
    ASSERT_EQ(events_state_changed.size(), 0);
    ASSERT_EQ(fainted_count, 0);
    ASSERT_EQ(disappear_count, 0);

    // Faint the entity
    {
        event_data::Fainted event_data;
        event_data.entity_id = blue_entity->GetID();
        event_data.vanquisher_id = blue_entity->GetID();
        world->EmitEvent<EventType::kFainted>(event_data);
    }

    // Make sure it's fainted for constant time steps
    for (int i = 0; i < kDisappearTimeSteps; i++)
    {
        // TimeStep again
        world->TimeStep();
        EXPECT_EQ(state_component.GetState(), CombatUnitState::kFainted);
        EXPECT_EQ(events_state_changed.size(), 1);
        EXPECT_EQ(fainted_count, 1);
        EXPECT_EQ(disappear_count, 0);
    }

    // TimeStep once more, should have disappeared
    world->TimeStep();
    EXPECT_EQ(state_component.GetState(), CombatUnitState::kDisappeared);
    EXPECT_EQ(events_state_changed.size(), 2);
    EXPECT_EQ(fainted_count, 1);
    EXPECT_EQ(disappear_count, 1);
}

}  // namespace simulation
