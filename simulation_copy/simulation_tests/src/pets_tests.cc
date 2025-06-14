#include "base_test_fixtures.h"

namespace simulation
{

class PetTest : public BaseTest
{
public:
    CombatUnitData MakeCombatUnitData(bool with_default_attack_ability = true)
    {
        CombatUnitData unit_data = CreateCombatUnitData();
        unit_data.type_id.line_name = "default";
        unit_data.radius_units = 1;
        unit_data.type_data.combat_affinity = CombatAffinity::kFire;
        unit_data.type_data.combat_class = CombatClass::kBehemoth;
        unit_data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 300_fp);
        unit_data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 5_fp);
        unit_data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);             // No critical attacks
        unit_data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);      // No Dodge
        unit_data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);  // No Miss
        unit_data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);                   // Once every tick
        unit_data.type_data.stats.Set(
            StatType::kAttackRangeUnits,
            1000_fp);                                                      // Very big attack radius so no need to move
        unit_data.type_data.stats.Set(StatType::kOmegaRangeUnits, 50_fp);  // Very big omega radius so no need to move
        unit_data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);
        unit_data.type_data.stats.Set(StatType::kEnergyCost, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP);
        unit_data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
        unit_data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);  // don't move

        // Add attack abilities
        if (with_default_attack_ability)
        {
            auto& ability = unit_data.type_data.attack_abilities.AddAbility();
            ability.name = "Simple attack ability";

            // Skills
            auto& skill = ability.AddSkill();
            skill.SetDefaults(AbilityType::kAttack);
            skill.name = "Energy damage";
            skill.targeting.type = SkillTargetingType::kCurrentFocus;
            skill.targeting.group = AllegianceType::kEnemy;
            skill.targeting.num = 1;
            skill.deployment.type = SkillDeploymentType::kDirect;
            skill.deployment.guidance = EnumSet<GuidanceType>::MakeFull();
            skill.AddDamageEffect(EffectDamageType::kEnergy, EffectExpression::FromValue(100_fp));

            ability.total_duration_ms = 100;
            skill.deployment.pre_deployment_delay_percentage = 0;
        }

        return unit_data;
    }

    Entity* Spawn(const Team team, const CombatUnitData& unit_data, const HexGridPosition position)
    {
        Entity* entity = nullptr;
        SpawnCombatUnit(team, position, unit_data, entity);
        return entity;
    }
};

TEST_F(PetTest, TriggerWhenOwnerFaints)
{
    CombatUnitData pet_data = MakeCombatUnitData(true);

    // Add pet innate - buff self when parent faints
    {
        auto& ability = pet_data.type_data.innate_abilities.AddAbility();
        ability.name = "Buff self ability";
        ability.total_duration_ms = 0;

        ability.activation_trigger_data.trigger_type = ActivationTriggerType::kOnFaint;
        ability.activation_trigger_data.only_from_parent = true;

        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kInnate);
        skill.name = "Buff self";
        skill.targeting.type = SkillTargetingType::kSelf;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.deployment.pre_deployment_retargeting_percentage = 0;
        skill.AddEffect(
            EffectData::CreateBuff(StatType::kAttackDamage, EffectExpression::FromValue(10_fp), kTimeInfinite));
    }

    // Create a combat unit data. All this combat unit does is innate ability to spawn pet on combat start
    CombatUnitData parent_data = MakeCombatUnitData(false);

    // Add spawn pet innate
    {
        auto& ability = parent_data.type_data.innate_abilities.AddAbility();
        ability.name = "Spawn Pet Ability";
        ability.total_duration_ms = 100;

        ability.activation_trigger_data.trigger_type = ActivationTriggerType::kOnBattleStart;

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Spawn Pet Skill";
        skill.targeting.type = SkillTargetingType::kDistanceCheck;
        skill.targeting.num = 1;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = false;
        skill.deployment.type = SkillDeploymentType::kSpawnedCombatUnit;
        skill.deployment.pre_deployment_delay_percentage = 0;

        auto combat_unit_ptr = std::make_shared<CombatUnitData>(pet_data);
        combat_unit_ptr->type_id.type = CombatUnitType::kPet;
        skill.spawn.combat_unit = combat_unit_ptr;
    }

    parent_data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);
    Entity* red = Spawn(Team::kRed, parent_data, {-35, 37});
    ASSERT_NE(red, nullptr);
    const EntityID red_id = red->GetID();

    parent_data.type_data.stats.Set(StatType::kMaxHealth, 2000_fp);
    Entity* blue = Spawn(Team::kBlue, parent_data, {35, 37});
    ASSERT_NE(blue, nullptr);
    const EntityID blue_id = blue->GetID();

    EventHistory<EventType::kCombatUnitCreated> combat_unit_created(*world);
    EventHistory<EventType::kFainted> combat_unit_fainted(*world);

    ASSERT_TRUE(TimeStepUntilEvent<EventType::kCombatUnitCreated>());
    ASSERT_EQ(combat_unit_created.Size(), 2);

    EntityID red_pet_id = combat_unit_created[0].entity_id;
    EntityID blue_pet_id = combat_unit_created[1].entity_id;

    if (combat_unit_created[0].parent_id != red->GetID())
    {
        std::swap(red_pet_id, blue_pet_id);
    }

    auto get_attack_damage = [&](const EntityID& id) -> FixedPoint
    {
        return world->GetLiveStats(id).Get(StatType::kAttackDamage);
    };

    const Entity* red_pet = world->GetByIDPtr(red_pet_id).get();
    ASSERT_NE(red_pet, nullptr);
    ASSERT_EQ(red_pet->GetParentID(), red->GetID());
    ASSERT_EQ(get_attack_damage(red_pet_id), 0_fp);

    const Entity* blue_pet = world->GetByIDPtr(blue_pet_id).get();
    ASSERT_NE(blue_pet, nullptr);
    ASSERT_EQ(blue_pet->GetParentID(), blue->GetID());
    ASSERT_EQ(get_attack_damage(blue_pet_id), 0_fp);

    // First faint: only one obtains attack damage buff
    ASSERT_TRUE(TimeStepUntilEvent<EventType::kFainted>());
    ASSERT_EQ(combat_unit_fainted.Size(), 1);
    ASSERT_EQ(combat_unit_fainted[0].entity_id, red_id);
    ASSERT_EQ(get_attack_damage(blue_pet_id), 0_fp);
    ASSERT_NE(get_attack_damage(red_pet_id), 0_fp);

    // Second faint: both will have attack damage buff
    ASSERT_TRUE(TimeStepUntilEvent<EventType::kFainted>());
    ASSERT_EQ(combat_unit_fainted.Size(), 2);
    ASSERT_EQ(combat_unit_fainted[1].entity_id, blue_id);
    ASSERT_NE(get_attack_damage(blue_pet_id), 0_fp);
    ASSERT_NE(get_attack_damage(red_pet_id), 0_fp);
}

}  // namespace simulation
