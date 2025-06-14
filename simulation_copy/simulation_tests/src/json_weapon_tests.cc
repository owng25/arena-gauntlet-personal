#include "base_json_test.h"

namespace simulation
{

// Helper function to verify WeaponTypeID fields
void VerifyWeaponTypeID(
    const CombatWeaponTypeID& type_id,
    const std::string& expected_name,
    int expected_stage,
    CombatAffinity expected_combat_affinity,
    const std::string& expected_variation)
{
    EXPECT_EQ(type_id.name, expected_name);
    EXPECT_EQ(type_id.stage, expected_stage);
    EXPECT_EQ(type_id.combat_affinity, expected_combat_affinity);
    EXPECT_EQ(type_id.variation, expected_variation);
}

// Helper function to verify a common set of stats
// Ensure StatType and the _fp literal are accessible here
void VerifyDefaultStats(const StatsData& stats)
{
    EXPECT_EQ(stats.Get(StatType::kMoveSpeedSubUnits), 10000_fp);
    EXPECT_EQ(stats.Get(StatType::kEnergyCost), 50000_fp);
    EXPECT_EQ(stats.Get(StatType::kAttackPhysicalDamage), 10_fp);
    EXPECT_EQ(stats.Get(StatType::kAttackEnergyDamage), 11_fp);
    EXPECT_EQ(stats.Get(StatType::kAttackSpeed), 160_fp);
    EXPECT_EQ(stats.Get(StatType::kAttackRangeUnits), 40_fp);
    EXPECT_EQ(stats.Get(StatType::kOmegaRangeUnits), 0_fp);
    EXPECT_EQ(stats.Get(StatType::kHitChancePercentage), 80_fp);
    EXPECT_EQ(stats.Get(StatType::kPhysicalPiercingPercentage), 21_fp);
    EXPECT_EQ(stats.Get(StatType::kEnergyPiercingPercentage), 22_fp);
    EXPECT_EQ(stats.Get(StatType::kCritAmplificationPercentage), 5_fp);
    EXPECT_EQ(stats.Get(StatType::kCritChancePercentage), 6_fp);
    EXPECT_EQ(stats.Get(StatType::kPhysicalResist), 15_fp);
    EXPECT_EQ(stats.Get(StatType::kEnergyResist), 17_fp);
    EXPECT_EQ(stats.Get(StatType::kMaxHealth), 100000_fp);
    EXPECT_EQ(stats.Get(StatType::kOnActivationEnergy), 25_fp);
    EXPECT_EQ(stats.Get(StatType::kStartingEnergy), 5000_fp);
    EXPECT_EQ(stats.Get(StatType::kOmegaPowerPercentage), 42_fp);
    EXPECT_EQ(stats.Get(StatType::kCritReductionPercentage), 25_fp);
}

// Helper function to verify ability lists
void VerifyAbilityList(
    const AbilitiesData& list_data,
    size_t expected_size,
    std::optional<AbilityUpdateType> expected_update_type_for_first = std::nullopt)
{
    ASSERT_EQ(list_data.abilities.size(), expected_size);
    if (expected_size > 0 && !list_data.abilities.empty() && expected_update_type_for_first.has_value())
    {
        // Ensure the list is not empty before accessing front() even if expected_size > 0
        // This check is somewhat redundant with ASSERT_EQ if expected_size > 0, but safe.
        auto& first_ability = list_data.abilities.front();
        EXPECT_EQ(first_ability->update_type, expected_update_type_for_first.value());
    }
}

class WeaponTestJSON : public BaseJSONTest
{
public:
    using Super = BaseJSONTest;
};

TEST_F(WeaponTestJSON, Normal)
{
    // Regular valid scenario
    {
        const char* weapon_json_raw = R"(
{
    "Name": "Pistol",
    "Stage": 2,
    "CombatAffinity": "Tsunami",
    "DominantCombatAffinity": "Water",
    "CombatClass": "Berserker",
    "DominantCombatClass": "Fighter",
    "Variation": "yo",
    "Type": "Normal",
    "Tier": 3,
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

        auto maybe_data = loader->ParseAndLoadWeapon(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());

        const CombatUnitWeaponData& data = *maybe_data;

        VerifyWeaponTypeID(data.type_id, "Pistol", 2, CombatAffinity::kTsunami, "yo");

        EXPECT_EQ(data.tier, 3);
        EXPECT_EQ(data.combat_affinity, CombatAffinity::kTsunami);
        EXPECT_EQ(data.dominant_combat_affinity, CombatAffinity::kWater);
        EXPECT_EQ(data.combat_class, CombatClass::kBerserker);
        EXPECT_EQ(data.dominant_combat_class, CombatClass::kFighter);

        VerifyDefaultStats(data.stats);

        VerifyAbilityList(data.attack_abilities, 1, AbilityUpdateType::kNone);
        VerifyAbilityList(data.omega_abilities, 1, AbilityUpdateType::kNone);
    }
}

TEST_F(WeaponTestJSON, AmplifierDefault)
{
    // Regular valid scenario
    {
        const char* weapon_json_raw = R"(
{
    "Name": "AmplifierTest",
    "Stage": 0,
    "Variation": "Original",
    "Type": "Amplifier",
    "Tier": 0,
    "AmplifierForWeapon": {
       "Name": "FlamewardenStaff",
       "Stage": 0,
       "Variation": "Original",
       "CombatAffinity": "Fire"
    },
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
    ],
    "InnateAbilities": [
       {
          "Name": "Zone Breaker",
          "ActivationTrigger": {
             "TriggerType": "OnActivateXAbilities",
             "AbilityTypes": [
                "Omega"
             ],
             "EveryX": true,
             "NumberOfAbilitiesActivated": 1
          },
          "TotalDurationMs": 0,
          "Skills": [
             {
                "Name": "ZoneBreaker_Ability",
                "Targeting": {
                   "Type": "CurrentFocus"
                },
                "Deployment": {
                   "Type": "Projectile",
                   "Guidance": [
                      "Ground",
                      "Airborne"
                   ],
                   "PreDeploymentDelayPercentage": 45
                },
                "Projectile": {
                   "SizeUnits": 3,
                   "IsHoming": true,
                   "SpeedSubUnits": 8000,
                   "IsBlockable": false,
                   "ApplyToAll": false
                },
                "PercentageOfAbilityDuration": 100,
                "EffectPackage": {
                   "Attributes": {
                      "Propagation": {
                         "PropagationType": "Splash",
                         "SplashRadiusUnits": 30,
                         "DeploymentGuidance": [
                            "Ground",
                            "Airborne",
                            "Underground"
                         ],
                         "EffectPackage": {
                            "Effects": []
                         }
                      }
                   },
                   "Effects": []
                }
             }
          ]
       }
    ]
}
)";

        auto maybe_data = loader->ParseAndLoadWeapon(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());

        const CombatUnitWeaponData& data = *maybe_data;

        VerifyWeaponTypeID(data.type_id, "AmplifierTest", 0, CombatAffinity::kNone, "Original");
        VerifyWeaponTypeID(data.amplifier_for_weapon_type_id, "FlamewardenStaff", 0, CombatAffinity::kFire, "Original");

        EXPECT_EQ(data.tier, 0);
        EXPECT_EQ(data.combat_affinity, CombatAffinity::kNone);
        EXPECT_EQ(data.dominant_combat_affinity, CombatAffinity::kNone);
        EXPECT_EQ(data.combat_class, CombatClass::kNone);
        EXPECT_EQ(data.dominant_combat_class, CombatClass::kNone);

        VerifyDefaultStats(data.stats);

        VerifyAbilityList(data.attack_abilities, 1, AbilityUpdateType::kReplace);
        VerifyAbilityList(data.omega_abilities, 1, AbilityUpdateType::kReplace);
        VerifyAbilityList(data.innate_abilities, 1, AbilityUpdateType::kAdd);
    }
}

TEST_F(WeaponTestJSON, AmplifierCustom)
{
    // Regular valid scenario
    {
        const char* weapon_json_raw = R"(
{
    "Name": "AmplifierTest",
    "Stage": 0,
    "Variation": "Original",
    "Type": "Amplifier",
    "Tier": 0,
    "AmplifierForWeapon": {
       "Name": "FlamewardenStaff",
       "Stage": 0,
       "Variation": "Original",
       "CombatAffinity": "Fire"
    },
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
    "AttackAbilities": [],
    "OmegaAbilitiesSelection": "Cycle",
    "OmegaAbilities": [
        {
            "Name": "Flip OmegaAttack Ability",
            "UpdateType": "Replace",
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
    ],
    "InnateAbilities": [
       {
          "Name": "Zone Breaker",
            "UpdateType": "Replace",
          "ActivationTrigger": {
             "TriggerType": "OnActivateXAbilities",
             "AbilityTypes": [
                "Omega"
             ],
             "EveryX": true,
             "NumberOfAbilitiesActivated": 1
          },
          "TotalDurationMs": 0,
          "Skills": [
             {
                "Name": "ZoneBreaker_Ability",
                "Targeting": {
                   "Type": "CurrentFocus"
                },
                "Deployment": {
                   "Type": "Projectile",
                   "Guidance": [
                      "Ground",
                      "Airborne"
                   ],
                   "PreDeploymentDelayPercentage": 45
                },
                "Projectile": {
                   "SizeUnits": 3,
                   "IsHoming": true,
                   "SpeedSubUnits": 8000,
                   "IsBlockable": false,
                   "ApplyToAll": false
                },
                "PercentageOfAbilityDuration": 100,
                "EffectPackage": {
                   "Attributes": {
                      "Propagation": {
                         "PropagationType": "Splash",
                         "SplashRadiusUnits": 30,
                         "DeploymentGuidance": [
                            "Ground",
                            "Airborne",
                            "Underground"
                         ],
                         "EffectPackage": {
                            "Effects": []
                         }
                      }
                   },
                   "Effects": []
                }
             }
          ]
       }
    ]
}
)";

        auto maybe_data = loader->ParseAndLoadWeapon(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());

        const CombatUnitWeaponData& data = *maybe_data;

        VerifyWeaponTypeID(data.type_id, "AmplifierTest", 0, CombatAffinity::kNone, "Original");
        VerifyWeaponTypeID(data.amplifier_for_weapon_type_id, "FlamewardenStaff", 0, CombatAffinity::kFire, "Original");

        EXPECT_EQ(data.tier, 0);
        EXPECT_EQ(data.combat_affinity, CombatAffinity::kNone);
        EXPECT_EQ(data.dominant_combat_affinity, CombatAffinity::kNone);
        EXPECT_EQ(data.combat_class, CombatClass::kNone);
        EXPECT_EQ(data.dominant_combat_class, CombatClass::kNone);

        VerifyDefaultStats(data.stats);

        VerifyAbilityList(data.attack_abilities, 0);  // No specific update type for empty list
        VerifyAbilityList(data.omega_abilities, 1, AbilityUpdateType::kReplace);
        VerifyAbilityList(data.innate_abilities, 1, AbilityUpdateType::kReplace);
    }
}

TEST_F(WeaponTestJSON, TypeID)
{
    // Perfectly valid case
    {
        const char* weapon_json_raw = R"({
            "Name": "Pistol",
            "Stage": 2,
            "CombatAffinity": "Water",
            "Variation": "yo"
        })";

        auto maybe_data = loader->ParseAndLoadWeaponTypeID(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());
        VerifyWeaponTypeID(*maybe_data, "Pistol", 2, CombatAffinity::kWater, "yo");
    }

    // Name must be present
    {
        const char* weapon_json_raw = R"({
            "Stage": 2,
            "CombatAffinity": "Water",
            "Variation": "yo"
        })";

        auto maybe_data = silent_loader->ParseAndLoadWeaponTypeID(weapon_json_raw);
        ASSERT_FALSE(maybe_data.has_value());
    }

    // Affinity can be none
    {
        const char* weapon_json_raw = R"({
            "Name": "Pistol",
            "Stage": 2,
            "CombatAffinity": "",
            "Variation": "yo"
        })";

        auto maybe_data = loader->ParseAndLoadWeaponTypeID(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());
        VerifyWeaponTypeID(*maybe_data, "Pistol", 2, CombatAffinity::kNone, "yo");
    }

    // Affinity must not be present
    {
        const char* weapon_json_raw = R"({
            "Name": "Pistol",
            "Stage": 2,
            "Variation": "yo"
        })";

        auto maybe_data = silent_loader->ParseAndLoadWeaponTypeID(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());
    }

    // Stage must be present
    {
        const char* weapon_json_raw = R"({
            "Name": "Pistol",
            "CombatAffinity": "",
            "Variation": "yo"
        })";

        auto maybe_data = silent_loader->ParseAndLoadWeaponTypeID(weapon_json_raw);
        ASSERT_FALSE(maybe_data.has_value());
    }

    // Variation can be empty
    {
        const char* weapon_json_raw = R"({
            "Name": "Pistol",
            "Stage": 2,
            "CombatAffinity": "Water",
            "Variation": ""
        })";

        auto maybe_data =
            loader->ParseAndLoadWeaponTypeID(weapon_json_raw);  // Changed to loader as per original successful parse
        ASSERT_TRUE(maybe_data.has_value());
        VerifyWeaponTypeID(*maybe_data, "Pistol", 2, CombatAffinity::kWater, "");
    }

    // Variation is optional field
    {
        const char* weapon_json_raw = R"({
            "Name": "Pistol",
            "Stage": 2,
            "CombatAffinity": "Water"
        })";

        auto maybe_data =
            loader->ParseAndLoadWeaponTypeID(weapon_json_raw);  // Changed to loader as per original successful parse
        ASSERT_TRUE(maybe_data.has_value());
        VerifyWeaponTypeID(*maybe_data, "Pistol", 2, CombatAffinity::kWater, "");
    }
}

TEST_F(WeaponTestJSON, Instance)
{
    // Perfectly valid case
    {
        const char* weapon_json_raw = R"({
          "ID": "Whatever",
          "TypeID":
          {
             "Name": "Pistol",
             "Stage": 2,
             "Variation": "Original",
             "CombatAffinity": "Fire"
          }
        })";

        auto maybe_data = loader->ParseAndLoadWeaponInstanceData(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());

        const auto& instance_data = *maybe_data;
        EXPECT_EQ(instance_data.id, "Whatever");
        VerifyWeaponTypeID(instance_data.type_id, "Pistol", 2, CombatAffinity::kFire, "Original");
        EXPECT_EQ(instance_data.equipped_amplifiers.size(), 0);
    }

    // Missing ID field
    {
        const char* weapon_json_raw = R"({
          "TypeID":
          {
             "Name": "Pistol",
             "Stage": 2,
             "Variation": "Original",
             "CombatAffinity": "Fire"
          }
        })";
        // This test originally ASSERT_TRUE with silent_loader, implying parsing succeeds even if ID field is missing.
        // The value of instance_data.id is not checked in the original for this specific sub-case.
        auto maybe_data = silent_loader->ParseAndLoadWeaponInstanceData(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());
        if (maybe_data.has_value())
        {  // Additional check to be safe before dereferencing
            const auto& instance_data = *maybe_data;
            // If ID defaults to empty or some other value, it could be checked here.
            // For now, sticking to original's scope for this sub-test.
            VerifyWeaponTypeID(instance_data.type_id, "Pistol", 2, CombatAffinity::kFire, "Original");
        }
    }

    // Empty ID field
    {
        const char* weapon_json_raw = R"({
            "ID": "",
          "TypeID":
          {
             "Name": "Pistol",
             "Stage": 2,
             "Variation": "Original",
             "CombatAffinity": "Fire"
          }
        })";

        auto maybe_data = silent_loader->ParseAndLoadWeaponInstanceData(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());

        const auto& instance_data = *maybe_data;
        EXPECT_EQ(instance_data.id, "");
        VerifyWeaponTypeID(instance_data.type_id, "Pistol", 2, CombatAffinity::kFire, "Original");
        EXPECT_EQ(instance_data.equipped_amplifiers.size(), 0);
    }

    // Empty list
    {
        const char* weapon_json_raw = R"({
          "ID": "Whatever",
          "TypeID":
          {
             "Name": "Pistol",
             "Stage": 2,
             "Variation": "Original",
             "CombatAffinity": "Fire"
          },
            "EquippedAmplifiers": []
        })";

        auto maybe_data = loader->ParseAndLoadWeaponInstanceData(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());

        const auto& instance_data = *maybe_data;
        EXPECT_EQ(instance_data.id, "Whatever");
        VerifyWeaponTypeID(instance_data.type_id, "Pistol", 2, CombatAffinity::kFire, "Original");
        EXPECT_EQ(instance_data.equipped_amplifiers.size(), 0);
    }

    // One amplifier
    {
        const char* weapon_json_raw = R"({
          "ID": "Whatever",
          "TypeID":
          {
             "Name": "Pistol",
             "Stage": 2,
             "Variation": "Original",
             "CombatAffinity": "Fire"
          },
            "EquippedAmplifiers": [
                {
                  "ID": "Amplifier Id",
                  "TypeID":
                  {
                        "Name": "FlamewardenStaffZonebreaker",
                        "Stage": 0,
                        "Variation": "Original",
                        "Type": "Amplifier"
                  }
                }
            ]
        })";
        // Note: "CombatAffinity" is missing in amplifier's TypeID, so it should default to kNone.
        // The "Type": "Amplifier" field within TypeID is unusual but present in original.

        auto maybe_data = loader->ParseAndLoadWeaponInstanceData(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());

        const auto& instance_data = *maybe_data;
        EXPECT_EQ(instance_data.id, "Whatever");
        VerifyWeaponTypeID(instance_data.type_id, "Pistol", 2, CombatAffinity::kFire, "Original");

        ASSERT_EQ(instance_data.equipped_amplifiers.size(), 1);
        const auto& first_amplifier = instance_data.equipped_amplifiers.front();
        EXPECT_EQ(first_amplifier.id, "Amplifier Id");
        VerifyWeaponTypeID(
            first_amplifier.type_id,
            "FlamewardenStaffZonebreaker",
            0,
            CombatAffinity::kNone,
            "Original");
    }

    // Two amplifiers
    {
        const char* weapon_json_raw = R"({
          "ID": "Whatever",
          "TypeID":
          {
             "Name": "Pistol",
             "Stage": 2,
             "Variation": "Original",
             "CombatAffinity": "Fire"
          },
            "EquippedAmplifiers": [
                {
                  "ID": "Amplifier Id",
                  "TypeID":
                  {
                        "Name": "FlamewardenStaffZonebreaker",
                        "Stage": 0,
                        "Variation": "Original"
                  }
                },
                {
                  "ID": "Amplifier Id 2",
                  "TypeID":
                  {
                        "Name": "FlamewardenStaffExecutionSurge",
                        "Stage": 0,
                        "Variation": "Original"
                  }
                }
            ]
        })";
        // Note: "CombatAffinity" is missing in amplifiers' TypeIDs, should default to kNone.

        auto maybe_data = loader->ParseAndLoadWeaponInstanceData(weapon_json_raw);
        ASSERT_TRUE(maybe_data.has_value());

        const auto& instance_data = *maybe_data;
        EXPECT_EQ(instance_data.id, "Whatever");
        VerifyWeaponTypeID(instance_data.type_id, "Pistol", 2, CombatAffinity::kFire, "Original");

        ASSERT_EQ(instance_data.equipped_amplifiers.size(), 2);

        const auto& first_amplifier = instance_data.equipped_amplifiers[0];
        EXPECT_EQ(first_amplifier.id, "Amplifier Id");
        VerifyWeaponTypeID(
            first_amplifier.type_id,
            "FlamewardenStaffZonebreaker",
            0,
            CombatAffinity::kNone,
            "Original");

        const auto& second_amplifier = instance_data.equipped_amplifiers[1];
        EXPECT_EQ(second_amplifier.id, "Amplifier Id 2");
        VerifyWeaponTypeID(
            second_amplifier.type_id,
            "FlamewardenStaffExecutionSurge",
            0,
            CombatAffinity::kNone,
            "Original");
    }
}

}  // namespace simulation