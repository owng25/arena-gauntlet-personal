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

POSITIONS_BLUE_LIST: List[Tuple[int, int]] = [(-67,46),(-40,46),(-22,45),(-6,47),(21,47),(-49,37),(12,38),(-58,28),(-31,28),(-13,27),(3,29),(30,29),(-40,19),(21,20),(-49,10),(-22,10),(-4,9),(12,11),(39,11)]
POSITIONS_RED_LIST: List[Tuple[int, int]] = [(68,-47),(41,-47),(23,-45),(6,-46),(-21,-46),(50,-38),(-12,-37),(59,-29),(32,-29),(14,-27),(-3,-28),(-30,-28),(41,-20),(-21,-19),(50,-11),(23,-11),(5,-9),(-12,-10),(-39,-10)]
if len(POSITIONS_BLUE_LIST) != len(POSITIONS_RED_LIST): raise ValueError("Position lists must have same length!")
MAX_BOARD_SLOTS = len(POSITIONS_BLUE_LIST); NUM_UNIQUE_POSITIONS = MAX_BOARD_SLOTS
UNIFIED_ID_TO_BLUE_POS_MAP: Dict[int, Tuple[int, int]] = {i + 1: pos for i, pos in enumerate(POSITIONS_BLUE_LIST)}
BLUE_POS_TO_UNIFIED_ID_MAP: Dict[Tuple[int, int], int] = {pos: i + 1 for i, pos in enumerate(POSITIONS_BLUE_LIST)}
UNIFIED_ID_TO_RED_POS_MAP: Dict[int, Tuple[int, int]] = {i + 1: POSITIONS_RED_LIST[i] for i in range(len(POSITIONS_RED_LIST))}
MAX_ILLUVIAL_ID = 100; MAX_AUGMENT_ID = 50 # Set rough max values (update if needed)

# --- MODIFIED Observation Space Size Calculation ---
FEATURES_PER_UNIT = 5 # <<< CHANGED from 4 to 5 >>>
if DETAILED_SELFPLAY_DEBUG: print(f"[INFO Config] FEATURES_PER_UNIT set to: {FEATURES_PER_UNIT}")

BOARD_FEATURES_PER_PLAYER = MAX_BOARD_SLOTS * FEATURES_PER_UNIT         # <<< MODIFIED: 19 * 5 = 95 >>>
TOTAL_BOARD_FEATURES = NUM_PLAYERS * BOARD_FEATURES_PER_PLAYER        # <<< MODIFIED: 8 * 95 = 760 >>>

# Features specific to the agent (POV player)
AGENT_SCALAR_FEATURES = 5 # Health, Gold, Level, XP, Round
AGENT_BENCH_FEATURES = BENCH_MAX_SIZE * FEATURES_PER_UNIT               # <<< MODIFIED: 15 * 5 = 75 >>>
AGENT_SHOP_FEATURES = SHOP_SIZE * 2 # ID + Cost                         # <<< MODIFIED: 7 * 2 = 14 >>>
AGENT_AUGMENT_FEATURES = 1 + NUM_AUGMENT_CHOICES # is_pending + IDs      # 1 + 3 = 4
AGENT_SPECIFIC_FEATURES = ( AGENT_SCALAR_FEATURES + AGENT_BENCH_FEATURES +
                            AGENT_SHOP_FEATURES + AGENT_AUGMENT_FEATURES ) # <<< MODIFIED: 5 + 75 + 14 + 4 = 98 >>>

# Features specific to EACH opponent (excluding POV player)
OPPONENT_SCALAR_FEATURES = 4 # Health, Gold, Level, XP
OPPONENT_BENCH_FEATURES = BENCH_MAX_SIZE * FEATURES_PER_UNIT            # <<< MODIFIED: 15 * 5 = 75 >>>
FEATURES_PER_OPPONENT = OPPONENT_SCALAR_FEATURES + OPPONENT_BENCH_FEATURES # <<< MODIFIED: 4 + 75 = 79 >>>
TOTAL_OPPONENT_EXTRA_FEATURES = (NUM_PLAYERS - 1) * FEATURES_PER_OPPONENT # <<< MODIFIED: 7 * 79 = 553 >>>

# Grand Total
FLAT_OBSERVATION_SIZE = (
    TOTAL_BOARD_FEATURES +          # 760 (All boards)
    AGENT_SPECIFIC_FEATURES +       # 98  (Agent specifics)
    TOTAL_OPPONENT_EXTRA_FEATURES   # 553 (Opponent specifics)
) # <<< MODIFIED: 760 + 98 + 553 = 1411 >>>
if DETAILED_SELFPLAY_DEBUG: print(f"[INFO Config] Recalculated FLAT_OBSERVATION_SIZE = {FLAT_OBSERVATION_SIZE}")
# --- END MODIFIED Observation Space Size Calculation ---


# --- Flattened Action Space Definitions (OLD - To be removed/replaced) ---
MAX_COMBINED_TARGET_IDX = MAX_BOARD_SLOTS + BENCH_MAX_SIZE # 19 + 15 = 34
NUM_POSITIONS_INCLUDING_BENCH = NUM_UNIQUE_POSITIONS + 1 # 19 + 1 = 20

FLAT_IDX_SELL_START = 0
FLAT_SELL_ACTIONS = MAX_COMBINED_TARGET_IDX # 34 actions
FLAT_IDX_BUY_START = FLAT_IDX_SELL_START + FLAT_SELL_ACTIONS # Index 34

FLAT_BUY_ACTIONS = SHOP_SIZE # 7 actions
FLAT_IDX_REROLL = FLAT_IDX_BUY_START + FLAT_BUY_ACTIONS # Index 34 + 7 = 41
FLAT_IDX_BUY_XP = FLAT_IDX_REROLL + 1 # Index 42
FLAT_IDX_ENTER_PLACEMENT = FLAT_IDX_BUY_XP + 1 # Index 43

FLAT_IDX_MODIFY_UNIT_START = FLAT_IDX_ENTER_PLACEMENT + 1 # Index 44
FLAT_MODIFY_UNIT_ACTIONS = MAX_COMBINED_TARGET_IDX * NUM_POSITIONS_INCLUDING_BENCH # 34 * 20 = 680 actions
FLAT_IDX_EXIT_PLACEMENT = FLAT_IDX_MODIFY_UNIT_START + FLAT_MODIFY_UNIT_ACTIONS # Index 44 + 680 = 724

FLAT_IDX_AUGMENT_SKIP = FLAT_IDX_EXIT_PLACEMENT + 1 # Index 725
FLAT_IDX_AUGMENT_APPLY_START = FLAT_IDX_AUGMENT_SKIP + 1 # Index 726
FLAT_AUGMENT_APPLY_ACTIONS = NUM_AUGMENT_CHOICES * MAX_COMBINED_TARGET_IDX # 3 * 34 = 102 actions
TOTAL_FLAT_ACTIONS = FLAT_IDX_AUGMENT_APPLY_START + FLAT_AUGMENT_APPLY_ACTIONS # 726 + 102 = 828
# --- End OLD Flattened Action Space Definitions ---

# --- New 4D Action Space Definitions ---
NUM_ACTION_TYPES = 6
DIM_ENTITY_SIZE = 60  # For player units or catalogue items
DIM_POSITION_SIZE = 12 # For board positions (subset or mapping needed for the 19 board slots + 1 bench)
DIM_ITEM_SIZE = 18    # For shop slots or item sub-selection (e.g. system actions)

NEW_TOTAL_FLAT_ACTIONS = NUM_ACTION_TYPES * DIM_ENTITY_SIZE * DIM_POSITION_SIZE * DIM_ITEM_SIZE

# Action Type Indices
AT_SYSTEM_ACTIONS = 0
AT_SELL_UNIT = 1
AT_BUY_ILLUVIAL = 2
AT_PLACE_UNIT = 3
AT_RANGER_EQUIP = 4
AT_RANGER_BOND = 5

# Specific item_idx for AT_SYSTEM_ACTIONS
SYSTEM_ACTION_PASS = 0 # Also used for Augment Skip if in augment phase
SYSTEM_ACTION_REROLL_ILLUVIAL_SHOP = 1
SYSTEM_ACTION_BUY_XP = 2
SYSTEM_ACTION_EXIT_PLACEMENT_PHASE = 3 # From placement to planning
SYSTEM_ACTION_END_PLANNING_PHASE = 4   # From planning/augment to combat_preparation
# --- End New 4D Action Space Definitions ---


# --- Data Loading & Basic Helpers ---
try: PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__)) if '__file__' in locals() else os.path.abspath('.')
except NameError: PROJECT_ROOT = os.getcwd()
ILLUVIAL_TEMPLATES_FILE = os.path.join(PROJECT_ROOT, "illuvial_templates.json")
MERGED_FILE = os.path.join(DATA_DIR, "merged_data.json")
COMPLETE_EQUIPMENT_DATABASE_FILE = os.path.join(PROJECT_ROOT, "complete_equipment_database.json")
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

            # Equipment definition maps
            self.weapon_definitions: Dict[str, Dict[str, Any]] = {}
            self.weapon_amplifier_definitions: Dict[str, Dict[str, Any]] = {}
            self.suit_definitions: Dict[str, Dict[str, Any]] = {}

            self._populate_id_maps() # This will now also populate equipment definitions
            
            global MAX_ILLUVIAL_ID, MAX_AUGMENT_ID 
            
            max_obs_component_value = 200.0
            if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"Setting Obs Space Shape: ({FLAT_OBSERVATION_SIZE},)")
            self.observation_space = Box(low=0, high=max_obs_component_value, shape=(FLAT_OBSERVATION_SIZE,), dtype=_global_np.float32)
            if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"Setting Act Space Size to NEW_TOTAL_FLAT_ACTIONS: {NEW_TOTAL_FLAT_ACTIONS}")
            self.action_space = Discrete(NEW_TOTAL_FLAT_ACTIONS) # MODIFIED

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
        global MAX_ILLUVIAL_ID, MAX_AUGMENT_ID
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

        # Populate equipment definition maps
        for name, data in self.weapons_data_from_db.items():
            if isinstance(data, dict) and data.get("Name"):
                self.weapon_definitions[data["Name"]] = data
            else:
                self.logger.warning(f"Invalid weapon data for '{name}' in equipment database. Skipping.")

        for name, data in self.weapon_amplifiers_data_from_db.items():
            if isinstance(data, dict) and data.get("Name"):
                self.weapon_amplifier_definitions[data["Name"]] = data
            else:
                self.logger.warning(f"Invalid weapon amplifier data for '{name}' in equipment database. Skipping.")

        for name, data in self.suits_data_from_db.items():
            if isinstance(data, dict) and data.get("Name"):
                self.suit_definitions[data["Name"]] = data
            else:
                self.logger.warning(f"Invalid suit data for '{name}' in equipment database. Skipping.")

        if DETAILED_SELFPLAY_DEBUG:
            self.logger.debug(f"Populated {len(self.weapon_definitions)} weapon definitions.")
            self.logger.debug(f"Populated {len(self.weapon_amplifier_definitions)} weapon amplifier definitions.")
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
        if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"{debug_prefix} Ranger unit (ID: {player.ranger_unit_id}) added to board at {ranger_position}.")


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

    def reset(self, *, seed: Optional[int] = None, options: Optional[dict] = None) -> Tuple[_global_np.ndarray, Dict[str, Any]]:
        reset_debug_prefix = f"[Env {self.env_id} Reset]"; 
        self.logger.debug(f"{reset_debug_prefix} --- RESET START --- Seed: {seed}, Options: {options}")
        super().reset(seed=seed) # Calls Gymnasium Env's reset to reset seed if provided
        
        if seed is not None: 
            # Note: Gymnasium's super().reset(seed=...) handles random.seed and _global_np.random.seed for the environment instance.
            # You might not need this explicit call unless you have other random generators.
            # For consistency with your original code's explicit seeding:
            effective_seed = seed + self.env_id
            random.seed(effective_seed)
            _global_np.random.seed(effective_seed)
            self.logger.debug(f"{reset_debug_prefix} Explicitly set random/numpy seed to {effective_seed}.")

        self.round_idx = 0
        self.episode_finished = False
        self._generated_files_this_step = []
        self._last_agent_sim_reward = 0.0
        self._cumulative_rewards_this_episode = defaultdict(float)
        self._pairs_presimulated_this_step = set()

        # Clear policy_cache for path-based models. Manager-based brains are handled by the manager instance itself.
        if self.policy_cache:
            self.logger.debug(f"{reset_debug_prefix} Clearing path-based policy_cache (had {len(self.policy_cache)} entries).")
        self.policy_cache.clear()

        self.players = {pid: Player(pid) for pid in range(NUM_PLAYERS)}
        self.agent = self.players.get(0)
        if self.agent is None: 
            self.logger.critical(f"{reset_debug_prefix} Failed to assign agent P0.")
            raise RuntimeError(f"{reset_debug_prefix} Failed to assign agent P0.")

        self.logger.debug(f"{reset_debug_prefix} Resetting players states and forcing initial units...")
        for pid, p in self.players.items():
            try: 
                p.clear_state_for_reset()
                p.coins = FORCED_COINS # From constant
                self._create_player_shops(p)
                self._force_initial_unit(p) # Ensure each player has an initial unit
            except Exception as e: 
                self.logger.critical(f"{reset_debug_prefix} FATAL error resetting P{pid}: {e}")
                self.logger.exception("Full traceback for player reset failure:")
                raise

        # Re-apply stored opponent configurations from self.current_opponent_configs
        if self.current_opponent_configs: # Check if attribute exists and is not empty
            self.logger.debug(f"{reset_debug_prefix} Re-applying stored opponent configurations ({len(self.current_opponent_configs)} configs)...")
            for opp_id, config_tuple in self.current_opponent_configs.items():
                if opp_id != 0 and opp_id in self.players:
                    player_obj = self.players[opp_id]
                    source_type, identifier = config_tuple
                    
                    player_obj.opponent_source_type = source_type
                    player_obj.opponent_identifier = identifier
                    
                    identifier_log_name = os.path.basename(identifier) if source_type == "path" and identifier else identifier
                    self.logger.debug(f"{reset_debug_prefix}   P{opp_id} configured to: source='{source_type}', id='{identifier_log_name or 'None'}'.")
                    
                    # Pre-warm cache for "path"-based models if an identifier (path) is provided
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
            self._last_observation = _global_np.zeros(self.observation_space.shape, dtype=self.observation_space.dtype)
            self._last_info = {"env_id": self.env_id, "round": 0, "error": "reset_initial_obs_fail"}
        
        # Ensure conversion to JSON-compatible types for info dict.
        try: self._last_info = deep_convert_np_types(self._last_info, context='info_reset_return')
        except Exception as e_convert_reset: 
            self.logger.error(f"{reset_debug_prefix} ERROR converting initial info: {e_convert_reset}")
            self.logger.exception("Traceback for info conversion failure:")
            self._last_info = {"env_id": self.env_id, "round": 0, "error": "reset_info_conversion_failed"}

        # Basic shape/dtype validation
        if self._last_observation.shape != self.observation_space.shape:
            self.logger.warning(f"{reset_debug_prefix} Initial Obs shape mismatch: {self._last_observation.shape} vs {self.observation_space.shape}. Padding/Truncating.");
            padded_obs = _global_np.zeros(self.observation_space.shape, dtype=self.observation_space.dtype); 
            copy_len = min(self._last_observation.size, padded_obs.size); 
            padded_obs.flat[:copy_len] = self._last_observation.flat[:copy_len]; 
            self._last_observation = padded_obs
        if self._last_observation.dtype != _global_np.float32: self._last_observation = self._last_observation.astype(_global_np.float32)
        
        self.logger.info(f"{reset_debug_prefix} Reset complete. Initial Obs valid: {_global_np.all(_global_np.isfinite(self._last_observation))}")
        return self._last_observation, self._last_info
    
    # In train_env.py
# Inside IlluviumSingleEnv class
    def action_masks(self) -> _global_np.ndarray:
        mask_debug_prefix = f"[DEBUG MASK Env {self.env_id} P{self.agent.player_id if self.agent else 'N/A'} R{self.round_idx}]"; flat_mask = _global_np.zeros(TOTAL_FLAT_ACTIONS, dtype=bool)
        try:
            if self.agent is None:
                if hasattr(self, 'logger') and self.logger: self.logger.error(f"{mask_debug_prefix} Agent is None, returning mostly False mask.")
                fallback_mask = _global_np.zeros(TOTAL_FLAT_ACTIONS, dtype=bool)
                if TOTAL_FLAT_ACTIONS > 0 and FLAT_IDX_ENTER_PLACEMENT >=0 : fallback_mask[FLAT_IDX_ENTER_PLACEMENT]=True
                elif TOTAL_FLAT_ACTIONS > 0: fallback_mask[0]=True
                return fallback_mask

            player = self.agent
            # State Consistency Check
            # This needs to be done carefully as phases might be implicitly handled by the new action types
            if player.is_augment_choice_pending and player.current_phase != "augment":
                 if DETAILED_ACTION_DEBUG: self.logger.warning(f"{mask_debug_prefix} State mismatch: AugPend but Phase={player.current_phase}. Forcing to augment phase for mask calc.")
                 # Do not change player.current_phase here, let actions handle phase transitions
            elif not player.is_augment_choice_pending and player.current_phase == "augment":
                 if DETAILED_ACTION_DEBUG: self.logger.warning(f"{mask_debug_prefix} State mismatch: Phase=augment but no AugPend. Acting as if planning phase for mask calc.")

            # Iterate through all possible actions in the new 4D space
            for i in range(NEW_TOTAL_FLAT_ACTIONS):
                action_type_idx, entity_idx, position_idx, item_idx = self.decode_new_flat_action(i)

                # Default to False, only enable if explicitly valid
                current_action_valid = False

                if action_type_idx == AT_SYSTEM_ACTIONS:
                    if item_idx == SYSTEM_ACTION_PASS: # Pass
                        current_action_valid = player.current_phase in ["planning", "augment"] # Can pass in planning or if augment is pending
                    elif item_idx == SYSTEM_ACTION_REROLL_ILLUVIAL_SHOP: # Reroll Illuvial Shop
                        current_action_valid = player.current_phase == "planning" and player.coins >= REROLL_COST and not player.special_shop_active
                    elif item_idx == SYSTEM_ACTION_BUY_XP: # Buy XP
                        current_action_valid = player.current_phase == "planning" and player.coins >= BUY_XP_COST
                    elif item_idx == SYSTEM_ACTION_EXIT_PLACEMENT_PHASE: # Exit Placement Phase
                        current_action_valid = player.current_phase == "placement"
                    elif item_idx == SYSTEM_ACTION_END_PLANNING_PHASE: # End Planning Phase / Begin Combat/Resolution
                        # This action is valid if in planning, or if in augment (implies planning is done, just need to make aug choice or skip)
                        current_action_valid = player.current_phase == "planning" or (player.current_phase == "augment" and player.is_augment_choice_pending)


                elif action_type_idx == AT_SELL_UNIT:
                    if player.current_phase == "planning" and not player.special_shop_active:
                        combined_units = player.get_combined_units()
                        if 0 <= entity_idx < len(combined_units):
                            unit_to_sell = combined_units[entity_idx]
                            if unit_to_sell.get("Instance", {}).get("ID") != player.ranger_unit_id:
                                current_action_valid = True

                elif action_type_idx == AT_BUY_ILLUVIAL:
                     if player.current_phase == "planning" and not player.special_shop_active and len(player.bench) < BENCH_MAX_SIZE:
                         if 0 <= item_idx < SHOP_SIZE: # Max 7 main shop slots
                             ill_template = player.illuvial_shop[item_idx]
                             if ill_template and player.coins >= get_illuvial_cost(ill_template):
                                 current_action_valid = True

                elif action_type_idx == AT_PLACE_UNIT:
                    if player.current_phase == "placement":
                        combined_units = player.get_combined_units()
                        if 0 <= entity_idx < len(combined_units):
                            target_unit_to_place = combined_units[entity_idx]
                            unit_is_ranger = target_unit_to_place.get("Instance", {}).get("ID") == player.ranger_unit_id

                            if position_idx == 0: # Target is Bench
                                if target_unit_to_place.get("Position") is not None and not unit_is_ranger: # Unit is on board and not ranger
                                    current_action_valid = len(player.bench) < BENCH_MAX_SIZE
                            elif 1 <= position_idx < (NUM_UNIQUE_POSITIONS + 1) and position_idx < DIM_POSITION_SIZE : # Target is a board slot (1 to 11 for DIM_POSITION_SIZE=12)
                                # Map position_idx (1-11) to actual board slot IDs (1-19)
                                actual_board_slot_id = position_idx # Assuming direct map for now for simplicity
                                if actual_board_slot_id <= NUM_UNIQUE_POSITIONS :
                                    # Simplified placement logic for mask: check if target is different from current, or if moving from bench
                                    # Detailed validation (level cap, swap logic) is complex for mask, _apply_action handles actual validity
                                    is_on_board = target_unit_to_place.get("Position") is not None
                                    target_coords = UNIFIED_ID_TO_BLUE_POS_MAP.get(actual_board_slot_id)
                                    occupying_unit = player.get_unit_at_pos(target_coords[0], target_coords[1]) if target_coords else True # Assume occupied if invalid coords

                                    if is_on_board: # Moving board unit or swapping
                                        current_action_valid = True # Allow attempt, _apply_action validates
                                    else: # Moving from bench
                                        current_action_valid = player.units_on_board_count() < player.level or occupying_unit is not None


                elif action_type_idx == AT_RANGER_EQUIP:
                    if player.current_phase == "planning" and player.special_shop_active:
                        if 0 <= item_idx < 5: # 5 special shop slots
                            if player.special_shop_items[item_idx] is not None:
                                if player.special_shop_type == "weapon":
                                    current_action_valid = True
                                elif player.special_shop_type == "weapon_amplifier":
                                    if player.ranger_weapon and player.ranger_weapon.get("TypeID", {}).get("Name") and len(player.ranger_weapon_amplifiers) < 2:
                                        current_action_valid = True
                
                elif action_type_idx == AT_RANGER_BOND:
                     if player.current_phase == "planning": # Assuming bonding happens in planning
                         combined_units = player.get_combined_units()
                         if 0 <= entity_idx < len(combined_units):
                             target_illuvial = combined_units[entity_idx]
                             if target_illuvial.get("TypeID", {}).get("UnitType") == "Illuvial" and \
                                target_illuvial.get("Instance", {}).get("ID") != player.ranger_unit_id:
                                 current_action_valid = True

                # Augment application (still uses old system for now, as it's complex)
                # This part needs to be mapped to the new system eventually if augments become part of the 4D space
                if player.current_phase == "augment" and player.is_augment_choice_pending:
                    # Allow skip via SYSTEM_ACTION_PASS
                    flat_mask[self.get_new_flat_index(AT_SYSTEM_ACTIONS, 0,0,SYSTEM_ACTION_PASS)] = True

                    # This is a placeholder for mapping old augment actions to new system if needed
                    # For now, we assume augments are handled by their own logic or a separate action type not in this 4D space.
                    # If they were to be included, one of the dimensions (e.g. item_idx for AT_SYSTEM_ACTION or a dedicated AT_APPLY_AUGMENT)
                    # would be used. The current 4D structure doesn't map directly to the old augment logic.
                    # The existing FLAT_IDX_AUGMENT_SKIP and FLAT_IDX_AUGMENT_APPLY_START would need remapping.
                    # This is a simplification for now.
                    pass


                if current_action_valid:
                    flat_mask[i] = True

            # Fallback if mask is all zeros
            if not flat_mask.any():
                if hasattr(self, 'logger') and self.logger: self.logger.warning(f"{mask_debug_prefix} No valid actions found after full check! Enabling Pass action.")
                flat_mask[self.get_new_flat_index(AT_SYSTEM_ACTIONS, 0, 0, SYSTEM_ACTION_PASS)] = True


            if flat_mask.shape[0] != NEW_TOTAL_FLAT_ACTIONS: # Check against new total
                if hasattr(self, 'logger') and self.logger: self.logger.critical(f"[CRITICAL MASK DIM ERROR] Phase {player.current_phase}: Shape {flat_mask.shape}, expected {NEW_TOTAL_FLAT_ACTIONS}. Force Zeros with Pass fallback.")
                flat_mask = _global_np.zeros(NEW_TOTAL_FLAT_ACTIONS, dtype=bool)
                flat_mask[self.get_new_flat_index(AT_SYSTEM_ACTIONS, 0, 0, SYSTEM_ACTION_PASS)] = True
                return flat_mask

            if DETAILED_ACTION_DEBUG and hasattr(self, 'logger') and self.logger: self.logger.debug(f"{mask_debug_prefix} Final mask sum: {_global_np.sum(flat_mask)}")
            return flat_mask

        except Exception as mask_e:
            if hasattr(self, 'logger') and self.logger:
                self.logger.critical(f"[FATAL EXCEPTION action_masks] Error: {mask_e}")
                self.logger.exception("Full traceback for action_masks failure:")
            else:
                print(f"[FATAL EXCEPTION action_masks] Error: {mask_e}")
                import traceback as tb_print
                tb_print.print_exc()

            fallback_mask = _global_np.zeros(NEW_TOTAL_FLAT_ACTIONS, dtype=bool) # Use new total
            try: fallback_mask[self.get_new_flat_index(AT_SYSTEM_ACTIONS, 0, 0, SYSTEM_ACTION_PASS)] = True
            except Exception:
                if NEW_TOTAL_FLAT_ACTIONS > 0: fallback_mask[0] = True
            return fallback_mask


    def _apply_action(self, player: Player, flat_action_index: int) -> float:
        debug_prefix = f"[APPLY P{player.player_id} FlIdx:{flat_action_index}]:"; action_reward = 0.0; original_phase = player.current_phase

        action_type_idx, entity_idx, position_idx, item_idx = self.decode_new_flat_action(flat_action_index)

        # For logging, create a decoded string representation (optional, but helpful for debugging)
        # This can be expanded later to provide more meaningful debug strings per action type.
        decoded_action_str_new = f"AT={action_type_idx},Ent={entity_idx},Pos={position_idx},Item={item_idx}"
        if DETAILED_ACTION_DEBUG:
            self.logger.debug(f"{debug_prefix} Decoded New: {decoded_action_str_new}. Phase='{original_phase}' SpecialShopActive={player.special_shop_active} SpecialShopType='{player.special_shop_type}'")

        try:
            merge_check_needed = False

            # --- Special Shop Active Logic (takes precedence if AT_RANGER_EQUIP is chosen) ---
            if player.special_shop_active and action_type_idx == AT_RANGER_EQUIP:
                # Full logic for AT_RANGER_EQUIP will be placed here in the next step.
                # For now, this correctly calls the new decoding and enters this block.
                if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Processing AT_RANGER_EQUIP (special shop). Item_idx: {item_idx}")
                # Placeholder:
                action_reward += 0.01 # Small reward for attempting
                # Actual equip logic will clear these:
                # player.special_shop_active = False
                # player.special_shop_type = None
                # player.special_shop_items = [None] * 5
                # For now, to test flow, we just return. This will be expanded.
                # self._cumulative_rewards_this_episode["ranger_equip"] += action_reward # Will add this with full logic
                return INVALID_ACTION_PENALTY # Placeholder until full logic implemented

            # If special shop is active, but the action is not AT_RANGER_EQUIP,
            # it might be a system action (like Pass/Skip) or an invalid one.
            if player.special_shop_active and action_type_idx != AT_SYSTEM_ACTIONS:
                if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Action type {action_type_idx} ('{decoded_action_str_new}') is invalid while special shop is active. Only AT_RANGER_EQUIP ({AT_RANGER_EQUIP}) or AT_SYSTEM_ACTIONS ({AT_SYSTEM_ACTIONS}) are allowed. Penalty.")
                return INVALID_ACTION_PENALTY

            # --- Main Action Logic (if special shop is not active or action is AT_SYSTEM_ACTIONS) ---
            if action_type_idx == AT_SYSTEM_ACTIONS:
                if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Processing AT_SYSTEM_ACTIONS with item_idx: {item_idx}")
                # Logic for AT_SYSTEM_ACTIONS will be added in the next diff
                pass

            elif action_type_idx == AT_SELL_UNIT:
                if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Placeholder for AT_SELL_UNIT entity: {entity_idx}")
                pass

            elif action_type_idx == AT_BUY_ILLUVIAL:
                if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Placeholder for AT_BUY_ILLUVIAL item: {item_idx}")
                pass

            elif action_type_idx == AT_PLACE_UNIT:
                if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Placeholder for AT_PLACE_UNIT entity: {entity_idx}, pos: {position_idx}")
                # ... (Logic for AT_PLACE_UNIT will be added in a subsequent step, involves new helper) ...
                pass

            elif action_type_idx == AT_RANGER_BOND:
                if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Placeholder for AT_RANGER_BOND entity: {entity_idx}")
                # ... (Logic for AT_RANGER_BOND will be added in the next diff) ...
                pass
            
            # This case means AT_RANGER_EQUIP was chosen but special_shop_active was False.
            elif action_type_idx == AT_RANGER_EQUIP and not player.special_shop_active:
                 if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} AT_RANGER_EQUIP action invalid because special shop is not active.")
                 return INVALID_ACTION_PENALTY

            else:
                # This 'else' block now primarily catches unmapped new action_type_idx or
                # handles the transition if old augment actions are still being passed.
                # For now, any unmapped action_type_idx is invalid.
                # We also need to consider if flat_action_index itself might be an old augment action.
                # This is complex during transition.
                
                # TEMPORARY BRIDGE: Check if flat_action_index matches old augment system if no new action type was matched.
                # This assumes that the NEW_TOTAL_FLAT_ACTIONS is different enough that old TOTAL_FLAT_ACTIONS
                # indices don't meaningfully overlap with the new ones in a confusing way.
                # This is a fragile assumption and should be removed once augments are fully in new system.
                
                # Safely check if old constants are defined (for robustness during transition)
                _FLAT_IDX_AUGMENT_SKIP = globals().get('FLAT_IDX_AUGMENT_SKIP', -1)
                _FLAT_IDX_AUGMENT_APPLY_START = globals().get('FLAT_IDX_AUGMENT_APPLY_START', -1)
                _FLAT_AUGMENT_APPLY_ACTIONS = globals().get('FLAT_AUGMENT_APPLY_ACTIONS', 0)
                _MAX_COMBINED_TARGET_IDX = globals().get('MAX_COMBINED_TARGET_IDX', 34) # Old default

                is_legacy_augment_skip = (flat_action_index == _FLAT_IDX_AUGMENT_SKIP and _FLAT_IDX_AUGMENT_SKIP != -1)
                is_legacy_augment_apply = (_FLAT_IDX_AUGMENT_APPLY_START != -1 and \
                                           _FLAT_IDX_AUGMENT_APPLY_START <= flat_action_index < (_FLAT_IDX_AUGMENT_APPLY_START + _FLAT_AUGMENT_APPLY_ACTIONS) )

                if is_legacy_augment_skip:
                    if player.is_augment_choice_pending and player.current_phase == "augment":
                        action_reward += SKIP_AUGMENT_REWARD
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Skipped augment (via legacy index). Reward: {action_reward:.3f}")
                        player.is_augment_choice_pending = False; player.available_augment_choices = [None] * NUM_AUGMENT_CHOICES; player.augment_offer_round = -1; player.current_phase = "planning"
                    else: return INVALID_ACTION_PENALTY
                elif is_legacy_augment_apply:
                     if player.is_augment_choice_pending and player.current_phase == "augment":
                        relative_idx_legacy = flat_action_index - _FLAT_IDX_AUGMENT_APPLY_START
                        choice_idx_0based_legacy = relative_idx_legacy // _MAX_COMBINED_TARGET_IDX
                        target_unit_idx_0based_legacy = relative_idx_legacy % _MAX_COMBINED_TARGET_IDX

                        if not (0 <= choice_idx_0based_legacy < len(player.available_augment_choices)): return INVALID_ACTION_PENALTY
                        chosen_aug_name = player.available_augment_choices[choice_idx_0based_legacy]
                        if not chosen_aug_name: return INVALID_ACTION_PENALTY
                        current_combined_units_aug_old = player.get_combined_units()
                        if not (0 <= target_unit_idx_0based_legacy < len(current_combined_units_aug_old)): return INVALID_ACTION_PENALTY
                        target_unit_old, _, _ = self._get_unit_by_combined_index(player, target_unit_idx_0based_legacy + 1)
                        if not target_unit_old: return INVALID_ACTION_PENALTY
                        instance_data_old = target_unit_old.setdefault("Instance", {})
                        equipped_augments_old = instance_data_old.setdefault("EquippedAugments", [])
                        if len(equipped_augments_old) >= MAX_AUGMENTS_PER_UNIT: return INVALID_ACTION_PENALTY
                        aug_data_key_old = f"{chosen_aug_name}_0_Original"
                        aug_data_from_merged_old = self.merged_data.get("Augments", {}).get(aug_data_key_old)
                        if aug_data_from_merged_old is None: return INVALID_ACTION_PENALTY
                        new_augment_old = { "ID": generate_random_unit_id(), "TypeID": { "Name": chosen_aug_name, "Stage": aug_data_from_merged_old.get("Stage", 0), "Variation": aug_data_from_merged_old.get("Variation", "Original") }, "SelectedAbilityIndex": 0 }
                        equipped_augments_old.append(new_augment_old); action_reward += APPLY_AUGMENT_REWARD
                        player.is_augment_choice_pending = False; player.available_augment_choices = [None] * NUM_AUGMENT_CHOICES; player.augment_offer_round = -1; player.current_phase = "planning"
                        if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Applied augment '{chosen_aug_name}' to unit {target_unit_idx_0based_legacy} (via legacy index).")
                     else: return INVALID_ACTION_PENALTY
                else:
                    if DETAILED_ACTION_DEBUG: self.logger.debug(f"{debug_prefix} Unhandled new action_type_idx {action_type_idx} or unmapped legacy flat_action_index {flat_action_index}. Penalty.")
                    return INVALID_ACTION_PENALTY

            if merge_check_needed: # This flag would be set by specific actions like buying an Illuvial
                merge_reward = self._check_and_merge_units(player)
                if merge_reward > 0:
                    action_reward += merge_reward
                    self._cumulative_rewards_this_episode["merge"] += merge_reward

            self._cumulative_rewards_this_episode["action"] += action_reward;
            return action_reward

        except Exception as e_apply:
            # Use action_type_idx if available, else a generic error message
            action_type_info = action_type_idx if 'action_type_idx' in locals() else 'ErrorDecodingOrPreCheck'
            self.logger.critical(f"[FATAL EXCEPTION _apply_action P{player.player_id}] FlIdx:{flat_action_index} Decoded AT={action_type_info} Err:{e_apply}")
            self.logger.exception("Full traceback for _apply_action failure:")
            if player: 
                 player.is_augment_choice_pending = False; # Ensure state consistency
                 if player.special_shop_active: # If error happened during special shop, reset it too
                     player.special_shop_active = False
                     player.special_shop_type = None
                     player.special_shop_items = [None] * 5
                 player.current_phase = "planning";
            return INVALID_ACTION_PENALTY

    def decode_new_flat_action(self, flat_action_index: int) -> Tuple[int, int, int, int]:
        """Decodes a flat index into (action_type_idx, entity_idx, position_idx, item_idx)."""
        item_idx = flat_action_index % DIM_ITEM_SIZE
        temp_idx = flat_action_index // DIM_ITEM_SIZE
        position_idx = temp_idx % DIM_POSITION_SIZE
        temp_idx = temp_idx // DIM_POSITION_SIZE
        entity_idx = temp_idx % DIM_ENTITY_SIZE
        action_type_idx = temp_idx // DIM_ENTITY_SIZE
        return action_type_idx, entity_idx, position_idx, item_idx

    def get_new_flat_index(self, action_type_idx: int, entity_idx: int, position_idx: int, item_idx: int) -> int:
        """Encodes the four components into a flat index."""
        # Validate inputs to prevent out-of-bounds errors during encoding
        if not (0 <= action_type_idx < NUM_ACTION_TYPES): raise ValueError(f"Invalid action_type_idx: {action_type_idx}")
        if not (0 <= entity_idx < DIM_ENTITY_SIZE): raise ValueError(f"Invalid entity_idx: {entity_idx}")
        if not (0 <= position_idx < DIM_POSITION_SIZE): raise ValueError(f"Invalid position_idx: {position_idx}")
        if not (0 <= item_idx < DIM_ITEM_SIZE): raise ValueError(f"Invalid item_idx: {item_idx}")

        flat_idx = action_type_idx
        flat_idx = flat_idx * DIM_ENTITY_SIZE + entity_idx
        flat_idx = flat_idx * DIM_POSITION_SIZE + position_idx
        flat_idx = flat_idx * DIM_ITEM_SIZE + item_idx
        return flat_idx

    def _get_obs(self) -> _global_np.ndarray: return self._get_perspective_observation(0)
    
    def _encode_player_board(self, player: Player) -> _global_np.ndarray:
        player_board_state = _global_np.zeros((MAX_BOARD_SLOTS, FEATURES_PER_UNIT), dtype=_global_np.float32)
        try: sorted_board = sorted(player.board, key=lambda u: self._get_position_id(u.get("Position", None)) or float('inf'))
        except Exception as e_sort:
            if DETAILED_SELFPLAY_DEBUG: self.logger.warning(f"[Env {self.env_id} P{player.player_id}] Error sorting board: {e_sort}. Using unsorted.")
            sorted_board = player.board

        for i, unit in enumerate(sorted_board):
            if i >= MAX_BOARD_SLOTS: break
            try:
                type_id = unit.get("TypeID", {}); instance = unit.get("Instance", {}); position = unit.get("Position")
                illuvial_id = self._get_illuvial_id(type_id.get("LineType")); unified_position_id = self._get_position_id(position)
                augments = instance.get("EquippedAugments", []); augment1_id = 0; augment2_id = 0
                if isinstance(augments, list):
                    if len(augments) > 0 and isinstance(augments[0], dict): augment1_id = self._get_augment_id(augments[0].get("TypeID", {}).get("Name"))
                    if len(augments) > 1 and isinstance(augments[1], dict): augment2_id = self._get_augment_id(augments[1].get("TypeID", {}).get("Name"))
                unit_stage = float(type_id.get("Stage", 1))
                player_board_state[i, 0] = float(illuvial_id); player_board_state[i, 1] = float(unified_position_id)
                player_board_state[i, 2] = float(augment1_id); player_board_state[i, 3] = float(augment2_id)
                player_board_state[i, 4] = unit_stage
            except Exception as e_inner:
                if DETAILED_SELFPLAY_DEBUG: self.logger.error(f"[Env {self.env_id} P{player.player_id}] Error encoding board unit {i}: {e_inner}"); 
                player_board_state[i, :] = 0.0 
                continue
        return player_board_state

    def _encode_player_bench(self, player: Player) -> _global_np.ndarray:
        player_bench_state = _global_np.zeros((BENCH_MAX_SIZE, FEATURES_PER_UNIT), dtype=_global_np.float32)
        for i, unit in enumerate(player.bench):
            if i >= BENCH_MAX_SIZE: break
            try:
                type_id = unit.get("TypeID", {}); instance = unit.get("Instance", {}); unified_position_id = 0 
                illuvial_id = self._get_illuvial_id(type_id.get("LineType"))
                augments = instance.get("EquippedAugments", []); augment1_id = 0; augment2_id = 0
                if isinstance(augments, list):
                    if len(augments) > 0 and isinstance(augments[0], dict): augment1_id = self._get_augment_id(augments[0].get("TypeID", {}).get("Name"))
                    if len(augments) > 1 and isinstance(augments[1], dict): augment2_id = self._get_augment_id(augments[1].get("TypeID", {}).get("Name"))
                unit_stage = float(type_id.get("Stage", 1))
                player_bench_state[i, 0] = float(illuvial_id); player_bench_state[i, 1] = float(unified_position_id)
                player_bench_state[i, 2] = float(augment1_id); player_bench_state[i, 3] = float(augment2_id)
                player_bench_state[i, 4] = unit_stage
            except Exception as e_inner:
                if DETAILED_SELFPLAY_DEBUG: self.logger.error(f"[Env {self.env_id} P{player.player_id}] Error encoding bench unit {i}: {e_inner}"); 
                player_bench_state[i, :] = 0.0
                continue
        return player_bench_state.flatten()

    def _encode_player_shop(self, player: Player) -> _global_np.ndarray:
        shop_state = _global_np.zeros(AGENT_SHOP_FEATURES, dtype=_global_np.float32)
        for i, item in enumerate(player.illuvial_shop):
            if i >= SHOP_SIZE: break
            if item:
                illuvial_id = self._get_illuvial_id(item.get("LineType")); cost = float(get_illuvial_cost(item))
                shop_state[i * 2] = float(illuvial_id); shop_state[i * 2 + 1] = cost
        return shop_state

    def _encode_player_augment_status(self, player: Player) -> _global_np.ndarray:
        aug_state = _global_np.zeros(AGENT_AUGMENT_FEATURES, dtype=_global_np.float32)
        aug_state[0] = 1.0 if player.is_augment_choice_pending else 0.0
        if player.is_augment_choice_pending:
            for i, aug_name in enumerate(player.available_augment_choices):
                 if i >= NUM_AUGMENT_CHOICES: break
                 if aug_name: aug_state[i + 1] = float(self._get_augment_id(aug_name))
        return aug_state

    def _get_perspective_observation(self, pov_player_id: int) -> _global_np.ndarray:
        all_player_board_states = []
        pov_player_instance = self.players.get(pov_player_id)
        if pov_player_instance: all_player_board_states.append(self._encode_player_board(pov_player_instance))
        else: 
            if DETAILED_SELFPLAY_DEBUG: self.logger.warning(f"[Env {self.env_id} _get_obs] POV P{pov_player_id} missing. Using zeros for board."); 
            all_player_board_states.append(_global_np.zeros((MAX_BOARD_SLOTS, FEATURES_PER_UNIT), dtype=_global_np.float32))
        
        other_player_ids = sorted([pid for pid in self.players.keys() if pid != pov_player_id])
        for other_pid in other_player_ids:
             other_player = self.players.get(other_pid);
             if other_player: all_player_board_states.append(self._encode_player_board(other_player))
             else: 
                 if DETAILED_SELFPLAY_DEBUG: self.logger.warning(f"[Env {self.env_id} _get_obs] Other P{other_pid} missing. Using zeros for board."); 
                 all_player_board_states.append(_global_np.zeros((MAX_BOARD_SLOTS, FEATURES_PER_UNIT), dtype=_global_np.float32))
        
        while len(all_player_board_states) < NUM_PLAYERS: 
            if DETAILED_SELFPLAY_DEBUG: self.logger.warning(f"[Env {self.env_id} _get_obs] Padding board obs. Current count: {len(all_player_board_states)}."); 
            all_player_board_states.append(_global_np.zeros((MAX_BOARD_SLOTS, FEATURES_PER_UNIT), dtype=_global_np.float32))
        
        try: board_obs_combined = _global_np.concatenate(all_player_board_states, axis=0).flatten()
        except ValueError as e_concat_board: 
            self.logger.error(f"[Env {self.env_id} _get_obs] Concat board states error: {e_concat_board}"); self.logger.exception("Traceback for board concat:");
            board_obs_combined = _global_np.zeros(TOTAL_BOARD_FEATURES, dtype=_global_np.float32)

        if pov_player_instance:
            agent_scalar_state = _global_np.array([ float(pov_player_instance.health), float(pov_player_instance.coins), float(pov_player_instance.level), float(pov_player_instance.xp), float(self.round_idx) ], dtype=_global_np.float32)
            agent_bench_state = self._encode_player_bench(pov_player_instance)
            agent_shop_state = self._encode_player_shop(pov_player_instance)
            agent_augment_state = self._encode_player_augment_status(pov_player_instance)
        else: 
            agent_scalar_state = _global_np.zeros(AGENT_SCALAR_FEATURES, dtype=_global_np.float32)
            agent_bench_state = _global_np.zeros(AGENT_BENCH_FEATURES, dtype=_global_np.float32)
            agent_shop_state = _global_np.zeros(AGENT_SHOP_FEATURES, dtype=_global_np.float32)
            agent_augment_state = _global_np.zeros(AGENT_AUGMENT_FEATURES, dtype=_global_np.float32)
        agent_specific_obs = _global_np.concatenate([ agent_scalar_state, agent_bench_state, agent_shop_state, agent_augment_state ]).astype(_global_np.float32)

        all_opponent_extra_features = []
        for opp_pid in other_player_ids: 
            opponent = self.players.get(opp_pid)
            if opponent:
                opp_scalar = _global_np.array([ float(opponent.health), float(opponent.coins), float(opponent.level), float(opponent.xp) ], dtype=_global_np.float32)
                opp_bench = self._encode_player_bench(opponent)
                all_opponent_extra_features.extend([opp_scalar, opp_bench])
            else: 
                if DETAILED_SELFPLAY_DEBUG: self.logger.warning(f"[Env {self.env_id} _get_obs] Opponent P{opp_pid} missing for extra features. Padding.");
                all_opponent_extra_features.append(_global_np.zeros(OPPONENT_SCALAR_FEATURES, dtype=_global_np.float32))
                all_opponent_extra_features.append(_global_np.zeros(OPPONENT_BENCH_FEATURES, dtype=_global_np.float32))
        
        try: 
            opponents_extra_obs = _global_np.concatenate(all_opponent_extra_features).astype(_global_np.float32) if all_opponent_extra_features else _global_np.zeros(TOTAL_OPPONENT_EXTRA_FEATURES, dtype=_global_np.float32)
            if opponents_extra_obs.shape[0] != TOTAL_OPPONENT_EXTRA_FEATURES:
                 if DETAILED_SELFPLAY_DEBUG: self.logger.warning(f"[Env {self.env_id} _get_obs] Opponent extra features size mismatch: {opponents_extra_obs.shape[0]} vs {TOTAL_OPPONENT_EXTRA_FEATURES}. Padding/Truncating.")
                 padded_opp = _global_np.zeros(TOTAL_OPPONENT_EXTRA_FEATURES, dtype=_global_np.float32)
                 copy_len = min(opponents_extra_obs.size, TOTAL_OPPONENT_EXTRA_FEATURES)
                 padded_opp[:copy_len] = opponents_extra_obs[:copy_len]; opponents_extra_obs = padded_opp
        except ValueError as e_concat_opp: 
            self.logger.error(f"[Env {self.env_id} _get_obs] Concat opponent extra features error: {e_concat_opp}"); self.logger.exception("Traceback for opponent concat:");
            opponents_extra_obs = _global_np.zeros(TOTAL_OPPONENT_EXTRA_FEATURES, dtype=_global_np.float32)

        try:
             final_obs_list = [ board_obs_combined, agent_specific_obs, opponents_extra_obs ]
             final_obs = _global_np.concatenate(final_obs_list).astype(_global_np.float32)
             if final_obs.shape[0] != FLAT_OBSERVATION_SIZE:
                 self.logger.critical(f"[Env {self.env_id} _get_obs] Observation final shape mismatch: {final_obs.shape}, expected ({FLAT_OBSERVATION_SIZE},). Padding/Truncating.")
                 padded_obs = _global_np.zeros((FLAT_OBSERVATION_SIZE,), dtype=_global_np.float32)
                 copy_len = min(final_obs.size, FLAT_OBSERVATION_SIZE); padded_obs[:copy_len] = final_obs[:copy_len]; final_obs = padded_obs

             if not _global_np.all(_global_np.isfinite(final_obs)):
                  if DETAILED_SELFPLAY_DEBUG: self.logger.warning(f"[Env {self.env_id} _get_obs] Observation created with non-finite values (NaN/inf). Clamping.")
                  high_bound = self.observation_space.high[0] if self.observation_space.high.size > 0 else 200.0
                  low_bound = self.observation_space.low[0] if self.observation_space.low.size > 0 else 0.0
                  final_obs = _global_np.nan_to_num(final_obs, nan=low_bound, posinf=high_bound, neginf=low_bound)
             return final_obs
        except ValueError as e_concat_all: 
            self.logger.error(f"[Env {self.env_id} _get_obs] Final observation concat error: {e_concat_all}"); self.logger.exception("Traceback for final concat:");
            return _global_np.zeros((FLAT_OBSERVATION_SIZE,), dtype=_global_np.float32)
        except Exception as e_obs_unexpected: 
            self.logger.error(f"[Env {self.env_id} _get_obs] Unexpected error assembling observation: {e_obs_unexpected}"); self.logger.exception("Traceback for unexpected obs error:");
            return _global_np.zeros((FLAT_OBSERVATION_SIZE,), dtype=_global_np.float32)
    
    # --- MODIFIED step method with detailed logging and full opponent action logic ---
    def step(self, action: int) -> Tuple[_global_np.ndarray, float, bool, bool, Dict[str, Any]]:
        log_prefix_step_method = f"[Env {self.env_id} Step Rnd {self.round_idx}]"
        _log_debug_step = lambda msg: None 
        _log_info_step = lambda msg: None
        _log_warning_step = lambda msg: None
        _log_error_step = lambda msg: None

        if hasattr(self, 'logger') and self.logger is not None:
            if DETAILED_SELFPLAY_DEBUG or DETAILED_ACTION_DEBUG: 
                _log_debug_step = self.logger.debug
            _log_info_step = self.logger.info
            _log_warning_step = self.logger.warning
            _log_error_step = self.logger.error
        elif DETAILED_SELFPLAY_DEBUG or DETAILED_ACTION_DEBUG:
            _log_debug_step = lambda msg: print(f"{log_prefix_step_method} DEBUG: {msg}", file=sys.__stderr__, flush=True)
            _log_info_step = lambda msg: print(f"{log_prefix_step_method} INFO: {msg}", file=sys.__stderr__, flush=True)
            _log_warning_step = lambda msg: print(f"{log_prefix_step_method} WARN: {msg}", file=sys.__stderr__, flush=True)
            _log_error_step = lambda msg: print(f"{log_prefix_step_method} ERROR: {msg}", file=sys.__stderr__, flush=True)

        _log_debug_step(f"--- STEP START --- Agent P0 Action: {action}")
        terminated = False; truncated = False
        agent_action_reward = 0.0
        sim_reward_change = 0.0
        
        try:
            if self.agent is None:
                _log_warning_step("Agent (self.agent) is None at start of step. Attempting emergency reset.")
                self.reset() 
                if self.agent is None: 
                    _log_error_step("Agent still None after emergency reset. Raising RuntimeError.")
                    raise RuntimeError(f"{log_prefix_step_method} Agent (self.agent) is None even after emergency reset.")

            # --- Special Shop Logic ---
            current_round_for_shop_logic = self.round_idx # Use current round_idx before any potential increment
            
            # Determine if it's a special shop round
            is_weapon_shop_round = (current_round_for_shop_logic == 4)
            is_amp_shop_round = (current_round_for_shop_logic == 11 or current_round_for_shop_logic == 18)

            for p_shop_check_id, p_shop_check in self.players.items():
                if not p_shop_check.is_alive():
                    if p_shop_check.special_shop_active: # Clear shop if player died while shop was active
                        p_shop_check.special_shop_active = False
                        p_shop_check.special_shop_type = None
                        p_shop_check.special_shop_items = [None] * 5
                        _log_debug_step(f"{log_prefix_step_method} P{p_shop_check_id} died, clearing active special shop.")
                    continue

                shop_opened_this_step_for_player = False
                if is_weapon_shop_round:
                    if not p_shop_check.special_shop_active or p_shop_check.special_shop_round_triggered != current_round_for_shop_logic:
                        p_shop_check.special_shop_active = True
                        p_shop_check.special_shop_type = "weapon"
                        p_shop_check.special_shop_round_triggered = current_round_for_shop_logic
                        self._populate_special_shop(p_shop_check, "weapon")
                        shop_opened_this_step_for_player = True
                        _log_info_step(f"{log_prefix_step_method} P{p_shop_check_id} opened WEAPON shop for Round {current_round_for_shop_logic}.")

                elif is_amp_shop_round:
                    if not p_shop_check.special_shop_active or p_shop_check.special_shop_round_triggered != current_round_for_shop_logic:
                        if p_shop_check.ranger_weapon and p_shop_check.ranger_weapon.get('TypeID', {}).get('Name'):
                            p_shop_check.special_shop_active = True
                            p_shop_check.special_shop_type = "weapon_amplifier"
                            p_shop_check.special_shop_round_triggered = current_round_for_shop_logic
                            self._populate_special_shop(p_shop_check, "weapon_amplifier")
                            shop_opened_this_step_for_player = True
                            _log_info_step(f"{log_prefix_step_method} P{p_shop_check_id} opened WEAPON_AMPLIFIER shop for Round {current_round_for_shop_logic} (Weapon: {p_shop_check.ranger_weapon['TypeID']['Name']}).")
                        else:
                            _log_warning_step(f"{log_prefix_step_method} P{p_shop_check_id} cannot open WEAPON_AMPLIFIER shop for Round {current_round_for_shop_logic} - No Ranger weapon equipped.")
                            p_shop_check.special_shop_active = False # Ensure it's off if they can't open it
                            p_shop_check.special_shop_type = None
                            p_shop_check.special_shop_items = [None] * 5

                # Reset shop if it's not a special shop round AND the shop wasn't just opened this step for this player
                # and if the shop was triggered in a *previous* round.
                if not is_weapon_shop_round and not is_amp_shop_round and not shop_opened_this_step_for_player:
                    if p_shop_check.special_shop_active and p_shop_check.special_shop_round_triggered != current_round_for_shop_logic :
                        _log_info_step(f"{log_prefix_step_method} P{p_shop_check_id} clearing old special shop from round {p_shop_check.special_shop_round_triggered} (current round {current_round_for_shop_logic}).")
                        p_shop_check.special_shop_active = False
                        p_shop_check.special_shop_type = None
                        p_shop_check.special_shop_items = [None] * 5
                        # p_shop_check.special_shop_round_triggered = -1 # Keep triggered round to prevent re-opening same round if logic changes

            if self.episode_finished:
                _log_debug_step(f"Episode already finished. Returning last state.")
                obs = self._last_observation if self._last_observation is not None else self._get_obs()
                info = self._last_info.copy() if self._last_info else self._get_info()
                if 'episode' not in info:
                     final_placement = info.get("final_placement", NUM_PLAYERS)
                     ep_rew = sum(self._cumulative_rewards_this_episode.values()) + PLACEMENT_REWARDS.get(final_placement, PLACEMENT_REWARDS[NUM_PLAYERS])
                     info["episode"] = { "r": ep_rew, "l": self.round_idx, "t": time.time(), "w": 1.0 if final_placement == 1 else 0.0 }
                if 'terminal_observation' not in info and obs is not None : 
                    info['terminal_observation'] = deep_convert_np_types(obs.copy(), context='info_step_already_done_obs')
                return obs, 0.0, True, False, info

            offered_augment_this_step = False
            is_augment_round = self.round_idx > 0 and (self.round_idx % AUGMENT_OFFER_FREQUENCY == 0)

            if is_augment_round:
                all_augment_base_names = list(self.augment_name_to_id.keys())
                if all_augment_base_names:
                    for pid, p in self.players.items():
                        if p.is_alive() and not p.is_augment_choice_pending and p.current_phase == "planning":
                            num_to_sample = min(len(all_augment_base_names), NUM_AUGMENT_CHOICES)
                            chosen_augs = random.sample(all_augment_base_names, num_to_sample)
                            p.available_augment_choices = (chosen_augs + [None] * NUM_AUGMENT_CHOICES)[:NUM_AUGMENT_CHOICES]
                            p.is_augment_choice_pending = True; p.augment_offer_round = self.round_idx; p.current_phase = "augment"
                            if DETAILED_ACTION_DEBUG: _log_debug_step(f"Offered augments to P{pid}: {p.available_augment_choices}")
                            if pid == 0: offered_augment_this_step = True
            
            action_decoded_str = decode_flat_action(action)[3]
            action_type_for_agent = decode_flat_action(action)[0] 
            if self.agent and self.agent.player_id == 0: # Ensure agent exists
                agent_phase_before_apply = self.agent.current_phase
                agent_aug_pend_before_apply = self.agent.is_augment_choice_pending
                self.logger.info( 
                    f"[AGENT P0 PRE-APPLY Rnd{self.round_idx}] Phase='{agent_phase_before_apply}', "
                    f"AugPend={agent_aug_pend_before_apply}, "
                    f"ActionTaken={action} (Type:{action_type_for_agent}, Decoded:'{action_decoded_str}')"
                )
            _log_debug_step(f"Agent (P0) applying action: {action} ({action_decoded_str})")
            agent_action_reward = self._apply_action(self.agent, action)
            _log_debug_step(f"Agent action applied. Reward from _apply_action: {agent_action_reward:.4f}")
            if self.agent and self.agent.player_id == 0: # Ensure agent exists
                agent_phase_after_apply = self.agent.current_phase
                agent_aug_pend_after_apply = self.agent.is_augment_choice_pending
                self.logger.info( 
                    f"[AGENT P0 POST-APPLY Rnd{self.round_idx}] Phase='{agent_phase_after_apply}', "
                    f"AugPend={agent_aug_pend_after_apply}, "
                    f"Reward={agent_action_reward:.3f}"
                )
            step_reward = agent_action_reward

            # --- Opponent Actions (Full Original Logic) ---
            opponent_actions_info = {}
            if DETAILED_SELFPLAY_DEBUG: _log_debug_step(f"Processing opponent actions...")
            for pid, opponent in self.players.items():
                if pid == 0 or not opponent.is_alive(): continue
                
                # DEFINE debug_opp_prefix HERE, for each opponent in the loop
                debug_opp_prefix = f"[Opponent P{pid} Env{self.env_id} R{self.round_idx} Ph:{opponent.current_phase[:4]}]" # Added phase for context

                opponent_action = -1 
                opponent_phase_before_action = opponent.current_phase
                action_source_log_info = opponent.opponent_source_type 
                policy_to_use: Optional[BasePolicy] = None
                
                if opponent.is_augment_choice_pending:
                    action_source_log_info = "random_augment_choice"
                    # Make sure any logging inside this block also uses _log_debug_step or similar,
                    # or define a specific prefix if needed.
                    # For now, the main debug_opp_prefix will cover general opponent context.
                    if random.random() < 0.5: opponent_action = FLAT_IDX_AUGMENT_SKIP
                    else:
                         valid_choices_indices = [i + 1 for i, name in enumerate(opponent.available_augment_choices) if name is not None]
                         if valid_choices_indices:
                             chosen_augment_p1 = random.choice(valid_choices_indices)
                             opponent_units = opponent.get_combined_units()
                             # Filter units that can have more augments, ensuring unit_idx_0based is valid for get_flat_augment_apply_index
                             eligible_units_indices = [ 
                                 idx for idx, u in enumerate(opponent_units) 
                                 if len(u.get("Instance", {}).get("EquippedAugments", [])) < MAX_AUGMENTS_PER_UNIT
                                 and idx < MAX_COMBINED_TARGET_IDX # Important check
                             ]
                             if eligible_units_indices:
                                 chosen_unit_p2_0based = random.choice(eligible_units_indices)
                                 try: opponent_action = get_flat_augment_apply_index(chosen_augment_p1, chosen_unit_p2_0based)
                                 except ValueError: 
                                     if DETAILED_SELFPLAY_DEBUG: _log_warning_step(f"{debug_opp_prefix} ValueError for random augment apply. Skipping.")
                                     opponent_action = FLAT_IDX_AUGMENT_SKIP 
                             else: opponent_action = FLAT_IDX_AUGMENT_SKIP
                         else: opponent_action = FLAT_IDX_AUGMENT_SKIP
                    _ = self._apply_action(opponent, opponent_action) # Opponent applies their augment choice
                
                else: # Opponent Planning/Placement Action
                    # debug_opp_prefix is already defined above for this opponent 'pid'
                    
                    if opponent.opponent_source_type == "path":
                        model_path = opponent.opponent_identifier
                        action_source_log_info = f"path({os.path.basename(model_path or 'None')})"
                        if model_path:
                            policy_to_use = self.policy_cache.get(model_path)
                            if not policy_to_use:
                                if DETAILED_SELFPLAY_DEBUG: _log_debug_step(f"{debug_opp_prefix} Path model '{os.path.basename(model_path)}' not in cache. Loading...")
                                policy_to_use = self._load_and_cache_policy(model_path, for_opponent_id=pid)
                            if opponent.opponent_source_type == "random": 
                                action_source_log_info += "->random_fallback(load_fail)"
                                policy_to_use = None
                        else: 
                            opponent.opponent_source_type = "random"; opponent.opponent_identifier = None
                            action_source_log_info = "path->random_fallback(no_id)"
                            policy_to_use = None

                    elif opponent.opponent_source_type == "manager":
                        brain_id = opponent.opponent_identifier
                        action_source_log_info = f"manager({brain_id or 'None'})"
                        if self.opponent_brain_manager and brain_id:
                            policy_to_use = self.opponent_brain_manager.get_brain_policy(brain_id)
                            if not policy_to_use:
                                opponent.opponent_source_type = "random"; opponent.opponent_identifier = None
                                action_source_log_info += "->random_fallback(id_not_found)"
                                # Now this log will work:
                                if DETAILED_SELFPLAY_DEBUG: _log_warning_step(f"{debug_opp_prefix} Brain ID '{brain_id}' not found in manager. Falling back to random.")
                        else:
                            opponent.opponent_source_type = "random"; opponent.opponent_identifier = None
                            action_source_log_info = "manager->random_fallback(no_manager_or_id)"
                            if DETAILED_SELFPLAY_DEBUG: _log_warning_step(f"{debug_opp_prefix} No opponent_brain_manager or no brain_id. Falling back to random.")
                    
                    if policy_to_use and opponent.opponent_source_type != "random":
                        try:
                            predict_on_device = torch.device("cpu") 
                            opp_obs_np = self._get_perspective_observation(pid)
                            opp_obs_tensor = torch.as_tensor(opp_obs_np).unsqueeze(0).to(predict_on_device)
                            
                            original_agent_ref = self.agent; self.agent = opponent; 
                            opp_mask_np = self.action_masks(); 
                            self.agent = original_agent_ref 
                            opp_mask_tensor = torch.as_tensor(opp_mask_np).unsqueeze(0).to(predict_on_device)

                            with torch.no_grad():
                                opponent_action_tensor, _ = policy_to_use.predict(
                                    observation=opp_obs_tensor,
                                    action_masks=opp_mask_tensor,
                                    deterministic=False 
                                )
                            opponent_action = opponent_action_tensor.item()
                            if DETAILED_SELFPLAY_DEBUG: _log_debug_step(f"{debug_opp_prefix} Action from '{action_source_log_info}': {opponent_action} ({decode_flat_action(opponent_action)[3]})") # Added decode
                        except Exception as predict_e:
                            _log_error_step(f"{debug_opp_prefix} Predict exception with '{action_source_log_info}': {predict_e}. Fallback to random.")
                            if hasattr(self,'logger'): self.logger.exception("Predict exception details:")
                            opponent.opponent_source_type = "random"; opponent.opponent_identifier = None
                            action_source_log_info += "->random_fallback(predict_exception)"
                            # Type hint for model_path if it's used below
                            current_model_path_for_opponent: Optional[str] = opponent.opponent_identifier if opponent.opponent_source_type == "path" else None
                            if opponent.opponent_source_type == "path" and current_model_path_for_opponent and current_model_path_for_opponent in self.policy_cache:
                                 if DETAILED_SELFPLAY_DEBUG: _log_debug_step(f"{debug_opp_prefix} Removing problematic path model '{os.path.basename(current_model_path_for_opponent)}' from cache.")
                                 del self.policy_cache[current_model_path_for_opponent]
                    
                    if opponent.opponent_source_type == "random": 
                        if not action_source_log_info.endswith("random") and "fallback" not in action_source_log_info: 
                            action_source_log_info = "random_default"
                        try:
                            original_agent_ref = self.agent; self.agent = opponent; 
                            valid_mask_for_random = self.action_masks(); 
                            self.agent = original_agent_ref
                            valid_indices = _global_np.where(valid_mask_for_random)[0] 
                            if len(valid_indices) > 0: opponent_action = random.choice(valid_indices)
                            else: 
                                if opponent.current_phase == "planning": opponent_action = FLAT_IDX_ENTER_PLACEMENT
                                elif opponent.current_phase == "placement": opponent_action = FLAT_IDX_EXIT_PLACEMENT
                                elif opponent.current_phase == "augment" : opponent_action = FLAT_IDX_AUGMENT_SKIP # If stuck in augment, skip
                                else: opponent_action = FLAT_IDX_ENTER_PLACEMENT # Default if phase unknown
                                if DETAILED_SELFPLAY_DEBUG: _log_warning_step(f"{debug_opp_prefix} Random Opp P{pid} had no valid actions from mask in {opponent.current_phase}! Defaulting to action {opponent_action} ({decode_flat_action(opponent_action)[3]}).")
                                action_source_log_info += "_empty_mask_fallback"
                        except Exception as e_mask_opp_random:
                            _log_error_step(f"{debug_opp_prefix} Random Opponent P{pid} Mask Gen Error: {e_mask_opp_random}. Defaulting action.")
                            if hasattr(self,'logger'): self.logger.exception("Mask gen error details:")
                            opponent_action = FLAT_IDX_ENTER_PLACEMENT if opponent.current_phase == "planning" else \
                                              (FLAT_IDX_EXIT_PLACEMENT if opponent.current_phase == "placement" else \
                                              (FLAT_IDX_AUGMENT_SKIP if opponent.current_phase == "augment" else FLAT_IDX_ENTER_PLACEMENT))
                            action_source_log_info += "_mask_exception_fallback"
                    
                    _ = self._apply_action(opponent, opponent_action) # Opponent applies their planning/placement action
                
                opponent_actions_info[f"P{pid}_flat_action"] = int(opponent_action)
                opponent_actions_info[f"P{pid}_phase_before"] = opponent_phase_before_action
                opponent_actions_info[f"P{pid}_phase_after"] = opponent.current_phase
                opponent_actions_info[f"P{pid}_action_source_details"] = action_source_log_info
            if DETAILED_SELFPLAY_DEBUG: _log_debug_step(f"Opponent actions processing complete.")
            # --- End Opponent Actions ---
            
            any_pending_pre_battle = any(p.is_alive() and p.is_augment_choice_pending for p in self.players.values())
            self._generated_files_this_step = []

            if any_pending_pre_battle:
                self._last_agent_sim_reward = 0.0
                _log_debug_step(f"Augment choices pending for one or more players. Skipping battle/income/XP for this step.")
            else: 
                _log_debug_step(f"No augments pending. Proceeding to end-of-round processing (income, XP, battles).")
                for pid_check, p_check in self.players.items():
                     if p_check.current_phase not in ["planning", "placement", "augment"]:
                         _log_warning_step(f"Forcing P{pid_check} from unexpected phase {p_check.current_phase} back to planning.")
                         p_check.current_phase = "planning"

                if self.round_idx >= 0: # Income and Passive XP
                    income_this_round = INCOME_AMOUNTS[0]
                    if self.round_idx >= INCOME_ROUND_BREAKPOINTS[1]: income_this_round = INCOME_AMOUNTS[2]
                    elif self.round_idx >= INCOME_ROUND_BREAKPOINTS[0]: income_this_round = INCOME_AMOUNTS[1]
                    if DETAILED_ACTION_DEBUG: _log_debug_step(f"Applying income ({income_this_round}) and passive XP ({PASSIVE_XP_PER_ROUND}).")
                    for p_income in self.players.values():
                        if p_income.is_alive():
                            p_income.coins += income_this_round
                            p_income.xp += PASSIVE_XP_PER_ROUND
                            self._check_level_up(p_income)
                    if DETAILED_ACTION_DEBUG: _log_debug_step(f"Income/XP application complete.")
                
                self.round_idx += 1
                _log_info_step(f"--- Advanced to Round {self.round_idx} ---")

                # --- Determine Pairings and Handle Battles (with injected logging) ---
                self._pairs_presimulated_this_step.clear()
                _log_debug_step(f"Calling _determine_pairings().")
                pairings = self._determine_pairings()
                _log_debug_step(f"_determine_pairings() returned {len(pairings)} pairings: {pairings}")
                
                battle_files_generated_this_round = []
                presim_agent_reward_change_this_round = 0.0
                round_damage_value = 1 # Example, use your constant

                if pairings:
                    _log_debug_step(f"Processing {len(pairings)} pairings for Round {self.round_idx}...")
                    for p1_id, p2_id in pairings:
                        current_pair_tuple_sorted = tuple(sorted((p1_id, p2_id)))
                        p1_player = self.players.get(p1_id); p2_player = self.players.get(p2_id)
                        if not p1_player or not p2_player or not p1_player.is_alive() or not p2_player.is_alive():
                             if DETAILED_ACTION_DEBUG: _log_debug_step(f"Skipping pairing ({p1_id}v{p2_id}): One/both players missing/dead.")
                             continue

                        p1_has_units = bool(p1_player.board); p2_has_units = bool(p2_player.board)
                        pre_simulated_outcome = False

                        # --- YOUR PRE-SIMULATION LOGIC ---
                        if p1_has_units and not p2_has_units:
                             p1_player.won_last_round = True; p2_player.won_last_round = False; p2_player.health = max(0, p2_player.health - round_damage_value); pre_simulated_outcome = True
                             if p1_id == 0: presim_agent_reward_change_this_round += ROUND_WIN_REWARD
                             elif p2_id == 0: presim_agent_reward_change_this_round += ROUND_LOSS_REWARD
                             if DETAILED_ACTION_DEBUG: _log_debug_step(f"PRE-SIM: P{p1_id} (units) vs P{p2_id} (no units): P1 wins. P2 Health: {p2_player.health}")
                        elif not p1_has_units and p2_has_units:
                             p2_player.won_last_round = True; p1_player.won_last_round = False; p1_player.health = max(0, p1_player.health - round_damage_value); pre_simulated_outcome = True
                             if p2_id == 0: presim_agent_reward_change_this_round += ROUND_WIN_REWARD
                             elif p1_id == 0: presim_agent_reward_change_this_round += ROUND_LOSS_REWARD
                             if DETAILED_ACTION_DEBUG: _log_debug_step(f"PRE-SIM: P{p1_id} (no units) vs P{p2_id} (units): P2 wins. P1 Health: {p1_player.health}")
                        elif not p1_has_units and not p2_has_units:
                             p1_player.won_last_round = False; p2_player.won_last_round = False; 
                             p1_player.health = max(0, p1_player.health - round_damage_value); p2_player.health = max(0, p2_player.health - round_damage_value); 
                             pre_simulated_outcome = True
                             if p1_id == 0 or p2_id == 0:
                                 self._cumulative_rewards_this_episode["empty_board_penalty"] += MUTUAL_EMPTY_BOARD_PENALTY
                                 presim_agent_reward_change_this_round += MUTUAL_EMPTY_BOARD_PENALTY
                             if DETAILED_ACTION_DEBUG: _log_debug_step(f"PRE-SIM: P{p1_id} vs P{p2_id}: Mutual loss (empty). P1H:{p1_player.health}, P2H:{p2_player.health}")
                        # --- END PRE-SIMULATION LOGIC ---
                        
                        if pre_simulated_outcome:
                            self._pairs_presimulated_this_step.add(current_pair_tuple_sorted)
                        else: # Both have units, need full simulation
                            battle_filename = f"Env{self.env_id}_Round{self.round_idx}_P{p1_id}vP{p2_id}.json";
                            battle_filepath = self._generate_battle_file(p1_player, p2_player, battle_filename); # This will use its internal logging
                            if battle_filepath:
                                battle_files_generated_this_round.append(battle_filepath)
                            else: # Handle failed generation
                                _log_warning_step(f"Battle file gen failed for {battle_filename}. Treating as draw/loss for P{p1_id} & P{p2_id}.")
                                p1_player.won_last_round = False; p2_player.won_last_round = False
                                p1_player.health = max(0, p1_player.health - round_damage_value); p2_player.health = max(0, p2_player.health - round_damage_value)
                                if p1_id == 0 or p2_id == 0:
                                    presim_agent_reward_change_this_round += ROUND_LOSS_REWARD
                                self._pairs_presimulated_this_step.add(current_pair_tuple_sorted)
                    
                    self._generated_files_this_step = battle_files_generated_this_round
                    sim_reward_change = presim_agent_reward_change_this_round
                    self._cumulative_rewards_this_episode["sim_presim_reward"] += sim_reward_change
                    if DETAILED_ACTION_DEBUG and sim_reward_change != 0:
                        _log_debug_step(f"Pre-simulation reward change for agent this round: {sim_reward_change:.3f}")
                else:
                    _log_debug_step(f"No pairings determined for Round {self.round_idx}. Skipping battle file generation.")
                    sim_reward_change = 0.0; self._generated_files_this_step = []
                self._last_agent_sim_reward = sim_reward_change
            
            step_reward += sim_reward_change 
            if DETAILED_ACTION_DEBUG: _log_debug_step(f"Total step reward (Action + Merge + PreSim): {step_reward:.4f}")

            opponents_alive_after_step = sum(1 for pid_term, p_term in self.players.items() if pid_term != 0 and p_term.is_alive());
            agent_is_alive_after_step = self.agent.is_alive();
            any_pending_for_termination_check = any(p.is_alive() and p.is_augment_choice_pending for p in self.players.values())
            if self.agent and self.agent.player_id == 0: # Log only for agent's perspective
                 self.logger.info(
                     f"[AGENT P0 TERMINATION CHECK Rnd{self.round_idx}] AgentAlive={agent_is_alive_after_step}, "
                     f"OppsAlive={opponents_alive_after_step}, "
                     f"AgentAugPend={self.agent.is_augment_choice_pending}, " 
                     f"AnyPlayerAugPendGlobal={any_pending_for_termination_check}" 
                 )
            terminated = (not agent_is_alive_after_step or opponents_alive_after_step == 0) and not any_pending_for_termination_check;
            self.episode_finished = terminated
            
            if DETAILED_ACTION_DEBUG or DETAILED_SELFPLAY_DEBUG: _log_debug_step(f"End of step checks: Agent Alive={agent_is_alive_after_step}, Opponents Alive={opponents_alive_after_step}, Any Aug Pending={any_pending_pre_battle} -> Terminated Flag={terminated}")

            obs = self._get_obs();
            info = self._get_info();
            info["action_reward_immediate"] = agent_action_reward;
            info["presim_reward_this_step"] = sim_reward_change;
            info.update(opponent_actions_info);
            if self.agent: info["agent_augment_pending_at_step_end"] = self.agent.is_augment_choice_pending

            if terminated:
                 final_placement = 1 if agent_is_alive_after_step else opponents_alive_after_step + 1
                 placement_reward = PLACEMENT_REWARDS.get(final_placement, PLACEMENT_REWARDS[NUM_PLAYERS])
                 self._cumulative_rewards_this_episode["placement"] = placement_reward
                 total_episode_reward = sum(v for k,v in self._cumulative_rewards_this_episode.items() if k != "total")
                 self._cumulative_rewards_this_episode["total"] = total_episode_reward
                 
                 info["final_placement"] = final_placement
                 info["placement_reward"] = placement_reward
                 info["agent_won_episode"] = (final_placement == 1)
                 if self.agent:
                      try: 
                          info["final_agent_board_on_terminate"] = deep_convert_np_types(self.agent.board, context='info_terminate')
                          info["final_agent_bench_on_terminate"] = deep_convert_np_types(self.agent.bench, context='info_terminate')
                      except Exception as e_conv_final_board: _log_warning_step(f"Failed deep convert final board/bench for info: {e_conv_final_board}");
                 
                 info["episode"] = { "r": total_episode_reward, "l": self.round_idx, "t": time.time(), "w": 1.0 if final_placement == 1 else 0.0 }
                 if 'terminal_observation' not in info and obs is not None: 
                     info['terminal_observation'] = deep_convert_np_types(obs.copy(), context='info_terminate_obs')
                 _log_info_step(f"Episode End (in step) R{self.round_idx}] Placement: {final_placement}, Total Reward (incl placement): {total_episode_reward:.3f}")

            final_info_for_return = {}
            try:
                final_info_for_return = deep_convert_np_types(info, context='info_step_return')
            except Exception as e_conv_info_step:
                _log_error_step(f"Info Conversion for Return failed: {e_conv_info_step}")
                final_info_for_return = {"error": "info_conversion_failed_in_step", "env_id": self.env_id, "round": self.round_idx}
                try: final_info_for_return.update(info)
                except Exception: pass

            self._last_observation = obs;
            self._last_info = final_info_for_return.copy(); 

            if DETAILED_ACTION_DEBUG or DETAILED_SELFPLAY_DEBUG: _log_debug_step(f"--- STEP END --- Returning Obs Shape: {obs.shape if hasattr(obs,'shape') else 'N/A'}, Step Reward: {step_reward:.4f}, Term: {terminated}, Trunc: {truncated}")
            return obs, step_reward, terminated, truncated, final_info_for_return

        except Exception as e_step_fatal:
            _log_error_fatal_step = self.logger.critical if hasattr(self, 'logger') and self.logger is not None else \
                                   lambda msg: print(f"{log_prefix_step_method} FATAL: {msg}", file=sys.__stderr__, flush=True)
            _log_error_fatal_step(f"Exception in step: {type(e_step_fatal).__name__}: {e_step_fatal}")
            if hasattr(self, 'logger') and self.logger is not None: self.logger.exception("Traceback for fatal step error:")
            else: traceback.print_exc(file=sys.__stderr__)
            
            import numpy as np
            dummy_obs_shape = self.observation_space.shape if hasattr(self,'observation_space') and self.observation_space is not None else (1,)
            dummy_obs_dtype = self.observation_space.dtype if hasattr(self,'observation_space') and self.observation_space is not None else _global_np.float32
            dummy_obs = _global_np.zeros(dummy_obs_shape, dtype=dummy_obs_dtype)
            
            dummy_info = { "error": str(e_step_fatal), "traceback": traceback.format_exc(), "env_id": self.env_id, "round": self.round_idx,
                           "episode": {"r": PLACEMENT_REWARDS[NUM_PLAYERS], "l": self.round_idx, "t": time.time(), "w": 0.0 } }
            if self.agent: self.agent.is_augment_choice_pending = False; self.agent.current_phase = "planning";
            
            self.episode_finished = True; self._last_observation = dummy_obs;
            self._last_info = deep_convert_np_types(dummy_info, context='info_fatal_exception')
            return dummy_obs, 0.0, True, False, self._last_info

    def get_battle_files_and_clear(self) -> List[str]:
        # Loggers for this method
        log_prefix_gbfc = f"[Env {self.env_id} get_battle_files_and_clear Rnd {self.round_idx}]"
        _log_debug_gbfc = lambda msg: None
        if hasattr(self, 'logger') and self.logger is not None:
            if DETAILED_ACTION_DEBUG: _log_debug_gbfc = self.logger.debug
        elif DETAILED_ACTION_DEBUG: 
            _log_debug_gbfc = lambda msg: print(f"{log_prefix_gbfc} DEBUG: {msg}", file=sys.__stderr__, flush=True)

        files = list(self._generated_files_this_step)
        self._generated_files_this_step.clear()
        if files: _log_debug_gbfc(f"Returning {len(files)} files.")
        else: _log_debug_gbfc(f"Returning 0 files.")
        return files


    # In train_env.py
# Inside IlluviumSingleEnv class, locate the apply_all_simulation_results method.
# Replace the entire existing method with this:

    def apply_all_simulation_results(self, results_dict: Dict[str, Dict[str, Any]]):
        debug_prefix = f"[Env {self.env_id} ApplySimResults Rnd {self.round_idx}]"
        # Using self.logger for consistency as recommended
        _log_debug_asr = self.logger.debug if hasattr(self, 'logger') and self.logger is not None and DETAILED_SELFPLAY_DEBUG else (lambda msg: None)
        _log_info_asr = self.logger.info if hasattr(self, 'logger') and self.logger is not None else (lambda msg: None)
        _log_warning_asr = self.logger.warning if hasattr(self.logger, 'warning') and self.logger is not None else (lambda msg: None)
        _log_error_asr = self.logger.error if hasattr(self.logger, 'error') and self.logger is not None else (lambda msg: None)

        if self.agent is None: 
            _log_error_asr("Agent is None. Cannot apply sim results.")
            return

        agent_sim_reward_from_files = 0.0
        sim_errors = results_dict.pop('__error__', None)
        if sim_errors: _log_warning_asr(f"Sim runner reported errors: {sim_errors}")

        _log_debug_asr(f"Applying results for {len(results_dict)} battles.")
        for fname_base, data in results_dict.items():
            # Ensure 're' is correctly imported at the top of train_env.py (it should be by default)
            match=re.search(r"_P(\d+)vP(\d+)", fname_base)
            if not match: 
                _log_warning_asr(f"Filename '{fname_base}' does not match battle pattern. Skipping.")
                continue
            try: p1_id, p2_id = map(int, match.groups())
            except ValueError: 
                _log_error_asr(f"Could not parse player IDs from filename '{fname_base}'. Skipping.")
                continue
            
            current_pair_sorted = tuple(_global_np.sort((p1_id, p2_id))) # Use _global_np.sort for consistency
            if current_pair_sorted in self._pairs_presimulated_this_step: 
                _log_debug_asr(f"Pair ({p1_id},{p2_id}) already presimulated. Skipping full sim result application.")
                continue
            
            p1, p2 = self.players.get(p1_id), self.players.get(p2_id)
            if not p1 or not p2 or not p1.is_alive() or not p2.is_alive(): 
                _log_debug_asr(f"Skipping result for ({p1_id}v{p2_id}): one or both players not alive or missing.")
                continue

            dmg = 1; p1_lost, p2_lost = False, False; sim_rew_p1 = 0.0; winner = data.get("WinningTeam")
            if winner == "Red": p1_lost = True; sim_rew_p1 = ROUND_LOSS_REWARD;
            elif winner == "Blue": p2_lost = True; sim_rew_p1 = ROUND_WIN_REWARD;
            else: # Draw or unexpected outcome (both lose or no clear winner)
                p1_lost, p2_lost = True, True; sim_rew_p1 = ROUND_LOSS_REWARD; # Treat as loss for P1's perspective
                _log_debug_asr(f"Battle {fname_base}: No clear winner ('{winner}'). Treating as mutual loss for damage and P1's reward.")
            
            if p1_lost: p1.health = max(0, p1.health - dmg); p1.won_last_round = False
            else: p1.won_last_round = True
            if p2_lost: p2.health = max(0, p2.health - dmg); p2.won_last_round = False
            else: p2.won_last_round = True

            _log_debug_asr(f"Battle {fname_base}: Winner '{winner}'. P{p1_id} Health: {p1.health}, P{p2_id} Health: {p2.health}.")
            
            if self.agent.player_id == p1_id: 
                agent_sim_reward_from_files += sim_rew_p1; 
                self.agent.won_last_round = not p1_lost
                _log_debug_asr(f"Agent (P{p1_id}) was P1. Reward adjustment: {sim_rew_p1:.3f}")
            elif self.agent.player_id == p2_id: 
                agent_sim_reward_from_files += (-sim_rew_p1); # P2's reward is inverse of P1's
                self.agent.won_last_round = not p2_lost
                _log_debug_asr(f"Agent (P{p2_id}) was P2. Reward adjustment: {-sim_rew_p1:.3f}")
        
        self._last_agent_sim_reward += agent_sim_reward_from_files # Add to any pre-sim rewards from step
        self._cumulative_rewards_this_episode["sim_files_reward"] += agent_sim_reward_from_files
        _log_info_asr(f"Applied {len(results_dict)} battle file results. Agent total sim reward from files this step: {agent_sim_reward_from_files:.3f}")

        # --- CRITICAL FIX START: Force opponent config refresh if needed ---
        # This checks if an opponent is configured to use a brain that's not in the manager
        # and triggers a full reassignment from the current pool.
        
        manager_instance = self.opponent_brain_manager
        if manager_instance and hasattr(manager_instance, 'get_available_brain_ids'):
            available_brain_ids_in_manager = manager_instance.get_available_brain_ids()
            
            needs_reassignment_check = False
            for pid, p_obj in self.players.items():
                if pid == 0 or not p_obj.is_alive(): continue
                if p_obj.opponent_source_type == "manager" and p_obj.opponent_identifier is not None:
                    # Check if the specific brain ID is NOT in the manager's current list
                    if p_obj.opponent_identifier not in available_brain_ids_in_manager:
                        _log_warning_asr(f"{debug_prefix} P{pid} configured with brain ID '{p_obj.opponent_identifier}' which is NOT in manager's available list. Triggering reassignment for all opponents in this env.")
                        needs_reassignment_check = True
                        break # Found one, so trigger a full reassignment.

            if needs_reassignment_check:
                _log_info_asr(f"{debug_prefix} Triggering in-env opponent config refresh due to missing brain ID.")
                opponent_player_ids = [p_id for p_id, p_obj in self.players.items() if p_id != 0 and p_obj.is_alive()]
                
                opponent_configs_to_set: Dict[int, Tuple[str, Optional[str]]] = {} 
                for p_id in opponent_player_ids:
                    if not available_brain_ids_in_manager: # This should ideally not happen if manager is populated
                        selected_brain_id = None # Fallback to random identifier
                        _log_error_asr(f"{debug_prefix} No brains available in manager for P{p_id} for in-env refresh, falling back to random (this should not happen).")
                        opponent_configs_to_set[p_id] = ("random", None)
                    else:
                        # Choose a brain from the available pool (random.choice is sufficient for this defensive reassignment)
                        selected_brain_id = random.choice(available_brain_ids_in_manager)
                        opponent_configs_to_set[p_id] = ("manager", selected_brain_id)
                    
                    _log_debug_asr(f"{debug_prefix}   -> In-env refresh: P{p_id} assigned '{selected_brain_id}'")
                    # Apply the new config immediately to the player object in THIS env
                    # This will also trigger the logging in Player.set_opponent_source
                    self.set_opponent_config(p_id, opponent_configs_to_set[p_id][0], opponent_configs_to_set[p_id][1])
            else:
                 _log_debug_asr(f"{debug_prefix} No immediate in-env opponent config refresh needed.")

        # --- CRITICAL FIX END ---

        # Replaced np.sum and np.array with _global_np.sum and _global_np.array
        opp_alive = _global_np.sum(1 for pid, p in self.players.items() if pid != 0 and p.is_alive()) 
        agent_alive = self.agent.is_alive()
        any_aug_pending_after_sim = any(p.is_alive() and p.is_augment_choice_pending for p in self.players.values())
        if self.agent and self.agent.player_id == 0: # Log only for agent's perspective
             _log_info_asr( 
                 f"[AGENT P0 TERMINATION CHECK POST-SIM Rnd{self.round_idx}] AgentAlive={agent_alive}, "
                 f"OppsAlive={opp_alive}, "
                 f"AgentAugPend={self.agent.is_augment_choice_pending}, " 
                 f"AnyPlayerAugPendGlobal={any_aug_pending_after_sim}" 
             )
        new_terminated_after_sim = (not agent_alive or opp_alive == 0) and not any_aug_pending_after_sim
        
        if not self.episode_finished and new_terminated_after_sim: 
            self.episode_finished = True
            _log_info_asr(f"Episode End R{self.round_idx} (Post-Full-Sim): Termination detected based on sim results.")
            
            if self._last_info is None: self._last_info = {}
            final_placement = 1 if agent_alive else opp_alive + 1
            placement_reward = PLACEMENT_REWARDS.get(final_placement, PLACEMENT_REWARDS[NUM_PLAYERS])
            self._cumulative_rewards_this_episode["placement"] = placement_reward
            total_ep_rew = _global_np.sum(v for k,v in self._cumulative_rewards_this_episode.items() if k != "total") 
            self._cumulative_rewards_this_episode["total"] = total_ep_rew
            
            self._last_info.update({
                "final_placement": final_placement, "placement_reward": placement_reward,
                "agent_won_episode": (final_placement == 1),
                "episode": { "r": total_ep_rew, "l": self.round_idx, "t": time.time(), "w": 1.0 if final_placement == 1 else 0.0 }
            })
            if self.agent:
                 try: 
                     self._last_info["final_agent_board"] = deep_convert_np_types(self.agent.board, context='info_apply_sim_term_board')
                     self._last_info["final_agent_bench"] = deep_convert_np_types(self.agent.bench, context='info_apply_sim_term_bench')
                 except Exception as e_fboard: _log_warning_asr(f"Failed deep convert final board/bench for info: {e_fboard}");
            if 'terminal_observation' not in self._last_info and self._last_observation is not None: 
                self._last_info['terminal_observation'] = deep_convert_np_types(self._last_observation.copy(), context='info_apply_sim_term_obs')
        
        self.episode_finished = new_terminated_after_sim 
        _log_debug_asr(f"Apply sim results finished. Agent Sim Rew (Files): {agent_sim_reward_from_files:.3f}. EpisodeFinished: {self.episode_finished}")

    def _generate_battle_file(self, p1: Player, p2: Player, filename: str) -> Optional[str]:
        log_prefix = f"[Env {self.env_id} Rnd {self.round_idx} _generate_battle_file]"
        _log_debug = _log_info = _log_warning = _log_error = lambda msg: None 
        if hasattr(self, 'logger') and self.logger is not None:
            if DETAILED_SELFPLAY_DEBUG: _log_debug = self.logger.debug
            _log_info = self.logger.info
            _log_warning = self.logger.warning
            _log_error = self.logger.error
        elif DETAILED_SELFPLAY_DEBUG: 
            _log_debug = lambda msg: print(f"{log_prefix} DEBUG: {msg}", file=sys.__stderr__, flush=True)
            _log_info = lambda msg: print(f"{log_prefix} INFO: {msg}", file=sys.__stderr__, flush=True)
            _log_warning = lambda msg: print(f"{log_prefix} WARN: {msg}", file=sys.__stderr__, flush=True)
            _log_error = lambda msg: print(f"{log_prefix} ERROR: {msg}", file=sys.__stderr__, flush=True)

        _log_debug(f"Attempting to generate file '{filename}' for P{p1.player_id} vs P{p2.player_id}.")
        _log_debug(f"P{p1.player_id} total units in board list: {len(p1.board)}, P{p2.player_id} total units in board list: {len(p2.board)}.")

        p1_units_on_board = [u for u in p1.board if u.get("Position")]
        p2_units_on_board = [u for u in p2.board if u.get("Position")]
        _log_debug(f"P{p1.player_id} units with valid Position: {len(p1_units_on_board)}, P{p2.player_id} units with valid Position: {len(p2_units_on_board)}.")

        if not p1_units_on_board or not p2_units_on_board:
            _log_info(f"One or both players ({p1.player_id},{p2.player_id}) have no units with positions on board. Skipping battle file generation for '{filename}'.")
            return None

        valid_units_for_sim = []
        for unit in p1_units_on_board:
            try:
                converted_unit = deep_convert_np_types(unit, context='battle_file_p1_unit')
                converted_unit["Team"] = "Blue"
                valid_units_for_sim.append(converted_unit)
            except Exception as e_conv:
                _log_error(f"Error converting P1 unit {unit.get('Instance',{}).get('ID','UnknownID')} for battle file: {e_conv}")
                if hasattr(self, 'logger'): self.logger.exception("P1 unit conversion traceback:")
                continue
        
        for unit in p2_units_on_board:
            try:
                converted_unit = deep_convert_np_types(unit, context='battle_file_p2_unit')
                blue_pos_dict = converted_unit.get("Position")
                if blue_pos_dict:
                    blue_unified_id = BLUE_POS_TO_UNIFIED_ID_MAP.get((blue_pos_dict.get("Q"), blue_pos_dict.get("R")))
                    if blue_unified_id and blue_unified_id in UNIFIED_ID_TO_RED_POS_MAP:
                        red_pos_tuple = UNIFIED_ID_TO_RED_POS_MAP[blue_unified_id]
                        converted_unit["Position"] = {"Q": red_pos_tuple[0], "R": red_pos_tuple[1]}
                    else:
                        _log_error(f"Could not map P2 unit position for {unit.get('Instance',{}).get('ID','UnknownID')}. Original Blue Pos: {blue_pos_dict}")
                        converted_unit["Position"] = {"Q": 0, "R": 0} 
                converted_unit["Team"] = "Red"
                valid_units_for_sim.append(converted_unit)
            except Exception as e_conv_p2:
                _log_error(f"Error converting P2 unit {unit.get('Instance',{}).get('ID','UnknownID')} for battle file: {e_conv_p2}")
                if hasattr(self, 'logger'): self.logger.exception("P2 unit conversion traceback:")
                continue

        if not valid_units_for_sim:
            _log_warning(f"No valid units for simulation after processing for P{p1.player_id} vs P{p2.player_id}. Skipping '{filename}'.")
            return None

        file_path_to_write = os.path.join(BATTLE_DIR, filename) 
        try:
            os.makedirs(BATTLE_DIR, exist_ok=True) 
            with open(file_path_to_write, "w", encoding="utf-8") as f:
                json.dump({"Version": 20, "CombatUnits": valid_units_for_sim}, f, indent=4)
            _log_info(f"Successfully generated battle file: {filename}")
            return file_path_to_write
        except Exception as e_gen_file:
            _log_error(f"Failed to write battle file {filename}: {e_gen_file}")
            if hasattr(self, 'logger') and self.logger is not None : self.logger.error(traceback.format_exc())
            else: traceback.print_exc(file=sys.__stderr__)
            return None

    def _get_info(self) -> Dict[str, Any]:
        if self.agent is None: return {"env_id": self.env_id, "status": "agent_none_in_getinfo", "round": self.round_idx}
        agent_synergies = self._calculate_synergies(self.agent)
        info = {
            "env_id": self.env_id, "agent_player_id": self.agent.player_id, "round": self.round_idx,
            "agent_coins": self.agent.coins, "agent_health": self.agent.health, 
            "agent_level": self.agent.level, "agent_xp": self.agent.xp,
            "agent_board_units": self.agent.units_on_board_count(), "agent_bench_units": len(self.agent.bench),
            "agent_phase": self.agent.current_phase, "agent_augment_pending": self.agent.is_augment_choice_pending,
            "agent_won_last_round": self.agent.won_last_round, 
            "current_round_total_sim_reward_for_agent": self._last_agent_sim_reward,
            "agent_total_synergies": agent_synergies,
            "opponents_alive": sum(1 for pid,p in self.players.items() if pid!=0 and p.is_alive()),
            **{f"P{pid}_health": p.health for pid, p in self.players.items()}
        }
        if DETAILED_SELFPLAY_DEBUG: # Add detailed opponent config info
            info["opponent_configurations_detailed"] = {
                f"P{pid}": { "source": p_obj.opponent_source_type, 
                              "id": (os.path.basename(p_obj.opponent_identifier) if p_obj.opponent_source_type=="path" and p_obj.opponent_identifier else p_obj.opponent_identifier) or "None",
                              "alive": p_obj.is_alive() } 
                for pid, p_obj in self.players.items() if pid != 0
            }
            info["num_path_cached_policies_in_env"] = len(self.policy_cache)
            if self.opponent_brain_manager and hasattr(self.opponent_brain_manager, 'get_available_brain_ids'):
                try: info["num_brains_in_external_manager"] = len(self.opponent_brain_manager.get_available_brain_ids())
                except Exception: info["num_brains_in_external_manager"] = "N/A (Error getting count)"
        
        if self.episode_finished:
            if "episode" not in info:
                final_placement_info = info.get("final_placement")
                if final_placement_info is None:
                    final_placement_info = 1 if self.agent.is_alive() else (sum(1 for pid,p in self.players.items() if pid!=0 and p.is_alive()) + 1)
                
                ep_rew_info = sum(self._cumulative_rewards_this_episode.values())
                info["episode"] = {"r": ep_rew_info, "l": self.round_idx, "t": time.time(), "w": 1.0 if final_placement_info == 1 else 0.0}
            
            if self._last_info:
                for key in ["final_placement", "placement_reward", "agent_won_episode", "terminal_observation"]:
                    if key in self._last_info and key not in info : info[key] = self._last_info[key]
        return info

    def _calculate_synergies(self, player: Player) -> Dict[str, int]:
        affinity_counts = Counter(); class_counts = Counter(); active_synergies = defaultdict(int)
        for unit in player.board:
            try:
                instance = unit.get("Instance", {}); affinity = instance.get("DominantCombatAffinity"); combat_class = instance.get("DominantCombatClass")
                if affinity: affinity_counts[affinity] += 1
                if combat_class: class_counts[combat_class] += 1
            except Exception: continue
        for affinity, count in affinity_counts.items():
            if count >= 2: active_synergies[f"Affinity_{affinity}"] = count
        for combat_class, count in class_counts.items():
            if count >= 2: active_synergies[f"Class_{combat_class}"] = count
        return dict(active_synergies)

    def render(self):
        if self.render_mode == "none" or self.agent is None: return
        aug_pend_str = " [AUG PEND]" if self.agent.is_augment_choice_pending else ""
        agent_units_str = f"B:{self.agent.units_on_board_count()}/{self.agent.level} Be:{len(self.agent.bench)}/{BENCH_MAX_SIZE}"
        agent_status_str = f"A(H:{self.agent.health} C:{self.agent.coins} L:{self.agent.level} XP:{self.agent.xp} {agent_units_str} Ph:{self.agent.current_phase}{aug_pend_str})"
        
        opp_parts = []
        for pid, p_obj in self.players.items():
            if pid == 0 or not p_obj.is_alive(): continue
            id_disp = p_obj.opponent_identifier
            if p_obj.opponent_source_type == "path" and id_disp: id_disp = os.path.basename(id_disp)
            opp_parts.append(f"P{pid}(H:{p_obj.health} L:{p_obj.level} S:{p_obj.opponent_source_type[0]}:{ (id_disp if id_disp else 'N')[:8] })")
        
        opp_summary_str = f"Opps({len(opp_parts)}):[{','.join(opp_parts)}]"
        output_str = f"E{self.env_id} R{self.round_idx}|{agent_status_str}|{opp_summary_str}"
        
        if self.episode_finished:
            status_str = "WON" if self.agent.is_alive() else ("LOST" if not self.agent.is_alive() else "UNKNOWN")
            plc_r = self._last_info.get('final_placement','?') if self._last_info else self._get_info().get('final_placement','?')
            rew_r = (self._last_info.get('episode',{}).get('r','?')) if self._last_info else (self._get_info().get('episode',{}).get('r','?'))
            rew_r_str = f"{rew_r:.2f}" if isinstance(rew_r, (float, _global_np.floating)) else str(rew_r)
            output_str += f"|DONE(Plc:{plc_r} Status:{status_str} Rew:{rew_r_str})"
        
        if self.render_mode in ["human","ansi"]: print(output_str)

    def close(self):
        if DETAILED_ACTION_DEBUG or DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"[Env {self.env_id} close] Closing environment instance.")
        if hasattr(self, 'policy_cache'):
            if DETAILED_SELFPLAY_DEBUG and self.policy_cache: self.logger.debug(f"[Env {self.env_id} close] Clearing path-based policy_cache ({len(self.policy_cache)} entries).")
            self.policy_cache.clear()
        pass

    def get_final_step_data(self) -> Tuple[float, bool, Dict[str, Any]]:
        """
        Called after the final simulation results are applied (if any), right before the
        environment might be reset. Provides a chance to finalize rewards/info based
        on the absolute final state.
        """
        if self.agent is None: 
            self.logger.error("Agent not initialized in get_final_step_data. This indicates a serious error.")
            err_info = {"error": "agent not initialized in get_final_step_data", "env_id": self.env_id,
                        "episode": {"r": 0.0, "l": self.round_idx, "t": time.time(), "w": 0.0}} # Default episode info
            return 0.0, True, err_info

        final_done = self.episode_finished 
        final_info = self._last_info.copy() if self._last_info else self._get_info()
        final_reward_adjustment = 0.0

        if final_done:
            current_placement = final_info.get("final_placement")
            if current_placement is None:
                agent_is_alive_final = self.agent.is_alive()
                opponents_alive_final = sum(1 for pid, p in self.players.items() if pid != 0 and p.is_alive())
                current_placement = 1 if agent_is_alive_final else opponents_alive_final + 1
                final_info["final_placement"] = current_placement

            current_placement_reward = final_info.get("placement_reward")
            if current_placement_reward is None:
                current_placement_reward = PLACEMENT_REWARDS.get(current_placement, PLACEMENT_REWARDS[NUM_PLAYERS])
                final_info["placement_reward"] = current_placement_reward
            
            if "placement" not in self._cumulative_rewards_this_episode or \
               not _global_np.isclose(self._cumulative_rewards_this_episode.get("placement", 0.0), current_placement_reward):
                self._cumulative_rewards_this_episode["placement"] = current_placement_reward
            
            expected_total_reward = sum(v for k,v in self._cumulative_rewards_this_episode.items() if k != "total")
            
            current_episode_info = final_info.get("episode", {})
            if not current_episode_info or not _global_np.isclose(current_episode_info.get("r", -_global_np.inf), expected_total_reward):
                 final_info["episode"] = { 
                     "r": expected_total_reward, "l": self.round_idx, 
                     "t": time.time(), "w": 1.0 if current_placement == 1 else 0.0 
                 }
                 if DETAILED_SELFPLAY_DEBUG and (not current_episode_info or not _global_np.isclose(current_episode_info.get("r", -_global_np.inf), expected_total_reward)):
                     self.logger.debug(f"[Env {self.env_id} FinalData] Updated/Ensured episode info in final_info: r={expected_total_reward:.3f}, l={self.round_idx}")
            
            if "agent_won_episode" not in final_info: final_info["agent_won_episode"] = (current_placement == 1)
            if self.agent:
                if "final_agent_board" not in final_info: 
                    try: final_info["final_agent_board"] = deep_convert_np_types(self.agent.board, context='info_final_data_board')
                    except Exception as e_fboard: self.logger.warning(f"[Env {self.env_id} FinalData] Failed deep convert board for final_info: {e_fboard}");
                if "final_agent_bench" not in final_info:
                    try: final_info["final_agent_bench"] = deep_convert_np_types(self.agent.bench, context='info_final_data_bench')
                    except Exception as e_fbench: self.logger.warning(f"[Env {self.env_id} FinalData] Failed deep convert bench for final_info: {e_fbench}");
            
            if 'terminal_observation' not in final_info and self._last_observation is not None: 
                final_info["terminal_observation"] = deep_convert_np_types(self._last_observation.copy(), context='info_final_data_obs')
        else: # Not done, but SB3 might still expect 'episode' if it's polling final data for some reason
            if "episode" not in final_info:
                final_info["episode"] = { "r": sum(self._cumulative_rewards_this_episode.values()), "l": self.round_idx, "t": time.time(), "w": 0.0}

        try: final_info_converted = deep_convert_np_types(final_info, context='info_final_data_return')
        except Exception as e_conv_final_get: 
            self.logger.error(f"[Env {self.env_id} CONVERT FINAL_INFO in get_final_step_data] {e_conv_final_get}"); 
            self.logger.exception("Traceback for final info conversion failure:")
            final_info_converted = {"error": "info_conversion_failed_in_get_final_data", "env_id": self.env_id}
            if "episode" not in final_info_converted and final_done: 
                final_info_converted["episode"] = final_info.get("episode", {"r": 0.0, "l": self.round_idx, "t": time.time(), "w": 0.0})
            final_info_converted.update(final_info)
        
        return final_reward_adjustment, final_done, final_info_converted

    def set_opponent_config(self, opponent_id: int, source_type: str, identifier: Optional[str]):
        if not (0 < opponent_id < NUM_PLAYERS and opponent_id in self.players):
            self.logger.warning(f"set_opponent_config: Invalid opponent_id {opponent_id}. Not in active players or is agent ID.")
            return

        player = self.players[opponent_id]
        # Use the Player's set_opponent_source helper
        player.set_opponent_source(source_type, identifier) # This will log the change automatically based on DETAILED_SELFPLAY_DEBUG

        # Update current_opponent_configs internal dictionary
        # This dictionary holds the *last* known configuration for an opponent
        # received from the main process, important for resets.
        self.current_opponent_configs[opponent_id] = (source_type, identifier)

        # Proactive loading for "path" type opponents if specified
        if DETAILED_SELFPLAY_DEBUG:
            identifier_log_name = os.path.basename(identifier) if source_type == "path" and identifier else identifier
            self.logger.debug(f"SetOpponentCfg P{opponent_id}: Storing config (in env): src='{source_type}', id='{identifier_log_name or 'None'}'.")

        if source_type == "path" and identifier: 
            if identifier not in self.policy_cache:
                if DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"SetOpponentCfg P{opponent_id}: Path model '{os.path.basename(identifier)}' not in cache. Triggering proactive load.")
                self._load_and_cache_policy(identifier, for_opponent_id=opponent_id)
            elif DETAILED_SELFPLAY_DEBUG: self.logger.debug(f"SetOpponentCfg P{opponent_id}: Path model '{os.path.basename(identifier)}' already in cache.")

    def set_opponent_configs_dict(self, configs_by_opp_id: Dict[int, Tuple[str, Optional[str]]]):
        if DETAILED_SELFPLAY_DEBUG:
            updates_log_summary = { k: (st, os.path.basename(ident) if ident and st=="path" else (ident if ident else "None")) 
                                    for k, (st, ident) in configs_by_opp_id.items() if k != 0 }
            self.logger.debug(f"SetOpponentCfgsDict: Received update for {len(updates_log_summary)} opponents: {updates_log_summary}.")

        for opponent_id, config_tuple in configs_by_opp_id.items():
            if opponent_id == 0: continue 
            source_type, identifier = config_tuple
            self.set_opponent_config(opponent_id, source_type, identifier)
        
        if DETAILED_SELFPLAY_DEBUG: # Log cache and manager stats after update
            self.logger.debug(f"SetOpponentCfgsDict: Policy cache size: {len(self.policy_cache)}")
            if self.opponent_brain_manager and hasattr(self.opponent_brain_manager, 'get_available_brain_ids'):
                try: self.logger.debug(f"SetOpponentCfgsDict: External brain manager reports {len(self.opponent_brain_manager.get_available_brain_ids())} brains available.")
                except Exception as e: self.logger.warning(f"Error getting brain count from manager: {e}")

    def get_battle_files_and_clear(self) -> List[str]:
        log_prefix_gbfc = f"[Env {self.env_id} get_battle_files_and_clear Rnd {self.round_idx}]"
        _log_debug_gbfc = lambda msg: None
        if hasattr(self, 'logger') and self.logger is not None:
            if DETAILED_ACTION_DEBUG: _log_debug_gbfc = self.logger.debug
        elif DETAILED_ACTION_DEBUG: 
            _log_debug_gbfc = lambda msg: print(f"{log_prefix_gbfc} DEBUG: {msg}", file=sys.__stderr__, flush=True)

        files = list(self._generated_files_this_step)
        self._generated_files_this_step.clear()
        if files: _log_debug_gbfc(f"Returning {len(files)} files.")
        else: _log_debug_gbfc(f"Returning 0 files.")
        return files

[end of train_env.py]
