#pragma once

#include <string>

namespace lyra
{
class cli;
class group;
}  // namespace lyra

namespace simulation::tool
{

/* -------------------------------------------------------------------------------------------------------
 * CLIRunBattleCommand
 *
 * This class handles `run` cli command.
 * It responsible for interaction with running battle simulation.
 * --------------------------------------------------------------------------------------------------------
 */
class CLIRunBatchCommand
{
public:
    explicit CLIRunBatchCommand(lyra::cli& cli);

private:
    void DoCommand(const lyra::group& g) const;

private:
    std::string battle_files_dir_;
    std::string battles_results_dir_;
};
}  // namespace simulation::tool
