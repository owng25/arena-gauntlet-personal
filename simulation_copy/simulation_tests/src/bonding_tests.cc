#include "base_test_fixtures.h"
#include "components/combat_unit_component.h"

namespace simulation
{
class BondingTest : public BaseTest
{
public:
    using Super = BaseTest;

    bool UseSynergiesData() const override
    {
        return true;
    }

    void SetUp() override
    {
        Super::SetUp();

        {
            default_suit_.name = "Suit";
            default_suit_.stage = 1;
            auto default_suit_data = std::make_shared<CombatUnitSuitData>();
            default_suit_data->type_id = default_suit_;
            game_data_container_->AddSuitData(default_suit_data->type_id, default_suit_data);
        }

        {
            default_weapon_.name = "Pistol";
            default_weapon_.stage = 2;
            auto default_weapon_data = std::make_shared<CombatUnitWeaponData>();
            default_weapon_data->tier = 3;
            default_weapon_data->type_id = default_weapon_;
            default_weapon_data->weapon_type = WeaponType::kNormal;
            game_data_container_->AddWeaponData(default_weapon_data->type_id, default_weapon_data);
        }

        // Make weapon for each affinity
        for (const auto affinity : EnumSet<CombatAffinity>::MakeFull())
        {
            auto weapon_data = std::make_shared<CombatUnitWeaponData>();
            weapon_data->type_id.name = fmt::format("Pistol_{}", affinity);
            weapon_data->weapon_type = WeaponType::kNormal;
            weapon_data->tier = 3;
            weapon_data->type_id.stage = 2;
            weapon_data->combat_affinity = affinity;
            weapon_data->dominant_combat_affinity = affinity;
            game_data_container_->AddWeaponData(weapon_data->type_id, weapon_data);
            affinity_to_weapon_[affinity] = weapon_data->type_id;
        }

        // Make weapon for each combat class
        for (const auto combat_class : EnumSet<CombatClass>::MakeFull())
        {
            auto weapon_data = std::make_shared<CombatUnitWeaponData>();
            weapon_data->type_id.name = fmt::format("Pistol_{}", combat_class);
            weapon_data->tier = 3;
            weapon_data->type_id.stage = 2;
            weapon_data->combat_class = combat_class;
            weapon_data->dominant_combat_class = combat_class;
            weapon_data->weapon_type = WeaponType::kNormal;
            game_data_container_->AddWeaponData(weapon_data->type_id, weapon_data);
            combat_class_to_weapon_[combat_class] = weapon_data->type_id;
        }
    }

    int GetNextId(const Team team)
    {
        if (team == Team::kRed)
        {
            return next_red_index_++;
        }

        return next_blue_index_++;
    }

    static HexGridPosition MakeFreePosition(int index, const Team team)
    {
        int q = 3 * index - 50, r = -53;

        if (team == Team::kRed)
        {
            r = -r;
        }

        return HexGridPosition{q, r};
    }

    Entity& SpawnWithFullData(const FullCombatUnitData& full_data)
    {
        Entity* entity = nullptr;
        SpawnCombatUnit(*world, full_data, entity);
        return *entity;
    }

    FullCombatUnitData
    MakeGenericCombatUnitData(const std::string_view line_name, const Team team, CombatUnitType combat_unit_type)
    {
        const int index = GetNextId(team);

        FullCombatUnitData full_data{};
        CombatUnitData& data = full_data.data;
        data = CreateCombatUnitData();
        data.type_id = CombatUnitTypeID(std::string(line_name), 1);
        data.type_id.type = combat_unit_type;

        CombatUnitInstanceData& instance = full_data.instance;
        instance = CreateCombatUnitInstanceData();
        instance.id = fmt::format("{}_{}", team, index);
        instance.team = team;
        instance.position = MakeFreePosition(index, team);

        return full_data;
    }

    Entity& SpawnIlluvial(
        const Team team,
        const CombatAffinity combat_affinity,
        const CombatClass combat_class,
        const CombatAffinity dominant_combat_affinity = CombatAffinity::kNone,
        const CombatClass dominant_combat_class = CombatClass::kNone)
    {
        FullCombatUnitData full_data = MakeGenericCombatUnitData("Illuvial", team, CombatUnitType::kIlluvial);
        full_data.data.type_data.combat_affinity = combat_affinity;
        full_data.data.type_data.combat_class = combat_class;

        full_data.instance.dominant_combat_affinity = dominant_combat_affinity;
        full_data.instance.dominant_combat_class = dominant_combat_class;
        return SpawnWithFullData(full_data);
    }

    Entity& SpawnRanger(
        const Team team,
        const Entity* bond_with_entity,
        const CombatAffinity affinity_from_item = CombatAffinity::kNone,
        const CombatClass combat_class_from_item = CombatClass::kNone)
    {
        FullCombatUnitData full_data = MakeGenericCombatUnitData("Ranger", team, CombatUnitType::kRanger);
        if (bond_with_entity)
        {
            full_data.instance.bonded_id = bond_with_entity->Get<CombatUnitComponent>().GetUniqueID();
        }

        const bool add_affinity_weapon = affinity_from_item != CombatAffinity::kNone;
        const bool add_combat_class_weapon = combat_class_from_item != CombatClass::kNone;

        assert(!(add_affinity_weapon && add_combat_class_weapon));

        full_data.instance.equipped_suit.type_id = default_suit_;
        full_data.instance.equipped_weapon.type_id = default_weapon_;

        if (add_combat_class_weapon)
        {
            full_data.instance.equipped_weapon.type_id = combat_class_to_weapon_[combat_class_from_item];
        }

        if (add_affinity_weapon)
        {
            full_data.instance.equipped_weapon.type_id = affinity_to_weapon_[affinity_from_item];
        }

        full_data.instance.equipped_suit.type_id = default_suit_;

        return SpawnWithFullData(full_data);
    }

    bool HasAffinity(const Entity& entity, const CombatAffinity combat_affinity) const
    {
        return world->GetSynergiesHelper().HasCombatAffinity(entity, combat_affinity);
    }

    bool HasClass(const Entity& entity, const CombatClass combat_class) const
    {
        return world->GetSynergiesHelper().HasCombatClass(entity, combat_class);
    }

    int next_red_index_ = 0;
    int next_blue_index_ = 0;
    CombatSuitTypeID default_suit_{};
    CombatWeaponTypeID default_weapon_{};
    std::unordered_map<CombatAffinity, CombatWeaponTypeID> affinity_to_weapon_;
    std::unordered_map<CombatClass, CombatWeaponTypeID> combat_class_to_weapon_;
};

TEST_F(BondingTest, Simple)
{
    const auto& helper = world->GetSynergiesHelper();

    EXPECT_EQ(helper.Bond(CombatClass::kFighter, CombatClass::kNone), CombatClass::kFighter);
    EXPECT_EQ(helper.Bond(CombatClass::kNone, CombatClass::kFighter), CombatClass::kFighter);
    EXPECT_EQ(helper.Bond(CombatClass::kFighter, CombatClass::kEmpath), CombatClass::kTemplar);
    EXPECT_EQ(helper.Bond(CombatClass::kEmpath, CombatClass::kFighter), CombatClass::kTemplar);
    EXPECT_EQ(helper.Bond(CombatClass::kPsion, CombatClass::kRogue), CombatClass::kPhantom);

    EXPECT_EQ(helper.Bond(CombatAffinity::kWater, CombatAffinity::kNone), CombatAffinity::kWater);
    EXPECT_EQ(helper.Bond(CombatAffinity::kNone, CombatAffinity::kWater), CombatAffinity::kWater);
    EXPECT_EQ(helper.Bond(CombatAffinity::kWater, CombatAffinity::kAir), CombatAffinity::kFrost);
    EXPECT_EQ(helper.Bond(CombatAffinity::kAir, CombatAffinity::kWater), CombatAffinity::kFrost);
    EXPECT_EQ(helper.Bond(CombatAffinity::kNature, CombatAffinity::kAir), CombatAffinity::kSpore);

    EXPECT_EQ(helper.Bond(CombatAffinity::kWater, CombatAffinity::kWater), CombatAffinity::kTsunami);
    EXPECT_EQ(helper.Bond(CombatAffinity::kNature, CombatAffinity::kNature), CombatAffinity::kVerdant);
    EXPECT_EQ(helper.Bond(CombatAffinity::kEarth, CombatAffinity::kEarth), CombatAffinity::kGranite);
    EXPECT_EQ(helper.Bond(CombatAffinity::kAir, CombatAffinity::kAir), CombatAffinity::kTempest);
    EXPECT_EQ(helper.Bond(CombatAffinity::kFire, CombatAffinity::kFire), CombatAffinity::kInferno);

    EXPECT_EQ(helper.Bond(CombatClass::kFighter, CombatClass::kFighter), CombatClass::kBerserker);
    EXPECT_EQ(helper.Bond(CombatClass::kEmpath, CombatClass::kEmpath), CombatClass::kMystic);
    EXPECT_EQ(helper.Bond(CombatClass::kBulwark, CombatClass::kBulwark), CombatClass::kColossus);
    EXPECT_EQ(helper.Bond(CombatClass::kPsion, CombatClass::kPsion), CombatClass::kInvoker);
    EXPECT_EQ(helper.Bond(CombatClass::kRogue, CombatClass::kRogue), CombatClass::kPredator);
}

TEST_F(BondingTest, CombatAffinityBonding)
{
    Entity& illuvial = SpawnIlluvial(Team::kRed, CombatAffinity::kWater, CombatClass::kNone, CombatAffinity::kWater);
    Entity& ranger = SpawnRanger(Team::kRed, &illuvial);

    world->TimeStep();  // Updates bonds at battle start

    ASSERT_TRUE(HasAffinity(ranger, CombatAffinity::kWater));

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    const auto& synergies = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kRed);
    ASSERT_EQ(synergies.size(), 1);
    EXPECT_EQ(synergies[0].synergy_stacks, 2);
    EXPECT_EQ(synergies[0].combat_synergy, CombatAffinity::kWater);
    EXPECT_EQ(synergies[0].combat_units.size(), 2);
}

TEST_F(BondingTest, CompositeCombatAffinityBonding)
{
    Entity& illuvial = SpawnIlluvial(Team::kRed, CombatAffinity::kMud, CombatClass::kNone, CombatAffinity::kWater);
    Entity& ranger = SpawnRanger(Team::kRed, &illuvial, CombatAffinity::kFire);

    world->TimeStep();  // Updates bonds at battle start

    ASSERT_TRUE(HasAffinity(ranger, CombatAffinity::kSteam));

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    const auto& synergies = synergies_state_container.GetTeamAllCombatAffinitySynergies(Team::kRed);
    ASSERT_EQ(synergies.size(), 4);

    EXPECT_EQ(synergies[0].synergy_stacks, 1);
    EXPECT_EQ(synergies[0].combat_synergy, CombatAffinity::kSteam);
    EXPECT_EQ(synergies[0].combat_units.size(), 1);

    EXPECT_EQ(synergies[1].synergy_stacks, 1);
    EXPECT_EQ(synergies[1].combat_synergy, CombatAffinity::kMud);
    EXPECT_EQ(synergies[1].combat_units.size(), 1);

    // Should have dominants
    EXPECT_EQ(synergies[2].synergy_stacks, 1);
    EXPECT_EQ(synergies[2].combat_synergy, CombatAffinity::kFire);
    EXPECT_EQ(synergies[2].combat_units.size(), 1);

    EXPECT_EQ(synergies[3].synergy_stacks, 1);
    EXPECT_EQ(synergies[3].combat_synergy, CombatAffinity::kWater);
    EXPECT_EQ(synergies[3].combat_units.size(), 1);
}

TEST_F(BondingTest, CombatClassBonding)
{
    Entity& illuvial = SpawnIlluvial(
        Team::kRed,
        CombatAffinity::kNone,
        CombatClass::kFighter,
        CombatAffinity::kWater,
        CombatClass::kFighter);
    Entity& ranger = SpawnRanger(Team::kRed, &illuvial);

    world->TimeStep();  // Updates bonds at battle start

    ASSERT_TRUE(HasClass(ranger, CombatClass::kFighter));

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    const auto& synergies = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kRed);
    ASSERT_EQ(synergies.size(), 1);
    EXPECT_EQ(synergies[0].synergy_stacks, 2);
    EXPECT_EQ(synergies[0].combat_synergy, CombatClass::kFighter);
    EXPECT_EQ(synergies[0].combat_units.size(), 2);
}

TEST_F(BondingTest, CompositeCombatClassBonding)
{
    Entity& illuvial = SpawnIlluvial(
        Team::kRed,
        CombatAffinity::kNone,
        CombatClass::kArcanite,
        CombatAffinity::kNone,
        CombatClass::kFighter);
    Entity& ranger = SpawnRanger(Team::kRed, &illuvial, CombatAffinity::kNone, CombatClass::kEmpath);

    world->TimeStep();  // Updates bonds at battle start

    ASSERT_TRUE(HasClass(ranger, CombatClass::kTemplar));

    const SynergiesStateContainer& synergies_state_container = world->GetSynergiesStateContainer();
    const auto& synergies = synergies_state_container.GetTeamAllCombatClassSynergies(Team::kRed);
    ASSERT_EQ(synergies.size(), 4);

    EXPECT_EQ(synergies[0].synergy_stacks, 1);
    EXPECT_EQ(synergies[0].combat_synergy, CombatClass::kTemplar);
    EXPECT_EQ(synergies[0].combat_units.size(), 1);

    EXPECT_EQ(synergies[1].synergy_stacks, 1);
    EXPECT_EQ(synergies[1].combat_synergy, CombatClass::kArcanite);
    EXPECT_EQ(synergies[1].combat_units.size(), 1);

    // Should have dominants
    EXPECT_EQ(synergies[2].synergy_stacks, 1);
    EXPECT_EQ(synergies[2].combat_synergy, CombatClass::kEmpath);
    EXPECT_EQ(synergies[2].combat_units.size(), 1);

    EXPECT_EQ(synergies[3].synergy_stacks, 1);
    EXPECT_EQ(synergies[3].combat_synergy, CombatClass::kFighter);
    EXPECT_EQ(synergies[3].combat_units.size(), 1);
}
}  // namespace simulation
