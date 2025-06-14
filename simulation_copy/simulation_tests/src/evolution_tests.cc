#include "base_test_fixtures.h"
#include "components/combat_synergy_component.h"
#include "utility/evolution_helper.h"

namespace simulation
{
class EvolutionTest : public BaseTest
{
public:
    using Super = BaseTest;

    bool UseSynergiesData() const override
    {
        return true;
    }

    bool UseMaxStagePlacementRadius() const override
    {
        return true;
    }

    std::shared_ptr<CombatUnitData> MakeCombatUnitData()
    {
        auto unit_data_ptr = std::make_shared<CombatUnitData>();
        auto& unit_data = *unit_data_ptr;
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

        return unit_data_ptr;
    }

    const CombatUnitTypeID& RegisterUnitData(std::string line, const int stage, std::string path, const int radius)
    {
        auto data_ptr = MakeCombatUnitData();
        data_ptr->type_id.line_name = std::move(line);
        data_ptr->type_id.stage = stage;
        data_ptr->type_id.path = std::move(path);
        data_ptr->radius_units = radius;
        game_data_container_->AddCombatUnitData(data_ptr->type_id, data_ptr);
        return data_ptr->type_id;
    }

    const std::optional<int> FindMaxPlacementRadius(const CombatUnitTypeID& type_id) const
    {
        int radius = -1;
        if (EvolutionHelper::FindMaximumPossibleRadiusInEvolutionGraph(
                game_data_container_->GetCombatUnitsDataContainer(),
                type_id,
                &radius))
        {
            return radius;
        }

        return std::nullopt;
    }
};

TEST_F(EvolutionTest, PlacementRadius_Simple)
{
    std::vector<CombatUnitTypeID> units;

    units.push_back(RegisterUnitData("Cat", 1, "Meow", 1));
    ASSERT_EQ(FindMaxPlacementRadius(units[0]), 1);

    units.push_back(RegisterUnitData("Cat", 2, "Meow", 2));
    ASSERT_EQ(FindMaxPlacementRadius(units[0]), 2);
    ASSERT_EQ(FindMaxPlacementRadius(units[1]), 2);

    units.push_back(RegisterUnitData("Cat", 3, "Meow", 3));
    ASSERT_EQ(FindMaxPlacementRadius(units[0]), 3);
    ASSERT_EQ(FindMaxPlacementRadius(units[1]), 3);
    ASSERT_EQ(FindMaxPlacementRadius(units[2]), 3);
}

TEST_F(EvolutionTest, PlacementRadius_DifferentLines)
{
    std::vector<CombatUnitTypeID> units;
    units.push_back(RegisterUnitData("Cat", 1, "", 1));
    units.push_back(RegisterUnitData("Cat", 2, "", 2));
    units.push_back(RegisterUnitData("Cat", 3, "", 3));
    units.push_back(RegisterUnitData("Dog", 3, "", 5));
    ASSERT_EQ(FindMaxPlacementRadius(units[0]), 3);
    ASSERT_EQ(FindMaxPlacementRadius(units[1]), 3);
    ASSERT_EQ(FindMaxPlacementRadius(units[2]), 3);
    ASSERT_EQ(FindMaxPlacementRadius(units[3]), 5);
}

TEST_F(EvolutionTest, PlacementRadius_DifferentPaths)
{
    std::vector<CombatUnitTypeID> units;
    units.push_back(RegisterUnitData("Cat", 1, "Meow", 1));
    units.push_back(RegisterUnitData("Cat", 2, "Meow", 2));
    units.push_back(RegisterUnitData("Cat", 3, "Meow", 3));
    units.push_back(RegisterUnitData("Cat", 4, "Roar", 5));
    ASSERT_EQ(FindMaxPlacementRadius(units[0]), 3);
    ASSERT_EQ(FindMaxPlacementRadius(units[1]), 3);
    ASSERT_EQ(FindMaxPlacementRadius(units[2]), 3);
    ASSERT_EQ(FindMaxPlacementRadius(units[3]), 5);
}

TEST_F(EvolutionTest, PlacementRadius_DifferentPaths_Lynx)
{
    std::vector<CombatUnitTypeID> units;
    units.push_back(RegisterUnitData("Lynx", 1, "Meow", 1));
    units.push_back(RegisterUnitData("Lynx", 2, "Meow", 2));
    units.push_back(RegisterUnitData("Lynx", 3, "Meow", 3));
    units.push_back(RegisterUnitData("Lynx", 4, "Roar", 5));
    ASSERT_EQ(FindMaxPlacementRadius(units[0]), 5);
    ASSERT_EQ(FindMaxPlacementRadius(units[1]), 5);
    ASSERT_EQ(FindMaxPlacementRadius(units[2]), 5);
    ASSERT_EQ(FindMaxPlacementRadius(units[3]), 5);
}

TEST_F(EvolutionTest, Intergation)
{
    const std::vector<CombatUnitTypeID> units{
        RegisterUnitData("Cat", 1, "", 1),
        RegisterUnitData("Cat", 2, "", 2),
        RegisterUnitData("Cat", 3, "", 3),
    };

    auto spawn = [&](const CombatUnitTypeID& type_id, const HexGridPosition& position, const bool log_error = false)
    {
        std::string error_message;

        FullCombatUnitData full_data;
        full_data.instance = CreateCombatUnitInstanceData();
        full_data.instance.team = Team::kRed;
        full_data.instance.position = position;
        full_data.data = *game_data_container_->GetCombatUnitsDataContainer().Find(type_id);
        const auto entity = EntityFactory::SpawnCombatUnit(*world, full_data, kInvalidEntityID, &error_message);

        if (log_error && !entity) world->LogErr("Failed to spawn entity. Details: {}", error_message);

        return entity;
    };

    // Test placement radius agains middle line
    ASSERT_EQ(spawn(units[0], {0, 3}), nullptr);
    ASSERT_EQ(spawn(units[1], {0, 3}), nullptr);
    ASSERT_NE(spawn(units[0], {0, 4}, true), nullptr);

    // Test placement radius agains another combat unit
    ASSERT_EQ(spawn(units[0], {0, 10}), nullptr);
    ASSERT_NE(spawn(units[0], {0, 11}), nullptr);

    world->TimeStep();
}

TEST_F(EvolutionTest, KeepDominantWhileBonding)
{
    auto fake_stages = [this](const FullCombatUnitData& full_data)
    {
        // Add data about 3 stages (for evolution placement)
        for (int stage = 1; stage != 4; ++stage)
        {
            auto stage_data = std::make_shared<CombatUnitData>(full_data.data);
            stage_data->type_id.stage = stage;
            game_data_container_->AddCombatUnitData(stage_data->type_id, stage_data);
        }
    };

    constexpr HexGridPosition red_illuvial_pos{10, 10};
    constexpr HexGridPosition blue_illuvial_pos = Reflect(red_illuvial_pos);
    constexpr HexGridPosition blue_ranger_pos = blue_illuvial_pos + HexGridPosition{1, 0};
    const std::string blue_illuvial_id = GenerateRandomUniqueID();
    Entity* red_illuvial = nullptr;
    Entity* blue_illuvial = nullptr;
    Entity* blue_ranger = nullptr;

    // Illuvial (red)
    {
        FullCombatUnitData red_illuvial_data;
        red_illuvial_data.data.radius_units = 0;
        red_illuvial_data.data.type_id.type = CombatUnitType::kIlluvial;
        red_illuvial_data.data.type_id.line_name = "Red Illuvial";
        red_illuvial_data.data.type_id.stage = 1;
        red_illuvial_data.data.type_data.combat_class = CombatClass::kPhantom;
        red_illuvial_data.data.type_data.combat_affinity = CombatAffinity::kInferno;
        red_illuvial_data.instance.id = GenerateRandomUniqueID();
        red_illuvial_data.instance.team = Team::kRed;
        red_illuvial_data.instance.position = red_illuvial_pos;
        red_illuvial_data.instance.dominant_combat_class = CombatClass::kRogue;
        red_illuvial_data.instance.dominant_combat_affinity = CombatAffinity::kFire;
        fake_stages(red_illuvial_data);

        SpawnCombatUnit(*world, red_illuvial_data, red_illuvial);
        ASSERT_NE(red_illuvial, nullptr);

        const auto& synergies_component = red_illuvial->Get<CombatSynergyComponent>();
        ASSERT_EQ(synergies_component.GetCombatAffinity(), CombatAffinity::kInferno);
        ASSERT_EQ(synergies_component.GetCombatClass(), CombatClass::kPhantom);
        ASSERT_EQ(synergies_component.GetDominantCombatAffinity(), CombatAffinity::kFire);
        ASSERT_EQ(synergies_component.GetDominantCombatClass(), CombatClass::kRogue);
    }

    // Illuvial (blue)
    {
        FullCombatUnitData blue_illuvial_data;
        blue_illuvial_data.data.radius_units = 0;
        blue_illuvial_data.data.type_id.type = CombatUnitType::kIlluvial;
        blue_illuvial_data.data.type_id.line_name = "Blue Illuvial";
        blue_illuvial_data.data.type_id.stage = 1;
        blue_illuvial_data.data.type_data.combat_class = CombatClass::kPhantom;
        blue_illuvial_data.data.type_data.combat_affinity = CombatAffinity::kInferno;
        blue_illuvial_data.instance.id = blue_illuvial_id;
        blue_illuvial_data.instance.team = Team::kBlue;
        blue_illuvial_data.instance.position = blue_illuvial_pos;
        blue_illuvial_data.instance.dominant_combat_class = CombatClass::kRogue;
        blue_illuvial_data.instance.dominant_combat_affinity = CombatAffinity::kFire;
        fake_stages(blue_illuvial_data);

        SpawnCombatUnit(*world, blue_illuvial_data, blue_illuvial);
        ASSERT_NE(blue_illuvial, nullptr);

        const auto& synergies_component = blue_illuvial->Get<CombatSynergyComponent>();
        ASSERT_EQ(synergies_component.GetCombatAffinity(), CombatAffinity::kInferno);
        ASSERT_EQ(synergies_component.GetCombatClass(), CombatClass::kPhantom);
        ASSERT_EQ(synergies_component.GetDominantCombatAffinity(), CombatAffinity::kFire);
        ASSERT_EQ(synergies_component.GetDominantCombatClass(), CombatClass::kRogue);
    }

    // Ranger (blue)
    {
        // Add weapon data
        CombatWeaponTypeID weapon_id;
        weapon_id.name = "Weapon";
        {
            auto weapon_data = std::make_shared<CombatUnitWeaponData>();
            weapon_data->type_id = weapon_id;
            weapon_data->combat_affinity = CombatAffinity::kFire;
            weapon_data->dominant_combat_affinity = CombatAffinity::kFire;
            weapon_data->combat_class = CombatClass::kRogue;
            weapon_data->dominant_combat_class = CombatClass::kRogue;
            weapon_data->weapon_type = WeaponType::kNormal;
            game_data_container_->AddWeaponData(weapon_id, weapon_data);
        }

        // Add suit data
        CombatSuitTypeID suit_id;
        suit_id.name = "Suit";
        {
            auto suit_data = std::make_shared<CombatUnitSuitData>();
            suit_data->type_id = suit_id;
            game_data_container_->AddSuitData(suit_id, suit_data);
        }

        FullCombatUnitData blue_ranger_data;
        blue_ranger_data.data.radius_units = 0;
        blue_ranger_data.data.type_id.type = CombatUnitType::kRanger;
        blue_ranger_data.data.type_id.line_name = "Blue Ranger";
        blue_ranger_data.data.type_data.combat_affinity = CombatAffinity::kNone;
        blue_ranger_data.data.type_data.combat_class = CombatClass::kNone;
        blue_ranger_data.instance.id = GenerateRandomUniqueID();
        blue_ranger_data.instance.team = Team::kBlue;
        blue_ranger_data.instance.bonded_id = blue_illuvial_id;
        blue_ranger_data.instance.position = blue_ranger_pos;
        blue_ranger_data.instance.dominant_combat_affinity = CombatAffinity::kNone;
        blue_ranger_data.instance.dominant_combat_class = CombatClass::kNone;
        blue_ranger_data.instance.equipped_weapon.type_id = weapon_id;
        blue_ranger_data.instance.equipped_weapon.id = GenerateRandomUniqueID();
        blue_ranger_data.instance.equipped_suit.type_id = suit_id;
        blue_ranger_data.instance.equipped_suit.id = GenerateRandomUniqueID();

        SpawnCombatUnit(*world, blue_ranger_data, blue_ranger);
        ASSERT_NE(blue_ranger, nullptr);

        // Synergies come from weapon only before the first time step
        ASSERT_EQ(blue_ranger->Get<CombatSynergyComponent>().GetCombatAffinity(), CombatAffinity::kFire);
        ASSERT_EQ(blue_ranger->Get<CombatSynergyComponent>().GetCombatClass(), CombatClass::kRogue);
        ASSERT_EQ(blue_ranger->Get<CombatSynergyComponent>().GetDominantCombatAffinity(), CombatAffinity::kFire);
        ASSERT_EQ(blue_ranger->Get<CombatSynergyComponent>().GetDominantCombatClass(), CombatClass::kRogue);
    }

    world->TimeStep();

    ASSERT_EQ(blue_ranger->Get<CombatSynergyComponent>().GetCombatAffinity(), CombatAffinity::kInferno);
    ASSERT_EQ(blue_ranger->Get<CombatSynergyComponent>().GetCombatClass(), CombatClass::kPredator);
    ASSERT_EQ(blue_ranger->Get<CombatSynergyComponent>().GetDominantCombatAffinity(), CombatAffinity::kFire);
    ASSERT_EQ(blue_ranger->Get<CombatSynergyComponent>().GetDominantCombatClass(), CombatClass::kRogue);

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    auto synergy_stacks = [&](const Team team, CombatSynergyBonus find_synergy)
    {
        int count = 0;
        for (const auto& synergy : synergies_state_container.GetTeamAllSynergies(team, true))
        {
            if (synergy.combat_synergy == find_synergy)
            {
                count += synergy.synergy_stacks;
            }
        }

        return count;
    };

    auto total_synergy_stacks = [&](const Team team)
    {
        int sum = 0;
        for (const auto& synergy : synergies_state_container.GetTeamAllSynergies(team, true))
        {
            sum += synergy.synergy_stacks;
        }

        return sum;
    };

    ASSERT_EQ(synergy_stacks(Team::kRed, CombatAffinity::kFire), 1);
    ASSERT_EQ(synergy_stacks(Team::kRed, CombatAffinity::kInferno), 1);
    ASSERT_EQ(synergy_stacks(Team::kRed, CombatClass::kRogue), 1);
    ASSERT_EQ(synergy_stacks(Team::kRed, CombatClass::kPhantom), 1);
    ASSERT_EQ(total_synergy_stacks(Team::kRed), 4);

    ASSERT_EQ(synergy_stacks(Team::kBlue, CombatAffinity::kInferno), 2) << "One from illuvial. One from weapon + bond";
    ASSERT_EQ(synergy_stacks(Team::kBlue, CombatClass::kPhantom), 1) << "From illuvial";
    ASSERT_EQ(synergy_stacks(Team::kBlue, CombatClass::kPredator), 1) << "From ranger weapon plus bond.";
    ASSERT_EQ(synergy_stacks(Team::kBlue, CombatAffinity::kFire), 2) << "Keep dominant after bonding";
    ASSERT_EQ(synergy_stacks(Team::kBlue, CombatClass::kRogue), 2) << "Keep dominant after bonding";
    ASSERT_EQ(total_synergy_stacks(Team::kBlue), 8)
        << "2 from illuvial. 2 from ranger composites. 2 from rager dominants";
}
}  // namespace simulation
