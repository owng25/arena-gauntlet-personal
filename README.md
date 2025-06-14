## How to run
1. Clone repository
2. You may have to build the simulation cli according to the insutrctions on the simulatin readme
3. run training with `python3 train_agent.py --num-envs 12`. 12 is stable on a 24 GB RAM Mac, but can be adjusted. 
4. run `tensorboard --logdir ./logs` to check logs

## Action Space and Masking
The environment utilizes a flattened, discrete action space managed by gym.spaces.Discrete. This design maps every potential granular action (selling a specific unit, buying from a shop slot, moving a unit, etc.) to a unique integer index.

### Action Space Structure
* Total Size: 608 actions (determined by configuration constants MAX_BOARD_SLOTS, BENCH_MAX_SIZE, SHOP_SIZE, NUM_AUGMENT_CHOICES).

* Blocks: The action space (indices 0-607) is structured as follows:
  * Sell (Indices 0-24): Sell unit at combined board/bench index 0 through 24. (Size: MAX_BOARD_SLOTS + BENCH_MAX_SIZE = 25)
  * Buy (Indices 25-27): Buy from shop slot 0, 1, or 2. (Size: SHOP_SIZE = 3)
  * Reroll (Index 28): Reroll the shop. (Size: 1)
  * Buy XP (Index 29): Buy experience points. (Size: 1)
  * Enter Placement (Index 30): Switch to the placement phase. (Size: 1)
  * Modify Unit (Indices 31-530): Move/Swap any unit (board/bench) to any board position or the bench. (Size: (MAX_BOARD_SLOTS + BENCH_MAX_SIZE) * (MAX_BOARD_SLOTS + 1) = 25 * 20 = 500)
  * Exit Placement (Index 531): Switch back to the planning phase. (Size: 1)
  * Augment Skip (Index 532): Choose to skip the current augment selection. (Size: 1)
  * Augment Apply (Indices 533-607): Apply offered augment 1, 2, or 3 to unit at combined index 0 through 24. (Size: NUM_AUGMENT_CHOICES * (MAX_BOARD_SLOTS + BENCH_MAX_SIZE) = 3 * 25 = 75)

### Action Masking
To ensure the agent only selects valid actions based on the current game state, the environment provides an action_masks() method. This method is used by compatible algorithms (like MaskablePPO from sb3_contrib) to filter the available actions.

The action_masks() function generates a boolean array (mask) of size 608. An index is True if the corresponding action is currently allowed, and False otherwise. The logic is phase-dependent:

#### A. Planning Phase (player.current_phase == "planning")

Sell: Enabled for indices corresponding to units the player currently owns (0 up to num_owned_units - 1).

Buy: Enabled for shop slots i if the bench isn't full, the slot contains an Illuvial, and the player has enough coins.

Reroll: Enabled if the player has enough coins (>= REROLL_COST).

Buy XP: Enabled if the player has enough coins (>= BUY_XP_COST).

Enter Placement: Always enabled.

Modify, Exit Placement, Augment: Disabled.

#### B. Placement Phase (player.current_phase == "placement")

Modify Unit:

To Bench (Target Pos 0): Enabled for a board unit i if the bench is not full.

To Board Tile (Target Pos 1-19): Enabled for a unit i moving to tile j if:

Tile j is empty, AND (unit i is on bench AND board has space) OR (unit i is on board).

Tile j is occupied by unit k (where k != i), AND (unit i is on board - swap board-board) OR (unit i is on bench AND bench has space - swap bench-board, putting k on bench).

Exit Placement: Always enabled.

Sell, Buy, Reroll, BuyXP, Enter Placement, Augment: Disabled.

#### C. Augment Phase (player.current_phase == "augment")

Only active if player.is_augment_choice_pending is True.

Augment Skip: Always enabled.

Augment Apply: Enabled for applying augment choice i to unit j if:

Augment choice i is valid (not None in player.available_augment_choices).

Unit j exists.

Unit j has less than MAX_AUGMENTS_PER_UNIT equipped.

Sell, Buy, Reroll, BuyXP, Placement, Modify: Disabled.

#### D. Fallback Masking

If, due to an unexpected state or logic path, no actions are marked as valid by the above rules, a simple fallback activates to prevent the agent from getting stuck:

It prioritizes enabling Sell (if planning & units exist), Exit Placement (if placement), or Skip Augment (if augment).

If none of those are possible, it defaults to enabling action 0 (Sell unit at index 0).

## Reward Shaping

* Placement Rewards (End of Episode): The primary reward signal, given only when the episode ends. Higher placements yield higher rewards (1st: +1.0, 8th: -1.0). Defined in PLACEMENT_REWARDS.

* Combat Rewards (Applied Post-Simulation):

ROUND_WIN_REWARD (+0.1): Given for winning a simulated battle.

ROUND_LOSS_REWARD (-0.1): Given for losing a simulated battle (or drawing).

* Economic & Team Building Rewards/Penalties (Per Action):

BUY_REWARD (+0.08): Small positive reward for buying an Illuvial. Encourages building a team.

BUY_XP_REWARD (+0.05): Small positive reward for buying XP. Encourages leveling up.

SELL_PENALTY (-0.05): Small negative reward (penalty) for selling any unit. Discourages unnecessary selling.

SELL_LAST_UNIT_PENALTY (-0.7): Additional large penalty applied on top of SELL_PENALTY if the sold unit was the player's only unit. Strongly discourages selling out completely.

REROLL_PENALTY (-0.01): Small penalty for rerolling the shop. Discourages wasting gold on excessive rerolls.

* Placement & Augment Rewards (Per Action):

MODIFY_UNIT_REWARD (+0.02): Small positive reward for successfully moving/swapping a unit during the placement phase.

APPLY_AUGMENT_REWARD (+0.1): Positive reward for choosing and applying an augment to a unit.

SKIP_AUGMENT_REWARD (0.0): Neutral reward for skipping augment selection.

* Other Penalties :

MUTUAL_EMPTY_BOARD_PENALTY (-0.25): Applied if both players in a pairing have no units on the board (pre-simulation check).

INVALID_ACTION_PENALTY (-0.5): Applied if the agent attempts an action flagged as invalid by the mask (should ideally be rare/non-existent with MaskablePPO).


## Observation Space

* Type: gym.spaces.Box (Continuous values, although many represent IDs)

* Shape: (FLAT_OBSERVATION_SIZE, ), which is (NUM_PLAYERS * MAX_BOARD_SLOTS * FEATURES_PER_UNIT, ) = (8 * 19 * 4, ) = (608,)

* Bounds: [0, max_possible_id + 1] where max_possible_id is the highest ID among Illuvials, Augments, and Positions.

#### Structure Breakdown:

* Player Blocks: The array consists of NUM_PLAYERS (8) consecutive blocks.

* The first block always represents the agent's (Player 0) board state.

* The subsequent blocks represent opponents' board states, ordered by Player ID (Player 1, Player 2, ..., Player 7).

* Unit Slots per Player: Each player block contains MAX_BOARD_SLOTS (19) slots, representing the maximum possible units on their board.

* Features per Unit: Each unit slot contains FEATURES_PER_UNIT (4) features:

Feature 0: Illuvial ID: An integer ID representing the Illuvial line type (e.g., 1 for Aapon, 2 for AtippoAir, etc., based on internal mapping). 0 indicates an empty board slot.

Feature 1: Position ID: An integer ID (1 to 19) representing the unified hex position on the board (based on POSITIONS_BLUE_LIST mapping). 0 indicates the unit is not on the board at a valid position (shouldn't happen for units within these slots).

Feature 2: Augment 1 ID: An integer ID representing the first equipped Augment. 0 indicates no augment in this slot.

Feature 3: Augment 2 ID: An integer ID representing the second equipped Augment. 0 indicates no augment in this slot.