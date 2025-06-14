#include "base_test_fixtures.h"
#include "data/combat_unit_data.h"
#include "gtest/gtest.h"
#include "test_data_loader.h"
#include "utility/entity_helper.h"

namespace simulation
{

class PetsTargetingTest : public BaseTest
{
public:
    FullCombatUnitData MakeCombatUnitData()
    {
        FullCombatUnitData full_data;
        auto& unit_data = full_data.data;

        full_data.instance.id = GenerateRandomUniqueID();

        unit_data.type_id.line_name = "default";
        unit_data.radius_units = 0;
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
        return full_data;
    }

    static constexpr std::string_view DummyPetJSON = R"(
        "Path": "Default",
        "Variation": "Original",
        "Line": "GrillaPet",
        "Stage": 3,
        "UnitType": "Pet",
        "CombatAffinity": "None",
        "CombatClass": "None",
        "Tier": 1,
        "SizeUnits": 0,
        "Stats": {
            "MaxHealth": 350000,
            "StartingEnergy": 100000,
            "EnergyCost": 100000,
            "PhysicalResist": 50,
            "EnergyResist": 50,
            "TenacityPercentage": 0,
            "WillpowerPercentage": 0,
            "Grit": 0,
            "Resolve": 0,
            "AttackPhysicalDamage": 0,
            "AttackEnergyDamage": 0,
            "AttackPureDamage": 0,
            "AttackSpeed": 60,
            "MoveSpeedSubUnits": 0,
            "HitChancePercentage": 100,
            "AttackDodgeChancePercentage": 0,
            "CritChancePercentage": 25,
            "CritAmplificationPercentage": 150,
            "OmegaPowerPercentage": 100,
            "AttackRangeUnits": 3,
            "OmegaRangeUnits": 3,
            "HealthRegeneration": 0,
            "EnergyRegeneration": 0,
            "EnergyGainEfficiencyPercentage": 100,
            "OnActivationEnergy": 0,
            "VulnerabilityPercentage": 100,
            "EnergyPiercingPercentage": 0,
            "PhysicalPiercingPercentage": 0,
            "HealthGainEfficiencyPercentage": 100,
            "PhysicalVampPercentage": 0,
            "EnergyVampPercentage": 0,
            "PureVampPercentage": 0,
            "Thorns": 0,
            "StartingShield": 0,
            "CritReductionPercentage": 0
        },
        "AttackAbilitiesSelection": "Cycle",
        "OmegaAbilitiesSelection": "Cycle",
        "AttackAbilities": [],
        "OmegaAbilities": []
    )";
};

TEST_F(PetsTargetingTest, HexagonalZone_HighestDensity)
{
    constexpr HexGridPosition blue_position{1, 1};

    // Blue entity is dummy. Does nothing
    {
        auto blue_data = MakeCombatUnitData();
        blue_data.instance.team = Team::kBlue;
        blue_data.instance.position = blue_position;

        Entity* blue_entity = nullptr;
        SpawnCombatUnit(blue_data, blue_entity);
        ASSERT_NE(blue_entity, nullptr);
    }

    // Red entity spawns pet and uses pets targeting to give it a positive state
    {
        auto red_data = MakeCombatUnitData();
        red_data.instance.team = Team::kRed;
        red_data.instance.position = Reflect(blue_position);

        TestingDataLoader loader(world->GetLogger());
        auto& innate_ability = red_data.data.type_data.innate_abilities.AddAbility();

        std::string spawn_pet_ability_text = R"({
            "Name": "Spawn and buff pet ability",
            "ActivationTrigger": {
                "TriggerType": "OnBattleStart"
            },
            "TotalDurationMs": 0,
            "Skills": [
                {
                    "Name": "Spawn Pet",
                    "Targeting": {
                        "Type": "Self"
                    },
                    "Deployment": {
                        "Type": "SpawnedCombatUnit",
                        "PreDeploymentDelayPercentage": 0
                    },
                    "EffectPackage": {
                        "Effects": []
                    },
                    "SpawnCombatUnit": {
                        "OnReservedPosition": false,
                        "CombatUnit": {
                            +++
                        }
                    }
                },
                {
                    "Name": "Buff Pet",
                    "Targeting": {
                        "Type": "Pets",
                        "Group": "Self"
                    },
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "EffectPackage": {
                        "Effects": [{
                            "Type": "PositiveState",
                            "PositiveState": "Invulnerable",
                            "DurationMs": -1
                        }]
                    }
                }
            ]
        })";

        ASSERT_TRUE(FindAndReplace(spawn_pet_ability_text, "+++", DummyPetJSON));
        const auto spawn_pet_ability_json = loader.ParseJSON(spawn_pet_ability_text);
        ASSERT_TRUE(spawn_pet_ability_json);

        BaseDataLoader::AbilityLoadingOptions options{};
        options.require_trigger = true;
        ASSERT_TRUE(loader.LoadAbility(*spawn_pet_ability_json, AbilityType::kInnate, options, &innate_ability));

        Entity* red_entity = nullptr;
        SpawnCombatUnit(red_data, red_entity);
        ASSERT_NE(red_entity, nullptr);
    }

    EventHistory<EventType::kCombatUnitCreated> created_unit(*world);
    world->TimeStep();

    ASSERT_EQ(created_unit.Size(), 1) << "Pet didn't spawn?";

    ASSERT_TRUE(EntityHelper::HasPositiveState(
        *world,
        created_unit.events.front().entity_id,
        EffectPositiveState::kInvulnerable));
}

}  // namespace simulation
