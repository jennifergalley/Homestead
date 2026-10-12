"""Shallow-level GREYBOX (flat-grey tunnels for layout review and Gameplay's first tests; not final art).

    .\\Scripts\\Blender\\New-Prop.ps1 mine_tunnels_blockout -NoBeauty

The level's air volume from ``mine/tunnels.py`` meshed coarsely (0.2 m voxels) and seen from
inside, with block timber sets, rails, the cart and the sump's water. Pivot = the tunnel frame's
origin (the ladder foot); place it at ``layout.tunnel_world()`` with the house's yaw.
"""
import math
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
from mine import layout as L  # noqa: E402
from mine import tunnels  # noqa: E402

import homestead_sdf as sdf  # noqa: E402

NAME = "MineTunnelsBlockout"
DESCRIPTION = "Greybox of the mine's shallow level: galleries, drifts, ladder chamber and shaft, sump."
COLLISION = "none"          # import_props.COLLISION_OVERRIDES gives it complex-as-simple
TRIANGLE_BUDGET = 400000
PROVENANCE = "Original project-authored blockout geometry."
REPORT = {"pivot": "Tunnel frame origin (ladder foot), Unreal frame; place at layout.tunnel_world(), yaw 0."}


def U(x, y, z):
    return (x, -y, z)


def mirror_y(obj):
    """Unreal frame -> Blender frame. Negating Y also reverses the winding, which turns the
    SDF's outward (into-rock) normals into the air, as a level seen from inside needs."""
    for v in obj.data.vertices:
        v.co.y = -v.co.y
    obj.data.update()


def build(kit):
    rock = kit.material("M_MineGreyRock", (0.30, 0.30, 0.29), roughness=0.95)
    timber = kit.material("M_MineGreyTimber", (0.24, 0.15, 0.08), roughness=0.85)
    iron = kit.material("M_MineGreyIron", (0.12, 0.10, 0.09), roughness=0.6)
    water = kit.material("M_MineGreyWater", (0.02, 0.03, 0.03), roughness=0.1)
    node_mat = {0: kit.material("M_MineGreyRockFace", (0.45, 0.43, 0.40), roughness=0.9),
                1: kit.material("M_MineGreyCoal", (0.03, 0.03, 0.03), roughness=0.6),
                2: kit.material("M_MineGreyCopper", (0.15, 0.35, 0.28), roughness=0.7),
                3: kit.material("M_MineGreyGem", (0.45, 0.25, 0.60), roughness=0.3)}

    shell = sdf.mesh("Shell", tunnels.air(), voxel=0.2, material=rock)
    mirror_y(shell)
    parts = [shell]

    def box(name, lo, hi, mat, rot_z=0.0, pivot=None):
        c = [(a + b) / 2 for a, b in zip(lo, hi)]
        s = [abs(b - a) for a, b in zip(lo, hi)]
        o = kit.box(name, s, U(*c), material=mat)
        if rot_z:
            px, py = pivot
            o.location = U(*c)
            # rotate about Z round the pivot (Blender frame: yaw is negated by the Y mirror)
            ang = -math.radians(rot_z)
            ox, oy = c[0] - px, c[1] - py
            rx, ry = ox * math.cos(math.radians(rot_z)) - oy * math.sin(math.radians(rot_z)), \
                ox * math.sin(math.radians(rot_z)) + oy * math.cos(math.radians(rot_z))
            o.location = U(px + rx, py + ry, c[2])
            o.rotation_euler = (0.0, 0.0, ang)
        parts.append(o)
        return o

    def along(edge, t, local, size, mat, name):
        """A box at arc fraction t of an edge, ``local`` = (along, left, z) offset, axis-aligned to the edge."""
        a, b, (ux, uy), (lx, ly), length = L.edge_frame(edge)
        x = a[0] + ux * length * t + ux * local[0] + lx * local[1]
        y = a[1] + uy * length * t + uy * local[0] + ly * local[1]
        o = kit.box(name, size, U(x, y, local[2]), material=mat)
        o.rotation_euler = (0.0, 0.0, -math.atan2(uy, ux))
        parts.append(o)
        return o

    # Timber sets (two legs and a cap) along the adit and at the drift mouths.
    def timber_set(edge, t, half, height):
        along(edge, t, (0, half - 0.18, height / 2 - 0.05), (0.22, 0.22, height), timber, "Leg")
        along(edge, t, (0, -half + 0.18, height / 2 - 0.05), (0.22, 0.22, height), timber, "Leg")
        along(edge, t, (0, 0, height + 0.06), (0.24, 2 * half - 0.1, 0.24), timber, "Cap")

    adit = ("L", "A")
    for t in (0.30, 0.45, 0.60, 0.75):
        timber_set(adit, t, L.GALLERY_HALF - 0.05, L.GALLERY_WALL_H - 0.1)
    for f, t_, kind in L.EDGES:
        if kind == "drift":
            timber_set((f, t_), 0.16, L.DRIFT_HALF - 0.05, L.DRIFT_WALL_H - 0.1)

    # Rails along the haulage way (two flat bars on sleepers).
    route = L.RAILS
    for i in range(len(route) - 1):
        edge = (route[i], route[i + 1])
        a, b, u, l, length = L.edge_frame(edge)
        mid = 0.5
        for side in (-0.30, 0.30):
            along(edge, mid, (0, side, 0.06), (length, 0.05, 0.07), iron, "Rail")
        for k in range(int(length / 0.9)):
            along(edge, (k + 0.5) * 0.9 / length, (0, 0, 0.03), (0.16, 0.95, 0.06), timber, "Sleeper")

    # The ore cart.
    along(L.CART["edge"], L.CART["t"], (0, 0, 0.55), (1.20, 0.80, 0.70), iron, "Cart")

    # The sump's water and its kerb rail.
    sx, sy = L.SUMP["centre"]
    hx, hy = L.SUMP["half"]
    box("Water", (sx - hx - 0.1, sy - hy - 0.1, -0.22), (sx + hx + 0.1, sy + hy + 0.1, -0.20), water)

    # Node placeholders (Gameplay spawns the real nodes; these show where the sockets are).
    for nid, kind, (x, y), yaw, (nx, ny) in L.node_sockets():
        size = {0: (0.5, 1.4, 1.5), 1: (0.3, 1.2, 0.5), 2: (0.3, 0.9, 0.9), 3: (0.3, 0.5, 0.5)}[kind]
        o = kit.box(f"Node{nid}", size, U(x - nx * size[0] / 2 + nx * 0.1, y - ny * size[0] / 2 + ny * 0.1, 0.4 + size[2] / 2),
                    material=node_mat[kind])
        o.rotation_euler = (0.0, 0.0, -math.atan2(ny, nx))
        parts.append(o)

    return kit.join(parts, "SM_MineTunnelsBlockout", pivot=None, smooth_angle=40.0)
