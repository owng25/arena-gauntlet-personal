#!/usr/bin/env python3
"""
This script collects Illuvial unit templates from the CombatUnitData JSON files.
It only processes Illuvial units that are Stage 1 and skips any file with 'Lynx', 'Dummy', or 'Ranger' in the name.
It now also extracts the 'Tier' field, which will be used as the cost for buying the unit.
The resulting list is saved as a JSON file ("illuvial_templates.json") in the same directory.
"""

import os
import json

def collect_illuvial_templates(data_dir: str) -> list:
    templates = []
    # Iterate over files in the CombatUnitData directory.
    for filename in os.listdir(data_dir):
        # Process only JSON files.
        if not filename.endswith(".json"):
            continue
        # Skip files with 'Lynx', 'Dummy', or 'Ranger' in their filename.
        if any(x in filename for x in ["Lynx", "Dummy", "Ranger"]):
            continue

        filepath = os.path.join(data_dir, filename)
        try:
            with open(filepath, "r", encoding="utf-8") as f:
                data = json.load(f)
        except Exception as e:
            print(f"Error loading {filepath}: {e}")
            continue

        # Only process entries for Illuvial units.
        if data.get("UnitType") != "Illuvial":
            continue

        # Only include units that are Stage 1.
        if data.get("Stage") != 1:
            continue

        # Extract the necessary attributes (and include the Tier as the cost).
        template = {
            "LineType": data.get("Line"),
            "Stage": data.get("Stage"),
            "Tier": data.get("Tier"),  # Tier is now extracted.
            "Path": data.get("Path"),
            "Variation": data.get("Variation"),
            "DominantCombatClass": data.get("DominantCombatClass"),
            "DominantCombatAffinity": data.get("DominantCombatAffinity")
        }
        templates.append(template)
    return templates

def main():
    # Get the directory of this script.
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # Directory containing CombatUnitData JSON files.
    data_directory = os.path.join(script_dir, "json_data_copy", "data", "CombatUnitData")
    
    if not os.path.isdir(data_directory):
        print(f"Directory {data_directory} does not exist.")
        return

    templates = collect_illuvial_templates(data_directory)
    output_json = json.dumps(templates, indent=4)
    
    # Save the JSON output in the same directory as this script.
    output_file = os.path.join(script_dir, "illuvial_templates.json")
    try:
        with open(output_file, "w", encoding="utf-8") as f:
            f.write(output_json)
        print(f"Templates JSON saved to {output_file}")
    except Exception as e:
        print(f"Failed to save JSON output: {e}")

if __name__ == "__main__":
    main()
