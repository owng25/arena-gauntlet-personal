#include "attached_effects_system_data_fixtures.h"
#include "components/focus_component.h"
#include "components/position_component.h"
#include "utility/attached_effects_helper.h"

namespace simulation
{
TEST_F(AttachedEffectsSystemWithMovementTest, ApplyEffectNegativeStateRoot)
{
    // Get the relevant components
    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kRoot, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    // Keep track of positions
    HexGridPosition blue_current_position = blue_position_component.ToSubUnits();
    HexGridPosition red_current_position = red_position_component.ToSubUnits();
    HexGridPosition blue_previous_position = blue_current_position;
    HexGridPosition red_previous_position = red_current_position;

    for (int i = 0; i < 3; i++)
    {
        events_apply_effect.clear();

        blue_previous_position = blue_current_position;
        red_previous_position = red_current_position;
        world->TimeStep();
        blue_current_position = blue_position_component.ToSubUnits();
        red_current_position = red_position_component.ToSubUnits();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be able to move
        ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

        // Red should not have moved but blue should have moved
        ASSERT_EQ(red_current_position, red_previous_position);
        ASSERT_NE(blue_current_position, blue_previous_position);
    }
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be movable
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, MinTenacity)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Tenacity is 0% so full root duration should occur
    red_stats_component.GetMutableTemplateStats().Set(StatType::kTenacityPercentage, 0_fp);
    red_stats_component.SetCurrentHealth(100_fp);

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kRoot, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    for (int i = 0; i < 3; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world (3 steps)
        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should be unmovable
        ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));
    }
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be movable
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, SomeTenacity)
{
    // Get the relevant components
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Tenacity is 33% so only 67% of the 300 ms duration should occur, 201 ms or 2 time step
    // duration
    red_stats_component.GetMutableTemplateStats().Set(StatType::kTenacityPercentage, 33_fp);
    red_stats_component.SetCurrentHealth(100_fp);

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kRoot, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    // The attached effect should be present
    ASSERT_EQ(red_attached_effects_component.GetAttachedEffects().size(), 1);

    // Should start as not movable, root lasts 2 time steps since duration = 300 ms(3 time steps)
    // Tenacity = 33 so duration is 67% of original amount = 2 time steps
    world->TimeStep();
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    // TimeStep and confirm still not movable
    world->TimeStep();
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    // Third time step should make target movable again
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    world->TimeStep();

    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 1);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be movable, does not reactivate
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, MaxTenacity)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Tenacity is 100% so effect is attached then immediately removed
    red_stats_component.GetMutableTemplateStats().Set(StatType::kTenacityPercentage, 100_fp);

    red_stats_component.SetCurrentHealth(100_fp);

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kRoot, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    world->TimeStep();
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 1);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be movable
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectPositiveStateImmuneFullDuration)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    red_stats_component.SetCurrentHealth(100_fp);

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Add an effect Wrap it in an attached effect
    const auto negative_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kRoot, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, negative_effect_data, effect_state);

    const auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
    EffectState immune_effect_state{};
    immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);

    for (int i = 0; i < 3; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world (3 steps)
        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be unmovable
        ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    }
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 2);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be movable
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectPositiveStateImmunePartialDuration)
{
    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Add an effect Wrap it in an attached effect
    const auto negative_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kRoot, 600);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, negative_effect_data, effect_state);

    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    const auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 200);
    EffectState immune_effect_state{};
    immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);

    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);

    for (int i = 0; i < 2; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world (2 steps)
        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should be movable - Can move because immune to rooted
        ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    }
    // Should be not movable
    world->TimeStep();
    world->TimeStep();
    world->TimeStep();
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateTauntedWithImmune)
{
    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsTaunted(*red_entity));

    // Add negative effect
    const auto taunted_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kTaunted, 600);
    EffectState taunted_effect_state{};
    taunted_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, blue_entity_id, taunted_effect_data, taunted_effect_state);

    // Add positive effect
    const auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
    EffectState immune_effect_state{};
    immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);

    ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));

    for (int i = 0; i < 6; i++)
    {
        events_apply_effect.clear();

        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be able to move
        if (i < 3)
        {
            // Should be immune, positions should have not changed
            ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::IsTaunted(*red_entity)) << "i = " << i;
        }
        else
        {
            // Immune should be removed in time step 3
            if (i == 3)
            {
                ASSERT_EQ(events_remove_effect.size(), 1);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
                events_remove_effect.clear();
            }

            // Immune expired
            ASSERT_FALSE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::IsTaunted(*red_entity)) << "i = " << i;
        }
    }
    ASSERT_TRUE(EntityHelper::IsTaunted(*red_entity));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be movable
    ASSERT_FALSE(EntityHelper::IsTaunted(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_FALSE(EntityHelper::IsTaunted(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateBlindWithImmune)
{
    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));

    // Add negative effect blind
    const auto blind_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kBlind, 600);
    EffectState blind_effect_state{};
    blind_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, blind_effect_data, blind_effect_state);

    // Add positive effect
    const auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
    EffectState immune_effect_state{};
    immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);

    ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));

    for (int i = 0; i < 6; i++)
    {
        events_apply_effect.clear();

        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        if (i < 3)
        {
            // Should be immune, thus not blinded
            ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity)) << "i = " << i;
        }
        else
        {
            // Immune should be removed in time step 3
            if (i == 3)
            {
                ASSERT_EQ(events_remove_effect.size(), 1);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
                events_remove_effect.clear();
            }

            // Immune expired
            ASSERT_FALSE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::IsBlinded(*red_entity)) << "i = " << i;
        }
    }
    ASSERT_TRUE(EntityHelper::IsBlinded(*red_entity));

    // TimeStep again, removing effect
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should not be blinded
    ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateLethargicWithAbsoluteImmune)
{
    // Validate initial state
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));

    // Add negative effect lethargic
    const auto lethargic_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kLethargic, 600);
    EffectState lethargic_effect_state{};
    lethargic_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, blue_entity_id, lethargic_effect_data, lethargic_effect_state);

    // Add positive effect
    {
        const auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
        EffectState immune_effect_state{};
        immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
        GetAttachedEffectsHelper()
            .AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);
    }

    ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kLethargic));
    ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));

    for (int i = 0; i < 6; i++)
    {
        events_apply_effect.clear();

        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        if (i < 3)
        {
            // Should be immune, thus able to gain energy
            ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity)) << "i = " << i;
        }
        else
        {
            // Immune should be removed in time step 3
            if (i == 3)
            {
                ASSERT_EQ(events_remove_effect.size(), 1);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
                events_remove_effect.clear();
            }

            // Immune expired
            ASSERT_FALSE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanGainEnergy(*red_entity)) << "i = " << i;
        }
    }
    ASSERT_FALSE(EntityHelper::CanGainEnergy(*red_entity));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be able to gain energy
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateLethargicWithLethargicImmune)
{
    // Validate initial state
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));

    // Add negative effect lethargic
    const auto lethargic_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kLethargic, 600);
    EffectState lethargic_effect_state{};
    lethargic_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, blue_entity_id, lethargic_effect_data, lethargic_effect_state);

    // Add positive effect
    {
        auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
        EffectTypeID& type_id = immune_effect_data.immuned_effect_types.emplace_back();
        type_id.type = EffectType::kNegativeState;
        type_id.negative_state = EffectNegativeState::kLethargic;
        EffectState immune_effect_state{};
        immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
        GetAttachedEffectsHelper()
            .AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);
    }

    ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kLethargic));
    ASSERT_FALSE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));

    for (int i = 0; i < 6; i++)
    {
        events_apply_effect.clear();

        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        if (i < 3)
        {
            // Should be immune, thus able to gain energy
            ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kLethargic))
                << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity)) << "i = " << i;
        }
        else
        {
            // Immune should be removed in time step 3
            if (i == 3)
            {
                ASSERT_EQ(events_remove_effect.size(), 1);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
                events_remove_effect.clear();
            }

            // Immune expired
            ASSERT_FALSE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kLethargic))
                << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanGainEnergy(*red_entity)) << "i = " << i;
        }
    }
    ASSERT_FALSE(EntityHelper::CanGainEnergy(*red_entity));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be able to gain energy
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateSilencedWithAbsoluteImmune)
{
    // Validate initial state
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));

    // Add negative effect silenced
    const auto silenced_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kSilenced, 600);
    EffectState silenced_effect_state{};
    silenced_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, blue_entity_id, silenced_effect_data, silenced_effect_state);

    // Add positive effect
    {
        const auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
        EffectState immune_effect_state{};
        immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
        GetAttachedEffectsHelper()
            .AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);
    }

    ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));
    ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kSilenced));

    for (int i = 0; i < 6; i++)
    {
        events_apply_effect.clear();

        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        if (i < 3)
        {
            // Should be immune, thus able to activate ult
            ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kSilenced))
                << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega)) << "i = " << i;
        }
        else
        {
            // Immune should be removed in time step 3
            if (i == 3)
            {
                ASSERT_EQ(events_remove_effect.size(), 1);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
                events_remove_effect.clear();
            }

            // Immune expired
            ASSERT_FALSE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kSilenced))
                << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega)) << "i = " << i;
        }
    }

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be able to activate ult
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateSilencedWithSilenceImmune)
{
    // Validate initial state
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));

    // Add negative effect silenced
    const auto silenced_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kSilenced, 600);
    EffectState silenced_effect_state{};
    silenced_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, blue_entity_id, silenced_effect_data, silenced_effect_state);

    // Add positive effect
    {
        auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
        EffectTypeID& type_id = immune_effect_data.immuned_effect_types.emplace_back();
        type_id.type = EffectType::kNegativeState;
        type_id.negative_state = EffectNegativeState::kSilenced;
        EffectState immune_effect_state{};
        immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
        GetAttachedEffectsHelper()
            .AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);
    }

    ASSERT_FALSE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));
    ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kSilenced));

    for (int i = 0; i < 6; i++)
    {
        events_apply_effect.clear();

        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        if (i < 3)
        {
            // Should be immune, thus able to activate ult
            ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kSilenced))
                << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega)) << "i = " << i;
        }
        else
        {
            // Immune should be removed in time step 3
            if (i == 3)
            {
                ASSERT_EQ(events_remove_effect.size(), 1);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
                events_remove_effect.clear();
            }

            // Immune expired
            ASSERT_FALSE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kSilenced))
                << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega)) << "i = " << i;
        }
    }

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be able to activate ult
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateDisarmWithAbsoluteImmune)
{
    // Validate initial state
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));

    // Add negative effect disarm
    const auto disarm_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kDisarm, 600);
    EffectState disarm_effect_state{};
    disarm_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, disarm_effect_data, disarm_effect_state);

    // Add positive effect
    {
        const auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
        EffectState immune_effect_state{};
        immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
        GetAttachedEffectsHelper()
            .AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);
    }

    ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kDisarm));
    ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));

    for (int i = 0; i < 6; i++)
    {
        events_apply_effect.clear();

        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        if (i < 3)
        {
            // Should be immune, thus able to activate attack
            ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kDisarm))
                << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack)) << "i = " << i;
        }
        else
        {
            // Immune should be removed in time step 3
            if (i == 3)
            {
                ASSERT_EQ(events_remove_effect.size(), 1);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
                events_remove_effect.clear();
            }

            // Immune expired
            ASSERT_FALSE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kDisarm))
                << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack)) << "i = " << i;
        }
    }
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be able to activate attack
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateDisarmWithDisarmImmune)
{
    // Validate initial state
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));

    // Add negative effect disarm
    const auto disarm_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kDisarm, 600);
    EffectState disarm_effect_state{};
    disarm_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, disarm_effect_data, disarm_effect_state);

    // Add positive effect
    {
        auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
        EffectTypeID& type_id = immune_effect_data.immuned_effect_types.emplace_back();
        type_id.type = EffectType::kNegativeState;
        type_id.negative_state = EffectNegativeState::kDisarm;
        EffectState immune_effect_state{};
        immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
        GetAttachedEffectsHelper()
            .AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);
    }

    ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kDisarm));
    ASSERT_FALSE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));

    for (int i = 0; i < 6; i++)
    {
        events_apply_effect.clear();

        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        if (i < 3)
        {
            // Should be immune, thus able to activate attack
            ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kDisarm))
                << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack)) << "i = " << i;
        }
        else
        {
            // Immune should be removed in time step 3
            if (i == 3)
            {
                ASSERT_EQ(events_remove_effect.size(), 1);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
                events_remove_effect.clear();
            }

            // Immune expired
            ASSERT_FALSE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kDisarm))
                << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack)) << "i = " << i;
        }
    }
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be able to activate attack
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
}

TEST_F(AttachedEffectsSystemWithMovementTest, ApplyEffectNegativeStateStunWithAbsoluteImmune)
{
    // Get the relevant components
    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // Add negative effect stun
    Stun(*red_entity, 600);

    // Add absolute immunity effect
    {
        const auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
        EffectState immune_effect_state{};
        immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
        GetAttachedEffectsHelper()
            .AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);
    }

    ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));
    ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kStun));

    // Keep track of positions
    HexGridPosition blue_current_position = blue_position_component.ToSubUnits();
    HexGridPosition red_current_position = red_position_component.ToSubUnits();
    HexGridPosition blue_previous_position = blue_current_position;
    HexGridPosition red_previous_position = red_current_position;

    for (int i = 0; i < 6; i++)
    {
        events_apply_effect.clear();

        blue_previous_position = blue_current_position;
        red_previous_position = red_current_position;
        world->TimeStep();
        blue_current_position = blue_position_component.ToSubUnits();
        red_current_position = red_position_component.ToSubUnits();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        if (i < 3)
        {
            // Should be immune, positions should have not changed
            ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kStun)) << "i = " << i;

            ASSERT_TRUE(EntityHelper::IsMovable(*red_entity)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega)) << "i = " << i;
        }
        else
        {
            // Immune should be removed in time step 3
            if (i == 3)
            {
                ASSERT_EQ(events_remove_effect.size(), 1);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
                events_remove_effect.clear();
            }

            // Immune expired
            ASSERT_FALSE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kStun)) << "i = " << i;

            ASSERT_FALSE(EntityHelper::IsMovable(*red_entity)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega)) << "i = " << i;
        }

        if (i <= 3)
        {
            // Should have moved, both
            ASSERT_NE(red_current_position, red_previous_position) << "i = " << i;
            ASSERT_NE(blue_current_position, blue_previous_position) << "i = " << i;
        }
        else
        {
            // Red should not have moved but blue should have moved
            ASSERT_EQ(red_current_position, red_previous_position) << "i = " << i;
            ASSERT_NE(blue_current_position, blue_previous_position) << "i = " << i;
        }
    }
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be movable, fire ult/attack
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
}

TEST_F(AttachedEffectsSystemWithMovementTest, ApplyEffectNegativeStateStunWithStunImmune)
{
    // Get the relevant components
    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // Add negative effect stun
    Stun(*red_entity, 600);

    // Add stun immunity effect
    {
        auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
        EffectTypeID& type_id = immune_effect_data.immuned_effect_types.emplace_back();
        type_id.type = EffectType::kNegativeState;
        type_id.negative_state = EffectNegativeState::kStun;
        EffectState immune_effect_state{};
        immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
        GetAttachedEffectsHelper()
            .AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);
    }

    ASSERT_FALSE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));
    ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kStun));

    // Keep track of positions
    HexGridPosition blue_current_position = blue_position_component.ToSubUnits();
    HexGridPosition red_current_position = red_position_component.ToSubUnits();
    HexGridPosition blue_previous_position = blue_current_position;
    HexGridPosition red_previous_position = red_current_position;

    for (int i = 0; i < 6; i++)
    {
        events_apply_effect.clear();

        blue_previous_position = blue_current_position;
        red_previous_position = red_current_position;
        world->TimeStep();
        blue_current_position = blue_position_component.ToSubUnits();
        red_current_position = red_position_component.ToSubUnits();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        if (i < 3)
        {
            // Should be immune, positions should have not changed
            ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kStun)) << "i = " << i;

            ASSERT_TRUE(EntityHelper::IsMovable(*red_entity)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega)) << "i = " << i;
        }
        else
        {
            // Immune should be removed in time step 3
            if (i == 3)
            {
                ASSERT_EQ(events_remove_effect.size(), 1);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
                events_remove_effect.clear();
            }

            // Immune expired
            ASSERT_FALSE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kStun)) << "i = " << i;

            ASSERT_FALSE(EntityHelper::IsMovable(*red_entity)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega)) << "i = " << i;
        }

        if (i <= 3)
        {
            // Should have moved, both
            ASSERT_NE(red_current_position, red_previous_position) << "i = " << i;
            ASSERT_NE(blue_current_position, blue_previous_position) << "i = " << i;
        }
        else
        {
            // Red should not have moved but blue should have moved
            ASSERT_EQ(red_current_position, red_previous_position) << "i = " << i;
            ASSERT_NE(blue_current_position, blue_previous_position) << "i = " << i;
        }
    }
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be movable, fire ult/attack
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
}

TEST_F(AttachedEffectsSystemWithMovementTest, ApplyEffectNegativeStateRootWithAbsoluteImmune)
{
    // Get the relevant components
    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Add negative effect root
    const auto root_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kRoot, 600);
    EffectState root_effect_state{};
    root_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, root_effect_data, root_effect_state);

    // Add absolute immunity effect
    {
        const auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
        EffectState immune_effect_state{};
        immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
        GetAttachedEffectsHelper()
            .AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);
    }

    ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));
    ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kRoot));

    // Keep track of positions
    HexGridPosition blue_current_position = blue_position_component.ToSubUnits();
    HexGridPosition red_current_position = red_position_component.ToSubUnits();
    HexGridPosition blue_previous_position = blue_current_position;
    HexGridPosition red_previous_position = red_current_position;

    for (int i = 0; i < 6; i++)
    {
        events_apply_effect.clear();

        blue_previous_position = blue_current_position;
        red_previous_position = red_current_position;
        world->TimeStep();
        blue_current_position = blue_position_component.ToSubUnits();
        red_current_position = red_position_component.ToSubUnits();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be able to move
        if (i < 3)
        {
            // Should be immune, positions should have not changed
            ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kRoot)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::IsMovable(*red_entity)) << "i = " << i;
        }
        else
        {
            // Immune should be removed in time step 3
            if (i == 3)
            {
                ASSERT_EQ(events_remove_effect.size(), 1);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
                events_remove_effect.clear();
            }

            // Immune expired
            ASSERT_FALSE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kRoot)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::IsMovable(*red_entity)) << "i = " << i;
        }

        if (i <= 3)
        {
            // Should have moved, both
            ASSERT_NE(red_current_position, red_previous_position) << "i = " << i;
            ASSERT_NE(blue_current_position, blue_previous_position) << "i = " << i;
        }
        else
        {
            // Red should not have moved but blue should have moved
            ASSERT_EQ(red_current_position, red_previous_position) << "i = " << i;
            ASSERT_NE(blue_current_position, blue_previous_position) << "i = " << i;
        }
    }
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be movable
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
}

TEST_F(AttachedEffectsSystemWithMovementTest, ApplyEffectNegativeStateRootWithRootImmune)
{
    // Get the relevant components
    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Add negative effect root
    const auto root_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kRoot, 600);
    EffectState root_effect_state{};
    root_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, root_effect_data, root_effect_state);

    // Add root immunity effect
    {
        auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
        EffectTypeID& type_id = immune_effect_data.immuned_effect_types.emplace_back();
        type_id.type = EffectType::kNegativeState;
        type_id.negative_state = EffectNegativeState::kRoot;
        EffectState immune_effect_state{};
        immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
        GetAttachedEffectsHelper()
            .AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);
    }

    ASSERT_FALSE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));
    ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kRoot));

    // Keep track of positions
    HexGridPosition blue_current_position = blue_position_component.ToSubUnits();
    HexGridPosition red_current_position = red_position_component.ToSubUnits();
    HexGridPosition blue_previous_position = blue_current_position;
    HexGridPosition red_previous_position = red_current_position;

    for (int i = 0; i < 6; i++)
    {
        events_apply_effect.clear();

        blue_previous_position = blue_current_position;
        red_previous_position = red_current_position;
        world->TimeStep();
        blue_current_position = blue_position_component.ToSubUnits();
        red_current_position = red_position_component.ToSubUnits();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be able to move
        if (i < 3)
        {
            // Should be immune, positions should have not changed
            ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kRoot)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::IsMovable(*red_entity)) << "i = " << i;
        }
        else
        {
            // Immune should be removed in time step 3
            if (i == 3)
            {
                ASSERT_EQ(events_remove_effect.size(), 1);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
                events_remove_effect.clear();
            }

            // Immune expired
            ASSERT_FALSE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kRoot)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::IsMovable(*red_entity)) << "i = " << i;
        }

        if (i <= 3)
        {
            // Should have moved, both
            ASSERT_NE(red_current_position, red_previous_position) << "i = " << i;
            ASSERT_NE(blue_current_position, blue_previous_position) << "i = " << i;
        }
        else
        {
            // Red should not have moved but blue should have moved
            ASSERT_EQ(red_current_position, red_previous_position) << "i = " << i;
            ASSERT_NE(blue_current_position, blue_previous_position) << "i = " << i;
        }
    }
    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be movable
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateBlind)
{
    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));

    // Add a blind effect and wrap it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kBlind, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    for (int i = 0; i < 3; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world (3 time steps)
        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should be blinded
        ASSERT_TRUE(EntityHelper::IsBlinded(*red_entity));
    }

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should not be blinded anymore
    ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateBlindWithTruesight)
{
    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));

    // Add a blind effect and wrap it in an attached effect
    const auto blind_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kBlind, 300);
    EffectState blind_effect_state{};
    blind_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, blind_effect_data, blind_effect_state);

    // Add a truesight effect and wrap it in an attached effect
    const auto truesight_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kTruesight, 300);
    EffectState truesight_effect_state{};
    truesight_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, blue_entity_id, truesight_effect_data, truesight_effect_state);

    for (int i = 0; i < 3; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world (3 time steps)
        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be blinded
        ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));

        // Should have truesight
        ASSERT_TRUE(EntityHelper::HasTruesight(*red_entity));
    }

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should not be blinded
    ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));

    // Should not have truesight anymore
    ASSERT_FALSE(EntityHelper::HasTruesight(*red_entity));

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 2);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);
    ASSERT_EQ(events_remove_effect.at(1).data.type_id.type, EffectType::kPositiveState);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectTruesightCancelBlind)
{
    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));

    // Add a blind effect and wrap it in an attached effect
    const auto blind_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kBlind, 400);
    EffectState blind_effect_state{};
    blind_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, blind_effect_data, blind_effect_state);

    world->TimeStep();
    // Should be blinded
    ASSERT_TRUE(EntityHelper::IsBlinded(*red_entity));

    // Add a truesight effect and wrap it in an attached effect
    const auto truesight_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kTruesight, 200);
    EffectState truesight_effect_state{};
    truesight_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, blue_entity_id, truesight_effect_data, truesight_effect_state);

    for (int i = 0; i < 2; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world (2 time steps)
        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be blinded
        ASSERT_FALSE(EntityHelper::IsBlinded(*red_entity));

        // Should have truesight
        ASSERT_TRUE(EntityHelper::HasTruesight(*red_entity));
    }

    world->TimeStep();

    // Should be blinded
    ASSERT_TRUE(EntityHelper::IsBlinded(*red_entity));

    // Should not have truesight
    ASSERT_FALSE(EntityHelper::HasTruesight(*red_entity));

    // time step again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
    events_remove_effect.clear();
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateDisarm)
{
    // Validate initial state
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));

    // Add an Disarm effect and wrap it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kDisarm, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    for (int i = 0; i < 3; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world (3 TimeSteps)
        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be able to attack
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    }

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be able to attack ability
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateTaunted)
{
    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsTaunted(*red_entity));

    // Add an taunted negative state it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kTaunted, 300);

    TryToApplyEffect(blue_entity_id, red_entity_id, effect_data);

    for (int i = 0; i < 3; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world (3 TimeSteps)
        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should be taunted
        ASSERT_TRUE(EntityHelper::IsTaunted(*red_entity));
    }

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be able to ult attack
    ASSERT_FALSE(EntityHelper::IsTaunted(*red_entity));
}

TEST_F(AttachedEffectsSystemWithAttackEveryTimeStep, RemoveTauntedEffectAfterTargetDeath)
{
    const auto& red_focus_component = red_entity->Get<FocusComponent>();
    const auto& blue_stats_component = blue_entity->Get<StatsComponent>();

    // Add an taunted negative state it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kTaunted, 1000);
    TryToApplyEffect(blue_entity_id, red_entity_id, effect_data);

    // Check taunted effect
    ASSERT_TRUE(EntityHelper::IsTaunted(*red_entity));
    ASSERT_EQ(red_focus_component.GetFocus()->GetID(), blue_entity_id);
    ASSERT_EQ(events_apply_effect.size(), 1);

    // Red should be able to attack at first step
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));

    // First tick -> first blue is dead
    world->TimeStep();

    // First blue is dead
    ASSERT_EQ(blue_stats_component.GetCurrentHealth(), 0_fp);
    // Still have a taunted effect for one tick
    ASSERT_TRUE(EntityHelper::IsTaunted(*red_entity));
    // Focus is not active
    ASSERT_FALSE(red_focus_component.IsFocusActive());

    // Second tick -> taunted effect is removed, but don't have a new focus
    world->TimeStep();

    ASSERT_FALSE(EntityHelper::IsTaunted(*red_entity));
    ASSERT_FALSE(red_focus_component.GetFocus());

    // Third tick -> Found a new focus
    world->TimeStep();

    // Second blue entity now in a focus
    ASSERT_EQ(red_focus_component.GetFocus()->GetID(), blue_entity2_id);

    // Taunt effect + damage
    ASSERT_EQ(events_apply_effect.size(), 2);
    ASSERT_EQ(events_apply_effect.at(0).data.type_id.type, EffectType::kNegativeState);
    ASSERT_EQ(events_apply_effect.at(0).data.type_id.negative_state, EffectNegativeState::kTaunted);
    ASSERT_EQ(events_apply_effect.at(1).data.type_id.type, EffectType::kInstantDamage);

    // Have notification about removing taunted effect
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.negative_state, EffectNegativeState::kTaunted);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateSilenced)
{
    // Validate initial state
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kSilenced, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    for (int i = 0; i < 3; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world (3 time steps)
        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be able to ult
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    }

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be able to ult attack
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
}

TEST_F(AttachedEffectsSystemWithMovementTest, ApplyEffectNegativeStateStun)
{
    // Get the relevant components
    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();

    // Keep track of positions
    HexGridPosition blue_current_position = blue_position_component.ToSubUnits();
    HexGridPosition red_current_position = red_position_component.ToSubUnits();
    HexGridPosition blue_previous_position = blue_current_position;
    HexGridPosition red_previous_position = red_current_position;

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // Add an effect Wrap it in an attached effect
    Stun(*red_entity, 300);

    for (int i = 0; i < 3; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world (3 time step)
        blue_previous_position = blue_current_position;
        red_previous_position = red_current_position;
        world->TimeStep();
        blue_current_position = blue_position_component.ToSubUnits();
        red_current_position = red_position_component.ToSubUnits();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be able to attack
        ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));
        ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

        // Red should not have moved but blue should have moved
        ASSERT_EQ(red_current_position, red_previous_position);
        ASSERT_NE(blue_current_position, blue_previous_position);
    }

    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    blue_previous_position = blue_current_position;
    red_previous_position = red_current_position;
    world->TimeStep();
    blue_current_position = blue_position_component.ToSubUnits();
    red_current_position = red_position_component.ToSubUnits();

    // Red should not have moved but blue should have moved
    ASSERT_EQ(red_current_position, red_previous_position);
    ASSERT_NE(blue_current_position, blue_previous_position);

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be able to attack/ult attack
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // Check this effect does not reactivate
    blue_previous_position = blue_current_position;
    red_previous_position = red_current_position;
    world->TimeStep();
    blue_current_position = blue_position_component.ToSubUnits();
    red_current_position = red_position_component.ToSubUnits();

    // Can move
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_NE(red_current_position, red_previous_position);
    ASSERT_NE(blue_current_position, blue_previous_position);
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateLethargic)
{
    // Get the relevant components
    const auto& red_stats_component = red_entity->Get<StatsComponent>();
    const auto& blue_stats_component = blue_entity->Get<StatsComponent>();
    ASSERT_GT(red_stats_component.GetCurrentHealth(), 0_fp);
    ASSERT_GT(blue_stats_component.GetCurrentHealth(), 0_fp);

    // Validate initial state
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kLethargic, 300);
    EffectState effect_state{};
    effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, effect_data, effect_state);

    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 0_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), 0_fp);
    FixedPoint blue_expected_energy = 0_fp;
    for (int i = 0; i < 3; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // Time Step the world (3 TimeSteps)
        blue_expected_energy += 1_fp;
        world->TimeStep();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be able to gain energy
        ASSERT_FALSE(EntityHelper::CanGainEnergy(*red_entity));

        // Should not increase red, just the blue increases energy
        ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), blue_expected_energy);
        ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 0_fp);
    }
    ASSERT_FALSE(EntityHelper::CanGainEnergy(*red_entity));

    // Emit some gain energy events, should still gain no energy
    constexpr FixedPoint energy_gain = 100_fp;
    world->BuildAndEmitEvent<EventType::kApplyEnergyGain>(
        red_entity_id,
        red_entity_id,
        EnergyGainType::kOnTakeDamage,
        energy_gain);
    world->BuildAndEmitEvent<EventType::kApplyEnergyGain>(red_entity_id, red_entity_id, EnergyGainType::kAttack, 0_fp);
    // Should not increase red, just the blue increases energy
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), blue_expected_energy);
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 0_fp);

    // time step again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    blue_expected_energy += 1_fp;
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Should be able to gain energy
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));

    // Emit some gain energy events, should gain some energy now
    world->BuildAndEmitEvent<EventType::kApplyEnergyGain>(
        red_entity_id,
        red_entity_id,
        EnergyGainType::kOnTakeDamage,
        energy_gain);
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), energy_gain / 20_fp);
    world->BuildAndEmitEvent<EventType::kApplyEnergyGain>(red_entity_id, red_entity_id, EnergyGainType::kAttack, 0_fp);
    ASSERT_EQ(blue_stats_component.GetCurrentEnergy(), blue_expected_energy);
    ASSERT_EQ(red_stats_component.GetCurrentEnergy(), 10_fp + energy_gain / 20_fp);

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyEffectNegativeStateTauntedTimeInfinite)
{
    // Get the relevant components
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // Set tenacity to ensure kTimeInfinite is working properly
    red_stats_component.GetMutableTemplateStats().Set(StatType::kTenacityPercentage, 99_fp);

    // Validate initial state
    ASSERT_FALSE(EntityHelper::IsTaunted(*red_entity));

    // Add an effect Wrap it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kTaunted, kTimeInfinite);

    TryToApplyEffect(blue_entity_id, red_entity_id, effect_data);

    for (int i = 0; i < 5; i++)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world
        world->TimeStep();

        // Should be taunted
        ASSERT_TRUE(EntityHelper::IsTaunted(*red_entity));
    }

    // Should still be taunted
    ASSERT_TRUE(EntityHelper::IsTaunted(*red_entity));
}

TEST_F(AttachedEffectsSystemTest, ApplyMultipleNegativeEffects)
{
    // Get the relevant components
    auto& red_attached_effects_component = red_entity->Get<AttachedEffectsComponent>();
    auto& red_stats_component = red_entity->Get<StatsComponent>();

    // set sane values
    red_stats_component.SetCurrentHealth(100_fp);

    const auto blind_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kBlind, 300);
    EffectState blind_effect_state{};
    blind_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), blind_effect_data, blind_effect_state);

    const auto blind_effect_data2 = EffectData::CreateNegativeState(EffectNegativeState::kBlind, 400);
    EffectState blind_effect_state2{};
    blind_effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), blind_effect_data2, blind_effect_state2);

    const auto silenced_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kSilenced, 300);
    EffectState silenced_effect_state{};
    silenced_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), silenced_effect_data, silenced_effect_state);

    const auto silenced_effect_data2 = EffectData::CreateNegativeState(EffectNegativeState::kSilenced, 400);
    EffectState silenced_effect_state2{};
    silenced_effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), silenced_effect_data2, silenced_effect_state2);

    const auto disarm_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kDisarm, 300);
    EffectState disarm_effect_state{};
    disarm_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), disarm_effect_data, disarm_effect_state);

    const auto disarm_effect_data2 = EffectData::CreateNegativeState(EffectNegativeState::kDisarm, 400);
    EffectState disarm_effect_state2{};
    disarm_effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), disarm_effect_data2, disarm_effect_state2);

    const auto lethargic_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kLethargic, 300);
    EffectState lethargic_effect_state{};
    lethargic_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), lethargic_effect_data, lethargic_effect_state);

    const auto lethargic_effect_data2 = EffectData::CreateNegativeState(EffectNegativeState::kLethargic, 400);
    EffectState lethargic_effect_state2{};
    lethargic_effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), lethargic_effect_data2, lethargic_effect_state2);

    const auto root_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kRoot, 300);
    EffectState root_effect_state{};
    root_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity->GetID(), root_effect_data, root_effect_state);

    const auto root_effect_data2 = EffectData::CreateNegativeState(EffectNegativeState::kRoot, 400);
    EffectState root_effect_state2{};
    root_effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), root_effect_data2, root_effect_state2);

    const auto stun_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kStun, 300);
    EffectState stun_effect_state{};
    stun_effect_state.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, red_entity->GetID(), stun_effect_data, stun_effect_state);

    const auto stun_effect_data2 = EffectData::CreateNegativeState(EffectNegativeState::kStun, 400);
    EffectState stun_effect_state2{};
    stun_effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());
    GetAttachedEffectsHelper()
        .AddAttachedEffect(*red_entity, red_entity->GetID(), stun_effect_data2, stun_effect_state2);

    const auto taunted_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kTaunted, 300);

    TryToApplyEffect(blue_entity_id, red_entity_id, taunted_effect_data);

    const auto taunted_effect_data2 = EffectData::CreateNegativeState(EffectNegativeState::kTaunted, 400);
    EffectState taunted_effect_state2{};
    taunted_effect_state2.sender_stats = world->GetFullStats(red_entity->GetID());

    TryToApplyEffect(blue_entity_id, red_entity_id, taunted_effect_data2);

    ASSERT_TRUE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kBlind));
    ASSERT_TRUE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kSilenced));
    ASSERT_TRUE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kDisarm));
    ASSERT_TRUE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kLethargic));
    ASSERT_TRUE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kRoot));
    ASSERT_TRUE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kStun));
    ASSERT_TRUE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kTaunted));

    // Removed merged effects
    ASSERT_EQ(events_remove_effect.size(), 14);
    EXPECT_EQ(events_remove_effect.at(0).data.type_id.negative_state, EffectNegativeState::kBlind);
    EXPECT_EQ(events_remove_effect.at(1).data.type_id.negative_state, EffectNegativeState::kBlind);
    EXPECT_EQ(events_remove_effect.at(2).data.type_id.negative_state, EffectNegativeState::kSilenced);
    EXPECT_EQ(events_remove_effect.at(3).data.type_id.negative_state, EffectNegativeState::kSilenced);
    EXPECT_EQ(events_remove_effect.at(4).data.type_id.negative_state, EffectNegativeState::kDisarm);
    EXPECT_EQ(events_remove_effect.at(5).data.type_id.negative_state, EffectNegativeState::kDisarm);
    EXPECT_EQ(events_remove_effect.at(6).data.type_id.negative_state, EffectNegativeState::kLethargic);
    EXPECT_EQ(events_remove_effect.at(7).data.type_id.negative_state, EffectNegativeState::kLethargic);
    EXPECT_EQ(events_remove_effect.at(8).data.type_id.negative_state, EffectNegativeState::kRoot);
    EXPECT_EQ(events_remove_effect.at(9).data.type_id.negative_state, EffectNegativeState::kRoot);
    EXPECT_EQ(events_remove_effect.at(10).data.type_id.negative_state, EffectNegativeState::kStun);
    EXPECT_EQ(events_remove_effect.at(11).data.type_id.negative_state, EffectNegativeState::kStun);
    EXPECT_EQ(events_remove_effect.at(12).data.type_id.negative_state, EffectNegativeState::kTaunted);
    EXPECT_EQ(events_remove_effect.at(13).data.type_id.negative_state, EffectNegativeState::kTaunted);

    // TimeStep past events
    events_remove_effect.clear();
    for (int i = 0; i < 2; i++)
    {
        // TimeStep the world (3 TimeSteps)
        world->TimeStep();
    }

    // TimeStep again to remove first negative events
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);

    // Second negative events should still be present
    ASSERT_TRUE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kBlind));
    ASSERT_TRUE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kSilenced));
    ASSERT_TRUE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kDisarm));
    ASSERT_TRUE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kLethargic));
    ASSERT_TRUE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kRoot));
    ASSERT_TRUE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kStun));
    ASSERT_TRUE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kTaunted));

    // Timestep again to remove second negative effects
    events_remove_effect.clear();
    world->TimeStep();
    world->TimeStep();

    // Remove active effects
    ASSERT_EQ(events_remove_effect.size(), 7);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.negative_state, EffectNegativeState::kBlind);
    ASSERT_EQ(events_remove_effect.at(1).data.type_id.negative_state, EffectNegativeState::kSilenced);
    ASSERT_EQ(events_remove_effect.at(2).data.type_id.negative_state, EffectNegativeState::kDisarm);
    ASSERT_EQ(events_remove_effect.at(3).data.type_id.negative_state, EffectNegativeState::kLethargic);
    ASSERT_EQ(events_remove_effect.at(4).data.type_id.negative_state, EffectNegativeState::kRoot);
    ASSERT_EQ(events_remove_effect.at(5).data.type_id.negative_state, EffectNegativeState::kStun);
    ASSERT_EQ(events_remove_effect.at(6).data.type_id.negative_state, EffectNegativeState::kTaunted);

    ASSERT_FALSE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kBlind));
    ASSERT_FALSE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kSilenced));
    ASSERT_FALSE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kDisarm));
    ASSERT_FALSE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kLethargic));
    ASSERT_FALSE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kRoot));
    ASSERT_FALSE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kStun));
    ASSERT_FALSE(red_attached_effects_component.HasNegativeState(EffectNegativeState::kTaunted));
}

TEST_F(AttachedEffectsSystemWithMovementTest, ApplyEffectNegativeStateFrozen)
{
    // Get the relevant components
    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));
    ASSERT_FALSE(EntityHelper::IsFrozen(*red_entity));

    // Keep track of positions
    HexGridPosition red_current_position = red_position_component.ToSubUnits();
    HexGridPosition blue_current_position = blue_position_component.ToSubUnits();
    HexGridPosition red_previous_position = red_current_position;
    HexGridPosition blue_previous_position = blue_current_position;

    // Add negative effect Frozen
    const auto frozen_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kFrozen, 300);
    EffectState frozen_effect_state{};
    frozen_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, frozen_effect_data, frozen_effect_state);

    // Need to be Frozen
    ASSERT_TRUE(EntityHelper::IsFrozen(*red_entity));

    for (int i = 0; i < 3; ++i)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world (3 time steps)
        blue_previous_position = blue_current_position;
        red_previous_position = red_current_position;
        world->TimeStep();
        blue_current_position = blue_position_component.ToSubUnits();
        red_current_position = red_position_component.ToSubUnits();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be able to attack
        ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));
        ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
        ASSERT_FALSE(EntityHelper::CanGainEnergy(*red_entity));

        // Red should not have moved but blue should have moved
        ASSERT_EQ(red_current_position, red_previous_position);
        ASSERT_NE(blue_current_position, blue_previous_position);
    }

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);

    red_previous_position = red_current_position;
    blue_previous_position = blue_current_position;
    world->TimeStep();
    red_current_position = red_position_component.ToSubUnits();
    blue_current_position = blue_position_component.ToSubUnits();

    // Red should not have moved but blue should have moved
    ASSERT_EQ(red_current_position, red_previous_position);
    ASSERT_NE(blue_current_position, blue_previous_position);

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Effect should be gone
    ASSERT_FALSE(EntityHelper::IsFrozen(*red_entity));

    // Should be able to attack/ult attack, gain energy
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));

    // Check this effect does not reactivate
    blue_previous_position = blue_current_position;
    red_previous_position = red_current_position;
    world->TimeStep();
    blue_current_position = blue_position_component.ToSubUnits();
    red_current_position = red_position_component.ToSubUnits();

    // Can move
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));

    ASSERT_NE(red_current_position, red_previous_position);
    ASSERT_NE(blue_current_position, blue_previous_position);
}

TEST_F(AttachedEffectsSystemWithMovementTest, ApplyEffectNegativeStateFrozenWithAbsoluteImmune)
{
    // Get the relevant components
    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));
    ASSERT_FALSE(EntityHelper::IsFrozen(*red_entity));

    // Keep track of positions
    HexGridPosition red_current_position = red_position_component.ToSubUnits();
    HexGridPosition blue_current_position = blue_position_component.ToSubUnits();
    HexGridPosition red_previous_position = red_current_position;
    HexGridPosition blue_previous_position = blue_current_position;

    // Add negative effect Frozen
    const auto frozen_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kFrozen, 600);
    EffectState frozen_effect_state{};
    frozen_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, frozen_effect_data, frozen_effect_state);

    // Need to be Frozen
    ASSERT_TRUE(EntityHelper::IsFrozen(*red_entity));

    // Add positive effect (absolute immunity)
    {
        auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
        EffectState immune_effect_state{};
        immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
        GetAttachedEffectsHelper()
            .AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);
    }

    ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kFrozen));
    ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));

    for (int i = 0; i < 6; i++)
    {
        events_apply_effect.clear();

        blue_previous_position = blue_current_position;
        red_previous_position = red_current_position;
        world->TimeStep();
        blue_current_position = blue_position_component.ToSubUnits();
        red_current_position = red_position_component.ToSubUnits();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        if (i < 3)
        {
            // Should be immune, positions should have not changed
            ASSERT_TRUE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity)) << "i = " << i;

            ASSERT_TRUE(EntityHelper::IsMovable(*red_entity)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity)) << "i = " << i;
        }
        else
        {
            // Immune should be removed in time step 3
            if (i == 3)
            {
                ASSERT_EQ(events_remove_effect.size(), 1);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
                events_remove_effect.clear();
            }

            // Immune expired
            ASSERT_FALSE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity)) << "i = " << i;

            ASSERT_FALSE(EntityHelper::IsMovable(*red_entity)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanGainEnergy(*red_entity)) << "i = " << i;
        }

        if (i <= 3)
        {
            // Should have moved, both
            ASSERT_NE(red_current_position, red_previous_position) << "i = " << i;
            ASSERT_NE(blue_current_position, blue_previous_position) << "i = " << i;
        }
        else
        {
            // Red should not have moved but blue should have moved
            ASSERT_EQ(red_current_position, red_previous_position) << "i = " << i;
            ASSERT_NE(blue_current_position, blue_previous_position) << "i = " << i;
        }
    }

    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_FALSE(EntityHelper::CanGainEnergy(*red_entity));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Effect should be gone
    ASSERT_FALSE(EntityHelper::IsFrozen(*red_entity));

    // Should be movable, fire ult/attack
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));
}

TEST_F(AttachedEffectsSystemWithMovementTest, ApplyEffectNegativeStateFrozenWithFrozenImmune)
{
    // Get the relevant components
    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));
    ASSERT_FALSE(EntityHelper::IsFrozen(*red_entity));

    // Keep track of positions
    HexGridPosition red_current_position = red_position_component.ToSubUnits();
    HexGridPosition blue_current_position = blue_position_component.ToSubUnits();
    HexGridPosition red_previous_position = red_current_position;
    HexGridPosition blue_previous_position = blue_current_position;

    // Add negative effect Frozen
    const auto frozen_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kFrozen, 600);
    EffectState frozen_effect_state{};
    frozen_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, frozen_effect_data, frozen_effect_state);

    // Need to be Frozen
    ASSERT_TRUE(EntityHelper::IsFrozen(*red_entity));

    // Add positive effect (Immune to kFrozen)
    {
        auto immune_effect_data = EffectData::CreatePositiveState(EffectPositiveState::kImmune, 300);
        EffectTypeID& frozen_effect_type_id = immune_effect_data.immuned_effect_types.emplace_back();
        frozen_effect_type_id.type = EffectType::kNegativeState;
        frozen_effect_type_id.negative_state = EffectNegativeState::kFrozen;
        EffectState immune_effect_state{};
        immune_effect_state.sender_stats = world->GetFullStats(red_entity_id);
        GetAttachedEffectsHelper()
            .AddAttachedEffect(*red_entity, red_entity_id, immune_effect_data, immune_effect_state);
    }

    ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kFrozen));
    ASSERT_FALSE(EntityHelper::IsImmuneToAllDetrimentalEffects(*red_entity));

    for (int i = 0; i < 6; i++)
    {
        events_apply_effect.clear();

        blue_previous_position = blue_current_position;
        red_previous_position = red_current_position;
        world->TimeStep();
        blue_current_position = blue_position_component.ToSubUnits();
        red_current_position = red_position_component.ToSubUnits();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        if (i < 3)
        {
            // Should be immune, positions should have not changed
            ASSERT_TRUE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kFrozen))
                << "i = " << i;

            ASSERT_TRUE(EntityHelper::IsMovable(*red_entity)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega)) << "i = " << i;
            ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity)) << "i = " << i;
        }
        else
        {
            // Immune should be removed in time step 3
            if (i == 3)
            {
                ASSERT_EQ(events_remove_effect.size(), 1);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kPositiveState);
                ASSERT_EQ(events_remove_effect.at(0).data.type_id.positive_state, EffectPositiveState::kImmune);
                events_remove_effect.clear();
            }

            // Immune expired
            ASSERT_FALSE(EntityHelper::IsImmuneToNegativeState(*red_entity, EffectNegativeState::kFrozen))
                << "i = " << i;

            ASSERT_FALSE(EntityHelper::IsMovable(*red_entity)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega)) << "i = " << i;
            ASSERT_FALSE(EntityHelper::CanGainEnergy(*red_entity)) << "i = " << i;
        }

        if (i <= 3)
        {
            // Should have moved, both
            ASSERT_NE(red_current_position, red_previous_position) << "i = " << i;
            ASSERT_NE(blue_current_position, blue_previous_position) << "i = " << i;
        }
        else
        {
            // Red should not have moved but blue should have moved
            ASSERT_EQ(red_current_position, red_previous_position) << "i = " << i;
            ASSERT_NE(blue_current_position, blue_previous_position) << "i = " << i;
        }
    }

    ASSERT_FALSE(EntityHelper::IsMovable(*red_entity));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_FALSE(EntityHelper::CanGainEnergy(*red_entity));

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);
    world->TimeStep();

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Effect should be gone
    ASSERT_FALSE(EntityHelper::IsFrozen(*red_entity));

    // Should be movable, fire ult/attack
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));

    // Check this effect does not reactivate
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));
}

TEST_F(AttachedEffectsSystemWithMovementTest, ApplyEffectNegativeStateFlee)
{
    // Get the relevant components
    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();

    // Validate initial state
    ASSERT_TRUE(EntityHelper::IsMovable(*red_entity));
    ASSERT_TRUE(EntityHelper::IsMovable(*blue_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_TRUE(EntityHelper::CanGainEnergy(*red_entity));
    ASSERT_FALSE(EntityHelper::IsFleeing(*red_entity));

    // Keep track of positions
    HexGridPosition red_current_position = red_position_component.ToSubUnits();
    HexGridPosition blue_current_position = blue_position_component.ToSubUnits();
    HexGridPosition red_previous_position = red_current_position;
    HexGridPosition blue_previous_position = blue_current_position;
    int current_distance = (red_current_position - blue_current_position).Length();
    int previous_distance = current_distance;

    // Add negative effect Flee to red
    const auto flee_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kFlee, 300);
    EffectState flee_effect_state{};
    flee_effect_state.sender_stats = world->GetFullStats(blue_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*red_entity, blue_entity_id, flee_effect_data, flee_effect_state);

    // Should be Fleeing
    ASSERT_TRUE(EntityHelper::IsFleeing(*red_entity));

    // Add negative effect Root to blue, so it prevent it from moving
    const auto root_effect_data = EffectData::CreateNegativeState(EffectNegativeState::kRoot, 600);
    EffectState root_effect_state{};
    root_effect_state.sender_stats = world->GetFullStats(red_entity_id);
    GetAttachedEffectsHelper().AddAttachedEffect(*blue_entity, red_entity_id, root_effect_data, root_effect_state);
    ASSERT_FALSE(EntityHelper::IsMovable(*blue_entity));

    for (int i = 0; i < 3; ++i)
    {
        // Clear before each call
        events_apply_effect.clear();

        // TimeStep the world (3 time steps)
        blue_previous_position = blue_current_position;
        red_previous_position = red_current_position;
        previous_distance = current_distance;
        world->TimeStep();
        blue_current_position = blue_position_component.ToSubUnits();
        red_current_position = red_position_component.ToSubUnits();
        current_distance = (red_current_position - blue_current_position).Length();

        // Should not fire any event
        ASSERT_EQ(events_apply_effect.size(), 0) << "i = " << i;

        // Should not be able to attack
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
        ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

        // Red should have moved from a blue
        ASSERT_NE(red_current_position, red_previous_position);

        // Blue is rooted and keep its position
        ASSERT_EQ(blue_previous_position, blue_current_position);

        // Red should move away from blue
        ASSERT_GT(current_distance, previous_distance) << "i = " << i;
    }

    // TimeStep again
    events_apply_effect.clear();
    ASSERT_EQ(events_remove_effect.size(), 0);

    red_previous_position = red_current_position;
    blue_previous_position = blue_current_position;
    previous_distance = current_distance;
    world->TimeStep();
    red_current_position = red_position_component.ToSubUnits();
    blue_current_position = blue_position_component.ToSubUnits();
    current_distance = (red_current_position - blue_current_position).Length();

    ASSERT_NE(red_current_position, red_previous_position);
    ASSERT_EQ(blue_current_position, blue_previous_position);
    // This time step still move away from blue entity
    ASSERT_GT(current_distance, previous_distance);

    // Should have fired end event
    ASSERT_EQ(events_apply_effect.size(), 0);
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.type, EffectType::kNegativeState);

    // Effect should be gone
    ASSERT_FALSE(EntityHelper::IsFleeing(*red_entity));

    // Should be able to attack/ult attack, gain energy
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // Check this effect does not reactivate
    blue_previous_position = blue_current_position;
    red_previous_position = red_current_position;
    previous_distance = current_distance;
    world->TimeStep();
    blue_current_position = blue_position_component.ToSubUnits();
    red_current_position = red_position_component.ToSubUnits();
    current_distance = (red_current_position - blue_current_position).Length();

    // Can move
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    ASSERT_NE(red_current_position, red_previous_position);
    ASSERT_EQ(blue_current_position, blue_previous_position);
    // Should be closer to a target
    ASSERT_LT(current_distance, previous_distance);
}

TEST_F(AttachedEffectsSystemFirstBlueCloser, ApplyEffectNegativeStateCharm)
{
    const auto& red_focus_component = red_entity->Get<FocusComponent>();
    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();
    const auto& blue2_position_component = blue_entity2->Get<PositionComponent>();

    // Red should be able to attack at first step
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // First tick -> red focuses on closer blue
    world->TimeStep();
    ASSERT_EQ(red_focus_component.GetFocus()->GetID(), blue_entity_id);

    // Keep track of positions
    const HexGridPosition blue_current_position = blue_position_component.ToSubUnits();
    const HexGridPosition blue2_current_position = blue2_position_component.ToSubUnits();
    HexGridPosition red_current_position = red_position_component.ToSubUnits();
    int current_distance_blue = (red_current_position - blue_current_position).Length();
    int current_distance_blue2 = (red_current_position - blue2_current_position).Length();

    // First blue entity is closer
    ASSERT_LT(current_distance_blue, current_distance_blue2);

    // Add an charm negative state it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kCharm, 300);
    TryToApplyEffect(blue_entity2_id, red_entity_id, effect_data);

    // Check charmed effect
    ASSERT_TRUE(EntityHelper::IsCharmed(*red_entity));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_EQ(red_focus_component.GetFocus()->GetID(), blue_entity2_id);
    ASSERT_EQ(events_apply_effect.size(), 1);

    // TimeStep the world (3 time steps) while red under charm
    for (int i = 0; i < 3; ++i)
    {
        const int previous_distance_blue = current_distance_blue;
        const int previous_distance_blue2 = current_distance_blue2;

        world->TimeStep();

        red_current_position = red_position_component.ToSubUnits();
        current_distance_blue = (red_current_position - blue_current_position).Length();
        current_distance_blue2 = (red_current_position - blue2_current_position).Length();

        // Move away to first blue
        ASSERT_GT(current_distance_blue, previous_distance_blue);

        // Move closer from second blue
        ASSERT_LT(current_distance_blue2, previous_distance_blue2);
    }

    world->TimeStep();

    // Have notification about removing charm effect
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.negative_state, EffectNegativeState::kCharm);

    // TODO by design refocus should happen immediately
    // Resets focus after expiring charm
    ASSERT_FALSE(red_focus_component.GetFocus());

    world->TimeStep();

    // First blue is still closer so refocus on it
    ASSERT_EQ(red_focus_component.GetFocus()->GetID(), blue_entity_id);
}

TEST_F(AttachedEffectsSystemFirstBlueCloser, ApplyEffectNegativeStateTaunted)
{
    const auto& red_focus_component = red_entity->Get<FocusComponent>();
    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();
    const auto& blue2_position_component = blue_entity2->Get<PositionComponent>();

    // Red should be able to attack at first step
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // First tick -> red focuses on closer blue
    world->TimeStep();
    ASSERT_EQ(red_focus_component.GetFocus()->GetID(), blue_entity_id);

    // Keep track of positions
    const HexGridPosition blue_current_position = blue_position_component.ToSubUnits();
    const HexGridPosition blue2_current_position = blue2_position_component.ToSubUnits();
    HexGridPosition red_current_position = red_position_component.ToSubUnits();
    int current_distance_blue = (red_current_position - blue_current_position).Length();
    int current_distance_blue2 = (red_current_position - blue2_current_position).Length();

    // First blue entity is closer
    ASSERT_LT(current_distance_blue, current_distance_blue2);

    // Add an charm negative state it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kTaunted, 500);
    TryToApplyEffect(blue_entity2_id, red_entity_id, effect_data);

    // Check charmed effect
    ASSERT_TRUE(EntityHelper::IsTaunted(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_EQ(red_focus_component.GetFocus()->GetID(), blue_entity2_id);
    ASSERT_EQ(events_apply_effect.size(), 1);

    // TimeStep the world (5 time steps) while red under charm
    for (int i = 0; i < 5; ++i)
    {
        const int previous_distance_blue = current_distance_blue;
        const int previous_distance_blue2 = current_distance_blue2;

        world->TimeStep();

        red_current_position = red_position_component.ToSubUnits();
        current_distance_blue = (red_current_position - blue_current_position).Length();
        current_distance_blue2 = (red_current_position - blue2_current_position).Length();

        // Move away to first blue
        ASSERT_GT(current_distance_blue, previous_distance_blue);

        // Move closer from second blue
        ASSERT_LT(current_distance_blue2, previous_distance_blue2);
    }

    world->TimeStep();

    // Have notification about removing charm effect
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.negative_state, EffectNegativeState::kTaunted);

    // TODO by design refocus should happen immediately
    // Resets focus after expiring charm
    ASSERT_FALSE(red_focus_component.GetFocus());

    world->TimeStep();

    // First blue is still closer so refocus on it
    ASSERT_EQ(red_focus_component.GetFocus()->GetID(), blue_entity_id);
}

TEST_F(AttachedEffectsSystemFirstBlueCloserRanged, ApplyEffectNegativeStateCharm)
{
    const auto& red_focus_component = red_entity->Get<FocusComponent>();
    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();
    const auto& blue2_position_component = blue_entity2->Get<PositionComponent>();

    // Red should be able to attack at first step
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // First tick -> red focuses on closer blue
    world->TimeStep();
    ASSERT_EQ(red_focus_component.GetFocus()->GetID(), blue_entity_id);

    // Keep track of positions
    const HexGridPosition blue_current_position = blue_position_component.ToSubUnits();
    const HexGridPosition blue2_current_position = blue2_position_component.ToSubUnits();
    HexGridPosition red_current_position = red_position_component.ToSubUnits();
    int current_distance_blue = (red_current_position - blue_current_position).Length();
    int current_distance_blue2 = (red_current_position - blue2_current_position).Length();

    // First blue entity is closer
    ASSERT_LT(current_distance_blue, current_distance_blue2);

    // Add an charm negative state it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kCharm, 300);
    TryToApplyEffect(blue_entity2_id, red_entity_id, effect_data);

    // Check charmed effect
    ASSERT_TRUE(EntityHelper::IsCharmed(*red_entity));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_EQ(red_focus_component.GetFocus()->GetID(), blue_entity2_id);
    ASSERT_EQ(events_apply_effect.size(), 1);

    // TimeStep the world (3 time steps) while red under charm
    for (int i = 0; i < 3; ++i)
    {
        const int previous_distance_blue = current_distance_blue;
        const int previous_distance_blue2 = current_distance_blue2;

        world->TimeStep();

        red_current_position = red_position_component.ToSubUnits();
        current_distance_blue = (red_current_position - blue_current_position).Length();
        current_distance_blue2 = (red_current_position - blue2_current_position).Length();

        // Move away to first blue
        ASSERT_GT(current_distance_blue, previous_distance_blue);

        // Move closer from second blue
        ASSERT_LT(current_distance_blue2, previous_distance_blue2);
    }

    world->TimeStep();

    // Have notification about removing charm effect
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.negative_state, EffectNegativeState::kCharm);

    // We don't refocus because target in attack range
    ASSERT_EQ(red_focus_component.GetFocus()->GetID(), blue_entity2_id);
}

TEST_F(AttachedEffectsSystemFirstBlueCloserRanged, ApplyEffectNegativeStateTaunted)
{
    const auto& red_focus_component = red_entity->Get<FocusComponent>();
    const auto& red_position_component = red_entity->Get<PositionComponent>();
    const auto& blue_position_component = blue_entity->Get<PositionComponent>();
    const auto& blue2_position_component = blue_entity2->Get<PositionComponent>();

    // Red should be able to attack at first step
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));

    // First tick -> red focuses on closer blue
    world->TimeStep();
    ASSERT_EQ(red_focus_component.GetFocus()->GetID(), blue_entity_id);

    // Keep track of positions
    const HexGridPosition blue_current_position = blue_position_component.ToSubUnits();
    const HexGridPosition blue2_current_position = blue2_position_component.ToSubUnits();
    const HexGridPosition red_current_position = red_position_component.ToSubUnits();
    const int current_distance_blue = (red_current_position - blue_current_position).Length();
    const int current_distance_blue2 = (red_current_position - blue2_current_position).Length();

    // First blue entity is closer
    ASSERT_LT(current_distance_blue, current_distance_blue2);

    // Add an charm negative state it in an attached effect
    const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kTaunted, 500);
    TryToApplyEffect(blue_entity2_id, red_entity_id, effect_data);

    // Check taunted effect
    ASSERT_TRUE(EntityHelper::IsTaunted(*red_entity));
    ASSERT_TRUE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kAttack));
    ASSERT_FALSE(EntityHelper::CanActivateAbility(*red_entity, AbilityType::kOmega));
    ASSERT_EQ(red_focus_component.GetFocus()->GetID(), blue_entity2_id);
    ASSERT_EQ(events_apply_effect.size(), 1);

    // TimeStep the world (5 time steps) while red under charm
    for (int i = 0; i < 5; ++i)
    {
        const HexGridPosition previous_red_position = red_position_component.ToSubUnits();

        world->TimeStep();

        // We don't move because taunt sender is in attack range
        ASSERT_EQ(previous_red_position, red_position_component.ToSubUnits());
    }

    world->TimeStep();

    // Have notification about removing taunt effect
    ASSERT_EQ(events_remove_effect.size(), 1);
    ASSERT_EQ(events_remove_effect.at(0).data.type_id.negative_state, EffectNegativeState::kTaunted);

    // We don't refocus because target in attack range
    ASSERT_EQ(red_focus_component.GetFocus()->GetID(), blue_entity2_id);
}

}  // namespace simulation
