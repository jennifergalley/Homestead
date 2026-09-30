"""Samples the heroine's back, shoulders and chest so the leather knapsack (leather_backpack.py) can sit on
her shoulder blades and its straps can lie over her shoulders instead of floating. Run headless, then
rebuild the knapsack:

    blender -b --python Scripts\\Blender\\Recipes\\leather_backpack_fit.py
    Scripts\\Blender\\New-Prop.ps1 leather_backpack -KeepPivot

Reads the bind pose of the coordinator's body export (E:\\Repos\\HomesteadShared\\MetaHumanBody\\
SKM_MHC_Heroine_BodyMesh_Full.fbx, or HOMESTEAD_BODY_FBX), in meters, Z up, -Y forward, X her left.
Arms and hands are left out, so only her torso counts.
- ``back[i][j]``: for heights ``z[i]`` and sideways offsets ``x[j]``, the Y of her back where a ray
  cast forward from behind her (+Y toward -Y) first hits it (null for a miss).
- ``shoulder[k]``: for each strap side (x = +-STRAP_X), the top of her shoulder at a run of Y
  offsets from back to front: a ray cast down from above.
- ``chest[i][s]``: at heights ``z_chest[i]``, her front at x = +-STRAP_X (a ray cast backward from
  in front of her).
Writes Assets\\Props\\LeatherBackpack\\leather_backpack_fit.json.
"""
import json
import os

import bpy
from mathutils.bvhtree import BVHTree

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
BODY = os.environ.get('HOMESTEAD_BODY_FBX',
                      r'E:\Repos\HomesteadShared\MetaHumanBody\SKM_MHC_Heroine_BodyMesh_Full.fbx')
OUT = os.path.join(ROOT, 'Assets', 'Props', 'LeatherBackpack', 'leather_backpack_fit.json')
Z_RANGE = (0.92, 1.46)
X_RANGE = (-0.22, 0.22)
STEP = 0.01
# Where the shoulder straps cross her shoulders (either side of her neck, on the trapezius).
STRAP_X = 0.105
SHOULDER_Y = (-0.12, 0.14)
Z_CHEST = (1.10, 1.42)
ARM_BONES = ('upperarm', 'lowerarm', 'hand', 'thumb', 'index', 'middle', 'ring', 'pinky')


def meshes(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    return [o for o in set(bpy.data.objects) - before if o.type == 'MESH']


def triangles(obj):
    names = {g.index: g.name.lower() for g in obj.vertex_groups}
    keep = []
    for v in obj.data.vertices:
        best = max(v.groups, key=lambda g: g.weight, default=None)
        bone = names.get(best.group, '') if best else ''
        keep.append(not any(a in bone for a in ARM_BONES))
    verts = [obj.matrix_world @ v.co for v in obj.data.vertices]
    polys = [p.vertices[:] for p in obj.data.polygons if all(keep[i] for i in p.vertices)]
    return verts, polys


def span(lo, hi, step):
    return [round(lo + step * i, 4) for i in range(int(round((hi - lo) / step)) + 1)]


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    verts, polys = [], []
    for obj in meshes(BODY):
        v, p = triangles(obj)
        base = len(verts)
        verts += v
        polys += [[i + base for i in poly] for poly in p]
    tree = BVHTree.FromPolygons(verts, polys)

    zs, xs = span(*Z_RANGE, STEP), span(*X_RANGE, STEP)
    back = []
    for z in zs:
        row = []
        for x in xs:
            hit, _, _, _ = tree.ray_cast((x, 0.6, z), (0.0, -1.0, 0.0), 0.6)
            row.append(round(hit.y, 5) if hit is not None and hit.y > -0.05 else None)
        back.append(row)

    ys = span(*SHOULDER_Y, STEP)
    shoulder = []
    for side in (1.0, -1.0):
        run = []
        for y in ys:
            hit, _, _, _ = tree.ray_cast((side * STRAP_X, y, 2.0), (0.0, 0.0, -1.0), 1.0)
            run.append(round(hit.z, 5) if hit is not None else None)
        shoulder.append(run)

    zc = span(*Z_CHEST, STEP)
    chest = []
    for z in zc:
        pair = []
        for side in (1.0, -1.0):
            hit, _, _, _ = tree.ray_cast((side * STRAP_X, -0.6, z), (0.0, 1.0, 0.0), 0.6)
            pair.append(round(hit.y, 5) if hit is not None and hit.y < 0.05 else None)
        chest.append(pair)

    data = {'body': os.path.basename(BODY), 'z': zs, 'x': xs, 'back': back,
            'strap_x': STRAP_X, 'shoulder_y': ys, 'shoulder': shoulder, 'z_chest': zc, 'chest': chest}
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, 'w') as fh:
        json.dump(data, fh)
    hits = sum(v is not None for row in back for v in row)
    print('LEATHER_BACKPACK_FIT back %d x %d (%d hits), shoulders %d, chest %d -> %s'
          % (len(zs), len(xs), hits, len(ys), len(zc), OUT))


main()
