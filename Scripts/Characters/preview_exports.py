"""Review the actual FBX meshes, their materials and exported animation clips."""
import importlib.util
import json
from pathlib import Path

import bpy
import numpy as np

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/"Assets"/"Characters"/"Heroine"
PREVIEW=ROOT/"Build"/"CharacterPreview"
spec=importlib.util.spec_from_file_location("heroine_recipe",Path(__file__).with_name("build_heroine.py"))
recipe=importlib.util.module_from_spec(spec)
spec.loader.exec_module(recipe)


def load(style):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.render.fps=30
    bpy.ops.import_scene.fbx(filepath=str(OUT/("SK_Heroine_"+style+".fbx")))
    rig=next(o for o in bpy.data.objects if o.type=="ARMATURE")
    obj=next(o for o in bpy.data.objects if o.type=="MESH")
    manifest=json.loads((OUT/"materials.json").read_text())
    for index,old in enumerate(obj.data.materials):
        record=manifest[old.name]
        texture=OUT/record["texture"] if record["texture"] else None
        mat=recipe.material(old.name+"_Preview",record["base_color"][:3],record["roughness"],
                            texture,record["alpha_mask"])
        mat.node_tree.nodes.get("Principled BSDF").inputs["Metallic"].default_value=record["metallic"]
        mat.node_tree.nodes.get("Principled BSDF").inputs["Specular IOR Level"].default_value=record["specular"]
        obj.data.materials[index]=mat
    cam=recipe.stage()
    bpy.context.scene.cycles.samples=24
    return obj,rig,cam


def apply_clip(rig,name):
    before=set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(OUT/(name+".fbx")))
    new_objects=set(bpy.data.objects)-before
    imported=next(o for o in new_objects if o.type=="ARMATURE")
    mismatched=[b.name for b in rig.data.bones if any(
        abs(b.matrix_local[row][col]-imported.data.bones[b.name].matrix_local[row][col])>0.001
        for row in range(4) for col in range(4))]
    if mismatched:
        raise RuntimeError("Animation/mesh rest-pose mismatch: "+str(mismatched))
    action=imported.animation_data.action
    slot=imported.animation_data.action_slot
    rig.animation_data_create()
    rig.animation_data.action=action
    rig.animation_data.action_slot=slot
    for obj in new_objects:
        bpy.data.objects.remove(obj,do_unlink=True)
    scene=bpy.context.scene
    scene.frame_start=int(action.frame_range[0])
    scene.frame_end=int(action.frame_range[1])
    scene.frame_set(int(action.frame_range[0]))
    first={b.name:b.matrix.copy() for b in rig.pose.bones}
    scene.frame_set(int(action.frame_range[0]+(action.frame_range[1]-action.frame_range[0])/4))
    changed=[b.name for b in rig.pose.bones if first[b.name].to_quaternion().rotation_difference(
        b.matrix.to_quaternion()).angle>0.0005]
    if not changed:
        raise RuntimeError("Imported animation did not bind to the review mesh's action slot")
    print("PREVIEW_BOUND_ANIMATION",name,changed)
    return action


def render_sheet(cam,rig):
    scene=bpy.context.scene
    scene.render.resolution_x=450
    scene.render.resolution_y=700
    scene.cycles.samples=16
    sheet=np.zeros((1400,1800,4),np.float32)
    frames=[1,5,9,13,17,21,25,29]
    for index,frame in enumerate(frames):
        scene.frame_set(frame)
        name=f"walk-frame-{frame:02}"
        recipe.render(cam,name,(2,-4,1.6),(0,0,0.86))
        # Read saved PNG, because Render Result is not always a readable pixel buffer headlessly.
        image=bpy.data.images.load(str(PREVIEW/(name+".png")),check_existing=False)
        values=np.array(image.pixels[:],np.float32).reshape((700,450,4))
        row,col=divmod(index,4)
        sheet[(1-row)*700:(2-row)*700,col*450:(col+1)*450]=values
        bpy.data.images.remove(image)
    image=bpy.data.images.new("Walk contact sheet",width=1800,height=1400,alpha=True)
    image.pixels.foreach_set(sheet.ravel())
    image.filepath_raw=str(PREVIEW/"heroine-walk-contact-sheet.png")
    image.file_format="PNG"
    image.save()


def main():
    obj,rig,cam=load("LongWave")
    apply_clip(rig,"AN_Heroine_Idle")
    bpy.context.scene.frame_set(1)
    recipe.render(cam,"export-long-front",(0,-4,1.1),(0,0,0.85))
    recipe.render(cam,"export-long-threequarter",(2,-4,1.6),(0,0,0.85))
    recipe.render(cam,"export-long-back",(1.5,4,1.6),(0,0,0.85))
    recipe.render(cam,"export-long-face",(0.25,-3,1.50),(0,-0.015,1.405),0.49)
    # Same asset at a plausible 3rd-person distance in ordinary daylight.
    recipe.render(cam,"export-gameplay-distance",(-2,4,2.5),(0,0,0.9),3.5)
    bpy.context.scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value=0.025
    bpy.data.lights["Key"].energy=110
    bpy.data.lights["Key"].color=(1,0.36,0.095)
    bpy.data.objects["Key"].location=(-1,-1,0.6)
    bpy.data.lights["Fill"].energy=12
    bpy.data.lights["Rim"].energy=25
    recipe.render(cam,"export-firelight",(2,-4,1.5),(0,0,0.85))
    obj,rig,cam=load("Bob")
    apply_clip(rig,"AN_Heroine_Idle")
    bpy.context.scene.frame_set(1)
    recipe.render(cam,"export-bob-threequarter",(2,-4,1.6),(0,0,0.85))
    recipe.render(cam,"export-bob-face",(0.25,-3,1.50),(0,-0.015,1.405),0.49)
    obj,rig,cam=load("LongWave")
    apply_clip(rig,"AN_Heroine_Walk")
    render_sheet(cam,rig)
    bpy.ops.wm.save_as_mainfile(filepath=str(PREVIEW/"ExportReview.blend"))
    print("EXPORTED_CHARACTER_PREVIEWS_COMPLETE")


if __name__=="__main__":
    main()
