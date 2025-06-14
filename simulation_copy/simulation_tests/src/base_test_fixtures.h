#pragma once

#include <memory>
#include <type_traits>

#include "data/combat_unit_data.h"
#include "data/containers/game_data_container.h"
#include "ecs/event.h"
#include "ecs/world.h"
#include "factories/entity_factory.h"
#include "gtest/gtest.h"
#include "test_environment.h"
#include "utility/stats_helper.h"

#ifdef ENABLE_VISUALIZATION
#include "battle_visualization.h"

// Add this macro to the beginning of the unit test to see visualization
#define VISUALIZE_TEST_STEPS tool::BattleVisualization _viz(world, false, true);

// Uncomment VISUALIZE_ALL_TESTS to enable visualization of all unit tests
// #define VISUALIZE_ALL_TESTS

#endif  // ENABLE_VISUALIZATION

namespace simulation
{
// Helper base fixture test class that has some helpers to time step the world
class BaseTest : public ::testing::Test
{
public:
    // Reflects hex grid postion across the middle line
    static constexpr HexGridPosition Reflect(const HexGridPosition& position)
    {
        return HexGridPosition{position.q + position.r, -position.r};
    };

protected:
    static FixedPoint EvaluateNoStats(const EffectExpression& expression)
    {
        return expression.Evaluate(ExpressionEvaluationContext{}, ExpressionStatsSource{});
    }

    template <typename T>
    static void CreateDefaultIfNull(std::shared_ptr<T>& ptr)
    {
        if (!ptr)
        {
            ptr = TestEnvironment::DefaultContainer<T>();
        }
    }

    static std::shared_ptr<World> CreateWorld(
        WorldConfig& config,
        std::shared_ptr<GameDataContainer> game_data_container)
    {
        if (config.logger == nullptr)
        {
            config.logger = TestEnvironment::GetLogger();
        }

        if (!game_data_container)
        {
            game_data_container = TestEnvironment::DefaultGameData();
        }
        CreateDefaultIfNull(config.obstacles);

        return World::Create(config, std::move(game_data_container));
    }

    // Create CombatUnitData with default affinity and class
    static CombatUnitData CreateCombatUnitData()
    {
        CombatUnitData data;
        data.type_id.line_name = "default";
        data.type_data.combat_affinity = CombatAffinity::kWater;
        data.type_data.combat_class = CombatClass::kFighter;

        StatsHelper::SetDefaults(data.type_data.stats);
        return data;
    }

    static CombatUnitData CreateRangerCombatUnitData(const FixedPoint max_health)
    {
        CombatUnitData data;
        data.type_id.type = CombatUnitType::kRanger;
        data.type_id.line_name = "FemaleRanger";
        data.radius_units = 3;
        data.type_data.stats.Set(StatType::kMaxHealth, max_health);
        return data;
    }

    static std::string GenerateRandomString(RandomGenerator& random_generator, const size_t length)
    {
        auto generate_random_char = [&]()
        {
            static constexpr std::string_view charset =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
            return charset[random_generator.Range(0, charset.size())];
        };

        std::string str;
        std::generate_n(std::back_inserter(str), length, generate_random_char);
        return str;
    }

    std::string GenerateRandomUniqueID()
    {
        return GenerateRandomString(random_generator_, 3);
    }

    // Create a Combat Unit Instance without RNG
    CombatUnitInstanceData CreateCombatUnitInstanceData()
    {
        CombatUnitInstanceData instance;
        instance.id = GenerateRandomUniqueID();

        assert(!instance.id.empty());

        return instance;
    }

    // New format to spawn a combat unit
    static void SpawnCombatUnit(World& world, const FullCombatUnitData& full_data, Entity*& out_entity)
    {
        std::string error_message;
        out_entity = EntityFactory::SpawnCombatUnit(world, full_data, kInvalidEntityID, &error_message);

        if (out_entity == nullptr)
        {
            ASSERT_FALSE(true) << "Can't spawn for team = " << fmt::format("{}", full_data.instance.team)
                               << ", position = " << fmt::format("{}", full_data.instance.position)
                               << ", radius = " << full_data.data.radius_units << ", message = " << error_message;
        }
    }

    void Stun(Entity& entity, int duration = kTimeInfinite) const
    {
        const auto effect_data = EffectData::CreateNegativeState(EffectNegativeState::kStun, duration);
        EffectState effect_state{};
        effect_state.sender_stats = world->GetFullStats(entity.GetID());
        GetAttachedEffectsHelper().AddAttachedEffect(entity, entity.GetID(), effect_data, effect_state);
        ASSERT_TRUE(entity.Get<AttachedEffectsComponent>().HasNegativeState(EffectNegativeState::kStun));
    }

    virtual std::shared_ptr<GameDataContainer> CreateGameData()
    {
        auto game_data = std::make_shared<GameDataContainer>(TestEnvironment::GetLogger());

        game_data->SetHyperData(GetHyperData());
        game_data->SetWorldEffectsConfig(GetWorldEffectsConfig());
        game_data->SetHyperConfig(GetHyperConfig());

        return game_data;
    }

    const GridHelper& GetGridHelper() const
    {
        return world->GetGridHelper();
    }
    const HexGridConfig& GetGridConfig() const
    {
        return world->GetGridConfig();
    }
    const AttachedEffectsHelper& GetAttachedEffectsHelper() const
    {
        return world->GetAttachedEffectsHelper();
    }

    void SpawnCombatUnit(
        const Team team,
        const HexGridPosition& position,
        const CombatUnitData& combat_unit_data,
        Entity*& out_entity)
    {
        CombatUnitInstanceData instance = CreateCombatUnitInstanceData();
        instance.team = team;
        instance.position = position;
        SpawnCombatUnit(instance, combat_unit_data, out_entity);
    }

    // New format to spawn a combat unit
    void SpawnCombatUnit(const CombatUnitInstanceData& instance, const CombatUnitData& data, Entity*& out_entity)
    {
        FullCombatUnitData full_data;
        full_data.instance = instance;
        full_data.data = data;
        SpawnCombatUnit(*world, full_data, out_entity);
    }
    void SpawnCombatUnit(const FullCombatUnitData& full_data, Entity*& out_entity)
    {
        SpawnCombatUnit(*world, full_data, out_entity);
    }

    virtual std::unordered_map<Team, std::vector<EncounterModInstanceData>> GetTeamsEncounterMods() const
    {
        return {};
    }

    virtual void InitWorld()
    {
        game_data_container_ = CreateGameData();

        WorldConfig config;
        config.stats_constants_scale = GetWorldStatsConstantsScale();
        config.logs_config.enable_calculate_live_stats = IsEnabledLogsForCalculateLiveStats();
        config.logs_config.enable_attached_effects = true;
        config.battle_config.random_seed = GetWorldRandomSeed();
        config.battle_config.sort_by_unique_id = SortEntitiesByUniqueID();
        config.battle_config.teams_encounter_mods = GetTeamsEncounterMods();
        config.battle_config.use_max_stage_placement_radius = UseMaxStagePlacementRadius();
        config.logger = GetWorldLogger();
        config.max_attack_speed = GetWorldMaxAttackSpeed();

        // We use these energy values for tests
        config.base_energy_gain_per_attack = 10_fp;
        config.one_energy_gain_per_damage_taken = 20_fp;

        config.battle_config.overload_config = GetOverloadConfig();

        world = CreateWorld(config, game_data_container_);

#if (defined ENABLE_VISUALIZATION && defined VISUALIZE_ALL_TESTS)
        constexpr bool _exit_when_finished = false;
        constexpr bool _space_to_time_step = true;
        _tests_viz = std::make_unique<tool::BattleVisualization>(world, _exit_when_finished, _space_to_time_step);
#endif  // ENABLE_VISUALIZATION + VISUALIZE_ALL_TESTS

        PostInitWorld();
    }

    virtual void PostInitWorld() {}

    void SetUp() override
    {
        random_generator_.Init(0);
        InitWorld();
        FillGameDataContainer(*game_data_container_);
    }

    virtual FixedPoint GetWorldStatsConstantsScale() const
    {
        return 1_fp;
    }
    virtual FixedPoint GetWorldMaxAttackSpeed() const
    {
        return 2000_fp;
    }
    virtual uint64_t GetWorldRandomSeed() const
    {
        return 0;
    }
    virtual bool SortEntitiesByUniqueID() const
    {
        return false;
    }
    virtual std::shared_ptr<Logger> GetWorldLogger() const
    {
        return nullptr;
    }
    virtual bool IsEnabledLogsForCalculateLiveStats() const
    {
        return false;
    }

    // Helper method to add a class synergy data
    template <typename TCombatSynergy>
    void AddSynergyData(const TCombatSynergy combat_synergy, const std::vector<TCombatSynergy>& components)
    {
        auto synergy_data = SynergyData::Create();
        synergy_data->combat_synergy = combat_synergy;
        for (const TCombatSynergy component : components)
        {
            synergy_data->combat_synergy_components.emplace_back(component);
        }

        const bool has_components = synergy_data->combat_synergy_components.size() > 0;
        synergy_data->synergy_thresholds = GetSynergyDataThresholds(has_components);
        GetSynergyDataThresholdsAbilities(
            synergy_data->combat_synergy,
            has_components,
            &synergy_data->unit_threshold_abilities);
        GetSynergyDataIntrinsicAbilities(synergy_data->intrinsic_abilities);

        synergy_data->hyper_abilities = GetSynergyHyperAbilities(combat_synergy).abilities;

        if constexpr (std::is_same_v<CombatClass, TCombatSynergy>)
        {
            game_data_container_->AddCombatClassSynergyData(combat_synergy, synergy_data);
        }
        else
        {
            static_assert(std::is_same_v<CombatAffinity, TCombatSynergy>);
            game_data_container_->AddCombatAffinitySynergyData(combat_synergy, synergy_data);
        }
    }

    // Gets the synergy_thresholds of a synergy data
    // Data from:
    // https://docs.google.com/spreadsheets/d/1_qBY8UQTu59oT2EVwgCmnrkXnokp0HIkzk-eCG4U4rY/edit#gid=1795943286
    virtual std::vector<int> GetSynergyDataThresholds(const bool has_components) const
    {
        if (has_components)
        {
            return {2, 3, 4};
        }

        return {3, 6, 9};
    }

    virtual void GetSynergyDataThresholdsAbilities(
        const CombatSynergyBonus,
        const bool,
        std::unordered_map<int, std::vector<std::shared_ptr<const AbilityData>>>*) const
    {
    }

    virtual void GetSynergyDataIntrinsicAbilities(std::vector<std::shared_ptr<const AbilityData>>&) const {}

    void FillGameDataContainer(GameDataContainer& game_data_container)
    {
        FillSynergiesData(game_data_container);
        FillEquipmentData(game_data_container);
        FillAugmentsData(game_data_container);
        FillDroneAugmentsData(game_data_container);
        FillConsumablesData(game_data_container);
        FillEncounterModsData(game_data_container);
    }
    virtual void FillAugmentsData(GameDataContainer&) const {}
    virtual void FillDroneAugmentsData(GameDataContainer&) const {}
    virtual void FillConsumablesData(GameDataContainer&) const {}
    virtual void FillEquipmentData(GameDataContainer&) const {}
    virtual void FillEncounterModsData(GameDataContainer&) const {}

    virtual bool UseSynergiesData() const
    {
        return false;
    }
    virtual void FillSynergiesData(GameDataContainer&)
    {
        // Create empty data
        if (!UseSynergiesData())
        {
            return;
        }

        // Data from:
        // https://docs.google.com/spreadsheets/d/1_qBY8UQTu59oT2EVwgCmnrkXnokp0HIkzk-eCG4U4rY/edit#gid=1795943286

        // Add classes synergies data
        AddSynergyData(CombatClass::kFighter, {});
        AddSynergyData(CombatClass::kBulwark, {});
        AddSynergyData(CombatClass::kRogue, {});
        AddSynergyData(CombatClass::kPsion, {});
        AddSynergyData(CombatClass::kEmpath, {});
        AddSynergyData(CombatClass::kBerserker, {CombatClass::kFighter, CombatClass::kFighter});
        AddSynergyData(CombatClass::kBehemoth, {CombatClass::kFighter, CombatClass::kBulwark});
        AddSynergyData(CombatClass::kSlayer, {CombatClass::kFighter, CombatClass::kRogue});
        AddSynergyData(CombatClass::kArcanite, {CombatClass::kFighter, CombatClass::kPsion});
        AddSynergyData(CombatClass::kTemplar, {CombatClass::kFighter, CombatClass::kEmpath});
        AddSynergyData(CombatClass::kColossus, {CombatClass::kBulwark, CombatClass::kBulwark});
        AddSynergyData(CombatClass::kVanguard, {CombatClass::kBulwark, CombatClass::kRogue});
        AddSynergyData(CombatClass::kHarbinger, {CombatClass::kBulwark, CombatClass::kPsion});
        AddSynergyData(CombatClass::kAegis, {CombatClass::kBulwark, CombatClass::kEmpath});
        AddSynergyData(CombatClass::kPredator, {CombatClass::kRogue, CombatClass::kRogue});
        AddSynergyData(CombatClass::kPhantom, {CombatClass::kRogue, CombatClass::kPsion});
        AddSynergyData(CombatClass::kRevenant, {CombatClass::kRogue, CombatClass::kEmpath});
        AddSynergyData(CombatClass::kInvoker, {CombatClass::kPsion, CombatClass::kPsion});
        AddSynergyData(CombatClass::kEnchanter, {CombatClass::kPsion, CombatClass::kEmpath});
        AddSynergyData(CombatClass::kMystic, {CombatClass::kEmpath, CombatClass::kEmpath});

        // Add affinities synergies data
        AddSynergyData(CombatAffinity::kWater, {});
        AddSynergyData(CombatAffinity::kFire, {});
        AddSynergyData(CombatAffinity::kNature, {});
        AddSynergyData(CombatAffinity::kAir, {});
        AddSynergyData(CombatAffinity::kTsunami, {CombatAffinity::kWater, CombatAffinity::kWater});
        AddSynergyData(CombatAffinity::kMud, {CombatAffinity::kWater, CombatAffinity::kEarth});
        AddSynergyData(CombatAffinity::kSteam, {CombatAffinity::kWater, CombatAffinity::kFire});
        AddSynergyData(CombatAffinity::kToxic, {CombatAffinity::kWater, CombatAffinity::kNature});
        AddSynergyData(CombatAffinity::kFrost, {CombatAffinity::kWater, CombatAffinity::kAir});
        AddSynergyData(CombatAffinity::kGranite, {CombatAffinity::kEarth, CombatAffinity::kEarth});
        AddSynergyData(CombatAffinity::kMagma, {CombatAffinity::kEarth, CombatAffinity::kFire});
        AddSynergyData(CombatAffinity::kBloom, {CombatAffinity::kEarth, CombatAffinity::kNature});
        AddSynergyData(CombatAffinity::kDust, {CombatAffinity::kEarth, CombatAffinity::kAir});
        AddSynergyData(CombatAffinity::kInferno, {CombatAffinity::kFire, CombatAffinity::kFire});
        AddSynergyData(CombatAffinity::kWildfire, {CombatAffinity::kFire, CombatAffinity::kNature});
        AddSynergyData(CombatAffinity::kShock, {CombatAffinity::kFire, CombatAffinity::kAir});
        AddSynergyData(CombatAffinity::kVerdant, {CombatAffinity::kNature, CombatAffinity::kNature});
        AddSynergyData(CombatAffinity::kSpore, {CombatAffinity::kNature, CombatAffinity::kAir});
        AddSynergyData(CombatAffinity::kTempest, {CombatAffinity::kAir, CombatAffinity::kAir});
    }

    virtual bool UseHyperData() const
    {
        return false;
    }
    virtual std::unique_ptr<HyperData> GetHyperData()
    {
        // Create empty data
        if (!UseHyperData())
        {
            return nullptr;
        }

        // Affinities
        auto hyper_data = std::make_unique<HyperData>();
        const std::unordered_map<CombatAffinity, int> combat_affinity_water = {
            {CombatAffinity::kWater, 0},
            {CombatAffinity::kEarth, 2},
            {CombatAffinity::kFire, 1},
            {CombatAffinity::kNature, -1},
            {CombatAffinity::kAir, -2},
        };
        const std::unordered_map<CombatAffinity, int> combat_affinity_earth = {
            {CombatAffinity::kWater, -2},
            {CombatAffinity::kEarth, 0},
            {CombatAffinity::kFire, 2},
            {CombatAffinity::kNature, 1},
            {CombatAffinity::kAir, -1},
        };
        const std::unordered_map<CombatAffinity, int> combat_affinity_fire = {
            {CombatAffinity::kWater, -1},
            {CombatAffinity::kEarth, -2},
            {CombatAffinity::kFire, 0},
            {CombatAffinity::kNature, 2},
            {CombatAffinity::kAir, 1},
        };
        const std::unordered_map<CombatAffinity, int> combat_affinity_nature = {
            {CombatAffinity::kWater, -1},
            {CombatAffinity::kEarth, -1},
            {CombatAffinity::kFire, -2},
            {CombatAffinity::kNature, 0},
            {CombatAffinity::kAir, 2},
        };
        const std::unordered_map<CombatAffinity, int> combat_affinity_air = {
            {CombatAffinity::kWater, 2},
            {CombatAffinity::kEarth, 1},
            {CombatAffinity::kFire, -1},
            {CombatAffinity::kNature, -2},
            {CombatAffinity::kAir, 0},
        };
        hyper_data->combat_affinity_opponents[CombatAffinity::kWater] = combat_affinity_water;
        hyper_data->combat_affinity_opponents[CombatAffinity::kEarth] = combat_affinity_earth;
        hyper_data->combat_affinity_opponents[CombatAffinity::kFire] = combat_affinity_fire;
        hyper_data->combat_affinity_opponents[CombatAffinity::kNature] = combat_affinity_nature;
        hyper_data->combat_affinity_opponents[CombatAffinity::kAir] = combat_affinity_air;

        return hyper_data;
    }

    virtual bool UseHyperConfig() const
    {
        return false;
    }

    virtual AbilitiesData GetSynergyHyperAbilities(CombatSynergyBonus synergy);

    virtual HyperConfig GetHyperConfig()
    {
        if (!UseHyperConfig())
        {
            return {};
        }

        HyperConfig hyper_config;
        hyper_config.sub_hyper_scale_percentage = GetSubHyperScalePercentage();
        hyper_config.enemies_range_units = GetHyperEnemiesRangeUnits();

        return hyper_config;
    }

    virtual OverloadConfig GetOverloadConfig() const
    {
        return {};
    }

    virtual FixedPoint GetSubHyperScalePercentage() const
    {
        return 100_fp;
    }
    virtual int GetHyperEnemiesRangeUnits() const
    {
        return 50;
    }

    virtual bool UseMaxStagePlacementRadius() const
    {
        return false;
    }

    virtual std::unique_ptr<WorldEffectsConfig> GetWorldEffectsConfig() const
    {
        auto config = std::make_unique<WorldEffectsConfig>();

        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44368654/Poison
        auto& poison = config->GetConditionType(EffectConditionType::kPoison);
        poison.duration_ms = 5000;
        poison.max_stacks = 5;
        poison.cleanse_on_max_stacks = false;
        // 1 * 5 (seconds) = 5%
        poison.dot_high_precision_percentage = 500_fp;
        poison.dot_damage_type = EffectDamageType::kPure;

        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44336079/Wound
        auto& wound = config->GetConditionType(EffectConditionType::kWound);
        wound.duration_ms = 8000;
        wound.max_stacks = 5;
        wound.cleanse_on_max_stacks = false;
        // 1.25 * 8 (seconds) = 10%
        wound.dot_high_precision_percentage = 1000_fp;
        wound.dot_damage_type = EffectDamageType::kPhysical;

        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/44368692/Burn
        auto& burn = config->GetConditionType(EffectConditionType::kBurn);
        burn.duration_ms = 8000;
        burn.max_stacks = 5;
        burn.cleanse_on_max_stacks = true;
        // 2 * 8 (seconds) = 16%
        burn.dot_high_precision_percentage = 1600_fp;
        burn.dot_damage_type = EffectDamageType::kEnergy;

        // https://illuvium.atlassian.net/wiki/spaces/AB/pages/204243633/Frost
        auto& frost = config->GetConditionType(EffectConditionType::kFrost);
        frost.duration_ms = 10000;
        frost.max_stacks = 9;
        frost.cleanse_on_max_stacks = true;
        frost.dot_high_precision_percentage = 0_fp;
        frost.debuff_percentage = 3_fp;
        frost.debuff_stat_type = StatType::kAttackSpeed;

        return config;
    }

    void TimeStepForSeconds(const int seconds)
    {
        TimeStepForMs(seconds * kMsPerSecond);
    }
    void TimeStepForMs(const int duration_ms)
    {
        TimeStepForNTimeSteps(Time::MsToTimeSteps(duration_ms));
    }
    void TimeStepForNTimeSteps(const int nr_time_steps)
    {
        for (int i = 0; i < nr_time_steps; i++)
        {
            world->TimeStep();
        }
    }

    template <EventType event_type>
    bool TimeStepUntilEvent(const int max_loop_limit = 1000)
    {
        bool event_happened = false;
        const auto event_handle = world->SubscribeToEvent(
            event_type,
            [&](const Event&)
            {
                event_happened = true;
            });

        while (!event_happened && world->GetTimeStepCount() < max_loop_limit)
        {
            world->TimeStep();
        }

        // Something is wrong, infinite loop check
        if (!event_happened)
        {
            world->LogErr("Infinite loop detected inside BaseTest::TimeStepUntilEvent, go and fix your tests");
        }

        world->UnsubscribeFromEvent(event_handle);

        return event_happened;
    }

    bool TimeStepUntilEventSkillDeploying()
    {
        return TimeStepUntilEvent<EventType::kSkillDeploying>();
    }
    bool TimeStepUntilEventAbilityActivated()
    {
        return TimeStepUntilEvent<EventType::kAbilityActivated>();
    }
    bool TimeStepUntilEventAbilityDeactivated()
    {
        return TimeStepUntilEvent<EventType::kAbilityDeactivated>();
    }
    bool TimeStepUntilGoneHyperactive()
    {
        return TimeStepUntilEvent<EventType::kGoneHyperactive>();
    }

    void KillEntity(EntityID sender_entity_id, EntityID receiver_entity_id) const
    {
        if (!world->HasEntity(sender_entity_id) || !world->HasEntity(receiver_entity_id))
        {
            world->LogWarn("Failed to kill entity {} by {}", receiver_entity_id, sender_entity_id);
            return;
        }

        // Effect data
        EffectState effect_state;
        effect_state.is_critical = false;
        effect_state.sender_stats = world->GetFullStats(sender_entity_id);
        effect_state.source_context.combat_unit_ability_type = AbilityType::kAttack;

        const auto damage_effect = EffectData::CreateDamage(
            EffectDamageType::kPure,
            EffectExpression::FromValue(FixedPoint::FromInt(std::numeric_limits<int>::max())));

        // Apply negative effect
        world->BuildAndEmitEvent<EventType::kTryToApplyEffect>(
            sender_entity_id,
            receiver_entity_id,
            damage_effect,
            effect_state);
    }

    // TODO(vampy): Move this into the EffectData once we have an EffectLifetime
    static EffectData CreateEmpowerGlobalEffectAttribute(
        const AbilityType activated_by,
        const int activations_until_expiry,
        const EffectTypeID empower_for_effect_type_id,
        const EffectDataAttributes attributes)
    {
        EffectData data;
        data.type_id.type = EffectType::kEmpower;
        data.lifetime.activated_by = activated_by;
        data.lifetime.is_consumable = true;
        data.lifetime.activations_until_expiry = activations_until_expiry;
        data.is_used_as_global_effect_attribute = true;
        data.empower_for_effect_type_id = empower_for_effect_type_id;
        data.attributes = attributes;

        return data;
    }

    static bool FindAndReplace(std::string& str, const std::string_view to_replace, const std::string_view replace_with)
    {
        auto pos = str.find(to_replace);
        if (pos == std::string::npos) return false;
        str.replace(pos, to_replace.size(), replace_with);
        return true;
    }

    static std::string FindAndReplace(
        const std::string_view format,
        const std::string_view to_replace,
        const std::string_view replace_with)
    {
        std::string result(format);
        FindAndReplace(result, to_replace, replace_with);
        return result;
    }

    std::shared_ptr<World> world;

    std::shared_ptr<GameDataContainer> game_data_container_;

#if (defined ENABLE_VISUALIZATION && defined VISUALIZE_ALL_TESTS)
    std::unique_ptr<tool::BattleVisualization> _tests_viz;
#endif  // ENABLE_VISUALIZATION + VISUALIZE_ALL_TESTS

private:
    RandomGenerator random_generator_;
};

template <EventType EnumType>
struct EventHistory
{
    using EventDataType = event_impl::EventTypeToDataType<EnumType>;

    explicit EventHistory(World& world)
    {
        world.SubscribeToEvent(
            EnumType,
            [this](const Event& event)
            {
                events.emplace_back(event.Get<EventDataType>());
            });
    }

    const EventDataType& operator[](size_t index) const
    {
        return events.at(index);
    }

    size_t Size() const
    {
        return events.size();
    }

    bool IsEmpty() const
    {
        return Size() == 0;
    }

    void Clear()
    {
        events.clear();
    }

    std::vector<EventDataType> events;
};

}  // namespace simulation
