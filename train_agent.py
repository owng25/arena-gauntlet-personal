# --- START OF FILE train_agent.py ---

# train_agent.py
import os
import shutil
import numpy as np
import argparse
import time
import csv
import random
import re
from tqdm import tqdm
import multiprocessing
from typing import List, Dict, Optional, Tuple, Callable, Sequence, Any
# <<< MODIFIED: Add deque for global save tracking >>>
from collections import Counter, deque
import traceback
import uuid
# --- NEW/MODIFIED IMPORTS ---
from stable_baselines3.common.policies import BasePolicy
from stable_baselines3.common.vec_env import VecEnv
from multiprocessing import Manager # <<< CRITICAL: Added for shared memory >>>
# --- END NEW/MODIFIED IMPORTS ---

# --- IMPORT Gymnasium ---
import gymnasium as gym
# --- IMPORT functools ---
import functools

# --- IMPORT Torch and Disable Validation ---
import torch
torch.distributions.Distribution.set_default_validate_args(False)
# --- END Disable Validation ---

# === MODIFIED IMPORTS ===
from sb3_contrib import MaskablePPO # <-- Use MaskablePPO
from sb3_contrib.common.maskable.utils import get_action_masks, is_masking_supported # Utility function
# === END MODIFIED IMPORTS ===

from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.callbacks import BaseCallback, CheckpointCallback, CallbackList
from stable_baselines3.common.utils import set_random_seed, safe_mean
from stable_baselines3.common.vec_env import VecEnv # , unwrap_vec_wrapper (Consider if needed)
from stable_baselines3.common.logger import configure as configure_logger # Use configure for setup
from stable_baselines3.common.buffers import RolloutBuffer

# Import local modules
try:
    # <<< MODIFIED: Import required constants >>>
    from train_env import (
        IlluviumSingleEnv, MAX_BOARD_SLOTS, NUM_PLAYERS, DETAILED_SELFPLAY_DEBUG,
        FIRST_PLACE_TEAMS_DIR, MAX_FIRST_PLACE_SAVES, SAVE_FIRST_PLACE_TEAMS,
        TRAINING_STEP_DEBUG # <<< IMPORTANT: Importing the master debug flag >>>
    )
    from train_env import (
        FLAT_IDX_SELL_START, FLAT_IDX_BUY_START, FLAT_IDX_REROLL, FLAT_IDX_BUY_XP,
        FLAT_IDX_ENTER_PLACEMENT, FLAT_IDX_MODIFY_UNIT_START, FLAT_IDX_EXIT_PLACEMENT,
        FLAT_IDX_AUGMENT_SKIP, FLAT_IDX_AUGMENT_APPLY_START, TOTAL_FLAT_ACTIONS,
        decode_flat_action, # Import the decoder function
        deep_convert_np_types # Import the deep conversion helper
    )
    # Define the original number of types for logging
    NUM_ORIGINAL_ACTION_TYPES = 8 # Based on the 8 conceptual action types
except ImportError as e:
    print(f"ERROR: Could not import from train_env: {e}")
    traceback.print_exc()
    MAX_BOARD_SLOTS = 19 # Fallback
    NUM_PLAYERS = 8 # <<< ADD FALLBACK HERE TOO >>>
    DETAILED_SELFPLAY_DEBUG = False # Fallback
    NUM_ORIGINAL_ACTION_TYPES = 8 # Fallback
    # <<< ADD FALLBACKS FOR SAVE LOGIC CONSTANTS >>>
    FIRST_PLACE_TEAMS_DIR = "./first_place_teams"
    MAX_FIRST_PLACE_SAVES = 20
    SAVE_FIRST_PLACE_TEAMS = False
    # Fallback for the new debug flag
    TRAINING_STEP_DEBUG = False # <<< IMPORTANT: Fallback for the master debug flag >>>
    # Mark flat indices as invalid if import fails
    FLAT_IDX_SELL_START = -1; FLAT_IDX_BUY_START = -1; FLAT_IDX_REROLL = -1; FLAT_IDX_BUY_XP = -1;
    FLAT_IDX_ENTER_PLACEMENT = -1; FLAT_IDX_MODIFY_UNIT_START = -1; FLAT_IDX_EXIT_PLACEMENT = -1;
    FLAT_IDX_AUGMENT_SKIP = -1; FLAT_IDX_AUGMENT_APPLY_START = -1; TOTAL_FLAT_ACTIONS = -1
    def decode_flat_action(idx): return -1, -1, -1, "Decode Error"
    def deep_convert_np_types(obj, context="unknown"): return obj # Dummy fallback with context
    # Exit if the core environment cannot be imported
    if "IlluviumSingleEnv" in str(e):
         print("FATAL: IlluviumSingleEnv could not be imported. Exiting.")
         exit(1)
    else: # Allow continuing if only helpers failed, though likely problematic
         pass

# <<< IMPORTANT: Define a module-level constant for the debug flag in train_agent.py >>>
# This makes it accessible to classes within this file without NameError.
_TRAINING_STEP_DEBUG_GLOBAL = TRAINING_STEP_DEBUG
# <<< END IMPORTANT >>>

try:
    from sync_vec_env import SyncSimulationVecEnv # Use the custom VecEnv
except ImportError as e:
     print(f"ERROR: Could not import SyncSimulationVecEnv from sync_vec_env: {e}")
     traceback.print_exc()
     exit(1) # Exit if the custom VecEnv cannot be imported

try:
    from simulation_utils import (
        force_reset_simulator, BATTLE_DIR, BATTLE_RESULTS_DIR,
        OPPONENT_MODELS_DIR, SIM_CLI_PATH, DATA_DIR
    )
except ImportError as e:
     print(f"WARN: Could not import from simulation_utils: {e}")
     OPPONENT_MODELS_DIR = "./opponent_models"; BATTLE_DIR = "./battle"; BATTLE_RESULTS_DIR = "./battle_results"
     def force_reset_simulator(): print("WARN: force_reset_simulator not available.")


# --- Constants ---
LOG_DIR = os.path.abspath("./logs")
MODEL_DIR = os.path.abspath("./models")
CSV_LOG_FILE = os.path.join(LOG_DIR, "metrics_log.csv")

# Ensure directories exist
for d in [LOG_DIR, MODEL_DIR, OPPONENT_MODELS_DIR, BATTLE_DIR, BATTLE_RESULTS_DIR]:
    try:
        os.makedirs(d, exist_ok=True)
    except OSError as e:
         print(f"WARN: Could not create directory {d}: {e}")

# ---------------------------
# In-Memory Opponent Brain Manager
# ---------------------------
class InMemoryOpponentBrainManager:
    def __init__(self, max_pool_size: int, 
                 _shared_brains_dict: Optional[Dict[str, BasePolicy]] = None,
                 _shared_brain_ids_list: Optional[List[str]] = None):
        
        self.max_pool_size = max_pool_size
        
        # These are multiprocessing.Manager.dict and multiprocessing.Manager.list proxies
        # when created by multiprocessing.Manager() in train() function.
        self.brains = _shared_brains_dict if _shared_brains_dict is not None else {}
        self.brain_ids_list = _shared_brain_ids_list if _shared_brain_ids_list is not None else []
        # No longer using collections.deque directly, simulating its behavior on the shared list
        
        if _TRAINING_STEP_DEBUG_GLOBAL: # <<< Use the new global flag >>>
            # Check if actual shared objects were passed (implies Manager was used)
            using_shared = (_shared_brains_dict is not None and isinstance(_shared_brains_dict, type(Manager().dict())))
            print(f"[BrainManager] Initialized with max_pool_size: {self.max_pool_size}. Using shared objects: {using_shared}")

    def add_brain(self, policy: BasePolicy, brain_id_prefix: Optional[str] = None) -> str:
        """Adds a policy (expected to be on CPU) to the manager.
           Manages storage limits (evicts oldest if pool is full).
           Returns the assigned brain_id.
        """
        unique_suffix = str(uuid.uuid4())[:8] # Short UUID for uniqueness
        final_brain_id = f"{brain_id_prefix}_{unique_suffix}" if brain_id_prefix else unique_suffix

        # If pool is full, remove the oldest brain to make space for the new one
        if len(self.brains) >= self.max_pool_size:
            if self.brain_ids_list: # Ensure list is not empty before trying to pop
                oldest_brain_id_to_remove = self.brain_ids_list.pop(0) # pop(0) simulates popleft
                if oldest_brain_id_to_remove in self.brains:
                    del self.brains[oldest_brain_id_to_remove] # Delete from the shared dictionary
                    if _TRAINING_STEP_DEBUG_GLOBAL: # <<< Use the new global flag >>>
                        print(f"[BrainManager] Pool full. Removed oldest brain: {oldest_brain_id_to_remove}")
                else:
                    # This case means the ID was in the list but not the dict, which is an inconsistency.
                    if _TRAINING_STEP_DEBUG_GLOBAL: # <<< Use the new global flag >>>
                         print(f"[BrainManager WARN] Oldest brain ID '{oldest_brain_id_to_remove}' from list not found in shared dict. Dict keys: {list(self.brains.keys())}. This indicates an inconsistency.")
            # else: # If the list is empty but brains dict is not (shouldn't happen with correct usage)
                # if _TRAINING_STEP_DEBUG_GLOBAL: print(f"[BrainManager WARN] Brains dict size {len(self.brains)} but brain_ids_list is empty. Cannot evict.")

        policy.to(torch.device("cpu")) # Ensure policy is on CPU before adding
        self.brains[final_brain_id] = policy # Add new policy to the shared dictionary
        self.brain_ids_list.append(final_brain_id) # Add new brain's ID to the end of the shared list (simulates append)

        if _TRAINING_STEP_DEBUG_GLOBAL: # <<< Use the new global flag >>>
            print(f"[BrainManager] Added brain: {final_brain_id}. Pool size: {len(self.brains)}/{self.max_pool_size}. Shared List size: {len(self.brain_ids_list)}")
        return final_brain_id

    def get_brain_policy(self, brain_id: str) -> Optional[BasePolicy]:
        """Retrieves a policy object by its brain_id from the shared dictionary."""
        policy = self.brains.get(brain_id)
        return policy

    def get_available_brain_ids(self) -> List[str]:
        """Returns a copy of the list of available brain_ids in the manager."""
        # Return a copy of the shared list to prevent external modification issues
        return list(self.brain_ids_list)

    def get_num_brains(self) -> int:
        """Returns the number of brains currently stored in the manager."""
        return len(self.brains)

# ---------------------------
# Callbacks (Modified for Placement Logging and Top 4 Rate)
# ---------------------------
class IlluviumTensorboardCallback(BaseCallback):
    def __init__(self, verbose=0, episode_metrics_buffer_size=100):
        super().__init__(verbose)
        self.action_names = {
            0: "Sell", 1: "Buy", 2: "Reroll", 3: "BuyXP",
            4: "EnterPlace", 5: "ModifyUnit", 6: "ExitPlace", 7: "AugmentAct",
        }
        self.num_action_types = NUM_ORIGINAL_ACTION_TYPES
        self.win_rate_buffer = deque(maxlen=episode_metrics_buffer_size)
        self.placement_buffer = deque(maxlen=episode_metrics_buffer_size)
        self.top4_rate_buffer = deque(maxlen=episode_metrics_buffer_size)
        self.episode_reward_buffer = deque(maxlen=episode_metrics_buffer_size)

    def _log_mean_from_infos(self, key: str, info_key: str):
        if self.logger is None: return
        if "infos" not in self.locals or not isinstance(self.locals["infos"], list):
            return
        values = [info.get(info_key) for info in self.locals["infos"] if info is not None and info.get(info_key) is not None]
        if values:
            try:
                mean_val = safe_mean(values)
                self.logger.record(key, mean_val)
            except Exception as e:
                print(f"Error logging {key}: {e}") # This print is fine, it's an internal callback error

    def _on_step(self) -> bool:
        if self.logger is None or "infos" not in self.locals or "actions" not in self.locals:
            return True

        infos = self.locals["infos"]
        actions = self.locals["actions"]

        if not isinstance(infos, list):
             return True

        # Log standard metrics
        metrics_to_log = {
            "illuvium/mean_coins": "agent_coins", "illuvium/mean_health": "agent_health",
            "illuvium/mean_level": "agent_level", "illuvium/mean_xp": "agent_xp",
            "illuvium/mean_board_units": "agent_board_units", "illuvium/mean_bench_units": "agent_bench_units",
            "illuvium/mean_opponents_alive": "opponents_alive", "illuvium/mean_sim_reward": "sim_reward_last_step",
            "illuvium/placement_reward": "placement_reward"
        }
        for key, info_key in metrics_to_log.items():
            self._log_mean_from_infos(key, info_key)

        # Log Mean Number of Active Synergies
        synergy_counts = []
        for info in infos:
            if info and isinstance(info, dict):
                synergies_dict = info.get("agent_total_synergies")
                if isinstance(synergies_dict, dict):
                    synergy_counts.append(len(synergies_dict))
        if synergy_counts:
            self.logger.record("illuvium/mean_num_active_synergies", safe_mean(synergy_counts))

        # Log win rate, top 4 rate, placement AND episode reward
        for info in infos:
            if info and isinstance(info, dict) and "episode" in info and isinstance(info["episode"], dict):
                ep_info = info["episode"]
                if "r" in ep_info: self.episode_reward_buffer.append(ep_info["r"])
                if "w" in ep_info: self.win_rate_buffer.append(ep_info["w"])
                if "final_placement" in info:
                    placement = info["final_placement"]
                    self.placement_buffer.append(placement)
                    self.top4_rate_buffer.append(1.0 if placement <= 4 else 0.0)

        # Record mean values from buffers
        if self.episode_reward_buffer: self.logger.record("rollout/ep_rew_mean", safe_mean(list(self.episode_reward_buffer)))
        if self.win_rate_buffer: self.logger.record("rollout/ep_win_rate", safe_mean(list(self.win_rate_buffer)))
        if self.top4_rate_buffer: self.logger.record("rollout/ep_top4_rate", safe_mean(list(self.top4_rate_buffer)))
        if self.placement_buffer: self.logger.record("rollout/ep_mean_placement", safe_mean(list(self.placement_buffer)))

        # Log action type distribution
        if actions is not None and isinstance(actions, np.ndarray):
            action_types = [decode_flat_action(idx)[0] for idx in actions]
            valid_types = [t for t in action_types if 0 <= t < self.num_action_types]

            if valid_types:
                action_counts = Counter(valid_types)
                total_valid = len(valid_types)
                for i in range(self.num_action_types):
                    count = action_counts.get(i, 0)
                    freq = count / total_valid if total_valid > 0 else 0
                    action_name = self.action_names.get(i, f"Type_{i}")
                    self.logger.record(f"actions/count_{action_name}", count)
                    self.logger.record(f"actions/freq_{action_name}", freq)
            else: # Log zeros if no valid actions were taken
                 for i in range(self.num_action_types):
                     action_name = self.action_names.get(i, f"Type_{i}")
                     self.logger.record(f"actions/count_{action_name}", 0)
                     self.logger.record(f"actions/freq_{action_name}", 0.0)

        return True


class TQDMCallback(BaseCallback):
    def __init__(self, total_timesteps, verbose=0):
        super().__init__(verbose)
        self.total_timesteps = total_timesteps
        self.pbar: Optional[tqdm] = None
        self.last_log_time = time.time()
        self.last_pbar_update_step = 0

    def _on_training_start(self):
        self.pbar = tqdm(total=self.total_timesteps, dynamic_ncols=True, desc="Training")

    def _on_step(self) -> bool:
        if self.pbar is None: return True
        
        n_envs = getattr(self.training_env, "num_envs", 1)
        
        if _TRAINING_STEP_DEBUG_GLOBAL: # <<< Use the module-level flag for conditional print >>>
            if n_envs > 0: self.pbar.update(n_envs) # Update every step if debug is on
            now = time.time()
            if now - self.last_log_time > 1.0 and self.logger is not None and hasattr(self.logger, 'name_to_value'):
                 postfix = {}; log_vals = self.logger.name_to_value
                 ep_r = log_vals.get("rollout/ep_rew_mean")
                 ep_l = log_vals.get("rollout/ep_len_mean"); win_r = log_vals.get("rollout/ep_win_rate")
                 h = log_vals.get("illuvium/mean_health"); c = log_vals.get("illuvium/mean_coins"); board = log_vals.get("illuvium/mean_board_units")
                 place = log_vals.get("rollout/ep_mean_placement") 
                 top4 = log_vals.get("rollout/ep_top4_rate") 
                 if ep_r is not None: postfix["ep_rew"]=f"{ep_r:.2f}"
                 if ep_l is not None: postfix["ep_len"]=f"{ep_l:.1f}"
                 if win_r is not None: postfix["win%"]=f"{win_r*100:.1f}"
                 if top4 is not None: postfix["top4%"]=f"{top4*100:.1f}"
                 if place is not None: postfix["plc"] = f"{place:.2f}"
                 if h is not None: postfix["hp"]=f"{h:.1f}"
                 if c is not None: postfix["coin"]=f"{c:.1f}"
                 if board is not None: postfix["board"]=f"{board:.1f}"
                 if postfix: self.pbar.set_postfix(postfix, refresh=False)
                 self.last_log_time = now
        else: # _TRAINING_STEP_DEBUG_GLOBAL is False (quiet mode)
            # Update progress bar less frequently, e.g., every 100 * n_envs steps of experience
            # to still show overall progress but avoid per-step updates.
            if self.num_timesteps >= self.last_pbar_update_step + (n_envs * 100): # Approx every 100 env steps processed
                if n_envs > 0: self.pbar.update(self.num_timesteps - self.last_pbar_update_step)
                self.last_pbar_update_step = self.num_timesteps
                # No set_postfix here, keeping it truly quiet
        return True

    def _on_training_end(self):
        if self.pbar:
            if self.num_timesteps < self.total_timesteps:
                self.pbar.update(self.total_timesteps - self.num_timesteps)
            elif self.num_timesteps > self.last_pbar_update_step and not _TRAINING_STEP_DEBUG_GLOBAL: # final update if not debug
                self.pbar.update(self.num_timesteps - self.last_pbar_update_step)

            self.pbar.close()


class FrequentDumpCallback(BaseCallback):
    def __init__(self, dump_freq_steps: int = 100, verbose: int = 0):
        super().__init__(verbose); self.dump_freq_steps = dump_freq_steps; self._last_dump_timestep = -1
    def _on_step(self) -> bool:
        if self.num_timesteps >= self._last_dump_timestep + self.dump_freq_steps:
             if self.num_timesteps > 0 and self.logger:
                 self.logger.dump(step=self.num_timesteps);
                 self._last_dump_timestep = self.num_timesteps
        return True


class CSVLoggerCallback(BaseCallback):
    def __init__(self, log_file=CSV_LOG_FILE, continue_training=False, verbose=0):
        super().__init__(verbose); self.log_file = log_file; self.continue_training = continue_training; self.csv_file = None; self.csv_writer = None
    def _on_training_start(self):
        os.makedirs(os.path.dirname(self.log_file), exist_ok=True); exists = os.path.isfile(self.log_file); mode = 'a' if self.continue_training and exists else 'w'
        try:
            self.csv_file = open(self.log_file, mode, newline="", encoding='utf-8'); self.csv_writer = csv.writer(self.csv_file)
            if mode == 'w' or not exists or os.path.getsize(self.log_file) == 0:
                self.csv_writer.writerow([
                    "timestep","ep_rew_mean","ep_len_mean","mean_win_rate",
                    "mean_top4_rate", "mean_placement", "mean_synergies",
                    "mean_board_units","mean_coins","mean_health","mean_level",
                    "mean_opp_alive","mean_sim_reward"
                ]);
                self.csv_file.flush()
        except IOError as e: print(f"[ERROR] CSVLogger: Open fail {self.log_file} mode '{mode}': {e}"); self.csv_file = self.csv_writer = None
    def _on_step(self) -> bool:
        if self.csv_writer is None or self.logger is None or not hasattr(self.logger, 'name_to_value'): return True
        log = self.logger.name_to_value
        ep_r=log.get("rollout/ep_rew_mean")
        ep_l=log.get("rollout/ep_len_mean"); win_r=log.get("rollout/ep_win_rate");
        top4=log.get("rollout/ep_top4_rate");
        place=log.get("rollout/ep_mean_placement");
        syn=log.get("illuvium/mean_num_active_synergies");
        board=log.get("illuvium/mean_board_units");
        c=log.get("illuvium/mean_coins"); h=log.get("illuvium/mean_health"); l=log.get("illuvium/mean_level"); oa=log.get("illuvium/mean_opponents_alive"); sr=log.get("illuvium/mean_sim_reward")
        if ep_r is not None or ep_l is not None or win_r is not None or place is not None or top4 is not None or syn is not None:
            try:
                self.csv_writer.writerow([
                    self.num_timesteps,
                    f"{ep_r:.3f}" if ep_r is not None else "",
                    f"{ep_l:.2f}" if ep_l is not None else "",
                    f"{win_r:.3f}" if win_r is not None else "",
                    f"{top4:.3f}" if top4 is not None else "",
                    f"{place:.2f}" if place is not None else "",
                    f"{syn:.2f}" if syn is not None else "",
                    f"{board:.2f}" if board is not None else "",
                    f"{c:.2f}" if c is not None else "",
                    f"{h:.2f}" if h is not None else "",
                    f"{l:.2f}" if l is not None else "",
                    f"{oa:.2f}" if oa is not None else "",
                    f"{sr:.3f}" if sr is not None else ""
                ]);
                if self.csv_file: self.csv_file.flush()
            except Exception as e: print(f"[WARN] CSVLogger write fail: {e}")
        return True
    def _on_training_end(self):
        if self.csv_file:
            try: self.csv_file.close()
            except Exception as e: print(f"[WARN] CSVLogger close fail: {e}")
            finally: self.csv_file = None; self.csv_writer = None


class DynamicOpponentCallback(BaseCallback): # Note: This class must inherit from BaseCallback to work with SB3.
    def __init__(self, 
                 opponent_brain_manager: InMemoryOpponentBrainManager,
                 replace_freq_steps: int = 10000,
                 verbose: int = 0):
        super().__init__(verbose) # Call super().__init__
        if replace_freq_steps <= 0: raise ValueError("replace_freq_steps must be > 0")
        
        self.opponent_brain_manager = opponent_brain_manager
        self.replace_freq_steps = replace_freq_steps
        self.brain_update_count = 0
        self._last_update_timestep = -1

        self.winning_team_saves: deque[str] = deque(maxlen=MAX_FIRST_PLACE_SAVES)
        if SAVE_FIRST_PLACE_TEAMS:
            os.makedirs(FIRST_PLACE_TEAMS_DIR, exist_ok=True)

    def _weighted_choice_index(self, pool: Sequence[str]) -> int:
        i = -1 
        if not pool: return i

        weights = list(range(1, len(pool) + 1))
        total_weight = sum(weights)

        if total_weight <= 0:
            i = random.randrange(len(pool))
        else:
            r = random.uniform(0, total_weight)
            u = 0.0
            for idx, weight in enumerate(weights):
                if u + weight >= r:
                    i = idx
                    break
                u += weight
            if i == -1:
                 # if _TRAINING_STEP_DEBUG_GLOBAL and DETAILED_SELFPLAY_DEBUG: # Already using TRAINING_STEP_DEBUG here
                    # print(f"[DynOpp WARN] Weighted choice loop finished unexpectedly. Returning last index {len(pool) - 1}.")
                 i = len(pool) - 1
        return i

    def _save_winning_team(self, info: Dict[str, Any], env_idx: int):
        if not SAVE_FIRST_PLACE_TEAMS: return
        try:
            if len(self.winning_team_saves) >= MAX_FIRST_PLACE_SAVES and self.winning_team_saves: # Check deque not empty
                oldest_filepath = self.winning_team_saves.popleft()
                try:
                    if os.path.exists(oldest_filepath): os.remove(oldest_filepath)
                    if _TRAINING_STEP_DEBUG_GLOBAL and DETAILED_SELFPLAY_DEBUG: # <<< Use new global flag >>>
                        print(f"[DEBUG DynOpp SaveWin] Removed oldest 1st place save: {os.path.basename(oldest_filepath)}")
                except OSError as e_rem: print(f"[WARN DynOpp SaveWin] Failed to remove old 1st place save {os.path.basename(oldest_filepath)}: {e_rem}")

            ts_ns = time.time_ns(); filename = f"win_ts{ts_ns}_step{self.num_timesteps}_env{env_idx}.json"; filepath = os.path.join(FIRST_PLACE_TEAMS_DIR, filename)
            team_data = {
                "env_id": info.get("env_id", env_idx), "round": info.get("round", -1), "timestep": self.num_timesteps, "timestamp_ns": ts_ns,
                "agent_final_health": info.get("agent_health", -1), "agent_final_coins": info.get("agent_coins", -1),
                "agent_level": info.get("agent_level", -1), "agent_xp": info.get("agent_xp", -1),
                "final_agent_board": info.get("final_agent_board", []), "final_agent_bench": info.get("final_agent_bench", []),
                "cumulative_rewards": info.get("episode", {}).get("r", 0.0)
            }
            saveable_data = deep_convert_np_types(team_data, context='info_save_win')
            with open(filepath, "w", encoding="utf-8") as f: json.dump(saveable_data, f, indent=2)
            self.winning_team_saves.append(filepath)
            print(f"[INFO DynOpp SaveWin] Saved 1st place team to: {filename} (Tracking {len(self.winning_team_saves)}/{MAX_FIRST_PLACE_SAVES})")
        except Exception as e_save: print(f"[ERROR DynOpp SaveWin] Failed to save winning team composition: {e_save}"); traceback.print_exc()

    def _on_step(self) -> bool:
        current_ts = self.num_timesteps # Get current timestep once
        
        if current_ts >= self._last_update_timestep + self.replace_freq_steps:
            if current_ts == self._last_update_timestep and self._last_update_timestep != -1: 
                if _TRAINING_STEP_DEBUG_GLOBAL and DETAILED_SELFPLAY_DEBUG: # <<< Use new global flag >>>
                    print(f"[DEBUG DynOpp Skip] Already updated at ts {current_ts}. Next update > {self._last_update_timestep + self.replace_freq_steps}.")
                return True
            
            self._last_update_timestep = current_ts # Update last_update_timestep BEFORE adding new brain
            self.brain_update_count += 1
            new_brain_id_prefix = f"brain_{self.brain_update_count:04d}_{current_ts:09d}"
            
            if _TRAINING_STEP_DEBUG_GLOBAL and DETAILED_SELFPLAY_DEBUG: # <<< Use new global flag >>>
                print(f"\n[DEBUG DynOpp NewBrain] Ts {current_ts}. Attempting to create new brain (Update #{self.brain_update_count}) with prefix: {new_brain_id_prefix}")

            try:
                agent_policy = self.model.policy
                policy_state_dict = agent_policy.state_dict()
                
                # Ensure lr_schedule is callable for the new policy
                # If self.model.lr_schedule is a float, make it a lambda
                lr_sched_fn = self.model.lr_schedule
                if not callable(self.model.lr_schedule):
                    lr_val = float(self.model.lr_schedule)
                    lr_sched_fn = lambda _: lr_val

                new_policy_for_opponent = self.model.policy_class(
                    observation_space=agent_policy.observation_space,
                    action_space=agent_policy.action_space,
                    lr_schedule=lr_sched_fn,
                    **self.model.policy_kwargs
                )
                new_policy_for_opponent.load_state_dict(policy_state_dict)
                new_policy_for_opponent.to(torch.device("cpu"))

                assigned_brain_id = self.opponent_brain_manager.add_brain(new_policy_for_opponent, brain_id_prefix=new_brain_id_prefix)
                if _TRAINING_STEP_DEBUG_GLOBAL and DETAILED_SELFPLAY_DEBUG: # <<< Use new global flag >>>
                    print(f"[DEBUG DynOpp NewBrain OK] New brain '{assigned_brain_id}' created and added to manager. Manager pool size: {self.opponent_brain_manager.get_num_brains()}")
            except Exception as e:
                print(f"[ERROR] DynOpp failed to create or add new brain: {e}"); traceback.print_exc()
                return True

            current_brain_pool_ids = self.opponent_brain_manager.get_available_brain_ids()
            if not current_brain_pool_ids:
                if _TRAINING_STEP_DEBUG_GLOBAL and DETAILED_SELFPLAY_DEBUG: # <<< Use new global flag >>>
                    print("[DEBUG DynOpp Distribute] Opponent brain pool (in manager) is empty.")
                return True

            opponent_configs_to_set: Dict[int, Tuple[str, str]] = {}
            opponent_player_ids = list(range(1, NUM_PLAYERS)) 
            if not opponent_player_ids: return True

            if _TRAINING_STEP_DEBUG_GLOBAL and DETAILED_SELFPLAY_DEBUG: # <<< Use new global flag >>>
                print(f"[DEBUG DynOpp Distribute] Selecting brains for opponents {opponent_player_ids} from manager pool: {current_brain_pool_ids}")
            
            for p_id in opponent_player_ids:
                if len(current_brain_pool_ids) == 1: selected_brain_id = current_brain_pool_ids[0]
                else:
                    chosen_idx = self._weighted_choice_index(current_brain_pool_ids)
                    selected_brain_id = current_brain_pool_ids[chosen_idx] if chosen_idx != -1 else random.choice(current_brain_pool_ids)
                
                opponent_configs_to_set[p_id] = ("manager", selected_brain_id)
                if _TRAINING_STEP_DEBUG_GLOBAL and DETAILED_SELFPLAY_DEBUG: # <<< Use new global flag >>>
                    print(f"[DEBUG DynOpp Distribute]   -> Assigning Opponent P{p_id} brain_id: '{selected_brain_id}' (Source: manager)")
            
            sync_env: Optional[SyncSimulationVecEnv] = None; venv = self.training_env
            while hasattr(venv, "venv"):
                 if isinstance(venv, SyncSimulationVecEnv): sync_env = venv; break
                 try: venv = venv.venv
                 except AttributeError: break
            if sync_env is None and isinstance(venv, SyncSimulationVecEnv): sync_env = venv

            if sync_env:
                if _TRAINING_STEP_DEBUG_GLOBAL and DETAILED_SELFPLAY_DEBUG: # <<< Use new global flag >>>
                    log_configs_for_broadcast = {k: v for k, v in opponent_configs_to_set.items()}
                    print(f"[DEBUG DynOpp Distribute] Broadcasting {len(log_configs_for_broadcast)} opponent configs to VecEnv: {log_configs_for_broadcast}")
                try:
                    sync_env.set_opponent_configs_dict(opponent_configs_to_set)
                    if _TRAINING_STEP_DEBUG_GLOBAL and DETAILED_SELFPLAY_DEBUG: # <<< Use new global flag >>>
                        print(f"[DEBUG DynOpp Distribute] Config update request sent successfully.")
                    else: # Keep concise message always unless all prints are off
                        print(f"[DynOpp] Sent brain config update for {len(opponent_configs_to_set)} opponents at step {current_ts}.")
                except Exception as e:
                    print(f"[ERROR] DynOpp set_opponent_configs_dict failed during broadcast: {e}"); traceback.print_exc()
            else: print("[WARN] DynOpp could not find SyncSimulationVecEnv instance to set opponent configs.")

            if self.logger:
                self.logger.record("custom/brain_update_count", self.brain_update_count)
                self.logger.record("custom/manager_pool_size", self.opponent_brain_manager.get_num_brains())

        if SAVE_FIRST_PLACE_TEAMS:
            infos = self.locals.get("infos", []); dones = self.locals.get("dones", [])
            if isinstance(infos, list) and isinstance(dones, list) and len(infos) == len(dones):
                for i, (info, done) in enumerate(zip(infos, dones)):
                    if done and info and info.get("agent_won_episode"):
                        if "final_agent_board" in info and "final_agent_bench" in info: self._save_winning_team(info, i)
        return True


# ---------------------------
# Environment Creation Helper
# ---------------------------
def _init_env(env_id: int, seed: int, render_mode: str, 
              opponent_brain_manager_instance: Optional[InMemoryOpponentBrainManager]) -> gym.Env:
    env_seed = seed + env_id
    try:
         mode = render_mode if render_mode in IlluviumSingleEnv.metadata["render_modes"] else "none"
         env = IlluviumSingleEnv(
             env_id=env_id, 
             render_mode=mode,
             opponent_brain_manager=opponent_brain_manager_instance
            )
    except NameError:
        print(f"FATAL ERROR in _init_env for Env {env_id}: IlluviumSingleEnv not defined.")
        traceback.print_exc()
        raise RuntimeError("IlluviumSingleEnv not found.")
    except Exception as e:
        print(f"FATAL ERROR in _init_env for Env {env_id}: {e}")
        traceback.print_exc()
        raise RuntimeError(f"Failed to initialize IlluviumSingleEnv: {e}")
    return env

def make_env(env_id: int, seed: int = 0, render_mode: str = "ansi", 
             opponent_brain_manager_instance: Optional[InMemoryOpponentBrainManager] = None) -> Callable[[], gym.Env]:
    return functools.partial(_init_env, env_id=env_id, seed=seed, render_mode=render_mode, 
                             opponent_brain_manager_instance=opponent_brain_manager_instance)


# ---------------------------
# Main Training Function
# ---------------------------
def parse_args():
    parser = argparse.ArgumentParser("Train MaskablePPO Agent for Illuvium Arena")
    parser.add_argument("--timesteps", type=int, default=10000000, help="Total training timesteps")
    parser.add_argument("--num-envs", type=int, default=8, help="Number of parallel environments")
    parser.add_argument("--seed", type=int, default=42, help="Random seed")
    parser.add_argument("--continue-training", action="store_true", help="Load latest model and continue")
    parser.add_argument("--load-path", type=str, default=None, help="Explicit path to load model from")
    parser.add_argument("--save-freq", type=int, default=50_000, help="Save checkpoint every N timesteps")
    parser.add_argument("--opp-update-freq", type=int, default=100000, help="Update opponents every N timesteps (Default: 10000)")
    parser.add_argument("--opp-pool-size", type=int, default=10, help="Maximum number of opponent models to keep in the pool (Default: 10)")
    parser.add_argument("--lr", type=float, default=3e-4, help="Learning rate")
    parser.add_argument("--n-steps", type=int, default=1024, help="Steps per enve per PPO collect (Default: 512)")
    parser.add_argument("--batch-size", type=int, default=64, help="PPO minibatch size (Default: 64)")
    parser.add_argument("--n-epochs", type=int, default=10, help="PPO epochs per update")
    parser.add_argument("--verbose", type=int, default=1, choices=[0, 1, 2], help="Verbosity level")
    parser.add_argument("--start-method", type=str, default="spawn", choices=["fork", "spawn", "forkserver"], help="Multiprocessing start method")
    parser.add_argument("--render-mode", type=str, default="none", choices=["human", "ansi", "none"], help="Env render mode")
    parser.add_argument("--policy-net", type=str, default="256,256, 128", help="Policy network layers (e.g., '128,64')")
    parser.add_argument("--log-folder", type=str, default=LOG_DIR, help="Path to logging directory")
    parser.add_argument("--model-folder", type=str, default=MODEL_DIR, help="Path to model saving directory")
    return parser.parse_args()

def parse_net_arch(arch_str: str) -> Dict[str, List[int]]:
    """ Parses network architecture string into a dict for policy_kwargs. """
    default_arch = dict(pi=[64, 64], vf=[64, 64])
    try:
        layers = [int(s.strip()) for s in arch_str.split(',') if s.strip()]
        if not layers: print(f"[WARN] Invalid net arch '{arch_str}'. Using default: {default_arch}."); return default_arch
        return dict(pi=list(layers), vf=list(layers))
    except Exception as e: print(f"[WARN] Failed parse net arch '{arch_str}': {e}. Using default: {default_arch}."); return default_arch

def verify_tensorboard(log_dir):
    try: import tensorboard; print(f"✅ TB installed. View: tensorboard --logdir {os.path.abspath(log_dir)}")
    except ImportError: print("⚠️ TB not installed. Run: pip install tensorboard")


def train():
    args = parse_args()
    set_random_seed(args.seed)

    # --- Instantiate InMemoryOpponentBrainManager ---
    manager_proxy = Manager()
    shared_brains = manager_proxy.dict()
    shared_brain_ids_list = manager_proxy.list()

    opponent_brain_manager = InMemoryOpponentBrainManager(
        max_pool_size=args.opp_pool_size,
        _shared_brains_dict=shared_brains,
        _shared_brain_ids_list=shared_brain_ids_list
    )
    if _TRAINING_STEP_DEBUG_GLOBAL: # <<< Use the global flag for this print >>>
        print(f"Initialized InMemoryOpponentBrainManager with max_pool_size: {args.opp_pool_size}")

    log_dir = os.path.abspath(args.log_folder)
    model_dir = os.path.abspath(args.model_folder)
    csv_log_file = os.path.join(log_dir, "metrics_log.csv")
    for d in [LOG_DIR, MODEL_DIR, OPPONENT_MODELS_DIR, BATTLE_DIR, BATTLE_RESULTS_DIR]:
        try: os.makedirs(d, exist_ok=True)
        except OSError as e: print(f"WARN: Could not create directory {d}: {e}")

    start_method = args.start_method
    try:
        current_method = multiprocessing.get_start_method(allow_none=True)
        if current_method is None or current_method != start_method:
            if start_method in multiprocessing.get_all_start_methods(): 
                if _TRAINING_STEP_DEBUG_GLOBAL: print(f"Set MP start method: {start_method}") # <<< Use global flag >>>
                multiprocessing.set_start_method(start_method, force=True); 
            else: 
                print(f"Warn: Start method '{start_method}' not available. Using '{current_method or multiprocessing.get_start_method()}'.")
    except (RuntimeError, ValueError, OSError) as e: print(f"Warn: Could not set start method '{start_method}': {e}")
    if _TRAINING_STEP_DEBUG_GLOBAL: # <<< Use global flag >>>
        print(f"Using multiprocessing start method: {multiprocessing.get_start_method()}")
    if os.name == 'nt' and multiprocessing.get_start_method() != 'spawn': print("WARN: Windows generally requires 'spawn' start method for stability.")

    is_continuing = args.continue_training or args.load_path
    if not is_continuing:
        for d_to_clear in [log_dir]: 
            if os.path.exists(d_to_clear):
                 if _TRAINING_STEP_DEBUG_GLOBAL: print(f"Fresh start. Removing: {d_to_clear}") # <<< Use global flag >>>
                 shutil.rmtree(d_to_clear, ignore_errors=True)
            os.makedirs(d_to_clear, exist_ok=True)
        if os.path.exists(csv_log_file): 
            try: os.remove(csv_log_file)
            except OSError as e: print(f"[WARN] CSVLogger: Could not remove old CSV log {csv_log_file}: {e}") # Corrected WARN message
    
    if _TRAINING_STEP_DEBUG_GLOBAL: # <<< Use global flag >>>
        print("===== Resetting Simulator Settings =====")
    try: 
        force_reset_simulator(); 
        if _TRAINING_STEP_DEBUG_GLOBAL: print("===== Simulator Reset Complete =====") # <<< Use global flag >>>
    except Exception as e: print(f"===== Simulator Reset FAILED: {e}. Continuing anyway... ====="); traceback.print_exc()

    if _TRAINING_STEP_DEBUG_GLOBAL: # <<< Use global flag >>>
        print(f"Creating {args.num_envs} parallel environments...")
    env_fns = [
        make_env(
            env_id=i, 
            seed=args.seed + i, 
            render_mode=args.render_mode,
            opponent_brain_manager_instance=opponent_brain_manager
        ) for i in range(args.num_envs)
    ]
    try:
        env: VecEnv = SyncSimulationVecEnv(env_fns, start_method=start_method)
        if _TRAINING_STEP_DEBUG_GLOBAL: # <<< Use global flag >>>
            print(f"VecEnv created: {env.__class__.__name__}")
            print(f"Observation Space: {env.observation_space}")
            print(f"Action Space: {env.action_space}")
        if not isinstance(env.action_space, gym.spaces.Discrete): print(f"[ERROR] Expected Discrete action space, got {type(env.action_space)}"); env.close(); return
        try:
            if not is_masking_supported(env): # type: ignore
                print("[ERROR] Environment does not support action masking (missing 'action_masks' method or has_attr fails).")
                env.close(); return
            else: 
                if _TRAINING_STEP_DEBUG_GLOBAL: print("✅ Environment supports action masking.") # <<< Use global flag >>>
        except Exception as e: print(f"[ERROR] Failed to check action masking support: {e}."); traceback.print_exc(); env.close(); return
    except (EOFError, RuntimeError, Exception) as e: print(f"\n!!! Error creating VecEnv: {e} !!!"); traceback.print_exc(); return

    chkpt_freq_env_steps = max(1, args.save_freq // args.num_envs)
    opp_update_freq_env_steps = max(1, args.opp_update_freq // args.num_envs)
    dump_freq_env_steps = max(1, 100 // args.num_envs) 

    if _TRAINING_STEP_DEBUG_GLOBAL: # <<< Use global flag >>>
        print(f"Checkpoint freq: {args.save_freq} total steps (~{chkpt_freq_env_steps} env steps/save)")
        print(f"Opponent update freq: {args.opp_update_freq} total steps (~{opp_update_freq_env_steps} env steps/update)")
        print(f"Opponent pool size for manager: {args.opp_pool_size}")
        print(f"Log dump freq: ~{dump_freq_env_steps * args.num_envs} total steps")

    callbacks_list = [
        IlluviumTensorboardCallback(episode_metrics_buffer_size=100),
        TQDMCallback(total_timesteps=args.timesteps), # This callback's _on_step handles its own verbosity
        FrequentDumpCallback(dump_freq_steps=dump_freq_env_steps * args.num_envs),
        CSVLoggerCallback(log_file=csv_log_file, continue_training=is_continuing),
        CheckpointCallback(save_freq=chkpt_freq_env_steps, save_path=model_dir, name_prefix="illuvium_maskable_ppo", save_replay_buffer=False, save_vecnormalize=False),
        DynamicOpponentCallback(
            opponent_brain_manager=opponent_brain_manager,
            replace_freq_steps=opp_update_freq_env_steps * args.num_envs
        )
    ]
    callbacks = CallbackList(callbacks_list)

    load_path = args.load_path; model = None
    policy_kwargs = dict(net_arch=parse_net_arch(args.policy_net))
    if _TRAINING_STEP_DEBUG_GLOBAL: # <<< Use global flag >>>
        print(f"Using policy network architecture: {policy_kwargs['net_arch']}")

    if is_continuing and not load_path:
        if os.path.exists(model_dir):
             checkpoints = []; prefixes = ["illuvium_maskable_ppo_"]
             for f_name in os.listdir(model_dir):
                  if f_name.endswith(".zip") and any(f_name.startswith(p) for p in prefixes):
                      fp = os.path.join(model_dir, f_name)
                      try:
                          ts_match = re.search(r'_(?P<ts>\d+)_steps\.zip', f_name) or \
                                     re.search(r'_(?P<ts>\d+)_INTERRUPTED\.zip', f_name)
                          ts_val = int(ts_match.group('ts')) if ts_match else os.path.getmtime(fp)
                          checkpoints.append((ts_val, fp))
                      except Exception as e_parse: print(f"[WARN] Parse steps from {f_name} failed, use mtime. Err: {e_parse}"); checkpoints.append((os.path.getmtime(fp), fp))
             if checkpoints: 
                load_path = sorted(checkpoints, key=lambda x: x[0], reverse=True)[0][1]; 
                if _TRAINING_STEP_DEBUG_GLOBAL: print(f"Continue training from: {os.path.basename(load_path)}") # <<< Use global flag >>>
             else: 
                if _TRAINING_STEP_DEBUG_GLOBAL: print("Continue enabled, no models found in:", model_dir) # <<< Use global flag >>>
        else: 
            if _TRAINING_STEP_DEBUG_GLOBAL: print("Continue enabled, model directory DNE:", model_dir) # <<< Use global flag >>>

    if load_path:
        if os.path.exists(load_path):
            if _TRAINING_STEP_DEBUG_GLOBAL: print(f"Loading model: {load_path} using MaskablePPO") # <<< Use global flag >>>
            try:
                model = MaskablePPO.load(
                    load_path,
                    env=env, # type: ignore
                    device="auto", 
                    print_system_info=False,
                    learning_rate=args.lr,
                )
                if _TRAINING_STEP_DEBUG_GLOBAL: print(f"Model loaded. Current Timestep: {model.num_timesteps}") # <<< Use global flag >>>
                current_lr_fn = model.lr_schedule # lr_schedule is a callable
                if _TRAINING_STEP_DEBUG_GLOBAL: print(f"Model learning rate after load (should reflect args.lr): {current_lr_fn(1.0):.6f}") # <<< Use global flag >>>
            except Exception as e: print(f"[ERROR] Failed load model: {e}. Initializing new."); traceback.print_exc(); model = None
        else: print(f"Error: Load path specified but not found: {load_path}. Initializing new."); model = None

    if model is None:
        if _TRAINING_STEP_DEBUG_GLOBAL: print("Initializing new MaskablePPO model...") # <<< Use global flag >>>
        model = MaskablePPO( "MlpPolicy", env, verbose=args.verbose, tensorboard_log=log_dir, # type: ignore
                             learning_rate=args.lr, n_steps=args.n_steps, batch_size=args.batch_size, 
                             n_epochs=args.n_epochs, gamma=0.99, gae_lambda=0.95, clip_range=0.15, 
                             ent_coef=0.02, vf_coef=0.5, max_grad_norm=0.5, 
                             seed=args.seed, policy_kwargs=policy_kwargs, device="auto" )
        if _TRAINING_STEP_DEBUG_GLOBAL: print("New model created.") # <<< Use global flag >>>

    # Define logger_output_formats unconditionally
    logger_output_formats = ["csv", "tensorboard"] 
    if _TRAINING_STEP_DEBUG_GLOBAL:
        logger_output_formats.insert(0, "stdout") # Add stdout only if debug is on
    
    new_logger = configure_logger(log_dir, logger_output_formats) # new_logger defined from above dynamic formats
    model.set_logger(new_logger)

    lr_value_at_start = model.lr_schedule(1.0) if callable(model.lr_schedule) else model.learning_rate
    if _TRAINING_STEP_DEBUG_GLOBAL: # <<< Use global flag for these important info prints >>>
        print(f"\n=== Starting Training ({type(model).__name__}) ===")
        print(f"  Total Timesteps: {args.timesteps}")
        print(f"  Environments:    {env.num_envs}")
        print(f"  Steps per Env:   {model.n_steps}")
        print(f"  Total Rollout:   {model.n_steps * env.num_envs}")
        print(f"  Minibatch Size:  {model.batch_size}")
        print(f"  PPO Epochs:      {model.n_epochs}")
        print(f"  Learning Rate:   {lr_value_at_start:.6f}")
    verify_tensorboard(log_dir) # This has its own prints

    interrupted = False; start_time = time.time()
    try:
        reset_ts = not is_continuing or model.num_timesteps == 0
        if model.num_timesteps > 0 : reset_ts = False
        
        if _TRAINING_STEP_DEBUG_GLOBAL: print(f"Calling model.learn(total_timesteps={args.timesteps}, reset_num_timesteps={reset_ts})") # <<< Use global flag >>>
        model.learn( total_timesteps=args.timesteps, callback=callbacks, log_interval=1, 
                     tb_log_name="MaskablePPO_Illuvium", reset_num_timesteps=reset_ts )
    except (KeyboardInterrupt, SystemExit): 
        print(f"\n!!! Training interrupted by user !!!"); interrupted = True
    except (EOFError, ConnectionResetError, RuntimeError) as pipe_err:
        print(f"\n!!! Training error (Pipe/Runtime): {pipe_err} !!!")
        print(">>> This often indicates a worker process crashed.")
        print(">>> Check worker logs in './worker_logs/' for details (e.g., env errors).")
        traceback.print_exc(); interrupted = True
    except ValueError as e:
         print(f"\n!!! Training error (ValueError): {e} !!!"); traceback.print_exc(); interrupted = True
         if "Expected parameter probs" in str(e): print("\n\n>>> This specific ValueError often indicates an issue with action masking or invalid actions being chosen despite masks.\n\n")
    except Exception as e: print(f"\n!!! Training error (Unexpected): {e} !!!"); traceback.print_exc(); interrupted = True
    finally:
        training_duration = time.time() - start_time
        if _TRAINING_STEP_DEBUG_GLOBAL: print(f"\nTraining finished or interrupted after {training_duration:.2f} seconds.") # <<< Use global flag >>>
        if 'model' in locals() and model is not None:
            save_sfx = "INTERRUPTED" if interrupted else "final"; ts_val = getattr(model, 'num_timesteps', 'unknown'); final_model_name = f"illuvium_maskable_ppo_{ts_val}_{save_sfx}.zip"; final_model_path = os.path.join(model_dir, final_model_name)
            if _TRAINING_STEP_DEBUG_GLOBAL: print(f"Saving {save_sfx} model to: {final_model_path}") # <<< Use global flag >>>
            try: 
                model.save(final_model_path); 
                if _TRAINING_STEP_DEBUG_GLOBAL: print("Model saved successfully.") # <<< Use global flag >>>
            except Exception as e_save_final: print(f"[ERROR] Final model save failed: {e_save_final}")
        else: 
            if _TRAINING_STEP_DEBUG_GLOBAL: print("\nModel object not available, cannot save final model.") # <<< Use global flag >>>
        
        if _TRAINING_STEP_DEBUG_GLOBAL: print("Closing environment...") # <<< Use global flag >>>
        if 'env' in locals() and env is not None and hasattr(env, 'closed') and not env.closed:
            try: 
                env.close(); 
                if _TRAINING_STEP_DEBUG_GLOBAL: print("Environment closed.") # <<< Use global flag >>>
            except Exception as e_env_close: print(f"[ERROR] Environment close failed: {e_env_close}")
        else: 
            if _TRAINING_STEP_DEBUG_GLOBAL: print("Environment unavailable or already closed.") # <<< Use global flag >>>

if __name__ == "__main__":
    multiprocessing.freeze_support()
    train()