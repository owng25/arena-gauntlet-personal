#include "ability_system_data_fixtures.h"
#include "components/position_component.h"
#include "data/enums.h"
#include "gtest/gtest.h"
#include "systems/effect_system.h"
#include "test_data_loader.h"

namespace simulation
{
class ExecuteEffectTest : public BaseTest
{
protected:
    static CombatUnitData MakeUnitDataWithDefaultStats()
    {
        CombatUnitData data = CreateCombatUnitData();

        data.radius_units = 5;
        data.type_data.combat_affinity = CombatAffinity::kFire;
        data.type_data.combat_class = CombatClass::kBehemoth;
        data.type_data.stats.Set(StatType::kEnergyResist, 0_fp);
        data.type_data.stats.Set(StatType::kPhysicalResist, 0_fp);
        data.type_data.stats.Set(StatType::kOmegaPowerPercentage, 300_fp);
        data.type_data.stats.Set(StatType::kCritAmplificationPercentage, 5_fp);
        data.type_data.stats.Set(StatType::kCritChancePercentage, 0_fp);         // No critical attacks
        data.type_data.stats.Set(StatType::kAttackDodgeChancePercentage, 0_fp);  // No Dodge
        data.type_data.stats.Set(StatType::kHitChancePercentage, 0_fp);          // No Miss
        data.type_data.stats.Set(StatType::kResolve, 0_fp);
        data.type_data.stats.Set(StatType::kGrit, 0_fp);
        data.type_data.stats.Set(StatType::kWillpowerPercentage, 0_fp);
        data.type_data.stats.Set(StatType::kTenacityPercentage, 0_fp);
        data.type_data.stats.Set(StatType::kEnergyGainEfficiencyPercentage, kMaxPercentageFP);
        data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 10000_fp);
        data.type_data.stats.Set(StatType::kHitChancePercentage, kMaxPercentageFP);

        // Health
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

        // Omega
        data.type_data.stats.Set(StatType::kOmegaRangeUnits, 150_fp);
        data.type_data.stats.Set(StatType::kEnergyCost, 100_fp);
        data.type_data.stats.Set(StatType::kStartingEnergy, 0_fp);

        // Attack
        data.type_data.stats.Set(StatType::kAttackRangeUnits, 150_fp);
        data.type_data.stats.Set(StatType::kAttackSpeed, 1000_fp);

        return data;
    }

    void AddExecuteEffect(const Entity& entity, const std::string_view effect_json_text)
    {
        TestingDataLoader loader(world->GetLogger());
        const auto effect_data = loader.ParseAndLoadEffect(effect_json_text);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(entity.GetID());
        GetAttachedEffectsHelper().AddAttachedEffect(entity, entity.GetID(), *effect_data, effect_state);
        ASSERT_TRUE(entity.Get<AttachedEffectsComponent>().HasExecute());
    }

    void ParseAndLoadAbility(
        const std::string_view ability_json_text,
        const AbilityType ability_type,
        AbilityData& out_ability_data)
    {
        TestingDataLoader loader(world->GetLogger());
        const auto ability_json = loader.ParseJSON(ability_json_text);
        ASSERT_TRUE(ability_json.has_value());
        ASSERT_TRUE(loader.LoadAbility(*ability_json, ability_type, &out_ability_data));
    }
};

TEST_F(ExecuteEffectTest, ExecuteWithAttack)
{
    static constexpr std::string_view ability_json_text = R"({
        "Name": "Ability",
        "TotalDurationMs": 3000,
        "Skills": [
            {
                "Name": "Execute",
                "Targeting": {
                    "Type": "CurrentFocus"
                },
                "Deployment": {
                    "Type": "Direct",
                    "PreDeploymentDelayPercentage": 0
                },
                "PercentageOfAbilityDuration": 100,
                "EffectPackage": {
                    "Effects": [
                        {
                            "Type": "InstantDamage",
                            "DamageType": "Energy",
                            "Expression": 201
                        }
                    ]
                }
            }
        ]
    })";

    auto data = MakeUnitDataWithDefaultStats();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);

    data.type_data.attack_abilities.selection_type = AbilitySelectionType::kCycle;
    ParseAndLoadAbility(ability_json_text, AbilityType::kAttack, data.type_data.attack_abilities.AddAbility());

    constexpr std::array<HexGridPosition, 1> blue_positions{{
        {0, -2},
    }};
    std::array<Entity*, blue_positions.size()> blue_entities{};
    for (size_t index = 0; index != blue_entities.size(); ++index)
    {
        SpawnCombatUnit(Team::kBlue, blue_positions[index], data, blue_entities[index]);
        ASSERT_NE(blue_entities[index], nullptr);
    }

    data.radius_units = 5;
    constexpr std::array<HexGridPosition, 1> red_positions{{
        {-4, 6},
    }};
    std::array<Entity*, red_positions.size()> red_entities{};
    for (size_t index = 0; index != red_entities.size(); ++index)
    {
        SpawnCombatUnit(Team::kRed, red_positions[index], data, red_entities[index]);
        ASSERT_NE(red_entities[index], nullptr);
    }

    static constexpr std::string_view effect_json_text = R"({
        "Type": "Execute",
        "DurationMs": -1,
        "Expression": 80,
        "AbilityTypes": [
            "Attack"
        ]
    })";

    AddExecuteEffect(*blue_entities.front(), effect_json_text);

    Stun(*red_entities.front());

    EventHistory<EventType::kAbilityActivated> activated(*world);
    EventHistory<EventType::kOnDamage> damage(*world);
    EventHistory<EventType::kFainted> fainted(*world);

    world->TimeStep();
    ASSERT_EQ(activated.Size(), 1);
    ASSERT_EQ(activated.events.front().ability_type, AbilityType::kAttack);
    ASSERT_EQ(activated.events.front().sender_id, blue_entities.front()->GetID());
    ASSERT_EQ(damage.Size(), 1);
    ASSERT_EQ(damage.events.front().sender_id, blue_entities.front()->GetID());
    ASSERT_EQ(damage.events.front().damage_value, 1000_fp);
    ASSERT_EQ(fainted.Size(), 1);
}

TEST_F(ExecuteEffectTest, ExecuteWithOmega)
{
    static constexpr std::string_view ability_json_text = R"({
        "Name": "Ability",
        "TotalDurationMs": 0,
        "Skills": [
            {
                "Name": "Execute",
                "Targeting": {
                    "Type": "CurrentFocus"
                },
                "Deployment": {
                    "Type": "Direct",
                    "PreDeploymentDelayPercentage": 0
                },
                "PercentageOfAbilityDuration": 100,
                "EffectPackage": {
                    "Effects": [
                        {
                            "Type": "InstantDamage",
                            "DamageType": "Energy",
                            "Expression": 201
                        }
                    ]
                }
            }
        ]
    })";

    auto data = MakeUnitDataWithDefaultStats();
    data.radius_units = 1;
    data.type_data.stats.Set(StatType::kMoveSpeedSubUnits, 0_fp);

    data.type_data.omega_abilities.selection_type = AbilitySelectionType::kCycle;
    ParseAndLoadAbility(ability_json_text, AbilityType::kOmega, data.type_data.omega_abilities.AddAbility());

    constexpr std::array<HexGridPosition, 1> blue_positions{{
        {0, -2},
    }};
    std::array<Entity*, blue_positions.size()> blue_entities{};
    for (size_t index = 0; index != blue_entities.size(); ++index)
    {
        SpawnCombatUnit(Team::kBlue, blue_positions[index], data, blue_entities[index]);
        ASSERT_NE(blue_entities[index], nullptr);
    }

    data.radius_units = 5;
    constexpr std::array<HexGridPosition, 1> red_positions{{
        {-4, 6},
    }};
    std::array<Entity*, red_positions.size()> red_entities{};
    for (size_t index = 0; index != red_entities.size(); ++index)
    {
        SpawnCombatUnit(Team::kRed, red_positions[index], data, red_entities[index]);
        ASSERT_NE(red_entities[index], nullptr);
    }

    static constexpr std::string_view effect_json_text = R"({
        "Type": "Execute",
        "DurationMs": -1,
        "Expression": 80,
        "AbilityTypes": [
            "Omega"
        ]
    })";

    AddExecuteEffect(*blue_entities.front(), effect_json_text);

    Stun(*red_entities.front());

    EventHistory<EventType::kAbilityActivated> activated(*world);
    EventHistory<EventType::kOnDamage> damage(*world);
    EventHistory<EventType::kFainted> fainted(*world);

    blue_entities.front()->Get<StatsComponent>().GetMutableTemplateStats().Set(StatType::kCurrentEnergy, 100_fp);

    world->TimeStep();
    ASSERT_EQ(activated.Size(), 1);
    ASSERT_EQ(activated.events.front().ability_type, AbilityType::kOmega);
    ASSERT_EQ(activated.events.front().sender_id, blue_entities.front()->GetID());
    ASSERT_EQ(damage.Size(), 1);
    ASSERT_EQ(damage.events.front().sender_id, blue_entities.front()->GetID());
    ASSERT_EQ(damage.events.front().damage_value, 1000_fp);
    ASSERT_EQ(fainted.Size(), 1);
}

}  // namespace simulation
