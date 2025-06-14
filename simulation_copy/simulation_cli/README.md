# Simulation CLI

The main goal of simulation cli is ability to load and run battle files that were saved by game client.

It allows testing and debugging these battles using only simulation library repository.

Currently cli supports two commands: [set](#set) and [run](#run)

Also, cli also supports help output that prints all possible options:
```bash
./simulation-cli.exe --help
```
## set
`set` command is used to set up settings that are required to run the simulation.

Set command works in a stateful way and is saved in `.cli_settings.json` file.

It allows other commands to have much fewer input parameters.

#### usage example

```bash
# Enable simulation debug logs output
./simulation-cli.exe set --enable_debug_logs true

# Set location of JSON data files
./simulation-cli.exe set --json_data_path "E:\Workplace\IlluviumGame\Content\LocalTestData"

# Set location of battle files folder
./simulation-cli.exe set --battle_files_path "E:\Workplace\IlluviumGame\Saved\Simulation\Battles"
```

## run
`run` command loads and runs battle files provided as an argument.

It will fail if `json_data_path` is not specified correctly or battle file cannot be found.

#### usage example
```bash
# Will try to run 2GhostBeetles(.json) from current folder or from <battle_files_path> folder
./simulation-cli.exe run 2GhostBeetles

# Full explicit path is also supported
./simulation-cli.exe run "E:\Workplace\IlluviumGame\Saved\Simulation\Battles\Shoebill_2_tornados.json"

# If <battle_files_path> points to gameclient folder with battle files its enough to provide only name
./simulation-cli.exe run 2022.11.08-15.44.42

# Visualise it
./simulation-cli.exe run 2022.11.08-15.44.42 -v
```
