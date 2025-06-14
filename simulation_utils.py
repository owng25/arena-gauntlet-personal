# --- START OF FILE simulation_utils.py ---

import os
import json
import re
import torch # This import might not be strictly needed in simulation_utils, but leave it if it was original
import subprocess
import shutil
import time
from typing import List, Dict, Any, Tuple, Union, Optional, Set
from pathlib import Path
import numpy as np # Keep np as is, assuming _global_np handles this in train_env.py
import stat
import traceback # Import traceback for detailed error printing
import platform

# Import the new flag from train_env
try:
    from train_env import TRAINING_STEP_DEBUG
except ImportError:
    print("[WARN simulation_utils] Could not import TRAINING_STEP_DEBUG from train_env. Defaulting to False.", file=sys.__stderr__, flush=True)
    TRAINING_STEP_DEBUG = False

# --- Paths ---
try: PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__)) if '__file__' in locals() else os.path.abspath('.')
except NameError: PROJECT_ROOT = os.getcwd()
BATTLE_DIR = os.path.abspath(os.path.join(PROJECT_ROOT, "./battle"))
BATTLE_RESULTS_DIR = os.path.abspath(os.path.join(PROJECT_ROOT, "./battle_results"))
sim_cli_name = "simulation-cli"
if platform.system() == "Windows":
    sim_cli_name += ".exe"
SIM_CLI_PATH = os.path.abspath(os.path.join(PROJECT_ROOT, "simulation_copy", "build", "bin", sim_cli_name))
# Fallback if structure is different (e.g., run from project root)
if not os.path.exists(SIM_CLI_PATH):
    alt_path = os.path.abspath(os.path.join(PROJECT_ROOT, "..", "simulation_copy", "build", "bin", sim_cli_name))
    if os.path.exists(alt_path): SIM_CLI_PATH = alt_path
    else: print(f"[WARN simulation_utils] SIM_CLI_PATH not found at default locations. Using: {SIM_CLI_PATH}")

DATA_DIR = os.path.abspath(os.path.join(PROJECT_ROOT, "json_data_copy", "data"))
# Fallback if structure is different
if not os.path.exists(DATA_DIR):
     alt_data_dir = os.path.abspath(os.path.join(PROJECT_ROOT, "..", "json_data_copy", "data"))
     if os.path.exists(alt_data_dir): DATA_DIR = alt_data_dir
     else: print(f"[WARN simulation_utils] DATA_DIR not found at default locations. Using: {DATA_DIR}")

OPPONENT_MODELS_DIR = os.path.abspath(os.path.join(PROJECT_ROOT, "./opponent_models"))

# --- MODIFIED: Ensure directories exist (excluding archive) ---
for directory in [BATTLE_DIR, BATTLE_RESULTS_DIR, OPPONENT_MODELS_DIR]:
    try: os.makedirs(directory, exist_ok=True)
    except OSError as e: print(f"[WARN simulation_utils] Could not create directory {directory}: {e}")
# --- END MODIFIED ---

# --- HELPER: Set CLI Param ---
def _set_cli_param(param_name: str, param_value: Union[str, Path]):
    try:
        if not os.path.isfile(SIM_CLI_PATH): raise FileNotFoundError(f"Sim CLI not found: {SIM_CLI_PATH}")
        param_value_str = str(param_value); cli_dir = os.path.dirname(SIM_CLI_PATH)
        # These internal debug prints were already commented out or conditional.
        # if TRAINING_STEP_DEBUG: print(f"[DEBUG _set_cli_param] Running: {SIM_CLI_PATH} set {param_name} {param_value_str} (CWD: {cli_dir})")
        result = subprocess.run( [SIM_CLI_PATH, "set", param_name, param_value_str], check=True, capture_output=True, text=True, timeout=15, cwd=cli_dir )
        # if TRAINING_STEP_DEBUG: print(f"[DEBUG _set_cli_param] Set '{param_name}' successful. Stdout: {result.stdout[:100]}...")
    except subprocess.CalledProcessError as e: print(f"[ERROR] Sim CLI 'set {param_name}' failed! RC={e.returncode}. Stderr:\n{e.stderr}\nStdout:{e.stdout}"); print(f"[WARN] Proceeding anyway.") # These are critical errors, keep
    except subprocess.TimeoutExpired as e: print(f"[ERROR] Sim CLI 'set {param_name}' timed out! Stderr:\n{e.stderr}\nStdout:{e.stdout}"); print(f"[WARN] Proceeding anyway.") # These are critical errors, keep
    except FileNotFoundError: print(f"[FATAL] Cannot proceed without Sim CLI at {SIM_CLI_PATH}"); raise # Critical error, keep
    except Exception as e: print(f"[ERROR] Unexpected error setting CLI param '{param_name}': {e}"); traceback.print_exc(); print(f"[WARN] Proceeding anyway.") # Critical error, keep

# --- Function to extract JSON from log string ---
def extract_battle_result_json(log_line: str) -> Optional[Dict[str, Any]]:
    match = re.search(r"BattleResultJSON:\s*\"({.*})\"", log_line, re.DOTALL)
    if match:
        json_str_escaped = match.group(1)
        try: data = json.loads(json_str_escaped.replace('\\"', '"')); return data
        except Exception as e: print(f"[ERROR] Decode BattleResultJSON fail: {e}. Raw: '{json_str_escaped[:100]}...'"); return None # Critical error, keep
    return None

# --- run_batch_simulation with BATTLE_DIR cleanup AND BATTLE_RESULTS_DIR cleanup ---
def run_batch_simulation(battle_files_paths: List[str]) -> Dict[str, Dict[str, Any]]:
    timestamp_start = time.strftime("%Y-%m-%d_%H:%M:%S")
    if TRAINING_STEP_DEBUG:
        print(f"\n--- [run_batch_simulation @ {timestamp_start}] Attempting {len(battle_files_paths)} Battles ---")
    
    if not battle_files_paths: 
        if TRAINING_STEP_DEBUG:
            print("[run_batch_simulation] No battle file paths provided. Skipping.")
        return {}

    expected_basenames_for_current_run = {os.path.splitext(os.path.basename(f))[0] for f in battle_files_paths}
    if TRAINING_STEP_DEBUG:
        print(f"[run_batch_simulation] Expected basenames for this run: {expected_basenames_for_current_run}")

    # --- Path and Permission Checks ---
    if not os.path.isfile(SIM_CLI_PATH): 
        print(f"[FATAL ERROR] Sim CLI not found! Path: {SIM_CLI_PATH}")
        return {"__error__": {"message": "Sim CLI not found", "path": SIM_CLI_PATH}}
    if not os.path.isdir(BATTLE_DIR) or not os.access(BATTLE_DIR, os.W_OK): 
        print(f"[FATAL ERROR] Battle dir not found or not writable! Path: {BATTLE_DIR}")
        return {"__error__": {"message": "Battle dir not found or not writable", "path": BATTLE_DIR}}
    if not os.access(BATTLE_RESULTS_DIR, os.W_OK): 
        print(f"[FATAL ERROR] Cannot write to BATTLE_RESULTS_DIR! Path: {BATTLE_RESULTS_DIR}")
        return {"__error__": {"message": "Cannot write to BATTLE_RESULTS_DIR", "path": BATTLE_RESULTS_DIR}}

    # --- BATTLE_DIR Cleanup (Remove files NOT in current batch) ---
    intended_files = {os.path.basename(f) for f in battle_files_paths}
    removed_battle_count = 0; skipped_battle_count = 0
    if TRAINING_STEP_DEBUG:
        print(f"[run_batch_simulation] Starting BATTLE_DIR cleanup. Intended files: {len(intended_files)}")
    try:
        if os.path.isdir(BATTLE_DIR):
            for item_name in os.listdir(BATTLE_DIR):
                if item_name.endswith(".json"):
                    item_path = os.path.join(BATTLE_DIR, item_name)
                    if item_name not in intended_files:
                        try: 
                            os.remove(item_path)
                            removed_battle_count += 1
                            if TRAINING_STEP_DEBUG:
                                print(f"[run_batch_simulation] Removed old battle file from BATTLE_DIR: {item_name}")
                        except OSError as e: 
                            print(f"[WARN] Failed to remove old battle file {item_path}: {e}")
                            skipped_battle_count += 1
        if TRAINING_STEP_DEBUG:
            print(f"[run_batch_simulation] BATTLE_DIR cleanup: Removed {removed_battle_count} old files, skipped {skipped_battle_count}.")
    except Exception as e: print(f"[ERROR] Exception during BATTLE_DIR cleanup: {e}")

    # --- BATTLE_RESULTS_DIR Cleanup (DELETE directories NOT expected for current batch) ---
    if TRAINING_STEP_DEBUG:
        print(f"[run_batch_simulation] Cleaning old/unexpected results from BATTLE_RESULTS_DIR: {BATTLE_RESULTS_DIR}")
    deleted_results_count = 0; skipped_delete_count = 0
    try:
        if os.path.isdir(BATTLE_RESULTS_DIR):
            for item_name in os.listdir(BATTLE_RESULTS_DIR):
                item_path = os.path.join(BATTLE_RESULTS_DIR, item_name)
                if os.path.isdir(item_path) and item_name not in expected_basenames_for_current_run:
                    try: 
                        shutil.rmtree(item_path)
                        deleted_results_count += 1
                    except OSError as e: 
                        print(f"[WARN] Failed to delete old result directory {item_path}: {e}")
                        skipped_delete_count += 1
        if TRAINING_STEP_DEBUG:
            print(f"[run_batch_simulation] BATTLE_RESULTS_DIR cleanup: Deleted {deleted_results_count} old/unexpected dirs, skipped {skipped_delete_count}.")
    except Exception as e: print(f"[ERROR] Exception during BATTLE_RESULTS_DIR cleanup: {e}")

    # Set CLI parameters
    try: 
        _set_cli_param("--json_data_path", DATA_DIR)
        _set_cli_param("--enable_debug_logs", "false")
    except Exception as e: 
        print(f"[FATAL ERROR] Failed set CLI params: {e}")
        return {"__error__": {"message": f"Failed set CLI params: {e}"}}

    # Debug: Check BATTLE_DIR contents AFTER cleanup
    try:
        actual_files_in_battle_dir = []; non_existent_files = []
        if os.path.isdir(BATTLE_DIR): actual_files_in_battle_dir = os.listdir(BATTLE_DIR)
        non_existent_files = [os.path.basename(f) for f in battle_files_paths if not os.path.exists(f)]
        if non_existent_files: 
            print(f"[ERROR!!!!] Files requested for sim but NOT FOUND ON DISK: {non_existent_files}")
    except Exception as list_err: print(f"[ERROR] Listing BATTLE_DIR failed: {list_err}")

    # --- Execute Simulation ---
    start_time = time.time(); completed_process = None
    cli_cwd = os.path.dirname(SIM_CLI_PATH);
    sim_command = [SIM_CLI_PATH, "run_batch", BATTLE_DIR, BATTLE_RESULTS_DIR]
    stdout_log_path = os.path.join(PROJECT_ROOT, "simulation_stdout.log")
    stderr_log_path = os.path.join(PROJECT_ROOT, "simulation_stderr.log")
    return_code = -99; stdout_capture = "[Not captured]"; stderr_capture = "[Not captured]"
    try:
        with open(stdout_log_path, "w") as f_out, open(stderr_log_path, "w") as f_err:
            completed_process = subprocess.run( sim_command, stdout=f_out, stderr=f_err, text=True, timeout=180, cwd=cli_cwd, check=False )
        return_code = completed_process.returncode
        try:
            with open(stdout_log_path, "r") as f: stdout_capture = f.read()
        except Exception as e: print(f"[ERROR] Failed to read {stdout_log_path}: {e}"); stdout_capture = "[Read Error]"
        try:
            with open(stderr_log_path, "r") as f: stderr_capture = f.read()
        except Exception as e: print(f"[ERROR] Failed to read {stderr_log_path}: {e}"); stderr_capture = "[Read Error]"

        results_dir_exists_after = os.path.isdir(BATTLE_RESULTS_DIR)
        if return_code != 0 or not results_dir_exists_after:
             print(f"[!!!! SIMULATION PROBLEM DETECTED !!!!]")
             if return_code !=0: print(f"[ERROR] Simulation CLI exited non-zero ({return_code}). Stderr log tail:\n'''\n{stderr_capture[-1000:]}\n'''")
             if not results_dir_exists_after: print(f"[ERROR] !!! BATTLE_RESULTS_DIR MISSING AFTER SIM CALL: {BATTLE_RESULTS_DIR} !!!")

    except subprocess.TimeoutExpired as e:
        print(f"[FATAL ERROR] Simulation CLI timed out ({e.timeout}s)! Cmd: {' '.join(map(str,e.cmd))}")
        stdout_capture = "[Timeout - Read Error]"; stderr_capture = "[Timeout - Read Error]"
        try:
            if os.path.exists(stdout_log_path):
                 with open(stdout_log_path, "r") as f: stdout_capture = f.read()
        except Exception as read_e: print(f"[ERROR] Timeout: Failed read stdout log {stdout_log_path}: {read_e}")
        try:
             if os.path.exists(stderr_log_path):
                 with open(stderr_log_path, "r") as f: stderr_capture = f.read()
        except Exception as read_e: print(f"[ERROR] Timeout: Failed read stderr log {stderr_log_path}: {read_e}")
        print(f"  Timeout Stdout (from file tail):\n'''\n{stdout_capture[-1000:]}\n'''")
        print(f"  Timeout Stderr (from file tail):\n'''\n{stderr_capture[-1000:]}\n'''")
        return {"__error__": {"message": f"Simulation timed out ({e.timeout}s)"}}

    except Exception as e: 
        print(f"[FATAL ERROR] Unexpected error running simulation: {e}"); traceback.print_exc()
        return {"__error__": {"message": f"Unexpected error running simulation: {e}"}}
    duration = time.time() - start_time;

    # --- Result Parsing from Filesystem ---
    results: Dict[str, Dict[str, Any]] = {}; file_parse_errors = [];
    found_results_count = 0; simulation_stdout_errors = []; processed_dirs = 0
    winner_regex = re.compile(r"\"WinningTeam\"\s*:\s*\"(\w+)\"", re.IGNORECASE)

    if TRAINING_STEP_DEBUG:
        print(f"[run_batch_simulation] Starting result parsing from {BATTLE_RESULTS_DIR}. Expected: {len(expected_basenames_for_current_run)}.")
    
    if not os.path.exists(BATTLE_RESULTS_DIR): 
        print(f"[ERROR] Results directory missing post-sim: {BATTLE_RESULTS_DIR}")
    else:
        for dir_name in expected_basenames_for_current_run:
            dir_path = os.path.join(BATTLE_RESULTS_DIR, dir_name)
            if not os.path.isdir(dir_path):
                file_parse_errors.append(f"{dir_name} (result directory missing)")
                if TRAINING_STEP_DEBUG:
                    print(f"[run_batch_simulation WARN] Result directory missing for {dir_name}. Path: {dir_path}")
                continue
            processed_dirs += 1;
            stdout_file = os.path.join(dir_path, "stdout.txt")
            if os.path.exists(stdout_file):
                winner = None; file_content = ""
                try:
                    file_size = os.path.getsize(stdout_file)
                    if file_size == 0: 
                        file_parse_errors.append(f"{dir_name} (stdout.txt is empty)")
                        if TRAINING_STEP_DEBUG:
                             print(f"[run_batch_simulation WARN] {stdout_file} is empty.")
                        continue
                    with open(stdout_file, "r", encoding="utf-8", errors='ignore') as f: file_content = f.read()
                    match = winner_regex.search(file_content)
                    if match: 
                        winner = match.group(1)
                        results[dir_name] = {"WinningTeam": winner}
                        found_results_count += 1
                    else: 
                        file_parse_errors.append(f"{dir_name} (WinningTeam line missing/regex fail in stdout.txt)")
                        if TRAINING_STEP_DEBUG:
                            print(f"[run_batch_simulation WARN] WinningTeam not found in {stdout_file}. Content start: {file_content[:200]}")
                    
                    for line_num, line in enumerate(file_content.splitlines()):
                        if "[error" in line.lower() or "[fatal" in line.lower() or "failed" in line.lower() or "exception" in line.lower() or "assert" in line.lower():
                             simulation_stdout_errors.append(f"{dir_name}/L{line_num+1}: {line.strip()}")
                except Exception as e: 
                    print(f"[ERROR] Failed read/process {stdout_file}: {e}");
                    file_parse_errors.append(f"{stdout_file} (Read/Process error: {e})")
            else: 
                file_parse_errors.append(f"{dir_name} (stdout.txt missing)")
                if TRAINING_STEP_DEBUG:
                    print(f"[run_batch_simulation WARN] stdout.txt missing for {dir_name}. Path: {stdout_file}")

    # --- Verification & Error Reporting ---
    missing_or_failed = []
    for base_name in expected_basenames_for_current_run:
        if base_name not in results:
            specific_error = next((err for err in file_parse_errors if err.startswith(base_name)), "result directory or stdout.txt missing/empty/unparsable")
            missing_or_failed.append(f"{base_name} ({specific_error})")

    # This block controls whether the summary of internal sim errors/warnings are printed
    if TRAINING_STEP_DEBUG: # Only print this block if debug is on
        if file_parse_errors:
             print(f"[WARN] Encountered {len(file_parse_errors)} issues during results parsing:")
             unique_parse_errors = sorted(list(set(file_parse_errors)))
             for i, err in enumerate(unique_parse_errors):
                 if i < 15: print(f"  - {err}")
                 elif i == 15: print(f"  - ... and {len(unique_parse_errors)-15} more.") ; break
        if missing_or_failed:
            error_report = {"message": f"Problem processing {len(missing_or_failed)}/{len(expected_basenames_for_current_run)} expected results.", "failed_battles_details": sorted(list(set(missing_or_failed)))}
            print(f"[WARN] {error_report['message']}")
        if simulation_stdout_errors:
             print(f"[WARN] Internal simulation errors/warnings detected in {len(simulation_stdout_errors)} stdout lines:")
             unique_errors = sorted(list(set(simulation_stdout_errors)));
             for i, ln in enumerate(unique_errors):
                  if i < 10: print(f"  - {ln}");
                  elif i == 10: print(f"  - ... and {len(unique_errors)-10} more.") ; break
    # Ensure the __error__ key is added for programmatic access even if not printed
    if '__error__' not in results and (file_parse_errors or missing_or_failed or simulation_stdout_errors):
        results['__error__'] = {}
        if file_parse_errors: results['__error__']['file_parse_errors'] = file_parse_errors
        if missing_or_failed: results['__error__']['failed_battles_details'] = missing_or_failed
        if simulation_stdout_errors: results['__error__']['internal_sim_errors'] = simulation_stdout_errors

    timestamp_end = time.strftime("%Y-%m-%d_%H:%M:%S")
    if TRAINING_STEP_DEBUG:
        print(f"--- [run_batch_simulation @ {timestamp_end}] Finished. Returned {len(results)} results (excluding errors). Processed dirs: {processed_dirs}. Found results count: {found_results_count} ---\n")
    return results


# --- force_reset_simulator ---
def force_reset_simulator():
    cli_dir=os.path.dirname(SIM_CLI_PATH); settings_path=os.path.join(cli_dir,".cli_settings.json")
    config={ "BattleFilesPath": str(Path(BATTLE_DIR).as_posix()), "EnableDebugLogs": False, "JsonDataPath": str(Path(DATA_DIR).as_posix()), "LogPattern":"[%=15!n] [general] %v", "ProfileFilePath":"simulation-cli.prof" }
    print(f"Attempting write simulator settings: {settings_path}"); print(f"Using JsonDataPath: {config['JsonDataPath']}")
    try:
        os.makedirs(cli_dir,exist_ok=True);
        with open(settings_path,"w") as f: json.dump(config,f,indent=4)
        print(f"Successfully wrote settings file."); print(f"Running 'set' command for json_data_path via CLI...")
        _set_cli_param("--json_data_path", Path(DATA_DIR));
        _set_cli_param("--enable_debug_logs", "false");
        print("Simulator 'set' commands finished.")
    except FileNotFoundError: print(f"[FATAL] Sim CLI not found: {SIM_CLI_PATH}. Cannot force settings."); raise
    except PermissionError as e: print(f"[ERROR] Permission denied write settings {settings_path}: {e}"); raise
    except Exception as e: print(f"[ERROR] Failed config simulator: {e}"); traceback.print_exc(); raise


# --- convert_np_types ---
def convert_np_types(o: Any):
    """Converts common NumPy types for JSON/pickling."""
    if isinstance(o, np.integer): return int(o)
    elif isinstance(o, np.floating): return float(o)
    elif isinstance(o, np.ndarray): return o.tolist()
    elif isinstance(o, bytes): return o.decode('utf-8', errors='ignore')
    elif isinstance(o, np.bool_): return bool(o)
    elif isinstance(o, Path): return str(o)
    if isinstance(o, (int, float, bool, str, list, dict, type(None))): return o
    try: json.dumps(o); return o
    except TypeError: return str(o)
# --- END OF FILE simulation_utils.py ---