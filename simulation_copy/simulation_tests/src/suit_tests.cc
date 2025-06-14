#include "base_json_test.h"
#include "utility/entity_helper.h"

namespace simulation
{

class SuitTest : public BaseJSONTest
{
public:
    using Super = BaseJSONTest;

    bool UseSynergiesData() const override
    {
        return true;
    }

    int GetNextId(const Team team)
    {
        if (team == Team::kRed)
        {
            return next_red_index_++;
        }

        return next_blue_index_++;
    }

    static HexGridPosition MakeFreePosition(const int index, const Team team)
    {
        HexGridPosition p{-60 + 3 * index, 6};

        if (team == Team::kBlue)
        {
            p = Reflect(p);
        }

        return p;
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
        data.type_data.stats.Set(StatType::kMaxHealth, 1000_fp);

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

    int next_red_index_ = 0;
    int next_blue_index_ = 0;
};

TEST_F(SuitTest, SuitWithAbility)
{
    CombatWeaponTypeID weapon{};
    {
        weapon.name = "Pistol";
        weapon.stage = 2;
        auto default_weapon_data = std::make_shared<CombatUnitWeaponData>();
        default_weapon_data->tier = 3;
        default_weapon_data->type_id = weapon;
        default_weapon_data->weapon_type = WeaponType::kNormal;
        game_data_container_->AddWeaponData(default_weapon_data->type_id, default_weapon_data);
    }

    auto spawn_ranger = [&](const Team team, const CombatSuitTypeID& suit) -> Entity&
    {
        FullCombatUnitData full_data = MakeGenericCombatUnitData("Ranger", team, CombatUnitType::kRanger);
        full_data.instance.equipped_suit.type_id = suit;
        full_data.instance.equipped_weapon.type_id = weapon;
        return SpawnWithFullData(full_data);
    };

    auto make_suit_with_pos_state = [&](const std::string_view suit_name, const EffectPositiveState positive_state)
    {
        CombatSuitTypeID suit_id(std::string(suit_name), 1, "");

        auto suit_data = std::make_shared<CombatUnitSuitData>();
        suit_data->type_id = suit_id;

        auto ability = AbilityData::Create();
        ability->activation_trigger_data.trigger_type = ActivationTriggerType::kOnBattleStart;
        ability->total_duration_ms = 0;

        auto& skill = ability->AddSkill();
        skill.percentage_of_ability_duration = 100;
        skill.targeting.type = SkillTargetingType::kSelf;
        skill.deployment.type = SkillDeploymentType::kDirect;
        skill.AddEffect(EffectData::CreatePositiveState(positive_state, 1000));

        suit_data->innate_abilities.emplace_back(ability);
        game_data_container_->AddSuitData(suit_data->type_id, suit_data);

        return suit_id;
    };

    const CombatSuitTypeID invulnerability_suit =
        make_suit_with_pos_state("Invulnerability", EffectPositiveState::kInvulnerable);
    const CombatSuitTypeID indomitability_suit =
        make_suit_with_pos_state("Indomitability", EffectPositiveState::kIndomitable);

    auto& blue = spawn_ranger(Team::kBlue, invulnerability_suit);
    ASSERT_FALSE(EntityHelper::HasPositiveState(blue, EffectPositiveState::kInvulnerable));

    auto& red = spawn_ranger(Team::kRed, indomitability_suit);
    ASSERT_FALSE(EntityHelper::HasPositiveState(red, EffectPositiveState::kIndomitable));

    world->TimeStep();
    world->TimeStep();
    ASSERT_TRUE(EntityHelper::HasPositiveState(blue, EffectPositiveState::kInvulnerable));
    ASSERT_TRUE(EntityHelper::HasPositiveState(red, EffectPositiveState::kIndomitable));
}

}  // namespace simulation
