"""
===============================================================================
INSTRUCTIONS FOR NON-PROGRAMMERS:
1. Place this script ('convert_avs.py') in a folder.
2. Put your old AVS '.json' files in that same folder.
3. Double-click this script to run it.
4. It will create new '.jsonc' files and move the old ones to 'backup'.
===============================================================================
"""

import json
import shutil
from pathlib import Path

DATA_DIR = Path("data")
BACKUP_DIR = DATA_DIR / "backup"
SHADOW_TYPES = ["round", "pointlight", "arealight", "bollard", "comet", "cylindernarrow", "defined", "defineddiffuse", "defineddiffusespot", "definedspot", "narrow", "jellyfish", "mediumscatter", "overhead", "parallelbeam", "pear", "round", "scatterlight", "softarrow", "softdisplay", "star", "starfocused", "threelobeumbrella", "threelobevee", "tightfocused", "toppost", "trapezoid", "umbrella", "vee", "veeup", "xarrow", "xarrowdiffuse", "xarrowsoft"]

def setup_directories():
    DATA_DIR.mkdir(exist_ok=True)
    BACKUP_DIR.mkdir(exist_ok=True)

def update_recursive(data):
    if isinstance(data, dict):
        for k, v in data.items():
            if k == "shadow" and isinstance(v, dict) and "type" in v and isinstance(v["type"], int):
                idx = v["type"]
                name = SHADOW_TYPES[idx] if 0 <= idx < len(SHADOW_TYPES) else "round"
                v["type"] = name
                if name != "round" and "size" in v:
                    v["size"] = float(v["size"]) * 0.666
            update_recursive(v)
    elif isinstance(data, list):
        for i in data: update_recursive(i)

def convert(p: Path):
    out = Path(str(p) + "c")
    with open(p, 'r', encoding='utf-8') as f:
        old_data = json.load(f)
    
    new_data = {"metadata": {"author": "Unknown", "desc": "AVS Upgrade", "minver": 20000}, "sirens": old_data}
    update_recursive(new_data["sirens"])
    
    with open(out, 'w', encoding='utf-8') as f:
        json.dump(new_data, f, indent=4)
    
    shutil.move(str(p), str(BACKUP_DIR / p.name))
    print(f"Done: {p.name} -> {out.name}")

if __name__ == "__main__":
    setup_directories()
    for f in Path(".").glob("*.json"):
        convert(f)
    input("\nPress Enter to close...")