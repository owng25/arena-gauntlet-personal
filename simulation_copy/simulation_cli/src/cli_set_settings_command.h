#pragma once

#include "cli_settings.h"

namespace lyra
{
class cli;
class group;
}  // namespace lyra

namespace simulation::tool
{
/* -------------------------------------------------------------------------------------------------------
 * CLISetSettingCommand
 *
 * This class handles `set` cli command.
 * It responsible for interaction with saving cli settings.
 * --------------------------------------------------------------------------------------------------------
 */
class CLISetSettingCommand
{
public:
    explicit CLISetSettingCommand(lyra::cli& cli);

private:
    CLISettings settings_;
};
}  // namespace simulation::tool
