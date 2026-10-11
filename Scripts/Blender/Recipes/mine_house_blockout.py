"""Mine house BLOCKOUT (grey massing for Jenny's silhouette and room-layout review; not final art).

    .\\Scripts\\Blender\\New-Prop.ps1 mine_house_blockout -NoBeauty

Flat-coloured boxes from ``mine/layout.py``: the granite shell with its round-arched openings, the
gable slate roof, the corner stack, the first floor, and the furnished ground-floor room (desk and
lamp, rock pile, trapdoor open over the ladder shaft). One mesh, pivot = house origin on the yard,
complex collision so she can walk in through the door. Replaced by the detailed ``mine_house``.
"""
import math
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
from mine import layout as L  # noqa: E402

NAME = "MineHouseBlockout"
DESCRIPTION = "Blockout of the mine house (engine-house style) and its furnished ground-floor room."
COLLISION = "none"          # import_props.COLLISION_OVERRIDES gives it complex-as-simple
TRIANGLE_BUDGET = 60000
PROVENANCE = "Original project-authored blockout geometry."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 3.0), "views": ["hero"]}
REPORT = {"pivot": "House origin on the mine yard (Anchor::MineEntrance), Unreal frame; place at yaw 0."}


def U(x, y, z):
    """Unreal-frame metres to Blender (FBX import negates Y)."""
    return (x, -y, z)


def build(kit):
    stone = kit.material("M_MineBlockStone", (0.26, 0.255, 0.24), roughness=0.9)
    dressed = kit.material("M_MineBlockDressed", (0.34, 0.33, 0.31), roughness=0.9)
    slate = kit.material("M_MineBlockSlate", (0.10, 0.11, 0.13), roughness=0.7)
    brick = kit.material("M_MineBlockBrick", (0.33, 0.14, 0.09), roughness=0.9)
    timber = kit.material("M_MineBlockTimber", (0.22, 0.14, 0.08), roughness=0.85)
    boards = kit.material("M_MineBlockBoards", (0.30, 0.21, 0.13), roughness=0.85)
    rock = kit.material("M_MineBlockRock", (0.30, 0.30, 0.29), roughness=0.95)
    lamp = kit.material("M_MineBlockLamp", (0.95, 0.70, 0.30), roughness=0.4)
    dark = kit.material("M_MineBlockShaft", (0.02, 0.02, 0.02), roughness=1.0)
    parts = []

    def box(name, lo, hi, mat):
        lo, hi = list(lo), list(hi)
        c = [(a + b) / 2 for a, b in zip(lo, hi)]
        s = [abs(b - a) for a, b in zip(lo, hi)]
        o = kit.box(name, s, U(*c), material=mat)
        parts.append(o)
        return o

    # --- footing, floor (with the trapdoor hole) and the first floor
    # The footing: a ring proud of the wall faces only (the room's floor sits inside it), broken at the door.
    ox, oy, w = L.OUT_X / 2 + 0.06, L.OUT_Y / 2 + 0.06, L.WALL + 0.06
    dh0 = L.DOOR["width"] / 2
    for lo, hi in (((-ox, -oy), (ox, -oy + w)), ((-ox, oy - w), (ox, oy)), ((-ox, -oy + w), (-ox + w, oy - w)),
                   ((ox - w, -oy + w), (ox, -dh0)), ((ox - w, dh0), (ox, oy - w))):
        box("Plinth", (*lo, -0.6), (*hi, L.PLINTH), dressed)
    box("Threshold", (L.OUT_X / 2 - L.WALL, -dh0, -0.6), (L.OUT_X / 2 + 0.06, dh0, L.FLOOR_Z), dressed)
    tx, ty = L.TRAPDOOR["centre"]
    hx, hy = L.TRAPDOOR["size"][0] / 2, L.TRAPDOOR["size"][1] / 2
    fz0, fz1 = L.FLOOR_Z - 0.10, L.FLOOR_Z
    box("FloorW", (-L.IN_X, -L.IN_Y, fz0), (tx - hx, L.IN_Y, fz1), boards)
    box("FloorE", (tx + hx, -L.IN_Y, fz0), (L.IN_X, L.IN_Y, fz1), boards)
    box("FloorS", (tx - hx, -L.IN_Y, fz0), (tx + hx, ty - hy, fz1), boards)
    box("FloorN", (tx - hx, ty + hy, fz0), (tx + hx, L.IN_Y, fz1), boards)
    box("UpperFloor", (-L.IN_X, -L.IN_Y, L.CEILING_Z), (L.IN_X, L.IN_Y, L.UPPER_FLOOR_Z), boards)
    for i in range(7):
        x = -L.IN_X + 0.6 + i * (2 * L.IN_X - 1.2) / 6
        box(f"Joist{i}", (x - 0.11, -L.IN_Y, L.CEILING_Z - 0.24), (x + 0.11, L.IN_Y, L.CEILING_Z), timber)

    # --- the shaft under the trapdoor: a granite-lined well with a ladder, fading to black
    depth = L.SHAFT_DEPTH_SHOWN
    box("ShaftW", (tx - hx - 0.4, ty - hy - 0.4, -depth), (tx - hx, ty + hy + 0.4, fz0), stone)
    box("ShaftE", (tx + hx, ty - hy - 0.4, -depth), (tx + hx + 0.4, ty + hy + 0.4, fz0), stone)
    box("ShaftS", (tx - hx, ty - hy - 0.4, -depth), (tx + hx, ty - hy, fz0), stone)
    box("ShaftN", (tx - hx, ty + hy, -depth), (tx + hx, ty + hy + 0.4, fz0), stone)
    box("ShaftBottom", (tx - hx, ty - hy, -depth - 0.2), (tx + hx, ty + hy, -depth), dark)
    for side in (-0.22, 0.22):   # ladder against the west side of the shaft
        box(f"LadderRail{side}", (tx - hx + 0.06, ty + side - 0.03, -depth), (tx - hx + 0.14, ty + side + 0.03, L.FLOOR_Z + 0.9), timber)
    for k in range(int((depth + L.FLOOR_Z) / 0.3)):
        z = -depth + 0.25 + k * 0.3
        box(f"Rung{k}", (tx - hx + 0.08, ty - 0.24, z - 0.02), (tx - hx + 0.12, ty + 0.24, z + 0.02), timber)
    # Collar round the opening and the lid thrown open against its hinge (west side).
    for name, lo, hi in (("CollarS", (tx - hx - 0.12, ty - hy - 0.12), (tx + hx + 0.12, ty - hy)),
                         ("CollarN", (tx - hx - 0.12, ty + hy), (tx + hx + 0.12, ty + hy + 0.12)),
                         ("CollarE", (tx + hx, ty - hy), (tx + hx + 0.12, ty + hy))):
        box(name, (*lo, L.FLOOR_Z), (*hi, L.FLOOR_Z + 0.05), timber)
    lid = kit.box("TrapdoorLid", (0.08, 2 * hy + 0.1, 2 * hx + 0.1), U(tx - hx - 0.16, ty, L.FLOOR_Z + hx + 0.05),
                  rotation=(0, -12.0, 0), material=timber)
    parts.append(lid)

    # --- walls with round-arched openings cut out
    walls = {
        "+X": ((L.OUT_X / 2 - L.WALL, -L.OUT_Y / 2, 0.0), (L.OUT_X / 2, L.OUT_Y / 2, L.EAVES_Z)),
        "-X": ((-L.OUT_X / 2, -L.OUT_Y / 2, 0.0), (-L.OUT_X / 2 + L.WALL, L.OUT_Y / 2, L.EAVES_Z)),
        "+Y": ((-L.OUT_X / 2 + L.WALL, L.OUT_Y / 2 - L.WALL, 0.0), (L.OUT_X / 2 - L.WALL, L.OUT_Y / 2, L.EAVES_Z)),
        "-Y": ((-L.OUT_X / 2 + L.WALL, -L.OUT_Y / 2, 0.0), (L.OUT_X / 2 - L.WALL, -L.OUT_Y / 2 + L.WALL, L.EAVES_Z)),
    }
    wall_objs = {k: box("Wall" + k, lo, hi, stone) for k, (lo, hi) in walls.items()}
    # Gable triangles on the X ends.
    run, rise = L.OUT_Y / 2, L.RIDGE_Z - L.EAVES_Z
    for key, x0, x1 in (("+X", L.OUT_X / 2 - L.WALL, L.OUT_X / 2), ("-X", -L.OUT_X / 2, -L.OUT_X / 2 + L.WALL)):
        v = [U(x0, -run, L.EAVES_Z), U(x0, run, L.EAVES_Z), U(x0, 0, L.RIDGE_Z),
             U(x1, -run, L.EAVES_Z), U(x1, run, L.EAVES_Z), U(x1, 0, L.RIDGE_Z)]
        g = kit.mesh("Gable" + key, v, [(0, 2, 1), (3, 4, 5), (0, 1, 4, 3), (1, 2, 5, 4), (2, 0, 3, 5)], stone)
        kit.recalc_normals(g)
        parts.append(g)
        wall_objs[key + "g"] = g

    cutters = []
    for opening in [L.DOOR] + L.WINDOWS:
        w, half = opening["wall"], opening["width"] / 2
        along_x = w in ("+Y", "-Y")
        c = opening["centre"]
        if w[1] == "X":
            x = (L.OUT_X / 2 - L.WALL / 2) * (1 if w[0] == "+" else -1)
            centre_rect = (x, c)
        else:
            y = (L.OUT_Y / 2 - L.WALL / 2) * (1 if w[0] == "+" else -1)
            centre_rect = (c, y)
        sill, spring = opening["sill"], opening["spring"]
        size = (2 * half, L.WALL * 3, spring - sill) if along_x else (L.WALL * 3, 2 * half, spring - sill)
        rect = kit.box("Cut", size, U(centre_rect[0], centre_rect[1], (sill + spring) / 2), material=stone)
        arch = kit.cylinder("CutArch", half, L.WALL * 3, U(centre_rect[0], centre_rect[1], spring),
                            rotation=(90, 0, 0) if along_x else (0, 90, 0), sides=24, material=stone)
        targets = [wall_objs[w]] + ([wall_objs[w + "g"]] if spring + half > L.EAVES_Z else [])
        for target in targets:
            for cutter in (rect, arch):
                mod = target.modifiers.new("cut", "BOOLEAN")
                mod.operation, mod.solver, mod.object = "DIFFERENCE", "EXACT", cutter
        cutters += [rect, arch]
        # Dressed voussoir ring and sill as a lighter band (the final has real voussoirs).
        sign = 1 if w[0] == "+" else -1
        face = (L.OUT_Y / 2 if along_x else L.OUT_X / 2) * sign
        ring = kit.cylinder("Voussoirs", half + 0.22, 0.06, U(*(centre_rect[0], face + 0.03 * sign, spring) if along_x
                                                               else (face + 0.03 * sign, centre_rect[1], spring)),
                            rotation=(90, 0, 0) if along_x else (0, 90, 0), sides=24, material=dressed)
        hole = kit.cylinder("VoussoirHole", half, 0.2, ring.location, rotation=tuple(math.degrees(a) for a in ring.rotation_euler), sides=24, material=dressed)
        cut_below = kit.box("VoussoirCut", (3, 3, 3), (ring.location[0], ring.location[1], spring - 1.5), material=dressed)
        for cutter in (hole, cut_below):
            mod = ring.modifiers.new("cut", "BOOLEAN")
            mod.operation, mod.solver, mod.object = "DIFFERENCE", "EXACT", cutter
        cutters += [hole, cut_below]
        parts.append(ring)
        if opening is not L.DOOR:
            sl = ((c - half - 0.1, face - 0.0 * sign, sill - 0.12), (c + half + 0.1, face + 0.08 * sign, sill)) if along_x else \
                 ((face, c - half - 0.1, sill - 0.12), (face + 0.08 * sign, c + half + 0.1, sill))
            lo = [min(a, b) for a, b in zip(*sl)]
            hi = [max(a, b) for a, b in zip(*sl)]
            box("Sill", lo, hi, dressed)

    # Door leaves: a pair of plank doors folded right back against the inside of the gable, open.
    dh = L.DOOR["width"] / 2
    for side in (-1, 1):
        y_lo, y_hi = (dh + 0.03, dh + 0.78) if side > 0 else (-dh - 0.78, -dh - 0.03)
        box(f"DoorLeaf{side}", (L.IN_X - 0.07, y_lo, L.FLOOR_Z), (L.IN_X - 0.01, y_hi, L.FLOOR_Z + 2.3), timber)
    box("DoorStep", (L.OUT_X / 2, -dh - 0.2, -0.2), (L.OUT_X / 2 + 0.45, dh + 0.2, L.FLOOR_Z - 0.02), dressed)

    # --- roof and stack
    slope = (L.OUT_Y / 2 + L.EAVES_OVERHANG) / math.cos(math.radians(L.ROOF_PITCH))
    for side in (-1, 1):
        mid_y = side * (L.OUT_Y / 2 + L.EAVES_OVERHANG) / 2
        mid_z = L.RIDGE_Z - (L.RIDGE_Z - L.EAVES_Z) * (L.OUT_Y / 2 + L.EAVES_OVERHANG) / (L.OUT_Y / 2) / 2 + 0.1
        r = kit.box(f"Roof{side}", (L.OUT_X + 2 * L.VERGE_OVERHANG, slope, 0.16), U(0, mid_y, mid_z),
                    rotation=(L.ROOF_PITCH * side, 0, 0), material=slate)
        parts.append(r)
    box("Ridge", (-L.OUT_X / 2 - L.VERGE_OVERHANG, -0.12, L.RIDGE_Z + 0.02), (L.OUT_X / 2 + L.VERGE_OVERHANG, 0.12, L.RIDGE_Z + 0.22), dressed)
    sx, sy = L.STACK["centre"]
    h, bh = L.STACK["half"], L.STACK["brick_half"]
    box("Stack", (sx - h, sy - h, 0.0), (sx + h, sy + h, L.STACK["brick_from_z"]), stone)
    box("StackBrick", (sx - bh, sy - bh, L.STACK["brick_from_z"]), (sx + bh, sy + bh, L.STACK["top_z"]), brick)
    box("StackCap", (sx - bh - 0.08, sy - bh - 0.08, L.STACK["top_z"] - 0.25), (sx + bh + 0.08, sy + bh + 0.08, L.STACK["top_z"] - 0.1), brick)

    # --- furnishing
    dx, dy = L.DESK["centre"]
    sx_, sy_, sz_ = L.DESK["size"]
    box("DeskTop", (dx - sx_ / 2, dy - sy_ / 2, L.FLOOR_Z + sz_ - 0.05), (dx + sx_ / 2, dy + sy_ / 2, L.FLOOR_Z + sz_), timber)
    box("DeskPedestal", (dx - sx_ / 2, dy - sy_ / 2, L.FLOOR_Z), (dx - sx_ / 2 + 0.42, dy + sy_ / 2, L.FLOOR_Z + sz_ - 0.05), timber)
    for lx in (dx + sx_ / 2 - 0.06,):
        for ly in (dy - sy_ / 2 + 0.06, dy + sy_ / 2 - 0.06):
            box("DeskLeg", (lx - 0.03, ly - 0.03, L.FLOOR_Z), (lx + 0.03, ly + 0.03, L.FLOOR_Z + sz_ - 0.05), timber)
    cx, cy = L.CHAIR["centre"]
    box("ChairSeat", (cx - 0.22, cy - 0.22, L.FLOOR_Z + 0.42), (cx + 0.22, cy + 0.22, L.FLOOR_Z + 0.47), timber)
    box("ChairBack", (cx - 0.22, cy + 0.18, L.FLOOR_Z + 0.47), (cx + 0.22, cy + 0.22, L.FLOOR_Z + 0.95), timber)
    lx, ly, lz = L.LAMP_ON_DESK
    parts.append(kit.cylinder("LampBase", 0.07, 0.10, U(lx, ly, lz + 0.05), material=timber, sides=12))
    parts.append(kit.cylinder("LampGlass", 0.05, 0.18, U(lx, ly, lz + 0.19), material=lamp, sides=12))
    box("Ledger", (lx - 0.55, ly - 0.15, lz), (lx - 0.25, ly + 0.08, lz + 0.03), dressed)

    px, py = L.ROCK_PILE["centre"]
    import random
    rng = random.Random(4)
    for i in range(26):
        a = rng.uniform(0, 2 * math.pi)
        d = rng.uniform(0, L.ROCK_PILE["radius"]) ** 0.8
        x, y = px + math.cos(a) * d, py + math.sin(a) * d
        x, y = min(x, L.IN_X), min(max(y, -L.IN_Y), L.IN_Y - 0.15)
        z = L.FLOOR_Z + L.ROCK_PILE["height"] * max(0.0, 1 - d / L.ROCK_PILE["radius"]) * 0.8
        s = rng.uniform(0.16, 0.32)
        o = kit.sphere("Rock", s, U(max(x, -L.IN_X + s * 0.7), y, z), material=rock, segments=8, rings=5,
                       scale=(1.0, rng.uniform(0.7, 1.0), rng.uniform(0.55, 0.8)))
        parts.append(o)
    # A pick and a shovel leaning by the pile; a couple of crates by the door.
    box("Crate1", (L.IN_X - 1.6, L.IN_Y - 0.65, L.FLOOR_Z), (L.IN_X - 0.95, L.IN_Y - 0.05, L.FLOOR_Z + 0.55), boards)
    box("Crate2", (L.IN_X - 1.5, L.IN_Y - 0.6, L.FLOOR_Z + 0.55), (L.IN_X - 1.05, L.IN_Y - 0.15, L.FLOOR_Z + 0.95), boards)

    # Apply booleans, drop the cutters, join.
    for obj in parts:
        if obj.modifiers:
            for o in bpy.context.scene.objects:
                o.select_set(False)
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            for mod in list(obj.modifiers):
                bpy.ops.object.modifier_apply(modifier=mod.name)
    for c in cutters:
        bpy.data.objects.remove(c, do_unlink=True)
    return kit.join(parts, "SM_MineHouseBlockout", pivot=None, smooth_angle=30.0)
