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
class CLIRunBattleCommand
{
public:
    explicit CLIRunBattleCommand(lyra::cli& cli);

private:
    void DoCommand(const lyra::group& g) const;

private:
    std::string battle_file_;

#ifdef ENABLE_VISUALIZATION
    bool visualize = false;
#endif
};
}  // namespace simulation::tool
