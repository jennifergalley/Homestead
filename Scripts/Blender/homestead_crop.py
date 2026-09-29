"""Shared helpers for the 1 m garden-plot crop recipes (Scripts/Blender/Recipes/crop_*.py).

    import homestead_crop as C
    objs = C.stage_meshes(kit, NAME, material, emit)      # five stages x three LODs, plot origin
    produce = C.single(kit, batch, f"SM_{NAME}_Produce", produce_material, still=True)

Every crop plot is one 1 m square with its pivot at the bottom centre of the tilled bed; the ridge
tops run along X at y = -0.3, 0, +0.3 m and sit about 4.5 cm up. The game places the stage mesh at
the plot centre and the produce instances on the report's plot-local anchors, so stage meshes keep
the plot origin rather than being re-centred on their bounds.
"""
import math
import random

from mathutils import Matrix, Vector

import homestead_foliage as F

RIDGES = (-0.30, 0.0, 0.30)
BASE_Z = 0.045
STAGES = ("Sprout", "Young", "Growing", "Mature", "Ripe")
PRODUCE_STAGES = ("Young", "Growing", "Mature", "Ripe")
EDGE = 0.47  # keep foliage inside the 1 m square with a little margin

UP = Vector((0, 0, 1))


def restore_plot_origin(objs):
    """Undo finish_lods' re-centring so every LOD keeps the plot's own origin."""
    for obj in objs:
        shift = Vector(obj.get("homestead_shift", (0, 0, 0)))
        if shift.length > 1e-8:
            obj.data.transform(Matrix.Translation(shift))
            obj["homestead_shift"] = [0.0, 0.0, 0.0]
            obj.data.update()
    return objs


def still(obj):
    """Produce and hand-held meshes don't sway: zero the wind weight, keep a phase."""
    attr = obj.data.attributes.get("Wind")
    if attr:
        values = []
        for i in range(len(attr.data)):
            values.extend((0.0, ((i * 41) % 251) / 251.0, 0.0, 1.0))
        attr.data.foreach_set("color_srgb", values)
        obj.data.color_attributes.active_color = obj.data.color_attributes["Wind"]
    return obj


def single(kit, batch, name, material, still_wind=False):
    obj = batch.build(name, material)
    obj = kit.join([obj], name, pivot=None, unwrap=False, reshade=True, smooth_angle=179.0)
    obj.data.color_attributes.active_color = obj.data.color_attributes["Wind"]
    if still_wind:
        still(obj)
    print("HOMESTEAD_LODS", name, F.lod_report([obj]))
    return obj


def stage_meshes(kit, name, material, emit, stages=STAGES):
    out = []
    for stage in stages:
        batches = [emit(stage, lod) for lod in range(3)]
        objs = F.finish_lods(kit, batches, f"SM_{name}_{stage}", material, smooth_angle=179.0)
        restore_plot_origin(objs)
        print("HOMESTEAD_LODS", stage, F.lod_report(objs))
        out.extend(objs)
    return out


def anchors_report(mesh, per_stage):
    """per_stage: {stage: [(Vector pos, yaw_deg, scale), ...]} -> the report's produce block."""
    return {"mesh": mesh, "anchors": {
        stage: [[round(p.x, 4), round(p.y, 4), round(p.z, 4), round(yaw, 1), round(scale, 3)]
                for p, yaw, scale in rows]
        for stage, rows in per_stage.items()}}


def heading(az):
    return Vector((math.cos(az), math.sin(az), 0.0))


def arc(base, head, elev, length, droop, n):
    """Points along an arc leaving ``base`` toward ``head`` at ``elev`` rad, bending down by
    ``droop`` rad over its length."""
    d = (head * math.cos(elev) + UP * math.sin(elev)).normalized()
    side = head.cross(UP)
    side = side.normalized() if side.length > 1e-5 else Vector((1, 0, 0))
    pts = [Vector(base)]
    for _ in range(max(n, 1)):
        d = (Matrix.Rotation(-droop / max(n, 1), 3, side) @ d).normalized()
        pts.append(pts[-1] + d * (length / max(n, 1)))
    return pts


def edge_scale(base, head, reach, margin=EDGE):
    """Shorten a leaf that would poke out of the plot square (0.5..1)."""
    allowed = 9.0
    if abs(head.x) > 1e-4:
        allowed = min(allowed, ((margin - base.x) if head.x > 0 else (-margin - base.x)) / head.x)
    if abs(head.y) > 1e-4:
        allowed = min(allowed, ((margin - base.y) if head.y > 0 else (-margin - base.y)) / head.y)
    if allowed <= 0:
        return 0.5
    return min(1.0, max(0.5, allowed / max(reach, 1e-4)))


def cupped_leaf(b, rect, base, radial, length, width, phase, rng, *, elev=0.65, droop=0.25,
                cup=0.030, rows=8, cols=6, flutter=0.65, wave=0.075, frill=0.010, narrow=0.38):
    """A real curved brassica-style leaf (not a flat card): narrow petiole, broad wavy blade,
    margins cupped up. ``rect`` is the atlas tile (u across, v base -> tip)."""
    radial = Vector(radial).normalized()
    side = radial.cross(UP)
    side = side.normalized() if side.length > 1e-4 else Vector((1, 0, 0))
    normal = (UP - radial * 0.30).normalized()
    direction = (radial * math.cos(elev) + UP * math.sin(elev)).normalized()
    u0, u1, v0, v1 = rect
    wave_phase = rng.uniform(0, math.tau)
    idx = []
    for i in range(rows + 1):
        v = i / rows
        prof = math.sin(math.pi * (0.04 + 0.92 * v)) ** 0.55
        prof *= (narrow + (1 - narrow) * min(v / 0.28, 1.0))
        if v > 0.86:
            prof *= 1.0 - 0.35 * (v - 0.86) / 0.14
        center = Vector(base) + direction * (length * v) - UP * (droop * length * v * v)
        row = []
        for j in range(cols + 1):
            s = j / cols * 2 - 1
            margin_wave = 1.0 + wave * math.sin(17.0 * v + wave_phase) * (abs(s) ** 4)
            hw = width * 0.5 * max(prof, 0.08) * margin_wave
            cupping = cup * (abs(s) ** 1.7) * (0.45 + 0.75 * math.sin(math.pi * v))
            fr = UP * (frill * math.sin(23 * v + wave_phase) * (abs(s) ** 6))
            row.append(b._vert(center + side * (s * hw) + normal * cupping + fr, phase,
                               flutter * (0.12 + 0.88 * v)))
        idx.append(row)
    for i in range(rows):
        for j in range(cols):
            b.faces.append((idx[i][j], idx[i][j + 1], idx[i + 1][j + 1], idx[i + 1][j]))
            b.uvs.append([(u0 + (u1 - u0) * (j / cols), v0 + (v1 - v0) * (i / rows)),
                          (u0 + (u1 - u0) * ((j + 1) / cols), v0 + (v1 - v0) * (i / rows)),
                          (u0 + (u1 - u0) * ((j + 1) / cols), v0 + (v1 - v0) * ((i + 1) / rows)),
                          (u0 + (u1 - u0) * (j / cols), v0 + (v1 - v0) * ((i + 1) / rows))])


def strap(b, rect, base, direction, length, width, phase, *, up=None, rows=5, droop=0.4,
          twist=0.0, fold=0.0, flutter=1.0, flutter_base=0.05, taper=0.55):
    """A long strap-leaf card (grass, grain or leek flag) carrying a painted blade: it narrows by
    ``taper`` toward the tip (the texture's alpha gives the real outline), arches over by
    ``droop`` rad and twists along its length."""
    d = Vector(direction).normalized()
    upv = Vector(up) if up is not None else UP
    side = d.cross(upv)
    side = side.normalized() if side.length > 1e-4 else Vector((1, 0, 0))
    u0, u1, v0, v1 = rect
    pos = Vector(base)
    head = d.copy()
    idx = []
    for i in range(rows + 1):
        t = i / rows
        if i:
            bend = Matrix.Rotation(-droop / rows * (0.4 + 1.2 * t), 3, side)
            head = (bend @ head).normalized()
            pos = pos + head * (length / rows)
        tw = Matrix.Rotation(twist * t, 3, head)
        sd = tw @ side
        nn = head.cross(sd).normalized()
        hw = width * 0.5 * (1.0 - taper * t * t)
        for s in (-1.0, 0.0, 1.0):
            off = sd * (s * hw) + nn * (fold * abs(s) * hw)
            idx.append(b._vert(pos + off, phase, flutter_base + (flutter - flutter_base) * t))
    for i in range(rows):
        for j in range(2):
            a = i * 3 + j
            b.faces.append((idx[a], idx[a + 1], idx[a + 4], idx[a + 3]))
            b.uvs.append([(u0 + (u1 - u0) * (jj / 2), v0 + (v1 - v0) * (ii / rows))
                          for ii, jj in ((i, j), (i, j + 1), (i + 1, j + 1), (i + 1, j))])
    return pos


def plant_rng(seed, *parts):
    """Deterministic per-plant RNG (str hashes are salted per process, so sum the characters)."""
    salt = sum((k + 1) * 131 * sum(ord(c) for c in str(p)) for k, p in enumerate(parts))
    return random.Random(seed * 1000003 + salt)
