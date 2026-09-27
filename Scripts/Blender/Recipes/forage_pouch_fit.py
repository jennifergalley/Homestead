"""Samples the outside of the heroine's right hip and thigh so the forage pouch can lie flat against
it instead of hanging off her side like a sack. Run headless, then rebuild the pouch:

    blender -b --python Scripts\\Blender\\Recipes\\forage_pouch_fit.py
    Scripts\\Blender\\New-Prop.ps1 forage_pouch -KeepPivot

Reads the bind pose of the coordinator's body export (E:\\Repos\\HomesteadShared\\MetaHumanBody\\
SKM_MHC_Heroine_BodyMesh_Full.fbx, or HOMESTEAD_BODY_FBX) and Assets\\Characters\\PrimitiveOutfit\\
SKM_PrimitiveShorts.fbx, in meters, Z up, -Y forward, X her left. Arms and hands are left out so
only hip, thigh and shorts count. On a grid of heights ``z`` and forward offsets ``f`` (= -y) it
casts a ray in from her right (+X) and keeps the first hit: the outermost of skin or cloth. Writes
Assets\\Props\\ForagePouch\\forage_pouch_fit.json with ``x[i][j]`` for ``z[i]``, ``f[j]`` (null for a
miss) and the cord belt's centreline at her right side, which the pouch hangs from.
"""
import json
import math
import os

import bpy
from mathutils.bvhtree import BVHTree

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
BODY = os.environ.get('HOMESTEAD_BODY_FBX',
                      r'E:\Repos\HomesteadShared\MetaHumanBody\SKM_MHC_Heroine_BodyMesh_Full.fbx')
SHORTS = os.path.join(ROOT, 'Assets', 'Characters', 'PrimitiveOutfit', 'SKM_PrimitiveShorts.fbx')
BELT = os.path.join(ROOT, 'Assets', 'Props', 'CordBelt', 'cord_belt_contour.json')
OUT = os.path.join(ROOT, 'Assets', 'Props', 'ForagePouch', 'forage_pouch_fit.json')
Z_RANGE = (0.74, 1.08)
F_RANGE = (-0.14, 0.16)
STEP = 0.004
ARM_BONES = ('clavicle', 'upperarm', 'lowerarm', 'hand', 'thumb', 'index', 'middle', 'ring', 'pinky')


def meshes(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    return [o for o in set(bpy.data.objects) - before if o.type == 'MESH']


def triangles(obj, skip_arms):
    names = {g.index: g.name.lower() for g in obj.vertex_groups}
    keep = []
    for v in obj.data.vertices:
        best = max(v.groups, key=lambda g: g.weight, default=None)
        bone = names.get(best.group, '') if best else ''
        keep.append(not (skip_arms and any(a in bone for a in ARM_BONES)))
    verts = [obj.matrix_world @ v.co for v in obj.data.vertices]
    polys = [p.vertices[:] for p in obj.data.polygons if all(keep[i] for i in p.vertices)]
    return verts, polys


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    verts, polys = [], []
    for path, skip in ((BODY, True), (SHORTS, False)):
        for obj in meshes(path):
            v, p = triangles(obj, skip)
            base = len(verts)
            verts += v
            polys += [[i + base for i in poly] for poly in p]
    tree = BVHTree.FromPolygons(verts, polys)

    zs = [Z_RANGE[0] + STEP * i for i in range(int(round((Z_RANGE[1] - Z_RANGE[0]) / STEP)) + 1)]
    fs = [F_RANGE[0] + STEP * j for j in range(int(round((F_RANGE[1] - F_RANGE[0]) / STEP)) + 1)]
    grid = []
    for z in zs:
        row = []
        for f in fs:
            hit, _, _, _ = tree.ray_cast((-0.5, -f, z), (1.0, 0.0, 0.0), 0.5)
            # Past her centreline the ray has missed the right leg (a gap, or the other thigh).
            row.append(round(hit.x, 5) if hit is not None and hit.x < -0.04 else None)
        grid.append(row)

    with open(BELT) as fh:
        belt = json.load(fh)
    pivot = belt['pivot']
    side = [(pivot[0] + p[0], pivot[1] + p[1], pivot[2] + p[2]) for p in belt['points']]
    side = [p for p in side if p[0] < -0.08]
    data = {'body': os.path.basename(BODY), 'shorts': os.path.relpath(SHORTS, ROOT).replace('\\', '/'),
            'z': zs, 'f': fs, 'x': grid, 'belt': side}
    with open(OUT, 'w') as fh:
        json.dump(data, fh)
    hits = sum(v is not None for row in grid for v in row)
    print('FORAGE_POUCH_FIT %d x %d grid, %d hits, %d belt points -> %s' % (len(zs), len(fs), hits, len(side), OUT))


main()
