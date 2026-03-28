"""
===============================================================================
INSTRUCTIONS FOR NON-PROGRAMMERS:
1. Place this script ('convert_carcols.py') in a folder.
2. Put your '.ivfc' files (ImVehFt carcols) in that same folder.
3. Double-click this script to run it.
4. It will extract colors and variations into a new '.jsonc' file.
===============================================================================
"""

import json
import shutil
from pathlib import Path

DATA_DIR = Path("data")
BACKUP_DIR = DATA_DIR / "backup"

def setup_directories():
    DATA_DIR.mkdir(exist_ok=True)
    BACKUP_DIR.mkdir(exist_ok=True)

def convert(p: Path):
    with open(p, 'r', encoding='utf-8') as f:
        lines = [l.strip() for l in f if l.strip() and not l.startswith('#')]

    model = -1
    for l in lines:
        if l.lower().startswith("vehicle_id"):
            model = int(l.split()[1])
            break
    
    if model == -1: return

    out = DATA_DIR / f"{model}.jsonc"
    carcols = {"colors": [], "variations": []}
    mode = None
    
    for l in lines:
        if "num_colors" in l.lower(): mode = "c"
        elif "num_variations" in l.lower(): mode = "v"
        else:
            parts = l.split()
            if mode == "c" and len(parts) >= 3:
                carcols["colors"].append({"red": int(parts[0]), "green": int(parts[1]), "blue": int(parts[2])})
            elif mode == "v" and len(parts) >= 4:
                carcols["variations"].append({"primary": int(parts[0]), "secondary": int(parts[1]), "tertiary": int(parts[2]), "quaternary": int(parts[3])})

    data = {"metadata": {"author": "Unknown", "desc": "IVF Carcols", "minver": 20000}, "carcols": carcols}
    with open(out, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=4)
        
    shutil.move(str(p), str(BACKUP_DIR / p.name))
    print(f"Done: {p.name} -> {out.name}")

if __name__ == "__main__":
    setup_directories()
    for f in Path(".").glob("*.ivfc"):
        convert(f)
    input("\nPress Enter to close...")