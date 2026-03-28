"""
===============================================================================
INSTRUCTIONS FOR NON-PROGRAMMERS:
1. Install Python from python.org (if you haven't already).
2. Place this script ('convert_eml.py') in a folder.
3. Put all your '.eml' files in that same folder.
4. Double-click this script to run it.
5. It will create a 'data' folder with your new '.jsonc' files and move 
   the originals to a 'backup' folder.
===============================================================================
"""

import json
import shutil
from pathlib import Path

# Configuration
DATA_DIR = Path("data")
BACKUP_DIR = DATA_DIR / "backup"

def setup_directories():
    DATA_DIR.mkdir(exist_ok=True)
    BACKUP_DIR.mkdir(exist_ok=True)

def read_color(val: str) -> int:
    if len(val) == 3: return int(val)
    return int(val, 16)

def convert_eml_to_jsonc(eml_path: Path):
    with open(eml_path, 'r', encoding='utf-8') as f:
        lines = [line.strip() for line in f if line.strip() and not line.startswith('#')]

    if not lines: return

    try:
        model = int(lines[0].split()[0])
    except: return

    json_path = DATA_DIR / f"{model}.jsonc"
    
    # Load existing or start new
    json_data = {}
    if json_path.exists():
        with open(json_path, 'r') as f: json_data = json.load(f)
    
    json_data["metadata"] = {"author": "Unknown", "desc": "Converted EML", "minver": 20000}
    
    sirens = {"imvehft": True, "states": {"1. modelextras": {}}}
    extras = sirens["states"]["1. modelextras"]
    
    for line in lines[1:]:
        p = line.split()
        if len(p) < 12: continue
        
        # Mapping the columns from the EML text file
        id_val = p[0]
        red, green, blue, alpha = read_color(p[2]), read_color(p[3]), read_color(p[4]), read_color(p[5])
        l_type, size, shadow, flash, sw, start = int(p[6]), float(p[7]), float(p[8]), float(p[9]), int(p[10]), int(p[11])
        
        pattern = []
        count = 0
        if len(p) >= 12 + sw:
            for i in range(sw):
                ms = int(p[12+i]) - count
                count += ms
                if ms != 0: pattern.append(ms)

        extras[id_val] = {
            "type": "directional" if l_type == 0 else ("inversed-directional" if l_type == 1 else "non-directional"),
            "size": size,
            "color": {"red": red, "green": green, "blue": blue, "alpha": alpha},
            "state": 1 if (count == 0 or count > 64553) else start,
            "pattern": [] if (count == 0 or count > 64553) else pattern,
            "inertia": flash / 100.0,
            "shadow": {
                "type": "pointlight" if l_type == 2 else "round",
                "size": shadow / 1.5,
                "angleoffset": 180.0 if l_type == 1 else 0.0
            }
        }
    
    json_data["sirens"] = sirens
    with open(json_path, 'w', encoding='utf-8') as f:
        json.dump(json_data, f, indent=4)
    
    shutil.move(str(eml_path), str(BACKUP_DIR / eml_path.name))
    print(f"Done: {eml_path.name} -> {json_path.name}")

if __name__ == "__main__":
    setup_directories()
    files = list(Path(".").glob("*.eml"))
    if not files:
        print("No .eml files found in this folder!")
    for f in files:
        convert_eml_to_jsonc(f)
    input("\nPress Enter to close...")