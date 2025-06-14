#include "attached_effects_system_data_fixtures.h"
#include "utility/attached_effects_helper.h"

namespace simulation
{
TEST_F(AttachedEffectsSystemTest, ApplyEffectPlaneChangeAirborne)
{
    // Get the relevant components
    auto& attached_effects_component = blue_entity->Get<AttachedEffectsComponent>();

    // Validate initial state
    ASSERT_FALSE(attached_effects_component.HasPlaneChange(EffectPlaneChange::kAirborne));

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreatePlaneChange(EffectPlaneChange::kAirborne, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        events_apply_effect.clear();
        world->TimeStep();

        //  Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should be airborne
        ASSERT_TRUE(attached_effects_component.HasPlaneChange(EffectPlaneChange::kAirborne));
    }
    ASSERT_TRUE(attached_effects_component.HasPlaneChange(EffectPlaneChange::kAirborne));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPlaneChange);

    // Should not be airborne
    ASSERT_FALSE(attached_effects_component.HasPlaneChange(EffectPlaneChange::kAirborne));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_FALSE(attached_effects_component.HasPlaneChange(EffectPlaneChange::kAirborne));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectPlaneChangeUnderground)
{
    // Get the relevant components
    auto& attached_effects_component = blue_entity->Get<AttachedEffectsComponent>();

    // Validate initial state
    ASSERT_FALSE(attached_effects_component.HasPlaneChange(EffectPlaneChange::kUnderground));

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreatePlaneChange(EffectPlaneChange::kUnderground, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, blue_entity_id, effect_data, effect_state);

    for (int i = 0; i < 3; i++)
    {
        // TimeStep the world (3 steps)
        events_apply_effect.clear();
        world->TimeStep();

        //  Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should be underground
        ASSERT_TRUE(attached_effects_component.HasPlaneChange(EffectPlaneChange::kUnderground));
    }
    ASSERT_TRUE(attached_effects_component.HasPlaneChange(EffectPlaneChange::kUnderground));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPlaneChange);

    // Should not be underground
    ASSERT_FALSE(attached_effects_component.HasPlaneChange(EffectPlaneChange::kUnderground));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_FALSE(attached_effects_component.HasPlaneChange(EffectPlaneChange::kUnderground));
}

}  // namespace simulation
