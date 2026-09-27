"""Fits the cord belt's path to the heroine's shorts so it lies on the waistband instead of floating
as an ellipse. Run headless, then rebuild the belt:

    blender -b --python Scripts\\Blender\\Recipes\\cord_belt_fit.py
    Scripts\\Blender\\New-Prop.ps1 cord_belt -KeepPivot

Reads the bind pose of Assets\\Characters\\PrimitiveOutfit\\SKM_PrimitiveShorts.fbx (meters, Z up,
-Y forward, X her left) and, all the way round, finds the waistband's top hem, drops just below it
and takes the garment's outer surface there. A tied cord is under tension, so it spans hollows
(the small of her back) rather than following them: the path is the convex hull of that section,
pushed out by the cord's radius. Writes Assets\\Props\\CordBelt\\cord_belt_contour.json:
``pivot`` (the loop centre, in the shorts' frame) and ``points`` (cord centreline, one per degree
from +X toward +Y, relative to the pivot). cord_belt.py follows it when present.

In Unreal the belt then sits at the pivot in the skeleton's reference component space (x, -y, z
in cm), which AHomesteadCharacter converts to a pelvis-relative transform.
"""
import json
import math
import os

import bpy

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
SHORTS = os.path.join(ROOT, 'Assets', 'Characters', 'PrimitiveOutfit', 'SKM_PrimitiveShorts.fbx')
OUT = os.path.join(ROOT, 'Assets', 'Props', 'CordBelt', 'cord_belt_contour.json')
CORD_R = 0.004
BELOW_HEM = 0.018      # cord centre below the top hem, on the waistband
CLEARANCE = 0.0012     # cloth doesn't compress under a light cord
STEPS = 360


def wedge_angle(a, b):
    return abs((a - b + math.pi) % (2 * math.pi) - math.pi)


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=SHORTS)
    mesh = next(o for o in bpy.data.objects if o.type == 'MESH')
    arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    pelvis = arm.matrix_world @ arm.data.bones['pelvis'].head_local
    verts = [mesh.matrix_world @ v.co for v in mesh.data.vertices]
    cx, cy = pelvis.x, pelvis.y
    polar = [(math.atan2(p.y - cy, p.x - cx), math.hypot(p.x - cx, p.y - cy), p.z) for p in verts]

    coarse = 72
    hem, radius = [], []
    for i in range(coarse):
        t = 2 * math.pi * i / coarse
        wedge = [q for q in polar if wedge_angle(q[0], t) < math.radians(4)]
        top = max(q[2] for q in wedge)
        hem.append(top)
    # Smooth the hem so stitch ripples don't make the cord wander.
    hem = [sum(hem[(i + k) % coarse] for k in range(-2, 3)) / 5 for i in range(coarse)]
    for i in range(coarse):
        t = 2 * math.pi * i / coarse
        z = hem[i] - BELOW_HEM
        band = [q[1] for q in polar if wedge_angle(q[0], t) < math.radians(4) and abs(q[2] - z) < 0.006]
        radius.append(max(band))

    # Convex hull of the section (Andrew's monotone chain), then back to one radius per degree.
    pts = sorted((cx + r * math.cos(2 * math.pi * i / coarse), cy + r * math.sin(2 * math.pi * i / coarse))
                 for i, r in enumerate(radius))

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    hull = lower[:-1] + upper[:-1]

    def hull_radius(t):
        d = (math.cos(t), math.sin(t))
        best = 0.0
        for a, b in zip(hull, hull[1:] + hull[:1]):
            ax, ay = a[0] - cx, a[1] - cy
            ex, ey = b[0] - a[0], b[1] - a[1]
            den = d[0] * ey - d[1] * ex
            if abs(den) < 1e-12:
                continue
            s = (ax * ey - ay * ex) / den
            u = (ax * d[1] - ay * d[0]) / den
            if s > 0 and -1e-9 <= u <= 1 + 1e-9:
                best = max(best, s)
        return best

    def hem_at(t):
        f = (t % (2 * math.pi)) / (2 * math.pi) * coarse
        i = int(f) % coarse
        w = f - int(f)
        return hem[i] * (1 - w) + hem[(i + 1) % coarse] * w

    centre = []
    for i in range(STEPS):
        t = 2 * math.pi * i / STEPS
        r = hull_radius(t) + CORD_R + CLEARANCE
        centre.append((cx + r * math.cos(t), cy + r * math.sin(t), hem_at(t) - BELOW_HEM))
    pivot = (cx, cy, sum(p[2] for p in centre) / STEPS)
    data = {
        'source': os.path.relpath(SHORTS, ROOT).replace('\\', '/'),
        'pivot': pivot,
        'points': [(p[0] - pivot[0], p[1] - pivot[1], p[2] - pivot[2]) for p in centre],
    }
    with open(OUT, 'w') as f:
        json.dump(data, f, indent=1)
    print('CORD_BELT_FIT pivot=(%.4f, %.4f, %.4f) around=%.3f m -> %s' % (
        *pivot, sum(math.dist(a, b) for a, b in zip(centre, centre[1:] + centre[:1])), OUT))


main()
