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
 * CLIRunTestCommand
 *
 * This class handles `run_test` cli command.
 * It responsible for interaction with running battle simulation.
 * --------------------------------------------------------------------------------------------------------
 */
class CLIRunTestCommand
{
public:
    explicit CLIRunTestCommand(lyra::cli& cli);

private:
    void DoCommand(const lyra::group& g) const;

private:
    std::string test_files_dir_;
};
}  // namespace simulation::tool
