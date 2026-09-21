"""CPU-only source palette diagnostic; never imports or runs Unreal."""
import importlib.util
import json
import re
import sys
from pathlib import Path

import bpy

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("hair_recipe", Path(__file__).with_name("build_hairstyle_refinement.py"))
recipe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(recipe)
rig, mesh = recipe.load(recipe.OUT/"Joined"/"SK_Heroine_Bob.fbx")
recipe.restore_preview_materials(mesh, "Bob")
material = next(m for m in mesh.data.materials if m.name.endswith("_Neutral"))
mix = next(n for n in material.node_tree.nodes if n.type == "MIX_RGB")
source = (recipe.ROOT/"Source"/"SurvivalGame"/"HomesteadAppearance.cpp").read_text()
function = source.split("FLinearColor NeutralHairTint(int32 Index)",1)[1].split("return Choice",1)[0]
colors = [tuple(float(v.strip().rstrip("f")) for v in args.split(","))
          for args in re.findall(r"FLinearColor\(([^)]+)\)",function)]
assert len(colors) == 5
names = ["Chestnut","DarkBrown","Black","Copper","Blonde"]
cam = recipe.author.stage()
scene = bpy.context.scene
scene.cycles.device = "CPU"
scene.cycles.samples = 8
scene.render.threads_mode = "FIXED"
scene.render.threads = 2
scene.render.resolution_x = 320
scene.render.resolution_y = 400
before = recipe.nonhair_record(mesh)
for name,color in zip(names,colors):
    mix.inputs[2].default_value = (*color,1)
    recipe.author.render(cam,"Palette-"+name,(.4,-4,1.55),(0,-.035,1.5),.34)
    assert recipe.assert_surfaces(before,recipe.nonhair_record(mesh),0) == 0
(recipe.PREVIEW/"palette-validation.json").write_text(json.dumps({
    "source_only":True,"neutral_linear_colors":dict(zip(names,colors)),
    "nonhair_geometry_uv_weights_unchanged":True,
    "legacy_pony_and_brows":"retain separate HairTint; never use these neutral values"
},indent=2))
print("HAIRSTYLE_PALETTE_VERIFIED",names)
