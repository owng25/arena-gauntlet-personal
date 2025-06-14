#!/usr/bin/env python3
"""
scrape_data.py

Collects augment data from json_data_copy/data/AugmentData/*.json
Specifically extracts fields needed for combat simulation (Name, Stage, Variation, etc.).
Then merges them into "merged_data.json" for easy reference in the environment.
"""

import os
import json
import glob
import traceback

def main():
    try:
        # Determine project root dynamically
        PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
        # Construct the expected path to the 'data' directory
        DATA_DIR_PRIMARY = os.path.join(PROJECT_ROOT, "json_data_copy", "data")
        DATA_DIR_FALLBACK = os.path.join(PROJECT_ROOT, "..", "json_data_copy", "data") # If script is in src/

        # Check which data directory exists
        if os.path.isdir(DATA_DIR_PRIMARY):
            DATA_DIR = DATA_DIR_PRIMARY
            print(f"[INFO] Using data directory: {DATA_DIR}")
        elif os.path.isdir(DATA_DIR_FALLBACK):
            DATA_DIR = DATA_DIR_FALLBACK
            print(f"[INFO] Using fallback data directory: {DATA_DIR}")
        else:
            print(f"[ERROR] Cannot find data directory at {DATA_DIR_PRIMARY} or {DATA_DIR_FALLBACK}")
            return # Exit if data directory not found

        augment_dir = os.path.join(DATA_DIR, "AugmentData")

        merged_output = {
            "Augments": {}
            # Removed "Consumables" and "Costs"
        }

        # 1) Scrape AugmentData
        if os.path.isdir(augment_dir):
            print(f"[INFO] Scraping augments from: {augment_dir}")
            augment_files = glob.glob(os.path.join(augment_dir, "*.json"))
            print(f"[INFO] Found {len(augment_files)} potential augment files.")
            processed_count = 0
            error_count = 0

            for path in augment_files:
                fname = os.path.basename(path)
                try:
                    with open(path, "r", encoding="utf-8") as f:
                        adata = json.load(f)

                    # --- Validate and Extract Key Fields ---
                    name = adata.get("Name")
                    stage = adata.get("Stage") # Keep stage as it might be relevant later
                    variation = adata.get("Variation")

                    # Basic validation - ensure essential keys are present
                    if name is None or stage is None or variation is None:
                        print(f"[WARN] Skipping {fname}: Missing 'Name', 'Stage', or 'Variation' key in JSON content.")
                        error_count += 1
                        continue

                    # Construct the unique key used by the environment (Name_Stage_Variation)
                    # E.g. "ApexEmpowerment_0_Original"
                    key = f"{name}_{stage}_{variation}"

                    # Store the *entire* augment data dictionary under this key.
                    # The environment can then access any needed sub-fields.
                    merged_output["Augments"][key] = adata
                    processed_count += 1

                except json.JSONDecodeError:
                    print(f"[ERROR] Failed to decode JSON in file: {fname}")
                    error_count += 1
                except Exception as e:
                    print(f"[ERROR] Unexpected error processing file {fname}: {e}")
                    traceback.print_exc()
                    error_count += 1

            print(f"[INFO] Successfully processed {processed_count} augment files.")
            if error_count > 0:
                print(f"[WARN] Skipped or encountered errors with {error_count} files.")
        else:
            print(f"[WARNING] AugmentData folder not found: {augment_dir}")

        # 2) Write merged_data.json
        # Ensure the output file goes into the correct DATA_DIR
        out_file = os.path.join(DATA_DIR, "merged_data.json")
        try:
            with open(out_file, "w", encoding="utf-8") as out:
                json.dump(merged_output, out, indent=4)
            print(f"[INFO] Wrote merged augment data to {out_file}")
            print(f"[INFO] Total augments saved: {len(merged_output['Augments'])}")
        except IOError as e:
            print(f"[ERROR] Failed to write merged data to {out_file}: {e}")
        except Exception as e:
            print(f"[ERROR] Unexpected error writing merged data: {e}")
            traceback.print_exc()

    except Exception as e:
        print(f"[FATAL ERROR] An unexpected error occurred in main(): {e}")
        traceback.print_exc()

if __name__ == "__main__":
    main()