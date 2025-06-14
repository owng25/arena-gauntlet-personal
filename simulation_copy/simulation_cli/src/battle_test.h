#pragma once

#include <optional>

#include "utility/file_helper.h"
#include "utility/json_helper.h"

namespace simulation
{
struct BattleWorldResult;
}

namespace simulation::tool
{
class CLISettings;
class BattleSimulation;

struct CheckData
{
    std::string description;
    std::vector<std::string> battle_files;
    int random_seed_count = 0;
    Team expected_winner = Team::kBlue;

    // By default we just need a result, but additional can check health percentage left on winner
    int expected_health_percentage = 0;
};

struct BattleTestData
{
    fs::path path;
    std::string file_name;
    std::string description;
    std::vector<CheckData> checks;
};

class BattleTestRunner
{
public:
    BattleTestRunner(const std::shared_ptr<CLISettings>& settings, std::shared_ptr<Logger> logger);

    bool Load(const fs::path& test_path, BattleTestData* out_test) const;
    bool Run(const BattleTestData& test) const;

protected:
    bool LoadChecks(const nlohmann::json& json_object, std::vector<CheckData>* out_test_checks) const;
    bool RunBattleFile(const fs::path& battle_file, const CheckData& check_data, std::optional<uint64_t> random_seed)
        const;
    bool RunCheck(const BattleTestData& test, const CheckData& check_data) const;

    int CalculateWinnerHealthPercentage(const BattleWorldResult& battle_result) const;

private:
    std::shared_ptr<Logger> logger_;
    std::unique_ptr<FileHelper> file_helper_;
    std::unique_ptr<JSONHelper> json_helper_;
    std::unique_ptr<BattleSimulation> battle_simulation_;
};

}  // namespace simulation::tool
