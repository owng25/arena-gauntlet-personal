# --- START OF FILE sync_vec_env.py ---

# sync_vec_env.py
import time
import torch
import os
import re
from typing import List, Tuple, Dict, Optional, Any, Callable, Sequence, Union
from collections import defaultdict
import numpy as np
import gymnasium as gym
from stable_baselines3.common.vec_env import SubprocVecEnv, VecEnv
from stable_baselines3.common.vec_env.base_vec_env import VecEnvObs, VecEnvStepReturn, VecEnvIndices
import multiprocessing
import sys
import traceback
import json
from pathlib import Path
import queue # For checking pipe status non-blockingly
import logging

# --- Constants and Utilities from local modules ---
try:
    from train_env import TRAINING_STEP_DEBUG, DETAILED_SELFPLAY_DEBUG, TOTAL_FLAT_ACTIONS
    g_detailed_selfplay_debug = DETAILED_SELFPLAY_DEBUG # This controls logger.debug
    g_training_step_debug = TRAINING_STEP_DEBUG # This controls direct print statements
    g_total_flat_actions = TOTAL_FLAT_ACTIONS
except ImportError as e_train_env:
    print(f"[WARN sync_vec_env] Could not import flags from train_env: {e_train_env}. Using default constants.", file=sys.__stderr__, flush=True)
    g_detailed_selfplay_debug = False
    g_training_step_debug = False # Fallback to False if import fails
    g_total_flat_actions = 0

try:
    from simulation_utils import run_batch_simulation
except ImportError as e_sim_utils:
    if g_training_step_debug: # Only print this warning if debug is on
        print(f"[WARN sync_vec_env] Could not import from simulation_utils: {e_sim_utils}. Using fallback run_batch_simulation.", file=sys.__stderr__, flush=True)
    def run_batch_simulation(paths: List[str]) -> Dict[str, Any]:
        if g_training_step_debug: # Only print this warning if debug is on
            print("[WARN sync_vec_env] Using DUMMY run_batch_simulation - returns empty dict.", file=sys.__stderr__, flush=True)
        return {}

def contains_numpy(obj: Any) -> bool:
    if isinstance(obj, np.ndarray): return True
    elif isinstance(obj, (np.integer, np.floating, np.bool_)): return True
    elif isinstance(obj, dict): return any(contains_numpy(v) for v in obj.values())
    elif isinstance(obj, (list, tuple)): return any(contains_numpy(elem) for elem in obj)
    elif isinstance(obj, (int, float, bool, str, type(None), Path)): return False
    else:
        try:
            json.dumps(obj)
            return False
        except TypeError:
            return True

# --- Custom Worker Function with Python Logging ---
def _custom_worker_with_logging(remote: multiprocessing.connection.Connection, parent_remote: multiprocessing.connection.Connection, env_fn_wrapper: Callable[[], gym.Env], worker_id: int) -> None:
    parent_remote.close()
    if g_training_step_debug: # <<< Conditional Print >>>
        print(f"[RAW W{worker_id} WORKER_INIT PID:{os.getpid()}] Logging Worker started. Attempting log setup.", file=sys.__stderr__, flush=True)
    
    logger = logging.getLogger(f"Worker{worker_id}")
    logger.setLevel(logging.DEBUG if g_detailed_selfplay_debug else logging.INFO) # Controls logger verbosity
    log_filename = os.path.abspath(f"./worker_logs/worker_{worker_id}_custom_output.log")
    
    fh = None
    try:
        if os.path.exists(log_filename): os.remove(log_filename)
        os.makedirs(os.path.dirname(log_filename), exist_ok=True)
        fh = logging.FileHandler(log_filename, mode='w')
        fh.setLevel(logging.DEBUG if g_detailed_selfplay_debug else logging.INFO)
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(process)d - %(levelname)s - %(message)s')
        fh.setFormatter(formatter)
        logger.addHandler(fh)
        
        def excepthook(exc_type, exc_value, exc_traceback):
            logger.critical("Unhandled exception in worker:", exc_info=(exc_type, exc_value, exc_traceback))
        sys.excepthook = excepthook
        logger.info(f"Logging initialized. Log file: {log_filename}")
    except Exception as log_setup_e:
        # If logging setup fails, these raw prints are the fallback
        print(f"[RAW W{worker_id} WORKER_INIT CRITICAL_ERR PID:{os.getpid()}] Failed to set up Python logging: {log_setup_e}", file=sys.__stderr__, flush=True)
        print(traceback.format_exc(), file=sys.__stderr__, flush=True)

    env: Optional[gym.Env] = None
    try:
        logger.info(f"Attempting to create environment using env_fn_wrapper.")
        env = env_fn_wrapper()
        logger.info(f"Environment created successfully. Type: {type(env)}")

        while True: 
            try:
                if g_detailed_selfplay_debug: logger.debug("Waiting for command...")
                cmd, data = remote.recv()
                if g_detailed_selfplay_debug: logger.debug(f"Received command: '{cmd}'")

                if cmd == "step":
                    if g_detailed_selfplay_debug: logger.debug(f"step_recv. Action: {data}")
                    obs, reward, terminated, truncated, info = env.step(data)
                    if g_detailed_selfplay_debug: logger.debug(f"step_done. Term: {terminated}, Trunc: {truncated}, Info keys: {list(info.keys()) if isinstance(info,dict) else 'N/A'}")
                    if isinstance(obs, np.ndarray) and not np.all(np.isfinite(obs)):
                        logger.warning("Observation from step contains NaN/Inf. Clamping.")
                        obs = np.nan_to_num(obs, nan=0.0, posinf=1e6, neginf=-1e6)
                    remote.send((obs, reward, terminated, truncated, info))
                elif cmd == "reset":
                    if g_detailed_selfplay_debug: logger.debug(f"reset_recv. Data: {data}")
                    seed, opts = data if isinstance(data, tuple) and len(data)==2 else (data if not isinstance(data, dict) else None, data if isinstance(data, dict) else {})
                    obs, info = env.reset(seed=seed, options=opts)
                    if g_detailed_selfplay_debug: logger.debug(f"reset_done. Obs shape: {getattr(obs, 'shape', 'N/A')}, Info keys: {list(info.keys()) if isinstance(info, dict) else 'N/A'}")
                    if isinstance(obs, np.ndarray) and not np.all(np.isfinite(obs)):
                        logger.warning("Observation from reset contains NaN/Inf. Clamping.")
                        obs = np.nan_to_num(obs, nan=0.0, posinf=1e6, neginf=-1e6)
                    remote.send((obs, info))
                elif cmd == "apply_sim_results":
                    if g_detailed_selfplay_debug: logger.debug(f"apply_sim_results_recv. Data keys: {list(data.keys()) if isinstance(data, dict) else 'N/A'}")
                    env.apply_all_simulation_results(data)
                    if g_detailed_selfplay_debug: logger.debug("apply_sim_results_done.")
                    remote.send(True)
                elif cmd == "get_final_data":
                    if g_detailed_selfplay_debug: logger.debug("get_final_data_recv.")
                    res = env.get_final_step_data()
                    if g_detailed_selfplay_debug: logger.debug(f"get_final_data_done. Result term: {res[1] if isinstance(res,tuple) and len(res)>1 else 'N/A'}")
                    remote.send(res)
                elif cmd == "get_battle_files":
                    if g_detailed_selfplay_debug: logger.debug("get_battle_files_recv.")
                    res = env.get_battle_files_and_clear()
                    if g_detailed_selfplay_debug: logger.debug(f"get_battle_files_done. Found {len(res)} files.")
                    remote.send(res)
                elif cmd == "set_opponent_configs_dict":
                    if g_detailed_selfplay_debug and logger:
                        log_data_worker = {}
                        if isinstance(data, dict):
                            log_data_worker = {
                                k: (stype, os.path.basename(ident) if stype == "path" and ident else (ident if ident else "None"))
                                for k, (stype, ident) in data.items()
                            }
                        logger.debug(f"set_opponent_configs_dict_recv. Data: {log_data_worker}")
                    
                    if hasattr(env, 'set_opponent_configs_dict'):
                        env.set_opponent_configs_dict(data) # type: ignore
                    else:
                        if logger: logger.error("Environment in worker does not have 'set_opponent_configs_dict' method.")
                        if hasattr(env, 'set_opponent_config') and isinstance(data, dict):
                            if logger: logger.warning("Falling back to individual set_opponent_config calls in worker.")
                            for opp_id_worker, config_tuple_worker in data.items():
                                if isinstance(config_tuple_worker, tuple) and len(config_tuple_worker) == 2:
                                    stype_worker, ident_worker = config_tuple_worker
                                    env.set_opponent_config(opp_id_worker, stype_worker, ident_worker) # type: ignore
                                else:
                                    if logger: logger.error(f"Invalid config_tuple for opp_id {opp_id_worker} in fallback: {config_tuple_worker}")
                        elif logger:
                             logger.critical("Environment has neither set_opponent_configs_dict nor set_opponent_config (or data is not dict).")

                    if g_detailed_selfplay_debug and logger: logger.debug("set_opponent_configs_dict_done.")
                    remote.send(True)
                elif cmd == "env_method":
                    method_name, method_args, method_kwargs = data
                    if g_detailed_selfplay_debug: logger.debug(f"env_method_recv: {method_name}, Args: {method_args}, Kwargs: {method_kwargs}")
                    method = getattr(env, method_name)
                    result = method(*method_args, **method_kwargs)
                    if g_detailed_selfplay_debug: logger.debug(f"env_method_done: '{method_name}'. Result type: {type(result)}")
                    remote.send(result)
                elif cmd == "get_attr":
                    attr_name = data
                    if g_detailed_selfplay_debug: logger.debug(f"get_attr_recv: {attr_name}")
                    result = getattr(env, attr_name)
                    if g_detailed_selfplay_debug: logger.debug(f"get_attr_done: '{attr_name}'. Result type: {type(result)}")
                    remote.send(result)
                elif cmd == "set_attr":
                    attr_name, value = data
                    if g_detailed_selfplay_debug: logger.debug(f"set_attr_recv: {attr_name}, Value type: {type(value)}")
                    setattr(env, attr_name, value)
                    if g_detailed_selfplay_debug: logger.debug(f"set_attr_done: '{attr_name}'.")
                    remote.send(True)
                elif cmd == "has_attr":
                    attr_name = data
                    if g_detailed_selfplay_debug: logger.debug(f"has_attr_recv: {attr_name}")
                    result = hasattr(env, attr_name)
                    if g_detailed_selfplay_debug: logger.debug(f"has_attr_done for '{attr_name}'. Result: {result}")
                    remote.send(result)
                elif cmd == "close":
                    logger.info("close_recv. Closing environment and remote connection.")
                    if env and hasattr(env, 'close'): env.close()
                    remote.close()
                    if g_training_step_debug: print(f"[RAW W{worker_id} CUSTOM_WORKER_EXIT PID:{os.getpid()}] Worker breaking from command loop.", file=sys.__stderr__, flush=True) # Added log
                    break 
                else:
                    logger.error(f"Unknown command received: '{cmd}'")
                    remote.send(("error", (NotImplementedError(f"Unknown command {cmd} in worker {worker_id}"), traceback.format_exc())))
            except EOFError: 
                if g_training_step_debug: print(f"[RAW W{worker_id} CUSTOM_WORKER_EXIT PID:{os.getpid()}] EOFError in command loop. Exiting.", file=sys.__stderr__, flush=True) # Added log
                break
            except BrokenPipeError: 
                if g_training_step_debug: print(f"[RAW W{worker_id} CUSTOM_WORKER_EXIT PID:{os.getpid()}] BrokenPipeError in command loop. Exiting.", file=sys.__stderr__, flush=True) # Added log
                break
            except Exception as loop_e: 
                logger.error(f"Exception in command loop (cmd='{cmd if 'cmd' in locals() else 'unknown'}'): {type(loop_e).__name__}: {loop_e}")
                logger.error(traceback.format_exc())
                if not remote.closed:
                    try: remote.send(("error", (loop_e, traceback.format_exc())))
                    except Exception as send_final_err_e: logger.error(f"Failed to send final error signal from loop: {send_final_err_e}")
                if g_training_step_debug: print(f"[RAW W{worker_id} CUSTOM_WORKER_EXIT PID:{os.getpid()}] Worker exiting due to exception.", file=sys.__stderr__, flush=True) # Added log
                break 
    except Exception as main_worker_e: 
        log_target = logger if 'logger' in locals() and logger is not None and (fh is not None and fh in logger.handlers) else None
        if log_target:
            log_target.critical(f"Worker (PID: {os.getpid()}) CRASHED during init/env_creation: {type(main_worker_e).__name__}: {main_worker_e}")
            log_target.critical(traceback.format_exc())
        else: 
            print(f"[RAW W{worker_id} WORKER_INIT CRITICAL_ERR PID:{os.getpid()}] Worker crashed before logging setup: {type(main_worker_e).__name__}: {main_worker_e}\n{traceback.format_exc()}", file=sys.__stderr__, flush=True)
        if 'remote' in locals() and remote and not remote.closed:
            try: remote.send(("error", (main_worker_e, traceback.format_exc())))
            except Exception: pass
    finally:
        final_msg_prefix = f"[W{worker_id} CUSTOM_WORKER finally PID {os.getpid()}]"
        log_target_finally = logger if 'logger' in locals() and logger is not None and (fh is not None and fh in logger.handlers) else None
        if log_target_finally:
            log_target_finally.info(f"{final_msg_prefix} Worker finally block.")
            if env and hasattr(env, 'close'):
                try: log_target_finally.info("Closing environment in finally."); env.close()
                except Exception as final_env_close_e: log_target_finally.error(f"Exception during final env.close(): {final_env_close_e}")
            logging.shutdown() 
        else:
            if g_training_step_debug: print(f"{final_msg_prefix} Worker finally block (logger not fully available).", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
            if env and hasattr(env, 'close'):
                try: 
                    if g_training_step_debug: print(f"{final_msg_prefix} Closing environment (fallback print).", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
                    env.close()
                except Exception as e: 
                    if g_training_step_debug: print(f"{final_msg_prefix} Error closing env (fallback print): {e}", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
        if 'remote' in locals() and remote and not remote.closed:
            try: remote.close()
            except Exception: pass
        if g_training_step_debug: print(f"[RAW W{worker_id} CUSTOM_WORKER_EXIT PID:{os.getpid()}] Worker exiting.", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>


# --- Synchronous Simulation VecEnv (Using your original class structure and methods) ---
class SyncSimulationVecEnv(SubprocVecEnv):
    def __init__(self, env_fns: List[Callable[[], gym.Env]], start_method: Optional[str] = None):
        self.waiting = False; self.closed = False; n_envs = len(env_fns)
        self.n_envs = n_envs; self.num_envs = n_envs
        
        # --- Start method selection logic (from our refined version) ---
        if start_method is None:
            start_method_check = multiprocessing.get_start_method(allow_none=True)
            if start_method_check is None : start_method = "forkserver" if "forkserver" in multiprocessing.get_all_start_methods() else "spawn"
            else: start_method = start_method_check 
            if sys.platform == "darwin" and start_method == "fork":
                if g_training_step_debug: print(f"[SyncVecEnv Init] macOS detected with 'fork' default. Forcing to 'spawn'.", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
                start_method = "spawn"
        if g_training_step_debug: print(f"[SyncVecEnv Init] Effective start method for multiprocessing context: '{start_method}'.", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
        try:
            current_mp_method = multiprocessing.get_start_method(allow_none=True)
            if current_mp_method is None and start_method in multiprocessing.get_all_start_methods():
                multiprocessing.set_start_method(start_method, force=False)
            elif current_mp_method != start_method :
                 if g_training_step_debug: print(f"[WARN SyncVecEnv Init] Main process start method ('{current_mp_method}') differs from preferred ('{start_method}'). Using '{current_mp_method}' for context.", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
                 start_method = current_mp_method
        except RuntimeError: 
            if g_training_step_debug: print(f"[INFO SyncVecEnv Init] Multiprocessing context likely already set ('{multiprocessing.get_start_method()}').", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
            start_method = multiprocessing.get_start_method()
        except Exception as e_set_start: 
            if g_training_step_debug: print(f"[WARN SyncVecEnv Init] Error managing start method: {e_set_start}. Using current: '{multiprocessing.get_start_method(allow_none=True)}'.", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
            start_method = multiprocessing.get_start_method(allow_none=True) or "spawn"

        ctx = multiprocessing.get_context(start_method)
        self.remotes, self.work_remotes = zip(*[ctx.Pipe() for _ in range(n_envs)]); self.processes = []; self.error_flags = [False] * n_envs
        os.makedirs("./worker_logs", exist_ok=True)
        if g_training_step_debug: print(f"[SyncVecEnv Init] Creating {n_envs} workers using '{start_method}' context...", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>

        for i, (work_remote, remote, env_fn) in enumerate(zip(self.work_remotes, self.remotes, env_fns)):
             args = (work_remote, remote, env_fn, i)
             process = ctx.Process(target=_custom_worker_with_logging, args=args, daemon=True)
             if g_training_step_debug: print(f"[SyncVecEnv Init] Starting worker {i} (target: _custom_worker_with_logging)...", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
             process.start(); self.processes.append(process); work_remote.close()
        
        if g_training_step_debug: print(f"[SyncVecEnv Init] Getting observation/action spaces from worker 0...", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
        try:
            # This is where the 'Worker previously errored' comes from if worker 0 failed early.
            # No need to add g_training_step_debug here, as _get_result_from_remote already handles it.
            self.remotes[0].send(("get_attr", "observation_space"));
            obs_space_recv = self._get_result_from_remote(0, timeout=240.0) # Increased timeout
            self.observation_space = obs_space_recv

            self.remotes[0].send(("get_attr", "action_space"));
            act_space_recv = self._get_result_from_remote(0, timeout=240.0) # Increased timeout
            self.action_space = act_space_recv
            
            if hasattr(self.action_space, 'n') and isinstance(self.action_space, gym.spaces.Discrete):
                self._action_mask_len = self.action_space.n
            elif g_total_flat_actions > 0:
                 self._action_mask_len = g_total_flat_actions
                 if g_training_step_debug: print(f"[WARN SyncVecEnv Init] Using g_total_flat_actions ({g_total_flat_actions}) for _action_mask_len as action_space is not Discrete or has no 'n'.", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
            else:
                self._action_mask_len = 0 
                if g_training_step_debug: print(f"[CRITICAL WARN SyncVecEnv Init] _action_mask_len is 0. Masking will likely fail if env requires masks.", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
            if g_training_step_debug: print(f"  Obs Space (Worker 0): {self.observation_space}", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
            if g_training_step_debug: print(f"  Act Space (Worker 0): {self.action_space}, Action Mask Len: {self._action_mask_len}", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>

        except Exception as e: 
            print(f"[FATAL ERROR SyncVecEnv Init] Failed to get spaces from worker 0: {type(e).__name__}: {e}", file=sys.__stderr__, flush=True) # Keep FATAL
            if not (isinstance(e, RuntimeError) and "Worker 0 error" in str(e)):
                traceback.print_exc(file=sys.__stderr__)
            self.close()
            raise RuntimeError(f"Could not initialize VecEnv spaces from worker 0: {e}") from e
        
        self._seeds = [None] * n_envs; self._options = [{} for _ in range(self.n_envs)]; self.waiting_for_sim = False
        if g_training_step_debug: print("[SyncVecEnv Init] Initialization complete.", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>

    # --- _get_result_from_remote (Updated with robust logging and checks) ---
    def _get_result_from_remote(self, remote_idx: int, timeout: Optional[float] = 60.0) -> Any:
        pid_str = f"(PID:{self.processes[remote_idx].pid if self.processes[remote_idx] and hasattr(self.processes[remote_idx], 'pid') else 'N/A'})"
        
        if self.error_flags[remote_idx]: # This means a prior call flagged it.
            # This is not where the flag gets set, but where it's detected and prevented from proceeding.
            # The 'Early exit' log now correctly occurs *before* this raise.
            raise RuntimeError(f"Worker {remote_idx} previously errored and is flagged.")
        
        if not self.processes[remote_idx].is_alive():
            if g_training_step_debug: print(f"[DEBUG SyncVecEnv GetResult W{remote_idx} {pid_str}] Setting error_flags=True BECAUSE process NOT ALIVE.", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
            self.error_flags[remote_idx] = True
            raise ConnectionResetError(f"Worker {remote_idx} process {pid_str} is not alive.")

        try:
            if timeout is not None:
                if not self.remotes[remote_idx].poll(timeout):
                    if g_training_step_debug: print(f"[DEBUG SyncVecEnv GetResult W{remote_idx} {pid_str}] Setting error_flags=True BECAUSE poll TIMEOUT ({timeout}s).", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
                    self.error_flags[remote_idx] = True
                    raise TimeoutError(f"Timeout ({timeout}s) waiting for response from worker {remote_idx} {pid_str}")
            
            result = self.remotes[remote_idx].recv()
            
            if g_detailed_selfplay_debug: # This controls logger.debug
                 result_log_value = result[0] if isinstance(result, tuple) else result
                 
                 if isinstance(result_log_value, np.ndarray) and result_log_value.dtype == bool and result_log_value.size > 100:
                     if g_training_step_debug: # <<< Conditional Print >>>
                         print(f"[DEBUG SyncVecEnv GetResult W{remote_idx} {pid_str}] Received action mask (len: {result_log_value.size}, sum: {np.sum(result_log_value)}).", file=sys.__stderr__, flush=True)
                 else:
                     if g_training_step_debug: # <<< Conditional Print >>>
                         print(f"[DEBUG SyncVecEnv GetResult W{remote_idx} {pid_str}] Received result. Type: {type(result)}, Value (first part): {result_log_value}...", file=sys.__stderr__, flush=True)

            is_error_signal = (isinstance(result, tuple) and len(result) == 2 and isinstance(result[0], str) and
                result[0] == "error" and isinstance(result[1], tuple) and len(result[1]) == 2 and
                isinstance(result[1][0], Exception))

            if is_error_signal:
                e_worker, tb_str_worker = result[1]
                if g_training_step_debug: print(f"[DEBUG SyncVecEnv GetResult W{remote_idx} {pid_str}] Setting error_flags=True BECAUSE worker SENT EXPLICIT ERROR.", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
                print(f"[FATAL ERROR SYNC_VEC_ENV _get_result_from_remote] Worker {remote_idx} {pid_str} sent explicit error signal: {type(e_worker).__name__}: {e_worker}\nTraceback from worker:\n{tb_str_worker}", file=sys.__stderr__, flush=True) # Keep FATAL
                self.error_flags[remote_idx] = True
                raise RuntimeError(f"Worker {remote_idx} error from worker: {e_worker}") from e_worker
            return result
        except (EOFError, ConnectionResetError, BrokenPipeError) as e_pipe:
            if g_training_step_debug: print(f"[DEBUG SyncVecEnv GetResult W{remote_idx} {pid_str}] Setting error_flags=True BECAUSE PIPE ERROR ({type(e_pipe).__name__}).", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
            print(f"[FATAL ERROR SYNC_VEC_ENV _get_result_from_remote] Pipe error Worker {remote_idx} {pid_str}: {type(e_pipe).__name__}: {e_pipe}. Process likely dead.", file=sys.__stderr__, flush=True) # Keep FATAL
            self.error_flags[remote_idx] = True
            raise ConnectionResetError(f"Pipe error with worker {remote_idx}") from e_pipe
        except TimeoutError as e_timeout_reraised:
            if g_training_step_debug: print(f"[DEBUG SyncVecEnv GetResult W{remote_idx} {pid_str}] Setting error_flags=True BECAUSE TIMEOUT EXCEPTION re-raised.", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
            if not self.error_flags[remote_idx]: self.error_flags[remote_idx] = True
            raise e_timeout_reraised
        except Exception as e_unexpected:
            if g_training_step_debug: print(f"[DEBUG SyncVecEnv GetResult W{remote_idx} {pid_str}] Setting error_flags=True BECAUSE UNEXPECTED EXCEPTION ({type(e_unexpected).__name__}).", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
            print(f"[FATAL ERROR SYNC_VEC_ENV _get_result_from_remote] Unexpected error Worker {remote_idx} {pid_str}: {type(e_unexpected).__name__}: {e_unexpected}", file=sys.__stderr__, flush=True) # Keep FATAL
            traceback.print_exc(file=sys.__stderr__)
            self.error_flags[remote_idx] = True
            raise RuntimeError(f"Unexpected error receiving from worker {remote_idx}") from e_unexpected

    def step_async(self, actions: np.ndarray) -> None:
        if g_training_step_debug: print("[WARN SYNC_VEC_ENV] step_async called while waiting! Previous actions might be overwritten.", file=sys.__stderr__) # <<< Conditional Print >>>
        self.actions = actions
        for i, remote in enumerate(self.remotes):
            if self.processes[i].is_alive() and not self.error_flags[i]:
                try: remote.send(("step", self.actions[i]))
                except (BrokenPipeError, EOFError) as e: 
                    print(f"[ERROR SYNC_VEC_ENV W{i}] Pipe error sending step: {e}. Marking as errored.", file=sys.__stderr__) # Keep ERROR
                    self.error_flags[i] = True
            elif not self.processes[i].is_alive() and not self.error_flags[i]:
                print(f"[WARN SYNC_VEC_ENV W{i}] Found dead before sending step command. Flagging.", file=sys.__stderr__) # Keep WARN
                self.error_flags[i] = True
        self.waiting = True; self.waiting_for_sim = True

    def step_wait(self) -> VecEnvStepReturn:
        if g_training_step_debug: print(f"\n--- [SyncVecEnv step_wait START] Waiting: {self.waiting}, WaitingForSim: {self.waiting_for_sim} ---", file=sys.__stderr__)
        if not self.waiting or not self.waiting_for_sim:
             print("[ERROR SYNC_VEC_ENV] step_wait called without step_async or not waiting for sim cycle! Attempting recovery.", file=sys.__stderr__)
             self.waiting = False; self.waiting_for_sim = False
             dummy_obs = self._stack_observations([np.zeros_like(self.observation_space.sample()) for _ in range(self.num_envs)])
             dummy_rews = np.zeros(self.num_envs, dtype=np.float32)
             dummy_dones = np.ones(self.num_envs, dtype=bool)
             dummy_infos = [{"error": "step_wait_desync_detected", "env_forced_reset":True} for _ in range(self.num_envs)]
             for i in range(self.num_envs): dummy_infos[i]["episode"] = {"r": 0.0, "l": 0, "t": time.time(), "w": 0.0}
             return dummy_obs, dummy_rews, dummy_dones, dummy_infos

        self.waiting = False; self.waiting_for_sim = False
        prelim_obs_list = [np.zeros_like(self.observation_space.sample()) for _ in range(self.num_envs)]
        prelim_rewards = np.zeros(self.num_envs, dtype=np.float32)
        prelim_terminated = np.ones(self.num_envs, dtype=bool) # This line initializes to True, but will be overwritten
        prelim_truncated = np.zeros(self.num_envs, dtype=bool)
        prelim_infos = [{"error": "worker_did_not_respond", "worker_crashed": True} for _ in range(self.num_envs)] # Default error info
        
        if g_training_step_debug: print(f"[SyncVecEnv DBG step_wait 1] Receiving {self.num_envs} preliminary results...", file=sys.__stderr__)
        for i in range(self.num_envs):
            if not self.error_flags[i] and self.processes[i].is_alive():
                try:
                    result = self._get_result_from_remote(i, timeout=20.0)
                    if isinstance(result, tuple) and len(result) == 5:
                        obs, rew, term, trunc, info = result
                        
                        # VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
                        # CRITICAL FIX: Assign the received values here!
                        prelim_rewards[i] = rew
                        prelim_terminated[i] = term
                        prelim_truncated[i] = trunc
                        prelim_infos[i] = info
                        # ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

                        if isinstance(obs, np.ndarray) and obs.shape == self.observation_space.shape:
                            prelim_obs_list[i] = obs
                            if not np.all(np.isfinite(obs)): 
                                if g_training_step_debug: print(f"[WARN SYNC_VEC_ENV W{i}] Prelim step obs NaN/Inf. Clamped.", file=sys.__stderr__);
                                prelim_obs_list[i] = np.nan_to_num(obs, nan=0.0, posinf=1e6, neginf=-1e6)
                        else: 
                            print(f"[ERROR SYNC_VEC_ENV W{i}] Invalid prelim obs format. Using zeros.", file=sys.__stderr__); 
                            prelim_infos[i]["error"] = "invalid_observation_format_obs"; self.error_flags[i] = True
                            prelim_terminated[i] = True # If obs is bad, consider it done
                    else: 
                        print(f"[FATAL ERROR SYNC_VEC_ENV W{i}] Unexpected prelim step result format: {type(result)}. Errored.", file=sys.__stderr__); 
                        self.error_flags[i] = True
                        prelim_terminated[i] = True # If result format is bad, consider it done
                except Exception as e: 
                    print(f"[ERROR SYNC_VEC_ENV W{i}] Error receiving prelim result: {e}. (error_flags[{i}] was {self.error_flags[i]})", file=sys.__stderr__)
                    if not self.error_flags[i]: 
                         if g_training_step_debug: print(f"[DEBUG SyncVecEnv step_wait W{i}] Setting error_flags=True BECAUSE EXCEPTION IN PRELIM RESULT RECV (not from _get_result_from_remote).", file=sys.__stderr__, flush=True)
                         self.error_flags[i] = True
                    prelim_terminated[i] = True # If an exception occurred, consider it done
            elif not self.error_flags[i]: # Worker dead but not flagged yet
                 print(f"[WARN SYNC_VEC_ENV W{i}] Found dead before prelim result. Flagging.", file=sys.__stderr__); 
                 if g_training_step_debug: print(f"[DEBUG SyncVecEnv step_wait W{i}] Setting error_flags=True BECAUSE DEAD BEFORE PRELIM RESULT.", file=sys.__stderr__, flush=True)
                 self.error_flags[i] = True
                 prelim_terminated[i] = True # If worker is dead, consider it done
        
        # This line now correctly uses the actual 'term' values from the workers
        prelim_dones = np.logical_or(prelim_terminated, prelim_truncated) | np.array(self.error_flags)
        live_env_indices_PRE_SIM = [i for i, done in enumerate(prelim_dones) if not done]
        if g_training_step_debug: print(f"[SyncVecEnv DBG step_wait 1] Prelim dones: {prelim_dones.tolist()}, Live for sim: {live_env_indices_PRE_SIM}", file=sys.__stderr__)

        battle_files_to_run: List[str] = []; env_needs_sim_results: List[int] = []; battle_file_to_env_map: Dict[str, int] = {}
        if live_env_indices_PRE_SIM:
            for idx in live_env_indices_PRE_SIM:
                if not self.error_flags[idx] and self.processes[idx].is_alive():
                    try: self.remotes[idx].send(("get_battle_files", None))
                    except Exception as e: print(f"[ERROR SYNC_VEC_ENV W{idx}] Pipe error sending get_battle_files: {e}", file=sys.__stderr__); self.error_flags[idx] = True # Keep ERROR
            
            for idx in live_env_indices_PRE_SIM:
                if not self.error_flags[idx] and self.processes[idx].is_alive():
                    try:
                        files_list_result = self._get_result_from_remote(idx, timeout=30.0)
                        if isinstance(files_list_result, list) and files_list_result:
                            env_needs_sim_results.append(idx)
                            for file_path in files_list_result:
                                 if isinstance(file_path, str) and os.path.exists(file_path): 
                                     battle_files_to_run.append(file_path); base = Path(file_path).stem; battle_file_to_env_map[base] = idx
                    except Exception as e: 
                        print(f"[ERROR SYNC_VEC_ENV W{idx}] Error getting battle files: {e}.", file=sys.__stderr__); 
                        if not self.error_flags[idx]: 
                             if g_training_step_debug: print(f"[DEBUG SyncVecEnv step_wait W{idx}] Setting error_flags=True BECAUSE EXCEPTION GETTING BATTLE FILES.", file=sys.__stderr__, flush=True)
                             self.error_flags[idx] = True
        if g_training_step_debug: print(f"[SyncVecEnv DBG step_wait 2] Files to run: {len(battle_files_to_run)}. Envs needing sim: {env_needs_sim_results}", file=sys.__stderr__)

        simulation_results: Dict[str, Any] = {}
        if battle_files_to_run:
            if g_training_step_debug: print(f"[SyncVecEnv DBG step_wait 3] Running sim for {len(battle_files_to_run)} battles...", file=sys.__stderr__)
            try: 
                simulation_results = run_batch_simulation(battle_files_to_run)
                sim_errors = simulation_results.pop('__error__', None)
                if sim_errors and g_training_step_debug: 
                    print(f"[WARN SYNC_VEC_ENV step_wait 3] Sim errors: {sim_errors}", file=sys.__stderr__) 
            except Exception as e: print(f"[FATAL SYNC_VEC_ENV step_wait 3] run_batch_simulation failed: {e}", file=sys.__stderr__); self.close(); raise 
            if g_training_step_debug: print(f"[SyncVecEnv DBG step_wait 3] Sim complete. Results keys: {len(simulation_results)}", file=sys.__stderr__)
        
        results_by_env: Dict[int, Dict[str, Dict]] = defaultdict(dict)
        for base, data in simulation_results.items():
            if base in battle_file_to_env_map: results_by_env[battle_file_to_env_map[base]][base] = data
        
        indices_applied_sim = []
        if env_needs_sim_results:
             for idx in env_needs_sim_results:
                 if not self.error_flags[idx] and self.processes[idx].is_alive():
                    data_to_send = results_by_env.get(idx, {});
                    try: self.remotes[idx].send(("apply_sim_results", data_to_send)); indices_applied_sim.append(idx)
                    except Exception as e: print(f"[ERROR SYNC_VEC_ENV W{idx}] Pipe err send apply_sim: {e}", file=sys.__stderr__); self.error_flags[idx] = True 
             
             for idx in indices_applied_sim:
                 if not self.error_flags[idx] and self.processes[idx].is_alive():
                    try:
                        confirmation = self._get_result_from_remote(idx, timeout=30.0)
                        if confirmation is not True: 
                            if g_training_step_debug: print(f"[WARN SYNC_VEC_ENV W{idx}] Bad apply_sim confirm: {confirmation}. Flagging.", file=sys.__stderr__);
                            self.error_flags[idx] = True
                    except Exception as e: 
                        print(f"[ERROR SYNC_VEC_ENV W{idx}] Err get apply_sim confirm: {e}", file=sys.__stderr__); 
                        self.error_flags[idx] = True
        if g_training_step_debug: print(f"[SyncVecEnv DBG step_wait 4] Applied sim to envs: {indices_applied_sim}", file=sys.__stderr__)

        final_rewards = np.copy(prelim_rewards); final_dones = np.array(prelim_dones); final_infos = [info.copy() for info in prelim_infos]
        live_env_indices_FINAL = [i for i, err in enumerate(self.error_flags) if not err and self.processes[i].is_alive()]
        if g_training_step_debug: print(f"[SyncVecEnv DBG step_wait 5] Fetching final data from: {live_env_indices_FINAL}", file=sys.__stderr__)

        if live_env_indices_FINAL:
            for idx in live_env_indices_FINAL:
                try: self.remotes[idx].send(("get_final_data", None))
                except Exception as e: print(f"[ERROR SYNC_VEC_ENV W{idx}] Pipe err send get_final_data: {e}", file=sys.__stderr__); self.error_flags[idx] = True; final_dones[idx] = True 
            
            for idx in live_env_indices_FINAL:
                if not self.error_flags[idx] and self.processes[idx].is_alive():
                    try:
                        result = self._get_result_from_remote(idx, timeout=30.0)
                        if isinstance(result, tuple) and len(result) == 3:
                            rew_adj, done_after_sim, info_after_sim = result
                            final_rewards[idx] += float(rew_adj)
                            final_dones[idx] = final_dones[idx] or bool(done_after_sim)
                            final_infos[idx].update(info_after_sim if isinstance(info_after_sim, dict) else {"error_final_info_type":type(info_after_sim)})
                        else: 
                            print(f"[ERROR SYNC_VEC_ENV W{idx}] Invalid final_data format: {type(result)}. Flagging.", file=sys.__stderr__); 
                            self.error_flags[idx] = True; final_dones[idx] = True
                    except Exception as e: 
                        print(f"[ERROR SYNC_VEC_ENV W{idx}] Err get final_data: {e}", file=sys.__stderr__); 
                        self.error_flags[idx] = True; final_dones[idx] = True
        
        for i in range(self.num_envs):
            if final_dones[i] and "episode" not in final_infos[i]:
                final_infos[i]["episode"] = {"r": final_rewards[i], "l": 0, "t": time.time(), "w": 0.0}
            if self.error_flags[i] and not final_dones[i]: final_dones[i] = True
        if g_training_step_debug: print(f"[SyncVecEnv DBG step_wait 6] Final dones after all updates: {final_dones.tolist()}", file=sys.__stderr__)

        reset_env_indices = [i for i, done in enumerate(final_dones) if done]
        current_obs_list = list(prelim_obs_list)
        if reset_env_indices:
             if g_training_step_debug: print(f"[SyncVecEnv DBG step_wait 7] Resetting envs: {reset_env_indices}", file=sys.__stderr__)
             indices_to_reset_live = [i for i in reset_env_indices if not self.error_flags[i] and self.processes[i].is_alive()]
             
             if indices_to_reset_live:
                 for idx in indices_to_reset_live:
                     try: self.remotes[idx].send(("reset", (self._seeds[idx], self._options[idx])))
                     except Exception as e: print(f"[ERROR SYNC_VEC_ENV W{idx}] Pipe err send reset: {e}", file=sys.__stderr__); self.error_flags[idx] = True 
                 
                 for idx in indices_to_reset_live:
                      if not self.error_flags[idx] and self.processes[idx].is_alive():
                        try:
                            new_obs, new_info_reset = self._get_result_from_remote(idx, timeout=60.0)
                            if "terminal_observation" not in final_infos[idx]: final_infos[idx]["terminal_observation"] = current_obs_list[idx] 
                            current_obs_list[idx] = new_obs
                        except Exception as e: 
                            print(f"[ERROR SYNC_VEC_ENV W{idx}] Err recv reset result: {e}. Using old obs.", file=sys.__stderr__); 
                            self.error_flags[idx] = True; final_infos[idx]["reset_failed"] = True
            
             for i in reset_env_indices:
                if self.error_flags[i]: 
                    if "terminal_observation" not in final_infos[i] and i < len(current_obs_list): final_infos[i]["terminal_observation"] = current_obs_list[i]
                    current_obs_list[i] = np.zeros_like(self.observation_space.sample())
                    final_infos[i].setdefault("error", "reset_skipped_due_to_error_flag")

        try: final_obs_stacked = self._stack_observations(current_obs_list)
        except Exception as e: print(f"[FATAL SYNC_VEC_ENV step_wait] Stack final obs: {e}", file=sys.__stderr__); traceback.print_exc(); self.close(); raise 
        
        if g_training_step_debug: print(f"--- [SyncVecEnv step_wait END] ---", file=sys.__stderr__)
        return final_obs_stacked, final_rewards, final_dones, final_infos

    def reset(self, seed: Optional[Union[int, List[int]]] = None, options: Optional[List[dict]] = None) -> VecEnvObs:
        obs_list = [self.observation_space.sample() for _ in range(self.num_envs)]
        for i, remote in enumerate(self.remotes):
            current_seed_i = None
            if seed is not None: current_seed_i = seed[i] if isinstance(seed, list) and i < len(seed) else (seed + i if isinstance(seed, int) else None)
            self._seeds[i] = current_seed_i 
            current_options_i = (options[i] if isinstance(options, list) and i < len(options) else (options if isinstance(options, dict) else {}))
            self._options[i] = current_options_i
            
            if self.processes[i].is_alive() and not self.error_flags[i]:
                try:
                    remote.send(("reset", (self._seeds[i], self._options[i])))
                    obs_i, _ = self._get_result_from_remote(i, timeout=90.0)
                    obs_list[i] = obs_i
                except Exception as e: print(f"[ERROR SYNC_VEC_ENV reset W{i}] Failed: {type(e).__name__} - {e}", file=sys.__stderr__); self.error_flags[i] = True # Keep ERROR
            else: self.error_flags[i] = True
        return self._stack_observations(obs_list)

    def _stack_observations(self, obs_list: List[Any]) -> VecEnvObs:
        if not hasattr(self, 'observation_space') or self.observation_space is None :
             print("[ERROR SyncVecEnv _stack_observations] observation_space not initialized!", file=sys.__stderr__) # Keep ERROR
             return np.array([np.array([0.0]) for _ in obs_list]) if obs_list else np.array([])
        if not isinstance(self.observation_space, gym.spaces.Box): 
            if g_training_step_debug: print(f"[WARN SyncVecEnv _stack_observations] Not a Box space: {type(self.observation_space)}. Trying generic stack.", file=sys.__stderr__) # <<< Conditional Print >>>
            try: return np.stack(obs_list) if obs_list else np.array([])
            except Exception as e_stack: print(f"[ERROR SyncVecEnv _stack_observations] Generic stack failed: {e_stack}", file=sys.__stderr__); return np.array([np.array([0.0]) for _ in obs_list]) if obs_list else np.array([]) # Keep ERROR

        shape = self.observation_space.shape; dtype = self.observation_space.dtype
        if not obs_list: return np.zeros((0,) + shape, dtype=dtype)
        valid_obs = []
        for i, obs_item in enumerate(obs_list):
            if isinstance(obs_item, np.ndarray) and obs_item.shape == shape:
                valid_obs.append(obs_item.astype(dtype))
            else: valid_obs.append(np.zeros(shape, dtype=dtype))
        return np.stack(valid_obs)

    def set_opponent_configs_dict(self, configs_by_opp_id: Dict[int, Tuple[str, Optional[str]]]):
        """
        Sends opponent configurations (source_type, identifier) to worker environments.
        """
        indices_to_send = [i for i, err in enumerate(self.error_flags) if not err and self.processes[i].is_alive()]
        
        if g_training_step_debug: # <<< Conditional Print >>>
            log_data_vecenv = {}
            if isinstance(configs_by_opp_id, dict):
                log_data_vecenv = {
                    k: (stype, os.path.basename(ident) if stype == "path" and ident else (ident if ident else "None")) 
                    for k, (stype, ident) in configs_by_opp_id.items()
                }
            print(f"[DEBUG SyncVecEnv Send] Broadcasting 'set_opponent_configs_dict' to workers {indices_to_send} with: {log_data_vecenv}", file=sys.__stderr__, flush=True)
        
        for i in indices_to_send:
            if i < len(self.remotes):
                try:
                    self.remotes[i].send(("set_opponent_configs_dict", configs_by_opp_id))
                except Exception as e:
                    print(f"[ERROR SYNC_VEC_ENV W{i} set_opponent_configs] Pipe error sending: {e}", file=sys.__stderr__, flush=True) # Keep ERROR
                    self.error_flags[i] = True
            else:
                 print(f"[ERROR SYNC_VEC_ENV set_opponent_configs] Worker index {i} out of bounds for remotes.", file=sys.__stderr__, flush=True) # Keep ERROR
                 self.error_flags[i] = True
        
        for i in indices_to_send:
             if i < len(self.remotes) and not self.error_flags[i] and self.processes[i].is_alive():
                 try:
                     confirmation = self._get_result_from_remote(i, timeout=120.0) 
                     if confirmation is not True:
                         if g_training_step_debug: print(f"[WARN SYNC_VEC_ENV W{i} set_opponent_configs] Unexpected confirmation: {confirmation}. Flagging.", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
                         self.error_flags[i] = True
                 except Exception as e:
                     print(f"[ERROR SYNC_VEC_ENV W{i} set_opponent_configs] Error receiving confirmation: {e}", file=sys.__stderr__, flush=True) # Keep ERROR
                     self.error_flags[i] = True

    def has_attr(self, attr_name: str, indices: VecEnvIndices = None) -> List[bool]:
        target_remotes = self._get_target_remotes(indices); results = []
        for i, remote in enumerate(self.remotes):
            if remote in target_remotes:
                if self.error_flags[i] or not self.processes[i].is_alive(): results.append(False)
                else:
                    try: remote.send(("has_attr", attr_name)); results.append(bool(self._get_result_from_remote(i, timeout=5.0)))
                    except Exception: results.append(False)
        if indices is not None:
            full_results = [False] * self.num_envs; target_indices_list = self._get_indices(indices); res_idx = 0
            for i_env in range(self.num_envs):
                if i_env in target_indices_list:
                    if res_idx < len(results): full_results[i_env] = results[res_idx]; res_idx +=1
            return full_results
        return results

    def env_method(self, method_name: str, *method_args, indices: VecEnvIndices = None, **method_kwargs) -> List[Any]:
        target_remotes = self._get_target_remotes(indices); results = []
        default_mask_len = self._action_mask_len if hasattr(self, '_action_mask_len') and self._action_mask_len > 0 else (g_total_flat_actions if g_total_flat_actions > 0 else 1)
        default_mask_val = np.zeros(default_mask_len, dtype=bool)

        for i, remote in enumerate(self.remotes):
            if remote in target_remotes:
                if self.error_flags[i] or not self.processes[i].is_alive():
                    results.append(default_mask_val if method_name == "action_masks" else RuntimeError(f"W{i} dead/errored"))
                else:
                    try: remote.send(("env_method", (method_name, method_args, method_kwargs))); results.append(self._get_result_from_remote(i, timeout=30.0))
                    except Exception as e: print(f"[ERROR SYNC_VEC_ENV W{i} env_method '{method_name}'] {e}", file=sys.__stderr__); results.append(default_mask_val if method_name == "action_masks" else e) # Keep ERROR
        if indices is not None:
            full_results = [default_mask_val if method_name == "action_masks" else None] * self.num_envs
            target_indices_list = self._get_indices(indices); res_idx = 0
            for i_env in range(self.num_envs):
                if i_env in target_indices_list:
                    if res_idx < len(results): full_results[i_env] = results[res_idx]; res_idx +=1
            return full_results
        return results

    def get_attr(self, attr_name: str, indices: VecEnvIndices = None) -> List[Any]:
        target_remotes = self._get_target_remotes(indices); results = []
        for i, remote in enumerate(self.remotes):
            if remote in target_remotes:
                if self.error_flags[i] or not self.processes[i].is_alive(): results.append(RuntimeError(f"W{i} dead/errored"))
                else:
                    try: remote.send(("get_attr", attr_name)); results.append(self._get_result_from_remote(i, timeout=30.0))
                    except Exception as e: results.append(e)
        if indices is not None:
            full_results = [None] * self.num_envs; target_indices_list = self._get_indices(indices); res_idx = 0
            for i_env in range(self.num_envs):
                if i_env in target_indices_list:
                    if res_idx < len(results): full_results[i_env] = results[res_idx]; res_idx +=1
            return full_results
        return results

    def set_attr(self, attr_name: str, value: Any, indices: VecEnvIndices = None) -> None:
        target_remotes = self._get_target_remotes(indices)
        for i, remote in enumerate(self.remotes):
            if remote in target_remotes and self.processes[i].is_alive() and not self.error_flags[i]:
                try:
                    remote.send(("set_attr", (attr_name, value)))
                    confirmation = self._get_result_from_remote(i, timeout=30.0)
                    if confirmation is not True: 
                        if g_training_step_debug: print(f"[WARN SYNC_VEC_ENV W{i} set_attr] Unexpected confirm: {confirmation}", file=sys.__stderr__) # <<< Conditional Print >>>
                except Exception as e: print(f"[ERROR SYNC_VEC_ENV W{i} set_attr '{attr_name}'] {e}", file=sys.__stderr__) # Keep ERROR

    def close(self) -> None:
        if self.closed: return
        if g_training_step_debug: print(f"[INFO SyncVecEnv Close] Closing VecEnv with {self.num_envs} workers...", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>
        if self.waiting:
            for i, remote in enumerate(self.remotes):
                if not remote.closed and remote.poll(0.01):
                    try: remote.recv()
                    except Exception: pass
        for i, remote in enumerate(self.remotes):
            if not remote.closed:
                try:
                    if self.processes[i].is_alive() and not self.error_flags[i]: remote.send(("close", None))
                except (BrokenPipeError, EOFError): pass 
                except Exception as e_send_close: print(f"[WARN SyncVecEnv Close W{i}] Error sending 'close': {e_send_close}", file=sys.__stderr__) # Keep WARN
                finally:
                    try: remote.close()
                    except Exception: pass 
        for i, process in enumerate(self.processes):
            try:
                if process.is_alive(): process.join(timeout=10)
                if process.is_alive(): process.terminate(); process.join(timeout=2)
            except Exception as e_join: print(f"[WARN SyncVecEnv Close W{i}] Error joining/term (PID: {process.pid if hasattr(process,'pid') else 'N/A'}): {e_join}", file=sys.__stderr__) # Keep WARN
        self.closed = True
        if g_training_step_debug: print("[INFO SyncVecEnv Close] VecEnv Closed.", file=sys.__stderr__, flush=True) # <<< Conditional Print >>>

# --- END OF FILE sync_vec_env.py ---