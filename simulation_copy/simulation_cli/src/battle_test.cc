#include "battle_test.h"

#include "battle_data_loader.h"
#include "battle_simulation.h"
#include "cli_settings.h"
#include "ecs/world.h"

static constexpr std::string_view description_field = "Description";
static constexpr std::string_view checks_field = "Checks";
static constexpr std::string_view battle_files_field = "BattleFiles";
static constexpr std::string_view random_seed_count_field = "RandomSeedCount";
static constexpr std::string_view expected_winner_field = "ExpectedWinner";
static constexpr std::string_view expected_health_percentage_field = "ExpectedHealthPercentage";

namespace simulation::tool
{
BattleTestRunner::BattleTestRunner(const std::shared_ptr<CLISettings>& settings, std::shared_ptr<Logger> logger)
    : logger_(std::move(logger))
{
    file_helper_ = std::make_unique<FileHelper>(logger_);
    json_helper_ = std::make_unique<JSONHelper>(logger_);

    // Create separate log for battle logs
    auto world_logger = Logger::Create(false);
    world_logger->SinkAddFile("battle_log.txt");
    battle_simulation_ = std::make_unique<BattleSimulation>(settings, logger_, world_logger);
}

bool BattleTestRunner::Load(const fs::path& path, BattleTestData* out_test) const
{
    const auto file_content = file_helper_->ReadAllContentFromFile(path);
    if (file_content.empty())
    {
        logger_->LogErr("Failed to load content from {}", path);
        return false;
    }

    constexpr bool allow_exceptions = false;
    constexpr bool ignore_comments = false;
    const auto test_json = nlohmann::json::parse(file_content, nullptr, allow_exceptions, ignore_comments);

    // We don't allow exceptions so just check if discarded
    if (test_json.is_discarded())
    {
        logger_->LogErr("Failed to parse JSON from file {}", path);
        return false;
    }

    out_test->path = path.parent_path();
    out_test->file_name = path.stem().string();

    if (!json_helper_->GetStringValue(test_json, description_field, &out_test->description))
    {
        return false;
    }

    if (!LoadChecks(test_json, &out_test->checks))
    {
        return false;
    }

    return true;
}

bool BattleTestRunner::Run(const BattleTestData& test) const
{
    logger_->LogInfo("----------------------------------");
    logger_->LogInfo("Run test: {}", test.file_name);
    logger_->LogDebug("Description: {}", test.description);

    bool success = true;

    for (uint32_t i = 0; i < test.checks.size(); i++)
    {
        const CheckData& check = test.checks[i];
        logger_->LogDebug("Check [{}]: {}", i, check.description);
        if (!RunCheck(test, check))
        {
            success = false;
        }
    }

    return success;
}

bool BattleTestRunner::RunCheck(const BattleTestData& test, const CheckData& check_data) const
{
    bool success = true;

    for (const std::string& battle_file : check_data.battle_files)
    {
        fs::path battle_file_path = test.path;
        battle_file_path += fs::path::preferred_separator;
        battle_file_path += battle_file;

        // Uses seeded random for battle seeds, used only if RandomSeedCount > 0
        RandomGenerator random_generator;
        random_generator.Init(0);

        // We run test check at least once
        // if random_seed_count == 0, we run check once with seed from battle file
        for (int i = 0; i <= check_data.random_seed_count; i++)
        {
            std::optional<uint64_t> random_seed;
            if (i != 0)
            {
                // Use random seed from second index, first check uses seed from battle file
                random_seed = random_generator.Range(0, 1000);
            }
            if (!RunBattleFile(battle_file_path, check_data, random_seed))
            {
                success = false;
            }
        }
    }

    return success;
}

int BattleTestRunner::CalculateWinnerHealthPercentage(const BattleWorldResult& battle_result) const
{
    float total_health = 0.0f;
    float total_health_left = 0.0f;

    for (const BattleEntityResult& state : battle_result.combat_units_end_state)
    {
        if (state.team == battle_result.winning_team)
        {
            total_health += state.max_health.AsFloat();
            total_health_left += state.current_health.AsFloat();
        }
    }

    if (total_health == 0.0f)
    {
        return 0;
    }

    return static_cast<int>(100.0f * total_health_left / total_health);
}

bool BattleTestRunner::LoadChecks(const nlohmann::json& json_object, std::vector<CheckData>* out_test_checks) const
{
    return json_helper_->WalkArray(
        json_object,
        checks_field,
        true,
        [&](const size_t /*index*/, const nlohmann::json& json_array_element) -> bool
        {
            if (!json_array_element.is_object())
            {
                logger_->LogErr("Checks array should contains only objects");
                return false;
            }

            CheckData check_data;
            if (!json_helper_->GetStringValue(json_array_element, description_field, &check_data.description))
            {
                return false;
            }

            if (!json_helper_->GetStringArray(json_array_element, battle_files_field, &check_data.battle_files))
            {
                return false;
            }

            // RandomSeedCount is optional, and by default run only one seed from battle files
            if (json_array_element.contains(random_seed_count_field))
            {
                if (!json_helper_
                         ->GetIntValue(json_array_element, random_seed_count_field, &check_data.random_seed_count))
                {
                    return false;
                }
            }

            if (!json_helper_->GetEnumValue(json_array_element, expected_winner_field, &check_data.expected_winner))
            {
                return false;
            }

            // ExpectedHealthPercentage is optional, can be used to make more check how easy was win
            if (json_array_element.contains(expected_health_percentage_field))
            {
                if (!json_helper_->GetIntValue(
                        json_array_element,
                        expected_health_percentage_field,
                        &check_data.expected_health_percentage))
                {
                    return false;
                }
            }

            out_test_checks->emplace_back(check_data);
            return true;
        });
}

bool BattleTestRunner::RunBattleFile(
    const fs::path& battle_file,
    const CheckData& check_data,
    std::optional<uint64_t> random_seed) const
{
    const auto world = battle_simulation_->OpenBattleFile(battle_file, random_seed);
    if (!world)
    {
        logger_->LogErr("Failed to open battle file: {}", battle_file.filename());
        return false;
    }

    battle_simulation_->TimeStepUntilFinished(world);
    const BattleWorldResult& battle_result = world->GetBattleResult();
    logger_->LogDebug(
        "Battle finished with winner = {}, duration is {} steps, seed {}",
        battle_result.winning_team,
        battle_result.duration_time_steps,
        world->GetBattleConfig().random_seed);
    if (check_data.expected_winner != battle_result.winning_team)
    {
        logger_->LogErr(
            "Check failed: expected winner is {}, battle_file {}",
            check_data.expected_winner,
            battle_file.filename());
        return false;
    }

    // check expected health percentage if set
    if (check_data.expected_health_percentage > 0)
    {
        const int health_percentage = CalculateWinnerHealthPercentage(battle_result);
        if (health_percentage < check_data.expected_health_percentage)
        {
            logger_->LogErr(
                "Check failed: expected health percentage is {}, current {}, battle_file {}",
                check_data.expected_health_percentage,
                health_percentage,
                battle_file.filename());
            return false;
        }

        logger_->LogDebug(
            "Health check passed: expected health percentage is {}, current {}",
            check_data.expected_health_percentage,
            health_percentage);
    }

    return true;
}
}  // namespace simulation::tool
