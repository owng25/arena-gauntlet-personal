# --- START OF FILE train_env.py ---
import os
import json
import random
import re
import time
import shutil
from typing import Any, Dict, List, Tuple, Optional, Sequence, Union
# <<< MODIFIED: Import Counter for synergy calc >>>
from collections import defaultdict, deque, Counter
import gymnasium as gym # Use Gymnasium
from gymnasium.spaces import Box, Discrete, MultiDiscrete, Dict as GymDict
import numpy as np
_global_np = np
# <<< MODIFIED: Import BasePolicy for type hinting opponent policy >>>
from stable_baselines3.common.policies import BasePolicy
import torch
# Use PPO for loading non-maskable opponents if needed, but MaskablePPO for the agent itself
from stable_baselines3 import PPO
from sb3_contrib import MaskablePPO # Import MaskablePPO here
import traceback # Import traceback for exception printing
import sys # Needed for checking modules
from pathlib import Path # Import Path
import warnings # Import warnings
import uuid # <<< ADDED: Import uuid for better unique IDs >>>
import logging

class OpponentBrainManagerInterface:
    def get_brain_policy(self, brain_id: str) -> Optional[BasePolicy]:
        """Retrieves a policy object by its brain_id. Assumed to be on CPU."""
        # In a real implementation, this would look up brain_id in its internal store.
        # Example:
        # if brain_id in self.brains: # self.brains: Dict[str, BasePolicy]
        #     return self.brains[brain_id]
        # return None
        raise NotImplementedError("This is an interface. A concrete implementation is needed.")

    def get_available_brain_ids(self) -> List[str]:
        """Returns a list of available brain_ids managed by this instance."""
        # Example:
        # return list(self.brains.keys())
        raise NotImplementedError("This is an interface.")

    # The training loop would be responsible for adding policies to the manager:
    # def add_brain(self, policy: BasePolicy, brain_id: Optional[str] = None) -> str:
    #     """Adds a policy (expected to be on CPU) to the manager.
    #        Manages storage limits (e.g., deque of fixed size).
    #        Returns the assigned brain_id.
    #     """
    #     raise NotImplementedError("This is an interface.")
# --- END: OpponentBrainManager Placeholder/Interface ---


DETAILED_ACTION_DEBUG = True
# <<< ADDED: Merge Debug Flag >>>
DETAILED_MERGE_DEBUG = True # Set to True for detailed merge logs
DETAILED_SELFPLAY_DEBUG = True
# <<< ADDED: Synergy Debug Flag >>>
DETAILED_SYNERGY_DEBUG = True

TRAINING_STEP_DEBUG = True

SAVE_FIRST_PLACE_TEAMS = False # This flag is now used by the callback, not the env
MAX_FIRST_PLACE_SAVES = 20 # Limit set to 20
FIRST_PLACE_TEAMS_DIR = "./first_place_teams"

# --- Simulation Utils Import ---
try:
    _base_dir = os.path.dirname(os.path.abspath(__file__)) if '__file__' in locals() else os.path.abspath('.')
    _utils_path = os.path.join(_base_dir, "simulation_utils.py")
    if not os.path.exists(_utils_path): _utils_path = os.path.join(os.path.dirname(_base_dir), "simulation_utils.py") # Try parent dir
    if os.path.exists(_utils_path):
        from simulation_utils import ( BATTLE_DIR, DATA_DIR, OPPONENT_MODELS_DIR, convert_np_types as sim_convert_np_types_base, force_reset_simulator )
        # print("[DEBUG train_env] Successfully imported from simulation_utils.") # Less verbose
        def sim_convert_np_types(o: Any, context: str = "unknown") -> Any:
            """Fallback converter with context for safer fallbacks, uses _global_np."""
            if isinstance(o, _global_np.integer): return int(o)
            elif isinstance(o, _global_np.floating): return float(o) if _global_np.isfinite(o) else 0.0 # Return 0.0 for non-finite
            elif isinstance(o, _global_np.ndarray): return o.tolist() # Needs recursive conversion via deep_convert
            elif isinstance(o, bytes): return o.decode('utf-8', errors='ignore')
            elif isinstance(o, _global_np.bool_): return bool(o)
            elif isinstance(o, Path): return str(o)
            elif isinstance(o, (int, float, bool, str, list, dict, type(None))): return o
            # <<< FIX TypeError: Return 0.0 if in observation context, else string >>>
            fallback_value = 0.0 if context == "observation" else str(o)
            if DETAILED_SELFPLAY_DEBUG: print(f"[WARN sim_convert_np_types fallback ({context})] Unhandled type {type(o)}, falling back to {type(fallback_value).__name__}.")
            return fallback_value
    else: raise ImportError("simulation_utils.py not found in expected locations.")
except ImportError:
    print("[WARN train_env] Could not import from simulation_utils.py. Using fallback values and functions.")
    PROJECT_ROOT_FALLBACK = os.path.dirname(os.path.abspath(__file__)) if '__file__' in locals() else os.path.abspath('.')
    BATTLE_DIR = os.path.join(PROJECT_ROOT_FALLBACK, "battle")
    OPPONENT_MODELS_DIR = os.path.join(PROJECT_ROOT_FALLBACK, "opponent_models") # Still used for "path" type opponents
    _default_data_dir = os.path.abspath(os.path.join(PROJECT_ROOT_FALLBACK, "..", "json_data_copy", "data"))
    DATA_DIR = os.environ.get("ILLUVIUM_DATA_DIR", _default_data_dir)
    print(f"[WARN train_env] Fallback DATA_DIR: {DATA_DIR}")
    def sim_convert_np_types(o: Any, context: str = "unknown") -> Any: # Added context
        """Fallback converter with context for safer fallbacks."""
        if isinstance(o, _global_np.integer): return int(o)
        elif isinstance(o, _global_np.floating): return float(o) if _global_np.isfinite(o) else 0.0 # Return 0.0 for non-finite
        elif isinstance(o, _global_np.ndarray): return o.tolist() # Needs recursive conversion via deep_convert
        elif isinstance(o, bytes): return o.decode('utf-8', errors='ignore')
        elif isinstance(o, _global_np.bool_): return bool(o)
        elif isinstance(o, Path): return str(o)
        elif isinstance(o, (int, float, bool, str, list, dict, type(None))): return o
        # <<< FIX TypeError: Return 0.0 if in observation context, else string >>>
        fallback_value = 0.0 if context == "observation" else str(o)
        if DETAILED_SELFPLAY_DEBUG: print(f"[WARN sim_convert_np_types fallback ({context})] Unhandled type {type(o)}, falling back to {type(fallback_value).__name__}.")
        return fallback_value
    def force_reset_simulator(): print("[WARN train_env] force_reset_simulator function is not available (ImportError).")

# OPPONENT_MODELS_DIR is still relevant for loading initial/static opponents from disk using "path" source type.
# The in-memory system with OpponentBrainManager is for the dynamic pool, avoiding disk writes for those updates.
for _dir in [BATTLE_DIR, OPPONENT_MODELS_DIR, DATA_DIR]:
     try: os.makedirs(_dir, exist_ok=True)
     except OSError as e: print(f"[WARN train_env] Could not create directory {_dir}: {e}")
# --- End Simulation Utils Import ---

# <<< START: Create directory for saving 1st place teams (kept for consistency) >>>
if SAVE_FIRST_PLACE_TEAMS:
    try:
        os.makedirs(FIRST_PLACE_TEAMS_DIR, exist_ok=True)
        print(f"[INFO train_env] Target directory for first place teams: {os.path.abspath(FIRST_PLACE_TEAMS_DIR)}")
    except OSError as e:
        print(f"[WARN train_env] Could not create directory {FIRST_PLACE_TEAMS_DIR}: {e}. Saving will fail in callback.")
        # We don't disable the flag here, the callback will handle the directory not existing
# <<< END: Create directory >>>

# --- Recursive Conversion Helper ---
def deep_convert_np_types(obj: Any, context: str = "unknown") -> Any:
    """Recursively converts NumPy types in nested structures for JSON compatibility."""
    if isinstance(obj, dict): return {k: deep_convert_np_types(v, context) for k, v in obj.items()}
    elif isinstance(obj, (list, tuple)): return [deep_convert_np_types(elem, context) for elem in obj]
    else: return sim_convert_np_types(obj, context)
# --- End Recursive Conversion Helper ---

# --- Configuration ---
# <<< MODIFIED CONSTANTS >>>
NUM_PLAYERS = 8;
BENCH_MAX_SIZE = 15;        # Increased from 6
SHOP_SIZE = 7;              # Increased from 3
MAX_STAGE = 3; MAX_COST = 5.0;
MAX_LEVEL = 10;             # Increased from 6
MAX_AUGMENTS_PER_UNIT = 2
INITIAL_HEALTH = 30;        # Increased from 10
FORCED_COINS = 10;
# <<< END MODIFIED CONSTANTS >>>

# --- Reward Shaping Constants (MODIFIED) ---
ROUND_WIN_REWARD = 0.1
ROUND_LOSS_REWARD = -0.1
BUY_REWARD = 0.0              # Set to 0
SELL_PENALTY = 0.0           # Set to 0 (was negative penalty)
REROLL_PENALTY = 0.0         # Set to 0
BUY_XP_REWARD = 0.0           # Set to 0
MODIFY_UNIT_REWARD = 0.0     # Kept the same
APPLY_AUGMENT_REWARD = 0.01    # Kept the same
SKIP_AUGMENT_REWARD = 0.0     # Kept the same
SELL_LAST_UNIT_PENALTY = -0.7 # Kept the same
MUTUAL_EMPTY_BOARD_PENALTY = -0.25 # Kept the same
INVALID_ACTION_PENALTY = -0.5 # Kept the same
MERGE_STAGE_REWARD = 0.02     # Reward for successful merge/evolution
PLACE_EVOLVED_REWARD = 0.01   # Reward for placing Stage > 1 unit
BENCH_TO_BOARD_REWARD = 0.0    # Base reward for bench->board move (PLACE_EVOLVED is added on top)
BOND_REWARD = 0.01 # New reward for bonding
# --- End Reward Shaping Constants ---

PLACEMENT_REWARDS = { 1: 1.0, 2: 0.5, 3: 0.2, 4: 0.0, 5: -0.2, 6: -0.5, 7: -0.8, 8: -1.0 }
# <<< MODIFIED: Updated XP Thresholds >>>
LEVEL_XP_THRESHOLDS = { 1: 2, 2: 2, 3: 6, 4: 10, 5: 20, 6: 30, 7: 40, 8: 60, 9: 80 } # Up to level 9 -> 10
# <<< END MODIFIED >>>
PASSIVE_XP_PER_ROUND = 2; BUY_XP_COST = 2; BUY_XP_AMOUNT = 4; REROLL_COST = 1
INCOME_ROUND_BREAKPOINTS = [5, 10]; INCOME_AMOUNTS = [3, 5, 7]; AUGMENT_OFFER_FREQUENCY = 5; NUM_AUGMENT_CHOICES = 3

# <<< MODIFIED: Shop Odds - Updated for Levels 1-10, Tier 2 removed, Renormalized >>>
_raw_shop_odds = {
    # Tiers:      0      1      3      4      5   (T2 excluded)
    1: {0: 1.00, 1: 0.00, 3: 0.00, 4: 0.00, 5: 0.00}, # Level 1: 100% T0
    2: {0: 0.70, 1: 0.25, 3: 0.00, 4: 0.00, 5: 0.00}, # Level 2: 70% T0, 25% T1 (5% T2 removed)
    3: {0: 0.60, 1: 0.30, 3: 0.04, 4: 0.00, 5: 0.00}, # Level 3: 60% T0, 30% T1, 4% T3 (6% T2 removed)
    4: {0: 0.45, 1: 0.40, 3: 0.08, 4: 0.05, 5: 0.02}, # Level 4: 45% T0, 40% T1, 8% T3, 5% T4, 2% T5 (8% T2 removed) -> Note: original L4 sum was 100% including T2
    5: {0: 0.25, 1: 0.50, 3: 0.10, 4: 0.10, 5: 0.04}, # Level 5: 25% T0, 50% T1, 10% T3, 10% T4, 4% T5 (10% T2 removed) -> Corrected T2 removal
    6: {0: 0.10, 1: 0.40, 3: 0.08, 4: 0.25, 5: 0.12}, # Level 6: 10% T0, 40% T1, 8% T3, 25% T4, 12% T5 (5% T2 removed) -> Corrected T2 removal
    7: {0: 0.00, 1: 0.25, 3: 0.05, 4: 0.34, 5: 0.24}, # Level 7: 0% T0, 25% T1, 5% T3, 34% T4, 24% T5 (12% T2 removed) -> Corrected T2 removal
    8: {0: 0.00, 1: 0.10, 3: 0.03, 4: 0.34, 5: 0.35}, # Level 8: 0% T0, 10% T1, 3% T3, 34% T4, 35% T5 (18% T2 removed) -> Corrected T2 removal
    9: {0: 0.00, 1: 0.10, 3: 0.01, 4: 0.24, 5: 0.35}, # Level 9: 0% T0, 10% T1, 1% T3, 24% T4, 35% T5 (30% T2 removed) -> Corrected T2 removal
   10: {0: 0.00, 1: 0.10, 3: 0.01, 4: 0.15, 5: 0.34}  # Level 10: 0% T0, 10% T1, 1% T3, 15% T4, 34% T5 (40% T2 removed) -> Corrected T2 removal
}
# Renormalize probabilities for each level
SHOP_ODDS_ADJUSTED = {}
for level, odds in _raw_shop_odds.items():
    # Filter out tiers with 0 probability *before* summing
    filtered_odds = {tier: prob for tier, prob in odds.items() if prob > 0}
    if not filtered_odds: # Handle case where all original odds might be zero (shouldn't happen with current data)
        print(f"[WARN Config] No valid shop odds defined for L{level}. Defaulting to T0=100%.")
        SHOP_ODDS_ADJUSTED[level] = {0: 1.0}
        continue
    current_sum = sum(filtered_odds.values())
    if _global_np.isclose(current_sum, 1.0):
        SHOP_ODDS_ADJUSTED[level] = filtered_odds
    elif current_sum > 0:
        if DETAILED_SELFPLAY_DEBUG: print(f"[INFO Config] Renormalizing shop odds for L{level}. Original sum (excluding T2): {current_sum:.4f}")
        SHOP_ODDS_ADJUSTED[level] = {tier: prob / current_sum for tier, prob in filtered_odds.items()}
        # Verify after normalization
        new_sum = sum(SHOP_ODDS_ADJUSTED[level].values())
        if not _global_np.isclose(new_sum, 1.0):
             print(f"[ERROR Config] Failed to renormalize odds for L{level}! Sum after norm: {new_sum}. Using unnormalized.")
             SHOP_ODDS_ADJUSTED[level] = filtered_odds # Fallback to original if renormalization fails badly
        else:
             # print(f"[DEBUG Config] L{level} odds normalized successfully. New sum: {new_sum:.4f}") # Optional success log
             pass
    else: # Sum is zero or negative - fallback
        print(f"[WARN Config] Invalid probability sum ({current_sum}) for L{level} before normalization. Defaulting to T0=100%.")
        SHOP_ODDS_ADJUSTED[level] = {0: 1.0}
del _raw_shop_odds # Remove temporary variable
# <<< END MODIFIED SHOP ODDS >>>

# Verify final sums
for level, odds_dict in SHOP_ODDS_ADJUSTED.items():
    if not _global_np.isclose(sum(odds_dict.values()), 1.0): print(f"[WARN Config FINAL CHECK] Shop odds L{level} sum: {sum(odds_dict.values())}")

# --- New 4D Action Space Definitions ---
NUM_ACTION_TYPES = 6
DIM_ENTITY_SIZE = 60  # For player units or catalogue items
DIM_POSITION_SIZE = 12 # For board positions (0 for bench, 1-11 for specific board slots)
DIM_ITEM_SIZE = 18    # For shop slots or item sub-selection (e.g. system actions)

NEW_TOTAL_FLAT_ACTIONS = NUM_ACTION_TYPES * DIM_ENTITY_SIZE * DIM_POSITION_SIZE * DIM_ITEM_SIZE

# Action Type Indices
AT_SYSTEM_ACTIONS = 0
AT_SELL_UNIT = 1
AT_BUY_ILLUVIAL = 2
AT_PLACE_UNIT = 3
AT_RANGER_EQUIP = 4
AT_RANGER_BOND = 5

# Specific item_idx for AT_SYSTEM_ACTIONS (within the DIM_ITEM_SIZE range)
# These names are now consistent with _apply_action
SYSTEM_ACTION_PASS = 0
SYSTEM_ACTION_REROLL = 1 # Standardized from SYSTEM_ACTION_REROLL_ILLUVIAL_SHOP
SYSTEM_ACTION_BUY_XP = 2
SYSTEM_ACTION_EXIT_PLACEMENT = 3 # Standardized from SYSTEM_ACTION_EXIT_PLACEMENT_PHASE
SYSTEM_ACTION_END_PLANNING = 4   # Standardized from SYSTEM_ACTION_END_PLANNING_PHASE
# --- End New 4D Action Space Definitions ---

POSITIONS_BLUE_LIST: List[Tuple[int, int]] = [(-67,46),(-40,46),(-22,45),(-6,47),(21,47),(-49,37),(12,38),(-58,28),(-31,28),(-13,27),(3,29),(30,29),(-40,19),(21,20),(-49,10),(-22,10),(-4,9),(12,11),(39,11)]
POSITIONS_RED_LIST: List[Tuple[int, int]] = [(68,-47),(41,-47),(23,-45),(6,-46),(-21,-46),(50,-38),(-12,-37),(59,-29),(32,-29),(14,-27),(-3,-28),(-30,-28),(41,-20),(-21,-19),(50,-11),(23,-11),(5,-9),(-12,-10),(-39,-10)]
if len(POSITIONS_BLUE_LIST) != len(POSITIONS_RED_LIST): raise ValueError("Position lists must have same length!")
MAX_BOARD_SLOTS = len(POSITIONS_BLUE_LIST); NUM_UNIQUE_POSITIONS = MAX_BOARD_SLOTS
UNIFIED_ID_TO_BLUE_POS_MAP: Dict[int, Tuple[int, int]] = {i + 1: pos for i, pos in enumerate(POSITIONS_BLUE_LIST)}
BLUE_POS_TO_UNIFIED_ID_MAP: Dict[Tuple[int, int], int] = {pos: i + 1 for i, pos in enumerate(POSITIONS_BLUE_LIST)}
UNIFIED_ID_TO_RED_POS_MAP: Dict[int, Tuple[int, int]] = {i + 1: POSITIONS_RED_LIST[i] for i in range(len(POSITIONS_RED_LIST))}
MAX_ILLUVIAL_ID = 100; MAX_AUGMENT_ID = 50 # Set rough max values (update if needed)
MAX_WEAPON_ID = 50 # Placeholder, will be updated in _populate_id_maps
MAX_WEAPON_AMPLIFIER_ID = 50 # Placeholder, will be updated in _populate_id_maps


# --- MODIFIED Observation Space Size Calculation ---
FEATURES_PER_UNIT = 5 # <<< CHANGED from 4 to 5 >>>
if DETAILED_SELFPLAY_DEBUG: print(f"[INFO Config] FEATURES_PER_UNIT set to: {FEATURES_PER_UNIT}")

BOARD_FEATURES_PER_PLAYER = MAX_BOARD_SLOTS * FEATURES_PER_UNIT         # <<< MODIFIED: 19 * 5 = 95 >>>
TOTAL_BOARD_FEATURES = NUM_PLAYERS * BOARD_FEATURES_PER_PLAYER        # <<< MODIFIED: 8 * 95 = 760 >>>

# Features specific to the agent (POV player)
AGENT_SCALAR_FEATURES = 5 # Health, Gold, Level, XP, Round
AGENT_BENCH_FEATURES = BENCH_MAX_SIZE * FEATURES_PER_UNIT               # 15 * 5 = 75
AGENT_SHOP_FEATURES = SHOP_SIZE * 2 # ID + Cost (Main Illuvial Shop)    # 7 * 2 = 14
AGENT_AUGMENT_FEATURES = 1 + NUM_AUGMENT_CHOICES # is_pending + IDs (Illuvial Augments) # 1 + 3 = 4
RANGER_EQUIPMENT_FEATURES = 3 # Weapon ID, Amp1 ID, Amp2 ID
SPECIAL_SHOP_ITEM_FEATURES = 5 # 5 item IDs in the special shop

AGENT_SPECIFIC_FEATURES = ( AGENT_SCALAR_FEATURES + AGENT_BENCH_FEATURES +
                            AGENT_SHOP_FEATURES + AGENT_AUGMENT_FEATURES +
                            RANGER_EQUIPMENT_FEATURES + SPECIAL_SHOP_ITEM_FEATURES) # 5 + 75 + 14 + 4 + 3 + 5 = 106

# Features specific to EACH opponent (excluding POV player)
# Opponents do not have Ranger equipment or special shop directly observed in this manner.
# Their state includes scalar values and bench. Board is globally observed.
OPPONENT_SCALAR_FEATURES = 4 # Health, Gold, Level, XP
OPPONENT_BENCH_FEATURES = BENCH_MAX_SIZE * FEATURES_PER_UNIT            # <<< MODIFIED: 15 * 5 = 75 >>>
FEATURES_PER_OPPONENT = OPPONENT_SCALAR_FEATURES + OPPONENT_BENCH_FEATURES # <<< MODIFIED: 4 + 75 = 79 >>>
TOTAL_OPPONENT_EXTRA_FEATURES = (NUM_PLAYERS - 1) * FEATURES_PER_OPPONENT # <<< MODIFIED: 7 * 79 = 553 >>>

# Grand Total
FLAT_OBSERVATION_SIZE = (
    TOTAL_BOARD_FEATURES +          # 760 (All boards)
    AGENT_SPECIFIC_FEATURES +       # 106 (Agent specifics)
    TOTAL_OPPONENT_EXTRA_FEATURES   # 553 (Opponent specifics)
) # MODIFIED: 760 + 106 + 553 = 1419
if DETAILED_SELFPLAY_DEBUG: print(f"[INFO Config] Recalculated FLAT_OBSERVATION_SIZE = {FLAT_OBSERVATION_SIZE} (Agent Specifics: {AGENT_SPECIFIC_FEATURES})")
# --- END MODIFIED Observation Space Size Calculation ---


# --- Old Flattened Action Space Definitions (REMOVED) ---
# MAX_COMBINED_TARGET_IDX = MAX_BOARD_SLOTS + BENCH_MAX_SIZE # 19 + 15 = 34
# NUM_POSITIONS_INCLUDING_BENCH = NUM_UNIQUE_POSITIONS + 1 # 19 + 1 = 20
#
# FLAT_IDX_SELL_START = 0
# FLAT_SELL_ACTIONS = MAX_COMBINED_TARGET_IDX # 34 actions
# FLAT_IDX_BUY_START = FLAT_IDX_SELL_START + FLAT_SELL_ACTIONS # Index 34
#
# FLAT_BUY_ACTIONS = SHOP_SIZE # 7 actions
# FLAT_IDX_REROLL = FLAT_IDX_BUY_START + FLAT_BUY_ACTIONS # Index 34 + 7 = 41
# FLAT_IDX_BUY_XP = FLAT_IDX_REROLL + 1 # Index 42
# FLAT_IDX_ENTER_PLACEMENT = FLAT_IDX_BUY_XP + 1 # Index 43
#
# FLAT_IDX_MODIFY_UNIT_START = FLAT_IDX_ENTER_PLACEMENT + 1 # Index 44
# FLAT_MODIFY_UNIT_ACTIONS = MAX_COMBINED_TARGET_IDX * NUM_POSITIONS_INCLUDING_BENCH # 34 * 20 = 680 actions
# FLAT_IDX_EXIT_PLACEMENT = FLAT_IDX_MODIFY_UNIT_START + FLAT_MODIFY_UNIT_ACTIONS # Index 44 + 680 = 724
#
# FLAT_IDX_AUGMENT_SKIP = FLAT_IDX_EXIT_PLACEMENT + 1 # Index 725
# FLAT_IDX_AUGMENT_APPLY_START = FLAT_IDX_AUGMENT_SKIP + 1 # Index 726
# FLAT_AUGMENT_APPLY_ACTIONS = NUM_AUGMENT_CHOICES * MAX_COMBINED_TARGET_IDX # 3 * 34 = 102 actions
# TOTAL_FLAT_ACTIONS = FLAT_IDX_AUGMENT_APPLY_START + FLAT_AUGMENT_APPLY_ACTIONS # 726 + 102 = 828
# --- End OLD Flattened Action Space Definitions ---


# --- Data Loading & Basic Helpers ---
try: PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__)) if '__file__' in locals() else os.path.abspath('.')
except NameError: PROJECT_ROOT = os.getcwd()
ILLUVIAL_TEMPLATES_FILE = os.path.join(PROJECT_ROOT, "illuvial_templates.json")
MERGED_FILE = os.path.join(DATA_DIR, "merged_data.json")
COMPLETE_EQUIPMENT_DATABASE_FILE = os.path.join(PROJECT_ROOT, "complete_equipment_database.json")

# --- Old Action Helper Functions (REMOVED) ---
# def get_flat_sell_index(unit_combined_idx_0based: int) -> int:
#     return FLAT_IDX_SELL_START + unit_combined_idx_0based
#
# def get_flat_buy_index(shop_slot_idx_0based: int) -> int:
#     return FLAT_IDX_BUY_START + shop_slot_idx_0based
#
# def get_flat_modify_index(unit_combined_idx_0based: int, target_pos_id_0based_bench_is_0: int) -> int:
#     return FLAT_IDX_MODIFY_UNIT_START + (unit_combined_idx_0based * NUM_POSITIONS_INCLUDING_BENCH) + target_pos_id_0based_bench_is_0
#
# def get_flat_augment_apply_index(augment_choice_idx_1based: int, target_unit_combined_idx_0based: int) -> int:
#     if not (1 <= augment_choice_idx_1based <= NUM_AUGMENT_CHOICES):
#         # print(f"[WARN get_flat_augment_apply_index] Invalid augment_choice_idx_1based: {augment_choice_idx_1based}")
#         return -1 # Invalid index
#     augment_choice_idx_0based = augment_choice_idx_1based - 1
#     return FLAT_IDX_AUGMENT_APPLY_START + (augment_choice_idx_0based * MAX_COMBINED_TARGET_IDX) + target_unit_combined_idx_0based
#
# def decode_flat_action(flat_action_index: int) -> Tuple[int, int, int, str]:
#     """Decodes a flat action index into its components: (unit_idx, target_idx, choice_idx, action_type_str). unit_idx is 0-based combined. target_idx is 0-based (position or shop slot). choice_idx is 1-based for augments."""
#     # This function needs to be updated if action space changes.
#     if FLAT_IDX_SELL_START <= flat_action_index < FLAT_IDX_BUY_START:
#         unit_idx = flat_action_index - FLAT_IDX_SELL_START
#         return unit_idx, -1, -1, "sell"
#     elif FLAT_IDX_BUY_START <= flat_action_index < FLAT_IDX_REROLL:
#         target_idx = flat_action_index - FLAT_IDX_BUY_START
#         return -1, target_idx, -1, "buy"
#     elif flat_action_index == FLAT_IDX_REROLL:
#         return -1, -1, -1, "reroll"
#     elif flat_action_index == FLAT_IDX_BUY_XP:
#         return -1, -1, -1, "buy_xp"
#     elif flat_action_index == FLAT_IDX_ENTER_PLACEMENT:
#         return -1, -1, -1, "enter_placement"
#     elif FLAT_IDX_MODIFY_UNIT_START <= flat_action_index < FLAT_IDX_EXIT_PLACEMENT:
#         relative_idx = flat_action_index - FLAT_IDX_MODIFY_UNIT_START
#         unit_idx = relative_idx // NUM_POSITIONS_INCLUDING_BENCH
#         target_idx = relative_idx % NUM_POSITIONS_INCLUDING_BENCH
#         return unit_idx, target_idx, -1, "modify_unit"
#     elif flat_action_index == FLAT_IDX_EXIT_PLACEMENT:
#         return -1, -1, -1, "exit_placement"
#     elif flat_action_index == FLAT_IDX_AUGMENT_SKIP:
#         return -1, -1, -1, "augment_skip"
#     elif FLAT_IDX_AUGMENT_APPLY_START <= flat_action_index < TOTAL_FLAT_ACTIONS:
#         relative_idx = flat_action_index - FLAT_IDX_AUGMENT_APPLY_START
#         choice_idx_0based = relative_idx // MAX_COMBINED_TARGET_IDX
#         unit_idx = relative_idx % MAX_COMBINED_TARGET_IDX
#         return unit_idx, -1, choice_idx_0based + 1, "augment_apply"
#     else:
#         # print(f"[WARN decode_flat_action] Index {flat_action_index} out of bounds for TOTAL_FLAT_ACTIONS {TOTAL_FLAT_ACTIONS}")
#         return -1, -1, -1, "unknown_invalid" # Should not happen with valid mask
# --- End Old Action Helper Functions ---

def load_json_safely(path: str) -> Any:
    """Loads JSON data, handling file not found and decode errors."""
    if not os.path.exists(path):
        print(f"[ERROR] JSON file not found: {path}")
        return None
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        print(f"[ERROR] Failed to decode JSON {path}: {e}")
        return None
    except Exception as e:
        print(f"[ERROR] Failed to load JSON {path}: {e}")
        return None
def load_illuvial_templates() -> List[Dict[str, Any]]:
    """Loads and validates illuvial template data."""
    data = load_json_safely(ILLUVIAL_TEMPLATES_FILE);
    if data is None: return []
    if not isinstance(data, list): print(f"[WARN] Expected list in {ILLUVIAL_TEMPLATES_FILE}, got {type(data)}. Returning empty list."); return []
    valid_templates = [item for item in data if isinstance(item, dict) and item.get("LineType") and item.get("Tier") is not None]
    if len(valid_templates) != len(data): print(f"[WARN] {len(data) - len(valid_templates)} invalid entries filtered from illuvial templates.")
    if not valid_templates: print("[WARN] ILLUVIAL_TEMPLATES list empty/invalid after filtering.")
    return valid_templates
ILLUVIAL_TEMPLATES = load_illuvial_templates();
def load_merged_data() -> Dict[str, Any]:
    data = load_json_safely(MERGED_FILE); default_data = {"Augments": {}}
    if data is None or not isinstance(data, dict): print(f"[WARN] Failed load/validate {MERGED_FILE}. Using default structure."); return default_data
    if "Augments" not in data:
        print(f"[WARN] Key 'Augments' missing in {MERGED_FILE}, adding empty default."); data["Augments"] = {}
    elif not isinstance(data["Augments"], dict):
        print(f"[WARN] Key 'Augments' in {MERGED_FILE} not dict ({type(data['Augments'])}). Replacing with empty dict."); data["Augments"] = {}
    return data
def clean_line_type(line: str) -> str: return re.sub(r'\d+$', '', line) if isinstance(line, str) else ""

def generate_random_unit_id() -> str:
    """Generates a highly unique ID using UUID4."""
    return str(uuid.uuid4())

def get_illuvial_cost(ill: Optional[Dict[str, Any]]) -> int:
    """Returns the cost of an illuvial template, defaulting to 1."""
    if ill is None: return 1
    tier = ill.get("Tier")
    if tier == 0: return 1
    elif isinstance(tier, int) and 1 <= tier <= 5: return tier
    else:
        print(f"[WARN get_illuvial_cost] Unexpected tier: {tier} for {ill.get('LineType')}. Default cost 1.")
        return 1
# --- End Data Loading & Basic Helpers ---

# --- Player Class (MODIFIED for new opponent brain management) ---
class Player:
    def __init__(self, player_id: int, health: int = INITIAL_HEALTH):
        self.player_id: int = player_id; self.health: int = health; self.coins: int = 0; self.board: List[Dict] = []; self.bench: List[Dict] = []
        self.xp: int = 0; self.level: int = 1;
        self.illuvial_shop: List[Optional[Dict]] = [None] * SHOP_SIZE
        
        self.opponent_source_type: str = "random"
        self.opponent_identifier: Optional[str] = None
        
        self.last_opponent_id: Optional[int] = None
        self.won_last_round: Optional[bool] = None; self.current_phase: str = "planning"
        self.available_augment_choices: List[Optional[str]] = [None] * NUM_AUGMENT_CHOICES; self.is_augment_choice_pending: bool = False; self.augment_offer_round: int = -1
        self._pairs_presimulated_this_step: set[tuple[int, int]] = set()

        # Ranger specific attributes
        self.ranger_unit_id: Optional[str] = None
        self.ranger_weapon: Optional[Dict] = None # Stores the INSTANCE of the equipped weapon
        self.ranger_weapon_amplifiers: List[Dict] = [] # Stores a list of INSTANCES of equipped amplifiers
        self.ranger_suit: Optional[Dict] = None # Stores the INSTANCE of the equipped suit (though initially might just be definition)

        # Special Ranger Shop attributes
        self.special_shop_active: bool = False
        self.special_shop_type: Optional[str] = None # e.g., "weapon", "weapon_amplifier"
        self.special_shop_items: List[Optional[Dict]] = [None] * 5 # Stores DEFINITIONS of items in special shop
        self.special_shop_round_triggered: int = -1

    def is_alive(self)->bool: return self.health>0
    def units_on_board_count(self)->int: return len(self.board)
    def get_combined_units(self) -> List[Dict]: return list(self.board) + list(self.bench)
    def get_unit_at_pos(self, q: int, r: int) -> Optional[Dict]: return next((unit for unit in self.board if isinstance(pos := unit.get("Position"), dict) and pos.get("Q") == q and pos.get("R") == r), None)
    
    # NEW METHOD: set_opponent_source
    def set_opponent_source(self, source_type: str, identifier: Optional[str]):
        """Helper to set opponent source and identifier with logging."""
        old_source = self.opponent_source_type
        old_id = self.opponent_identifier
        self.opponent_source_type = source_type
        self.opponent_identifier = identifier
        if DETAILED_SELFPLAY_DEBUG and hasattr(self, 'logger') and self.logger is not None:
            source_log_new = f"'{source_type}'"
            id_log_new = f"'{identifier}'" if identifier else 'None'
            source_log_old = f"'{old_source}'"
            id_log_old = f"'{old_id}'" if old_id else 'None'
            self.logger.debug(f"[Opponent P{self.player_id}] Source updated: {source_log_old} -> {source_log_new}, ID: {id_log_old} -> {id_log_new}")

    def clear_state_for_reset(self):
        self.health=INITIAL_HEALTH; self.coins=0; self.board=[]; self.bench=[]; self.xp=0; self.level=1;
        self.illuvial_shop=[None]*SHOP_SIZE;
        
        self.opponent_source_type="random"; self.opponent_identifier=None;
        
        self.last_opponent_id=None;
        self.won_last_round = None; self.current_phase = "planning"; self.available_augment_choices = [None] * NUM_AUGMENT_CHOICES; self.is_augment_choice_pending = False; self.augment_offer_round = -1

        # Reset Ranger attributes
        self.ranger_unit_id = None
        self.ranger_weapon = None
        self.ranger_weapon_amplifiers = []
        self.ranger_suit = None

        # Reset Special Ranger Shop attributes
        self.special_shop_active = False
        self.special_shop_type = None
        self.special_shop_items = [None] * 5
        self.special_shop_round_triggered = -1

# --- The Single Environment ---
class IlluviumSingleEnv(gym.Env):
    metadata = {"render_modes": ["human", "ansi", "none"], "render_fps": 4}
        
    # In train_env.py
# Inside IlluviumSingleEnv class
    def __init__(self, env_id: int, render_mode="none",
                 opponent_brain_manager: Optional[OpponentBrainManagerInterface] = None):
        # --- Logger initialization (CORRECT LOCATION) ---
        self.logger = logging.getLogger(f"Env{env_id}.{self.__class__.__name__}")
        self.logger.setLevel(logging.DEBUG) 
        # Configure logger handlers if not already configured by a central setup
        if not self.logger.handlers and not getattr(self.logger.parent, 'handlers', None):
            pass # Keep pass if handlers are not added here
        
        self.logger.info(f"Initializing Env {env_id} on process {os.getpid()}...")
        super().__init__()
        
        self.env_id = env_id
        self.render_mode = render_mode if render_mode in self.metadata["render_modes"] else "none"
        
        # Store the passed brain manager instance
        self.opponent_brain_manager = opponent_brain_manager
        if self.opponent_brain_manager and DETAILED_SELFPLAY_DEBUG:
            self.logger.debug(f"OpponentBrainManager instance received: {type(self.opponent_brain_manager)}")
        elif DETAILED_SELFPLAY_DEBUG:
            self.logger.debug(f"No OpponentBrainManager instance provided to Env {env_id}.")

        try: # --- ADDED TRY BLOCK FOR INIT ERRORS ---
            self.merged_data = load_merged_data()
            self.illuvial_templates = ILLUVIAL_TEMPLATES
            if not isinstance(self.illuvial_templates, list) or not self.illuvial_templates: 
                self.logger.error(f"Illuvial templates invalid/empty for Env {env_id}. Check illuvial_templates.json")

            # Load equipment data
            equipment_data_content = load_json_safely(COMPLETE_EQUIPMENT_DATABASE_FILE)
            if equipment_data_content is None:
                self.logger.error(f"Failed to load equipment data from {COMPLETE_EQUIPMENT_DATABASE_FILE}. Using empty defaults.")
                equipment_data_content = {}
            
            self.weapons_data_from_db: Dict[str, Any] = equipment_data_content.get("weapons", {})
            self.weapon_amplifiers_data_from_db: Dict[str, Any] = equipment_data_content.get("weapon_amplifiers", {})
            self.suits_data_from_db: Dict[str, Any] = equipment_data_content.get("suits", {})

            if not self.weapons_data_from_db: self.logger.warning(f"No weapons data loaded from {COMPLETE_EQUIPMENT_DATABASE_FILE}")
            if not self.weapon_amplifiers_data_from_db: self.logger.warning(f"No weapon_amplifiers data loaded from {COMPLETE_EQUIPMENT_DATABASE_FILE}")
            if not self.suits_data_from_db: self.logger.warning(f"No suits data loaded from {COMPLETE_EQUIPMENT_DATABASE_FILE}")

            self.illuvial_line_to_id: Dict[str, int] = {}
            self.augment_name_to_id: Dict[str, int] = {}
            self.id_to_illuvial_line: Dict[int, str] = {}
            self.id_to_augment_name: Dict[int, str] = {}

            # Equipment definition maps (stores definitions by Name)
            self.weapon_definitions: Dict[str, Dict[str, Any]] = {}
            self.weapon_amplifier_definitions: Dict[str, Dict[str, Any]] = {}
            self.suit_definitions: Dict[str, Dict[str, Any]] = {}

            # Equipment name to ID maps (for observation encoding)
            self.weapon_name_to_id: Dict[str, int] = {}
            self.id_to_weapon_name: Dict[int, str] = {}
            self.weapon_amplifier_name_to_id: Dict[str, int] = {}
            self.id_to_weapon_amplifier_name: Dict[int, str] = {}
            
            self._populate_id_maps() # This will now also populate equipment definitions and their ID maps
            
            global MAX_ILLUVIAL_ID, MAX_AUGMENT_ID, MAX_WEAPON_ID, MAX_WEAPON_AMPLIFIER_ID

            max_obs_component_value = float(max(MAX_ILLUVIAL_ID, MAX_AUGMENT_ID, MAX_WEAPON_ID, MAX_WEAPON_AMPLIFIER_ID, 200)) # Ensure high enough for IDs and other features
            if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"Setting Obs Space Shape: ({FLAT_OBSERVATION_SIZE},) with max_obs_component_value: {max_obs_component_value}")
            self.observation_space = Box(low=0, high=max_obs_component_value, shape=(FLAT_OBSERVATION_SIZE,), dtype=_global_np.float32)

            self.action_space = Discrete(NEW_TOTAL_FLAT_ACTIONS)
            self.logger.info(f"New Action Space Size: {self.action_space.n}, New Observation Space Size: {FLAT_OBSERVATION_SIZE}")


            self.round_idx: int = 0
            self.players: Dict[int, Player] = {}
            self.agent: Optional[Player] = None
            self.episode_finished: bool = False
            self._last_observation: Optional[_global_np.ndarray] = None
            self._last_info: Dict[str, Any] = {}
            self._generated_files_this_step: List[str] = []
            self._last_agent_sim_reward: float = 0.0
            self._cumulative_rewards_this_episode: Dict[str, float] = defaultdict(float)

            if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"Checking for available torch devices...")
            if torch.cuda.is_available(): self.device = torch.device("cuda")
            elif hasattr(torch.backends, "mps") and torch.backends.mps.is_available() and torch.backends.mps.is_built(): self.device = torch.device("mps")
            else: self.device = torch.device("cpu")
            if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"Final device for this env instance: {self.device}")

            self._pairs_presimulated_this_step: set[tuple[int, int]] = set()

            self.policy_cache: Dict[str, BasePolicy] = {} # For "path" type opponents
            self.current_opponent_configs: Dict[int, Tuple[str, Optional[str]]] = {}

            self.logger.info(f"Initialization complete for Env {env_id} on device: {self.device}. Obs Shape: {self.observation_space.shape}, Act Size: {self.action_space.n}")
            if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"Max Illuvial ID: {MAX_ILLUVIAL_ID}, Max Augment ID: {MAX_AUGMENT_ID}")
        except Exception as e_init: # --- CATCH ANY EXCEPTION HERE ---
            self.logger.critical(f"[FATAL Env {env_id} __init__] CRITICAL ERROR during initialization: {e_init}")
            self.logger.exception("Full traceback for __init__ failure:")
            raise # Re-raise to signal parent about the issue
        
    def reset(self, *, seed: Optional[int] = None, options: Optional[dict] = None) -> Tuple[_global_np.ndarray, Dict[str, Any]]: # Using _global_np
        reset_debug_prefix = f"[Env {self.env_id} Reset]"; 
        self.logger.debug(f"{reset_debug_prefix} --- RESET START --- Seed: {seed}, Options: {options}")
        
        try: # --- ADD THIS TRY BLOCK ---
            super().reset(seed=seed)
            if seed is not None: 
                effective_seed = seed + self.env_id
                random.seed(effective_seed)
                _global_np.random.seed(effective_seed) # Using _global_np
                self.logger.debug(f"{reset_debug_prefix} Explicitly set random/numpy seed to {effective_seed}.")

            self.round_idx = 0
            self.episode_finished = False
            self._generated_files_this_step = []
            self._last_agent_sim_reward = 0.0
            self._cumulative_rewards_this_episode = defaultdict(float)
            self._pairs_presimulated_this_step = set()

            if self.policy_cache:
                self.logger.debug(f"{reset_debug_prefix} Clearing path-based policy_cache (had {len(self.policy_cache)} entries).")
            self.policy_cache.clear()

            self.players = {pid: Player(pid) for pid in range(NUM_PLAYERS)}
            self.agent = self.players.get(0)
            if self.agent is None: 
                self.logger.critical(f"{reset_debug_prefix} Failed to assign agent P0.")
                raise RuntimeError(f"{reset_debug_prefix} Failed to assign agent P0.")

            self.logger.debug(f"{reset_debug_prefix} Resetting players states, initializing Rangers, and forcing initial units...")
            for pid, p in self.players.items():
                try: 
                    p.clear_state_for_reset()
                    self._initialize_player_ranger(p) # Add Ranger unit first
                    p.coins = FORCED_COINS
                    self._create_player_shops(p)
                    # _force_initial_unit will only run if the board is empty after Ranger placement.
                    if not p.board: # Check if board is still empty after Ranger placement
                        self._force_initial_unit(p)
                except Exception as e: 
                    self.logger.critical(f"{reset_debug_prefix} FATAL error resetting P{pid}: {e}")
                    self.logger.exception("Full traceback for player reset failure:")
                    raise # Re-raise to ensure main process is aware

            if self.current_opponent_configs:
                self.logger.debug(f"{reset_debug_prefix} Re-applying stored opponent configurations ({len(self.current_opponent_configs)} configs)...")
                for opp_id, config_tuple in self.current_opponent_configs.items():
                    if opp_id != 0 and opp_id in self.players:
                        player_obj = self.players[opp_id]
                        source_type, identifier = config_tuple
                        
                        player_obj.opponent_source_type = source_type
                        player_obj.opponent_identifier = identifier
                        
                        identifier_log_name = os.path.basename(identifier) if source_type == "path" and identifier else identifier
                        self.logger.debug(f"{reset_debug_prefix}   P{opp_id} configured to: source='{source_type}', id='{identifier_log_name or 'None'}'.")
                        
                        if source_type == "path" and identifier:
                            self.logger.debug(f"{reset_debug_prefix}     Attempting pre-load into policy_cache for P{opp_id} path-based model: '{os.path.basename(identifier)}'")
                            self._load_and_cache_policy(identifier, for_opponent_id=opp_id) 
            else:
                self.logger.debug(f"{reset_debug_prefix} No stored opponent configurations. Opponents will be default (random).")

            self.logger.debug(f"{reset_debug_prefix} Player reset and opponent config application complete. Generating initial observation...")
            try: 
                self._last_observation = self._get_obs()
                self._last_info = self._get_info()
            except Exception as e_obs: 
                self.logger.critical(f"{reset_debug_prefix} FATAL initial _get_obs() error: {e_obs}")
                self.logger.exception("Full traceback for initial observation generation failure:")
                self._last_observation = _global_np.zeros(self.observation_space.shape, dtype=self.observation_space.dtype) # Using _global_np
                self._last_info = {"env_id": self.env_id, "round": 0, "error": "reset_initial_obs_fail"}
            
            try: self._last_info = deep_convert_np_types(self._last_info, context='info_reset_return')
            except Exception as e_convert_reset: 
                self.logger.error(f"{reset_debug_prefix} ERROR converting initial info: {e_convert_reset}")
                self.logger.exception("Traceback for info conversion failure:")
                self._last_info = {"env_id": self.env_id, "round": 0, "error": "reset_info_conversion_failed"}

            if self._last_observation.shape != self.observation_space.shape:
                self.logger.warning(f"{reset_debug_prefix} Initial Obs shape mismatch: {self._last_observation.shape} vs {self.observation_space.shape}. Padding/Truncating.");
                padded_obs = _global_np.zeros(self.observation_space.shape, dtype=self.observation_space.dtype); # Using _global_np
                copy_len = min(self._last_observation.size, padded_obs.size); 
                padded_obs.flat[:copy_len] = self._last_observation.flat[:copy_len]; 
                self._last_observation = padded_obs
            if self._last_observation.dtype != _global_np.float32: self._last_observation = self._last_observation.astype(_global_np.float32) # Using _global_np
            
            self.logger.info(f"{reset_debug_prefix} Reset complete. Initial Obs valid: {_global_np.all(_global_np.isfinite(self._last_observation))}") # Using _global_np
            return self._last_observation, self._last_info
        except Exception as e_reset_outer: # --- CATCH ANY EXCEPTION HERE ---
            self.logger.critical(f"[FATAL Env {self.env_id} Reset] CRITICAL ERROR during reset: {e_reset_outer}")
            self.logger.exception("Full traceback for reset failure:")
            raise
        
    def set_opponent_configs_dict(self, configs_by_opp_id: Dict[int, Tuple[str, Optional[str]]]):
        """
        Sets opponent configurations for multiple opponents at once.
        configs_by_opp_id: Dict where key is opponent_id (int) and 
                           value is a tuple (source_type: str, identifier: Optional[str]).
        """
        if DETAILED_SELFPLAY_DEBUG and hasattr(self, 'logger') and self.logger:
            log_data_env = {
                k: (stype, os.path.basename(ident) if stype == "path" and ident else (ident if ident else "None"))
                for k, (stype, ident) in configs_by_opp_id.items()
            }
            self.logger.debug(f"[Env {self.env_id}] set_opponent_configs_dict called with: {log_data_env}")

        for opponent_id_key, config_tuple in configs_by_opp_id.items():
            if opponent_id_key == 0: # Skip agent
                continue 
            if opponent_id_key in self.players:
                source_type, identifier = config_tuple
                # Use the existing single opponent config setter
                self.set_opponent_config(opponent_id_key, source_type, identifier)
            elif DETAILED_SELFPLAY_DEBUG and hasattr(self, 'logger') and self.logger:
                self.logger.warning(f"[Env {self.env_id}] Opponent ID {opponent_id_key} from config dict not in current players.")
        
        if DETAILED_SELFPLAY_DEBUG and hasattr(self, 'logger') and self.logger:
            # Log current_opponent_configs after update for verification
            current_configs_log = {
                pid: (p_obj.opponent_source_type, 
                      os.path.basename(p_obj.opponent_identifier) if p_obj.opponent_source_type == "path" and p_obj.opponent_identifier else (p_obj.opponent_identifier if p_obj.opponent_identifier else "None")
                     )
                for pid, p_obj in self.players.items() if pid != 0
            }
            self.logger.debug(f"[Env {self.env_id}] After set_opponent_configs_dict, current live player configs: {current_configs_log}")
            self.logger.debug(f"[Env {self.env_id}] Stored self.current_opponent_configs: {self.current_opponent_configs}")

    def _load_and_cache_policy(self, model_path_to_load: str, for_opponent_id: Optional[int] = None) -> Optional[BasePolicy]:
        """
        Loads a policy from the given file PATH and caches it in self.policy_cache.
        Returns the loaded policy (on CPU) or None if loading fails.
        This is primarily for "path" sourced opponents.
        """
        if not model_path_to_load or not os.path.exists(model_path_to_load):
            if DETAILED_SELFPLAY_DEBUG:
                player_prefix = f"for P{for_opponent_id} " if for_opponent_id is not None else ""
                self.logger.debug(f"{player_prefix}Model path '{os.path.basename(model_path_to_load) if model_path_to_load else 'None'}' invalid or DNE. Cannot load.")
            return None

        if model_path_to_load in self.policy_cache:
            if DETAILED_SELFPLAY_DEBUG:
                self.logger.debug(f"Path model '{os.path.basename(model_path_to_load)}' already in cache. Skipping reload.")
            return self.policy_cache[model_path_to_load]

        log_prefix_base = f"[PolicyLoadFromPath] "
        log_prefix = f"[Opponent P{for_opponent_id} {log_prefix_base}] " if for_opponent_id is not None else log_prefix_base

        if DETAILED_SELFPLAY_DEBUG:
            self.logger.debug(f"{log_prefix}Attempting to load '{os.path.basename(model_path_to_load)}' to device '{self.device}', then moving policy to CPU for cache.")

        load_start_time = time.time()
        policy_to_use: Optional[BasePolicy] = None
        try:
            custom_obs_for_load = {
                'observation_space': self.observation_space,
                'action_space': self.action_space,
            }
            loaded_model_on_initial_device = MaskablePPO.load(
                model_path_to_load,
                device=self.device, # Load to env's main device first
                custom_objects=custom_obs_for_load
            )
            policy_to_use = loaded_model_on_initial_device.policy.to(torch.device("cpu")) # Then move to CPU for caching
            self.policy_cache[model_path_to_load] = policy_to_use
            load_duration = time.time() - load_start_time
            if DETAILED_SELFPLAY_DEBUG:
                self.logger.info(f"{log_prefix}Successfully loaded and cached path model '{os.path.basename(model_path_to_load)}' in {load_duration:.3f}s. Policy on CPU. Type: {type(policy_to_use)}")
            return policy_to_use
        except Exception as load_e:
            self.logger.error(f"{log_prefix}Failed to load path-based model '{os.path.basename(model_path_to_load)}' from path '{model_path_to_load}': {load_e}.")
            self.logger.exception("Full traceback for policy load failure:")
            
            if for_opponent_id is not None and for_opponent_id in self.players:
                self.logger.warning(f"{log_prefix}Setting P{for_opponent_id} to source_type 'random' due to load failure of '{os.path.basename(model_path_to_load)}'.")
                self.players[for_opponent_id].opponent_source_type = "random"
                self.players[for_opponent_id].opponent_identifier = None
                if for_opponent_id in self.current_opponent_configs:
                    self.current_opponent_configs[for_opponent_id] = ("random", None)
            return None

    def _populate_id_maps(self):
        global MAX_ILLUVIAL_ID, MAX_AUGMENT_ID, MAX_WEAPON_ID, MAX_WEAPON_AMPLIFIER_ID
        ill_id_counter = 1
        self.illuvial_line_to_id = {}
        self.id_to_illuvial_line = {}
        if self.illuvial_templates:
            all_lines = sorted(list(set(clean_line_type(t['LineType']) for t in self.illuvial_templates if t.get('LineType'))))
            for line in all_lines:
                if line not in self.illuvial_line_to_id:
                    self.illuvial_line_to_id[line] = ill_id_counter
                    self.id_to_illuvial_line[ill_id_counter] = line
                    ill_id_counter += 1
        if not self.illuvial_line_to_id: 
            self.logger.warning("No illuvial lines found. Adding fallback 'ErrorUnit'.")
            self.illuvial_line_to_id["ErrorUnit"] = ill_id_counter
            self.id_to_illuvial_line[ill_id_counter] = "ErrorUnit"
            ill_id_counter += 1
        MAX_ILLUVIAL_ID = max(1, ill_id_counter - 1)

        aug_id_counter = 1
        self.augment_name_to_id = {}
        self.id_to_augment_name = {}
        aug_data = self.merged_data.get("Augments", {})
        if isinstance(aug_data, dict):
             all_augments = sorted(list(aug_data.keys()))
             for full_key in all_augments:
                 augment_info = aug_data[full_key]
                 base_name = augment_info.get("Name")
                 if base_name and base_name not in self.augment_name_to_id:
                     self.augment_name_to_id[base_name] = aug_id_counter
                     self.id_to_augment_name[aug_id_counter] = base_name
                     aug_id_counter += 1
                 elif not base_name:
                      self.logger.warning(f"Augment key '{full_key}' has missing 'Name' field inside JSON. Skipping.")

        if not self.augment_name_to_id: 
            self.logger.warning("No augments found. Adding fallback 'FallbackAugment'.")
            self.augment_name_to_id["FallbackAugment"] = aug_id_counter
            self.id_to_augment_name[aug_id_counter] = "FallbackAugment"
            aug_id_counter += 1
        MAX_AUGMENT_ID = max(1, aug_id_counter - 1)

        # Populate equipment definition maps (already string-keyed from JSON)
        self.weapon_definitions = {name: data for name, data in self.weapons_data_from_db.items() if isinstance(data, dict) and data.get("Name")}
        self.weapon_amplifier_definitions = {name: data for name, data in self.weapon_amplifiers_data_from_db.items() if isinstance(data, dict) and data.get("Name")}
        self.suit_definitions = {name: data for name, data in self.suits_data_from_db.items() if isinstance(data, dict) and data.get("Name")}

        # Populate weapon name to ID maps
        weapon_id_counter = 1
        self.weapon_name_to_id = {}
        self.id_to_weapon_name = {}
        sorted_weapon_names = sorted(list(self.weapon_definitions.keys()))
        for name in sorted_weapon_names:
            if name not in self.weapon_name_to_id:
                self.weapon_name_to_id[name] = weapon_id_counter
                self.id_to_weapon_name[weapon_id_counter] = name
                weapon_id_counter += 1
        if not self.weapon_name_to_id:
            self.logger.warning("No weapons found. Adding fallback 'ErrorWeapon'.")
            self.weapon_name_to_id["ErrorWeapon"] = weapon_id_counter
            self.id_to_weapon_name[weapon_id_counter] = "ErrorWeapon"
            weapon_id_counter +=1
        MAX_WEAPON_ID = max(1, weapon_id_counter - 1)

        # Populate weapon amplifier name to ID maps
        amp_id_counter = 1
        self.weapon_amplifier_name_to_id = {}
        self.id_to_weapon_amplifier_name = {}
        sorted_amplifier_names = sorted(list(self.weapon_amplifier_definitions.keys()))
        for name in sorted_amplifier_names:
            if name not in self.weapon_amplifier_name_to_id:
                self.weapon_amplifier_name_to_id[name] = amp_id_counter
                self.id_to_weapon_amplifier_name[amp_id_counter] = name
                amp_id_counter += 1
        if not self.weapon_amplifier_name_to_id:
            self.logger.warning("No weapon amplifiers found. Adding fallback 'ErrorAmplifier'.")
            self.weapon_amplifier_name_to_id["ErrorAmplifier"] = amp_id_counter
            self.id_to_weapon_amplifier_name[amp_id_counter] = "ErrorAmplifier"
            amp_id_counter += 1
        MAX_WEAPON_AMPLIFIER_ID = max(1, amp_id_counter - 1)

        if DETAILED_SELFPLAY_DEBUG:
            self.logger.debug(f"Populated {len(self.weapon_definitions)} weapon definitions, {len(self.weapon_name_to_id)} weapon IDs. MAX_WEAPON_ID: {MAX_WEAPON_ID}")
            self.logger.debug(f"Populated {len(self.weapon_amplifier_definitions)} weapon amplifier definitions, {len(self.weapon_amplifier_name_to_id)} amplifier IDs. MAX_WEAPON_AMPLIFIER_ID: {MAX_WEAPON_AMPLIFIER_ID}")
            self.logger.debug(f"Populated {len(self.suit_definitions)} suit definitions.")

    def _populate_special_shop(self, player: Player, shop_type: str):
        """Populates the player's special shop with items of the given type."""
        debug_prefix = f"[Env {self.env_id} P{player.player_id} PopulateSpecialShop Type:{shop_type}]:"
        player.special_shop_items = [None] * 5 # Reset/Initialize with 5 slots

        if shop_type == "weapon":
            all_weapon_names = list(self.weapon_definitions.keys())
            if not all_weapon_names:
                self.logger.warning(f"{debug_prefix} No weapon definitions available to populate shop.")
                return

            num_to_sample = min(len(all_weapon_names), 5)
            sampled_weapon_names = random.sample(all_weapon_names, num_to_sample)

            for i, name in enumerate(sampled_weapon_names):
                player.special_shop_items[i] = self.weapon_definitions.get(name)
            if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"{debug_prefix} Populated with weapons: {[item.get('Name', 'None') if item else 'None' for item in player.special_shop_items]}")

        elif shop_type == "weapon_amplifier":
            if not player.ranger_weapon or not player.ranger_weapon.get('TypeID', {}).get('Name'):
                self.logger.warning(f"{debug_prefix} Cannot populate weapon_amplifier shop. Ranger has no weapon equipped or weapon name is missing.")
                return

            equipped_weapon_name = player.ranger_weapon['TypeID']['Name']
            compatible_amplifiers = [
                amp_data for amp_data in self.weapon_amplifier_definitions.values()
                if amp_data.get('AmplifierForWeapon', {}).get('Name') == equipped_weapon_name
            ]

            if not compatible_amplifiers:
                self.logger.warning(f"{debug_prefix} No compatible amplifiers found for weapon '{equipped_weapon_name}'.")
                return

            num_to_sample = min(len(compatible_amplifiers), 5)
            sampled_amplifiers = random.sample(compatible_amplifiers, num_to_sample)

            for i, amp_data in enumerate(sampled_amplifiers):
                player.special_shop_items[i] = amp_data
            if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"{debug_prefix} Populated with amplifiers for '{equipped_weapon_name}': {[item.get('Name', 'None') if item else 'None' for item in player.special_shop_items]}")

        else:
            self.logger.warning(f"{debug_prefix} Unknown special shop type: {shop_type}")


    def _initialize_player_ranger(self, player: Player):
        """Initializes the Ranger unit for a player."""
        debug_prefix = f"[Env {self.env_id} P{player.player_id} InitRanger]:"
        player.ranger_unit_id = generate_random_unit_id()

        # Suit
        jumpsuit_def = self.suit_definitions.get("Jumpsuit")
        equipped_suit_type_id: Dict[str, Any]
        if jumpsuit_def:
            player.ranger_suit = jumpsuit_def # Store full definition
            equipped_suit_type_id = {
                "Name": jumpsuit_def.get("Name", "Jumpsuit"),
                "Stage": jumpsuit_def.get("Stage", 1),
                "Variation": jumpsuit_def.get("Variation", "Original")
            }
            if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"{debug_prefix} Jumpsuit found and assigned.")
        else:
            self.logger.warning(f"{debug_prefix} 'Jumpsuit' not found in suit_definitions. Using placeholder for Ranger suit.")
            player.ranger_suit = {"Name": "Jumpsuit_Placeholder", "Stats": {"MaxHealth": 300000}} # Minimal placeholder
            equipped_suit_type_id = {"Name": "Jumpsuit", "Stage": 1, "Variation": "Original"}

        ranger_instance = {
            "ID": player.ranger_unit_id,
            "DominantCombatAffinity": "None",
            "DominantCombatClass": "None",
            "BondedId": "",
            "Level": 1,
            "EquippedWeapon": {
                "ID": generate_random_unit_id(), # Instance ID for the equipped weapon
                "TypeID": {"Name": "", "Stage": 0, "Variation": "Original", "CombatAffinity": "None"}, # Definition (empty by default)
                "EquippedAmplifiers": [] # List of amplifier instances
            },
            "EquippedSuit": { # Instance of the suit
                "ID": generate_random_unit_id(), # Instance ID for the equipped suit
                "TypeID": equipped_suit_type_id # Definition from Jumpsuit
            },
            "EquippedConsumables": [],
            "EquippedAugments": [],
            "Finish": "None"
        }
        # Player attributes now point to the INSTANCE dicts within the Ranger's Instance data
        player.ranger_weapon = ranger_instance["EquippedWeapon"]
        player.ranger_weapon_amplifiers = ranger_instance["EquippedWeapon"]["EquippedAmplifiers"]
        # player.ranger_suit already stores the definition, the instance is in ranger_instance["EquippedSuit"]


        ranger_type_id = {
            "UnitType": "Ranger",
            "LineType": "FemaleRanger", # Default, can be configurable later if needed
            "Stage": 0, # Rangers don't have stages like Illuvials
            "Path": "",
            "Variation": ""
        }

        # Assign position
        # Simple unique position for now, can be made more robust
        pos_idx = player.player_id % len(POSITIONS_BLUE_LIST)
        chosen_pos_coords = POSITIONS_BLUE_LIST[pos_idx]

        # Check if the chosen position is already occupied by another Ranger (unlikely with simple modulo but good practice)
        # This is a basic check; a more robust system would ensure no overlap if MAX_BOARD_SLOTS < NUM_PLAYERS
        is_occupied = any(
            u.get("Position", {}).get("Q") == chosen_pos_coords[0] and
            u.get("Position", {}).get("R") == chosen_pos_coords[1]
            for other_p_id, other_p in self.players.items() if other_p_id != player.player_id for u in other_p.board
        )
        if is_occupied: # Fallback if somehow occupied, find first free or use the first one
            self.logger.warning(f"{debug_prefix} Position {chosen_pos_coords} for P{player.player_id} Ranger was occupied. Finding fallback.")
            fallback_pos = self._find_free_tile_for_player(player, preferred_unified_id=1) # Try first tile
            if fallback_pos: chosen_pos_coords = fallback_pos
            else: # Absolute fallback if no free tiles (should not happen in reset)
                 chosen_pos_coords = POSITIONS_BLUE_LIST[0]
                 self.logger.error(f"{debug_prefix} No free tiles for Ranger. Using {chosen_pos_coords}. Expect issues.")

        ranger_position = {"Q": chosen_pos_coords[0], "R": chosen_pos_coords[1]}

        ranger_unit_dict = {
            "TypeID": ranger_type_id,
            "Instance": ranger_instance,
            "Position": ranger_position
        }
        player.board.append(ranger_unit_dict)
        if DETAILED_ACTION_DEBUG: # Changed from DETAILED_SELFPLAY_DEBUG
            jumpsuit_name = player.ranger_suit.get("Name", "UnknownSuit") if player.ranger_suit else "NoSuitDef"
            # The suit instance ID is within ranger_instance before it's appended to board
            suit_instance_id = ranger_instance.get("EquippedSuit", {}).get("ID", "UnknownSuitID_BoardLogic")
            self.logger.debug(f"{debug_prefix} Ranger ID: {player.ranger_unit_id}, Jumpsuit: '{jumpsuit_name}' (Instance: {suit_instance_id}), Position: {ranger_position}")


    def _get_illuvial_id(self, line_type: Optional[str]) -> int: return self.illuvial_line_to_id.get(clean_line_type(line_type), 0) if line_type else 0
    def _get_augment_id(self, augment_name: Optional[str]) -> int: return self.augment_name_to_id.get(augment_name, 0) if augment_name else 0
    def _get_position_id(self, position: Optional[Dict[str, int]]) -> int:
        if position is None: return 0
        q = position.get("Q")
        r = position.get("R")
        return BLUE_POS_TO_UNIFIED_ID_MAP.get((q, r), 0) if q is not None and r is not None else 0
    
    # --- MODIFIED _determine_pairings with detailed logging ---
    # In train_env.py, inside IlluviumSingleEnv class

    def _determine_pairings(self) -> List[Tuple[int, int]]:
        log_prefix = f"[Env {self.env_id} Rnd {self.round_idx} _determine_pairings]"
        _log_debug = self.logger.debug if DETAILED_SELFPLAY_DEBUG else (lambda msg: None) # Use direct logger

        alive_players_list: List[Player] = [p for p_id, p in self.players.items() if p.is_alive()]
        _log_debug(f"Alive players: {[p.player_id for p in alive_players_list]} (Count: {len(alive_players_list)})")
        
        if len(alive_players_list) < 2:
            _log_debug(f"Less than 2 alive players. Returning empty pairings.")
            return []
        
        # --- MODIFIED LOGIC FOR PAIRING ---
        # Create a list of players currently available for pairing
        available_players_for_pairing = list(alive_players_list)
        random.shuffle(available_players_for_pairing) # Shuffle for randomness
        
        pairings = []
        
        _log_debug(f"Starting pairing loop with {len(available_players_for_pairing)} available players.")

        # Iterate until fewer than 2 players are available
        while len(available_players_for_pairing) >= 2:
            p1 = available_players_for_pairing.pop(0) # Take the first player from the shuffled list
            
            # Find potential opponents for p1 from the rest of the list
            potential_opponents = list(available_players_for_pairing) # Copy to iterate over
            
            if not potential_opponents:
                _log_debug(f"No potential opponents left for P{p1.player_id}. P{p1.player_id} gets a bye.")
                break # p1 gets a bye if no one else is left
            
            # Prefer not to play the last opponent
            preferred_opponents = [p for p in potential_opponents if p.player_id != p1.last_opponent_id]
            
            p2: Optional[Player] = None
            if preferred_opponents:
                p2 = random.choice(preferred_opponents)
                _log_debug(f"P{p1.player_id} selected preferred opponent P{p2.player_id} (was not last_opponent_id={p1.last_opponent_id}).")
            elif potential_opponents:
                p2 = random.choice(potential_opponents)
                _log_debug(f"P{p1.player_id} selected P{p2.player_id} (no non-last_opponent available).")
            else: 
                _log_debug(f"Logic error: P{p1.player_id} has no potential opponents left after checks. Breaking.")
                break

            if p2 is None: 
                _log_debug(f"P2 is None for P{p1.player_id}. This indicates a logic error in pairing. Breaking.")
                break

            pairings.append(tuple(sorted((p1.player_id, p2.player_id))))
            
            # Remove p2 from the list of available players
            # This is crucial: we remove by object identity to ensure the correct player is removed.
            try:
                available_players_for_pairing.remove(p2)
            except ValueError:
                self.logger.error(f"{log_prefix} [ERROR] P{p2.player_id} (selected as P2) not found in available_players_for_pairing. This is a severe logic error. Continuing but expect issues.")
                # If this happens, it indicates a deep bug in list management.
                # For safety, we might break or skip the current pair.

            # Update last opponent for both
            p1.last_opponent_id = p2.player_id
            p2.last_opponent_id = p1.player_id
            
            _log_debug(f"Formed pair: ({p1.player_id}, {p2.player_id}). Remaining available: {[p.player_id for p in available_players_for_pairing]}")

        # Any remaining player in available_players_for_pairing gets a bye.
        if available_players_for_pairing:
            _log_debug(f"Player(s) left unmatched: {[p.player_id for p in available_players_for_pairing]}. They get a bye (or no battle).")
            
        _log_debug(f"Finished pairing. Returning {len(pairings)} pairings: {pairings}")
        return pairings

    def _is_position_occupied(self, q: int, r: int, player: Player) -> bool: return player.get_unit_at_pos(q, r) is not None
    
    def _find_free_tile_for_player(self, player: Player, preferred_unified_id: Optional[int] = None) -> Optional[Tuple[int, int]]:
         if preferred_unified_id is not None and preferred_unified_id in UNIFIED_ID_TO_BLUE_POS_MAP:
              blue_tile = UNIFIED_ID_TO_BLUE_POS_MAP[preferred_unified_id];
              if not self._is_position_occupied(blue_tile[0], blue_tile[1], player):
                  return blue_tile
         available_blue_tiles = [pos for i, pos in enumerate(POSITIONS_BLUE_LIST) if not self._is_position_occupied(pos[0], pos[1], player)];
         if not available_blue_tiles: return None; return random.choice(available_blue_tiles)

    def _force_initial_unit(self, player: Player):
        # This function should ensure ALL players have a unit on board initially for battles to happen
        if player.board or player.bench: return # Already has units, don't force
        
        ills = self._get_valid_illuvials_for_tier(tier=1); # Start with T1
        if not ills: 
            self.logger.critical(f"[FATAL Env {self.env_id} P{player.player_id}] No T1 Illuvials available to force initial unit! Check illuvial_templates.json.")
            raise RuntimeError(f"FATAL: No Tier 1 Illuvials defined for Env {self.env_id}. Cannot force initial unit.")
        
        ill_data = random.choice(ills).copy(); 
        tile_blue_coords = self._find_free_tile_for_player(player, preferred_unified_id=1);
        if tile_blue_coords is None: 
            self.logger.warning(f"[Env {self.env_id} P{player.player_id}] No free tiles for initial unit. Using first available blue tile as fallback.")
            tile_blue_coords = POSITIONS_BLUE_LIST[0] if POSITIONS_BLUE_LIST else (0,0); # Fallback to first blue tile
            if not POSITIONS_BLUE_LIST: self.logger.error("POSITIONS_BLUE_LIST is empty, cannot place initial unit.")
        
        line_t = clean_line_type(ill_data.get("LineType", "ErrorUnit")); 
        cost = get_illuvial_cost(ill_data); 
        inst = {"ID": generate_random_unit_id(), "Cost": cost, "EquippedAugments": []};
        if "DominantCombatClass" in ill_data: inst["DominantCombatClass"] = ill_data["DominantCombatClass"]
        if "DominantCombatAffinity" in ill_data: inst["DominantCombatAffinity"] = ill_data["DominantCombatAffinity"]
        type_id = { "UnitType": "Illuvial", "LineType": line_t, "Stage": ill_data.get("Stage", 1), "Path": ill_data.get("Path", "Default"), "Variation": ill_data.get("Variation", "Original") }
        new_unit = {"TypeID": type_id, "Instance": inst, "Position": {"Q": tile_blue_coords[0], "R": tile_blue_coords[1]} }; 
        player.board.append(new_unit)
        self.logger.debug(f"[Env {self.env_id} P{player.player_id}] Forced initial unit '{line_t}' (ID:{inst['ID']}) to {tile_blue_coords}.")


    def _check_level_up(self, player: Player):
        leveled_up = False;
        while player.level < MAX_LEVEL:
            xp_needed = LEVEL_XP_THRESHOLDS.get(player.level);
            if xp_needed is None or player.xp < xp_needed: break
            player.xp -= xp_needed; player.level += 1; leveled_up = True;
        if leveled_up:
             if DETAILED_ACTION_DEBUG: self.logger.debug(f"[Env {self.env_id} P{player.player_id}] Leveled up to L{player.level}!")
             self._create_player_shops(player) # Refresh shop on level up

    def _get_unit_by_combined_index(self, player: Player, index_1based: int) -> Tuple[Optional[Dict[str, Any]], str, int]:
        """Gets a unit by its 1-based index in the combined board+bench list."""
        try:
            index_1based = int(index_1based)
        except (ValueError, TypeError):
             if DETAILED_ACTION_DEBUG: self.logger.debug(f"[Env {self.env_id} _get_unit P{player.player_id}] Failed convert index_1based to int: {index_1based}")
             return None, "none", -1
        if index_1based <= 0:
            if DETAILED_ACTION_DEBUG: self.logger.debug(f"[Env {self.env_id} _get_unit P{player.player_id}] Invalid index_1based: {index_1based}")
            return None, "none", -1
        index_0based = index_1based - 1

        current_board = list(player.board) # Get fresh copies of lists
        current_bench = list(player.bench)
        current_combined = current_board + current_bench
        current_combined_len = len(current_combined)

        if not (0 <= index_0based < current_combined_len):
             if DETAILED_ACTION_DEBUG: self.logger.debug(f"[Env {self.env_id} _get_unit P{player.player_id}] Index {index_0based} out of bounds for len {current_combined_len}")
             return None, "none", -1
        target_unit = current_combined[index_0based]
        
        try:
            board_index = current_board.index(target_unit) # Check if unit object is on board
            return target_unit, "board", board_index
        except ValueError:
            try:
                bench_index = current_bench.index(target_unit) # Check if unit object is on bench
                return target_unit, "bench", bench_index
            except ValueError:
                if DETAILED_ACTION_DEBUG: self.logger.debug(f"[Env {self.env_id} _get_unit P{player.player_id} CRITICAL] Unit from combined list not found in board or bench! Unit ID: {target_unit.get('Instance',{}).get('ID')}")
                return None, "none", -1

    def _remove_unit_by_combined_index(self, player: Player, index_0based: int) -> bool:
        debug_prefix = f"    [REMOVE_UNIT P{player.player_id} Idx0={index_0based}]:"; 
        combined_units = player.get_combined_units();
        board_size = len(player.board) 

        if 0 <= index_0based < len(combined_units):
             unit_to_remove = combined_units[index_0based]; unit_id_to_remove = unit_to_remove.get("Instance", {}).get("ID")
             if not unit_id_to_remove: self.logger.error(f"{debug_prefix} [ERROR] Unit at index {index_0based} has no ID."); return False
             
             try:
                 if index_0based < board_size: # Unit should be on the board
                     try:
                         player.board.remove(unit_to_remove)
                         if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Removed board unit (ID: {unit_id_to_remove}) by object identity.")
                         return True
                     except ValueError: 
                          original_board_len = len(player.board)
                          player.board = [u for u in player.board if u.get("Instance", {}).get("ID") != unit_id_to_remove]
                          if len(player.board) < original_board_len:
                              if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Removed board unit (ID: {unit_id_to_remove}) by ID fallback.")
                              return True
                          else: self.logger.error(f"{debug_prefix} [ERROR] Remove board unit (ID: {unit_id_to_remove}) failed (not found by ID).")
                          return False
                 else: # Unit should be on the bench
                     try:
                         player.bench.remove(unit_to_remove)
                         if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Removed bench unit (ID: {unit_id_to_remove}) by object identity.")
                         return True
                     except ValueError: 
                          original_bench_len = len(player.bench)
                          player.bench = [u for u in player.bench if u.get("Instance", {}).get("ID") != unit_id_to_remove]
                          if len(player.bench) < original_bench_len:
                              if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Removed bench unit (ID: {unit_id_to_remove}) by ID fallback.")
                              return True
                          else: self.logger.error(f"{debug_prefix} [ERROR] Remove bench unit (ID: {unit_id_to_remove}) failed (not found by ID).")
                          return False
             except Exception as e_rem: 
                 self.logger.error(f"{debug_prefix} [ERROR] Unexpected error during remove unit (ID: {unit_id_to_remove}). Error: {e_rem}"); self.logger.exception("Traceback for remove unit error:")
                 return False
        else: 
            self.logger.error(f"{debug_prefix} [ERROR] Invalid remove index: {index_0based}. Combined units: {len(combined_units)}")
            return False

    def _check_and_merge_units(self, player: Player) -> float:
        total_merge_reward = 0.0
        merge_debug_prefix = f"    [MERGE P{player.player_id}]:"

        while True: 
            merged_in_this_pass = False
            for stage_to_check in range(1, MAX_STAGE):
                target_stage = stage_to_check + 1
                if DETAILED_MERGE_DEBUG: self.logger.debug(f"{merge_debug_prefix} Checking for Stage {stage_to_check} -> {target_stage} merges.")

                units_to_check = player.get_combined_units()
                potential_merge_units = [unit for unit in units_to_check if unit.get("TypeID", {}).get("Stage") == stage_to_check ]

                if len(potential_merge_units) < 2:
                    if DETAILED_MERGE_DEBUG: self.logger.debug(f"{merge_debug_prefix} Less than 2 Stage {stage_to_check} units. Skipping.")
                    continue

                groups = defaultdict(list)
                for unit in potential_merge_units:
                    type_id = unit.get("TypeID")
                    if type_id:
                        key = (type_id.get("LineType"), type_id.get("Path"), type_id.get("Variation"))
                        groups[key].append(unit)

                if DETAILED_MERGE_DEBUG: 
                    self.logger.debug(f"{merge_debug_prefix} Found {len(groups)} groups for Stage {stage_to_check} merge candidates.")
                    for k,v in groups.items():
                        self.logger.debug(f"  Group {k}: {len(v)} units.")

                for key, units_in_group in groups.items():
                    if len(units_in_group) >= 2:
                        unit1, unit2 = units_in_group[0], units_in_group[1]
                        if DETAILED_MERGE_DEBUG: self.logger.debug(f"{merge_debug_prefix} Potential merge of {key} (Units ID: {unit1.get('Instance',{}).get('ID')}, {unit2.get('Instance',{}).get('ID')})")
                        
                        # Get indices *after* ensuring both units are still present in a fresh list
                        combined_units_current_for_idx = player.get_combined_units() 
                        try:
                            idx1_0based = combined_units_current_for_idx.index(unit1)
                            idx2_0based = combined_units_current_for_idx.index(unit2)
                        except ValueError:
                            self.logger.warning(f"{merge_debug_prefix} [WARN] One of the merge units not found in current combined list during indexing. Skipping pair {key}.")
                            continue

                        if len(player.bench) >= BENCH_MAX_SIZE:
                            if DETAILED_MERGE_DEBUG: self.logger.debug(f"{merge_debug_prefix} Bench full ({len(player.bench)}/{BENCH_MAX_SIZE}). Cannot merge {key}.")
                            continue 

                        # Remove units: remove higher index first to avoid shifting lower index
                        remove_success1 = self._remove_unit_by_combined_index(player, max(idx1_0based, idx2_0based)) 
                        remove_success2 = self._remove_unit_by_combined_index(player, min(idx1_0based, idx2_0based))

                        if remove_success1 and remove_success2:
                            new_merged_instance = {"ID": generate_random_unit_id(), "Cost": unit1.get("Instance", {}).get("Cost", 1), "EquippedAugments": [], "DominantCombatClass": unit1.get("Instance", {}).get("DominantCombatClass"), "DominantCombatAffinity": unit1.get("Instance", {}).get("DominantCombatAffinity")}
                            new_merged_type_id = unit1["TypeID"].copy(); new_merged_type_id["Stage"] = target_stage
                            new_merged_unit = {"TypeID": new_merged_type_id, "Instance": new_merged_instance, "Position": None } # Position None means on bench
                            player.bench.append(new_merged_unit)
                            merge_reward_this_merge = MERGE_STAGE_REWARD; total_merge_reward += merge_reward_this_merge
                            merged_in_this_pass = True
                            self.logger.info(f"[INFO Env {self.env_id} P{player.player_id}] *** MERGE SUCCESSFUL *** Stage {stage_to_check} -> {target_stage} for {key[0]}. Reward: +{merge_reward_this_merge:.3f}")
                            if DETAILED_MERGE_DEBUG: 
                                self.logger.debug(f"{merge_debug_prefix} Merged {unit1.get('TypeID',{}).get('LineType')} to Stage {target_stage}. New unit: {new_merged_unit.get('Instance',{}).get('ID')}. Bench size: {len(player.bench)}")
                            break # Break from group loop to restart pass (as lists are modified)
                        else:
                            self.logger.error(f"{merge_debug_prefix} [ERROR] Failed to remove one/both units for merge {key}. Aborting merge. This indicates an issue with _remove_unit_by_combined_index.")
                            continue 
                    if merged_in_this_pass: break # Break from group loop
                if merged_in_this_pass: break # Break from stage loop to restart pass
            if not merged_in_this_pass: break # No merges in a full pass through all stages
        return total_merge_reward

    def _get_valid_illuvials_for_tier(self, tier: int) -> List[Dict[str, Any]]:
        if not isinstance(self.illuvial_templates, list): return []
        target_tiers = {tier};
        if tier == 1: target_tiers.add(0) # Allow tier 0 if tier 1 is requested
        valid = [item for item in self.illuvial_templates if item.get("Tier") in target_tiers and item.get("LineType")]
        return valid

    def _create_player_shops(self, player: Player):
        player.illuvial_shop = [None] * SHOP_SIZE;
        level_odds = SHOP_ODDS_ADJUSTED.get(player.level, SHOP_ODDS_ADJUSTED[1])
        tiers_available = list(level_odds.keys())
        probabilities = list(level_odds.values())
        valid_tiers_probs = [(t, p) for t, p in zip(tiers_available, probabilities) if p > 0]

        if not valid_tiers_probs:
            self.logger.warning(f"[Env {self.env_id} P{player.player_id}] No valid shop odds for level {player.level}. Defaulting to T0.")
            valid_tiers = [0]; valid_probs_list = [1.0]
        else:
            valid_tiers, valid_probs_list = zip(*valid_tiers_probs)
            valid_tiers = list(valid_tiers); valid_probs_list = list(valid_probs_list)
            
        for i in range(SHOP_SIZE):
            if not valid_tiers: continue;

            chosen_tier = random.choices(valid_tiers, weights=valid_probs_list, k=1)[0];
            candidates = self._get_valid_illuvials_for_tier(chosen_tier)

            if candidates:
                player.illuvial_shop[i] = random.choice(candidates).copy()
            else:
                 if chosen_tier != 0:
                     candidates_fallback_t0 = self._get_valid_illuvials_for_tier(0)
                 else:
                     candidates_fallback_t0 = []
                     
                 if candidates_fallback_t0:
                     self.logger.warning(f"[Env {self.env_id} P{player.player_id}] No candidates for T{chosen_tier} (L{player.level}). Using T0 fallback for shop slot {i}.")
                     player.illuvial_shop[i] = random.choice(candidates_fallback_t0).copy()
                     continue

                 candidates_fallback_t1 = self._get_valid_illuvials_for_tier(1);
                 if candidates_fallback_t1:
                     self.logger.warning(f"[Env {self.env_id} P{player.player_id}] No candidates for T{chosen_tier} or T0 (L{player.level}). Using T1 fallback for shop slot {i}.")
                     player.illuvial_shop[i] = random.choice(candidates_fallback_t1).copy()
                 else:
                     self.logger.critical(f"No T0 or T1 candidates found! Cannot fill shop slot {i}. Check illuvial_templates.json.")
                     player.illuvial_shop[i] = None 
                     if not any(ill.get("Tier") in [0,1] for ill in self.illuvial_templates):
                         raise RuntimeError(f"FATAL: No Tier 0 or Tier 1 Illuvials defined in templates for Env {self.env_id}. Cannot populate shop.")

    def get_new_flat_index(self, action_type_idx: int, entity_idx: int, position_idx: int, item_idx: int) -> int:
        """ Encodes a 4D action into a flat index. """
        if not (0 <= action_type_idx < NUM_ACTION_TYPES): raise ValueError(f"Invalid action_type_idx: {action_type_idx}")
        if not (0 <= entity_idx < DIM_ENTITY_SIZE): raise ValueError(f"Invalid entity_idx: {entity_idx}")
        if not (0 <= position_idx < DIM_POSITION_SIZE): raise ValueError(f"Invalid position_idx: {position_idx}")
        if not (0 <= item_idx < DIM_ITEM_SIZE): raise ValueError(f"Invalid item_idx: {item_idx}")

        # Formula for flattening a 4D index
        # flat_idx = item_idx +
        #            position_idx * DIM_ITEM_SIZE +
        #            entity_idx * DIM_ITEM_SIZE * DIM_POSITION_SIZE +
        #            action_type_idx * DIM_ITEM_SIZE * DIM_POSITION_SIZE * DIM_ENTITY_SIZE

        # Simpler calculation order
        flat_idx = action_type_idx * (DIM_ENTITY_SIZE * DIM_POSITION_SIZE * DIM_ITEM_SIZE) + \
                   entity_idx     * (DIM_POSITION_SIZE * DIM_ITEM_SIZE) + \
                   position_idx   * (DIM_ITEM_SIZE) + \
                   item_idx
        return int(flat_idx) # Ensure it's an int

    def decode_new_flat_action(self, flat_action_index: int) -> Tuple[int, int, int, int]:
        """ Decodes a flat action index back into its 4D components. """
        if not (0 <= flat_action_index < NEW_TOTAL_FLAT_ACTIONS):
            self.logger.error(f"[DecodeAction] Flat index {flat_action_index} is out of bounds for {NEW_TOTAL_FLAT_ACTIONS}. Returning (0,0,0,0).")
            return 0, 0, 0, 0 # Return a default valid action (e.g. PASS)

        # Reverse the flattening formula
        action_type_idx = flat_action_index // (DIM_ENTITY_SIZE * DIM_POSITION_SIZE * DIM_ITEM_SIZE)
        remainder = flat_action_index % (DIM_ENTITY_SIZE * DIM_POSITION_SIZE * DIM_ITEM_SIZE)

        entity_idx = remainder // (DIM_POSITION_SIZE * DIM_ITEM_SIZE)
        remainder = remainder % (DIM_POSITION_SIZE * DIM_ITEM_SIZE)

        position_idx = remainder // DIM_ITEM_SIZE
        item_idx = remainder % DIM_ITEM_SIZE

        return int(action_type_idx), int(entity_idx), int(position_idx), int(item_idx)

    def action_masks(self) -> _global_np.ndarray:
        mask_debug_prefix = f"[DEBUG MASK Env {self.env_id} P{self.agent.player_id if self.agent else 'N/A'} R{self.round_idx}]"
        flat_mask = _global_np.zeros(NEW_TOTAL_FLAT_ACTIONS, dtype=bool)

        try:
            player = self.agent
            if player is None:
                self.logger.error(f"{mask_debug_prefix} Agent is None, returning mostly False mask.")
                # Enable pass action if possible
                try:
                    pass_flat_idx = self.get_new_flat_index(AT_SYSTEM_ACTIONS, 0, 0, SYSTEM_ACTION_PASS)
                    if 0 <= pass_flat_idx < NEW_TOTAL_FLAT_ACTIONS: flat_mask[pass_flat_idx] = True
                except Exception as e_pass:
                     self.logger.error(f"{mask_debug_prefix} Error getting pass_flat_idx for None agent: {e_pass}")
                     if NEW_TOTAL_FLAT_ACTIONS > 0: flat_mask[0] = True # Fallback to first action if pass fails
                return flat_mask

            # Iterate through all possible actions by their components
            for action_type_idx_loop in range(NUM_ACTION_TYPES):
                for entity_idx_loop in range(DIM_ENTITY_SIZE):
                    for position_idx_loop in range(DIM_POSITION_SIZE):
                        for item_idx_loop in range(DIM_ITEM_SIZE):
                            current_action_valid = False
                            flat_idx = -1 # Initialize to invalid
                            try:
                                flat_idx = self.get_new_flat_index(action_type_idx_loop, entity_idx_loop, position_idx_loop, item_idx_loop)
                            except ValueError: # Should not happen if loops are correct
                                if DETAILED_ACTION_DEBUG: self.logger.error(f"{mask_debug_prefix} Invalid index combination in loop: {action_type_idx_loop},{entity_idx_loop},{position_idx_loop},{item_idx_loop}")
                                continue # Skip this invalid combination

                            if action_type_idx_loop == AT_SYSTEM_ACTIONS:
                                # For system actions, entity_idx and position_idx are usually fixed (e.g., 0)
                                if entity_idx_loop == 0 and position_idx_loop == 0:
                                    if item_idx_loop == SYSTEM_ACTION_PASS: # item_idx is the specific system action
                                        current_action_valid = player.current_phase in ["planning", "augment"] or player.special_shop_active
                                    elif item_idx_loop == SYSTEM_ACTION_REROLL:
                                        current_action_valid = player.current_phase == "planning" and player.coins >= REROLL_COST and not player.special_shop_active
                                    elif item_idx_loop == SYSTEM_ACTION_BUY_XP:
                                        current_action_valid = player.current_phase == "planning" and player.coins >= BUY_XP_COST and not player.special_shop_active
                                    elif item_idx_loop == SYSTEM_ACTION_EXIT_PLACEMENT:
                                        current_action_valid = player.current_phase == "placement"
                                    elif item_idx_loop == SYSTEM_ACTION_END_PLANNING:
                                        # Can end planning if in planning OR if in augment phase (implies choice made/skipped)
                                        current_action_valid = (player.current_phase == "planning" and not player.special_shop_active) or \
                                                               (player.current_phase == "augment" and player.is_augment_choice_pending)


                            elif action_type_idx_loop == AT_SELL_UNIT:
                                # For sell actions, position_idx and item_idx are usually fixed (e.g., 0)
                                if position_idx_loop == 0 and item_idx_loop == 0:
                                    if player.current_phase == "planning" and not player.special_shop_active:
                                        combined_units = player.get_combined_units()
                                        num_combined_units = len(combined_units)
                                        # entity_idx_loop is the 0-based index for combined_units
                                        if 0 <= entity_idx_loop < num_combined_units:
                                            target_unit_data = combined_units[entity_idx_loop]
                                            if target_unit_data.get("Instance", {}).get("ID") != player.ranger_unit_id:
                                                current_action_valid = True

                            elif action_type_idx_loop == AT_BUY_ILLUVIAL:
                                # For buy illuvial, entity_idx and position_idx are usually fixed (e.g., 0)
                                if entity_idx_loop == 0 and position_idx_loop == 0:
                                    if player.current_phase == "planning" and not player.special_shop_active:
                                        # item_idx_loop is the 0-based shop slot index
                                        if 0 <= item_idx_loop < SHOP_SIZE:
                                            if player.illuvial_shop[item_idx_loop] is not None:
                                                cost = get_illuvial_cost(player.illuvial_shop[item_idx_loop])
                                                if player.coins >= cost and len(player.bench) < BENCH_MAX_SIZE:
                                                    current_action_valid = True

                            # Placeholder for other action types (AT_PLACE_UNIT, AT_RANGER_EQUIP, AT_RANGER_BOND)
                            # For now, these will remain False unless explicitly enabled by future logic
                            elif action_type_idx_loop == AT_PLACE_UNIT:
                                if player.current_phase == "placement" and not player.special_shop_active:
                                    combined_units = player.get_combined_units()
                                    num_combined_units = len(combined_units)
                                    # entity_idx_loop is the unit to move (0 to num_combined_units-1)
                                    if 0 <= entity_idx_loop < num_combined_units:
                                        unit_to_move = combined_units[entity_idx_loop]
                                        unit_id_to_move = unit_to_move.get("Instance", {}).get("ID")

                                        # position_idx_loop: 0 for bench, 1 to DIM_POSITION_SIZE-1 for board slots
                                        # DIM_POSITION_SIZE is 12, so board slots are 1-11.
                                        # These map to UNIFIED_ID_TO_BLUE_POS_MAP keys 1 through 11.
                                        if 0 <= position_idx_loop < DIM_POSITION_SIZE:
                                            is_moving_to_bench = (position_idx_loop == 0)

                                            if is_moving_to_bench:
                                                # Cannot move Ranger to bench
                                                if unit_id_to_move == player.ranger_unit_id:
                                                    current_action_valid = False
                                                # Unit must be on board to be moved to bench
                                                elif any(u.get("Instance", {}).get("ID") == unit_id_to_move for u in player.board):
                                                     current_action_valid = len(player.bench) < BENCH_MAX_SIZE
                                                else: # Unit is already on bench or doesn't exist on board
                                                    current_action_valid = False
                                            else: # Moving to a board slot (position_idx_loop is 1 to 11)
                                                target_board_slot_unified_id = position_idx_loop # Direct mapping
                                                if not (1 <= target_board_slot_unified_id <= MAX_BOARD_SLOTS):
                                                    current_action_valid = False # Invalid board slot ID
                                                else:
                                                    target_coords = UNIFIED_ID_TO_BLUE_POS_MAP.get(target_board_slot_unified_id)
                                                    if not target_coords: # Should not happen if ID is valid
                                                        current_action_valid = False
                                                    else:
                                                        unit_at_target_coords = player.get_unit_at_pos(target_coords[0], target_coords[1])
                                                        is_unit_on_bench = any(u.get("Instance", {}).get("ID") == unit_id_to_move for u in player.bench)

                                                        if unit_at_target_coords and unit_at_target_coords.get("Instance",{}).get("ID") == unit_id_to_move:
                                                            current_action_valid = False # Moving to its own spot is no-op / invalid
                                                        elif is_unit_on_bench: # Moving from bench to board
                                                            if unit_at_target_coords: # Target slot occupied, implies swap
                                                                current_action_valid = True # Swap bench <-> board always allowed if bench has space (implicit)
                                                            else: # Target slot empty
                                                                current_action_valid = player.units_on_board_count() < player.level
                                                        else: # Moving from board to board
                                                            current_action_valid = True # Board to board move/swap always allowed

                            elif action_type_idx_loop == AT_RANGER_EQUIP:
                                if player.current_phase == "planning" and player.special_shop_active:
                                    # item_idx_loop is the special shop slot (0-4 for a 5-slot shop)
                                    if 0 <= item_idx_loop < 5 and player.special_shop_items[item_idx_loop] is not None:
                                        if player.special_shop_type == "weapon":
                                            current_action_valid = True # Always possible to equip a new weapon
                                        elif player.special_shop_type == "weapon_amplifier":
                                            # Can equip if weapon exists and has < 2 amps
                                            if player.ranger_weapon and player.ranger_weapon.get("TypeID", {}).get("Name") and \
                                               len(player.ranger_weapon_amplifiers) < 2:
                                                current_action_valid = True

                            elif action_type_idx_loop == AT_RANGER_BOND:
                                if player.current_phase == "planning" and not player.special_shop_active:
                                    combined_units = player.get_combined_units()
                                    num_combined_units = len(combined_units)
                                    # entity_idx_loop is the target Illuvial for bonding
                                    if 0 <= entity_idx_loop < num_combined_units:
                                        target_unit = combined_units[entity_idx_loop]
                                        # Must be an Illuvial and not the Ranger itself
                                        if target_unit.get("TypeID", {}).get("UnitType") == "Illuvial" and \
                                           target_unit.get("Instance", {}).get("ID") != player.ranger_unit_id:
                                            current_action_valid = True

                            if current_action_valid:
                                flat_mask[flat_idx] = True

            # Fallback: If no actions are valid, enable PASS action
            if not flat_mask.any():
                self.logger.warning(f"{mask_debug_prefix} No valid actions found after full check! Phase='{player.current_phase}', SpecialShop='{player.special_shop_active}'. Enabling PASS action as fallback.")
                pass_flat_idx = -1
                try:
                    pass_flat_idx = self.get_new_flat_index(AT_SYSTEM_ACTIONS, 0, 0, SYSTEM_ACTION_PASS)
                    if 0 <= pass_flat_idx < NEW_TOTAL_FLAT_ACTIONS:
                        flat_mask[pass_flat_idx] = True
                    else:
                        self.logger.error(f"{mask_debug_prefix} Fallback PASS action index {pass_flat_idx} out of bounds!")
                        if NEW_TOTAL_FLAT_ACTIONS > 0: flat_mask[0] = True # Absolute fallback
                except ValueError as e_val: # Catch error from get_new_flat_index if constants are bad
                     self.logger.error(f"{mask_debug_prefix} Error calculating fallback PASS action index: {e_val}. Enabling action 0 if possible.")
                     if NEW_TOTAL_FLAT_ACTIONS > 0: flat_mask[0] = True # Absolute fallback

            if DETAILED_ACTION_DEBUG: self.logger.debug(f"{mask_debug_prefix} Final mask sum: {_global_np.sum(flat_mask)}")
            return flat_mask

        except Exception as mask_e:
            self.logger.critical(f"[FATAL EXCEPTION action_masks Env {self.env_id}] Error: {mask_e}")
            self.logger.exception("Full traceback for action_masks failure:")

            # Critical error, return a mask with only PASS enabled if possible
            fallback_mask = _global_np.zeros(NEW_TOTAL_FLAT_ACTIONS, dtype=bool)
            try:
                pass_idx_fatal = self.get_new_flat_index(AT_SYSTEM_ACTIONS, 0, 0, SYSTEM_ACTION_PASS)
                if 0 <= pass_idx_fatal < NEW_TOTAL_FLAT_ACTIONS: fallback_mask[pass_idx_fatal] = True
                else: fallback_mask[0] = True # Fallback to first action if pass_idx is bad
            except Exception: # If even get_new_flat_index fails
                if NEW_TOTAL_FLAT_ACTIONS > 0: fallback_mask[0] = True
            return fallback_mask

    def _apply_action(self, player: Player, flat_action_index: int) -> float:
        debug_prefix = f"[APPLY P{player.player_id} FlIdx:{flat_action_index}]:"; action_reward = 0.0; original_phase = player.current_phase

        # Use the newly defined helper method for decoding
        action_type_idx, entity_idx, position_idx, item_idx = self.decode_new_flat_action(flat_action_index)

        decoded_action_str_new = f"AT={action_type_idx},Ent={entity_idx},Pos={position_idx},Item={item_idx}"
        if DETAILED_ACTION_DEBUG:
            self.logger.debug(f"{debug_prefix} Decoded New: Type={action_type_idx}, Entity={entity_idx}, Pos={position_idx}, Item={item_idx} Phase='{original_phase}' SpecialShopActive={player.special_shop_active} SpecialShopType='{player.special_shop_type}'")

        try:
            merge_check_needed = False

            # This check should be early to prevent invalid actions when special shop is active.
            # Allow SYSTEM_ACTION_PASS (item_idx 0) if special shop is active (to skip it)
            if player.special_shop_active and \
               action_type_idx not in [AT_RANGER_EQUIP, AT_SYSTEM_ACTIONS] and \
               not (action_type_idx == AT_SYSTEM_ACTIONS and item_idx == SYSTEM_ACTION_PASS) :
                if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Action type {action_type_idx} ('{decoded_action_str_new}') is invalid while special shop is active. Only AT_RANGER_EQUIP ({AT_RANGER_EQUIP}) or AT_SYSTEM_ACTIONS ({AT_SYSTEM_ACTIONS}) with PASS are allowed. Penalty.")
                return INVALID_ACTION_PENALTY

            if action_type_idx == AT_SYSTEM_ACTIONS:
                # item_idx defines the specific system action
                if item_idx == SYSTEM_ACTION_PASS: # 0 = Pass / Augment Skip / Special Shop Skip
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} System Action: Pass/Skip")
                    if player.current_phase == "augment" and player.is_augment_choice_pending:
                        action_reward += SKIP_AUGMENT_REWARD
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Augment Skipped. Reward: {action_reward:.3f}")
                        player.is_augment_choice_pending = False; player.available_augment_choices = [None] * NUM_AUGMENT_CHOICES; player.augment_offer_round = -1
                        player.current_phase = "planning" # Transition back to planning after augment choice/skip
                    elif player.special_shop_active:
                         if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Special Shop Skipped.")
                         player.special_shop_active = False
                         player.special_shop_type = None
                         player.special_shop_items = [None] * 5
                         action_reward += 0.0 # No specific reward/penalty for skipping special shop
                    else: # General pass in planning or other phases
                        action_reward += 0.0

                elif item_idx == SYSTEM_ACTION_REROLL: # 1 = Reroll Main Shop (using standardized name)
                    if player.current_phase != "planning" or player.special_shop_active:
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Invalid Reroll. Phase: {player.current_phase}, SpecialShop: {player.special_shop_active}. Penalty.")
                        return INVALID_ACTION_PENALTY
                    if player.coins < REROLL_COST:
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Not enough coins for Reroll. Have: {player.coins}, Need: {REROLL_COST}. Penalty.")
                        return INVALID_ACTION_PENALTY
                    player.coins -= REROLL_COST
                    self._create_player_shops(player)
                    action_reward += REROLL_PENALTY
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} System Action: Rerolled shop. Coins: {player.coins}. Reward: {action_reward:.3f}")

                elif item_idx == SYSTEM_ACTION_BUY_XP: # 2 = Buy XP
                    if player.current_phase != "planning" or player.special_shop_active:
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Invalid Buy XP. Phase: {player.current_phase}, SpecialShop: {player.special_shop_active}. Penalty.")
                        return INVALID_ACTION_PENALTY
                    if player.coins < BUY_XP_COST:
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Not enough coins for XP. Have: {player.coins}, Need: {BUY_XP_COST}. Penalty.")
                        return INVALID_ACTION_PENALTY
                    player.coins -= BUY_XP_COST
                    player.xp += BUY_XP_AMOUNT
                    self._check_level_up(player)
                    action_reward += BUY_XP_REWARD
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} System Action: Bought XP. Coins: {player.coins}, XP: {player.xp}. Reward: {action_reward:.3f}")

                elif item_idx == SYSTEM_ACTION_EXIT_PLACEMENT: # 3 = Exit Placement Phase (using standardized name)
                    if player.current_phase != "placement":
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Invalid Exit Placement. Phase: {player.current_phase}. Penalty.")
                        return INVALID_ACTION_PENALTY
                    player.current_phase = "planning"
                    action_reward += 0.0
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} System Action: Exited placement phase. Now in planning.")

                elif item_idx == SYSTEM_ACTION_END_PLANNING: # 4 = End Planning Phase (Proceed to Combat) (using standardized name)
                    if player.current_phase not in ["planning", "augment"]: # Can end from planning or augment (implies augment choice done/skipped)
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Invalid End Planning. Phase: {player.current_phase}. Penalty.")
                        return INVALID_ACTION_PENALTY
                    if player.special_shop_active: # Cannot end planning if special shop is active
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Invalid End Planning. Special shop is active. Penalty.")
                        return INVALID_ACTION_PENALTY
                    if player.is_augment_choice_pending: # Implicitly skip augment if ending planning phase while choice is pending
                        action_reward += SKIP_AUGMENT_REWARD # Same reward as explicit skip
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Augment implicitly skipped by End Planning Phase. Reward: {action_reward:.3f}")
                        player.is_augment_choice_pending = False; player.available_augment_choices = [None] * NUM_AUGMENT_CHOICES; player.augment_offer_round = -1
                    player.current_phase = "combat_preparation" # Signal to main loop to proceed
                    action_reward += 0.0
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} System Action: End Planning Phase. Now in combat_preparation.")
                else: # Invalid item_idx for system actions
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Unknown system action item_idx: {item_idx}. Penalty.")
                    return INVALID_ACTION_PENALTY

            elif action_type_idx == AT_SELL_UNIT:
                if player.current_phase != "planning" or player.special_shop_active:
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Sell Unit: Invalid phase ({player.current_phase}) or special shop active ({player.special_shop_active}). Penalty.")
                    return INVALID_ACTION_PENALTY

                combined_units = player.get_combined_units()
                if not (0 <= entity_idx < len(combined_units)):
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Sell Unit: entity_idx {entity_idx} out of bounds for {len(combined_units)} units. Penalty.")
                    return INVALID_ACTION_PENALTY

                target_unit_data = combined_units[entity_idx] # entity_idx is already 0-based for combined list

                if not target_unit_data: # Should not happen if entity_idx is in bounds
                    self.logger.error(f"{debug_prefix} Sell Unit: Unit not found at combined_idx {entity_idx} despite bounds check. THIS IS A BUG.")
                    return INVALID_ACTION_PENALTY

                unit_id_to_sell = target_unit_data.get("Instance", {}).get("ID")
                if unit_id_to_sell == player.ranger_unit_id:
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Sell Unit: Attempted to sell Ranger. Invalid.")
                    return INVALID_ACTION_PENALTY

                # Check if selling the last non-Ranger unit. Ranger itself cannot be sold this way.
                non_ranger_units_count = sum(1 for u in combined_units if u.get("Instance", {}).get("ID") != player.ranger_unit_id)
                is_selling_last_non_ranger_unit = (non_ranger_units_count == 1 and unit_id_to_sell != player.ranger_unit_id)

                refund = target_unit_data.get("Instance", {}).get("Cost", 1)
                remove_success = self._remove_unit_by_combined_index(player, entity_idx) # Use 0-based entity_idx

                if remove_success:
                    player.coins += refund
                    action_reward += SELL_PENALTY
                    if is_selling_last_non_ranger_unit : action_reward += SELL_LAST_UNIT_PENALTY
                    merge_check_needed = True
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Sold unit {entity_idx} (ID: {unit_id_to_sell}), got {refund} coins. New coins: {player.coins}. Reward: {action_reward:.3f}")
                else:
                    # This case should ideally not be reached if entity_idx is valid and unit is not Ranger.
                    action_reward += INVALID_ACTION_PENALTY
                    self.logger.warning(f"{debug_prefix} Sell unit {entity_idx} (ID: {unit_id_to_sell}) failed to remove from lists. This might indicate an issue.")

            elif action_type_idx == AT_BUY_ILLUVIAL:
                if player.current_phase != "planning" or player.special_shop_active:
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Buy Illuvial: Invalid phase ({player.current_phase}) or special shop active ({player.special_shop_active}). Penalty.")
                    return INVALID_ACTION_PENALTY

                shop_slot_idx = item_idx # item_idx (0-DIM_ITEM_SIZE-1) is used for shop slot index
                if not (0 <= shop_slot_idx < SHOP_SIZE):
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Buy Illuvial: shop_slot_idx {shop_slot_idx} out of bounds for shop size {SHOP_SIZE}. Penalty.")
                    return INVALID_ACTION_PENALTY

                ill_template = player.illuvial_shop[shop_slot_idx]
                if ill_template is None:
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Buy Illuvial: No item in shop slot {shop_slot_idx}. Penalty.")
                    return INVALID_ACTION_PENALTY
                
                cost = get_illuvial_cost(ill_template)
                if player.coins < cost:
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Buy Illuvial: Not enough coins ({player.coins}) for item cost ({cost}). Penalty.")
                    return INVALID_ACTION_PENALTY
                if len(player.bench) >= BENCH_MAX_SIZE:
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Buy Illuvial: Bench full ({len(player.bench)}). Penalty.")
                    return INVALID_ACTION_PENALTY
                
                player.coins -= cost
                line_t = clean_line_type(ill_template.get("LineType", "ErrorUnit"))
                instance_dict = {"ID": generate_random_unit_id(), "Cost": cost, "EquippedAugments": []}
                if "DominantCombatClass" in ill_template: instance_dict["DominantCombatClass"] = ill_template["DominantCombatClass"]
                if "DominantCombatAffinity" in ill_template: instance_dict["DominantCombatAffinity"] = ill_template["DominantCombatAffinity"]

                type_id_dict = {"UnitType": "Illuvial", "LineType": line_t, "Stage": ill_template.get("Stage", 1), "Path": ill_template.get("Path", "Default"), "Variation": ill_template.get("Variation", "Original")}
                new_unit = {"TypeID": type_id_dict, "Instance": instance_dict, "Position": None}

                player.bench.append(new_unit)
                player.illuvial_shop[shop_slot_idx] = None
                action_reward += BUY_REWARD
                merge_check_needed = True
                if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Bought Illuvial {line_t} (cost {cost}). New coins: {player.coins}, Bench: {len(player.bench)}. Reward: {action_reward:.3f}")

            elif action_type_idx == AT_PLACE_UNIT: # 3
                if player.current_phase != "placement":
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Place Unit: Not in placement phase. Penalty.")
                    return INVALID_ACTION_PENALTY
                if player.special_shop_active:
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Place Unit: Cannot place unit while special shop is active. Penalty.")
                    return INVALID_ACTION_PENALTY

                target_unit_combined_idx_0based = entity_idx

                # position_idx: 0 for bench, 1-11 for board slots (maps to UNIFIED_ID_TO_BLUE_POS_MAP[1] to [11])
                target_pos_is_bench = (position_idx == 0)
                # Ensure target_board_slot_id_1based is only calculated if not moving to bench & position_idx is valid for board
                target_board_slot_id_1based = -1
                if not target_pos_is_bench:
                    if 1 <= position_idx < DIM_POSITION_SIZE: # Max board slots for this action are DIM_POSITION_SIZE -1
                         target_board_slot_id_1based = position_idx # 1-based index for UNIFIED_ID_TO_BLUE_POS_MAP
                    else:
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Place Unit: Invalid board position_idx {position_idx}. Penalty.")
                        return INVALID_ACTION_PENALTY

                current_combined_units = player.get_combined_units()
                if not (0 <= target_unit_combined_idx_0based < len(current_combined_units)):
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Place Unit: target_unit_combined_idx_0based {target_unit_combined_idx_0based} out of bounds. Penalty.")
                    return INVALID_ACTION_PENALTY

                target_unit_to_move, unit_original_location, original_list_idx = self._get_unit_by_combined_index(player, target_unit_combined_idx_0based + 1)

                if not target_unit_to_move:
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Place Unit: Unit to move not found at index {target_unit_combined_idx_0based}. Penalty.")
                    return INVALID_ACTION_PENALTY

                unit_id_to_move = target_unit_to_move.get("Instance", {}).get("ID", "UnknownID")

                if target_pos_is_bench: # Moving to Bench
                    if unit_id_to_move == player.ranger_unit_id:
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Place Unit: Ranger cannot be moved to bench. Invalid.")
                        return INVALID_ACTION_PENALTY
                    if unit_original_location == "bench": # Already on bench
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Place Unit: Unit {unit_id_to_move} already on bench. No-op/Invalid.")
                        return 0.0 # Or INVALID_ACTION_PENALTY if considered invalid
                    if len(player.bench) >= BENCH_MAX_SIZE:
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Place Unit: Bench full. Cannot move {unit_id_to_move} to bench. Penalty.")
                        return INVALID_ACTION_PENALTY

                    # Move from board to bench
                    moved_unit = player.board.pop(original_list_idx)
                    moved_unit["Position"] = None
                    player.bench.append(moved_unit)
                    action_reward += BENCH_TO_BOARD_REWARD # Using this constant, though it's board->bench
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Moved unit {unit_id_to_move} from board to bench. Reward: {action_reward:.3f}")

                else: # Moving to a Board Slot (target_board_slot_id_1based is valid: 1 to DIM_POSITION_SIZE-1)
                    if not (1 <= target_board_slot_id_1based <= MAX_BOARD_SLOTS and target_board_slot_id_1based < DIM_POSITION_SIZE): # Double check mapping
                        self.logger.error(f"{debug_prefix} Place Unit: target_board_slot_id_1based {target_board_slot_id_1based} invalid after mapping from position_idx {position_idx}. THIS IS A BUG.")
                        return INVALID_ACTION_PENALTY

                    target_blue_coords = UNIFIED_ID_TO_BLUE_POS_MAP.get(target_board_slot_id_1based)
                    if not target_blue_coords:
                        self.logger.error(f"{debug_prefix} Place Unit: No blue coords for target_board_slot_id_1based {target_board_slot_id_1based}. THIS IS A BUG.")
                        return INVALID_ACTION_PENALTY
                    q_target, r_target = target_blue_coords

                    unit_at_target_coords = player.get_unit_at_pos(q_target, r_target)

                    if unit_at_target_coords and unit_at_target_coords.get("Instance", {}).get("ID") == unit_id_to_move: # Moving to its own spot
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Place Unit: Unit {unit_id_to_move} moved to its own spot. No-op.")
                        return 0.0

                    current_board_count = player.units_on_board_count()
                    can_place_new = current_board_count < player.level

                    if unit_original_location == "bench": # Moving from bench to board
                        if not unit_at_target_coords and not can_place_new:
                             if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Place Unit: Cannot place new unit from bench to board. Board full (Lvl {player.level}, OnBoard {current_board_count}). Target empty. Penalty.")
                             return INVALID_ACTION_PENALTY

                        moved_unit_from_bench = player.bench.pop(original_list_idx)
                        moved_unit_from_bench["Position"] = {"Q": q_target, "R": r_target}

                        if unit_at_target_coords: # Swap bench <-> board
                            unit_at_target_coords["Position"] = None
                            player.bench.append(unit_at_target_coords)
                            player.board.remove(unit_at_target_coords)
                            if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Swapped bench unit {unit_id_to_move} with board unit {unit_at_target_coords.get('Instance',{}).get('ID')}. Target slot: {target_board_slot_id_1based}")

                        player.board.append(moved_unit_from_bench)
                        action_reward += BENCH_TO_BOARD_REWARD
                        if moved_unit_from_bench.get("TypeID",{}).get("Stage",1) > 1 : action_reward += PLACE_EVOLVED_REWARD
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Moved unit {unit_id_to_move} from bench to board slot {target_board_slot_id_1based}. Reward: {action_reward:.3f}")

                    elif unit_original_location == "board": # Moving from board to board (swap or simple move)
                        target_unit_to_move["Position"] = {"Q": q_target, "R": r_target} # Update position of the unit being moved
                        if unit_at_target_coords: # Swap board <-> board
                            original_pos_of_moved_unit_coords_tuple = None
                            # Need to find the original coords of the unit being moved if it was on board
                            # This is a bit tricky as original_list_idx is for the *current* board state before this move
                            # We need its previous position dict
                            # This part of logic for board-to-board swap needs careful review of how original_list_idx relates to coords
                            # For now, assuming player.board[original_list_idx] before modification was the unit_at_target_coords's destination
                            # This is likely incorrect. A better way: store unit_to_move's original coords before changing its pos.
                            # However, the current _get_unit_by_combined_index doesn't return original coords directly.
                            # A simpler approach for board-to-board swap:
                            # 1. unit_to_move (already identified) gets target_blue_coords
                            # 2. unit_at_target_coords (if exists) gets unit_to_move's *original* board position.
                            # This implies we need to fetch unit_to_move's original position *before* this block.
                            # Let's assume for now `target_unit_to_move` still holds its original position if it was on board.
                            original_q_r_of_moving_unit = (target_unit_to_move.get("Position",{}).get("Q"), target_unit_to_move.get("Position",{}).get("R"))

                            if unit_at_target_coords.get("Position"): # Should always have if on board
                                unit_at_target_coords["Position"] = {"Q": original_q_r_of_moving_unit[0], "R": original_q_r_of_moving_unit[1]}
                            else: # Should not happen
                                self.logger.error(f"{debug_prefix} Swapped unit {unit_at_target_coords.get('Instance',{}).get('ID')} has no position. BUG!")


                            if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Swapped board unit {unit_id_to_move} with board unit {unit_at_target_coords.get('Instance',{}).get('ID')}. Target slot: {target_board_slot_id_1based}, Moved unit's original slot was implicitly handled by swapping.")
                        else: # Simple move within board to empty slot
                            if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Moved board unit {unit_id_to_move} to empty board slot {target_board_slot_id_1based}.")
                        action_reward += MODIFY_UNIT_REWARD

                    else: # Should not happen
                        self.logger.error(f"{debug_prefix} Place Unit: Unit {unit_id_to_move} has unknown original location '{unit_original_location}'. BUG.")
                        return INVALID_ACTION_PENALTY
                merge_check_needed = True # Merges can happen after units return to bench

            elif action_type_idx == AT_RANGER_EQUIP: # 4
                if not player.special_shop_active:
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Ranger Equip: Special shop not active.")
                    return INVALID_ACTION_PENALTY
                if player.current_phase != "planning":
                     if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Ranger Equip: Not in planning phase.")
                     return INVALID_ACTION_PENALTY

                shop_slot_idx = item_idx
                if not (0 <= shop_slot_idx < 5):
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Ranger Equip: shop_slot_idx {shop_slot_idx} out of bounds for 5-slot special shop.")
                    return INVALID_ACTION_PENALTY

                selected_item_definition = player.special_shop_items[shop_slot_idx]
                if selected_item_definition is None:
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Ranger Equip: No item in special shop slot {shop_slot_idx}.")
                    return INVALID_ACTION_PENALTY

                ranger_unit_on_board = next((u for u in player.board if u.get("Instance", {}).get("ID") == player.ranger_unit_id), None)
                if not ranger_unit_on_board:
                    self.logger.error(f"{debug_prefix} Ranger Equip: Ranger unit not found on board for P{player.player_id}.")
                    return INVALID_ACTION_PENALTY

                item_name_for_log = selected_item_definition.get('Name', 'UnknownItem')

                if player.special_shop_type == "weapon":
                    new_weapon_instance_id = generate_random_unit_id()
                    # Player's direct attribute IS the instance
                    player.ranger_weapon["ID"] = new_weapon_instance_id
                    player.ranger_weapon["TypeID"] = selected_item_definition # Store the full definition as TypeID for weapons
                    player.ranger_weapon["EquippedAmplifiers"].clear() # Clear old amplifiers
                    # player.ranger_weapon_amplifiers list is already pointing to player.ranger_weapon["EquippedAmplifiers"]

                    # Update the ranger unit on the board. player.ranger_weapon IS the dict in ranger_unit_on_board
                    # No need to reassign ranger_unit_on_board["Instance"]["EquippedWeapon"] if player.ranger_weapon points to it.
                    # Verify this pointer relationship holds. If not, explicit update needed.
                    # Based on _initialize_player_ranger, player.ranger_weapon points to the dict within the Ranger's board instance.
                    action_reward += APPLY_AUGMENT_REWARD
                    if DETAILED_ACTION_DEBUG:
                        weapon_id_log = player.ranger_weapon.get("TypeID", {}).get("ID", "N/A_Def") # Assuming TypeID has an ID field, or use Name
                        weapon_instance_id_log = player.ranger_weapon.get("ID", "N/A_Inst")
                        self.logger.debug(f"{debug_prefix} Ranger equipped new weapon: {item_name_for_log} (DefID: {weapon_id_log}, InstID: {weapon_instance_id_log}). Previous amplifiers cleared.")
                        self.logger.debug(f"{debug_prefix} Player ranger_weapon state: Name='{player.ranger_weapon.get('TypeID',{}).get('Name')}', Instance_ID='{player.ranger_weapon.get('ID')}'")
                        self.logger.debug(f"{debug_prefix} Player ranger_weapon_amplifiers (should be empty): {player.ranger_weapon_amplifiers}")


                elif player.special_shop_type == "weapon_amplifier":
                    if not player.ranger_weapon or not player.ranger_weapon.get("TypeID",{}).get("Name"):
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Ranger Equip: Tried to equip amplifier but no weapon is equipped. Penalty.")
                        return INVALID_ACTION_PENALTY

                    if len(player.ranger_weapon_amplifiers) < 2:
                        new_amplifier_instance = {
                            "ID": generate_random_unit_id(),
                            "TypeID": selected_item_definition # Store the full definition as TypeID
                        }
                        player.ranger_weapon_amplifiers.append(new_amplifier_instance)
                        action_reward += APPLY_AUGMENT_REWARD
                        if DETAILED_ACTION_DEBUG:
                            amp_def_id_log = new_amplifier_instance.get("TypeID", {}).get("ID", "N/A_Def") # Assuming TypeID has an ID field or use Name
                            self.logger.debug(f"{debug_prefix} Ranger equipped weapon amplifier: {item_name_for_log} (DefID: {amp_def_id_log}, InstID: {new_amplifier_instance.get('ID')})")
                            current_amps_log = [(amp.get('TypeID',{}).get('Name'), amp.get('ID')) for amp in player.ranger_weapon_amplifiers]
                            self.logger.debug(f"{debug_prefix} Player ranger_weapon_amplifiers: {current_amps_log}")
                    else:
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Ranger Equip: Already has 2 weapon amplifiers. Penalty.")
                        return INVALID_ACTION_PENALTY
                else:
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Ranger Equip: Unknown special_shop_type '{player.special_shop_type}'.")
                    return INVALID_ACTION_PENALTY

                player.special_shop_active = False
                player.special_shop_type = None
                player.special_shop_items = [None] * 5

            elif action_type_idx == AT_RANGER_BOND: # 5
                if player.current_phase != "planning" or player.special_shop_active:
                     if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Ranger Bond: Invalid phase ({player.current_phase}) or special shop active ({player.special_shop_active}). Penalty.")
                     return INVALID_ACTION_PENALTY

                ranger_unit_on_board = next((u for u in player.board if u.get("Instance", {}).get("ID") == player.ranger_unit_id), None)
                if not ranger_unit_on_board:
                    self.logger.error(f"{debug_prefix} Ranger Bond: Ranger unit not found on board for P{player.player_id}. THIS IS A BUG.")
                    return INVALID_ACTION_PENALTY

                target_illuvial_combined_idx_0based = entity_idx
                current_player_combined_units = player.get_combined_units()
                if not (0 <= target_illuvial_combined_idx_0based < len(current_player_combined_units)) :
                     if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Bond: entity_idx {target_illuvial_combined_idx_0based} out of bounds for combined units len {len(current_player_combined_units)}. Penalty.")
                     return INVALID_ACTION_PENALTY

                target_illuvial_data = current_player_combined_units[target_illuvial_combined_idx_0based]

                if not target_illuvial_data: # Should be caught by the index check above
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Bond: Target Illuvial at combined_idx {target_illuvial_combined_idx_0based} not found. Penalty.")
                    return INVALID_ACTION_PENALTY
                if target_illuvial_data.get("TypeID", {}).get("UnitType") != "Illuvial":
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Bond: Target unit {target_illuvial_data.get('TypeID', {}).get('LineType')} is not an Illuvial. Penalty.")
                    return INVALID_ACTION_PENALTY

                target_illuvial_id = target_illuvial_data.get("Instance", {}).get("ID")
                if target_illuvial_id == player.ranger_unit_id: # Ranger cannot bond with itself
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Bond: Ranger cannot bond with itself. Penalty.")
                    return INVALID_ACTION_PENALTY

                ranger_unit_on_board["Instance"]["BondedId"] = target_illuvial_id
                action_reward += BOND_REWARD
                self._cumulative_rewards_this_episode["bond"] += action_reward # Assuming this was meant for action_reward, not BOND_REWARD
                if DETAILED_ACTION_DEBUG:
                    self.logger.debug(f"{debug_prefix} Ranger (ID: {player.ranger_unit_id}) attempting to bond with Illuvial (ID: {target_illuvial_id}). Reward: {action_reward:.3f}")
                    self.logger.debug(f"{debug_prefix} Ranger's board instance BondedId after bond: {ranger_unit_on_board['Instance']['BondedId']}")

            else: # This else corresponds to action_type_idx not in [0,1,2,3,4,5]
                if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Invalid action_type_idx: {action_type_idx}. Penalty.") # Log for unknown action type
                return INVALID_ACTION_PENALTY

            if merge_check_needed:
                merge_reward = self._check_and_merge_units(player)
                action_reward += merge_reward
                if merge_reward > 0 and DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Merge check yielded +{merge_reward:.3f} reward.")

            # Check if augment phase should be triggered
            if player.current_phase == "planning" and not player.is_augment_choice_pending and (self.round_idx + 1) % AUGMENT_OFFER_FREQUENCY == 0 and self.round_idx > 0 : # Check round_idx + 1 for current round offers
                # Make sure we are not in special shop mode
                if not player.special_shop_active:
                    self._offer_augments(player)
                    if player.is_augment_choice_pending: # Check if offer was successful
                        player.current_phase = "augment" # Transition to augment phase
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Transitioned to augment phase after action.")
                else:
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Augment offer skipped due to active special shop.")


            # If phase was changed by action (e.g., exit placement, end planning), it's already set.
            # If an augment was chosen/skipped, phase is set back to planning.
            # If an augment was offered, phase is set to augment.
            # Otherwise, phase remains as it was.
            if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Action processed. Final phase: {player.current_phase}. Total reward for this action: {action_reward:.3f}")
            return action_reward

        except Exception as e_apply:
            self.logger.critical(f"[FATAL EXCEPTION _apply_action Env {self.env_id} P{player.player_id}] Action: {decoded_action_str_new}, Error: {e_apply}")
            self.logger.exception("Full traceback for _apply_action failure:")
            return INVALID_ACTION_PENALTY # Penalize for any error during action application

    def _get_obs(self) -> _global_np.ndarray: # Using _global_np
        obs_debug_prefix = f"[Env {self.env_id} Rnd {self.round_idx} _get_obs]";
        obs_list = []

        try: # --- ADD THIS TRY BLOCK ---
            # 1. Board features for ALL players
            for i in range(NUM_PLAYERS):
                player = self.players.get(i)
                if player:
                    for slot_idx in range(MAX_BOARD_SLOTS): # 19 slots
                        # Determine actual (q,r) for this slot_idx (e.g. from POSITIONS_BLUE_LIST)
                        q, r = POSITIONS_BLUE_LIST[slot_idx] if slot_idx < len(POSITIONS_BLUE_LIST) else (None, None)
                        unit = player.get_unit_at_pos(q,r) if q is not None else None

                        if unit:
                            type_id_data = unit.get("TypeID", {})
                            instance_data = unit.get("Instance", {})
                            obs_list.extend([
                                float(self._get_illuvial_id(type_id_data.get("LineType"))),
                                float(type_id_data.get("Stage", 0)),
                                float(instance_data.get("Level", 0)), # Level of the unit instance
                                float(len(instance_data.get("EquippedAugments", []))),
                                float(instance_data.get("Cost", 0)) # Cost
                            ])
                        else:
                            obs_list.extend([0.0] * FEATURES_PER_UNIT)
                else: # Player does not exist (should not happen with current setup)
                    obs_list.extend([0.0] * BOARD_FEATURES_PER_PLAYER)
            
            # 2. Agent-specific features (self.agent is player 0)
            agent = self.agent
            if agent:
                obs_list.extend([float(agent.health), float(agent.coins), float(agent.level), float(agent.xp), float(self.round_idx)])
                
                # Agent Bench
                for i in range(BENCH_MAX_SIZE): # 15 slots
                    if i < len(agent.bench):
                        unit = agent.bench[i]
                        type_id_data = unit.get("TypeID", {})
                        instance_data = unit.get("Instance", {})
                        obs_list.extend([
                            float(self._get_illuvial_id(type_id_data.get("LineType"))),
                            float(type_id_data.get("Stage", 0)),
                            float(instance_data.get("Level", 0)),
                            float(len(instance_data.get("EquippedAugments", []))),
                            float(instance_data.get("Cost", 0))
                        ])
                    else:
                        obs_list.extend([0.0] * FEATURES_PER_UNIT)
                
                # Agent Shop
                for i in range(SHOP_SIZE): # 7 slots
                    ill = agent.illuvial_shop[i]
                    if ill:
                        obs_list.extend([float(self._get_illuvial_id(ill.get("LineType"))), float(get_illuvial_cost(ill))])
                    else:
                        obs_list.extend([0.0, 0.0])
                
                # Agent Augments (Illuvial Augments)
                obs_list.append(1.0 if agent.is_augment_choice_pending else 0.0)
                for i in range(NUM_AUGMENT_CHOICES): # 3 choices
                    aug_name = agent.available_augment_choices[i]
                    obs_list.append(float(self._get_augment_id(aug_name if aug_name else "")))

                # Ranger Equipment Features
                ranger_equipment_state = _global_np.zeros(RANGER_EQUIPMENT_FEATURES, dtype=_global_np.float32)
                if agent.ranger_weapon and agent.ranger_weapon.get('TypeID', {}).get('Name'):
                    weapon_name = agent.ranger_weapon['TypeID']['Name']
                    ranger_equipment_state[0] = float(self.weapon_name_to_id.get(weapon_name, 0))
                    
                    for i, amp_instance in enumerate(agent.ranger_weapon.get('EquippedAmplifiers', [])):
                        if i < 2: # Max 2 amplifiers, state indices 1 and 2
                            amp_name = amp_instance.get('TypeID', {}).get('Name')
                            if amp_name: # Ensure amp_name is not None
                                ranger_equipment_state[i + 1] = float(self.weapon_amplifier_name_to_id.get(amp_name, 0))
                obs_list.extend(ranger_equipment_state.tolist())

                # Special Shop Item Features
                special_shop_state = _global_np.zeros(SPECIAL_SHOP_ITEM_FEATURES, dtype=_global_np.float32)
                if agent.special_shop_active:
                    for idx, item_data_def in enumerate(agent.special_shop_items):
                        if idx < SPECIAL_SHOP_ITEM_FEATURES and item_data_def: # Ensure within bounds
                            item_name = item_data_def.get('Name')
                            item_id_to_encode = 0
                            if item_name: # Ensure item_name is not None
                                if agent.special_shop_type == "weapon":
                                    item_id_to_encode = self.weapon_name_to_id.get(item_name, 0)
                                elif agent.special_shop_type == "weapon_amplifier":
                                    item_id_to_encode = self.weapon_amplifier_name_to_id.get(item_name, 0)
                            special_shop_state[idx] = float(item_id_to_encode)
                obs_list.extend(special_shop_state.tolist())

            else: # Agent is None (should not happen)
                # This needs to extend by the *new* AGENT_SPECIFIC_FEATURES size
                obs_list.extend([0.0] * AGENT_SPECIFIC_FEATURES)


            # 3. Other opponents' scalar and bench features
            for i in range(NUM_PLAYERS):
                if i == 0: continue # Skip agent (player 0)
                opponent = self.players.get(i)
                if opponent:
                    obs_list.extend([float(opponent.health), float(opponent.coins), float(opponent.level), float(opponent.xp)])
                    for bench_idx in range(BENCH_MAX_SIZE): # 15 slots
                        if bench_idx < len(opponent.bench):
                            unit = opponent.bench[bench_idx]
                            type_id_data = unit.get("TypeID", {})
                            instance_data = unit.get("Instance", {})
                            obs_list.extend([
                                float(self._get_illuvial_id(type_id_data.get("LineType"))),
                                float(type_id_data.get("Stage", 0)),
                                float(instance_data.get("Level", 0)),
                                float(len(instance_data.get("EquippedAugments", []))),
                                float(instance_data.get("Cost", 0))
                            ])
                        else:
                            obs_list.extend([0.0] * FEATURES_PER_UNIT)
                else: # Opponent does not exist
                    obs_list.extend([0.0] * FEATURES_PER_OPPONENT)
            
            final_obs_array = _global_np.array(obs_list, dtype=_global_np.float32) # Using _global_np
            expected_len = self.observation_space.shape[0]
            if final_obs_array.size != expected_len:
                self.logger.error(f"{obs_debug_prefix} Obs length mismatch! Expected {expected_len}, Got {final_obs_array.size}. Padding/Truncating.")
                # Create an array of zeros with the correct shape
                padded_obs = _global_np.zeros((expected_len,), dtype=_global_np.float32) # Using _global_np
                # Copy data from final_obs_array to padded_obs, truncating if necessary
                copy_len = min(final_obs_array.size, expected_len)
                padded_obs[:copy_len] = final_obs_array[:copy_len]
                final_obs_array = padded_obs

            if not _global_np.all(_global_np.isfinite(final_obs_array)): # Using _global_np
                self.logger.warning(f"{obs_debug_prefix} Non-finite values found in observation. Clamping to zero.")
                final_obs_array = _global_np.nan_to_num(final_obs_array, nan=0.0, posinf=0.0, neginf=0.0) # Using _global_np

            return final_obs_array
        except Exception as e_obs_outer: # --- CATCH ANY EXCEPTION HERE ---
            self.logger.critical(f"[FATAL Env {self.env_id} _get_obs] CRITICAL ERROR: {e_obs_outer}")
            self.logger.exception("Full traceback for _get_obs failure:")
            # Return a zero observation of the correct shape and type
            return _global_np.zeros(self.observation_space.shape, dtype=self.observation_space.dtype) # Using _global_np

    def _get_info(self) -> Dict[str, Any]:
        info_debug_prefix = f"[Env {self.env_id} Rnd {self.round_idx} _get_info]"
        try: # --- ADD THIS TRY BLOCK ---
            agent_player = self.agent
            if agent_player is None:
                self.logger.error(f"{info_debug_prefix} Agent is None. Returning minimal info.")
                return {"env_id": self.env_id, "round": self.round_idx, "error": "agent_is_none"}

            # Agent specific detailed info
            agent_info: Dict[str, Any] = {
                "player_id": agent_player.player_id, "health": agent_player.health, "coins": agent_player.coins,
                "xp": agent_player.xp, "level": agent_player.level, "current_phase": agent_player.current_phase,
                "board_units": len(agent_player.board), "bench_units": len(agent_player.bench),
                "shop_items": [item.get("LineType") if item else None for item in agent_player.illuvial_shop],
                "is_augment_pending": agent_player.is_augment_choice_pending,
                "augment_choices": [aug_name if aug_name else None for aug_name in agent_player.available_augment_choices],
                "won_last_round": agent_player.won_last_round,
                "last_opponent_id": agent_player.last_opponent_id,
                "special_shop_active": agent_player.special_shop_active,
                "special_shop_type": agent_player.special_shop_type,
                "special_shop_items": [item.get("Name") if item else None for item in agent_player.special_shop_items],
                "ranger_unit_id": agent_player.ranger_unit_id,
                "ranger_weapon": agent_player.ranger_weapon.get("TypeID",{}).get("Name") if agent_player.ranger_weapon and agent_player.ranger_weapon.get("TypeID") else None,
                "ranger_amps": [amp.get("TypeID",{}).get("Name") for amp in agent_player.ranger_weapon_amplifiers if amp.get("TypeID")] if agent_player.ranger_weapon_amplifiers else [],
                "ranger_suit": agent_player.ranger_suit.get("Name") if agent_player.ranger_suit else None,
            }

            # Opponent summaries
            opponents_info = []
            for i in range(NUM_PLAYERS):
                if i == agent_player.player_id: continue
                p = self.players.get(i)
                if p:
                    opp_board_summary = [(unit.get("TypeID", {}).get("LineType"), unit.get("TypeID", {}).get("Stage")) for unit in p.board]
                    opponents_info.append({
                        "player_id": p.player_id, "health": p.health, "level": p.level,
                        "board_size": len(p.board), "bench_size": len(p.bench),
                        "board_summary": opp_board_summary, # List of (LineType, Stage) for board units
                        "opponent_source_type": p.opponent_source_type,
                        "opponent_identifier": p.opponent_identifier
                    })

            # Game state info
            game_info = {
                "env_id": self.env_id, "round_idx": self.round_idx,
                "alive_players": [p_id for p_id, p in self.players.items() if p.is_alive()],
                "episode_finished": self.episode_finished,
                "cumulative_rewards": dict(self._cumulative_rewards_this_episode) # Copy of the defaultdict
            }

            # Combine all info
            full_info = {"agent": agent_info, "opponents": opponents_info, "game": game_info}

            # Deep convert for JSON compatibility (handles NumPy types etc.)
            try: return deep_convert_np_types(full_info, context='info_get_info_return')
            except Exception as e_convert:
                self.logger.error(f"{info_debug_prefix} Error during deep_convert_np_types: {e_convert}")
                self.logger.exception("Traceback for info conversion failure:")
                # Fallback: return a simplified version if conversion fails
                return {"env_id": self.env_id, "round": self.round_idx, "error": "info_conversion_failed"}
                
        except Exception as e_info_outer: # --- CATCH ANY EXCEPTION HERE ---
            self.logger.critical(f"[FATAL Env {self.env_id} _get_info] CRITICAL ERROR: {e_info_outer}")
            self.logger.exception("Full traceback for _get_info failure:")
            return {"env_id": self.env_id, "round": self.round_idx, "error": "get_info_exception"}

    def _offer_augments(self, player: Player):
        offer_debug_prefix = f"[Env {self.env_id} P{player.player_id} Rnd {self.round_idx} OfferAugments]"
        if not isinstance(self.merged_data, dict) or "Augments" not in self.merged_data or not isinstance(self.merged_data["Augments"], dict):
            self.logger.error(f"{offer_debug_prefix} Augments data missing or invalid. Cannot offer choices.")
            return

        all_augment_defs = self.merged_data["Augments"]
        available_augment_names = [aug_data.get("Name") for aug_key, aug_data in all_augment_defs.items() if aug_data.get("Name")]

        if not available_augment_names:
            self.logger.warning(f"{offer_debug_prefix} No augment names available from merged_data. Cannot offer choices.")
            return

        if len(available_augment_names) < NUM_AUGMENT_CHOICES:
            self.logger.warning(f"{offer_debug_prefix} Not enough unique augments ({len(available_augment_names)}) to offer {NUM_AUGMENT_CHOICES}. Offering with replacement or fewer choices.")
            choices = random.choices(available_augment_names, k=NUM_AUGMENT_CHOICES) if available_augment_names else [None]*NUM_AUGMENT_CHOICES
        else:
            choices = random.sample(available_augment_names, NUM_AUGMENT_CHOICES)

        player.available_augment_choices = choices
        player.is_augment_choice_pending = True
        player.augment_offer_round = self.round_idx
        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{offer_debug_prefix} Offered augments: {choices}")

    def _resolve_combat(self, p1_id: int, p2_id: int) -> Tuple[float, float, bool, bool, bool]:
        combat_debug_prefix = f"[Env {self.env_id} Rnd {self.round_idx} Combat P{p1_id}vP{p2_id}]"
        self.logger.debug(f"{combat_debug_prefix} --- COMBAT START ---")

        p1, p2 = self.players.get(p1_id), self.players.get(p2_id)
        if not p1 or not p2:
            self.logger.error(f"{combat_debug_prefix} One or both players not found ({p1_id}, {p2_id}). Aborting combat.")
            return 0.0, 0.0, False, False, False # No rewards, no one eliminated

        # Mutual empty board check (Ranger should prevent this for agent, but good for general case)
        if not p1.board and not p2.board:
            self.logger.warning(f"{combat_debug_prefix} Mutual empty board for P{p1_id} and P{p2_id}. Both take damage. No simulation.")
            p1_reward = MUTUAL_EMPTY_BOARD_PENALTY; p2_reward = MUTUAL_EMPTY_BOARD_PENALTY
            p1.health -= 1; p2.health -= 1 # Nominal damage
            p1.won_last_round = False; p2.won_last_round = False
            self._cumulative_rewards_this_episode[f"p{p1_id}_combat"] += p1_reward
            self._cumulative_rewards_this_episode[f"p{p2_id}_combat"] += p2_reward
            return p1_reward, p2_reward, not p1.is_alive(), not p2.is_alive(), True # is_draw=True

        # Handle one-sided empty boards (loss for the empty one, win for the other)
        if not p1.board: # P1 has empty board, P2 wins
            self.logger.info(f"{combat_debug_prefix} P{p1_id} has empty board. P{p2_id} wins by default.")
            p1_reward = ROUND_LOSS_REWARD; p2_reward = ROUND_WIN_REWARD
            p1.health -= 1 # Nominal damage for empty board loss
            p1.won_last_round = False; p2.won_last_round = True
            self._cumulative_rewards_this_episode[f"p{p1_id}_combat"] += p1_reward
            self._cumulative_rewards_this_episode[f"p{p2_id}_combat"] += p2_reward
            return p1_reward, p2_reward, not p1.is_alive(), False, False

        if not p2.board: # P2 has empty board, P1 wins
            self.logger.info(f"{combat_debug_prefix} P{p2_id} has empty board. P{p1_id} wins by default.")
            p1_reward = ROUND_WIN_REWARD; p2_reward = ROUND_LOSS_REWARD
            p2.health -= 1 # Nominal damage for empty board loss
            p1.won_last_round = True; p2.won_last_round = False
            self._cumulative_rewards_this_episode[f"p{p1_id}_combat"] += p1_reward
            self._cumulative_rewards_this_episode[f"p{p2_id}_combat"] += p2_reward
            return p1_reward, p2_reward, False, not p2.is_alive(), False

        # --- Actual Simulation (if both have boards) ---

        def format_team_for_battle_file(player_obj: Player, team_name: str, all_players_dict: Dict[int, Player]) -> Dict[str, Any]:
            formatted_units = []
            for unit_data_on_board in player_obj.board:
                unit_instance_id = unit_data_on_board.get("Instance", {}).get("ID")
                is_ranger = (unit_instance_id == player_obj.ranger_unit_id)
                
                # Base structure, common for Illuvials and Rangers (some fields overridden for Ranger)
                # Ensure deepcopy if modifying unit_data_on_board directly, or build fresh dict
                processed_unit_data = {
                    "TypeID": unit_data_on_board.get("TypeID", {}).copy(), # Start with a copy
                    "Instance": unit_data_on_board.get("Instance", {}).copy(),
                    "Position": unit_data_on_board.get("Position", {}).copy(),
                    "Team": team_name
                }

                if is_ranger:
                    # Ranger Specific TypeID
                    processed_unit_data["TypeID"] = {
                        "UnitType": "Ranger",
                        "LineType": "FemaleRanger", # TODO: Make configurable if Ranger variations exist
                        "Stage": 0,
                        "Path": "",
                        "Variation": ""
                    }
                    # Ranger Specific Instance data
                    ranger_instance_data = processed_unit_data["Instance"] # Get a reference to the instance dict
                    ranger_instance_data["ID"] = player_obj.ranger_unit_id
                    # Affinity/Class for Ranger might come from weapon or suit, or be base stats.
                    # For now, ensure they are present, defaulting to "None" or values from board data.
                    ranger_instance_data["DominantCombatAffinity"] = unit_data_on_board.get("Instance", {}).get("DominantCombatAffinity", "None")
                    ranger_instance_data["DominantCombatClass"] = unit_data_on_board.get("Instance", {}).get("DominantCombatClass", "None")
                    ranger_instance_data["BondedId"] = unit_data_on_board.get("Instance", {}).get("BondedId", "")
                    ranger_instance_data["Level"] = unit_data_on_board.get("Instance", {}).get("Level", 1)

                    # EquippedWeapon
                    if player_obj.ranger_weapon and player_obj.ranger_weapon.get("TypeID", {}).get("Name"):
                        weapon_instance = player_obj.ranger_weapon
                        ranger_instance_data["EquippedWeapon"] = {
                            "ID": weapon_instance.get("ID", generate_random_unit_id()), # Stored instance ID
                            "TypeID": weapon_instance.get("TypeID", {}), # This is the weapon definition
                            "EquippedAmplifiers": [
                                {"ID": amp.get("ID"), "TypeID": amp.get("TypeID")}
                                for amp in weapon_instance.get("EquippedAmplifiers", []) if amp and amp.get("TypeID")
                            ]
                        }
                    else: # Default empty if no weapon
                         ranger_instance_data["EquippedWeapon"] = {
                            "ID": generate_random_unit_id(), "TypeID": {"Name": "", "Stage": 0, "Variation": ""}, "EquippedAmplifiers": []
                        }

                    # EquippedSuit
                    suit_instance_id_from_board = unit_data_on_board.get("Instance", {}).get("EquippedSuit", {}).get("ID")
                    if not suit_instance_id_from_board:
                        self.logger.warning(f"{combat_debug_prefix} P{player_obj.player_id} Ranger on board missing EquippedSuit.ID. Generating new one for battle file.")
                        suit_instance_id_from_board = generate_random_unit_id()
                        # Persist this generated ID back to the board data for consistency if this function is called multiple times
                        # for the same combat (e.g. if a re-simulation feature was added)
                        # However, _initialize_player_ranger should have set this. This is a fallback.
                        if "EquippedSuit" not in ranger_instance_data: ranger_instance_data["EquippedSuit"] = {}
                        ranger_instance_data["EquippedSuit"]["ID"] = suit_instance_id_from_board
                        
                    ranger_instance_data["EquippedSuit"] = {
                        "ID": suit_instance_id_from_board,
                        "TypeID": player_obj.ranger_suit.get("TypeID", {"Name": "Jumpsuit", "Stage": 1, "Variation": "Original"}) if player_obj.ranger_suit else {"Name": "Jumpsuit", "Stage": 1, "Variation": "Original"}
                    }
                    
                    ranger_instance_data["EquippedConsumables"] = []
                    ranger_instance_data["EquippedAugments"] = [] # Ranger uses equipment, not Illuvial augments here
                    ranger_instance_data["Finish"] = unit_data_on_board.get("Instance", {}).get("Finish", "None")
                else: # Illuvial
                    # Ensure Illuvials do not have Ranger-specific equipment fields in the battle file
                    processed_unit_data["Instance"].pop("EquippedWeapon", None)
                    processed_unit_data["Instance"].pop("EquippedSuit", None)
                    # EquippedAugments for Illuvials are already part of unit_data_on_board.Instance

                formatted_units.append(processed_unit_data)

            return {
                "PlayerID": player_obj.player_id,
                "Health": player_obj.health,
                "Mana": player_obj.coins, # Assuming Mana for simulation is player's coins
                "Units": formatted_units
            }

        p1_battle_team_data = format_team_for_battle_file(p1, "Blue", self.players)
        p2_battle_team_data = format_team_for_battle_file(p2, "Red", self.players)

        # Deep convert for simulation (handles NumPy types, Paths, etc.)
        # This should be done *after* constructing the final battle file structure
        p1_battle_team_data_final = deep_convert_np_types(p1_battle_team_data, context='sim_input_p1_battle')
        p2_battle_team_data_final = deep_convert_np_types(p2_battle_team_data, context='sim_input_p2_battle')

        battle_file_content = {
            "Version": 25, # Updated Version
            "TeamBlue": p1_battle_team_data_final,
            "TeamRed": p2_battle_team_data_final,
            "OutputFilename": sim_output_filename # Existing variable
        }

        # Prepare input file for simulation
        sim_input_filename = os.path.join(BATTLE_DIR, f"sim_input_env{self.env_id}_r{self.round_idx}_p{p1_id}v{p2_id}.json")
        sim_output_filename = os.path.join(BATTLE_DIR, f"sim_output_env{self.env_id}_r{self.round_idx}_p{p1_id}v{p2_id}.json") # Already defined

        # Ensure BATTLE_DIR exists (should be done at init, but good to double check)
        os.makedirs(BATTLE_DIR, exist_ok=True)

        with open(sim_input_filename, "w") as f:
            json.dump(battle_file_content, f, indent=2)
        self._generated_files_this_step.append(sim_input_filename) # Track generated file

        # --- Call Simulation (Placeholder for actual subprocess call) ---
        # In a real setup, this would be:
        # subprocess.run(["./simulation_executable", sim_input_filename], check=True)
        # For now, we'll mock the result or read a pre-generated output if it exists.

        # --- MOCK SIMULATION RESULT ---
        # This is where you'd integrate your actual simulation call.
        # For testing, we'll create a dummy output file.
        # If you have a way to run your simulation, replace this mock.

        # Check if a real simulation output exists (e.g., if you ran it manually)
        # If not, create a mock output.
        if not os.path.exists(sim_output_filename):
             self.logger.warning(f"{combat_debug_prefix} Simulation output file {sim_output_filename} not found. Creating MOCK result.")
             # Mock result: P1 wins, P2 takes 5 damage.
             mock_result = {
                 "Winner": p1.player_id, "Loser": p2.player_id,
                 "DamageDealtToLoser": 5, "IsDraw": False,
                 "RemainingUnitsBlue": len(p1.board), "RemainingUnitsRed": 0
             }
             with open(sim_output_filename, "w") as f_out: json.dump(mock_result, f_out, indent=2)
             self._generated_files_this_step.append(sim_output_filename)
        # --- END MOCK SIMULATION ---

        # Read simulation result
        try:
            with open(sim_output_filename, "r") as f: result_data = json.load(f)
        except (FileNotFoundError, json.JSONDecodeError) as e:
            self.logger.error(f"{combat_debug_prefix} Failed to read/decode sim output {sim_output_filename}: {e}. Assuming P1 wins by default.")
            result_data = {"Winner": p1.player_id, "Loser": p2.player_id, "DamageDealtToLoser": 1, "IsDraw": False} # Fallback result

        winner_id = result_data.get("Winner")
        loser_id = result_data.get("Loser")
        damage = result_data.get("DamageDealtToLoser", 1)
        is_draw = result_data.get("IsDraw", False)

        p1_eliminated, p2_eliminated = False, False

        if is_draw:
            self.logger.info(f"{combat_debug_prefix} Combat resulted in a DRAW.")
            p1.health -= damage; p2.health -= damage # Both take damage on a draw
            p1_reward = ROUND_LOSS_REWARD / 2; p2_reward = ROUND_LOSS_REWARD / 2 # Split loss penalty or small positive
            p1.won_last_round = False; p2.won_last_round = False
        elif winner_id == p1_id:
            self.logger.info(f"{combat_debug_prefix} P{p1_id} WON against P{p2_id}. P{p2_id} takes {damage} damage.")
            p2.health -= damage
            p1_reward = ROUND_WIN_REWARD; p2_reward = ROUND_LOSS_REWARD
            p1.won_last_round = True; p2.won_last_round = False
        elif winner_id == p2_id:
            self.logger.info(f"{combat_debug_prefix} P{p2_id} WON against P{p1_id}. P{p1_id} takes {damage} damage.")
            p1.health -= damage
            p1_reward = ROUND_LOSS_REWARD; p2_reward = ROUND_WIN_REWARD
            p1.won_last_round = False; p2.won_last_round = True
        else: # Should not happen if sim output is correct
            self.logger.error(f"{combat_debug_prefix} Unknown winner_id: {winner_id}. Treating as a draw with 1 damage to both.")
            p1.health -= 1; p2.health -= 1
            p1_reward = ROUND_LOSS_REWARD / 2; p2_reward = ROUND_LOSS_REWARD / 2
            p1.won_last_round = False; p2.won_last_round = False
            is_draw = True # Treat as draw for elimination logic

        p1_eliminated = not p1.is_alive()
        p2_eliminated = not p2.is_alive()

        if p1_eliminated: self.logger.info(f"{combat_debug_prefix} P{p1_id} was eliminated (Health: {p1.health}).")
        if p2_eliminated: self.logger.info(f"{combat_debug_prefix} P{p2_id} was eliminated (Health: {p2.health}).")

        self._cumulative_rewards_this_episode[f"p{p1_id}_combat"] += p1_reward
        self._cumulative_rewards_this_episode[f"p{p2_id}_combat"] += p2_reward

        self.logger.debug(f"{combat_debug_prefix} --- COMBAT END --- P1 Reward: {p1_reward:.3f}, P2 Reward: {p2_reward:.3f}")
        return p1_reward, p2_reward, p1_eliminated, p2_eliminated, is_draw

    def _handle_player_elimination(self, player_id: int, placement_rewards_map: Dict[int, float]):
        elim_debug_prefix = f"[Env {self.env_id} Rnd {self.round_idx} Elim P{player_id}]"
        player = self.players.get(player_id)
        if not player or player.is_alive(): # Should not happen if called correctly
            self.logger.warning(f"{elim_debug_prefix} Player already dead or not found. Skipping.")
            return 0.0 # No placement reward

        # Determine placement
        # This logic assumes players are eliminated one by one. If multiple can be eliminated in one round,
        # their placement would be tied. For simplicity, we use the count of remaining alive players.
        num_alive_currently = sum(1 for p_id, p_obj in self.players.items() if p_obj.is_alive())
        placement = num_alive_currently + 1 # If 0 alive, they are 1st. If 1 alive, they are 2nd, etc.

        # Ensure placement is within the bounds of NUM_PLAYERS
        placement = max(1, min(placement, NUM_PLAYERS))

        placement_reward = placement_rewards_map.get(placement, 0.0)
        self.logger.info(f"{elim_debug_prefix} Player P{player_id} eliminated. Placement: {placement}. Reward: {placement_reward:.3f}")

        # Add placement reward to cumulative for the episode
        self._cumulative_rewards_this_episode[f"p{player_id}_placement"] += placement_reward

        # If the eliminated player is the agent, set episode_finished
        if player_id == self.agent.player_id:
            self.episode_finished = True
            self.logger.info(f"{elim_debug_prefix} Agent P{player_id} eliminated. Episode finished.")

        return placement_reward # Return the placement reward itself

    def _get_opponent_action(self, opponent: Player, current_obs_for_opponent: _global_np.ndarray, action_mask_for_opponent: _global_np.ndarray) -> int: # Using _global_np
        opp_act_debug_prefix = f"[Env {self.env_id} OpponentAction P{opponent.player_id}]"

        source_type = opponent.opponent_source_type
        identifier = opponent.opponent_identifier # e.g., path for "path", brain_id for "manager"

        policy_to_use: Optional[BasePolicy] = None

        if source_type == "path":
            if identifier and identifier in self.policy_cache:
                policy_to_use = self.policy_cache[identifier]
                if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"{opp_act_debug_prefix} Using cached path-policy: '{os.path.basename(identifier)}'")
            elif identifier: # Path provided but not in cache - try to load
                if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"{opp_act_debug_prefix} Path-policy '{os.path.basename(identifier)}' not in cache. Attempting load.")
                policy_to_use = self._load_and_cache_policy(identifier, for_opponent_id=opponent.player_id)
                if not policy_to_use: # Load failed, fallback to random
                    self.logger.warning(f"{opp_act_debug_prefix} Failed to load path-policy '{os.path.basename(identifier)}'. Falling back to random action for this step.")
                    source_type = "random" # Override for this step
            else: # No identifier for path type, fallback to random
                self.logger.warning(f"{opp_act_debug_prefix} Path source type but no identifier. Falling back to random.")
                source_type = "random"

        elif source_type == "manager":
            if self.opponent_brain_manager and identifier:
                policy_to_use = self.opponent_brain_manager.get_brain_policy(identifier)
                if policy_to_use:
                    if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"{opp_act_debug_prefix} Using manager-sourced policy for brain_id: '{identifier}'")
                else: # Brain ID not found in manager, fallback to random
                    self.logger.warning(f"{opp_act_debug_prefix} Brain_id '{identifier}' not found in manager. Falling back to random.")
                    source_type = "random"
            elif not self.opponent_brain_manager:
                self.logger.warning(f"{opp_act_debug_prefix} Manager source type but no OpponentBrainManager. Falling back to random.")
                source_type = "random"
            else: # No identifier for manager type, fallback to random
                self.logger.warning(f"{opp_act_debug_prefix} Manager source type but no brain_id. Falling back to random.")
                source_type = "random"

        # If source_type is "random" (either initially or as fallback)
        if source_type == "random":
            if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"{opp_act_debug_prefix} Using random action selection.")
            valid_actions = _global_np.where(action_mask_for_opponent)[0] # Using _global_np
            if valid_actions.size > 0:
                return _global_np.random.choice(valid_actions) # Using _global_np
            else: # No valid actions, should not happen if mask is correct (pass is always possible)
                self.logger.error(f"{opp_act_debug_prefix} No valid actions for random opponent! Defaulting to 0 (Pass).")
                # Default to pass action using the new system, if possible
                try: return self.get_new_flat_index(AT_SYSTEM_ACTIONS, 0, 0, SYSTEM_ACTION_PASS)
                except: return 0


        if policy_to_use:
            # Ensure observation is in the correct format (e.g., unsqueezed if model expects batch)
            # SB3 policies expect a batch of observations.
            obs_for_policy = current_obs_for_opponent.reshape(1, -1) # Reshape to (1, obs_dim)
            
            # Convert to PyTorch tensor and move to policy's device (expected to be CPU from cache/manager)
            obs_tensor = torch.as_tensor(obs_for_policy, device=policy_to_use.device)
            
            # Get action from the policy
            # Note: For MaskablePPO, action_masks should be passed.
            # Assuming the policy is a standard BasePolicy from SB3 (like PPO's MlpPolicy)
            # For MaskablePPO, it's predict(observation, action_masks=..., deterministic=...)
            action_mask_tensor_for_policy = torch.as_tensor(action_mask_for_opponent.reshape(1,-1), device=policy_to_use.device)
            
            try:
                with torch.no_grad():
                    # If policy_to_use is from MaskablePPO, it will use the masks.
                    # If it's a standard PPO policy, it will ignore action_masks if not part of its predict method.
                    action_tuple = policy_to_use.predict(obs_tensor, action_masks=action_mask_tensor_for_policy, deterministic=False) # Get raw action index

                action = action_tuple[0].item() # action_tuple is (actions, states), actions is an array

                if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"{opp_act_debug_prefix} NN policy action: {action}")

                # Validate if the chosen action is actually allowed by the mask
                if not (0 <= action < len(action_mask_for_opponent) and action_mask_for_opponent[action]):
                    self.logger.warning(f"{opp_act_debug_prefix} NN policy chose invalid action {action} (mask value: {action_mask_for_opponent[action] if 0<=action<len(action_mask_for_opponent) else 'OOB'}). Falling back to random valid action.")
                    valid_actions = _global_np.where(action_mask_for_opponent)[0] # Using _global_np
                    if valid_actions.size > 0: return _global_np.random.choice(valid_actions) # Using _global_np
                    else: # Should not happen
                        self.logger.error(f"{opp_act_debug_prefix} No valid actions on fallback from NN invalid choice. Defaulting to 0 (Pass).")
                        try: return self.get_new_flat_index(AT_SYSTEM_ACTIONS, 0, 0, SYSTEM_ACTION_PASS)
                        except: return 0
                return action
            except Exception as e_predict:
                self.logger.error(f"{opp_act_debug_prefix} Error during policy.predict: {e_predict}. Falling back to random.")
                self.logger.exception("Full traceback for policy.predict failure:")
                valid_actions = _global_np.where(action_mask_for_opponent)[0] # Using _global_np
                if valid_actions.size > 0: return _global_np.random.choice(valid_actions) # Using _global_np
                else:
                    try: return self.get_new_flat_index(AT_SYSTEM_ACTIONS, 0, 0, SYSTEM_ACTION_PASS)
                    except: return 0

        # Fallback if policy_to_use was not set for some reason (should be covered by source_type = "random" above)
        self.logger.error(f"{opp_act_debug_prefix} Policy not determined, falling back to random. THIS IS A BUG.")
        valid_actions = _global_np.where(action_mask_for_opponent)[0] # Using _global_np
        if valid_actions.size > 0: return _global_np.random.choice(valid_actions) # Using _global_np
        else:
            try: return self.get_new_flat_index(AT_SYSTEM_ACTIONS, 0, 0, SYSTEM_ACTION_PASS)
            except: return 0


    def step(self, action: int) -> Tuple[_global_np.ndarray, float, bool, bool, Dict[str, Any]]: # Using _global_np
        step_debug_prefix = f"[Env {self.env_id} Rnd {self.round_idx} Step]"
        self.logger.debug(f"{step_debug_prefix} --- STEP START --- Agent Action: {action}")

        current_total_reward = 0.0
        self._generated_files_this_step = [] # Clear list for this step
        self._pairs_presimulated_this_step = set() # Clear set for this step

        try: # --- ADDED TRY BLOCK for overall step logic ---
            if self.episode_finished:
                self.logger.warning(f"{step_debug_prefix} Step called on finished episode. Resetting environment.")
                obs, info = self.reset()
                # Ensure info is JSON serializable after reset
                try: info = deep_convert_np_types(info, context='info_step_ep_finished_reset_return')
                except Exception as e_convert_ep_done: self.logger.error(f"{step_debug_prefix} Error converting info after ep_finished reset: {e_convert_ep_done}")
                return obs, 0.0, True, False, info # Return current obs, 0 reward, terminated=True, truncated=False

            if self.agent is None: # Should not happen if reset correctly
                self.logger.critical(f"{step_debug_prefix} Agent is None at start of step. Resetting.")
                obs, info = self.reset()
                try: info = deep_convert_np_types(info, context='info_step_agent_none_reset_return')
                except Exception as e_convert_agent_none: self.logger.error(f"{step_debug_prefix} Error converting info after agent_none reset: {e_convert_agent_none}")
                return obs, 0.0, True, False, info

            # --- Agent's Turn ---
            # Phase checks and action application for agent (Player 0)
            # Ensure action mask is checked *before* applying action
            current_mask_for_agent = self.action_masks() # Get current valid actions for agent
            if not (0 <= action < self.action_space.n and current_mask_for_agent[action]):
                self.logger.warning(f"{step_debug_prefix} Agent (P0) chose invalid action {action} (Mask val: {current_mask_for_agent[action] if 0<=action<self.action_space.n else 'OOB'}). Applying penalty.")
                current_total_reward += INVALID_ACTION_PENALTY
                # Optionally, force a "Pass" action or a random valid action instead of just penalizing
                # Forcing PASS:
                # try: action = self.get_new_flat_index(AT_SYSTEM_ACTIONS, 0, 0, SYSTEM_ACTION_PASS)
                # except: action = 0 # Fallback if get_new_flat_index fails
                # self.logger.warning(f"{step_debug_prefix} Forcing PASS action ({action}) due to invalid choice.")
            
            # Apply agent's action (even if invalid, _apply_action might penalize further)
            action_reward = self._apply_action(self.agent, action)
            current_total_reward += action_reward
            self._cumulative_rewards_this_episode["agent_actions"] += action_reward
            self.logger.debug(f"{step_debug_prefix} Agent P0 Action Reward: {action_reward:.3f}, Cumulative Step Reward: {current_total_reward:.3f}")

            # --- Opponents' Turns (if in planning/augment phase) ---
            # Opponents take actions only if the agent is in a phase where others would also act.
            # This is a simplified model; in a real multi-agent setup, turns might be more interleaved or simultaneous.
            if self.agent.current_phase in ["planning", "augment"]:
                for i in range(1, NUM_PLAYERS): # Iterate through opponents
                    opponent = self.players.get(i)
                    if opponent and opponent.is_alive():
                        # Get observation and action mask from opponent's perspective
                        # This requires _get_obs to be adaptable or to generate a global state opponents can parse.
                        # For simplicity, assume _get_obs() gives a global view, and action_masks() can work for any player.
                        # This part needs careful implementation of how opponents perceive state and valid actions.

                        # Create a temporary "agent" view for the opponent to get their obs and mask
                        original_agent = self.agent
                        self.agent = opponent # Temporarily set opponent as current agent for obs/mask generation

                        obs_for_opponent = self._get_obs()
                        mask_for_opponent = self.action_masks()

                        self.agent = original_agent # Restore actual agent

                        opponent_action = self._get_opponent_action(opponent, obs_for_opponent, mask_for_opponent)

                        if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"{step_debug_prefix} Opponent P{i} chose action {opponent_action} (Source: {opponent.opponent_source_type}, ID: {opponent.opponent_identifier})")

                        # Apply opponent's action (reward is not directly used for training agent, but can be logged)
                        _ = self._apply_action(opponent, opponent_action)
                        # No need to add opponent's action_reward to current_total_reward for the agent.
            
            # --- Round Progression & Combat ---
            # If agent action resulted in "combat_preparation", proceed to combat and next round.
            if self.agent.current_phase == "combat_preparation":
                self.logger.info(f"{step_debug_prefix} Agent ended planning. Proceeding to combat for Round {self.round_idx}.")

                # 1. Determine pairings
                pairings = self._determine_pairings()
                if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"{step_debug_prefix} Combat pairings: {pairings}")

                # 2. Resolve combats
                eliminated_this_round_ids: List[int] = []
                for p1_id, p2_id in pairings:
                    # Ensure pair hasn't been pre-simulated (e.g. if agent was part of a pairing resolved early for its own state)
                    # This check is more relevant if _resolve_combat could be called outside this main loop for the agent.
                    # For now, assume all pairs are fresh unless explicitly handled.

                    p1_reward_combat, p2_reward_combat, p1_elim, p2_elim, _ = self._resolve_combat(p1_id, p2_id)

                    if p1_id == self.agent.player_id: current_total_reward += p1_reward_combat
                    if p2_id == self.agent.player_id: current_total_reward += p2_reward_combat # Agent could be p2

                    if p1_elim: eliminated_this_round_ids.append(p1_id)
                    if p2_elim: eliminated_this_round_ids.append(p2_id)

                # 3. Handle eliminations and placement rewards
                # Sort by health if multiple eliminations, or by order of combat - current is by order of combat.
                unique_eliminated_ids = list(set(eliminated_this_round_ids)) # Remove duplicates if any
                for elim_id in unique_eliminated_ids:
                    # Check if player is truly eliminated (health <= 0)
                    if self.players.get(elim_id) and not self.players[elim_id].is_alive():
                        placement_rew = self._handle_player_elimination(elim_id, PLACEMENT_REWARDS)
                        if elim_id == self.agent.player_id:
                            current_total_reward += placement_rew
                    elif self.players.get(elim_id) and self.players[elim_id].is_alive():
                        # This case means they were marked 'elim' by combat but health might be >0 due to some other effect or bug.
                         self.logger.warning(f"{step_debug_prefix} Player P{elim_id} marked elim by combat but still alive (Health: {self.players[elim_id].health}). Not processing placement reward yet.")


                # 4. Check for game end (sole survivor or agent eliminated)
                alive_players = [p for p_id, p in self.players.items() if p.is_alive()]
                if len(alive_players) <= 1:
                    self.episode_finished = True
                    if len(alive_players) == 1 and alive_players[0].player_id == self.agent.player_id: # Agent is the sole survivor
                        self.logger.info(f"{step_debug_prefix} Agent P{self.agent.player_id} is the SOLE SURVIVOR! Final placement reward.")
                        # Agent gets 1st place reward if not already given by _handle_player_elimination
                        # This usually happens if others are eliminated, then agent is last one standing.
                        # If agent was already eliminated, episode_finished is true, handled at start of step.
                        # We need to ensure 1st place reward is given if agent is the winner.
                        # _handle_player_elimination gives reward based on `num_alive_currently + 1`.
                        # If agent is last one, num_alive_currently would be 1 (the agent itself). Placement = 1+1=2. This is wrong.
                        # So, if agent is sole survivor, explicitly give 1st place reward if not already given or if it was lower.

                        # Check if agent already received a placement reward this episode.
                        # This logic might be tricky; _handle_player_elimination should correctly assign 1st if others die first.
                        # The problem is if agent is sole survivor *before* its own elimination is processed.

                        # Simpler: if episode ends and agent is alive, they are 1st.
                        # Check if a 1st place reward was already attributed via _handle_player_elimination
                        # This can happen if agent was P0, and P1..P7 all got eliminated, then P0 wins.
                        # _handle_player_elimination for P1..P7 would have been called.
                        # If agent is alive and episode_finished is set here, it means they are the winner.
                        # The placement reward should be for 1st place.
                        # Let's assume _handle_player_elimination for others correctly sets things up.
                        # If agent is the one remaining, and it wasn't due to its own elimination, it's a win.

                        # The reward for winning (1st place) should be handled here if not by elimination sequence
                        # Check if agent's placement reward has been set for 1st place
                        agent_placement_reward_key = f"p{self.agent.player_id}_placement"
                        if self._cumulative_rewards_this_episode.get(agent_placement_reward_key, 0.0) < PLACEMENT_REWARDS.get(1,0.0) or \
                           agent_placement_reward_key not in self._cumulative_rewards_this_episode :

                           # Clear any previous lower placement reward for the agent if they are now 1st.
                           previous_placement_reward = self._cumulative_rewards_this_episode.pop(agent_placement_reward_key, 0.0)
                           if previous_placement_reward != 0:
                               self.logger.info(f"{step_debug_prefix} Agent P{self.agent.player_id} was previously given placement reward {previous_placement_reward}, but is now 1st. Overwriting.")
                               current_total_reward -= previous_placement_reward # Subtract old one from this step's reward if it was added this step

                           first_place_reward = PLACEMENT_REWARDS.get(1, 1.0) # Default to 1.0 if not in map
                           current_total_reward += first_place_reward
                           self._cumulative_rewards_this_episode[agent_placement_reward_key] = first_place_reward
                           self.logger.info(f"{step_debug_prefix} Agent P{self.agent.player_id} WINS (sole survivor). Added 1st place reward: {first_place_reward:.3f}")

                # 5. Prepare for next round (if game not over)
                if not self.episode_finished:
                    self.round_idx += 1
                    self.logger.info(f"{step_debug_prefix} --- ROUND {self.round_idx} START ---")
                    for p_id, p_obj in self.players.items():
                        if p_obj.is_alive():
                            p_obj.coins += INCOME_AMOUNTS[min(p_obj.coins // INCOME_ROUND_BREAKPOINTS[0] if INCOME_ROUND_BREAKPOINTS[0]>0 else float('inf'), len(INCOME_AMOUNTS)-1)] if INCOME_ROUND_BREAKPOINTS else INCOME_AMOUN bestimmte income
                            p_obj.xp += PASSIVE_XP_PER_ROUND
                            self._check_level_up(p_obj)
                            self._create_player_shops(p_obj) # Refresh shops for all alive players
                            p_obj.current_phase = "planning" # Reset phase for all alive players

                            # Special Shop Trigger Logic
                            if p_obj.player_id == self.agent.player_id: # Only for agent for now
                                if self.round_idx == 4 and not p_obj.special_shop_active: # Weapon shop on round 4 (actual round number)
                                    p_obj.special_shop_active = True
                                    p_obj.special_shop_type = "weapon"
                                    self._populate_special_shop(p_obj, "weapon")
                                    p_obj.special_shop_round_triggered = self.round_idx
                                    self.logger.info(f"{step_debug_prefix} Agent P{p_obj.player_id} triggered SPECIAL WEAPON shop.")
                                elif self.round_idx in [11, 18] and not p_obj.special_shop_active: # Amplifier shops
                                    if p_obj.ranger_weapon and p_obj.ranger_weapon.get("TypeID", {}).get("Name"): # Only if weapon equipped
                                        p_obj.special_shop_active = True
                                        p_obj.special_shop_type = "weapon_amplifier"
                                        self._populate_special_shop(p_obj, "weapon_amplifier")
                                        p_obj.special_shop_round_triggered = self.round_idx
                                        self.logger.info(f"{step_debug_prefix} Agent P{p_obj.player_id} triggered SPECIAL AMPLIFIER shop.")
                                    else:
                                        self.logger.info(f"{step_debug_prefix} Agent P{p_obj.player_id} would trigger Amplifier shop, but no weapon equipped.")
                                elif p_obj.special_shop_active and (self.round_idx > p_obj.special_shop_round_triggered):
                                    # If special shop was active and not used in the round it was triggered, clear it.
                                    self.logger.info(f"{step_debug_prefix} Agent P{p_obj.player_id} special shop from round {p_obj.special_shop_round_triggered} expired unused.")
                                    p_obj.special_shop_active = False
                                    p_obj.special_shop_type = None
                                    p_obj.special_shop_items = [None] * 5
                                    p_obj.special_shop_round_triggered = -1
                else: # Episode finished
                     self.logger.info(f"{step_debug_prefix} Episode finished. Final cumulative rewards: {dict(self._cumulative_rewards_this_episode)}")


            # End of "combat_preparation" block
            # If not in "combat_preparation", agent is still in planning/augment/placement.

            # --- Finalize Step ---
            self._last_observation = self._get_obs()
            self._last_info = self._get_info()
            
            # Ensure info is JSON serializable before returning
            try: self._last_info = deep_convert_np_types(self._last_info, context='info_step_main_return')
            except Exception as e_convert_step:
                self.logger.error(f"{step_debug_prefix} Error converting _last_info: {e_convert_step}")
                # Fallback info if conversion fails
                self._last_info = {"env_id": self.env_id, "round": self.round_idx, "error": "step_info_conversion_failed"}


            # terminated is True if episode is finished (agent eliminated or sole survivor)
            terminated = self.episode_finished
            # truncated is False unless a time limit or other condition not related to game objective is met.
            # For now, not using truncation explicitly.
            truncated = False

            if DETAILED_ACTION_DEBUG: self.logger.debug(f"{step_debug_prefix} Step End. Reward: {current_total_reward:.4f}, Terminated: {terminated}, Truncated: {truncated}")
            if terminated: self.logger.info(f"{step_debug_prefix} Episode terminated. Final reward for agent this step: {current_total_reward:.4f}")

            return self._last_observation, current_total_reward, terminated, truncated, self._last_info

        except Exception as e_step_outer: # --- CATCH ANY EXCEPTION in overall step logic ---
            self.logger.critical(f"[FATAL Env {self.env_id} Step] CRITICAL ERROR: {e_step_outer}")
            self.logger.exception("Full traceback for step failure:")
            
            # Attempt to return a valid observation and info, even if it's just zeros/defaults
            failed_obs = _global_np.zeros(self.observation_space.shape, dtype=self.observation_space.dtype) # Using _global_np
            failed_info = {"env_id": self.env_id, "round": self.round_idx, "error": "critical_step_exception",
                           "exception_details": str(e_step_outer)}
            try: failed_info = deep_convert_np_types(failed_info, context='info_step_exception_return')
            except: pass # Ignore if even this fails
            
            # Terminate the episode on critical error to prevent loops
            return failed_obs, 0.0, True, False, failed_info


    def render(self, mode: Optional[str] = None) -> Optional[Union[str, _global_np.ndarray]]: # Using _global_np for ndarray
        # Inherit mode from constructor if not specified
        effective_mode = mode if mode else self.render_mode

        if effective_mode == "human":
            # Implement human-readable rendering (e.g., Pygame, Matplotlib)
            # This is a placeholder. A full GUI would be complex.
            print(f"\n--- ENV {self.env_id} RENDER (Human) ---")
            if self.agent:
                print(f"Round: {self.round_idx}, Player {self.agent.player_id} (Agent) Phase: {self.agent.current_phase}")
                print(f"  Health: {self.agent.health}, Coins: {self.agent.coins}, Level: {self.agent.level}, XP: {self.agent.xp}")
                print(f"  Board ({len(self.agent.board)}/{self.agent.level}):")
                for unit in self.agent.board:
                    tid = unit.get("TypeID", {})
                    inst = unit.get("Instance", {})
                    pos = unit.get("Position", {})
                    print(f"    - {tid.get('LineType')} S{tid.get('Stage')} (ID: {inst.get('ID', 'N/A')[:4]}) @ ({pos.get('Q')},{pos.get('R')}) Augs: {len(inst.get('EquippedAugments',[]))}")
                print(f"  Bench ({len(self.agent.bench)}/{BENCH_MAX_SIZE}):")
                for unit in self.agent.bench:
                    tid = unit.get("TypeID", {})
                    inst = unit.get("Instance", {})
                    print(f"    - {tid.get('LineType')} S{tid.get('Stage')} (ID: {inst.get('ID', 'N/A')[:4]}) Augs: {len(inst.get('EquippedAugments',[]))}")
                print(f"  Shop: {[item.get('LineType') if item else 'Empty' for item in self.agent.illuvial_shop]}")
                if self.agent.is_augment_choice_pending:
                    print(f"  Augment Choices: {self.agent.available_augment_choices}")
                if self.agent.special_shop_active:
                    print(f"  Special Shop ({self.agent.special_shop_type}): {[item.get('Name') if item else 'Empty' for item in self.agent.special_shop_items]}")

            else: print("  Agent is None.")
            print("--- END RENDER ---")
            return None # Or return an RGB array if using a graphical library
            
        elif effective_mode == "ansi":
            # Implement ANSI string representation for console output
            ansi_str = f"\n--- ENV {self.env_id} RENDER (ANSI) ---\n"
            if self.agent:
                ansi_str += f"Round: {self.round_idx}, Player {self.agent.player_id} (Agent) Phase: {self.agent.current_phase}\n"
                ansi_str += f"  Health: {self.agent.health}, Coins: {self.agent.coins}, Level: {self.agent.level}, XP: {self.agent.xp}\n"
                # ... (add more details for board, bench, shop as in "human" mode but formatted for string)
            else: ansi_str += "  Agent is None.\n"
            ansi_str += "--- END RENDER ---\n"
            return ansi_str

        elif effective_mode == "none" or effective_mode is None:
            return None # No rendering

        # If mode is 'rgb_array', it should return _global_np.ndarray
        # This would require a graphical backend (e.g., Pygame offscreen)
        # For now, not implemented.
        if effective_mode == "rgb_array":
            self.logger.warning("rgb_array render mode not yet implemented. Returning None.")
            return None

        return None # Default for unknown modes

    def close(self):
        close_debug_prefix = f"[Env {self.env_id} Close]"
        self.logger.info(f"{close_debug_prefix} Closing environment.")
        # Clean up resources, if any (e.g., simulation subprocesses, files)

        # Example: Clean up generated simulation files if needed
        # for f_path in self._generated_files_this_step: # Or a list of all historical files
        #     try:
        #         if os.path.exists(f_path): os.remove(f_path)
        #     except OSError as e:
        #         self.logger.warning(f"{close_debug_prefix} Error removing file {f_path}: {e}")

        # If using a persistent simulation process, signal it to terminate.
        # force_reset_simulator() # If this function handles global sim cleanup

        # Clear caches if they hold significant memory and won't be reused
        self.policy_cache.clear()
        if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"{close_debug_prefix} Policy cache cleared.")

        self.logger.info(f"{close_debug_prefix} Environment closed.")

    # --- Opponent Configuration Methods (NEW) ---
    def set_opponent_config(self, opponent_player_id: int, source_type: str, identifier: Optional[str]):
        """
        Configures a specific opponent player.
        - opponent_player_id: The ID of the player to configure (1 to NUM_PLAYERS-1).
        - source_type: "random", "path", or "manager".
        - identifier: Model path for "path", brain_id for "manager", None for "random".
        """
        config_debug_prefix = f"[Env {self.env_id} SetOpponent P{opponent_player_id}]"

        if opponent_player_id == 0: # Cannot configure agent (player 0)
            self.logger.warning(f"{config_debug_prefix} Attempt to configure agent (P0). Ignoring.")
            return
        if opponent_player_id not in self.players:
            self.logger.warning(f"{config_debug_prefix} Opponent ID not found. Ignoring.")
            return

        player_to_configure = self.players[opponent_player_id]

        valid_source_types = ["random", "path", "manager"]
        if source_type not in valid_source_types:
            self.logger.warning(f"{config_debug_prefix} Invalid source_type '{source_type}'. Defaulting to 'random'.")
            source_type = "random"
            identifier = None # Ensure identifier is None for random

        if source_type == "path" and not identifier:
            self.logger.warning(f"{config_debug_prefix} Source_type 'path' requires an identifier (model path). Defaulting to 'random'.")
            source_type = "random"

        if source_type == "manager" and not identifier:
            self.logger.warning(f"{config_debug_prefix} Source_type 'manager' requires an identifier (brain_id). Defaulting to 'random'.")
            source_type = "random"

        # Store this configuration for use on next reset
        self.current_opponent_configs[opponent_player_id] = (source_type, identifier)

        # Apply live to the player object
        player_to_configure.opponent_source_type = source_type
        player_to_configure.opponent_identifier = identifier

        # If type is "path", try to load and cache the policy immediately
        if source_type == "path" and identifier:
            if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"{config_debug_prefix} Path type. Attempting to load and cache model: '{os.path.basename(identifier)}'")
            self._load_and_cache_policy(identifier, for_opponent_id=opponent_player_id)
            # If loading fails, _load_and_cache_policy will log and set the player to random.
            # Update current_opponent_configs to reflect this potential fallback.
            if player_to_configure.opponent_source_type == "random": # Check if it was changed by load failure
                 self.current_opponent_configs[opponent_player_id] = ("random", None)
                 if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"{config_debug_prefix} Path model load failed for '{os.path.basename(identifier)}'. Stored config updated to random.")


        identifier_log_name = os.path.basename(identifier) if source_type == "path" and identifier else identifier
        self.logger.info(f"{config_debug_prefix} Configured to source='{player_to_configure.opponent_source_type}', id='{identifier_log_name or 'None'}'. Stored for next reset.")

    def get_opponent_configs(self) -> Dict[int, Tuple[str, Optional[str]]]:
        """Returns the current configurations for all opponents."""
        # Return the stored configurations that will be used on reset
        return self.current_opponent_configs.copy()

# --- End of IlluviumSingleEnv Class ---

# Example usage (Optional: for direct testing of the environment)
if __name__ == "__main__":
    print("--- IlluviumSingleEnv Direct Test ---")

    # Configure logging for the test
    logging.basicConfig(level=logging.DEBUG, format='[%(levelname)s %(asctime)s %(name)s] %(message)s', datefmt='%H:%M:%S')
    # Silence overly verbose loggers if needed (e.g., from libraries)
    # logging.getLogger("some_library_logger").setLevel(logging.WARNING)

    # Test with a dummy OpponentBrainManager if you want to test "manager" type opponents
    class DummyBrainManager(OpponentBrainManagerInterface):
        def __init__(self): self.brains = {} # type: Dict[str, BasePolicy]
        def get_brain_policy(self, brain_id: str) -> Optional[BasePolicy]: return self.brains.get(brain_id)
        def get_available_brain_ids(self) -> List[str]: return list(self.brains.keys())
        def add_brain(self, policy: BasePolicy, brain_id: str): self.brains[brain_id] = policy

    dummy_manager = DummyBrainManager()
    # If you have a saved model, you can load it and add to the manager for testing
    # try:
    #     ppo_policy = MaskablePPO.load("path_to_your_model.zip", device="cpu").policy
    #     dummy_manager.add_brain(ppo_policy, "test_brain_01")
    #     print("Loaded and added a test brain to DummyBrainManager.")
    # except Exception as e:
    #     print(f"Could not load test brain for DummyBrainManager: {e}")


    # Create environment instance
    # env = IlluviumSingleEnv(env_id=0, render_mode="human", opponent_brain_manager=dummy_manager)
    env = IlluviumSingleEnv(env_id=0, render_mode="human") # Without manager for basic test

    # Configure an opponent (e.g., Player 1) to use a specific policy path or manager brain
    # env.set_opponent_config(1, "path", "path_to_opponent_model.zip")
    # env.set_opponent_config(2, "manager", "test_brain_01")
    # env.set_opponent_config(3, "random", None) # Explicitly random

    print(f"Observation Space: {env.observation_space}, Shape: {env.observation_space.shape}")
    print(f"Action Space: {env.action_space}, Size: {env.action_space.n}")

    # Test reset
    obs, info = env.reset()
    print(f"Initial Observation (shape {obs.shape}, dtype {obs.dtype}):\n{obs[:20]}...{obs[-20:]}") # Print part of obs
    print(f"Initial Info: {json.dumps(info, indent=2)}")
    env.render()

    # Test a few steps with random actions or specific actions
    for step_num in range(50): # Number of test steps
        action_mask = env.action_masks()
        # print(f"Action Mask (sum {action_mask.sum()}): {action_mask}")

        # Choose a random valid action
        valid_actions = _global_np.where(action_mask)[0] # Using _global_np
        if len(valid_actions) == 0:
            print(f"[ERROR MainTestLoop] No valid actions for agent at step {step_num}! Mask: {action_mask}")
            # Try to get pass action if no valid actions
            try: action = env.get_new_flat_index(AT_SYSTEM_ACTIONS, 0, 0, SYSTEM_ACTION_PASS)
            except: action = 0 # Fallback
        else:
            action = _global_np.random.choice(valid_actions) # Using _global_np

        # Or, to test a specific action type:
        # action = env.get_new_flat_index(AT_SYSTEM_ACTIONS, 0, 0, SYSTEM_ACTION_END_PLANNING) # Example: End Planning
        # if not action_mask[action]:
        #     print(f"Chosen specific action {action} is not valid, falling back to random.")
        #     action = _global_np.random.choice(valid_actions) if len(valid_actions) > 0 else 0


        print(f"\n--- Step {step_num + 1} ---")
        decoded_action = env.decode_new_flat_action(action)
        print(f"Taking Action (Flat: {action}, Decoded: {decoded_action})")

        obs, reward, terminated, truncated, info = env.step(action)

        print(f"Reward: {reward:.4f}")
        # print(f"Observation (shape {obs.shape}):\n{obs[:20]}...{obs[-20:]}") # Print part of obs
        # print(f"Info: {json.dumps(info, indent=2)}") # Can be very verbose
        print(f"  Agent Info: {info.get('agent', {}).get('player_id')} H:{info.get('agent', {}).get('health')} G:{info.get('agent', {}).get('coins')} L:{info.get('agent', {}).get('level')} XP:{info.get('agent', {}).get('xp')} Ph:{info.get('agent', {}).get('current_phase')}")
        print(f"  Game Info: Round {info.get('game', {}).get('round_idx')}, Alive: {info.get('game', {}).get('alive_players')}")

        env.render()

        if terminated or truncated:
            print(f"Episode finished after {step_num + 1} steps. Terminated: {terminated}, Truncated: {truncated}")
            final_rewards = info.get("game",{}).get("cumulative_rewards",{})
            print(f"Final Cumulative Rewards: {final_rewards}")

            # Example: Save team if agent won (for debugging/analysis)
            # agent_info = info.get("agent", {})
            # if agent_info.get("player_id") in info.get("game", {}).get("alive_players", []) and len(info.get("game", {}).get("alive_players", [])) == 1:
            #    print("Agent won! Saving final team...")
            #    # Add logic to save team if desired, e.g., using parts of the SaveBestTeamCallback

            # break # Stop loop if episode ended
            print("Resetting for new episode...")
            obs, info = env.reset()
            env.render()


    env.close()
    print("--- Test Finished ---")

# --- END OF FILE train_env.py ---
