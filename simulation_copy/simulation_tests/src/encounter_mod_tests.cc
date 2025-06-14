#include "base_test_fixtures.h"

namespace simulation
{

class EncounterModTest : public BaseTest
{
public:
    CombatUnitData MakeCombatUnitData(bool with_default_attack_ability = true)
    {
        CombatUnitData unit_data;
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

    void FillEncounterModsData(GameDataContainer& game_data) const override
    {
        auto data_ptr = std::make_shared<EncounterModData>();

        EncounterModData& data = *data_ptr;
        data.type_id.name = "EncounterA";
        data.type_id.stage = 0;

        auto ability = AbilityData::Create();
        ability->name = "EncounterA Ability";
        ability->activation_trigger_data.trigger_type = ActivationTriggerType::kOnActivateXAbilities;
        ability->activation_trigger_data.number_of_abilities_activated = 1;
        ability->activation_trigger_data.ability_types = MakeEnumSet(AbilityType::kAttack);
        ability->activation_trigger_data.every_x = true;

        auto& skill = ability->AddSkill();
        skill.SetDefaults(AbilityType::kInnate);
        skill.name = "Pure damage on every attack";
        skill.targeting.type = SkillTargetingType::kCurrentFocus;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.targeting.guidance = EnumSet<GuidanceType>::MakeFull();
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(10_fp));

        data.innate_abilities.emplace_back(ability);

        game_data.AddEncounterModData(data.type_id, data_ptr);
    }

    std::unordered_map<Team, std::vector<EncounterModInstanceData>> GetTeamsEncounterMods() const override
    {
        std::unordered_map<Team, std::vector<EncounterModInstanceData>> r;
        r[Team::kRed].push_back({"", EncounterModTypeID{"EncounterA"}});
        return r;
    }
};

TEST_F(EncounterModTest, Simple)
{
    CombatUnitData data = MakeCombatUnitData();

    Entity* red = Spawn(Team::kRed, data, {-35, 37});
    ASSERT_NE(red, nullptr);
    Entity* blue = Spawn(Team::kBlue, data, {35, 37});
    ASSERT_NE(blue, nullptr);

    world->TimeStep();

    auto& red_innates = red->Get<AbilitiesComponent>().GetDataInnateAbilities();
    ASSERT_EQ(red_innates.abilities.size(), 1);

    auto& blue_innates = blue->Get<AbilitiesComponent>().GetDataInnateAbilities();
    ASSERT_EQ(blue_innates.abilities.size(), 0);
}

TEST_F(EncounterModTest, Pets)
{
    constexpr bool with_default_attack_ability = false;
    CombatUnitData data = MakeCombatUnitData(with_default_attack_ability);

    // Add attack ability
    {
        auto& ability = data.type_data.attack_abilities.AddAbility();
        ability.name = "Spawn Pet Ability";
        ability.total_duration_ms = 100;

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Spawn Pet Skill";
        skill.targeting.type = SkillTargetingType::kSelf;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kSpawnedCombatUnit;
        skill.deployment.pre_deployment_delay_percentage = 0;

        auto combat_unit_ptr = std::make_shared<CombatUnitData>(MakeCombatUnitData());
        combat_unit_ptr->type_id.type = CombatUnitType::kPet;
        skill.spawn.combat_unit = combat_unit_ptr;
    }

    Entity* red = Spawn(Team::kRed, data, {-35, 37});
    ASSERT_NE(red, nullptr);
    Entity* blue = Spawn(Team::kBlue, data, {35, 37});
    ASSERT_NE(blue, nullptr);

    EventHistory<EventType::kCombatUnitCreated> combat_unit_created(*world);

    world->TimeStep();

    ASSERT_EQ(combat_unit_created.Size(), 2);

    EntityID red_pet_id = combat_unit_created[0].entity_id;
    EntityID blue_pet_id = combat_unit_created[1].entity_id;

    if (combat_unit_created[0].parent_id != red->GetID())
    {
        std::swap(red_pet_id, blue_pet_id);
    }

    const Entity* red_pet = world->GetByIDPtr(red_pet_id).get();
    ASSERT_NE(red_pet, nullptr);
    ASSERT_EQ(red_pet->GetParentID(), red->GetID());
    ASSERT_TRUE(red_pet->Has<AbilitiesComponent>());
    ASSERT_EQ(red_pet->Get<AbilitiesComponent>().GetDataInnateAbilities().abilities.size(), 1);

    const Entity* blue_pet = world->GetByIDPtr(blue_pet_id).get();
    ASSERT_NE(blue_pet, nullptr);
    ASSERT_EQ(blue_pet->GetParentID(), blue->GetID());
    ASSERT_TRUE(blue_pet->Has<AbilitiesComponent>());
    ASSERT_EQ(blue_pet->Get<AbilitiesComponent>().GetDataInnateAbilities().abilities.size(), 0);
}

}  // namespace simulation
