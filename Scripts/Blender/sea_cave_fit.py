"""Refit the sea cave's outputs without remeshing: the landscape holes and the runtime table.

    blender -b --factory-startup --python Scripts/Blender/sea_cave_fit.py

Writes Scripts/Terrain/sea_cave_holes.json and Source/SurvivalGame/Simulation/HomesteadSeaCave.inc from
Recipes/sea_cave.py, Scripts/Terrain/sea_cave_site.json and the estate heightfield. Rebuild the mesh itself
(New-Prop.ps1 sea_cave) whenever the site or the ground under it changes; this alone is for table-only edits
(the dressing list, the light).
"""
import importlib.util
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
spec = importlib.util.spec_from_file_location("sea_cave", HERE / "Recipes" / "sea_cave.py")
cave = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cave)
site, void, holes = cave.write_fit()
print(f"SEA_CAVE_FIT site ({site.ax}, {site.ay}) floor {site.z0:.2f} m; {len(holes)} hole quads")
