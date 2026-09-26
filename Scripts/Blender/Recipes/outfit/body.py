"""Body import, closest-surface queries and anatomical landmarks for garment fitting.

Everything here works in Blender world space, meters, Z up, -Y forward (the character
faces -Y; her left side is +X, matching the ``*_l`` bones).
"""
import math
import re

import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

ARM_RE = re.compile(r"^(upperarm|lowerarm|hand|thumb|index|middle|ring|pinky)")
LEG_RE = re.compile(r"^(thigh|calf|foot|ball|bigtoe|indextoe|middletoe|ringtoe|littletoe)")
HEAD_RE = re.compile(r"^(neck|head)")


def import_skeletal(path):
    """Import a UE skeletal-mesh FBX and bake the 0.01 unit empty away, so the armature
    has an identity transform with bones in meters and meshes are its children."""
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(path))
    new = [o for o in bpy.data.objects if o not in before]
    arm = next(o for o in new if o.type == "ARMATURE")
    meshes = [o for o in new if o.type == "MESH"]
    for obj in [arm] + meshes:
        mw = obj.matrix_world.copy()
        obj.parent = None
        obj.matrix_world = mw
    for obj in new:
        if obj.type == "EMPTY":
            bpy.data.objects.remove(obj)
    bpy.ops.object.select_all(action="DESELECT")
    for obj in [arm] + meshes:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for mesh in meshes:
        mesh.parent = arm
        for mod in mesh.modifiers:
            if mod.type == "ARMATURE":
                mod.object = arm
    return arm, meshes


def mesh_arrays(obj, evaluated=True):
    """World-space vertices, vertex normals and triangles of ``obj`` as numpy arrays."""
    dg = bpy.context.evaluated_depsgraph_get()
    src = obj.evaluated_get(dg) if evaluated else obj
    me = src.to_mesh()
    me.calc_loop_triangles()
    n = len(me.vertices)
    co = np.empty(n * 3, np.float32)
    me.vertices.foreach_get("co", co)
    nor = np.empty(n * 3, np.float32)
    me.vertex_normals.foreach_get("vector", nor)
    tris = np.empty(len(me.loop_triangles) * 3, np.int32)
    me.loop_triangles.foreach_get("vertices", tris)
    src.to_mesh_clear()
    m = np.array(obj.matrix_world, dtype=np.float64)
    verts = co.reshape(-1, 3).astype(np.float64) @ m[:3, :3].T + m[:3, 3]
    nrm = nor.reshape(-1, 3).astype(np.float64) @ m[:3, :3].T
    nrm /= np.maximum(np.linalg.norm(nrm, axis=1, keepdims=True), 1e-12)
    return verts, nrm, tris.reshape(-1, 3)


def group_matrix(obj):
    names = [g.name for g in obj.vertex_groups]
    w = np.zeros((len(obj.data.vertices), len(names)), np.float32)
    for v in obj.data.vertices:
        for g in v.groups:
            w[v.index, g.group] = g.weight
    return names, w


class Surface:
    """Closest-point / ray queries against a (sub)set of a triangle mesh with smooth normals."""

    def __init__(self, verts, normals, tris):
        self.V, self.N, self.T = verts, normals, tris
        self.bvh = BVHTree.FromPolygons(verts.tolist(), tris.tolist(), all_triangles=True)

    def nearest(self, pts):
        loc = np.empty_like(pts)
        idx = np.empty(len(pts), np.int64)
        find = self.bvh.find_nearest
        for i, p in enumerate(pts):
            hit = find(Vector(p))
            loc[i] = hit[0]
            idx[i] = hit[2]
        tri = self.T[idx]
        a, b, c = self.V[tri[:, 0]], self.V[tri[:, 1]], self.V[tri[:, 2]]
        v0, v1, v2 = b - a, c - a, loc - a
        d00 = (v0 * v0).sum(1); d01 = (v0 * v1).sum(1); d11 = (v1 * v1).sum(1)
        d20 = (v2 * v0).sum(1); d21 = (v2 * v1).sum(1)
        den = np.maximum(d00 * d11 - d01 * d01, 1e-20)
        bv = np.clip((d11 * d20 - d01 * d21) / den, 0, 1)
        bw = np.clip((d00 * d21 - d01 * d20) / den, 0, 1)
        bu = np.clip(1 - bv - bw, 0, 1)
        nrm = (bu[:, None] * self.N[tri[:, 0]] + bv[:, None] * self.N[tri[:, 1]]
               + bw[:, None] * self.N[tri[:, 2]])
        nrm /= np.maximum(np.linalg.norm(nrm, axis=1, keepdims=True), 1e-12)
        return loc, nrm, idx

    def signed(self, pts):
        loc, nrm, idx = self.nearest(pts)
        return ((pts - loc) * nrm).sum(1), loc, nrm

    def ray(self, origin, direction, dist=5.0):
        hit = self.bvh.ray_cast(Vector(origin), Vector(direction).normalized(), dist)
        if hit[0] is None:
            return None, None
        loc = np.array(hit[0])
        _, nrm, _ = self.nearest(loc[None])
        return loc, nrm[0]

    def subset(self, face_mask):
        return Surface(self.V, self.N, self.T[face_mask])


class Body:
    """The heroine's body in bind pose, with region-restricted surfaces and landmarks."""

    def __init__(self, arm, obj, extra=()):
        """``extra`` meshes (the MetaHuman face/head, which owns the neck and the tops of the
        shoulders) join the collision and projection surfaces but not the landmarks."""
        self.arm, self.obj, self.extra = arm, obj, list(extra)
        self.V, self.N, self.T = mesh_arrays(obj)
        self.n_body = len(self.V)
        names, w = group_matrix(obj)
        total = np.maximum(w.sum(1), 1e-6)

        def share(pred):
            idx = [i for i, n in enumerate(names) if pred(n)]
            return w[:, idx].sum(1) / total

        self.arm_share = share(lambda n: bool(ARM_RE.match(n)))
        self.leg_share = share(lambda n: bool(LEG_RE.match(n)))
        self.head_share = share(lambda n: bool(HEAD_RE.match(n)))
        for ex in self.extra:
            v, n, t = mesh_arrays(ex)
            t = t + len(self.V)
            self.V, self.N, self.T = np.vstack([self.V, v]), np.vstack([self.N, n]), np.vstack([self.T, t])
            z = np.zeros(len(v))
            self.arm_share = np.concatenate([self.arm_share, z])
            self.leg_share = np.concatenate([self.leg_share, z])
            self.head_share = np.concatenate([self.head_share, z + 1])
        self.all = Surface(self.V, self.N, self.T)
        tri_arm = self.arm_share[self.T].mean(1)
        cx = self.V[self.T][:, :, 0].mean(1)
        self.torso = self.all.subset(tri_arm < 0.5)
        self.leg_l = self.all.subset((tri_arm < 0.5) & (cx > 0.0))
        self.leg_r = self.all.subset((tri_arm < 0.5) & (cx < 0.0))
        self.bones = {b.name: (np.array(arm.matrix_world @ b.head_local),
                               np.array(arm.matrix_world @ b.tail_local)) for b in arm.data.bones}
        self.lm = self.landmarks()

    def torso_mask(self):
        return (self.arm_share < 0.3) & (self.leg_share < 0.5) & (self.head_share < 0.5)

    def center_y(self, z):
        """Mid-depth of the torso at height z (axis for cylindrical ray casts)."""
        m = (self.arm_share < 0.3) & (np.abs(self.V[:, 0]) < 0.06)
        sel = m & (np.abs(self.V[:, 2] - z) < 0.008)
        ys = self.V[sel, 1]
        return float(0.5 * (ys.min() + ys.max())) if len(ys) else 0.0

    def cyl_hit(self, theta_deg, z, surf=None, yc=None):
        """Outermost surface point at height z in direction theta (0 = front, +90 = her left)."""
        surf = surf or self.torso
        t = math.radians(theta_deg)
        d = np.array([math.sin(t), -math.cos(t), 0.0])
        yc = self.center_y(z) if yc is None else yc
        origin = np.array([0.0, yc, z]) + d * 0.6
        return surf.ray(origin, -d)

    def front_profile(self, x, zs):
        """Frontmost torso y at each height along a vertical line at x (rays cast toward +Y)."""
        out = []
        for z in zs:
            loc, _ = self.torso.ray((x, -0.6, z), (0, 1, 0))
            out.append(loc[1] if loc is not None else np.nan)
        return np.array(out)

    def landmarks(self):
        V = self.V
        lm = {}
        b = self.bones
        lm["jugular_z"] = float(b["clavicle_l"][0][2]) + 0.004
        lm["shoulder_x"] = float(b["upperarm_l"][0][0])
        zs = np.arange(0.95, 1.18, 0.002)
        yf = self.front_profile(0.0, zs)
        k = 9
        dip = np.full_like(yf, -1.0)
        dip[k:-k] = yf[k:-k] - 0.5 * (yf[:-2 * k] + yf[2 * k:])
        i = int(np.argmax(dip))
        lm["navel"] = [0.0, float(yf[i]), float(zs[i])]
        lm["navel_dip_mm"] = float(dip[i] * 1000)
        tm = self.torso_mask()
        sel = tm & (V[:, 0] > 0.03) & (V[:, 0] < 0.16) & (V[:, 2] > 1.12) & (V[:, 2] < 1.42)
        j = np.where(sel)[0][np.argmin(V[sel, 1])]
        apex = V[j]
        lm["apex"] = apex.tolist()
        zz = np.arange(apex[2] - 0.16, apex[2], 0.003)
        prof = self.front_profile(apex[0], zz)
        slope = np.gradient(prof, zz)
        fold = apex[2] - 0.08
        for z, s in zip(zz[::-1], slope[::-1]):
            if z < apex[2] - 0.03 and s > -0.35:
                fold = z
                break
        lm["underbust_z"] = float(fold)
        tr = (self.arm_share > 0.3) & (self.arm_share < 0.7) & (V[:, 0] > 0.05) & (V[:, 2] > 1.1)
        lm["armpit_z"] = float(V[tr, 2].min()) if tr.any() else float(b["upperarm_l"][0][2]) - 0.10
        zn = lm["jugular_z"] + 0.025
        loc, _ = self.torso.ray((0.5, self.center_y(zn), zn), (-1, 0, 0))
        lm["neck_x"] = float(loc[0]) if loc is not None and loc[0] < 0.1 else 0.06
        best = None
        for y in np.linspace(-0.08, 0.06, 57):
            loc, _ = self.all.ray((0.0, y, 0.35), (0, 0, 1))
            if loc is not None and loc[2] < 1.0 and (best is None or loc[2] < best[2]):
                best = loc
        lm["perineum"] = best.tolist()
        lm["hip_joint_z"] = float(b["thigh_l"][0][2])
        lm["height_body_top"] = float(V[:, 2].max())
        return lm

    def shoulder_crest(self, x):
        """Top of the shoulder at x, midway between the front and back of the shoulder."""
        z0 = self.lm["armpit_z"] + 0.06
        front, _ = self.torso.ray((x, -0.6, z0), (0, 1, 0))
        back, _ = self.torso.ray((x, 0.6, z0), (0, -1, 0))
        y = 0.5 * (front[1] + back[1]) if front is not None and back is not None else 0.0
        loc, _ = self.torso.ray((x, y, 1.9), (0, 0, -1))
        return loc

    def thigh_axis(self, side, z):
        head, _ = self.bones[f"thigh_{side}"]
        knee, _ = self.bones[f"calf_{side}"]
        t = (z - head[2]) / (knee[2] - head[2])
        return head + (knee - head) * t
