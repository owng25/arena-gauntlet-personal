#include <optional>
#include <string>

#include "base_json_test.h"
#include "base_test_fixtures.h"
#include "components/abilities_component.h"
#include "components/combat_synergy_component.h"
#include "components/combat_unit_component.h"
#include "components/focus_component.h"
#include "components/movement_component.h"
#include "components/position_component.h"
#include "components/stats_component.h"
#include "data/combat_synergy_bonus.h"
#include "data/loaders/battle_base_data_loader.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"
#include "test_data_loader.h"
#include "utility/entity_helper.h"
#include "utility/equipment_helper.h"

namespace simulation
{
class JSONTest : public BaseTest
{
    bool IsEnabledLogsForCalculateLiveStats() const override
    {
        return false;
    }
};

class JSONRangerTest : public JSONTest
{
    typedef JSONTest Super;

public:
    void TestWeapon(const CombatUnitWeaponData& weapon)
    {
        TestWeaponsStats(weapon.stats);

        EXPECT_EQ(weapon.type_id.name, "Pistol");
        EXPECT_EQ(weapon.tier, 3);
        EXPECT_EQ(weapon.type_id.stage, 2);
        EXPECT_EQ(weapon.type_id.combat_affinity, CombatAffinity::kWater);
        EXPECT_EQ(weapon.combat_class, CombatClass::kTemplar);
        EXPECT_EQ(weapon.combat_affinity, CombatAffinity::kWater);
        EXPECT_EQ(weapon.weapon_type, WeaponType::kNormal);

        TestWeaponAbilities(weapon.attack_abilities, weapon.omega_abilities);
    }

    void TestSuit(const CombatUnitSuitData& suit)
    {
        TestSuitStats(suit.stats);

        EXPECT_EQ(suit.type_id.name, "CoolSuit");
        EXPECT_EQ(suit.type_id.stage, 1);
        EXPECT_EQ(suit.suit_type, SuitType::kNormal);
    }

    void TestWeaponsStats(const StatsData& weapon_stats)
    {
        EXPECT_EQ(weapon_stats.Get(StatType::kCurrentEnergy), 0_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kCurrentHealth), 0_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kMoveSpeedSubUnits), 10000_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kEnergyCost), 50000_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kAttackPhysicalDamage), 10_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kAttackEnergyDamage), 11_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kPhysicalPiercingPercentage), 21_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kEnergyPiercingPercentage), 22_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kCritAmplificationPercentage), 5_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kCritChancePercentage), 6_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kPhysicalResist), 15_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kEnergyResist), 17_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kMaxHealth), 100000_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kOnActivationEnergy), 25_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kStartingEnergy), 5000_fp);
        EXPECT_EQ(weapon_stats.Get(StatType::kOmegaPowerPercentage), 42_fp);
    }

    void TestWeaponAbilities(const AbilitiesData& attack_abilities, const AbilitiesData& omega_abilities)
    {
        // Check attack ability
        {
            EXPECT_EQ(attack_abilities.selection_type, AbilitySelectionType::kCycle);

            ASSERT_EQ(attack_abilities.abilities.size(), 1);
            const auto& attack_ability = attack_abilities.abilities.at(0);
            EXPECT_EQ(attack_ability->name, "Attack Ability");

            EXPECT_EQ(attack_ability->update_type, AbilityUpdateType::kNone);

            // Should be 0 as the attack ability duration comes from the attack speed
            EXPECT_EQ(attack_ability->total_duration_ms, 0);

            // Check attack_skill
            ASSERT_EQ(attack_ability->skills.size(), 1);
            const auto& attack_skill = attack_ability->skills.at(0);
            EXPECT_EQ(attack_skill->name, "Attack Skill");
            EXPECT_EQ(attack_skill->targeting.type, SkillTargetingType::kCurrentFocus);
            EXPECT_EQ(attack_skill->targeting.guidance.Contains(GuidanceType::kGround), true);
            EXPECT_EQ(attack_skill->deployment.type, SkillDeploymentType::kDirect);
            EXPECT_EQ(attack_skill->deployment.guidance.Contains(GuidanceType::kGround), true);
            EXPECT_EQ(attack_skill->deployment.pre_deployment_delay_percentage, 50);

            // Check attack effects
            ASSERT_EQ(attack_skill->effect_package.effects.size(), 1);
            const auto& attack_effect = attack_skill->effect_package.effects.at(0);
            EXPECT_EQ(attack_effect.type_id.type, EffectType::kInstantDamage);
            EXPECT_EQ(attack_effect.type_id.damage_type, EffectDamageType::kPhysical);
            EXPECT_EQ(attack_effect.GetExpression().base_value.value, 423_fp);
        }

        // Check omega abilities
        {
            EXPECT_EQ(omega_abilities.selection_type, AbilitySelectionType::kCycle);

            ASSERT_EQ(omega_abilities.abilities.size(), 1);
            const auto& omega_ability = omega_abilities.abilities.at(0);
            EXPECT_EQ(omega_ability->name, "Flip OmegaAttack Ability");
            EXPECT_EQ(omega_ability->total_duration_ms, 1123);

            // Check omega skill
            ASSERT_EQ(omega_ability->skills.size(), 1);
            const auto& omega_skill = omega_ability->skills.at(0);
            EXPECT_EQ(omega_skill->name, "Flip OmegaAttack Skill");
            EXPECT_EQ(omega_skill->targeting.type, SkillTargetingType::kCurrentFocus);
            EXPECT_EQ(omega_skill->targeting.guidance.Contains(GuidanceType::kGround), true);
            EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kDirect);
            EXPECT_EQ(omega_skill->deployment.guidance.Contains(GuidanceType::kGround), true);
            EXPECT_EQ(omega_skill->deployment.pre_deployment_delay_percentage, 90);
            EXPECT_EQ(omega_skill->percentage_of_ability_duration, 100);

            // Check skill effects
            ASSERT_EQ(omega_skill->effect_package.effects.size(), 1);
            const auto& omega_effect = omega_skill->effect_package.effects.at(0);
            EXPECT_EQ(omega_effect.type_id.type, EffectType::kInstantDamage);
            EXPECT_EQ(omega_effect.type_id.damage_type, EffectDamageType::kEnergy);
            EXPECT_EQ(omega_effect.GetExpression().base_value.value, 600000_fp);
        }
    }

    void TestSuitStats(const StatsData& suit_stats)
    {
        EXPECT_EQ(suit_stats.Get(StatType::kCurrentEnergy), 0_fp);
        EXPECT_EQ(suit_stats.Get(StatType::kCurrentHealth), 0_fp);
        EXPECT_EQ(suit_stats.Get(StatType::kMaxHealth), 50000_fp);
        EXPECT_EQ(suit_stats.Get(StatType::kEnergyResist), 33_fp);
        EXPECT_EQ(suit_stats.Get(StatType::kPhysicalResist), 34_fp);
        EXPECT_EQ(suit_stats.Get(StatType::kGrit), 13_fp);
        EXPECT_EQ(suit_stats.Get(StatType::kResolve), 14_fp);
    }
};

TEST_F(JSONTest, TestIntegerOverflow)
{
    auto logger = Logger::Create();
    logger->Disable();
    JSONHelper helper(logger);

    // Absolutely valid case.
    {
        const char* json_text = R"({ "key": 123 })";
        auto object = nlohmann::json::parse(json_text, nullptr, false);
        EXPECT_FALSE(object.is_discarded());

        int value = -1;
        EXPECT_TRUE(helper.GetIntValue(object, "key", &value));
        EXPECT_EQ(value, 123);
    }

    // Value overflows int32 but parsing should not fail
    {
        const char* json_text = R"({ "key": 2147483648 })";
        auto object = nlohmann::json::parse(json_text, nullptr, false);
        EXPECT_FALSE(object.is_discarded());

        int value = -1;
        EXPECT_FALSE(helper.GetIntValue(object, "key", &value));
    }

    // Value is bigger than int64 max value
    {
        const char* json_text = R"({ "key": 9223372036854775808 })";
        auto object = nlohmann::json::parse(json_text, nullptr, false);
        EXPECT_FALSE(object.is_discarded());

        int value = -1;
        EXPECT_FALSE(helper.GetIntValue(object, "key", &value));
    }

    // Value is bigger than uint64 max value
    // Will not be discarded (shomewhy) but GetIntValue should fail
    {
        const char* json_text = R"({ "key": 18446744073709551616 })";
        auto object = nlohmann::json::parse(json_text, nullptr, false);
        EXPECT_FALSE(object.is_discarded());

        int value = -1;
        EXPECT_FALSE(helper.GetIntValue(object, "key", &value));
    }

    // Value is less than int64 lowest value.
    // Will not be discarded (shomewhy) but GetIntValue should fail
    {
        const char* json_text = R"({ "key": -9223372036854775809 })";
        auto object = nlohmann::json::parse(json_text, nullptr, false);
        EXPECT_FALSE(object.is_discarded());

        int value = -1;
        EXPECT_FALSE(helper.GetIntValue(object, "key", &value));
    }
}

TEST_F(JSONRangerTest, TestWeaponAffinityAndClassEmpty)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "FemaleRanger",
    "UnitType": "Ranger",
    "Stage": 1,
    "SizeUnits": 10
}
)";

    const char* weapon_json_raw = R"(
{
    "Name": "Pistol",
    "Tier": 3,
    "Stage": 2,
    "CombatAffinity": "",
    "CombatClass": "",
	"Type": "Normal",
    "Stats": {
        "MoveSpeedSubUnits": 10000,
        "EnergyCost": 50000,
        "AttackPhysicalDamage": 10,
        "AttackEnergyDamage": 11,
        "AttackSpeed": 160,
        "AttackRangeUnits": 40,
        "OmegaRangeUnits": 0,
        "HitChancePercentage": 80,
        "PhysicalPiercingPercentage": 21,
        "EnergyPiercingPercentage": 22,
        "CritAmplificationPercentage": 5,
        "CritChancePercentage": 6,
        "PhysicalResist": 15,
        "EnergyResist": 17,
        "MaxHealth": 100000,
        "OnActivationEnergy": 25,
        "StartingEnergy": 5000,
        "OmegaPowerPercentage": 42,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack Ability",
            "Skills": [
                {
                    "Name": "Attack Skill",
                    "Targeting": {
                        "Type": "CurrentFocus"
                    },
                    "Deployment": {
                        "Type": "Direct",
                        "PreDeploymentDelayPercentage": 50
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 423
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Flip OmegaAttack Ability",
            "TotalDurationMs": 1123,
            "Skills": [
                {
                    "Name": "Flip OmegaAttack Skill",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Direct",
                        "PreDeploymentDelayPercentage": 90,
                        "Guidance": [ "Ground" ]
                    },
                    "PercentageOfAbilityDuration": 100,
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Energy",
                                "Expression": 600000
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";

    const char* suit_json_raw = R"(
{
    "Name": "CoolSuit",
    "Tier": 4,
    "Stage": 1,
    "Stats": {
        "EnergyResist": 33,
        "PhysicalResist": 34,
        "MaxHealth": 50000,
        "Grit": 13,
        "Resolve": 14
    }
}
)";

    FullCombatUnitData full_data;
    full_data.instance = CreateCombatUnitInstanceData();
    full_data.instance.team = Team::kBlue;
    full_data.instance.position = {10, 20};
    full_data.instance.equipped_suit.type_id = CombatSuitTypeID{"CoolSuit", 1, ""};
    full_data.instance.equipped_weapon.type_id = CombatWeaponTypeID{"Pistol", 2, CombatAffinity::kNone};

    // Load from json
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &full_data.data);

    auto weapon_data = std::make_shared<CombatUnitWeaponData>();
    type_data_loader.LoadCombatWeaponData(nlohmann::json::parse(weapon_json_raw), weapon_data.get());
    game_data_container_->AddWeaponData(weapon_data->type_id, weapon_data);

    auto suit_data = std::make_shared<CombatUnitSuitData>();
    type_data_loader.LoadCombatSuitData(nlohmann::json::parse(suit_json_raw), suit_data.get());
    game_data_container_->AddSuitData(suit_data->type_id, suit_data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(full_data, entity);

    EXPECT_EQ(world->GetEquipmentHelper().GetDefaultTypeDataCombatClass(full_data), CombatClass::kNone);
    EXPECT_EQ(world->GetEquipmentHelper().GetDefaultTypeDataCombatAffinity(full_data), CombatAffinity::kNone);
}

TEST_F(JSONRangerTest, LoadRangerWeapon)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "FemaleRanger",
    "UnitType": "Ranger",
    "Stage": 1,
    "SizeUnits": 10,
    "Stats": {
        "AttackSpeed": 40
    }
})";

    const char* weapon_json_raw = R"(
{
    "Name": "Pistol",
    "Tier": 3,
    "Stage": 2,
    "Type": "Normal",
    "CombatAffinity": "Water",
    "DominantCombatAffinity": "Water",
    "CombatClass": "Templar",
    "DominantCombatClass": "Fighter",
    "Stats": {
        "MoveSpeedSubUnits": 10000,
        "EnergyCost": 50000,
        "AttackPhysicalDamage": 10,
        "AttackEnergyDamage": 11,
        "AttackSpeed": 160,
        "AttackRangeUnits": 40,
        "OmegaRangeUnits": 0,
        "HitChancePercentage": 80,
        "PhysicalPiercingPercentage": 21,
        "EnergyPiercingPercentage": 22,
        "CritAmplificationPercentage": 5,
        "CritChancePercentage": 6,
        "PhysicalResist": 15,
        "EnergyResist": 17,
        "MaxHealth": 100000,
        "OnActivationEnergy": 25,
        "StartingEnergy": 5000,
        "OmegaPowerPercentage": 42,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack Ability",
            "Skills": [
                {
                    "Name": "Attack Skill",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Direct",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 50
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 423
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Flip OmegaAttack Ability",
            "TotalDurationMs": 1123,
            "Skills": [
                {
                    "Name": "Flip OmegaAttack Skill",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [
                            "Ground"
                        ]
                    },
                    "Deployment": {
                        "Type": "Direct",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 90
                    },
                    "PercentageOfAbilityDuration": 100,
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Energy",
                                "Expression": 600000
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";

    const char* suit_json_raw = R"(
{
    "Name": "CoolSuit",
    "Tier": 4,
    "Stage": 1,
    "Type": "Normal",
    "Stats": {
        "EnergyResist": 33,
        "PhysicalResist": 34,
        "MaxHealth": 50000,
        "Grit": 13,
        "Resolve": 14
    }
}
)";

    FullCombatUnitData full_data;
    full_data.instance = CreateCombatUnitInstanceData();
    full_data.instance.team = Team::kBlue;
    full_data.instance.position = {10, 20};
    full_data.instance.equipped_suit.type_id = CombatSuitTypeID{"CoolSuit", 1, ""};
    full_data.instance.equipped_weapon.type_id = CombatWeaponTypeID{"Pistol", 2, CombatAffinity::kWater};

    // Load from json
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &full_data.data);

    auto weapon_data = std::make_shared<CombatUnitWeaponData>();
    type_data_loader.LoadCombatWeaponData(nlohmann::json::parse(weapon_json_raw), weapon_data.get());
    ASSERT_EQ(weapon_data->tier, 3);
    game_data_container_->AddWeaponData(weapon_data->type_id, weapon_data);

    auto suit_data = std::make_shared<CombatUnitSuitData>();
    type_data_loader.LoadCombatSuitData(nlohmann::json::parse(suit_json_raw), suit_data.get());
    game_data_container_->AddSuitData(suit_data->type_id, suit_data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(full_data, entity);

    // Get the components
    const auto& position_component = entity->Get<PositionComponent>();
    const auto& movement_component = entity->Get<MovementComponent>();
    const auto& stats_component = entity->Get<StatsComponent>();
    const auto& combat_synergy_component = entity->Get<CombatSynergyComponent>();
    const auto& combat_unit_component = entity->Get<CombatUnitComponent>();

    // CombatUnitComponent
    {
        const CombatUnitTypeID& type_id = combat_unit_component.GetTypeID();
        EXPECT_TRUE(combat_unit_component.IsARanger());
        EXPECT_FALSE(combat_unit_component.IsAIlluvial());

        EXPECT_EQ(type_id.line_name, "FemaleRanger");
        EXPECT_EQ(type_id.stage, 0);
        EXPECT_EQ(type_id.type, CombatUnitType::kRanger);
    }

    // Check weapon values
    TestWeapon(*weapon_data);
    TestWeaponsStats(stats_component.GetWeaponStats());

    // Check suit values
    TestSuit(*suit_data);
    TestSuitStats(stats_component.GetSuitStats());

    // Test helpers on CombatUnitdata
    EXPECT_EQ(world->GetEquipmentHelper().GetDefaultTypeDataCombatClass(full_data), CombatClass::kTemplar);
    EXPECT_EQ(world->GetEquipmentHelper().GetDefaultTypeDataCombatAffinity(full_data), CombatAffinity::kWater);

    {
        AbilitiesData attack_abilities;
        AbilitiesData omega_abilities;
        world->GetEquipmentHelper().GetMergedAbilitiesDataForAbilityType(
            full_data,
            AbilityType::kAttack,
            &attack_abilities);
        world->GetEquipmentHelper().GetMergedAbilitiesDataForAbilityType(
            full_data,
            AbilityType::kOmega,
            &omega_abilities);
        TestWeaponAbilities(attack_abilities, omega_abilities);
    }

    auto get_stat_data = [&](StatType stat_type)
    {
        return world->GetEquipmentHelper().GetDefaultTypeDataStatValueForType(full_data, stat_type);
    };

    EXPECT_EQ(get_stat_data(StatType::kPhysicalResist), 49_fp);
    EXPECT_EQ(get_stat_data(StatType::kEnergyResist), 50_fp);
    EXPECT_EQ(get_stat_data(StatType::kGrit), 13_fp);
    EXPECT_EQ(get_stat_data(StatType::kResolve), 14_fp);
    EXPECT_EQ(get_stat_data(StatType::kMaxHealth), 150000_fp);
    EXPECT_EQ(get_stat_data(StatType::kAttackRangeUnits), 40_fp);
    EXPECT_EQ(get_stat_data(StatType::kOmegaRangeUnits), 0_fp);
    EXPECT_EQ(get_stat_data(StatType::kCritReductionPercentage), 25_fp);

    // Test values
    // Ensure position correct
    EXPECT_EQ(position_component.GetQ(), 10);
    EXPECT_EQ(position_component.GetR(), 20);

    // Ensure movement attributes correct
    EXPECT_EQ(movement_component.GetMovementSpeedSubUnits(), 10000_fp);
    EXPECT_EQ(movement_component.GetMovementType(), MovementComponent::MovementType::kNavigation);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackRangeUnits), 40_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackSpeed), 200_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kHitChancePercentage), 80_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackPhysicalDamage), 10_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackEnergyDamage), 11_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackPureDamage), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackDodgeChancePercentage), 0_fp);

    // 900000 + 100000 + 50000
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kMaxHealth), 150000_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kHealthRegeneration), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kHealthGainEfficiencyPercentage), 100_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kShieldGainEfficiencyPercentage), 100_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kPhysicalVampPercentage), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyVampPercentage), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kPureVampPercentage), 0_fp);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyCost), 50000_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kStartingEnergy), 5000_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyRegeneration), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyGainEfficiencyPercentage), 100_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kOnActivationEnergy), 25_fp);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kPhysicalPiercingPercentage), 21_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyPiercingPercentage), 22_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kPhysicalResist), 49_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyResist), 50_fp);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kTenacityPercentage), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kWillpowerPercentage), 0_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kGrit), 13_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kResolve), 14_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kVulnerabilityPercentage), 100_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kCritChancePercentage), 6_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kCritAmplificationPercentage), 5_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kOmegaPowerPercentage), 42_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kCritReductionPercentage), 25_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kThorns), 0_fp);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kStartingShield), 0_fp);
    EXPECT_EQ(stats_component.GetCritChanceOverflowPercentage(), 0_fp);

    // Ensure current values correct
    EXPECT_EQ(stats_component.GetCurrentEnergy(), 5000_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 150000_fp);

    // Check combat class and combat affinity and entity_size
    EXPECT_EQ(position_component.GetRadius(), 10);
    EXPECT_TRUE(position_component.IsTakingSpace());
    EXPECT_EQ(combat_synergy_component.GetCombatClass(), CombatClass::kTemplar);
    EXPECT_EQ(combat_synergy_component.GetCombatAffinity(), CombatAffinity::kWater);

    // Check abilities
    {
        const auto& abilities_component = entity->Get<AbilitiesComponent>();
        ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kAttack));
        const auto& attack_abilities = abilities_component.GetDataAttackAbilities();
        ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));
        const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();
        TestWeaponAbilities(attack_abilities, omega_abilities);
    }
}

TEST_F(JSONTest, LoadCombatUnit)  // NOLINT
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Axolotl",
    "UnitType": "Illuvial",
    "Stage": 1,
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "DominantCombatAffinity": "Water",
    "DominantCombatClass": "Bulwark",
    "PreferredLineDominantCombatAffinity": "Air",
    "PreferredLineDominantCombatClass": "Psion",
    "Tier": 3,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MoveSpeedSubUnits": 5000,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 40,
        "AttackSpeed": 60,
        "HitChancePercentage": 55,
        "AttackPhysicalDamage": 11,
        "AttackEnergyDamage": 12,
        "AttackPureDamage": 13,
        "AttackDodgeChancePercentage": 14,
        "MaxHealth": 950000,
        "HealthGainEfficiencyPercentage": 75,
        "PhysicalVampPercentage": 66,
        "EnergyVampPercentage": 67,
        "PureVampPercentage": 68,
        "EnergyCost": 60000,
        "StartingEnergy": 30000,
        "EnergyRegeneration": 765,
        "EnergyGainEfficiencyPercentage": 129,
        "OnActivationEnergy": 666,
        "EnergyPiercingPercentage": 31,
        "PhysicalPiercingPercentage": 32,
        "EnergyResist": 33,
        "PhysicalResist": 34,
        "TenacityPercentage": 1,
        "WillpowerPercentage": 2,
        "Grit": 3,
        "Resolve": 4,
        "VulnerabilityPercentage": 5,
        "CritChancePercentage": 6,
        "CritAmplificationPercentage": 7,
        "OmegaPowerPercentage": 8,
        "Thorns": 9,
        "HealthRegeneration": 12345,
        "StartingShield": 20,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack",
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Projectile",
                        "Guidance": [ "Ground", "Airborne" ],
                        "PreDeploymentDelayPercentage": 25
                    },
                    "Projectile": {
                        "SizeUnits": 5,
                        "SpeedSubUnits": 1234,
                        "IsHoming": false,
                        "IsBlockable": true,
                        "ApplyToAll": true
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 123
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "InnateAbilitiesSelection": "Cycle",
    "InnateAbilities": [
        {
            "Name": "Innate Attack Ability",
            "TotalDurationMs": 2345,
            "ActivationTrigger": {
                "TriggerType": "OnActivateXAbilities",
                "AbilityTypes": ["Attack", "Omega", "Innate"],
                "ActivationTimeLimitMs": 1849,
                "ActivateEveryTimeMs": 15000,
                "HealthLowerLimitPercentage": 50,
                "MaxActivations": 3,
                "NumberOfAbilitiesActivated": 1
            },
            "Skills": [
                {
                    "Name": "Innate Attack Ability",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [
                            "Ground",
                            "Airborne",
                            "Underground"
                        ]
                    },
                    "Deployment": {
                        "Type": "Projectile",
                        "Guidance": [ "Ground", "Airborne", "Underground" ],
                        "PreDeploymentDelayPercentage": 25
                    },
                    "Projectile": {
                        "SizeUnits": 6,
                        "SpeedSubUnits": 1234,
                        "IsHoming": false,
                        "IsBlockable": true,
                        "ApplyToAll": true,
                        "ContinueAfterTarget": true
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": {
                                    "Value": 999
                                }
                            }
                        ]
                    }
                }
            ]
        },
        {
            "Name": "Innate Attack Ability2",
            "TotalDurationMs": 2345,
            "ActivationTrigger": {
                "TriggerType": "HealthInRange",
                "HealthLowerLimitPercentage": 50
            },
            "Skills": [
                {
                    "Name": "Innate Attack Ability2",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [
                            "Ground",
                            "Underground"
                        ]
                    },
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": {
                                    "Value": 444,
                                    "IsUsedAsPercentage": true
                                }
                            }
                        ]
                    }
                }
            ]
        },
        {
            "Name": "Innate Attack Ability3",
            "TotalDurationMs": 2345,
            "ActivationTrigger": {
                "TriggerType": "EveryXTime",
                "ActivateEveryTimeMs": 15000
            },
            "Skills": [
                {
                    "Name": "Innate Attack Ability3",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [
                            "Ground",
                            "Underground"
                        ]
                    },
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 123
                            }
                        ]
                    }
                }
            ]
        },
        {
            "Name": "Innate Attack Ability4",
            "TotalDurationMs": 2345,
            "ActivationTrigger": {
                "AbilityTypes": ["Attack"],
                "TriggerType": "OnActivateXAbilities",
                "NumberOfAbilitiesActivated": 5,
                "EveryX": true,
                "ActivationCooldownMs": 300
            },
            "Skills": [
                {
                    "Name": "Innate Attack Ability4",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [
                            "Ground",
                            "Airborne"
                        ]
                    },
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 123
                            }
                        ]
                    }
                }
            ]
        },
        {
            "Name": "Innate Attack Ability5",
            "TotalDurationMs": 2345,
            "MovementLock": false,
            "ActivationTrigger": {
                "TriggerType": "InRange",
                "ActivationRadiusUnits": 5,
                "SenderConditions": [
                    "Poisoned"
                ],
                "ReceiverConditions": [
                    "Poisoned"
                ]
            },
            "Skills": [
                {
                    "Name": "Innate Attack Ability5",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [
                            "Ground"
                        ]
                    },
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 123
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";

    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);

    // Spawn the CombatUnit
    CombatUnitInstanceData instance = CreateCombatUnitInstanceData();
    instance.team = Team::kBlue;
    instance.position = {10, 20};
    instance.level = 60;
    Entity* entity = nullptr;
    SpawnCombatUnit(instance, data, entity);

    // Get the components
    const auto& position_component = entity->Get<PositionComponent>();
    const auto& movement_component = entity->Get<MovementComponent>();
    const auto& stats_component = entity->Get<StatsComponent>();
    const auto& combat_synergy_component = entity->Get<CombatSynergyComponent>();

    // Test values
    // Ensure position correct
    EXPECT_EQ(position_component.GetQ(), 10);
    EXPECT_EQ(position_component.GetR(), 20);

    // Ensure movement attributes correct
    EXPECT_EQ(movement_component.GetMovementSpeedSubUnits(), 5000_fp);
    EXPECT_EQ(movement_component.GetMovementType(), MovementComponent::MovementType::kNavigation);

    ASSERT_EQ(data.type_data.tier, 3);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kMoveSpeedSubUnits), 5000_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackRangeUnits), 42_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kOmegaRangeUnits), 42_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackSpeed), 60_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kHitChancePercentage), 55_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackPhysicalDamage), 14_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackEnergyDamage), 15_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackPureDamage), 16_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackDodgeChancePercentage), 14_fp);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kMaxHealth), 1187500_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kHealthRegeneration), 12345_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kHealthGainEfficiencyPercentage), 75_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kPhysicalVampPercentage), 66_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyVampPercentage), 67_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kPureVampPercentage), 68_fp);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyCost), 60000_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kStartingEnergy), 30000_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyRegeneration), 765_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyGainEfficiencyPercentage), 129_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kOnActivationEnergy), 666_fp);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyPiercingPercentage), 31_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kPhysicalPiercingPercentage), 32_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyResist), 33_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kPhysicalResist), 34_fp);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kTenacityPercentage), 1_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kWillpowerPercentage), 2_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kGrit), 3_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kResolve), 4_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kVulnerabilityPercentage), 5_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kCritChancePercentage), 6_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kCritAmplificationPercentage), 7_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kOmegaPowerPercentage), 10_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kCritReductionPercentage), 25_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kThorns), 9_fp);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kStartingShield), 20_fp);
    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 0_fp);

    // Ensure current values correct
    EXPECT_EQ(stats_component.GetCurrentEnergy(), 30000_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 1187500_fp);

    world->TimeStep();

    // Check combat class, combat affinity and size
    EXPECT_EQ(position_component.GetRadius(), 5);
    EXPECT_TRUE(position_component.IsTakingSpace());
    EXPECT_EQ(combat_synergy_component.GetCombatClass(), CombatClass::kBulwark);
    EXPECT_EQ(combat_synergy_component.GetCombatAffinity(), CombatAffinity::kWater);
    EXPECT_EQ(data.type_data.preferred_line_dominant_combat_class, CombatClass::kPsion);
    EXPECT_EQ(data.type_data.preferred_line_dominant_combat_affinity, CombatAffinity::kAir);
    EXPECT_EQ(data.type_data.dominant_combat_class, CombatClass::kBulwark);
    EXPECT_EQ(data.type_data.dominant_combat_affinity, CombatAffinity::kWater);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kAttack));
    const auto& attack_abilities = abilities_component.GetDataAttackAbilities();
    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kInnate));
    const auto& innate_abilities = abilities_component.GetDataInnateAbilities();

    // Check attack ability
    {
        ASSERT_EQ(attack_abilities.abilities.size(), 1);
        const auto& attack_ability = attack_abilities.abilities.at(0);
        EXPECT_EQ(attack_ability->name, "Attack");
        // Should be 0 as the attack ability duration comes from the attack speed
        EXPECT_EQ(attack_ability->total_duration_ms, 0);

        // Check attack_skill
        ASSERT_EQ(attack_ability->skills.size(), 1);
        const auto& attack_skill = attack_ability->skills.at(0);
        EXPECT_EQ(attack_skill->name, "Attack");
        EXPECT_EQ(attack_skill->targeting.type, SkillTargetingType::kCurrentFocus);
        EXPECT_EQ(attack_skill->targeting.guidance.Size(), 1);
        EXPECT_EQ(attack_skill->targeting.guidance.Contains(GuidanceType::kGround), true);
        EXPECT_EQ(attack_skill->deployment.type, SkillDeploymentType::kProjectile);
        EXPECT_EQ(attack_skill->deployment.guidance.Size(), 2);
        EXPECT_EQ(attack_skill->deployment.guidance.Contains(GuidanceType::kGround), true);
        EXPECT_EQ(attack_skill->deployment.guidance.Contains(GuidanceType::kAirborne), true);
        EXPECT_EQ(attack_skill->projectile.size_units, 5);
        EXPECT_EQ(attack_skill->projectile.speed_sub_units, 1234);
        EXPECT_EQ(attack_skill->projectile.is_homing, false);
        EXPECT_EQ(attack_skill->projectile.is_blockable, true);
        EXPECT_EQ(attack_skill->projectile.apply_to_all, true);
        EXPECT_EQ(attack_skill->deployment.pre_deployment_delay_percentage, 25);

        // Check attack effects
        ASSERT_EQ(attack_skill->effect_package.effects.size(), 1);
        const auto& attack_effect = attack_skill->effect_package.effects.at(0);
        EXPECT_EQ(attack_effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(attack_effect.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(attack_effect.GetExpression().base_value.value, 123_fp);
    }

    // Check innate ability
    {
        ASSERT_EQ(innate_abilities.abilities.size(), 5);
        const auto& innate_ability = innate_abilities.abilities.at(0);
        EXPECT_EQ(innate_ability->name, "Innate Attack Ability");
        EXPECT_EQ(innate_ability->total_duration_ms, 2345);
        EXPECT_EQ(innate_ability->activation_trigger_data.trigger_type, ActivationTriggerType::kOnActivateXAbilities);
        EXPECT_EQ(innate_ability->activation_trigger_data.activation_time_limit_ms, 1849);
        EXPECT_EQ(innate_ability->activation_trigger_data.activate_every_time_ms, 0);
        EXPECT_EQ(innate_ability->activation_trigger_data.max_activations, 3);
        EXPECT_EQ(innate_ability->activation_trigger_data.health_lower_limit_percentage, 0_fp);

        // Check attack_skill
        ASSERT_EQ(innate_ability->skills.size(), 1);
        const auto& innate_skill = innate_ability->skills.at(0);
        EXPECT_EQ(innate_skill->name, "Innate Attack Ability");
        EXPECT_EQ(innate_skill->targeting.type, SkillTargetingType::kCurrentFocus);
        EXPECT_EQ(innate_skill->targeting.guidance.Size(), 3);
        EXPECT_EQ(innate_skill->targeting.guidance.Contains(GuidanceType::kGround), true);
        EXPECT_EQ(innate_skill->targeting.guidance.Contains(GuidanceType::kUnderground), true);
        EXPECT_EQ(innate_skill->targeting.guidance.Contains(GuidanceType::kAirborne), true);
        EXPECT_EQ(innate_skill->deployment.type, SkillDeploymentType::kProjectile);
        EXPECT_EQ(innate_skill->deployment.guidance.Size(), 3);
        EXPECT_EQ(innate_skill->deployment.guidance.Contains(GuidanceType::kGround), true);
        EXPECT_EQ(innate_skill->deployment.guidance.Contains(GuidanceType::kUnderground), true);
        EXPECT_EQ(innate_skill->deployment.guidance.Contains(GuidanceType::kAirborne), true);
        EXPECT_EQ(innate_skill->projectile.size_units, 6);
        EXPECT_EQ(innate_skill->projectile.speed_sub_units, 1234);
        EXPECT_FALSE(innate_skill->projectile.is_homing);
        EXPECT_TRUE(innate_skill->projectile.is_blockable);
        EXPECT_TRUE(innate_skill->projectile.apply_to_all);
        EXPECT_TRUE(innate_skill->projectile.continue_after_target);
        EXPECT_EQ(innate_skill->deployment.pre_deployment_delay_percentage, 25);

        // Check attack effects
        ASSERT_EQ(innate_skill->effect_package.effects.size(), 1);
        const auto& innate_effect = innate_skill->effect_package.effects.at(0);
        EXPECT_EQ(innate_effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(innate_effect.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(innate_effect.GetExpression().base_value.value, 999_fp);
        EXPECT_FALSE(innate_effect.GetExpression().is_used_as_percentage);
    }

    // Check for kHealthInRange trigger
    {
        const auto& innate_ability = innate_abilities.abilities.at(1);
        EXPECT_EQ(innate_ability->name, "Innate Attack Ability2");
        EXPECT_EQ(innate_ability->activation_trigger_data.trigger_type, ActivationTriggerType::kHealthInRange);
        EXPECT_EQ(innate_ability->activation_trigger_data.health_lower_limit_percentage, 50_fp);

        // Check attack_skill
        ASSERT_EQ(innate_ability->skills.size(), 1);
        const auto& innate_skill = innate_ability->skills.at(0);

        // Check attack effects
        ASSERT_EQ(innate_skill->effect_package.effects.size(), 1);
        const auto& innate_effect = innate_skill->effect_package.effects.at(0);
        EXPECT_EQ(innate_effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(innate_effect.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(innate_effect.GetExpression().base_value.value, 444_fp);
        EXPECT_TRUE(innate_effect.GetExpression().is_used_as_percentage);
    }

    // Check for kEveryXTime trigger
    {
        const auto& innate_ability = innate_abilities.abilities.at(2);
        EXPECT_EQ(innate_ability->name, "Innate Attack Ability3");
        EXPECT_EQ(innate_ability->activation_trigger_data.trigger_type, ActivationTriggerType::kEveryXTime);
        EXPECT_EQ(innate_ability->activation_trigger_data.activate_every_time_ms, 15000);
    }

    // Check for kEveryXAttack trigger
    {
        const auto& innate_ability = innate_abilities.abilities.at(3);
        EXPECT_EQ(innate_ability->name, "Innate Attack Ability4");
        EXPECT_EQ(innate_ability->activation_trigger_data.trigger_type, ActivationTriggerType::kOnActivateXAbilities);
        EXPECT_EQ(innate_ability->activation_trigger_data.number_of_abilities_activated, 5);
        EXPECT_EQ(innate_ability->activation_trigger_data.activation_cooldown_ms, 300);
        EXPECT_EQ(innate_ability->activation_trigger_data.every_x, true);
    }

    // Check for kEveryXAttack trigger
    {
        const auto& innate_ability = innate_abilities.abilities.at(4);
        EXPECT_EQ(innate_ability->name, "Innate Attack Ability5");
        EXPECT_EQ(innate_ability->activation_trigger_data.trigger_type, ActivationTriggerType::kInRange);
        EXPECT_EQ(innate_ability->activation_trigger_data.activation_radius_units, 5);
        EXPECT_EQ(
            innate_ability->activation_trigger_data.required_sender_conditions.Contains(ConditionType::kPoisoned),
            true);
        EXPECT_EQ(
            innate_ability->activation_trigger_data.required_receiver_conditions.Contains(ConditionType::kPoisoned),
            true);
    }

    // Check Movement Lock Attribute
    {
        const auto& innate_ability3 = innate_abilities.abilities.at(3);
        EXPECT_TRUE(innate_ability3->movement_lock);

        const auto& innate_ability4 = innate_abilities.abilities.at(4);
        EXPECT_FALSE(innate_ability4->movement_lock);
    }
}

TEST_F(JSONTest, LoadCombatUnitGuidanceNotSet)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Axolotl",
    "UnitType": "Illuvial",
    "Stage": 1,
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "Tier": 3,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MoveSpeedSubUnits": 5000,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 40,
        "AttackSpeed": 60,
        "HitChancePercentage": 55,
        "AttackPhysicalDamage": 11,
        "AttackEnergyDamage": 12,
        "AttackPureDamage": 13,
        "AttackDodgeChancePercentage": 14,
        "MaxHealth": 950000,
        "HealthGainEfficiencyPercentage": 75,
        "PhysicalVampPercentage": 66,
        "EnergyVampPercentage": 67,
        "PureVampPercentage": 68,
        "EnergyCost": 60000,
        "StartingEnergy": 30000,
        "EnergyRegeneration": 765,
        "EnergyGainEfficiencyPercentage": 129,
        "OnActivationEnergy": 666,
        "EnergyPiercingPercentage": 31,
        "PhysicalPiercingPercentage": 32,
        "EnergyResist": 33,
        "PhysicalResist": 34,
        "TenacityPercentage": 1,
        "WillpowerPercentage": 2,
        "Grit": 3,
        "Resolve": 4,
        "VulnerabilityPercentage": 5,
        "CritChancePercentage": 6,
        "CritAmplificationPercentage": 7,
        "OmegaPowerPercentage": 8,
        "Thorns": 9,
        "HealthRegeneration": 12345,
        "StartingShield": 20,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack",
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": {
                        "Type": "CurrentFocus"
                    },
                    "Deployment": {
                        "Type": "Projectile",
                        "PreDeploymentDelayPercentage": 25
                    },
                    "Projectile": {
                        "SizeUnits": 5,
                        "SpeedSubUnits": 1234,
                        "IsHoming": false,
                        "IsBlockable": true,
                        "ApplyToAll": true
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 123
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";

    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);

    // Spawn the CombatUnit
    CombatUnitInstanceData instance = CreateCombatUnitInstanceData();
    instance.team = Team::kBlue;
    instance.position = {10, 20};
    instance.level = 60;
    Entity* entity = nullptr;
    SpawnCombatUnit(instance, data, entity);

    // Get the components
    const auto& position_component = entity->Get<PositionComponent>();
    const auto& movement_component = entity->Get<MovementComponent>();
    const auto& stats_component = entity->Get<StatsComponent>();
    const auto& combat_synergy_component = entity->Get<CombatSynergyComponent>();

    // Test values
    // Ensure position correct
    EXPECT_EQ(position_component.GetQ(), 10);
    EXPECT_EQ(position_component.GetR(), 20);

    // Ensure movement attributes correct
    EXPECT_EQ(movement_component.GetMovementSpeedSubUnits(), 5000_fp);
    EXPECT_EQ(movement_component.GetMovementType(), MovementComponent::MovementType::kNavigation);

    ASSERT_EQ(data.type_data.tier, 3);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kMoveSpeedSubUnits), 5000_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackRangeUnits), 42_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kOmegaRangeUnits), 42_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackSpeed), 60_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kHitChancePercentage), 55_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackPhysicalDamage), 14_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackEnergyDamage), 15_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackPureDamage), 16_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kAttackDodgeChancePercentage), 14_fp);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kMaxHealth), 1187500_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kHealthRegeneration), 12345_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kHealthGainEfficiencyPercentage), 75_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kPhysicalVampPercentage), 66_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyVampPercentage), 67_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kPureVampPercentage), 68_fp);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyCost), 60000_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kStartingEnergy), 30000_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyRegeneration), 765_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyGainEfficiencyPercentage), 129_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kOnActivationEnergy), 666_fp);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyPiercingPercentage), 31_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kPhysicalPiercingPercentage), 32_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kEnergyResist), 33_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kPhysicalResist), 34_fp);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kTenacityPercentage), 1_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kWillpowerPercentage), 2_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kGrit), 3_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kResolve), 4_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kVulnerabilityPercentage), 5_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kCritChancePercentage), 6_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kCritAmplificationPercentage), 7_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kOmegaPowerPercentage), 10_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kCritReductionPercentage), 25_fp);
    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kThorns), 9_fp);

    EXPECT_EQ(stats_component.GetBaseValueForType(StatType::kStartingShield), 20_fp);
    EXPECT_EQ(stats_component.GetCurrentSubHyper(), 0_fp);

    // Ensure current values correct
    EXPECT_EQ(stats_component.GetCurrentEnergy(), 30000_fp);
    EXPECT_EQ(stats_component.GetCurrentHealth(), 1187500_fp);

    world->TimeStep();

    // Check combat class, combat affinity and size
    EXPECT_EQ(position_component.GetRadius(), 5);
    EXPECT_TRUE(position_component.IsTakingSpace());
    EXPECT_EQ(combat_synergy_component.GetCombatClass(), CombatClass::kBulwark);
    EXPECT_EQ(combat_synergy_component.GetCombatAffinity(), CombatAffinity::kWater);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kAttack));
    const auto& attack_abilities = abilities_component.GetDataAttackAbilities();

    // Check attack ability
    {
        ASSERT_EQ(attack_abilities.abilities.size(), 1);
        const auto& attack_ability = attack_abilities.abilities.at(0);
        EXPECT_EQ(attack_ability->name, "Attack");
        // Should be 0 as the attack ability duration comes from the attack speed
        EXPECT_EQ(attack_ability->total_duration_ms, 0);

        // Check attack_skill
        ASSERT_EQ(attack_ability->skills.size(), 1);
        const auto& attack_skill = attack_ability->skills.at(0);
        EXPECT_EQ(attack_skill->name, "Attack");
        EXPECT_EQ(attack_skill->targeting.type, SkillTargetingType::kCurrentFocus);
        EXPECT_EQ(attack_skill->targeting.guidance.Contains(GuidanceType::kGround), true);
        EXPECT_EQ(attack_skill->deployment.type, SkillDeploymentType::kProjectile);
        EXPECT_EQ(attack_skill->deployment.guidance.Contains(GuidanceType::kGround), true);
        EXPECT_EQ(attack_skill->projectile.size_units, 5);
        EXPECT_EQ(attack_skill->projectile.speed_sub_units, 1234);
        EXPECT_EQ(attack_skill->projectile.is_homing, false);
        EXPECT_EQ(attack_skill->projectile.is_blockable, true);
        EXPECT_EQ(attack_skill->projectile.apply_to_all, true);
        EXPECT_EQ(attack_skill->deployment.pre_deployment_delay_percentage, 25);
        EXPECT_TRUE(attack_skill->effect_package.attributes.use_hit_chance);

        // Check attack effects
        ASSERT_EQ(attack_skill->effect_package.effects.size(), 1);
        const auto& attack_effect = attack_skill->effect_package.effects.at(0);
        EXPECT_EQ(attack_effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(attack_effect.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(attack_effect.GetExpression().base_value.value, 123_fp);
    }
}

TEST_F(JSONTest, LoadEffectExpression)
{
    TestingDataLoader loader(world->GetLogger());

    {
        constexpr std::string_view json_buffer = R"(
        {
            "Stat": "AttackPhysicalDamage",
            "StatSource": "Sender"
        })";

        auto opt_expression = loader.ParseAndLoadExpression(json_buffer);
        ASSERT_TRUE(opt_expression.has_value());

        const EffectExpression& expression = opt_expression.value();
        EXPECT_EQ(expression.base_value.stat, StatType::kAttackPhysicalDamage);
        EXPECT_EQ(expression.base_value.stat_evaluation_type, StatEvaluationType::kLive);
        EXPECT_EQ(expression.base_value.data_source_type, ExpressionDataSourceType::kSender);
    }
}

TEST_F(JSONTest, LoadCombatUnitSpawnSplashSkill)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Super Axolotl",
    "Stage": 1,
    "UnitType": "Illuvial",
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "Tier": 55,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MaxHealth": 950,
        "StartingEnergy": 30,
        "EnergyCost": 60,
        "PhysicalResist": 60,
        "EnergyResist": 60,
        "TenacityPercentage": 0,
        "WillpowerPercentage": 0,
        "Grit": 0,
        "Resolve": 0,
        "OnActivationEnergy": 0,
        "AttackPhysicalDamage": 45,
        "AttackEnergyDamage": 0,
        "AttackPureDamage": 0,
        "AttackSpeed": 60,
        "MoveSpeedSubUnits": 5000,
        "HitChancePercentage": 100,
        "AttackDodgeChancePercentage": 0,
        "CritChancePercentage": 25,
        "CritAmplificationPercentage": 150,
        "OmegaPowerPercentage": 1,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 42,
        "HealthRegeneration": 0,
        "EnergyRegeneration": 0,
        "EnergyGainEfficiencyPercentage": 100,
        "EnergyGainEfficiencyPercentage": 0,
        "VulnerabilityPercentage": 0,
        "EnergyPiercingPercentage": 0,
        "PhysicalPiercingPercentage": 0,
        "HealthGainEfficiencyPercentage": 100,
        "PhysicalVampPercentage": 0,
        "EnergyVampPercentage": 0,
        "PureVampPercentage": 0,
        "Thorns": 0,
        "StartingShield": 0,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack",
            "TotalDurationMs": 2345,
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [
                            "Ground"
                        ]
                    },
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 0
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Ability 1 - SpawnShield",
            "TotalDurationMs": 1200,
            "Skills": [
                {
                    "Name": "Shield",
                    "PercentageOfAbilityDuration": 100,
                    "Targeting": {
                        "Type": "Self",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Direct",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 25
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "SpawnShield",
                                "DurationMs": 5000,
                                "Expression": {
                                    "Operation": "+",
                                    "Operands": [
                                        {
                                            "Percentage": 20,
                                            "Stat": "MaxHealth",
                                            "StatEvaluationType": "Base",
                                            "StatSource": "Receiver"
                                        },
                                        {
                                            "Operation": "*",
                                            "Operands": [
                                                100,
                                                {
                                                    "Stat": "OmegaPowerPercentage",
                                                    "StatEvaluationType": "Bonus",
                                                    "StatSource": "Sender"
                                                }
                                            ]
                                        }
                                    ]
                                }
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";

    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Get the components
    const auto& stats_component = entity->Get<StatsComponent>();

    // Test values
    EXPECT_EQ(stats_component.GetMaxHealth(), 950_fp);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));
    const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();

    ASSERT_EQ(omega_abilities.abilities.size(), 1);

    // Ability 1 with 1 skill
    {
        const auto& omega_ability = omega_abilities.abilities.at(0);
        EXPECT_EQ(omega_ability->name, "Ability 1 - SpawnShield");
        EXPECT_EQ(omega_ability->total_duration_ms, 1200);

        // Check omega skill
        ASSERT_EQ(omega_ability->skills.size(), 1);
        const auto& omega_skill = omega_ability->skills.at(0);
        EXPECT_EQ(omega_skill->name, "Shield");
        EXPECT_EQ(omega_skill->targeting.type, SkillTargetingType::kSelf);
        EXPECT_EQ(omega_skill->targeting.guidance.Contains(GuidanceType::kGround), true);
        EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kDirect);
        EXPECT_EQ(omega_skill->deployment.guidance.Contains(GuidanceType::kGround), true);
        EXPECT_EQ(omega_skill->percentage_of_ability_duration, 100);
        EXPECT_EQ(omega_skill->deployment.pre_deployment_delay_percentage, 25);

        // Check omega effects
        ASSERT_EQ(omega_skill->effect_package.effects.size(), 1);
        const auto& omega_effect = omega_skill->effect_package.effects.at(0);
        EXPECT_EQ(omega_effect.type_id.type, EffectType::kSpawnShield);
        EXPECT_EQ(omega_effect.lifetime.duration_time_ms, 5000);
        EXPECT_EQ(omega_effect.GetExpression().operation_type, EffectOperationType::kAdd);

        // Check omega ability expression operands
        ASSERT_EQ(omega_effect.GetExpression().operands.size(), 2);

        const auto& operand1 = omega_effect.GetExpression().operands.at(0);
        EXPECT_EQ(operand1.base_value.type, EffectValueType::kStatPercentage);
        EXPECT_EQ(operand1.base_value.stat, StatType::kMaxHealth);
        EXPECT_EQ(operand1.base_value.stat_evaluation_type, StatEvaluationType::kBase);
        EXPECT_TRUE(operand1.base_value.data_source_type == ExpressionDataSourceType::kReceiver);

        const auto& operand2 = omega_effect.GetExpression().operands.at(1);
        EXPECT_EQ(operand2.operation_type, EffectOperationType::kMultiply);
        ASSERT_EQ(operand2.operands.size(), 2);
        EXPECT_EQ(operand2.operands[0].base_value.value, 100_fp);
        EXPECT_EQ(operand2.operands[1].base_value.stat, StatType::kOmegaPowerPercentage);
        EXPECT_EQ(operand2.operands[1].base_value.stat_evaluation_type, StatEvaluationType::kBonus);
        EXPECT_EQ(operand2.operands[1].base_value.data_source_type, ExpressionDataSourceType::kSender);
    }
}

TEST_F(JSONTest, LoadCombatUnitWithEffectsAndValidations)
{
    const char* combat_unit_json_raw = R"(
{
  "Line": "Super Axolotl",
  "Stage": 1,
  "UnitType": "Illuvial",
  "CombatAffinity": "Water",
  "CombatClass": "Bulwark",
  "Tier": 1,
  "SizeUnits": 5,
  "Path": "",
  "Variation": "",
  "Stats": {
    "MaxHealth": 950,
    "StartingEnergy": 30,
    "EnergyCost": 60,
    "PhysicalResist": 60,
    "EnergyResist": 60,
    "TenacityPercentage": 0,
    "WillpowerPercentage": 0,
    "Grit": 0,
    "Resolve": 0,
    "AttackPhysicalDamage": 45,
    "AttackEnergyDamage": 0,
    "AttackPureDamage": 0,
    "AttackSpeed": 60,
    "MoveSpeedSubUnits": 5000,
    "HitChancePercentage": 100,
    "AttackDodgeChancePercentage": 0,
    "CritChancePercentage": 25,
    "CritAmplificationPercentage": 150,
    "OmegaPowerPercentage": 1,
    "AttackRangeUnits": 42,
    "OmegaRangeUnits": 0,
    "HealthRegeneration": 0,
    "EnergyRegeneration": 0,
    "EnergyGainEfficiencyPercentage": 100,
    "OnActivationEnergy": 0,
    "VulnerabilityPercentage": 0,
    "EnergyPiercingPercentage": 0,
    "PhysicalPiercingPercentage": 0,
    "HealthGainEfficiencyPercentage": 100,
    "PhysicalVampPercentage": 0,
    "EnergyVampPercentage": 0,
    "PureVampPercentage": 0,
    "Thorns": 0,
    "StartingShield": 0,
    "CritReductionPercentage": 25
  },
  "AttackAbilitiesSelection": "Cycle",
  "AttackAbilities": [
    {
      "Name": "Attack",
      "TotalDurationMs": 2345,
      "Skills": [
        {
          "Name": "Attack",
          "Targeting": {
              "Type": "CurrentFocus"
          },
          "Deployment": {
              "Type": "Direct"
          },
          "EffectPackage": {
            "Effects": [
              {
                "Type": "InstantDamage",
                "DamageType": "Physical",
                "Expression": 0
              }
            ]
          }
        }
      ]
    }
  ],
  "OmegaAbilitiesSelection": "Cycle",
  "OmegaAbilities": [
    {
      "Name": "Ability Name - Zones",
      "TotalDurationMs": 5000,
      "Skills": [
        {
          "Name": "Ability Skill 1",
          "PercentageOfAbilityDuration": 100,
          "Deployment": {
              "Type": "Direct",
              "PreDeploymentDelayPercentage": 21
          },
          "Targeting": {
            "Type": "Synergy",
            "Group": "Enemy",
            "CombatClass": "Fighter"
          },
          "EffectPackage": {
            "Effects": [
              {
                "Type": "Empower",
                "ActivatedBy": "Omega",
                "DurationMs": 1600,
                "AttachedEffects": [
                  {
                    "Type": "PositiveState",
                    "PositiveState": "Untargetable",
                    "OverlapProcessType": "Sum",
                    "DurationMs": 6500,
                    "DeactivateIfValidationListNotValid": true,
                    "MaxStacks": 10,
                    "ValidationList": [
                      {
                        "Type": "Expression",
                        "ComparisonType": "<",
                        "Left": {
                          "Stat": "CurrentHealth%",
                          "StatSource": "Sender"
                        },
                        "Right": {
                          "Value": 50
                        }
                      },
                      {
                        "Type": "Expression",
                        "ComparisonType": ">",
                        "Left": {
                          "Stat": "AttackSpeed",
                          "StatSource": "Sender"
                        },
                        "Right": {
                          "Value": 2
                        }
                      }
                    ]
                  },
                  {
                    "Type": "NegativeState",
                    "NegativeState": "Root",
                    "DurationMs": 3000,
                    "ValidationList": [
                      {
                        "Type": "Expression",
                        "ComparisonType": "==",
                        "Left": {
                          "Stat": "CurrentHealth",
                          "StatSource": "Sender"
                        },
                        "Right": {
                          "Value": 2000
                        }
                      },
                      {
                        "Type": "DistanceCheck",
                        "FirstEntity": "Self",
                        "ComparisonEntities": "Ally",
                        "ComparisonEntitiesCount": 3,
                        "ComparisonType": ">",
                        "DistanceUnits": 20
                      }
                    ]
                  }
                ]
              }
            ]
          }
        }
      ]
    }
  ]
}
)";

    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));
    const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();

    ASSERT_EQ(omega_abilities.abilities.size(), 1);

    // Ability with 3 zone skills
    {
        const auto& omega_ability = omega_abilities.abilities.at(0);
        EXPECT_EQ(omega_ability->name, "Ability Name - Zones");
        EXPECT_EQ(omega_ability->total_duration_ms, 5000);
        ASSERT_EQ(omega_ability->skills.size(), 1);

        // First skill
        {
            const auto& omega_skill = omega_ability->skills.at(0);
            EXPECT_EQ(omega_skill->name, "Ability Skill 1");

            // Check omega effects
            ASSERT_EQ(omega_skill->effect_package.effects.size(), 1);

            // First effect
            {
                const auto& omega_effect = omega_skill->effect_package.effects.at(0);
                EXPECT_EQ(omega_effect.type_id.type, EffectType::kEmpower);
                EXPECT_EQ(omega_effect.lifetime.duration_time_ms, 1600);

                EXPECT_EQ(omega_effect.attached_effects.size(), 2);

                // First Attached effect
                {
                    const auto& attached_effect = omega_effect.attached_effects.at(0);
                    EXPECT_EQ(attached_effect.type_id.type, EffectType::kPositiveState);
                    EXPECT_EQ(attached_effect.type_id.positive_state, EffectPositiveState::kUntargetable);
                    EXPECT_EQ(attached_effect.lifetime.duration_time_ms, 6500);
                    EXPECT_EQ(attached_effect.lifetime.deactivate_if_validation_list_not_valid, true);
                    EXPECT_EQ(attached_effect.lifetime.max_stacks, 10);
                    EXPECT_EQ(attached_effect.lifetime.overlap_process_type, EffectOverlapProcessType::kSum);

                    // Validations
                    {
                        const auto& effect_expression_comparisons = attached_effect.validations.expression_comparisons;
                        ASSERT_EQ(effect_expression_comparisons.size(), 2);

                        // First Validator (Metered state expression)
                        {
                            const auto& validation = effect_expression_comparisons.at(0);
                            EXPECT_EQ(validation.validation_type, ValidationType::kExpression);
                            EXPECT_EQ(validation.comparison_type, ComparisonType::kLess);
                            EXPECT_EQ(validation.left.base_value.type, EffectValueType::kMeteredStatPercentage);
                            EXPECT_EQ(validation.left.base_value.stat, StatType::kCurrentHealth);
                            EXPECT_EQ(validation.right.base_value.value, 50_fp);
                        }

                        // Second Validator (UnMetered state expression)
                        {
                            const auto& validation = effect_expression_comparisons.at(1);
                            EXPECT_EQ(validation.validation_type, ValidationType::kExpression);
                            EXPECT_EQ(validation.comparison_type, ComparisonType::kGreater);
                            EXPECT_EQ(validation.left.base_value.stat, StatType::kAttackSpeed);
                            EXPECT_EQ(validation.right.base_value.value, 2_fp);
                        }
                    }
                }

                // Second Attached effect
                {
                    const auto& attached_effect = omega_effect.attached_effects.at(1);
                    EXPECT_EQ(attached_effect.type_id.type, EffectType::kNegativeState);
                    EXPECT_EQ(attached_effect.type_id.negative_state, EffectNegativeState::kRoot);
                    EXPECT_EQ(attached_effect.lifetime.duration_time_ms, 3000);

                    // Validation
                    {
                        const auto& distance_checks = attached_effect.validations.distance_checks;
                        const auto& effect_expression_comparisons = attached_effect.validations.expression_comparisons;
                        EXPECT_EQ(distance_checks.size(), 1);
                        EXPECT_EQ(effect_expression_comparisons.size(), 1);

                        // First Validator (DistanceCheck)
                        {
                            const auto& validation = distance_checks.at(0);
                            EXPECT_EQ(validation.validation_type, ValidationType::kDistanceCheck);
                            EXPECT_EQ(validation.comparison_type, ComparisonType::kGreater);
                            EXPECT_EQ(validation.allegiance, AllegianceType::kAlly);
                            EXPECT_EQ(validation.distance_units, 20);
                            EXPECT_EQ(validation.first_entity, ValidationStartEntity::kSelf);
                            EXPECT_EQ(validation.entities_count, 3);
                        }

                        // Second Validator (UnMetered state expression)
                        {
                            const auto& validation = effect_expression_comparisons.at(0);
                            EXPECT_EQ(validation.validation_type, ValidationType::kExpression);
                            EXPECT_EQ(validation.comparison_type, ComparisonType::kEqual);
                            EXPECT_EQ(validation.left.base_value.stat, StatType::kCurrentHealth);
                            EXPECT_EQ(validation.right.base_value.value, 2000_fp);
                        }
                    }
                }
            }
        }
    }
}

TEST_F(JSONTest, LoadCombatUnitZoneSkill)  // NOLINT
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Super Axolotl",
    "Stage": 1,
    "UnitType": "Illuvial",
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "Tier": 1,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MaxHealth": 950,
        "StartingEnergy": 30,
        "EnergyCost": 60,
        "PhysicalResist": 60,
        "EnergyResist": 60,
        "TenacityPercentage": 0,
        "WillpowerPercentage": 0,
        "Grit": 0,
        "Resolve": 0,
        "AttackPhysicalDamage": 45,
        "AttackEnergyDamage": 0,
        "AttackPureDamage": 0,
        "AttackSpeed": 60,
        "MoveSpeedSubUnits": 5000,
        "HitChancePercentage": 100,
        "AttackDodgeChancePercentage": 0,
        "CritChancePercentage": 25,
        "CritAmplificationPercentage": 150,
        "OmegaPowerPercentage": 1,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 0,
        "HealthRegeneration": 0,
        "EnergyRegeneration": 0,
        "EnergyGainEfficiencyPercentage": 100,
        "OnActivationEnergy": 0,
        "VulnerabilityPercentage": 0,
        "EnergyPiercingPercentage": 0,
        "PhysicalPiercingPercentage": 0,
        "HealthGainEfficiencyPercentage": 100,
        "PhysicalVampPercentage": 0,
        "EnergyVampPercentage": 0,
        "PureVampPercentage": 0,
        "Thorns": 0,
        "StartingShield": 0,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack",
            "TotalDurationMs": 2345,
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 0
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Ability Name - Zones",
            "TotalDurationMs": 5000,
            "Skills": [
                {
                    "Name": "Ability Skill 1",
                    "PercentageOfAbilityDuration": 33,
                    "Targeting": {
                        "Type": "CombatStatCheck",
                        "Group": "Enemy",
                        "Lowest": true,
                        "Self": true,
                        "Num": 3,
                        "Stat": "CurrentHealth",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Zone",
                        "Guidance": [ "Ground"],
                        "PreDeploymentDelayPercentage": 40
                    },
                    "Zone": {
                        "Shape": "Triangle",
                        "RadiusUnits": 7000,
                        "MaxRadiusUnits": 8000,
                        "DirectionDegrees": 0,
                        "DurationMs": 1456,
                        "FrequencyMs": 2379,
                        "GrowthRateSubUnits": 312,
                        "MovementSpeedSubUnits": 1235,
                        "ApplyOnce": true,
                        "Attach": true,
                        "DestroyWithSender": true
                    },
                    "EffectPackage": {
                        "Attributes": {
                            "CanCrit": true,
                            "CumulativeDamage": false,
                            "SplitDamage": true,
                            "AlwaysCrit": true,
                            "IsTrueshot": true,
                            "RotateToTarget": true,
                            "EnergyGainBonus": 5,
                            "EnergyGainAmplification": 7,
                            "EnergyBurnBonus": 3,
                            "EnergyBurnAmplification": 8,
                            "HealBonus": 5,
                            "HealAmplification": 15,
                            "DamageBonus": 17,
                            "PhysicalDamageBonus": 3,
                            "PureDamageBonus": 4,
                            "EnergyDamageBonus": 5,
                            "DamageAmplification": 9,
                            "PhysicalDamageAmplification": 6,
                            "PureDamageAmplification": 7,
                            "EnergyDamageAmplification": 8,
                            "ShieldBonus": 3,
                            "ShieldAmplification": 13
                        },
                        "Effects": [
                            {
                                "Type": "NegativeState",
                                "NegativeState": "Root",
                                "IsAlwaysStackable": true,
                                "CanCleanse": false,
                                "DurationMs": 2000
                            },
                            {
                                "Type": "DOT",
                                "DamageType": "Pure",
                                "DurationMs": 5000,
                                "FrequencyMs": 200,
                                "Expression": 500000
                            },
                            {
                                "Type": "NegativeState",
                                "NegativeState": "Focused",
                                "DurationMs": 2000
                            },
                            {
                                "Type": "Execute",
                                "DurationMs": 3000,
                                "Expression": 50,
                                "AbilityTypes": ["Attack"]
                            }
                        ]
                    }
                },)"
                                       // Split this giant string to fight MSVC string literal limit (error C2026)
                                       R"(                {
                    "Name": "Ability Skill 2",
                    "PercentageOfAbilityDuration": 33,
                    "Targeting": {
                        "Type": "DistanceCheck",
                        "Group": "Ally",
                        "Lowest": true,
                        "Num": 5,
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Zone",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 10
                    },
                    "Zone": {
                        "Shape": "Hexagon",
                        "RadiusUnits": 5000,
                        "DurationMs": 3214,
                        "FrequencyMs": 4000,
                        "GrowthRateSubUnits": 120,
                        "MovementSpeedSubUnits": 2346,
                        "ApplyOnce": true,
                        "GlobalCollisionID": 1,
                        "PredefinedSpawnPosition": "AllyBorderCenter",
                        "PredefinedTargetPosition": "AllyBorderCenter"
                    },
                    "EffectPackage": {
                        "Attributes": {
                            "ExcessVampToShieldDurationMs": 7,
                            "VampiricPercentage": 1,
                            "PiercingPercentage": 2,
                            "CumulativeDamage": true,
                            "SplitDamage": false,
                            "ExcessVampToShield": true,
                            "ExploitWeakness": true
                        },
                        "Effects": [
                            {
                                "Type": "DOT",
                                "DamageType": "Energy",
                                "Expression": 20000,
                                "IsConsumable": true,
                                "ActivatedBy": "Omega",
                                "ActivationsUntilExpiry": 5,
                                "ExcessHealToShield": 3,
                                "MissingHealthPercentageToHealth": 4,
                                "MaxHealthPercentageToHealth": 5,
                                "CleanseNegativeStates": true,
                                "CleanseConditions": true,
                                "CleanseDOTs": true,
                                "CleanseDebuffs": true,
                                "ToShieldPercentage": 8,
                                "ToShieldDurationMs": 9,
                                "ShieldBypass": true,
                                "Conditions": ["Poisoned", "Wounded", "Burned", "Frosted", "Shielded"]
                            }
                        ]
                    }

                },
                {
                    "Name": "Ability Skill 3",
                    "PercentageOfAbilityDuration": 34,
                    "Targeting": {
                        "Type": "InZone",
                        "Group": "All",
                        "RadiusUnits": 142
                    },
                    "Deployment": {
                        "Type": "Zone",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 20
                    },
                    "Zone": {
                        "Shape": "Rectangle",
                        "WidthUnits": 12000,
                        "HeightUnits": 6000,
                        "DurationMs": 4214,
                        "FrequencyMs": 5000,
                        "GrowthRateSubUnits": 220,
                        "MovementSpeedSubUnits": 3457,
                        "ApplyOnce": true,
                        "PredefinedSpawnPosition": "EnemyBorderCenter",
                        "PredefinedTargetPosition": "EnemyBorderCenter"
                    },
                    "EffectPackage": {
                        "Attributes": {
                            "RefundHealth": 50,
                            "RefundEnergy": 50
                        },
                        "Effects": [
                            {
                                "Type": "Empower",
                                "ActivatedBy": "Omega",
                                "DurationMs": 1600,
                                "AttachedEffects": [
                                    {
                                        "Type": "InstantDamage",
                                        "DamageType": "Pure",
                                        "Expression": 45
                                    },
                                    {
                                        "Type": "InstantDamage",
                                        "DamageType": "Physical",
                                        "Expression": 234
                                    },
                                    {
                                        "Type": "PlaneChange",
                                        "PlaneChange": "Airborne",
                                        "DurationMs": 6500
                                    }
                                ],
                                "AttachedEffectPackageAttributes": {
                                    "IsTrueshot": true,
                                    "CanCrit": true,
                                    "AlwaysCrit": true,
                                    "RotateToTarget": true
                                }
                            }
                        ]
                    }
                },
                {
                    "Name": "Ability Skill 4",
                    "PercentageOfAbilityDuration": 34,
                    "Targeting": {
                        "Type": "ExpressionCheck",
                        "Num": 1,
                        "Group": "Ally",
                        "Lowest": true,
                        "Expression": {
                            "Operation": "/",
                            "Operands": [
                                {
                                    "Operation": "*",
                                    "Operands": [
                                        {
                                            "Value": 100
                                        },
                                        {
                                            "Stat": "CurrentHealth",
                                            "StatSource": "Sender"
                                        }
                                    ]
                                },
                                {
                                    "Stat": "MaxHealth",
                                    "StatSource": "Sender"
                                }
                            ]
                        }
                    },
                    "Deployment": {
                        "Type": "Direct",
                        "PreDeploymentDelayPercentage": 20
                    },
                    "EffectPackage": {
                        "Attributes": {
                            "RefundHealth": 50,
                            "RefundEnergy": 50
                        },
                        "Effects": [
                            {
                                "Type": "Empower",
                                "ActivatedBy": "Omega",
                                "DurationMs": 1600,
                                "AttachedEffects": [
                                    {
                                        "Type": "InstantDamage",
                                        "DamageType": "Pure",
                                        "Expression": 45
                                    },
                                    {
                                        "Type": "InstantDamage",
                                        "DamageType": "Physical",
                                        "Expression": 234
                                    },
                                    {
                                        "Type": "PositiveState",
                                        "PositiveState": "Untargetable",
                                        "IsConsumable": true,
                                        "ActivationsUntilExpiry": 15,
                                        "ActivatedBy": "Attack"
                                    },
                                    {
                                        "Type": "PositiveState",
                                        "PositiveState": "Immune",
                                        "IsConsumable": true,
                                        "ActivationsUntilExpiry": 15,
                                        "ActivatedBy": "Attack"
                                    },
                                    {
                                        "Type": "PositiveState",
                                        "PositiveState": "Immune",
                                        "IsConsumable": true,
                                        "ActivationsUntilExpiry": 15,
                                        "ActivatedBy": "Attack",
                                        "AttachableEffectTypeList": [
                                            {
                                                "Type": "NegativeState",
                                                "NegativeState": "Focused"
                                            },
                                            {
                                                "Type": "NegativeState",
                                                "NegativeState": "Root"
                                            }
                                        ]
                                    }
                                ],
                                "AttachedEffectPackageAttributes": {
                                    "IsTrueshot": true,
                                    "CanCrit": true,
                                    "AlwaysCrit": true,
                                    "RotateToTarget": true
                                }
                            }
                        ]
                    }
                }
            ]
        }
    ]
})";

    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Get the components
    const auto& stats_component = entity->Get<StatsComponent>();

    // Test values
    EXPECT_EQ(stats_component.GetMaxHealth(), 950_fp);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));
    const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();

    ASSERT_EQ(omega_abilities.abilities.size(), 1);

    // Ability with 3 zone skills
    {
        const auto& omega_ability = omega_abilities.abilities.at(0);
        EXPECT_EQ(omega_ability->name, "Ability Name - Zones");
        EXPECT_EQ(omega_ability->total_duration_ms, 5000);
        ASSERT_EQ(omega_ability->skills.size(), 4);

        // First skill
        {
            const auto& omega_skill = omega_ability->skills.at(0);
            EXPECT_EQ(omega_skill->name, "Ability Skill 1");
            EXPECT_EQ(omega_skill->percentage_of_ability_duration, 33);
            EXPECT_EQ(omega_skill->deployment.pre_deployment_delay_percentage, 40);

            EXPECT_EQ(omega_skill->effect_package.attributes.can_crit, true);
            EXPECT_EQ(omega_skill->effect_package.attributes.cumulative_damage, false);
            EXPECT_EQ(omega_skill->effect_package.attributes.split_damage, true);
            EXPECT_EQ(omega_skill->effect_package.attributes.always_crit, true);
            EXPECT_EQ(omega_skill->effect_package.attributes.is_trueshot, true);
            EXPECT_EQ(omega_skill->effect_package.attributes.rotate_to_target, true);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.damage_bonus), 17_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.physical_damage_bonus), 3_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.pure_damage_bonus), 4_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.energy_damage_bonus), 5_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.physical_damage_amplification), 6_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.pure_damage_amplification), 7_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.energy_damage_amplification), 8_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.damage_amplification), 9_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.energy_gain_bonus), 5_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.energy_gain_amplification), 7_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.energy_burn_amplification), 8_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.energy_burn_bonus), 3_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.heal_bonus), 5_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.heal_amplification), 15_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.shield_bonus), 3_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.shield_amplification), 13_fp);

            EXPECT_EQ(omega_skill->targeting.type, SkillTargetingType::kCombatStatCheck);
            EXPECT_EQ(omega_skill->targeting.group, AllegianceType::kEnemy);
            EXPECT_EQ(omega_skill->targeting.lowest, true);
            EXPECT_EQ(omega_skill->targeting.self, true);
            EXPECT_EQ(omega_skill->targeting.num, 3);
            EXPECT_EQ(omega_skill->targeting.stat_type, StatType::kCurrentHealth);

            EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kZone);
            EXPECT_EQ(omega_skill->zone.shape, ZoneEffectShape::kTriangle);
            EXPECT_EQ(omega_skill->zone.radius_units, 7000);
            EXPECT_EQ(omega_skill->zone.max_radius_units, 8000);
            EXPECT_EQ(omega_skill->zone.duration_ms, 1456);
            EXPECT_EQ(omega_skill->zone.frequency_ms, 2379);
            EXPECT_EQ(omega_skill->zone.growth_rate_sub_units_per_time_step, 312);
            EXPECT_EQ(omega_skill->zone.apply_once, true);
            EXPECT_EQ(omega_skill->zone.global_collision_id, kInvalidGlobalCollisionID);
            EXPECT_EQ(omega_skill->zone.movement_speed_sub_units_per_time_step, 1235);
            EXPECT_EQ(omega_skill->zone.attach_to_target, true);
            EXPECT_EQ(omega_skill->zone.destroy_with_sender, true);
            EXPECT_EQ(omega_skill->zone.predefined_spawn_position, PredefinedGridPosition::kNone);
            EXPECT_EQ(omega_skill->zone.predefined_target_position, PredefinedGridPosition::kNone);

            // Check omega effects
            ASSERT_EQ(omega_skill->effect_package.effects.size(), 4);

            // First effect
            {
                const auto& omega_effect = omega_skill->effect_package.effects.at(0);
                EXPECT_EQ(omega_effect.type_id.type, EffectType::kNegativeState);
                EXPECT_EQ(omega_effect.lifetime.duration_time_ms, 2000);
                EXPECT_EQ(omega_effect.type_id.negative_state, EffectNegativeState::kRoot);
                EXPECT_FALSE(omega_effect.can_cleanse);
            }

            // Second effect
            {
                const auto& omega_effect = omega_skill->effect_package.effects.at(1);
                EXPECT_EQ(omega_effect.type_id.type, EffectType::kDamageOverTime);
                EXPECT_EQ(omega_effect.lifetime.duration_time_ms, 5000);
                EXPECT_EQ(omega_effect.type_id.damage_type, EffectDamageType::kPure);
                EXPECT_EQ(omega_effect.GetExpression().base_value.value, 500000_fp);
                EXPECT_EQ(omega_effect.lifetime.frequency_time_ms, 200);
            }

            // Third effect
            {
                const auto& omega_effect = omega_skill->effect_package.effects.at(2);
                EXPECT_EQ(omega_effect.type_id.type, EffectType::kNegativeState);
                EXPECT_EQ(omega_effect.lifetime.duration_time_ms, 2000);
                EXPECT_EQ(omega_effect.type_id.negative_state, EffectNegativeState::kFocused);
            }

            // Fourth effect
            {
                const auto& omega_effect = omega_skill->effect_package.effects.at(3);
                EXPECT_EQ(omega_effect.type_id.type, EffectType::kExecute);
                EXPECT_EQ(omega_effect.lifetime.duration_time_ms, 3000);

                EXPECT_TRUE(VectorHelper::ContainsValue(omega_effect.ability_types, AbilityType::kAttack));
                EXPECT_EQ(omega_effect.GetExpression().base_value.value, 50_fp);
            }
        }

        // Second skill
        {
            const auto& omega_skill = omega_ability->skills.at(1);
            EXPECT_EQ(omega_skill->name, "Ability Skill 2");
            EXPECT_EQ(omega_skill->percentage_of_ability_duration, 33);
            EXPECT_EQ(omega_skill->deployment.pre_deployment_delay_percentage, 10);

            EXPECT_EQ(omega_skill->effect_package.attributes.cumulative_damage, true);
            EXPECT_EQ(omega_skill->effect_package.attributes.split_damage, false);
            EXPECT_EQ(omega_skill->effect_package.attributes.can_crit, false);
            EXPECT_EQ(omega_skill->effect_package.attributes.always_crit, false);
            EXPECT_EQ(omega_skill->effect_package.attributes.is_trueshot, false);
            EXPECT_EQ(omega_skill->effect_package.attributes.rotate_to_target, false);

            EXPECT_EQ(omega_skill->targeting.type, SkillTargetingType::kDistanceCheck);
            EXPECT_EQ(omega_skill->targeting.group, AllegianceType::kAlly);
            EXPECT_EQ(omega_skill->targeting.lowest, true);
            EXPECT_EQ(omega_skill->targeting.num, 5);

            EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kZone);
            EXPECT_EQ(omega_skill->zone.shape, ZoneEffectShape::kHexagon);
            EXPECT_EQ(omega_skill->zone.radius_units, 5000);
            EXPECT_EQ(omega_skill->zone.duration_ms, 3214);
            EXPECT_EQ(omega_skill->zone.frequency_ms, 4000);
            EXPECT_EQ(omega_skill->zone.growth_rate_sub_units_per_time_step, 120);
            EXPECT_EQ(omega_skill->zone.apply_once, true);
            EXPECT_EQ(omega_skill->zone.global_collision_id, 1);
            EXPECT_EQ(omega_skill->effect_package.attributes.excess_vamp_to_shield, true);
            EXPECT_EQ(omega_skill->effect_package.attributes.exploit_weakness, true);
            EXPECT_EQ(omega_skill->zone.movement_speed_sub_units_per_time_step, 2346);
            EXPECT_EQ(omega_skill->zone.predefined_spawn_position, PredefinedGridPosition::kAllyBorderCenter);
            EXPECT_EQ(omega_skill->zone.predefined_target_position, PredefinedGridPosition::kAllyBorderCenter);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.vampiric_percentage), 1_fp);
            EXPECT_EQ(omega_skill->effect_package.attributes.excess_vamp_to_shield_duration_ms, 7);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.piercing_percentage), 2_fp);

            // Check omega ability effects
            ASSERT_EQ(omega_skill->effect_package.effects.size(), 1);

            // First effect
            {
                const auto& omega_effect = omega_skill->effect_package.effects.at(0);
                EXPECT_EQ(omega_effect.type_id.type, EffectType::kDamageOverTime);
                EXPECT_EQ(omega_effect.type_id.damage_type, EffectDamageType::kEnergy);
                EXPECT_EQ(omega_effect.GetExpression().base_value.value, 20000_fp);

                EXPECT_EQ(omega_effect.lifetime.is_consumable, true);
                EXPECT_EQ(omega_effect.lifetime.activations_until_expiry, 5);

                EXPECT_EQ(omega_effect.attributes.excess_heal_to_shield, 3_fp);
                EXPECT_EQ(omega_effect.attributes.missing_health_percentage_to_health, 4_fp);
                EXPECT_EQ(omega_effect.attributes.max_health_percentage_to_health, 5_fp);
                EXPECT_EQ(omega_effect.attributes.cleanse_negative_states, true);
                EXPECT_EQ(omega_effect.attributes.cleanse_conditions, true);
                EXPECT_EQ(omega_effect.attributes.cleanse_dots, true);
                EXPECT_EQ(omega_effect.attributes.cleanse_debuffs, true);
                EXPECT_EQ(omega_effect.attributes.shield_bypass, true);
                EXPECT_EQ(omega_effect.attributes.to_shield_percentage, 8_fp);
                EXPECT_EQ(omega_effect.attributes.to_shield_duration_ms, 9);

                ASSERT_EQ(omega_effect.required_conditions.Size(), 5);
                EXPECT_TRUE(omega_effect.required_conditions.Contains(ConditionType::kPoisoned));
                EXPECT_TRUE(omega_effect.required_conditions.Contains(ConditionType::kWounded));
                EXPECT_TRUE(omega_effect.required_conditions.Contains(ConditionType::kBurned));
                EXPECT_TRUE(omega_effect.required_conditions.Contains(ConditionType::kFrosted));
                EXPECT_TRUE(omega_effect.required_conditions.Contains(ConditionType::kShielded));
            }
        }

        // Third skill
        {
            const auto& omega_skill = omega_ability->skills.at(2);
            EXPECT_EQ(omega_skill->name, "Ability Skill 3");
            EXPECT_EQ(omega_skill->percentage_of_ability_duration, 34);
            EXPECT_EQ(omega_skill->deployment.pre_deployment_delay_percentage, 20);

            EXPECT_EQ(omega_skill->effect_package.attributes.can_crit, false);
            EXPECT_EQ(omega_skill->effect_package.attributes.always_crit, false);
            EXPECT_EQ(omega_skill->effect_package.attributes.is_trueshot, false);
            EXPECT_EQ(omega_skill->effect_package.attributes.rotate_to_target, false);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.refund_health), 50_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.refund_energy), 50_fp);

            EXPECT_EQ(omega_skill->targeting.type, SkillTargetingType::kInZone);
            EXPECT_EQ(omega_skill->targeting.group, AllegianceType::kAll);
            EXPECT_EQ(omega_skill->targeting.radius_units, 142);

            EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kZone);
            EXPECT_EQ(omega_skill->deployment.guidance.Contains(GuidanceType::kGround), true);
            EXPECT_EQ(omega_skill->zone.shape, ZoneEffectShape::kRectangle);
            EXPECT_EQ(omega_skill->zone.width_units, 12000);
            EXPECT_EQ(omega_skill->zone.height_units, 6000);
            EXPECT_EQ(omega_skill->zone.duration_ms, 4214);
            EXPECT_EQ(omega_skill->zone.frequency_ms, 5000);
            EXPECT_EQ(omega_skill->zone.growth_rate_sub_units_per_time_step, 220);
            EXPECT_EQ(omega_skill->zone.apply_once, true);
            EXPECT_EQ(omega_skill->zone.movement_speed_sub_units_per_time_step, 3457);
            EXPECT_EQ(omega_skill->zone.predefined_spawn_position, PredefinedGridPosition::kEnemyBorderCenter);
            EXPECT_EQ(omega_skill->zone.predefined_target_position, PredefinedGridPosition::kEnemyBorderCenter);

            // Check omega ability effects
            ASSERT_EQ(omega_skill->effect_package.effects.size(), 1);

            // First effect
            {
                const auto& omega_effect = omega_skill->effect_package.effects.at(0);
                EXPECT_EQ(omega_effect.type_id.type, EffectType::kEmpower);
                EXPECT_EQ(omega_effect.lifetime.activated_by, AbilityType::kOmega);
                EXPECT_EQ(omega_effect.lifetime.duration_time_ms, 1600);

                // Empower has 3 effects
                ASSERT_EQ(omega_effect.attached_effects.size(), 3);
                {
                    const auto& empower_effect = omega_effect.attached_effects.at(0);
                    EXPECT_EQ(empower_effect.type_id.type, EffectType::kInstantDamage);
                    EXPECT_EQ(empower_effect.type_id.damage_type, EffectDamageType::kPure);
                    EXPECT_EQ(empower_effect.GetExpression().base_value.value, 45_fp);
                }
                {
                    const auto& empower_effect = omega_effect.attached_effects.at(1);
                    EXPECT_EQ(empower_effect.type_id.type, EffectType::kInstantDamage);
                    EXPECT_EQ(empower_effect.type_id.damage_type, EffectDamageType::kPhysical);
                    EXPECT_EQ(empower_effect.GetExpression().base_value.value, 234_fp);
                }
                {
                    const auto& empower_effect = omega_effect.attached_effects.at(2);
                    EXPECT_EQ(empower_effect.type_id.type, EffectType::kPlaneChange);
                    EXPECT_EQ(empower_effect.type_id.plane_change, EffectPlaneChange::kAirborne);
                    EXPECT_EQ(empower_effect.lifetime.duration_time_ms, 6500);
                }

                EXPECT_TRUE(omega_effect.attached_effect_package_attributes.always_crit);
                EXPECT_TRUE(omega_effect.attached_effect_package_attributes.is_trueshot);
                EXPECT_TRUE(omega_effect.attached_effect_package_attributes.can_crit);
                EXPECT_TRUE(omega_effect.attached_effect_package_attributes.rotate_to_target);
            }
        }

        // Fourth skill
        {
            const auto& omega_skill = omega_ability->skills.at(3);
            EXPECT_EQ(omega_skill->name, "Ability Skill 4");
            EXPECT_EQ(omega_skill->percentage_of_ability_duration, 34);
            EXPECT_EQ(omega_skill->deployment.pre_deployment_delay_percentage, 20);

            EXPECT_EQ(omega_skill->effect_package.attributes.can_crit, false);
            EXPECT_EQ(omega_skill->effect_package.attributes.always_crit, false);
            EXPECT_EQ(omega_skill->effect_package.attributes.is_trueshot, false);
            EXPECT_EQ(omega_skill->effect_package.attributes.rotate_to_target, false);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.refund_health), 50_fp);
            EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.refund_energy), 50_fp);

            EXPECT_EQ(omega_skill->targeting.type, SkillTargetingType::kExpressionCheck);
            EXPECT_EQ(omega_skill->targeting.group, AllegianceType::kAlly);
            EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kDirect);

            // Check omega ability expression operands
            ASSERT_EQ(omega_skill->targeting.expression.operands.size(), 2);

            const auto& operand1 = omega_skill->targeting.expression.operands.at(0);
            EXPECT_EQ(operand1.operation_type, EffectOperationType::kMultiply);
            EXPECT_EQ(operand1.base_value.type, EffectValueType::kValue);
            EXPECT_EQ(operand1.operands[0].base_value.value, 100_fp);
            EXPECT_EQ(operand1.operands[1].base_value.stat, StatType::kCurrentHealth);
            EXPECT_EQ(operand1.base_value.data_source_type, ExpressionDataSourceType::kSender);

            const auto& operand2 = omega_skill->targeting.expression.operands.at(1);
            EXPECT_EQ(operand2.base_value.stat, StatType::kMaxHealth);

            // Check omega ability effects
            ASSERT_EQ(omega_skill->effect_package.effects.size(), 1);

            // First effect
            {
                const auto& omega_effect = omega_skill->effect_package.effects.at(0);
                EXPECT_EQ(omega_effect.type_id.type, EffectType::kEmpower);
                EXPECT_EQ(omega_effect.lifetime.activated_by, AbilityType::kOmega);
                EXPECT_EQ(omega_effect.lifetime.duration_time_ms, 1600);

                // Empower has 5 effects
                ASSERT_EQ(omega_effect.attached_effects.size(), 5);
                {
                    const auto& empower_effect = omega_effect.attached_effects.at(0);
                    EXPECT_EQ(empower_effect.type_id.type, EffectType::kInstantDamage);
                    EXPECT_EQ(empower_effect.type_id.damage_type, EffectDamageType::kPure);
                    EXPECT_EQ(empower_effect.GetExpression().base_value.value, 45_fp);
                }
                {
                    const auto& empower_effect = omega_effect.attached_effects.at(1);
                    EXPECT_EQ(empower_effect.type_id.type, EffectType::kInstantDamage);
                    EXPECT_EQ(empower_effect.type_id.damage_type, EffectDamageType::kPhysical);
                    EXPECT_EQ(empower_effect.GetExpression().base_value.value, 234_fp);
                }
                {
                    const auto& empower_effect = omega_effect.attached_effects.at(2);
                    EXPECT_EQ(empower_effect.type_id.type, EffectType::kPositiveState);
                    EXPECT_EQ(empower_effect.type_id.positive_state, EffectPositiveState::kUntargetable);
                    EXPECT_EQ(empower_effect.lifetime.duration_time_ms, kTimeInfinite);
                    EXPECT_EQ(empower_effect.lifetime.is_consumable, true);
                    EXPECT_EQ(empower_effect.lifetime.activated_by, AbilityType::kAttack);
                    EXPECT_EQ(empower_effect.lifetime.activations_until_expiry, 15);
                }
                {
                    const auto& empower_effect = omega_effect.attached_effects.at(3);
                    EXPECT_EQ(empower_effect.type_id.type, EffectType::kPositiveState);
                    EXPECT_EQ(empower_effect.type_id.positive_state, EffectPositiveState::kImmune);
                    EXPECT_EQ(empower_effect.immuned_effect_types.size(), 0);
                }
                {
                    const auto& empower_effect = omega_effect.attached_effects.at(4);
                    EXPECT_EQ(empower_effect.type_id.type, EffectType::kPositiveState);
                    EXPECT_EQ(empower_effect.type_id.positive_state, EffectPositiveState::kImmune);
                    EXPECT_EQ(empower_effect.immuned_effect_types.size(), 2);

                    // ignored effect type 0: (negative state, focused)
                    EXPECT_EQ(empower_effect.immuned_effect_types.at(0).type, EffectType::kNegativeState);
                    EXPECT_EQ(empower_effect.immuned_effect_types.at(0).negative_state, EffectNegativeState::kFocused);

                    // ignored effect type 1: (negative state, root)
                    EXPECT_EQ(empower_effect.immuned_effect_types.at(1).type, EffectType::kNegativeState);
                    EXPECT_EQ(empower_effect.immuned_effect_types.at(1).negative_state, EffectNegativeState::kRoot);
                }

                EXPECT_TRUE(omega_effect.attached_effect_package_attributes.always_crit);
                EXPECT_TRUE(omega_effect.attached_effect_package_attributes.is_trueshot);
                EXPECT_TRUE(omega_effect.attached_effect_package_attributes.can_crit);
                EXPECT_TRUE(omega_effect.attached_effect_package_attributes.rotate_to_target);
            }
        }
    }
}

TEST_F(JSONTest, LoadCombatUnitSpawnSkill)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Super Axolotl",
    "Stage": 1,
    "UnitType": "Illuvial",
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "Path": "sadsad",
    "Variation": "Meow",
    "Tier": 1,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MaxHealth": 950,
        "StartingEnergy": 30,
        "EnergyCost": 60,
        "PhysicalResist": 60,
        "EnergyResist": 60,
        "TenacityPercentage": 0,
        "WillpowerPercentage": 0,
        "Grit": 0,
        "Resolve": 0,
        "AttackPhysicalDamage": 45,
        "AttackEnergyDamage": 0,
        "AttackPureDamage": 0,
        "AttackSpeed": 60,
        "MoveSpeedSubUnits": 5000,
        "HitChancePercentage": 100,
        "AttackDodgeChancePercentage": 0,
        "CritChancePercentage": 25,
        "CritAmplificationPercentage": 150,
        "OmegaPowerPercentage": 1,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 42,
        "HealthRegeneration": 0,
        "EnergyRegeneration": 0,
        "EnergyGainEfficiencyPercentage": 100,
        "OnActivationEnergy": 0,
        "VulnerabilityPercentage": 0,
        "EnergyPiercingPercentage": 0,
        "PhysicalPiercingPercentage": 0,
        "HealthGainEfficiencyPercentage": 100,
        "PhysicalVampPercentage": 0,
        "EnergyVampPercentage": 0,
        "PureVampPercentage": 0,
        "Thorns": 0,
        "StartingShield": 0,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack",
            "TotalDurationMs": 2345,
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "EffectPackage": {
                        "Attributes": {
                            "UseHitChance": false
                        },
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 0
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Ability Name - Spawning",
            "TotalDurationMs": 2500,
            "Skills": [
                {
                    "Name": "Ability Skill 1",
                    "PercentageOfAbilityDuration": 100,
                    "Targeting": {
                        "Type": "CurrentFocus"
                    },
                    "EffectPackage": {
                        "Effects": []
                    },
                    "Deployment": {
                        "Type": "SpawnedCombatUnit",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 21
                    },
                    "SpawnCombatUnit": {
                        "Direction": "TopLeft",
                        "Linked": true,
                        "CombatUnit": {
                            "Line": "Axolotl Creep",
                            "Stage": 1,
                            "Path": "sadsad",
                            "Variation": "Meow",
                            "UnitType": "Pet",
                            "CombatAffinity": "Water",
                            "CombatClass": "Bulwark",
                            "Tier": 1,
                            "SizeUnits": 15,
                            "Stats": {
                                "MoveSpeedSubUnits": 2500,
                                "AttackRangeUnits": 24,
                                "OmegaRangeUnits": 42,
                                "AttackSpeed": 30,
                                "HitChancePercentage": 25,
                                "AttackPhysicalDamage": 5,
                                "AttackEnergyDamage": 6,
                                "AttackPureDamage": 7,
                                "AttackDodgeChancePercentage": 6,
                                "MaxHealth": 450000,
                                "HealthRegeneration": 1234,
                                "HealthGainEfficiencyPercentage": 35,
                                "PhysicalVampPercentage": 33,
                                "EnergyVampPercentage": 34,
                                "PureVampPercentage": 35,
                                "EnergyCost": 30000,
                                "StartingEnergy": 15000,
                                "EnergyRegeneration": 432,
                                "EnergyGainEfficiencyPercentage": 65,
                                "OnActivationEnergy": 333,
                                "EnergyPiercingPercentage": 15,
                                "PhysicalPiercingPercentage": 16,
                                "EnergyResist": 17,
                                "PhysicalResist": 18,
                                "TenacityPercentage": 1,
                                "WillpowerPercentage": 2,
                                "Grit": 3,
                                "Resolve": 4,
                                "VulnerabilityPercentage": 5,
                                "CritChancePercentage": 6,
                                "CritAmplificationPercentage": 7,
                                "OmegaPowerPercentage": 8,
                                "Thorns": 9,
                                "StartingShield": 30,
                                "CritReductionPercentage": 25
                            },
                            "AttackAbilitiesSelection": "Cycle",
                            "AttackAbilities": [
                                {
                                    "Name": "Attack",
                                    "Skills": [
                                        {
                                            "Name": "Attack",
                                            "Targeting": {
                                                "Type": "CurrentFocus",
                                                "Guidance": [
                                                    "Ground"
                                                ]
                                            },
                                            "Deployment": {
                                                "Type": "Projectile",
                                                "Guidance": [ "Ground" ],
                                                "PreDeploymentDelayPercentage": 25
                                            },
                                            "Projectile": {
                                                "SizeUnits": 5,
                                                "SpeedSubUnits": 567,
                                                "IsHoming": false,
                                                "IsBlockable": true,
                                                "ApplyToAll": true
                                            },
                                            "EffectPackage": {
                                                "Effects": [
                                                    {
                                                        "Type": "InstantDamage",
                                                        "DamageType": "Physical",
                                                        "Expression": 65
                                                    }
                                                ]
                                            }
                                        }
                                    ]
                                }
                            ]
                        }
                    }
                }
            ]
        }
    ]
}
)";

    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);
    ASSERT_EQ(data.radius_units, 5);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Get the components
    const auto& stats_component = entity->Get<StatsComponent>();

    // Test values
    EXPECT_EQ(stats_component.GetMaxHealth(), 950_fp);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kAttack));

    const auto& attack_abilities = abilities_component.GetDataAttackAbilities();
    ASSERT_EQ(attack_abilities.abilities.size(), 1);

    {
        const auto& attack_ability = attack_abilities.abilities.at(0);
        ASSERT_EQ(attack_ability->skills.size(), 1);

        const auto& skill = attack_ability->skills[0];
        EXPECT_FALSE(skill->effect_package.attributes.use_hit_chance);
    }

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));
    const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();
    ASSERT_EQ(omega_abilities.abilities.size(), 1);

    // Ability 3 with 1 spawn skill
    {
        const auto& omega_ability = omega_abilities.abilities.at(0);
        EXPECT_EQ(omega_ability->name, "Ability Name - Spawning");
        EXPECT_EQ(omega_ability->total_duration_ms, 2500);
        ASSERT_EQ(omega_ability->skills.size(), 1);

        // First skill
        {
            const auto& omega_skill = omega_ability->skills.at(0);
            EXPECT_EQ(omega_skill->name, "Ability Skill 1");
            EXPECT_EQ(omega_skill->percentage_of_ability_duration, 100);
            EXPECT_EQ(omega_skill->deployment.pre_deployment_delay_percentage, 21);

            EXPECT_EQ(omega_skill->targeting.type, SkillTargetingType::kCurrentFocus);
            EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kSpawnedCombatUnit);

            const auto spawned_combat_unit = omega_skill->spawn.combat_unit;
            ASSERT_NE(spawned_combat_unit, nullptr);

            // Spawn the child CombatUnit
            Entity* child_entity = nullptr;
            SpawnCombatUnit(Team::kBlue, {-40, -40}, *spawned_combat_unit, child_entity);
            ASSERT_NE(child_entity, nullptr);

            EXPECT_TRUE(EntityHelper::IsAPetCombatUnit(*child_entity));

            // Get the child components
            const auto& child_position_component = child_entity->Get<PositionComponent>();
            const auto& child_movement_component = child_entity->Get<MovementComponent>();
            const auto& child_stats_component = child_entity->Get<StatsComponent>();
            const auto& child_combat_synergy_component = child_entity->Get<CombatSynergyComponent>();
            const auto& child_abilities_component = child_entity->Get<AbilitiesComponent>();

            // Test child values
            // Ensure position correct
            EXPECT_EQ(child_position_component.GetQ(), -40);
            EXPECT_EQ(child_position_component.GetR(), -40);

            // Ensure movement attributes correct
            EXPECT_EQ(child_movement_component.GetMovementSpeedSubUnits(), 2500_fp);
            EXPECT_EQ(child_movement_component.GetMovementType(), MovementComponent::MovementType::kNavigation);

            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kAttackRangeUnits), 24_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kAttackSpeed), 30_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kHitChancePercentage), 25_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kAttackPhysicalDamage), 5_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kAttackEnergyDamage), 6_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kAttackPureDamage), 7_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kAttackDodgeChancePercentage), 6_fp);

            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kMaxHealth), 450000_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kHealthRegeneration), 1234_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kHealthGainEfficiencyPercentage), 35_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kPhysicalVampPercentage), 33_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kEnergyVampPercentage), 34_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kPureVampPercentage), 35_fp);

            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kEnergyCost), 30000_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kStartingEnergy), 15000_fp);

            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kEnergyRegeneration), 432_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kEnergyGainEfficiencyPercentage), 65_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kOnActivationEnergy), 333_fp);

            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kEnergyPiercingPercentage), 15_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kPhysicalPiercingPercentage), 16_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kEnergyResist), 17_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kPhysicalResist), 18_fp);

            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kTenacityPercentage), 1_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kWillpowerPercentage), 2_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kGrit), 3_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kResolve), 4_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kVulnerabilityPercentage), 5_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kCritChancePercentage), 6_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kCritAmplificationPercentage), 7_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kOmegaPowerPercentage), 8_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kCritReductionPercentage), 25_fp);
            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kThorns), 9_fp);

            EXPECT_EQ(child_stats_component.GetBaseValueForType(StatType::kStartingShield), 30_fp);

            // Ensure current values correct
            EXPECT_EQ(child_stats_component.GetCurrentEnergy(), 15000_fp);
            // EXPECT_EQ(child_stats_component.GetCurrentHealth(), 450000);

            // Check combat class, combat affinity and size
            EXPECT_EQ(child_position_component.GetRadius(), 15);
            EXPECT_TRUE(child_position_component.IsTakingSpace());
            EXPECT_EQ(child_combat_synergy_component.GetCombatClass(), CombatClass::kBulwark);
            EXPECT_EQ(child_combat_synergy_component.GetCombatAffinity(), CombatAffinity::kWater);

            // Confirm abilities loaded
            ASSERT_TRUE(child_abilities_component.HasAnyAbility(AbilityType::kAttack));
            const auto& child_attack_abilities = abilities_component.GetDataAttackAbilities();
            EXPECT_EQ(child_attack_abilities.abilities.size(), 1);

            ASSERT_FALSE(child_abilities_component.HasAnyAbility(AbilityType::kOmega));
        }
    }
}

TEST_F(JSONTest, LoadCombatUnitSplashSkill)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Super Axolotl",
    "Stage": 1,
    "UnitType": "Illuvial",
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "Tier": 1,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MaxHealth": 950,
        "StartingEnergy": 30,
        "EnergyCost": 60,
        "PhysicalResist": 60,
        "EnergyResist": 60,
        "TenacityPercentage": 0,
        "WillpowerPercentage": 0,
        "Grit": 0,
        "Resolve": 0,
        "AttackPhysicalDamage": 45,
        "AttackEnergyDamage": 0,
        "AttackPureDamage": 0,
        "AttackSpeed": 60,
        "MoveSpeedSubUnits": 5000,
        "HitChancePercentage": 100,
        "AttackDodgeChancePercentage": 0,
        "CritChancePercentage": 25,
        "CritAmplificationPercentage": 150,
        "OmegaPowerPercentage": 1,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 42,
        "HealthRegeneration": 0,
        "EnergyRegeneration": 0,
        "EnergyGainEfficiencyPercentage": 100,
        "OnActivationEnergy": 0,
        "VulnerabilityPercentage": 0,
        "EnergyPiercingPercentage": 0,
        "PhysicalPiercingPercentage": 0,
        "HealthGainEfficiencyPercentage": 100,
        "PhysicalVampPercentage": 0,
        "EnergyVampPercentage": 0,
        "PureVampPercentage": 0,
        "Thorns": 0,
        "StartingShield": 0,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack",
            "TotalDurationMs": 2345,
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 0
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Ability 1 Name - Splash",
            "TotalDurationMs": 0,
            "Skills": [
                {
                    "Name": "Ability 1 Skill 1",
                    "PercentageOfAbilityDuration": 100,
                    "Deployment": {
                        "Type": "Direct",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 21
                    },
                    "Targeting": {
                        "Type": "Synergy",
                        "Group": "Enemy",
                        "CombatClass": "Fighter",
                        "NotCombatAffinity": "Water"
                    },
                    "EffectPackage": {
                        "Attributes": {
                            "Propagation": {
                                "PropagationType": "Splash",
                                "IgnoreFirstPropagationReceiver": true,
                                "SplashRadiusUnits": 6542,
                                "EffectPackage": {
                                    "Effects": [
                                        {
                                            "Type": "InstantDamage",
                                            "DamageType": "Energy",
                                            "Expression": 750
                                        }
                                    ]
                                }
                            }
                        },
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 123
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";

    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Get the components
    const auto& stats_component = entity->Get<StatsComponent>();

    // Test values
    // Check max_health was initialized properly based on level
    // Should be the same as the base level
    EXPECT_EQ(stats_component.GetMaxHealth(), 950_fp);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));
    const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();

    ASSERT_EQ(omega_abilities.abilities.size(), 1);

    const auto& omega_ability = omega_abilities.abilities.at(0);
    EXPECT_EQ(omega_ability->name, "Ability 1 Name - Splash");
    EXPECT_EQ(omega_ability->total_duration_ms, 0);
    ASSERT_EQ(omega_ability->skills.size(), 1);

    // First skill
    {
        const auto& omega_skill = omega_ability->skills.at(0);
        EXPECT_EQ(omega_skill->name, "Ability 1 Skill 1");
        EXPECT_EQ(omega_skill->percentage_of_ability_duration, 100);
        EXPECT_EQ(omega_skill->deployment.pre_deployment_delay_percentage, 21);
        EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kDirect);
        ASSERT_EQ(omega_skill->deployment.guidance.Size(), 1);
        EXPECT_EQ(omega_skill->deployment.guidance.Contains(GuidanceType::kGround), true);
        EXPECT_EQ(omega_skill->targeting.type, SkillTargetingType::kSynergy);
        EXPECT_EQ(omega_skill->targeting.group, AllegianceType::kEnemy);
        EXPECT_EQ(omega_skill->targeting.combat_synergy, CombatClass::kFighter);
        EXPECT_EQ(omega_skill->targeting.not_combat_synergy, CombatAffinity::kWater);

        // Check effect package attributes
        {
            const EffectPackagePropagationAttributes& propagation = omega_skill->effect_package.attributes.propagation;
            EXPECT_EQ(propagation.type, EffectPropagationType::kSplash);
            EXPECT_EQ(propagation.ignore_first_propagation_receiver, true);
            EXPECT_TRUE(propagation.effect_package);
            EXPECT_EQ(propagation.effect_package->effects.size(), 1);
            EXPECT_EQ(propagation.splash_radius_units, 6542);
            const EffectData& effect_data = propagation.effect_package->effects.front();
            EXPECT_EQ(effect_data.type_id.type, EffectType::kInstantDamage);
            EXPECT_EQ(effect_data.type_id.damage_type, EffectDamageType::kEnergy);
            EXPECT_EQ(EvaluateNoStats(effect_data.GetExpression()), 750_fp);
        }

        // Check omega ability effects
        ASSERT_EQ(omega_skill->effect_package.effects.size(), 1);

        // First effect
        {
            const auto& omega_effect = omega_skill->effect_package.effects.at(0);

            EXPECT_EQ(omega_effect.type_id.type, EffectType::kInstantDamage);
            EXPECT_EQ(omega_effect.type_id.damage_type, EffectDamageType::kPhysical);
            EXPECT_EQ(omega_effect.GetExpression().base_value.value, 123_fp);
        }
    }
}

TEST_F(JSONTest, LoadCombatUnitChainSkill)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Super Axolotl",
    "Stage": 1,
    "UnitType": "Illuvial",
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "Tier": 1,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MaxHealth": 950,
        "StartingEnergy": 30,
        "EnergyCost": 60,
        "PhysicalResist": 60,
        "EnergyResist": 60,
        "TenacityPercentage": 0,
        "WillpowerPercentage": 0,
        "Grit": 0,
        "Resolve": 0,
        "AttackPhysicalDamage": 45,
        "AttackEnergyDamage": 0,
        "AttackPureDamage": 0,
        "AttackSpeed": 60,
        "MoveSpeedSubUnits": 5000,
        "HitChancePercentage": 100,
        "AttackDodgeChancePercentage": 0,
        "CritChancePercentage": 25,
        "CritAmplificationPercentage": 150,
        "OmegaPowerPercentage": 1,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 42,
        "HealthRegeneration": 0,
        "EnergyRegeneration": 0,
        "EnergyGainEfficiencyPercentage": 100,
        "OnActivationEnergy": 0,
        "VulnerabilityPercentage": 0,
        "EnergyPiercingPercentage": 0,
        "PhysicalPiercingPercentage": 0,
        "HealthGainEfficiencyPercentage": 100,
        "PhysicalVampPercentage": 0,
        "EnergyVampPercentage": 0,
        "PureVampPercentage": 0,
        "Thorns": 0,
        "StartingShield": 0,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack",
            "TotalDurationMs": 2345,
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 123
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Ability Name - Chains",
            "TotalDurationMs": 2500,
            "Skills": [
                {
                    "Name": "Ability 3 Skill 1",
                    "PercentageOfAbilityDuration": 100,
                    "Deployment": {
                        "Type": "Direct",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 21
                    },
                    "Targeting": {
                        "Type": "Synergy",
                        "Group": "Ally",
                        "CombatAffinity": "Steam"
                    },
                    "EffectPackage": {
                        "Attributes": {
                            "Propagation": {
                                "PropagationType": "Chain",
                                "TargetingGroup": "Ally",
                                "ChainNumber": 2,
                                "ChainDelayMs": 500,
                                "ChainBounceMaxDistanceUnits": 5,
                                "AddOriginalEffectPackage": true,
                                "OnlyNewTargets": true,
                                "PrioritizeNewTargets": true,
                                "IgnoreFirstPropagationReceiver": true,
                                "PreDeploymentDelayPercentage": 45,
                                "TargetingGuidance": [ "Ground", "Airborne" ],
                                "DeploymentGuidance": [ "Ground", "Airborne" ],
                                "EffectPackage": {
                                    "Effects": [
                                        {
                                            "Type": "InstantDamage",
                                            "DamageType": "Energy",
                                            "Expression": 750
                                        }
                                    ]
                                }
                            }
                        },
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 123
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";
    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Get the components
    const auto& stats_component = entity->Get<StatsComponent>();

    // Test values
    EXPECT_EQ(stats_component.GetMaxHealth(), 950_fp);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));
    const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();

    ASSERT_EQ(omega_abilities.abilities.size(), 1);

    const auto& omega_ability = omega_abilities.abilities.at(0);
    EXPECT_EQ(omega_ability->name, "Ability Name - Chains");
    EXPECT_EQ(omega_ability->total_duration_ms, 2500);
    ASSERT_EQ(omega_ability->skills.size(), 1);

    // First skill
    {
        const auto& omega_skill = omega_ability->skills.at(0);
        EXPECT_EQ(omega_skill->name, "Ability 3 Skill 1");
        EXPECT_EQ(omega_skill->percentage_of_ability_duration, 100);
        EXPECT_EQ(omega_skill->deployment.pre_deployment_delay_percentage, 21);
        EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kDirect);
        ASSERT_EQ(omega_skill->deployment.guidance.Size(), 1);
        EXPECT_EQ(omega_skill->deployment.guidance.Contains(GuidanceType::kGround), true);
        EXPECT_EQ(omega_skill->targeting.type, SkillTargetingType::kSynergy);
        EXPECT_EQ(omega_skill->targeting.group, AllegianceType::kAlly);
        EXPECT_EQ(omega_skill->targeting.combat_synergy, CombatAffinity::kSteam);
        EXPECT_TRUE(omega_skill->targeting.not_combat_synergy.IsEmpty());

        {
            const EffectPackagePropagationAttributes& propagation = omega_skill->effect_package.attributes.propagation;
            EXPECT_EQ(propagation.type, EffectPropagationType::kChain);
            EXPECT_EQ(propagation.targeting_group, AllegianceType::kAlly);
            EXPECT_EQ(propagation.chain_number, 2);
            EXPECT_EQ(propagation.chain_delay_ms, 500);
            EXPECT_EQ(propagation.chain_bounce_max_distance_units, 5);
            EXPECT_EQ(propagation.add_original_effect_package, true);
            EXPECT_EQ(propagation.only_new_targets, true);
            EXPECT_TRUE(propagation.ignore_first_propagation_receiver);
            EXPECT_EQ(propagation.pre_deployment_delay_percentage, 45);
            ASSERT_EQ(propagation.deployment_guidance.Size(), 2);
            EXPECT_EQ(propagation.deployment_guidance.Contains(GuidanceType::kGround), true);
            EXPECT_EQ(propagation.deployment_guidance.Contains(GuidanceType::kAirborne), true);
            EXPECT_EQ(propagation.deployment_guidance.Contains(GuidanceType::kUnderground), false);
            ASSERT_EQ(propagation.targeting_guidance.Size(), 2);
            EXPECT_EQ(propagation.targeting_guidance.Contains(GuidanceType::kGround), true);
            EXPECT_EQ(propagation.targeting_guidance.Contains(GuidanceType::kAirborne), true);
            EXPECT_EQ(propagation.targeting_guidance.Contains(GuidanceType::kUnderground), false);

            ASSERT_TRUE(propagation.effect_package != nullptr);
            const EffectData& effect_data = propagation.effect_package->effects.front();
            EXPECT_EQ(effect_data.type_id.type, EffectType::kInstantDamage);
            EXPECT_EQ(effect_data.type_id.damage_type, EffectDamageType::kEnergy);
            EXPECT_EQ(EvaluateNoStats(effect_data.GetExpression()), 750_fp);
        }

        // Check omega ability effects
        ASSERT_EQ(omega_skill->effect_package.effects.size(), 1);

        // First effect
        {
            const auto& omega_effect = omega_skill->effect_package.effects.at(0);

            EXPECT_EQ(omega_effect.type_id.type, EffectType::kInstantDamage);
            EXPECT_EQ(omega_effect.type_id.damage_type, EffectDamageType::kPhysical);
            EXPECT_EQ(omega_effect.GetExpression().base_value.value, 123_fp);
        }
    }
}

TEST_F(JSONTest, LoadCombatUnitBeamSkill)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Super Axolotl",
    "Stage": 1,
    "UnitType": "Illuvial",
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "Tier": 1,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MaxHealth": 950,
        "StartingEnergy": 30,
        "EnergyCost": 60,
        "PhysicalResist": 60,
        "EnergyResist": 60,
        "TenacityPercentage": 0,
        "WillpowerPercentage": 0,
        "Grit": 0,
        "Resolve": 0,
        "AttackPhysicalDamage": 45,
        "AttackEnergyDamage": 0,
        "AttackPureDamage": 0,
        "AttackSpeed": 60,
        "MoveSpeedSubUnits": 5000,
        "HitChancePercentage": 100,
        "AttackDodgeChancePercentage": 0,
        "CritChancePercentage": 25,
        "CritAmplificationPercentage": 150,
        "OmegaPowerPercentage": 1,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 42,
        "HealthRegeneration": 0,
        "EnergyRegeneration": 0,
        "EnergyGainEfficiencyPercentage": 100,
        "OnActivationEnergy": 0,
        "VulnerabilityPercentage": 0,
        "EnergyPiercingPercentage": 0,
        "PhysicalPiercingPercentage": 0,
        "HealthGainEfficiencyPercentage": 100,
        "PhysicalVampPercentage": 0,
        "EnergyVampPercentage": 0,
        "PureVampPercentage": 0,
        "Thorns": 0,
        "StartingShield": 0,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack",
            "TotalDurationMs": 1000,
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Direct",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 25
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 0
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Pressure Shield I",
            "TotalDurationMs": 1200,
            "Skills": [
                {
                    "Name": "Beam",
                    "Targeting": {
                        "Type": "Self",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Beam",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 0
                    },
                    "Beam": {
                        "WidthUnits": 10,
                        "FrequencyMs": 500,
                        "ApplyOnce": true,
                        "IsHoming": true,
                        "IsBlockable": true,
                        "BlockAllegiance": "Enemy"
                    },
                    "PercentageOfAbilityDuration": 100,
                    "ChannelTimeMs": 3000,
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 10
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";

    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Get the components
    const auto& stats_component = entity->Get<StatsComponent>();

    // Test values
    EXPECT_EQ(stats_component.GetMaxHealth(), 950_fp);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));
    const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();

    // Check omega ability
    ASSERT_EQ(omega_abilities.abilities.size(), 1);
    auto omega_ability = omega_abilities.abilities.front();

    // Check omega ability skill
    ASSERT_EQ(omega_ability->skills.size(), 1);
    const auto& omega_skill = omega_ability->skills.front();
    EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kBeam);
    EXPECT_EQ(omega_skill->channel_time_ms, 3000);
    EXPECT_EQ(omega_skill->beam.width_units, 10);
    EXPECT_EQ(omega_skill->beam.frequency_ms, 500);
    EXPECT_EQ(omega_skill->beam.is_blockable, true);
    EXPECT_EQ(omega_skill->beam.is_homing, true);
    EXPECT_EQ(omega_skill->beam.apply_once, true);
    EXPECT_EQ(omega_skill->beam.block_allegiance, AllegianceType::kEnemy);

    // Check omega ability effects
    ASSERT_EQ(omega_skill->effect_package.effects.size(), 1);
    const auto& omega_effect = omega_skill->effect_package.effects.front();
    ASSERT_EQ(omega_effect.type_id.type, EffectType::kInstantDamage);
    EXPECT_EQ(omega_effect.type_id.damage_type, EffectDamageType::kPhysical);
    EXPECT_EQ(omega_effect.GetExpression().base_value.value, 10_fp);
}

TEST_F(JSONTest, LoadCombatUnitDashSkill)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Super Axolotl",
    "Stage": 1,
    "UnitType": "Illuvial",
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "Tier": 1,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MaxHealth": 950,
        "StartingEnergy": 30,
        "EnergyCost": 60,
        "PhysicalResist": 60,
        "EnergyResist": 60,
        "TenacityPercentage": 0,
        "WillpowerPercentage": 0,
        "Grit": 0,
        "Resolve": 0,
        "AttackPhysicalDamage": 45,
        "AttackEnergyDamage": 0,
        "AttackPureDamage": 0,
        "AttackSpeed": 60,
        "MoveSpeedSubUnits": 5000,
        "HitChancePercentage": 100,
        "AttackDodgeChancePercentage": 0,
        "CritChancePercentage": 25,
        "CritAmplificationPercentage": 150,
        "OmegaPowerPercentage": 1,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 42,
        "HealthRegeneration": 0,
        "EnergyRegeneration": 0,
        "EnergyGainEfficiencyPercentage": 100,
        "OnActivationEnergy": 0,
        "VulnerabilityPercentage": 0,
        "EnergyPiercingPercentage": 0,
        "PhysicalPiercingPercentage": 0,
        "HealthGainEfficiencyPercentage": 100,
        "PhysicalVampPercentage": 0,
        "EnergyVampPercentage": 0,
        "PureVampPercentage": 0,
        "Thorns": 0,
        "StartingShield": 0,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack",
            "TotalDurationMs": 1000,
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Direct",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 25
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 0
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Pressure Shield I",
            "TotalDurationMs": 1200,
            "Skills": [
                {
                    "Name": "Dash",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Dash",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 0
                    },
                    "Dash": {
                        "ApplyToAll": true
                    },
                    "PercentageOfAbilityDuration": 100,
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 10
                            }
                        ]
                    }
                },
                {
                    "Name": "Dash",
                    "Targeting": {
                        "Type": "CurrentFocus"
                    },
                    "PercentageOfAbilityDuration": 100,
                    "Deployment": {
                        "Type": "Dash",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 0
                    },
                    "Dash": {
                        "SpeedSubUnits": 2345,
                        "ApplyToAll": true
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Pure",
                                "Expression": 20
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";

    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Get the components
    const auto& stats_component = entity->Get<StatsComponent>();

    // Test values
    EXPECT_EQ(stats_component.GetMaxHealth(), 950_fp);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));
    const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();

    // Check omega ability
    ASSERT_EQ(omega_abilities.abilities.size(), 1);
    const auto& omega_ability = omega_abilities.abilities.front();

    // Check omega ability skill
    ASSERT_EQ(omega_ability->skills.size(), 2);
    {
        const auto& omega_skill = omega_ability->skills.at(0);
        EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kDash);
        EXPECT_EQ(omega_skill->targeting.type, SkillTargetingType::kCurrentFocus);
        EXPECT_EQ(omega_skill->dash.apply_to_all, true);

        // Check omega ability effects
        ASSERT_EQ(omega_skill->effect_package.effects.size(), 1);
        const auto& omega_effect = omega_skill->effect_package.effects.front();
        ASSERT_EQ(omega_effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(omega_effect.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(omega_effect.GetExpression().base_value.value, 10_fp);
    }
    {
        const auto& omega_skill = omega_ability->skills.at(1);
        EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kDash);
        EXPECT_EQ(omega_skill->targeting.type, SkillTargetingType::kCurrentFocus);
        EXPECT_EQ(omega_skill->dash.apply_to_all, true);

        // Check omega ability effects
        ASSERT_EQ(omega_skill->effect_package.effects.size(), 1);
        const auto& omega_effect = omega_skill->effect_package.effects.front();
        ASSERT_EQ(omega_effect.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(omega_effect.type_id.damage_type, EffectDamageType::kPure);
        EXPECT_EQ(omega_effect.GetExpression().base_value.value, 20_fp);
    }
}

TEST_F(JSONTest, LoadCombatUnitBlinkSkill)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Super Axolotl",
    "Stage": 1,
    "UnitType": "Illuvial",
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "Tier": 1,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MaxHealth": 950,
        "StartingEnergy": 30,
        "EnergyCost": 60,
        "PhysicalResist": 60,
        "EnergyResist": 60,
        "TenacityPercentage": 0,
        "WillpowerPercentage": 0,
        "Grit": 0,
        "Resolve": 0,
        "AttackPhysicalDamage": 45,
        "AttackEnergyDamage": 0,
        "AttackPureDamage": 0,
        "AttackSpeed": 60,
        "MoveSpeedSubUnits": 5000,
        "HitChancePercentage": 100,
        "AttackDodgeChancePercentage": 0,
        "CritChancePercentage": 25,
        "CritAmplificationPercentage": 150,
        "OmegaPowerPercentage": 1,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 0,
        "HealthRegeneration": 0,
        "EnergyRegeneration": 0,
        "EnergyGainEfficiencyPercentage": 100,
        "OnActivationEnergy": 0,
        "VulnerabilityPercentage": 0,
        "EnergyPiercingPercentage": 0,
        "PhysicalPiercingPercentage": 0,
        "HealthGainEfficiencyPercentage": 100,
        "PhysicalVampPercentage": 0,
        "EnergyVampPercentage": 0,
        "PureVampPercentage": 0,
        "Thorns": 0,
        "StartingShield": 0,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack",
            "TotalDurationMs": 1000,
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Direct",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 25
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 0
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Pressure Shield I",
            "TotalDurationMs": 1200,
            "Skills": [
                {
                    "Name": "Blink current focus",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Direct",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 0
                    },
                    "PercentageOfAbilityDuration": 100,
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "Blink",
                                "BlinkTarget": "BehindReceiver",
                                "BlinkDelayMs": 150,
                                "DurationMs": 300
                            },
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 10
                            }
                        ]
                    }
                },
                {
                    "Name": "Blink lowest health",
                    "PercentageOfAbilityDuration": 100,
                    "Deployment": {
                        "Type": "Direct",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 0
                    },
                    "Targeting": {
                        "Type": "CombatStatCheck",
                        "Group": "Enemy",
                        "Lowest": true,
                        "Num": 1,
                        "Stat": "CurrentHealth"
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "Blink",
                                "BlinkTarget": "NearReceiver",
                                "BlinkDelayMs": 150,
                                "DurationMs": 300
                            },
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 10
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";

    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Get the components
    const auto& stats_component = entity->Get<StatsComponent>();

    // Test values
    // Check max_health was initialized properly based on level
    EXPECT_EQ(stats_component.GetMaxHealth(), 950_fp);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));
    const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();

    // Check omega ability
    ASSERT_EQ(omega_abilities.abilities.size(), 1);
    const auto& omega_ability = omega_abilities.abilities.front();

    // Check omega ability skill
    ASSERT_EQ(omega_ability->skills.size(), 2);
    {
        const auto& omega_skill = omega_ability->skills.at(0);
        EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kDirect);
        EXPECT_EQ(omega_skill->targeting.type, SkillTargetingType::kCurrentFocus);

        // Check omega ability effects
        ASSERT_EQ(omega_skill->effect_package.effects.size(), 2);
        const auto& omega_effect1 = omega_skill->effect_package.effects[0];
        ASSERT_EQ(omega_effect1.type_id.type, EffectType::kBlink);
        EXPECT_EQ(omega_effect1.blink_target, ReservedPositionType::kBehindReceiver);
        EXPECT_EQ(omega_effect1.blink_delay_ms, 150);
        EXPECT_EQ(omega_effect1.lifetime.duration_time_ms, 300);
        const auto& omega_effect2 = omega_skill->effect_package.effects[1];
        ASSERT_EQ(omega_effect2.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(omega_effect2.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(omega_effect2.GetExpression().base_value.value, 10_fp);
    }
    {
        const auto& omega_skill = omega_ability->skills.at(1);
        EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kDirect);
        EXPECT_EQ(omega_skill->targeting.type, SkillTargetingType::kCombatStatCheck);
        EXPECT_EQ(omega_skill->targeting.group, AllegianceType::kEnemy);
        EXPECT_EQ(omega_skill->targeting.num, 1);
        EXPECT_EQ(omega_skill->targeting.lowest, true);
        EXPECT_EQ(omega_skill->targeting.stat_type, StatType::kCurrentHealth);

        // Check omega ability effects
        ASSERT_EQ(omega_skill->effect_package.effects.size(), 2);
        const auto& omega_effect1 = omega_skill->effect_package.effects[0];
        ASSERT_EQ(omega_effect1.type_id.type, EffectType::kBlink);
        EXPECT_EQ(omega_effect1.blink_target, ReservedPositionType::kNearReceiver);
        EXPECT_EQ(omega_effect1.blink_delay_ms, 150);
        EXPECT_EQ(omega_effect1.lifetime.duration_time_ms, 300);
        const auto& omega_effect2 = omega_skill->effect_package.effects[1];
        ASSERT_EQ(omega_effect2.type_id.type, EffectType::kInstantDamage);
        EXPECT_EQ(omega_effect2.type_id.damage_type, EffectDamageType::kPhysical);
        EXPECT_EQ(omega_effect2.GetExpression().base_value.value, 10_fp);
    }
}

TEST_F(JSONTest, LoadCombatUnitWithEmpowers)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Super Axolotl",
    "Stage": 1,
    "UnitType": "Illuvial",
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "Tier": 1,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MaxHealth": 950,
        "StartingEnergy": 30,
        "EnergyCost": 60,
        "PhysicalResist": 60,
        "EnergyResist": 60,
        "TenacityPercentage": 0,
        "WillpowerPercentage": 0,
        "Grit": 0,
        "Resolve": 0,
        "AttackPhysicalDamage": 45,
        "AttackEnergyDamage": 0,
        "AttackPureDamage": 0,
        "AttackSpeed": 60,
        "MoveSpeedSubUnits": 5000,
        "HitChancePercentage": 100,
        "AttackDodgeChancePercentage": 0,
        "CritChancePercentage": 25,
        "CritAmplificationPercentage": 150,
        "OmegaPowerPercentage": 1,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 0,
        "HealthRegeneration": 0,
        "EnergyRegeneration": 0,
        "EnergyGainEfficiencyPercentage": 100,
        "OnActivationEnergy": 0,
        "VulnerabilityPercentage": 0,
        "EnergyPiercingPercentage": 0,
        "PhysicalPiercingPercentage": 0,
        "HealthGainEfficiencyPercentage": 100,
        "PhysicalVampPercentage": 0,
        "EnergyVampPercentage": 0,
        "PureVampPercentage": 0,
        "Thorns": 0,
        "StartingShield": 0,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack",
            "TotalDurationMs": 2345,
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 0
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Ability Name - Zones",
            "TotalDurationMs": 5000,
            "Skills": [
                {
                    "Name": "Ability Skill 1",
                    "PercentageOfAbilityDuration": 100,
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Direct",
                        "PreDeploymentDelayPercentage": 40
                    },
                    "EffectPackage": {
                        "Attributes": {
                            "VampiricPercentage": 1,
                            "ExcessVampToShieldDurationMs": 5,
                            "PiercingPercentage": 2,
                            "ExcessVampToShield": true,
                            "ExploitWeakness": true
                        },
                        "Effects": [
                            {
                                "Type": "Empower",
                                "ActivatedBy": "Omega",
                                "DurationMs": 1600,
                                "AttachedEffects": [
                                    {
                                        "Type": "InstantDamage",
                                        "DamageType": "Pure",
                                        "Expression": 45
                                    },
                                    {
                                        "Type": "InstantDamage",
                                        "DamageType": "Physical",
                                        "Expression": 234
                                    },
                                    {
                                        "Type": "PositiveState",
                                        "PositiveState": "Untargetable",
                                        "DurationMs": 6500
                                    }
                                ],
                                "AttachedEffectPackageAttributes": {
                                    "IsTrueshot": true,
                                    "CanCrit": true,
                                    "AlwaysCrit": true,
                                    "RotateToTarget": true,
                                    "CumulativeDamage": true,
                                    "RefundHealth": 120,
                                    "RefundEnergy": 130
                                }
                            },
                            {
                                "Type": "Empower",
                                "ActivatedBy": "Omega",
                                "IsConsumable": true,
                                "ActivationsUntilExpiry": 3,

                                "IsUsedAsGlobalEffectAttribute": true,
                                "ForEffectTypeID": {
                                    "Type": "InstantDamage",
                                    "DamageType": "Energy"
                                },

                                "ExcessHealToShield": 3,
                                "MissingHealthPercentageToHealth": 4,
                                "ToShieldPercentage": 6,
                                "ToShieldDurationMs": 7,
                                "MaxHealthPercentageToHealth": 8,

                                "CleanseNegativeStates": true,
                                "CleanseConditions": true,
                                "CleanseDOTs": true,
                                "CleanseBOTs": true,
                                "CleanseDebuffs": true
                            },
                            {
                                "Type": "Empower",
                                "ActivatedBy": "Omega",
                                "IsConsumable": true,
                                "ActivationsUntilExpiry": 4,

                                "AttachedEffectPackageAttributes": {
                                    "IsTrueshot": true,
                                    "CanCrit": true,
                                    "AlwaysCrit": true,
                                    "RotateToTarget": true,
                                    "CumulativeDamage": true,
                                    "RefundHealth": 120,
                                    "RefundEnergy": 130
                                }
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";
    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Get the components
    const auto& stats_component = entity->Get<StatsComponent>();

    // Test values
    EXPECT_EQ(stats_component.GetMaxHealth(), 950_fp);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));
    const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();

    ASSERT_EQ(omega_abilities.abilities.size(), 1);

    // Ability with 3 zone skills
    {
        const auto& omega_ability = omega_abilities.abilities.at(0);
        EXPECT_EQ(omega_ability->name, "Ability Name - Zones");
        EXPECT_EQ(omega_ability->total_duration_ms, 5000);
        ASSERT_EQ(omega_ability->skills.size(), 1);

        // First skill
        const auto& omega_skill = omega_ability->skills.at(0);
        EXPECT_EQ(omega_skill->name, "Ability Skill 1");
        EXPECT_EQ(omega_skill->percentage_of_ability_duration, 100);
        EXPECT_EQ(omega_skill->deployment.pre_deployment_delay_percentage, 40);

        EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kDirect);

        EXPECT_EQ(omega_skill->effect_package.attributes.excess_vamp_to_shield, true);
        EXPECT_EQ(omega_skill->effect_package.attributes.exploit_weakness, true);
        EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.vampiric_percentage), 1_fp);
        EXPECT_EQ(omega_skill->effect_package.attributes.excess_vamp_to_shield_duration_ms, 5);
        EXPECT_EQ(EvaluateNoStats(omega_skill->effect_package.attributes.piercing_percentage), 2_fp);

        // Check omega ability effects
        ASSERT_EQ(omega_skill->effect_package.effects.size(), 3);

        // First effect
        {
            const auto& omega_effect = omega_skill->effect_package.effects.at(0);
            EXPECT_EQ(omega_effect.type_id.type, EffectType::kEmpower);
            EXPECT_EQ(omega_effect.lifetime.activated_by, AbilityType::kOmega);
            EXPECT_EQ(omega_effect.lifetime.duration_time_ms, 1600);

            // Empower has 3 effects
            ASSERT_EQ(omega_effect.attached_effects.size(), 3);
            {
                const auto& empower_effect = omega_effect.attached_effects.at(0);
                EXPECT_EQ(empower_effect.type_id.type, EffectType::kInstantDamage);
                EXPECT_EQ(empower_effect.type_id.damage_type, EffectDamageType::kPure);
                EXPECT_EQ(empower_effect.GetExpression().base_value.value, 45_fp);
            }
            {
                const auto& empower_effect = omega_effect.attached_effects.at(1);
                EXPECT_EQ(empower_effect.type_id.type, EffectType::kInstantDamage);
                EXPECT_EQ(empower_effect.type_id.damage_type, EffectDamageType::kPhysical);
                EXPECT_EQ(empower_effect.GetExpression().base_value.value, 234_fp);
            }
            {
                const auto& empower_effect = omega_effect.attached_effects.at(2);
                EXPECT_EQ(empower_effect.type_id.type, EffectType::kPositiveState);
                EXPECT_EQ(empower_effect.type_id.positive_state, EffectPositiveState::kUntargetable);
                EXPECT_EQ(empower_effect.lifetime.duration_time_ms, 6500);
            }

            EXPECT_TRUE(omega_effect.attached_effect_package_attributes.always_crit);
            EXPECT_TRUE(omega_effect.attached_effect_package_attributes.is_trueshot);
            EXPECT_TRUE(omega_effect.attached_effect_package_attributes.can_crit);
            EXPECT_TRUE(omega_effect.attached_effect_package_attributes.rotate_to_target);
            EXPECT_TRUE(omega_effect.attached_effect_package_attributes.cumulative_damage);
            EXPECT_EQ(EvaluateNoStats(omega_effect.attached_effect_package_attributes.refund_health), 120_fp);
            EXPECT_EQ(EvaluateNoStats(omega_effect.attached_effect_package_attributes.refund_energy), 130_fp);
        }

        // Second effects, empower global effect attribute
        {
            const auto& omega_effect = omega_skill->effect_package.effects.at(1);
            EXPECT_EQ(omega_effect.type_id.type, EffectType::kEmpower);
            EXPECT_EQ(omega_effect.lifetime.activated_by, AbilityType::kOmega);
            EXPECT_EQ(omega_effect.lifetime.is_consumable, true);
            EXPECT_EQ(omega_effect.lifetime.activations_until_expiry, 3);
            ASSERT_EQ(omega_effect.attached_effects.size(), 0);

            EXPECT_EQ(omega_effect.is_used_as_global_effect_attribute, true);

            EXPECT_EQ(omega_effect.empower_for_effect_type_id.type, EffectType::kInstantDamage);
            EXPECT_EQ(omega_effect.empower_for_effect_type_id.damage_type, EffectDamageType::kEnergy);

            EXPECT_EQ(omega_effect.attributes.excess_heal_to_shield, 3_fp);
            EXPECT_EQ(omega_effect.attributes.missing_health_percentage_to_health, 4_fp);
            EXPECT_EQ(omega_effect.attributes.to_shield_percentage, 6_fp);
            EXPECT_EQ(omega_effect.attributes.to_shield_duration_ms, 7);
            EXPECT_EQ(omega_effect.attributes.max_health_percentage_to_health, 8_fp);
            EXPECT_EQ(omega_effect.attributes.cleanse_negative_states, true);
            EXPECT_EQ(omega_effect.attributes.cleanse_conditions, true);
            EXPECT_EQ(omega_effect.attributes.cleanse_dots, true);
            EXPECT_EQ(omega_effect.attributes.cleanse_bots, true);
            EXPECT_EQ(omega_effect.attributes.cleanse_debuffs, true);
        }

        // Third effects
        {
            const auto& omega_effect = omega_skill->effect_package.effects.at(2);
            EXPECT_EQ(omega_effect.type_id.type, EffectType::kEmpower);
            EXPECT_EQ(omega_effect.lifetime.activated_by, AbilityType::kOmega);
            EXPECT_EQ(omega_effect.lifetime.is_consumable, true);
            EXPECT_EQ(omega_effect.lifetime.activations_until_expiry, 4);
            ASSERT_EQ(omega_effect.attached_effects.size(), 0);

            EXPECT_TRUE(omega_effect.attached_effect_package_attributes.always_crit);
            EXPECT_TRUE(omega_effect.attached_effect_package_attributes.is_trueshot);
            EXPECT_TRUE(omega_effect.attached_effect_package_attributes.can_crit);
            EXPECT_TRUE(omega_effect.attached_effect_package_attributes.rotate_to_target);
            EXPECT_TRUE(omega_effect.attached_effect_package_attributes.cumulative_damage);
            EXPECT_EQ(EvaluateNoStats(omega_effect.attached_effect_package_attributes.refund_health), 120_fp);
            EXPECT_EQ(EvaluateNoStats(omega_effect.attached_effect_package_attributes.refund_energy), 130_fp);
        }
    }
}

TEST_F(JSONTest, LoadCombatUnitWithDisempowers)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Super Axolotl",
    "Stage": 1,
    "UnitType": "Illuvial",
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "Tier": 1,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MaxHealth": 950,
        "StartingEnergy": 30,
        "EnergyCost": 60,
        "PhysicalResist": 60,
        "EnergyResist": 60,
        "TenacityPercentage": 0,
        "WillpowerPercentage": 0,
        "Grit": 0,
        "Resolve": 0,
        "AttackPhysicalDamage": 45,
        "AttackEnergyDamage": 0,
        "AttackPureDamage": 0,
        "AttackSpeed": 60,
        "MoveSpeedSubUnits": 5000,
        "HitChancePercentage": 100,
        "AttackDodgeChancePercentage": 0,
        "CritChancePercentage": 25,
        "CritAmplificationPercentage": 150,
        "OmegaPowerPercentage": 1,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 0,
        "HealthRegeneration": 0,
        "EnergyRegeneration": 0,
        "EnergyGainEfficiencyPercentage": 100,
        "OnActivationEnergy": 0,
        "VulnerabilityPercentage": 0,
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
    "AttackAbilities": [
        {
            "Name": "Attack",
            "TotalDurationMs": 2345,
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": {
                        "Type": "CurrentFocus"
                    },
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "EffectPackage": {
                    "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 0
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Ability Name - Zones",
            "TotalDurationMs": 5000,
            "Skills": [
                {
                    "Name": "Ability Skill 1",
                    "PercentageOfAbilityDuration": 100,
                    "Targeting": {
                        "Type": "CurrentFocus"
                    },
                    "Deployment": {
                        "Type": "Direct",
                        "PreDeploymentDelayPercentage": 40
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "Disempower",
                                "ActivatedBy": "Omega",
                                "IsConsumable": true,
                                "ActivationsUntilExpiry": 4,

                                "AttachedEffectPackageAttributes": {
                                    "RefundHealth": 120,
                                    "RefundEnergy": 130
                                }
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";
    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Get the components
    const auto& stats_component = entity->Get<StatsComponent>();

    // Test values
    EXPECT_EQ(stats_component.GetMaxHealth(), 950_fp);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));
    const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();

    ASSERT_EQ(omega_abilities.abilities.size(), 1);

    // Ability with 3 zone skills
    {
        const auto& omega_ability = omega_abilities.abilities.at(0);
        EXPECT_EQ(omega_ability->name, "Ability Name - Zones");
        EXPECT_EQ(omega_ability->total_duration_ms, 5000);
        ASSERT_EQ(omega_ability->skills.size(), 1);

        // First skill
        const auto& omega_skill = omega_ability->skills.at(0);
        EXPECT_EQ(omega_skill->name, "Ability Skill 1");
        EXPECT_EQ(omega_skill->percentage_of_ability_duration, 100);
        EXPECT_EQ(omega_skill->deployment.pre_deployment_delay_percentage, 40);

        EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kDirect);

        // Check omega ability effects
        ASSERT_EQ(omega_skill->effect_package.effects.size(), 1);

        // First effect
        {
            const auto& omega_effect = omega_skill->effect_package.effects.at(0);
            EXPECT_EQ(omega_effect.type_id.type, EffectType::kDisempower);
            EXPECT_EQ(omega_effect.lifetime.activated_by, AbilityType::kOmega);
            EXPECT_EQ(omega_effect.lifetime.is_consumable, true);
            EXPECT_EQ(omega_effect.lifetime.activations_until_expiry, 4);
            ASSERT_EQ(omega_effect.attached_effects.size(), 0);

            EXPECT_EQ(EvaluateNoStats(omega_effect.attached_effect_package_attributes.refund_health), 120_fp);
            EXPECT_EQ(EvaluateNoStats(omega_effect.attached_effect_package_attributes.refund_energy), 130_fp);
        }
    }
}

TEST_F(JSONTest, MarkEffect)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Super Axolotl",
    "Stage": 1,
    "UnitType": "Illuvial",
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "Tier": 1,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MoveSpeedSubUnits": 5000,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 40,
        "AttackSpeed": 60,
        "HitChancePercentage": 55,
        "AttackPhysicalDamage": 11,
        "AttackEnergyDamage": 12,
        "AttackPureDamage": 13,
        "AttackDodgeChancePercentage": 14,
        "MaxHealth": 950000,
        "HealthGainEfficiencyPercentage": 75,
        "PhysicalVampPercentage": 66,
        "EnergyVampPercentage": 67,
        "PureVampPercentage": 68,
        "EnergyCost": 60000,
        "StartingEnergy": 30000,
        "EnergyRegeneration": 765,
        "EnergyGainEfficiencyPercentage": 129,
        "OnActivationEnergy": 666,
        "EnergyPiercingPercentage": 31,
        "PhysicalPiercingPercentage": 32,
        "EnergyResist": 33,
        "PhysicalResist": 34,
        "TenacityPercentage": 1,
        "WillpowerPercentage": 2,
        "Grit": 3,
        "Resolve": 4,
        "VulnerabilityPercentage": 5,
        "CritChancePercentage": 6,
        "CritAmplificationPercentage": 7,
        "OmegaPowerPercentage": 8,
        "Thorns": 9,
        "HealthRegeneration": 12345,
        "StartingShield": 20,
        "CritReductionPercentage": 25
    },
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Omega name",
            "TotalDurationMs": 1500,
            "Skills": [
                {
                    "Name": "Mark",
                    "Targeting": {
                        "Type": "CurrentFocus"
                    },
                    "Deployment": {
                        "Type": "Direct",
                        "PreDeploymentDelayPercentage": 25
                    },
                    "PercentageOfAbilityDuration": 100,
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "SpawnMark",
                                "MarkType": "Beneficial",
                                "DurationMs": 100,
                                "ShouldDestroyAttachedEntityOnSenderDeath": true,
                                "AttachedAbilities": [
                                    {
                                        "Name": "Mark Name",
                                        "ActivationTrigger": {
                                            "TriggerType": "OnHit",
                                            "AbilityTypes": ["Attack"],
                                            "SenderAllegiance": "Ally",
                                            "ReceiverAllegiance": "Enemy",
                                            "RangeUnits": 20,
                                            "NotBeforeMs": 10,
                                            "NotAfterMs": 15,
                                            "OnlyFocus": true,
                                            "OncePerSpawnedEntity": true
                                        },
                                        "TotalDurationMs": 100,
                                        "Skills": [
                                            {
                                                "Name": "Skill Name",
                                                "Deployment": {
                                                    "Type": "Direct"
                                                },
                                                "Targeting": {
                                                    "Type": "CurrentFocus"
                                                },
                                                "EffectPackage": {
                                                    "Effects": [
                                                        {
                                                            "Type": "InstantHeal",
                                                            "HealType": "Normal",
                                                            "Expression": 1000
                                                        }
                                                    ]
                                                }
                                            }
                                        ]
                                    }
                                ]
                            }
                        ]
                    }

                }
            ]
        }
    ]
}
)";

    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();
    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));

    // Check omega abilities
    {
        const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();
        ASSERT_EQ(omega_abilities.abilities.size(), 1);
        const auto& omega_ability = omega_abilities.abilities.at(0);
        ASSERT_EQ(omega_ability->name, "Omega name");
        ASSERT_EQ(omega_ability->total_duration_ms, 1500);

        // Check omega skills
        ASSERT_EQ(omega_ability->skills.size(), 1);
        const auto& omega_skill = omega_ability->skills.at(0);
        ASSERT_EQ(omega_skill->name, "Mark");
        ASSERT_EQ(omega_skill->deployment.type, SkillDeploymentType::kDirect);
        ASSERT_EQ(omega_skill->targeting.type, SkillTargetingType::kCurrentFocus);

        // Check omega effects
        ASSERT_EQ(omega_skill->effect_package.effects.size(), 1);
        const auto& omega_effect = omega_skill->effect_package.effects.at(0);
        ASSERT_EQ(omega_effect.type_id.type, EffectType::kSpawnMark);
        ASSERT_EQ(omega_effect.type_id.mark_type, MarkEffectType::kBeneficial);
        ASSERT_EQ(omega_effect.lifetime.duration_time_ms, 100);
        ASSERT_EQ(omega_effect.attached_abilities.size(), 1);
        ASSERT_EQ(omega_effect.should_destroy_attached_entity_on_sender_death, true);

        // Check innate abilities
        ASSERT_EQ(omega_effect.attached_abilities.size(), 1);
        const auto& mark_abilities_data = omega_effect.attached_abilities.at(0);
        ASSERT_EQ(mark_abilities_data->name, "Mark Name");
        ASSERT_EQ(mark_abilities_data->activation_trigger_data.trigger_type, ActivationTriggerType::kOnHit);
        ASSERT_EQ(mark_abilities_data->activation_trigger_data.ability_types, MakeEnumSet(AbilityType::kAttack));
        ASSERT_EQ(mark_abilities_data->activation_trigger_data.sender_allegiance, AllegianceType::kAlly);
        ASSERT_EQ(mark_abilities_data->activation_trigger_data.receiver_allegiance, AllegianceType::kEnemy);
        ASSERT_EQ(mark_abilities_data->activation_trigger_data.range_units, 20);
        ASSERT_EQ(mark_abilities_data->activation_trigger_data.not_before_ms, 10);
        ASSERT_EQ(mark_abilities_data->activation_trigger_data.not_after_ms, 15);
        ASSERT_EQ(mark_abilities_data->activation_trigger_data.only_focus, true);
        ASSERT_EQ(mark_abilities_data->activation_trigger_data.once_per_spawned_entity, true);
        ASSERT_EQ(mark_abilities_data->total_duration_ms, 100);

        // Check innate skills
        ASSERT_EQ(mark_abilities_data->skills.size(), 1);
        const auto& innate_skill = mark_abilities_data->skills.at(0);
        EXPECT_EQ(innate_skill->name, "Skill Name");
        EXPECT_EQ(innate_skill->deployment.type, SkillDeploymentType::kDirect);
        EXPECT_EQ(innate_skill->targeting.type, SkillTargetingType::kCurrentFocus);

        // Check innate effects
        ASSERT_EQ(innate_skill->effect_package.effects.size(), 1);
        const auto& innate_effect = innate_skill->effect_package.effects.at(0);
        EXPECT_EQ(innate_effect.type_id.type, EffectType::kInstantHeal);
        EXPECT_EQ(innate_effect.type_id.heal_type, EffectHealType::kNormal);
        EXPECT_EQ(innate_effect.GetExpression().base_value.value, 1000_fp);
    }
}
TEST_F(JSONTest, LoadCombatUnitWithEffectPackageBlock)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Super Axolotl",
    "Stage": 1,
    "UnitType": "Illuvial",
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "Tier": 1,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MaxHealth": 950,
        "StartingEnergy": 30,
        "EnergyCost": 60,
        "PhysicalResist": 60,
        "EnergyResist": 60,
        "TenacityPercentage": 0,
        "WillpowerPercentage": 0,
        "Grit": 0,
        "Resolve": 0,
        "AttackPhysicalDamage": 45,
        "AttackEnergyDamage": 0,
        "AttackPureDamage": 0,
        "AttackSpeed": 60,
        "MoveSpeedSubUnits": 5000,
        "HitChancePercentage": 100,
        "AttackDodgeChancePercentage": 0,
        "CritChancePercentage": 25,
        "CritAmplificationPercentage": 150,
        "OmegaPowerPercentage": 1,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 0,
        "HealthRegeneration": 0,
        "EnergyRegeneration": 0,
        "EnergyGainEfficiencyPercentage": 100,
        "OnActivationEnergy": 0,
        "VulnerabilityPercentage": 0,
        "EnergyPiercingPercentage": 0,
        "PhysicalPiercingPercentage": 0,
        "HealthGainEfficiencyPercentage": 100,
        "PhysicalVampPercentage": 0,
        "EnergyVampPercentage": 0,
        "PureVampPercentage": 0,
        "Thorns": 0,
        "StartingShield": 0,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack",
            "TotalDurationMs": 2345,
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": {
                        "Type": "CurrentFocus"
                    },
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 0
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Ability Name - Zones",
            "TotalDurationMs": 5000,
            "Skills": [
                {
                    "Name": "Ability Skill 1",
                    "PercentageOfAbilityDuration": 100,
                    "Deployment": {
                        "Type": "Direct",
                        "PreDeploymentDelayPercentage": 21
                    },
                    "Targeting": {
                        "Type": "Synergy",
                        "Group": "Enemy",
                        "CombatClass": "Fighter"
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "Empower",
                                "ActivatedBy": "Omega",
                                "DurationMs": 1600,
                                "AttachedEffects": [
                                    {
                                        "Type": "PositiveState",
                                        "PositiveState": "EffectPackageBlock",
                                        "DurationMs": -1,
                                        "BlocksUntilExpiry": 3,
                                        "AbilityTypes": [
                                            "Omega"
                                        ]
                                    }
                                ]
                            },
                            {
                                "Type": "Buff",
                                "Stat": "EnergyResist",
                                "DurationMs": -1,
                                "Expression": {
                                    "Percentage": 5,
                                    "Stat": "MaxHealth",
                                    "StatSource": "Receiver"
                                }
                            },
                            {
                                "Type": "Debuff",
                                "Stat": "PhysicalResist",
                                "DurationMs": -1,
                                "Expression": {
                                    "Percentage": 4,
                                    "Stat": "MaxHealth",
                                    "StatSource": "Sender"
                                }
                            },
                            {
                                "Type": "Debuff",
                                "Stat": "OmegaRangeUnits",
                                "DurationMs": -1,
                                "Expression": {
                                    "Percentage": 1,
                                    "Stat": "Grit",
                                    "StatEvaluationType": "Base",
                                    "StatSource": "Sender"
                                }
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";

    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));
    const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();

    ASSERT_EQ(omega_abilities.abilities.size(), 1);

    const auto& omega_ability = omega_abilities.abilities.at(0);
    EXPECT_EQ(omega_ability->name, "Ability Name - Zones");
    EXPECT_EQ(omega_ability->total_duration_ms, 5000);
    ASSERT_EQ(omega_ability->skills.size(), 1);

    // First skill
    const auto& omega_skill = omega_ability->skills.at(0);
    EXPECT_EQ(omega_skill->name, "Ability Skill 1");

    // Check omega effects
    ASSERT_EQ(omega_skill->effect_package.effects.size(), 4);

    // First effect
    {
        const auto& omega_effect = omega_skill->effect_package.effects.at(0);
        EXPECT_EQ(omega_effect.type_id.type, EffectType::kEmpower);
        EXPECT_EQ(omega_effect.lifetime.duration_time_ms, 1600);

        EXPECT_EQ(omega_effect.attached_effects.size(), 1);

        // First Attached effect
        {
            const auto& attached_effect = omega_effect.attached_effects.at(0);
            EXPECT_EQ(attached_effect.type_id.type, EffectType::kPositiveState);
            EXPECT_EQ(attached_effect.type_id.positive_state, EffectPositiveState::kEffectPackageBlock);
            EXPECT_EQ(attached_effect.lifetime.duration_time_ms, kTimeInfinite);
            EXPECT_EQ(attached_effect.lifetime.blocks_until_expiry, 3);
            ASSERT_EQ(attached_effect.ability_types.size(), 1);
            EXPECT_EQ(attached_effect.ability_types[0], AbilityType::kOmega);
        }
    }

    // Second effect
    {
        const auto& omega_effect = omega_skill->effect_package.effects.at(1);
        EXPECT_EQ(omega_effect.type_id.type, EffectType::kBuff);
        EXPECT_EQ(omega_effect.type_id.stat_type, StatType::kEnergyResist);
        EXPECT_EQ(omega_effect.lifetime.duration_time_ms, kTimeInfinite);

        EXPECT_EQ(omega_effect.GetExpression().base_value.type, EffectValueType::kStatPercentage);
        EXPECT_EQ(omega_effect.GetExpression().base_value.stat, StatType::kMaxHealth);
        EXPECT_EQ(omega_effect.GetExpression().base_value.value, 5_fp);
        EXPECT_EQ(omega_effect.GetExpression().base_value.data_source_type, ExpressionDataSourceType::kReceiver);

        // Default should be live
        EXPECT_EQ(omega_effect.GetExpression().base_value.stat_evaluation_type, StatEvaluationType::kLive);
    }

    // Third effect
    {
        const auto& omega_effect = omega_skill->effect_package.effects.at(2);
        EXPECT_EQ(omega_effect.type_id.type, EffectType::kDebuff);
        EXPECT_EQ(omega_effect.type_id.stat_type, StatType::kPhysicalResist);
        EXPECT_EQ(omega_effect.lifetime.duration_time_ms, kTimeInfinite);

        EXPECT_EQ(omega_effect.GetExpression().base_value.type, EffectValueType::kStatPercentage);
        EXPECT_EQ(omega_effect.GetExpression().base_value.stat, StatType::kMaxHealth);
        EXPECT_EQ(omega_effect.GetExpression().base_value.value, 4_fp);
        EXPECT_EQ(omega_effect.GetExpression().base_value.data_source_type, ExpressionDataSourceType::kSender);

        // Default should be live
        EXPECT_EQ(omega_effect.GetExpression().base_value.stat_evaluation_type, StatEvaluationType::kLive);
    }

    // Fourth effect
    {
        const auto& omega_effect = omega_skill->effect_package.effects.at(3);
        EXPECT_EQ(omega_effect.type_id.type, EffectType::kDebuff);
        EXPECT_EQ(omega_effect.type_id.stat_type, StatType::kOmegaRangeUnits);
        EXPECT_EQ(omega_effect.lifetime.duration_time_ms, kTimeInfinite);

        EXPECT_EQ(omega_effect.GetExpression().base_value.type, EffectValueType::kStatPercentage);
        EXPECT_EQ(omega_effect.GetExpression().base_value.stat, StatType::kGrit);
        EXPECT_EQ(omega_effect.GetExpression().base_value.value, 1_fp);
        EXPECT_EQ(omega_effect.GetExpression().base_value.data_source_type, ExpressionDataSourceType::kSender);

        // Should be base
        EXPECT_EQ(omega_effect.GetExpression().base_value.stat_evaluation_type, StatEvaluationType::kBase);
    }
}

TEST_F(JSONTest, LoadCombatUnitThrowDisplacement)
{
    const char* combat_unit_json_raw = R"(
{
    "Line": "Test Axolotl",
    "Stage": 1,
    "UnitType": "Illuvial",
    "CombatAffinity": "Water",
    "CombatClass": "Bulwark",
    "Tier": 1,
    "SizeUnits": 5,
    "Path": "",
    "Variation": "",
    "Stats": {
        "MaxHealth": 950,
        "StartingEnergy": 30,
        "EnergyCost": 60,
        "PhysicalResist": 60,
        "EnergyResist": 60,
        "TenacityPercentage": 0,
        "WillpowerPercentage": 0,
        "Grit": 0,
        "Resolve": 0,
        "AttackPhysicalDamage": 45,
        "AttackEnergyDamage": 0,
        "AttackPureDamage": 0,
        "AttackSpeed": 60,
        "MoveSpeedSubUnits": 5000,
        "HitChancePercentage": 100,
        "AttackDodgeChancePercentage": 0,
        "CritChancePercentage": 25,
        "CritAmplificationPercentage": 150,
        "OmegaPowerPercentage": 1,
        "AttackRangeUnits": 42,
        "OmegaRangeUnits": 42,
        "HealthRegeneration": 0,
        "EnergyRegeneration": 0,
        "EnergyGainEfficiencyPercentage": 100,
        "OnActivationEnergy": 0,
        "VulnerabilityPercentage": 0,
        "EnergyPiercingPercentage": 0,
        "PhysicalPiercingPercentage": 0,
        "HealthGainEfficiencyPercentage": 100,
        "PhysicalVampPercentage": 0,
        "EnergyVampPercentage": 0,
        "PureVampPercentage": 0,
        "Thorns": 0,
        "StartingShield": 0,
        "CritReductionPercentage": 25
    },
    "AttackAbilitiesSelection": "Cycle",
    "AttackAbilities": [
        {
            "Name": "Attack",
            "TotalDurationMs": 2345,
            "Skills": [
                {
                    "Name": "Attack",
                    "Targeting": {
                        "Type": "CurrentFocus",
                        "Guidance": [ "Ground" ]
                    },
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "InstantDamage",
                                "DamageType": "Physical",
                                "Expression": 0
                            }
                        ]
                    }
                }
            ]
        }
    ],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Ability Name - Throw",
            "TotalDurationMs": 1000,
            "Skills": [
                {
                    "Name": "Throw Skill",
                    "PercentageOfAbilityDuration": 100,
                    "Targeting": {
                        "Type": "CurrentFocus"
                    },
                    "Deployment": {
                        "Type": "Direct",
                        "Guidance": [ "Ground" ],
                        "PreDeploymentDelayPercentage": 33
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "Displacement",
                                "DisplacementType": "ThrowToFurthestEnemy",
                                "DurationMs": 500,
                                "OnFinished" : {
                                    "Name": "AoE Damage",
                                    "Deployment": {
                                        "Type": "Zone"
                                    },
                                    "Zone": {
                                        "Shape": "Hexagon",
                                        "RadiusUnits": 20,
                                        "DurationMs": 100,
                                        "FrequencyMs": 100,
                                        "ApplyOnce": true
                                    },
                                    "Targeting": {
                                        "Type": "Self"
                                    },
                                    "EffectPackage": {
                                        "Effects": [
                                            {
                                                "Type": "InstantDamage",
                                                "DamageType": "Physical",
                                                "Expression": 1000000
                                            }
                                        ]
                                    }
                                }
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";

    CombatUnitData data = CreateCombatUnitData();
    BaseDataLoader type_data_loader(world->GetLogger());
    type_data_loader.LoadCombatUnitData(nlohmann::json::parse(combat_unit_json_raw), &data);
    ASSERT_EQ(data.radius_units, 5);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(Team::kBlue, {10, 20}, data, entity);

    // Check abilities
    const auto& abilities_component = entity->Get<AbilitiesComponent>();

    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kAttack));
    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kOmega));
    const auto& omega_abilities = abilities_component.GetDataOmegaAbilities();

    ASSERT_EQ(omega_abilities.abilities.size(), 1);

    // Ability 3 with 1 throw skill
    {
        const auto& omega_ability = omega_abilities.abilities.at(0);
        EXPECT_EQ(omega_ability->name, "Ability Name - Throw");
        EXPECT_EQ(omega_ability->total_duration_ms, 1000);
        ASSERT_EQ(omega_ability->skills.size(), 1);

        // throw skill
        {
            const auto& omega_skill = omega_ability->skills.at(0);
            EXPECT_EQ(omega_skill->name, "Throw Skill");
            EXPECT_EQ(omega_skill->percentage_of_ability_duration, 100);
            EXPECT_EQ(omega_skill->deployment.pre_deployment_delay_percentage, 33);

            EXPECT_EQ(omega_skill->targeting.type, SkillTargetingType::kCurrentFocus);
            EXPECT_EQ(omega_skill->deployment.type, SkillDeploymentType::kDirect);

            // throw effect
            {
                const auto& throw_effect = omega_skill->effect_package.effects.at(0);
                EXPECT_EQ(throw_effect.type_id.type, EffectType::kDisplacement);
                EXPECT_EQ(throw_effect.type_id.displacement_type, EffectDisplacementType::kThrowToFurthestEnemy);
                EXPECT_EQ(throw_effect.lifetime.duration_time_ms, 500);

                // On Finished skill
                EXPECT_EQ(throw_effect.event_skills.count(EventType::kDisplacementStopped), 1);

                const auto& skill_data = throw_effect.event_skills.at(EventType::kDisplacementStopped);
                EXPECT_EQ(skill_data->targeting.type, SkillTargetingType::kSelf);
                EXPECT_EQ(skill_data->deployment.type, SkillDeploymentType::kZone);
            }
        }
    }
}

TEST_F(JSONTest, LoadStatOverrideEffect)
{
    auto silent_loader = TestingDataLoader::MakeSilent();

    // Should fail because stat type is required
    EXPECT_FALSE(silent_loader.IsValidEffect(R"({
        "Type": "StatOverride",
        "Value": 150
    })"));

    // Should fail Value is required
    EXPECT_FALSE(silent_loader.IsValidEffect(R"({
        "Type": "StatOverride",
        "Stat": "CritChancePercentage"
    })"));

    auto logging_loader = TestingDataLoader::MakeFrom(world);

    // Should fail value is required
    const char* json_text = R"({
        "Type": "StatOverride",
        "Stat": "CritChancePercentage",
        "Value": 150
    })";

    auto maybe_effect = logging_loader.ParseAndLoadEffect(json_text);
    EXPECT_TRUE(maybe_effect);

    const EffectData& effect = *maybe_effect;
    EXPECT_EQ(effect.type_id.type, EffectType::kStatOverride);
    EXPECT_EQ(effect.type_id.stat_type, StatType::kCritChancePercentage);
    EXPECT_EQ(effect.GetExpression().base_value.value, 150_fp);
}

TEST_F(JSONTest, LoadDynamicBuff)
{
    auto loader = TestingDataLoader::MakeFrom(world);

    // Should fail because stat type is required
    const auto opt_effect = loader.ParseAndLoadEffect(R"({
        "Type": "Buff",
        "Stat": "OmegaPowerPercentage",
        "DurationMs": -1,
        "FrequencyMs": 333,
        "Expression": {
            "Operation": "+",
            "Operands": [
                {
                    "Stat": "OmegaPowerPercentage",
                    "StatSource": "Sender"
                },
                3
            ]
        }
    })");
    ASSERT_TRUE(opt_effect.has_value());
    const auto& effect = opt_effect.value();
    ASSERT_EQ(effect.type_id.type, EffectType::kBuff);
    ASSERT_EQ(effect.type_id.stat_type, StatType::kOmegaPowerPercentage);
    ASSERT_EQ(effect.lifetime.duration_time_ms, kTimeInfinite);
    ASSERT_EQ(effect.lifetime.frequency_time_ms, 333);
    ASSERT_EQ(effect.GetExpression().operation_type, EffectOperationType::kAdd);
    ASSERT_EQ(effect.GetExpression().operands.size(), 2);
    ASSERT_EQ(effect.GetExpression().operands.front().base_value.stat_evaluation_type, StatEvaluationType::kLive);
}

TEST_F(JSONTest, DebuffWithBaseStat)
{
    auto loader = TestingDataLoader::MakeFrom(world);

    // Should fail because stat type is required
    const auto opt_effect = loader.ParseAndLoadEffect(R"({
        "Type": "Debuff",
        "Stat": "OmegaRangeUnits",
        "DurationMs": -1,
        "Expression": {
            "Percentage": 1,
            "Stat": "Grit",
            "StatEvaluationType": "Base",
            "StatSource": "Sender"
        }
    })");
    ASSERT_TRUE(opt_effect.has_value());
    const auto& effect = opt_effect.value();
    ASSERT_EQ(effect.type_id.type, EffectType::kDebuff);
    ASSERT_EQ(effect.type_id.stat_type, StatType::kOmegaRangeUnits);
    ASSERT_EQ(effect.lifetime.duration_time_ms, kTimeInfinite);
    ASSERT_EQ(effect.lifetime.frequency_time_ms, 0);
    ASSERT_EQ(effect.GetExpression().operation_type, EffectOperationType::kNone);
    ASSERT_EQ(effect.GetExpression().operands.size(), 0);
    ASSERT_EQ(effect.GetExpression().base_value.stat_evaluation_type, StatEvaluationType::kBase);
}

TEST_F(JSONTest, LoadExpressionUseCurrentFocus)
{
    auto logging_loader = TestingDataLoader::MakeFrom(world);

    const char* json_text = R"({
        "Percentage": 20,
        "Stat": "AttackSpeed",
        "StatSource": "SenderFocus"
    })";

    auto maybe_expression = logging_loader.ParseAndLoadExpression(json_text);
    ASSERT_TRUE(maybe_expression.has_value());

    const EffectExpression& expression = *maybe_expression;
    EXPECT_EQ(expression.operation_type, EffectOperationType::kNone);
    EXPECT_EQ(expression.base_value.data_source_type, ExpressionDataSourceType::kSenderFocus);
    EXPECT_EQ(expression.base_value.stat, StatType::kAttackSpeed);
    EXPECT_EQ(expression.base_value.value, 20_fp);
}

TEST_F(JSONTest, LoadConsumables)
{
    const char* consumable_json_raw = R"(
{
    "Name": "BasketFruit",
    "Stage": 0,
    "Tier": 0,
    "Abilities": [
        {
            "Name": "Consumable - Floraball",
            "ActivationTrigger": {
                "TriggerType": "OnBattleStart"
            },
            "TotalDurationMs": 0,
            "Skills": [
                {
                    "Name": "Max Health and Energy/Physical Resist buff",
                    "Deployment": {
                        "Type": "Direct"
                    },
                    "Targeting": {
                        "Type": "Self"
                    },
                    "EffectPackage": {
                        "Effects": [
                            {
                                "Type": "Buff",
                                "Stat": "MaxHealth",
                                "Expression": 400,
                                "DurationMs": -1
                            },
                            {
                                "Type": "Buff",
                                "Stat": "EnergyResist",
                                "Expression": 25,
                                "DurationMs": -1
                            },
                            {
                                "Type": "Buff",
                                "Stat": "PhysicalResist",
                                "Expression": 25,
                                "DurationMs": -1
                            }
                        ]
                    }
                }
            ]
        }
    ]
}
)";

    BattleBaseDataLoader battle_data_loader(world->GetLogger());
    auto consumable_data = battle_data_loader.LoadConsumableFromJSON(nlohmann::json::parse(consumable_json_raw));
    ASSERT_NE(consumable_data, nullptr);

    game_data_container_->AddConsumableData(consumable_data->type_id, consumable_data);
    const ConsumableTypeID type_id = consumable_data->type_id;

    EXPECT_EQ(type_id.stage, 0);
    EXPECT_EQ(consumable_data->tier, 0);
    EXPECT_EQ(type_id.name, "BasketFruit");

    FullCombatUnitData full_data;
    full_data.instance = CreateCombatUnitInstanceData();
    full_data.instance.team = Team::kBlue;
    full_data.instance.position = {10, 20};
    full_data.instance.equipped_suit.type_id = CombatSuitTypeID{"CoolSuit", 1, ""};
    full_data.instance.equipped_weapon.type_id = CombatWeaponTypeID{"Pistol", 2, CombatAffinity::kWater};

    ConsumableInstanceData consumable_instance_data{};
    consumable_instance_data.type_id = type_id;
    full_data.instance.equipped_consumables.push_back(consumable_instance_data);

    // Spawn the CombatUnit
    Entity* entity = nullptr;
    SpawnCombatUnit(full_data, entity);
    TimeStepForMs(100);

    const auto& abilities_component = entity->Get<AbilitiesComponent>();
    ASSERT_TRUE(abilities_component.HasAnyAbility(AbilityType::kInnate));
}

TEST_F(JSONTest, LoadMarkEffect)
{
    auto silent_loader = TestingDataLoader::MakeSilent();

    // Should fail becase MarkType is required
    {
        const char* json_text = R"({
            "Type": "SpawnMark"
        })";

        auto maybe_type_id = silent_loader.ParseAndLoadEffectTypeID(json_text);
        ASSERT_FALSE(maybe_type_id.has_value());
    }

    auto logging_loader = TestingDataLoader::MakeFrom(world);

    {
        const char* json_text = R"({
            "Type": "SpawnMark",
            "MarkType": "Beneficial"
        })";

        auto maybe_type_id = logging_loader.ParseAndLoadEffectTypeID(json_text);
        ASSERT_TRUE(maybe_type_id.has_value());

        const EffectTypeID& effect_type_id = *maybe_type_id;
        EXPECT_EQ(effect_type_id.type, EffectType::kSpawnMark);
        EXPECT_EQ(effect_type_id.mark_type, MarkEffectType::kBeneficial);
    }

    {
        const char* json_text = R"({
            "Type": "SpawnMark",
            "MarkType": "Detrimental"
        })";

        auto maybe_type_id = logging_loader.ParseAndLoadEffectTypeID(json_text);
        ASSERT_TRUE(maybe_type_id.has_value());

        const EffectTypeID& effect_type_id = *maybe_type_id;
        EXPECT_EQ(effect_type_id.type, EffectType::kSpawnMark);
        EXPECT_EQ(effect_type_id.mark_type, MarkEffectType::kDetrimental);
    }
}

TEST_F(JSONTest, ParseStatParameters)
{
    auto stat_params = TestingDataLoader::ParseStatParameters("something");
    EXPECT_EQ(stat_params.is_custom_evaluation_function, false);
    EXPECT_EQ(stat_params.is_metered_stat_percentage, false);
    EXPECT_EQ(stat_params.stat_str, "something");

    stat_params = TestingDataLoader::ParseStatParameters("something%");
    EXPECT_EQ(stat_params.is_custom_evaluation_function, false);
    EXPECT_EQ(stat_params.is_metered_stat_percentage, true);
    EXPECT_EQ(stat_params.stat_str, "something");

    stat_params = TestingDataLoader::ParseStatParameters("$something");
    EXPECT_EQ(stat_params.is_custom_evaluation_function, true);
    EXPECT_EQ(stat_params.is_metered_stat_percentage, false);
    EXPECT_EQ(stat_params.stat_str, "something");

    stat_params = TestingDataLoader::ParseStatParameters("$something%");
    EXPECT_EQ(stat_params.is_custom_evaluation_function, true);
    EXPECT_EQ(stat_params.is_metered_stat_percentage, true);
    EXPECT_EQ(stat_params.stat_str, "something");

    stat_params = TestingDataLoader::ParseStatParameters("");
    EXPECT_EQ(stat_params.is_custom_evaluation_function, false);
    EXPECT_EQ(stat_params.is_metered_stat_percentage, false);
    EXPECT_EQ(stat_params.stat_str, "");

    stat_params = TestingDataLoader::ParseStatParameters("%");
    EXPECT_EQ(stat_params.is_custom_evaluation_function, false);
    EXPECT_EQ(stat_params.is_metered_stat_percentage, true);
    EXPECT_EQ(stat_params.stat_str, "");

    stat_params = TestingDataLoader::ParseStatParameters("$");
    EXPECT_EQ(stat_params.is_custom_evaluation_function, true);
    EXPECT_EQ(stat_params.is_metered_stat_percentage, false);
    EXPECT_EQ(stat_params.stat_str, "");

    stat_params = TestingDataLoader::ParseStatParameters("$%");
    EXPECT_EQ(stat_params.is_custom_evaluation_function, true);
    EXPECT_EQ(stat_params.is_metered_stat_percentage, true);
    EXPECT_EQ(stat_params.stat_str, "");
}

TEST_F(JSONTest, LoadExcessHealToShield)
{
    auto loader = TestingDataLoader::MakeFrom(world);

    const char* json_text = R"({
            "Type": "InstantHeal",
            "HealType": "Normal",
            "Expression": 150,
            "ExcessHealToShield": 1
        })";

    auto maybe_effect = loader.ParseAndLoadEffect(json_text);
    ASSERT_TRUE(maybe_effect.has_value());

    const EffectData& effect = *maybe_effect;

    EXPECT_EQ(effect.type_id.type, EffectType::kInstantHeal);
    EXPECT_EQ(effect.type_id.heal_type, EffectHealType::kNormal);

    EXPECT_EQ(effect.attributes.excess_heal_to_shield, 1_fp);
}

class LoadCombatUnitStatsJSONTests : public BaseJSONTest
{
};

TEST_F(LoadCombatUnitStatsJSONTests, LoadStatsMapFromJSON)
{
    {
        static constexpr std::string_view json = R"({
            "CurrentHealth": 42
        })";

        auto opt_map = loader->ParseAndLoadStatsMap(json);
        ASSERT_TRUE(opt_map.has_value());

        auto& map = *opt_map;
        EXPECT_EQ(map[StatType::kCurrentHealth], 42_fp);
    }

    // Floats are not allowed in this mode
    {
        static constexpr std::string_view json = R"({
            "CurrentHealth": 42.0
        })";

        auto opt_map = silent_loader->ParseAndLoadStatsMap(json);
        EXPECT_FALSE(opt_map.has_value());
    }

    // Strings are not allowed in this mode
    {
        static constexpr std::string_view json = R"({
            "CurrentHealth": "42"
        })";

        auto opt_map = silent_loader->ParseAndLoadStatsMap(json);
        EXPECT_FALSE(opt_map.has_value());
    }

    // With strings
    {
        static constexpr std::string_view json = R"({
            "CurrentHealth": "42"
        })";

        auto opt_map = loader->ParseAndLoadStatsMap_AllowStrings(json);
        ASSERT_TRUE(opt_map.has_value());

        auto& map = *opt_map;
        EXPECT_EQ(map[StatType::kCurrentHealth], 42_fp);
    }

    // With one symbol after dot
    {
        static constexpr std::string_view json = R"({
            "CurrentHealth": "42.1"
        })";

        auto opt_map = loader->ParseAndLoadStatsMap_AllowStrings(json);
        ASSERT_TRUE(opt_map.has_value());

        auto& map = *opt_map;
        EXPECT_EQ(map[StatType::kCurrentHealth], "42.1"_fp);
    }

    // With two symbols after dot
    {
        static constexpr std::string_view json = R"({
            "CurrentHealth": "42.12"
        })";

        auto opt_map = loader->ParseAndLoadStatsMap_AllowStrings(json);
        ASSERT_TRUE(opt_map.has_value());

        auto& map = *opt_map;
        EXPECT_EQ(map[StatType::kCurrentHealth], "42.12"_fp);
    }

    // With three symbols after dot
    {
        static constexpr std::string_view json = R"({
            "CurrentHealth": "42.123"
        })";

        auto opt_map = loader->ParseAndLoadStatsMap_AllowStrings(json);
        ASSERT_TRUE(opt_map.has_value());

        auto& map = *opt_map;
        EXPECT_EQ(map[StatType::kCurrentHealth], "42.123"_fp);
    }

    // With four symbols after dot
    {
        static constexpr std::string_view json = R"({
            "CurrentHealth": "42.1234"
        })";

        auto opt_map = silent_loader->ParseAndLoadStatsMap_AllowStrings(json);
        EXPECT_FALSE(opt_map.has_value());
    }
}

}  // namespace simulation
