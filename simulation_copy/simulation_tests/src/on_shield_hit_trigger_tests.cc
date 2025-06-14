#include "base_test_fixtures.h"
#include "utility/stats_helper.h"

namespace simulation
{
class OnShieldHitTriggerTest : public BaseTest
{
public:
};

TEST_F(OnShieldHitTriggerTest, Basic)
{
    CombatUnitData data{};
    data.type_id.type = CombatUnitType::kIlluvial;
    data.type_data.combat_affinity = CombatAffinity::kWater;
    data.type_data.combat_class = CombatClass::kBulwark;
    data.type_data.tier = 0;

    StatsHelper::SetDefaults(data.type_data.stats);
    data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);
    data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, 0_fp);
    data.type_data.stats.Set(StatType::kAttackRangeUnits, 1000_fp);
    data.type_data.stats.Set(StatType::kStartingShield, 100_fp);
    data.type_data.stats.Set(StatType::kMaxHealth, 100_fp);
    data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);
    data.type_data.stats.Set(StatType::kAttackPhysicalDamage, 10_fp);
    data.type_data.stats.Set(StatType::kEnergyResist, 0_fp);

    // Attack ability
    {
        data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
        auto& ability = data.type_data.attack_abilities.AddAbility();
        ability.name = "Damage ability";
        auto& skill = ability.AddSkill();
        skill.name = "Damage Skill";
        skill.percentage_of_ability_duration = kMaxPercentage;
        skill.targeting.type = SkillTargetingType::kCurrentFocus;
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.deployment.guidance = EnumSet<GuidanceType>::MakeFull();
        skill.AddDamageEffect(
            EffectDamageType::kPhysical,
            EffectExpression::FromSenderLiveStat(StatType::kAttackDamage));
    }

    // Innate ability
    {
        data.type_data.innate_abilities.selection_type = AbilitySelectionType::kCycle;
        auto& ability = data.type_data.innate_abilities.AddAbility();
        ability.name = "Buff on shield hit";
        ability.total_duration_ms = 0;
        ability.activation_trigger_data.trigger_type = ActivationTriggerType::kOnShieldHit;
        ability.activation_trigger_data.sender_allegiance = AllegianceType::kSelf;
        ability.activation_trigger_data.receiver_allegiance = AllegianceType::kEnemy;
        auto& skill = ability.AddSkill();
        skill.name = "Energy resist buff";
        skill.percentage_of_ability_duration = kMaxPercentage;
        skill.targeting.type = SkillTargetingType::kSelf;
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.deployment.guidance = EnumSet<GuidanceType>::MakeFull();
        skill.AddEffect(
            EffectData::CreateBuff(StatType::kEnergyResist, EffectExpression::FromValue(10_fp), kTimeInfinite));
    }

    Entity* blue_entity = nullptr;
    constexpr HexGridPosition blue_position{5, 5};
    SpawnCombatUnit(Team::kBlue, blue_position, data, blue_entity);
    ASSERT_NE(blue_entity, nullptr);

    Entity* red_entity = nullptr;
    constexpr HexGridPosition red_position = Reflect(blue_position);
    SpawnCombatUnit(Team::kRed, red_position, data, red_entity);
    ASSERT_NE(red_entity, nullptr);

    Stun(*blue_entity);

    EventHistory<EventType::kShieldWasHit> shield_hit(*world);
    EventHistory<EventType::kAbilityActivated> activated(*world);
    EventHistory<EventType::kOnEffectApplied> effect(*world);

    world->TimeStep();

    // Check activated abilities
    ASSERT_EQ(activated.Size(), 2);

    {
        const auto& e = activated.events[0];
        ASSERT_EQ(e.sender_id, red_entity->GetID());
        ASSERT_EQ(e.ability_name, "Damage ability");
    }

    {
        const auto& e = activated.events[1];
        ASSERT_EQ(e.sender_id, red_entity->GetID());
        ASSERT_EQ(e.ability_name, "Buff on shield hit");
    }

    // Check applied effects
    ASSERT_EQ(effect.Size(), 2);

    {
        const auto& e = effect.events[0];
        ASSERT_EQ(e.data.type_id.type, EffectType::kInstantDamage);
        ASSERT_EQ(e.sender_id, red_entity->GetID());
        ASSERT_EQ(e.receiver_id, blue_entity->GetID());
    }

    {
        const auto& e = effect.events[1];
        ASSERT_EQ(e.data.type_id.type, EffectType::kBuff);
        ASSERT_EQ(e.sender_id, red_entity->GetID());
        ASSERT_EQ(e.receiver_id, red_entity->GetID());
    }

    // Check shield hit events
    {
        ASSERT_EQ(shield_hit.Size(), 1);
        const auto& e = shield_hit.events.front();
        EXPECT_EQ(e.receiver_id, blue_entity->GetID());
        EXPECT_EQ(e.sender_id, blue_entity->GetID());
        EXPECT_EQ(e.hit_by, red_entity->GetID());
    }
}
}  // namespace simulation
