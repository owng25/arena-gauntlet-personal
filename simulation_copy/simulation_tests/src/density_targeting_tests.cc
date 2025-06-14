#include "base_test_fixtures.h"
#include "data/combat_unit_data.h"
#include "gtest/gtest.h"

namespace simulation
{

class DensityTargetingTest : public BaseTest
{
public:
    virtual CombatUnitData MakeCombatUnitData()
    {
        CombatUnitData unit_data;
        unit_data.type_id.line_name = "default";
        unit_data.radius_units = 2;
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
        return unit_data;
    }

    Entity* Spawn(const Team team, const CombatUnitData& unit_data, const HexGridPosition position)
    {
        Entity* entity = nullptr;
        SpawnCombatUnit(team, position, unit_data, entity);
        return entity;
    }
};

TEST_F(DensityTargetingTest, HexagonalZone_HighestDensity)
{
    CombatUnitData unit_data = MakeCombatUnitData();

    // Add attack abilities
    {
        auto& ability = unit_data.type_data.attack_abilities.AddAbility();
        ability.name = "Zone With Density Targeting Ability";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Zone Attack";
        skill.targeting.type = SkillTargetingType::kDensity;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = false;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kZone;
        skill.deployment.guidance = EnumSet<GuidanceType>::MakeFull();

        // Set the zone data
        skill.zone.shape = ZoneEffectShape::kHexagon;
        skill.zone.radius_units = 6;
        skill.zone.max_radius_units = 6;
        skill.zone.duration_ms = kTimeInfinite;
        skill.zone.frequency_ms = 100;
        skill.zone.apply_once = true;

        // More than the shield so we affect the health too
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(10_fp));

        ability.total_duration_ms = 1000;
        skill.deployment.pre_deployment_delay_percentage = 10;
    }

    unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    std::vector<Entity*> blue_entities;
    blue_entities.push_back(Spawn(Team::kBlue, unit_data, {0, -3}));
    ASSERT_NE(blue_entities.back(), nullptr);

    std::vector<Entity*> red_entities;

    const std::array<HexGridPosition, 6> red_spawn_positions{{
        {-72, 37},
        {-66, 37},
        {-60, 37},

        {-22, 37},
        {-16, 37},

        {38, 37},
    }};

    for (HexGridPosition position : red_spawn_positions)
    {
        red_entities.push_back(Spawn(Team::kRed, unit_data, position));
        ASSERT_NE(red_entities.back(), nullptr);
        Stun(*red_entities.back());
    }

    EventHistory<EventType::kZoneCreated> created_zones(*world);

    world->TimeStep();

    world->TimeStep();
    ASSERT_EQ(created_zones.Size(), 1);
    ASSERT_EQ(created_zones[0].position, HexGridPosition(-66, 37));
}

TEST_F(DensityTargetingTest, HexagonalZone_LowestDensity)
{
    CombatUnitData unit_data = MakeCombatUnitData();

    // Add attack abilities
    {
        auto& ability = unit_data.type_data.attack_abilities.AddAbility();
        ability.name = "Zone With Density Targeting Ability";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Zone Attack";
        skill.targeting.type = SkillTargetingType::kDensity;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = true;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kZone;
        skill.deployment.guidance = EnumSet<GuidanceType>::MakeFull();

        // Set the zone data
        skill.zone.shape = ZoneEffectShape::kHexagon;
        skill.zone.radius_units = 6;
        skill.zone.max_radius_units = 6;
        skill.zone.duration_ms = kTimeInfinite;
        skill.zone.frequency_ms = 100;
        skill.zone.apply_once = true;

        // More than the shield so we affect the health too
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(10_fp));

        ability.total_duration_ms = 1000;
        skill.deployment.pre_deployment_delay_percentage = 10;
    }

    unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    std::vector<Entity*> blue_entities;
    blue_entities.push_back(Spawn(Team::kBlue, unit_data, {0, -3}));
    ASSERT_NE(blue_entities.back(), nullptr);

    std::vector<Entity*> red_entities;

    const std::array<HexGridPosition, 6> red_spawn_positions{{
        {-72, 37},
        {-66, 37},
        {-60, 37},

        {-22, 37},
        {-16, 37},

        {38, 37},
    }};

    for (HexGridPosition position : red_spawn_positions)
    {
        red_entities.push_back(Spawn(Team::kRed, unit_data, position));
        ASSERT_NE(red_entities.back(), nullptr);
        Stun(*red_entities.back());
    }

    EventHistory<EventType::kZoneCreated> created_zones(*world);

    world->TimeStep();

    world->TimeStep();
    ASSERT_EQ(created_zones.Size(), 1);
    ASSERT_EQ(created_zones[0].position, HexGridPosition(38, 37));
}

TEST_F(DensityTargetingTest, RectangleZone_HighestDensity)
{
    CombatUnitData unit_data = MakeCombatUnitData();

    // Add attack abilities
    {
        auto& ability = unit_data.type_data.attack_abilities.AddAbility();
        ability.name = "Zone With Density Targeting Ability";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Zone Attack";
        skill.targeting.type = SkillTargetingType::kDensity;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = false;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kZone;
        skill.deployment.guidance = EnumSet<GuidanceType>::MakeFull();

        // Set the zone data
        skill.zone.shape = ZoneEffectShape::kRectangle;
        skill.zone.width_units = 12;
        skill.zone.height_units = 6;
        skill.zone.duration_ms = kTimeInfinite;
        skill.zone.frequency_ms = 100;
        skill.zone.apply_once = true;

        // More than the shield so we affect the health too
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(10_fp));

        ability.total_duration_ms = 1000;
        skill.deployment.pre_deployment_delay_percentage = 10;
    }

    unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    std::vector<Entity*> blue_entities;
    blue_entities.push_back(Spawn(Team::kBlue, unit_data, {0, -3}));
    ASSERT_NE(blue_entities.back(), nullptr);

    std::vector<Entity*> red_entities;

    const std::array<HexGridPosition, 6> red_spawn_positions{{
        {-72, 37},
        {-66, 37},
        {-60, 37},

        {-22, 37},
        {-16, 37},

        {38, 37},
    }};

    for (HexGridPosition position : red_spawn_positions)
    {
        red_entities.push_back(Spawn(Team::kRed, unit_data, position));
        ASSERT_NE(red_entities.back(), nullptr);
        Stun(*red_entities.back());
    }

    EventHistory<EventType::kZoneCreated> created_zones(*world);

    world->TimeStep();

    world->TimeStep();
    ASSERT_EQ(created_zones.Size(), 1);
    ASSERT_EQ(created_zones[0].position, HexGridPosition(-66, 37));
};

TEST_F(DensityTargetingTest, RectangleZone_LowestDensity)
{
    CombatUnitData unit_data = MakeCombatUnitData();

    // Add attack abilities
    {
        auto& ability = unit_data.type_data.attack_abilities.AddAbility();
        ability.name = "Zone With Density Targeting Ability";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Zone Attack";
        skill.targeting.type = SkillTargetingType::kDensity;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = true;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kZone;
        skill.deployment.guidance = EnumSet<GuidanceType>::MakeFull();

        // Set the zone data
        skill.zone.shape = ZoneEffectShape::kRectangle;
        skill.zone.width_units = 12;
        skill.zone.height_units = 6;
        skill.zone.duration_ms = kTimeInfinite;
        skill.zone.frequency_ms = 100;
        skill.zone.apply_once = true;

        // More than the shield so we affect the health too
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(10_fp));

        ability.total_duration_ms = 1000;
        skill.deployment.pre_deployment_delay_percentage = 10;
    }

    unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    std::vector<Entity*> blue_entities;
    blue_entities.push_back(Spawn(Team::kBlue, unit_data, {0, -3}));
    ASSERT_NE(blue_entities.back(), nullptr);

    std::vector<Entity*> red_entities;

    const std::array<HexGridPosition, 6> red_spawn_positions{{
        {-72, 37},
        {-66, 37},
        {-60, 37},

        {-22, 37},
        {-16, 37},

        {38, 37},
    }};

    for (HexGridPosition position : red_spawn_positions)
    {
        red_entities.push_back(Spawn(Team::kRed, unit_data, position));
        ASSERT_NE(red_entities.back(), nullptr);
        Stun(*red_entities.back());
    }

    EventHistory<EventType::kZoneCreated> created_zones(*world);

    world->TimeStep();

    world->TimeStep();
    ASSERT_EQ(created_zones.Size(), 1);
    ASSERT_EQ(created_zones[0].position, HexGridPosition(38, 37));
}

TEST_F(DensityTargetingTest, TriangleZone_HighestDensity)
{
    CombatUnitData unit_data = MakeCombatUnitData();

    // Add attack abilities
    {
        auto& ability = unit_data.type_data.attack_abilities.AddAbility();
        ability.name = "Zone With Density Targeting Ability";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Zone Attack";
        skill.targeting.type = SkillTargetingType::kDensity;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = false;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kZone;
        skill.deployment.guidance = EnumSet<GuidanceType>::MakeFull();

        // Set the zone data
        skill.zone.shape = ZoneEffectShape::kTriangle;
        skill.zone.radius_units = 12;
        skill.zone.max_radius_units = 12;
        skill.zone.direction_degrees = 0;
        skill.zone.duration_ms = kTimeInfinite;
        skill.zone.frequency_ms = 100;
        skill.zone.apply_once = true;

        // More than the shield so we affect the health too
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(10_fp));

        ability.total_duration_ms = 1000;
        skill.deployment.pre_deployment_delay_percentage = 10;
    }

    unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    std::vector<Entity*> blue_entities;
    blue_entities.push_back(Spawn(Team::kBlue, unit_data, {0, -3}));
    ASSERT_NE(blue_entities.back(), nullptr);

    std::vector<Entity*> red_entities;

    const std::array<HexGridPosition, 6> red_spawn_positions{{
        {-22, 37},
        {-25, 43},

        {38, 37},

        {-71, 48},
        {-72, 41},
        {-60, 37},
    }};

    for (HexGridPosition position : red_spawn_positions)
    {
        red_entities.push_back(Spawn(Team::kRed, unit_data, position));
        ASSERT_NE(red_entities.back(), nullptr);
        Stun(*red_entities.back());
    }

    EventHistory<EventType::kZoneCreated> created_zones(*world);

    world->TimeStep();

    world->TimeStep();
    ASSERT_EQ(created_zones.Size(), 1);
    ASSERT_EQ(created_zones[0].position, HexGridPosition(-60, 37));
}

TEST_F(DensityTargetingTest, TriangleZone_LowestDensity)
{
    CombatUnitData unit_data = MakeCombatUnitData();

    // Add attack abilities
    {
        auto& ability = unit_data.type_data.attack_abilities.AddAbility();
        ability.name = "Zone With Density Targeting Ability";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Zone Attack";
        skill.targeting.type = SkillTargetingType::kDensity;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = true;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kZone;
        skill.deployment.guidance = EnumSet<GuidanceType>::MakeFull();

        // Set the zone data
        skill.zone.shape = ZoneEffectShape::kTriangle;
        skill.zone.radius_units = 12;
        skill.zone.max_radius_units = 12;
        skill.zone.direction_degrees = 0;
        skill.zone.duration_ms = kTimeInfinite;
        skill.zone.frequency_ms = 100;
        skill.zone.apply_once = true;

        // More than the shield so we affect the health too
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(10_fp));

        ability.total_duration_ms = 1000;
        skill.deployment.pre_deployment_delay_percentage = 10;
    }

    unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    std::vector<Entity*> blue_entities;
    blue_entities.push_back(Spawn(Team::kBlue, unit_data, {0, -3}));
    ASSERT_NE(blue_entities.back(), nullptr);

    std::vector<Entity*> red_entities;

    const std::array<HexGridPosition, 3> red_spawn_positions{{
        {-4, 5},
        {-6, 10},
        {-8, 15},
    }};

    for (HexGridPosition position : red_spawn_positions)
    {
        red_entities.push_back(Spawn(Team::kRed, unit_data, position));
        ASSERT_NE(red_entities.back(), nullptr);
        Stun(*red_entities.back());
    }

    EventHistory<EventType::kZoneCreated> created_zones(*world);

    world->TimeStep();

    world->TimeStep();
    ASSERT_EQ(created_zones.Size(), 1);
    ASSERT_EQ(created_zones[0].position, HexGridPosition(-8, 15));
}

TEST_F(DensityTargetingTest, Beam_HighestDensity)
{
    CombatUnitData unit_data = MakeCombatUnitData();

    // Add attack abilities
    {
        auto& ability = unit_data.type_data.attack_abilities.AddAbility();
        ability.name = "Beam With Density Targeting Ability";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Beam Attack";
        skill.targeting.type = SkillTargetingType::kDensity;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = false;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kBeam;
        skill.deployment.guidance = EnumSet<GuidanceType>::Create(GuidanceType::kGround);

        // Set beam data
        skill.beam.apply_once = true;
        skill.beam.frequency_ms = 100;
        skill.beam.width_units = 3;

        // More than the shield so we affect the health too
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(10_fp));

        ability.total_duration_ms = 1000;
        skill.deployment.pre_deployment_delay_percentage = 10;
    }

    unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    std::vector<Entity*> blue_entities;
    blue_entities.push_back(Spawn(Team::kBlue, unit_data, {0, -3}));
    ASSERT_NE(blue_entities.back(), nullptr);

    std::vector<Entity*> red_entities;

    const std::array<HexGridPosition, 6> red_spawn_positions{{
        {38, 37},

        {-22, 37},
        {-25, 43},

        {-54, 33},
        {-60, 37},
        {-66, 41},
    }};

    for (HexGridPosition position : red_spawn_positions)
    {
        red_entities.push_back(Spawn(Team::kRed, unit_data, position));
        ASSERT_NE(red_entities.back(), nullptr);
        Stun(*red_entities.back());
    }

    EventHistory<EventType::kBeamCreated> created_zones(*world);

    world->TimeStep();
    world->TimeStep();

    ASSERT_EQ(created_zones.Size(), 1);
    ASSERT_EQ(created_zones[0].receiver_id, red_entities.back()->GetID());
}

TEST_F(DensityTargetingTest, Beam_LowestDensity)
{
    CombatUnitData unit_data = MakeCombatUnitData();

    // Add attack abilities
    {
        auto& ability = unit_data.type_data.attack_abilities.AddAbility();
        ability.name = "Beam With Density Targeting Ability";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Beam Attack";
        skill.targeting.type = SkillTargetingType::kDensity;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = true;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kBeam;
        skill.deployment.guidance = EnumSet<GuidanceType>::Create(GuidanceType::kGround);

        // Set beam data
        skill.beam.apply_once = true;
        skill.beam.frequency_ms = 100;
        skill.beam.width_units = 12;

        // More than the shield so we affect the health too
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(10_fp));

        ability.total_duration_ms = 1000;
        skill.deployment.pre_deployment_delay_percentage = 10;
    }

    unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    std::vector<Entity*> blue_entities;
    blue_entities.push_back(Spawn(Team::kBlue, unit_data, {0, -3}));
    ASSERT_NE(blue_entities.back(), nullptr);

    std::vector<Entity*> red_entities;

    const std::array<HexGridPosition, 3> red_spawn_positions{{
        {-18, 33},
        {-13, 33},

        {15, 33},
    }};

    for (HexGridPosition position : red_spawn_positions)
    {
        red_entities.push_back(Spawn(Team::kRed, unit_data, position));
        ASSERT_NE(red_entities.back(), nullptr);
        Stun(*red_entities.back());
    }

    EventHistory<EventType::kBeamCreated> created_zones(*world);

    world->TimeStep();
    world->TimeStep();

    ASSERT_EQ(created_zones.Size(), 1);
    ASSERT_EQ(created_zones[0].receiver_id, red_entities.back()->GetID());
}

TEST_F(DensityTargetingTest, Dash_HighestDensity)
{
    CombatUnitData unit_data = MakeCombatUnitData();

    // Add attack abilities
    {
        auto& ability = unit_data.type_data.attack_abilities.AddAbility();
        ability.name = "Dash With Density Targeting Ability";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Dash Attack";
        skill.targeting.type = SkillTargetingType::kDensity;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = false;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kDash;
        skill.deployment.guidance = EnumSet<GuidanceType>::Create(GuidanceType::kGround);

        // Set dash data
        skill.dash.apply_to_all = true;
        skill.dash.land_behind = true;

        // More than the shield so we affect the health too
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(10_fp));

        ability.total_duration_ms = 1000;
        skill.deployment.pre_deployment_delay_percentage = 10;
    }

    unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    std::vector<Entity*> blue_entities;
    blue_entities.push_back(Spawn(Team::kBlue, unit_data, {0, -3}));
    ASSERT_NE(blue_entities.back(), nullptr);

    std::vector<Entity*> red_entities;

    const std::array<HexGridPosition, 6> red_spawn_positions{{
        {38, 37},

        {-22, 37},
        {-25, 43},

        {-58, 35},
        {-63, 36},
        {-66, 41},
    }};

    for (HexGridPosition position : red_spawn_positions)
    {
        red_entities.push_back(Spawn(Team::kRed, unit_data, position));
        ASSERT_NE(red_entities.back(), nullptr);
        Stun(*red_entities.back());
    }

    EventHistory<EventType::kDashCreated> created_dashes(*world);

    world->TimeStep();
    world->TimeStep();
    ASSERT_EQ(created_dashes.Size(), 1);
    ASSERT_EQ(created_dashes[0].receiver_id, red_entities.back()->GetID());
}

TEST_F(DensityTargetingTest, Dash_LowestDensity)
{
    CombatUnitData unit_data = MakeCombatUnitData();

    // Add attack abilities
    {
        auto& ability = unit_data.type_data.attack_abilities.AddAbility();
        ability.name = "Dash With Density Targeting Ability";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Dash Attack";
        skill.targeting.type = SkillTargetingType::kDensity;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = true;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kDash;
        skill.deployment.guidance = EnumSet<GuidanceType>::Create(GuidanceType::kGround);

        // Set dash data
        skill.dash.apply_to_all = true;
        skill.dash.land_behind = true;

        // More than the shield so we affect the health too
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(10_fp));

        ability.total_duration_ms = 1000;
        skill.deployment.pre_deployment_delay_percentage = 10;
    }

    unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    std::vector<Entity*> blue_entities;
    blue_entities.push_back(Spawn(Team::kBlue, unit_data, {0, -3}));
    ASSERT_NE(blue_entities.back(), nullptr);

    std::vector<Entity*> red_entities;

    const std::array<HexGridPosition, 6> red_spawn_positions{{
        {38, 37},

        {-22, 37},
        {-25, 43},

        {-54, 33},
        {-60, 37},
        {-66, 41},
    }};

    for (HexGridPosition position : red_spawn_positions)
    {
        red_entities.push_back(Spawn(Team::kRed, unit_data, position));
        ASSERT_NE(red_entities.back(), nullptr);
        Stun(*red_entities.back());
    }

    EventHistory<EventType::kDashCreated> created_dashes(*world);

    world->TimeStep();
    world->TimeStep();
    ASSERT_EQ(created_dashes.Size(), 1);
    ASSERT_EQ(created_dashes[0].receiver_id, red_entities.front()->GetID());
}

TEST_F(DensityTargetingTest, Projectile_HighestDensity)
{
    CombatUnitData unit_data = MakeCombatUnitData();

    // Add attack abilities
    {
        auto& ability = unit_data.type_data.attack_abilities.AddAbility();
        ability.name = "Projectile ability with highest density targeting";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Projectile Attack";
        skill.targeting.type = SkillTargetingType::kDensity;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = false;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kProjectile;
        skill.deployment.guidance = EnumSet<GuidanceType>::Create(GuidanceType::kGround);

        // Set dash data
        skill.projectile.apply_to_all = true;
        skill.projectile.is_blockable = false;
        skill.projectile.is_homing = false;
        skill.projectile.size_units = 3;
        skill.projectile.speed_sub_units = Math::UnitsToSubUnits(8);

        // More than the shield so we affect the health too
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(10_fp));

        ability.total_duration_ms = 1000;
        skill.deployment.pre_deployment_delay_percentage = 10;
    }

    unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    std::vector<Entity*> blue_entities;
    blue_entities.push_back(Spawn(Team::kBlue, unit_data, {0, -3}));
    ASSERT_NE(blue_entities.back(), nullptr);

    std::vector<Entity*> red_entities;

    const std::array<HexGridPosition, 6> red_spawn_positions{{
        {38, 37},

        {-22, 37},
        {-25, 43},

        {-54, 33},
        {-60, 37},
        {-66, 41},
    }};

    for (HexGridPosition position : red_spawn_positions)
    {
        red_entities.push_back(Spawn(Team::kRed, unit_data, position));
        ASSERT_NE(red_entities.back(), nullptr);
        Stun(*red_entities.back());
    }

    EventHistory<EventType::kProjectileCreated> created_projectiles(*world);

    world->TimeStep();
    world->TimeStep();
    ASSERT_EQ(created_projectiles.Size(), 1);
    ASSERT_EQ(created_projectiles[0].receiver_id, red_entities.back()->GetID());
}

TEST_F(DensityTargetingTest, Projectile_LowestDensity)
{
    CombatUnitData unit_data = MakeCombatUnitData();

    // Add attack abilities
    {
        auto& ability = unit_data.type_data.attack_abilities.AddAbility();
        ability.name = "Projectile ability with highest density targeting";

        // Skills
        auto& skill = ability.AddSkill();
        skill.SetDefaults(AbilityType::kAttack);
        skill.name = "Projectile Attack";
        skill.targeting.type = SkillTargetingType::kDensity;
        skill.targeting.group = AllegianceType::kEnemy;
        skill.targeting.lowest = true;
        skill.targeting.num = 1;
        skill.deployment.type = SkillDeploymentType::kProjectile;
        skill.deployment.guidance = EnumSet<GuidanceType>::Create(GuidanceType::kGround);

        // Set dash data
        skill.projectile.apply_to_all = true;
        skill.projectile.is_blockable = false;
        skill.projectile.is_homing = false;
        skill.projectile.size_units = 3;
        skill.projectile.speed_sub_units = Math::UnitsToSubUnits(8);

        // More than the shield so we affect the health too
        skill.AddDamageEffect(EffectDamageType::kPure, EffectExpression::FromValue(10_fp));

        ability.total_duration_ms = 1000;
        skill.deployment.pre_deployment_delay_percentage = 10;
    }

    unit_data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;

    std::vector<Entity*> blue_entities;
    blue_entities.push_back(Spawn(Team::kBlue, unit_data, {0, -3}));
    ASSERT_NE(blue_entities.back(), nullptr);

    std::vector<Entity*> red_entities;

    const std::array<HexGridPosition, 6> red_spawn_positions{{
        {38, 37},

        {-22, 37},
        {-25, 43},

        {-54, 33},
        {-60, 37},
        {-66, 41},
    }};

    for (HexGridPosition position : red_spawn_positions)
    {
        red_entities.push_back(Spawn(Team::kRed, unit_data, position));
        ASSERT_NE(red_entities.back(), nullptr);
        Stun(*red_entities.back());
    }

    EventHistory<EventType::kProjectileCreated> created_projectiles(*world);

    world->TimeStep();
    world->TimeStep();
    ASSERT_EQ(created_projectiles.Size(), 1);
    ASSERT_EQ(created_projectiles[0].receiver_id, red_entities.front()->GetID());
}
}  // namespace simulation
