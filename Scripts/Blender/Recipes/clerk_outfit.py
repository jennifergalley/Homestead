"""Shopkeeper's outfit for Mr. Callum Aldridge (the Homestead clerk MetaHuman): linen shirt with
rolled sleeves, wool waistcoat, wool trousers, canvas bib apron, madder neckerchief, leather ankle
boots, plus a carpenter's pencil prop and its behind-the-ear fit.

Original procedural work: every garment is generated here from the clerk body's own landmarks,
reusing the heroine wardrobe pipeline (``clothing_wardrobe`` + ``outfit``) without changing it.
The heroine's cord-belt contour is replaced by a synthetic waist line from the clerk's landmarks.

Usage (repo root, PowerShell):
  $S='E:\\CopilotScratch\\fa19573e-88a5-4c54-ada3-3f9a07de5fc2'; $env:TEMP="$S\\tmp"; $env:TMP="$S\\tmp"
  & "C:\\Program Files\\Blender Foundation\\Blender 5.2\\blender.exe" --background --factory-startup --python Scripts\\Blender\\Recipes\\clerk_outfit.py -- --stage all

Stages: ``fit`` (fit every garment and write quick colour previews to WORK) or ``all`` (fit, then
build: textures, weights, thickness, exports, metrics, masks, pencil, report, final previews).
The fit is deterministic (seeded), so ``all`` always refits. ``--reuse-textures`` skips the
cloth texture synthesis when WORK already holds the cached map paths.
"""
import argparse
import datetime
import json
import math
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Matrix, Vector

HERE = Path(__file__).resolve()
sys.path.insert(0, str(HERE.parent))

import clothing_wardrobe as CW  # noqa: E402  (main() is guarded)
from outfit import body as B, layers as Ly, tops as Tp, trousers as Tr, rig as R  # noqa: E402
from outfit import wardrobe_rig as WR, wardrobe_tex as WT, wardrobe_detail as WD, detail as D  # noqa: E402
from outfit.garments import boundary_loops, edges_of  # noqa: E402
from clerk_parts import shapes as S, texture as TX  # noqa: E402

ROOT = HERE.parents[3]
SCRATCH = Path(r"E:\CopilotScratch\fa19573e-88a5-4c54-ada3-3f9a07de5fc2")
INPUT_DIR = SCRATCH / "MetaHumanBody"
BODY_FBX = INPUT_DIR / "SKM_MHC_Clerk_BodyMesh_Full.fbx"
FACE_FBX = INPUT_DIR / "SKM_MHC_Clerk_FaceMesh.fbx"
OUT = ROOT / "Assets" / "Characters" / "ClerkClothing"
WORK = SCRATCH / "clerk_garments"
PREVIEWS = WORK / "previews"
SEED = 20261004
DATE = "2026-10-04"

SHIRT, WAISTCOAT, TROUSERS = "SKM_ClerkShirt", "SKM_ClerkWaistcoat", "SKM_ClerkTrousers"
APRON, NECKERCHIEF, BOOTS, PENCIL = "SKM_ClerkApron", "SKM_ClerkNeckerchief", "SKM_ClerkBoots", "SM_ClerkPencil"
GARMENTS = (BOOTS, TROUSERS, SHIRT, WAISTCOAT, NECKERCHIEF, APRON)

THICKNESS = {  # cloth (m): base, at hems/edges
    SHIRT: (0.0010, 0.0024), WAISTCOAT: (0.0025, 0.0040), TROUSERS: (0.0019, 0.0034),
    APRON: (0.0018, 0.0030), NECKERCHIEF: (0.0012, 0.0020)}
BOOT_LEATHER = 0.0025       # m, boot shell thickness
BOOT_TOP_Z = 0.15           # m, boot shaft top (trousers end over it)
BOOT_PUSH = 0.0045          # m, boot last offset from the skin before the leather shell
SHIRT_HEM_Z = 1.005         # m, shirt hem at centre front/back (sides +1 cm), tucked under the waistcoat
NK_Z_FRONT = 1.445          # m, neckerchief lower edge at the throat (min); elsewhere it follows the neck base
NK_LIFT = 0.004             # m, neckerchief lower edge above the neck base (over the collar band)
NK_HEIGHT = 0.032           # m, folded neckerchief band height
BOOT_LAST_ROUNDS = 25       # smooth + collide rounds that round the toes into a boot last
BOOT_DECIMATE = 0.5         # boot shell triangle ratio (the feet are rarely seen)
BOOT_HULL_Z = 0.085         # m, foot hull (toe box / boot last) is taken below this height
BOOT_HULL_OFF = 0.004       # m, boot last outside the foot hull
# grid pitches (m), coarser than the heroine's so the whole outfit stays near 110k triangles
SHIRT_PITCH = 0.011
WAISTCOAT_PITCH = 0.0115
TROUSERS_PITCH = 0.0135
PENCIL_TILT_DEG = 16.0      # deg, preferred forward-down tilt
PENCIL_TILTS = (10.0, 16.0, 22.0)   # deg, searched forward-down tilts
PENCIL_YAWS = (0.0, 5.0, 10.0)      # deg, searched tip-outward yaws
PENCIL_BACK = (0.015, 0.025, 0.035)  # m, pencil centre behind the ear top (tip ends near the temple)
PENCIL_CLEAR = 0.0045       # m, pencil centreline to skin (0.4 cm radius + 0.5 mm)

# linear base colours: shirt #D8CFBC, waistcoat #3B2A1E, trousers #2E2A26, apron #9C8F74, madder #8A3B2E
LIN = dict(shirt=(0.686, 0.624, 0.503), waistcoat=(0.044, 0.023, 0.012), trousers=(0.027, 0.023, 0.019),
           apron=(0.332, 0.275, 0.175), neckerchief=(0.254, 0.044, 0.027))

STYLES = {
    SHIRT: dict(fabric="linen", base=LIN["shirt"], thread=(0.60, 0.56, 0.47), warp=14.0, weft=13.0, streak=0.02,
                fade=0.05, seams={"hem": ("roll", 0.9, [0.62]), "cuff": ("roll", 0.8, [0.55]),
                                  "collartop": ("bind", 0.9, [0.2, 0.7]), "neck": ("bind", 0.9, [0.2, 0.7]),
                                  "side": ("felled", 0.8, [0.15, 0.65]), "shoulder": ("felled", 0.8, [0.15, 0.65]),
                                  "armhole": ("felled", 0.8, [0.15, 0.65])},
                grime=dict(collartop=0.10, cuff=0.10, hem=0.05), yellow=0.3, dust=0.03),
    WAISTCOAT: dict(fabric="wool", base=LIN["waistcoat"], thread=(0.075, 0.052, 0.036), warp=9.0, weft=8.5,
                    streak=0.02, fade=0.04, heather=0.15, wool_wear=False, dust=0.02,
                    seams={"edge": ("bind", 1.2, [0.3, 1.0]), "armhole": ("bind", 1.0, [0.3, 0.8])},
                    grime={}, yellow=0.0),
    TROUSERS: dict(fabric="wool", base=LIN["trousers"], thread=(0.060, 0.052, 0.045), warp=7.5, weft=7.0,
                   streak=0.03, fade=0.06, heather=0.18, dust=0.05,
                   seams={"waist": ("casing", 3.0, [0.35, 2.8]), "leghem": ("turn", 2.4, [1.8]),
                          "side": ("felled", 0.9, [0.15, 0.75]), "inseam": ("felled", 0.9, [0.15, 0.75]),
                          "crotch": ("felled", 0.9, [0.15, 0.75])},
                   grime=dict(leghem=0.18), yellow=0.0),
    APRON: dict(fabric="linen", base=LIN["apron"], thread=(0.30, 0.255, 0.17), warp=9.0, weft=8.0, streak=0.03,
                fade=0.04, seams={"hem": ("turn", 1.6, [1.2]), "side": ("roll", 1.0, [0.7]),
                                  "bibtop": ("turn", 1.4, [1.0]), "pocket": ("felled", 0.8, [0.3]),
                                  "pocketedge": ("turn", 1.2, [0.9])},
                grime=dict(hem=0.30, pocketedge=0.25), yellow=0.0, dust=0.0),
    NECKERCHIEF: dict(fabric="linen", base=LIN["neckerchief"], thread=(0.21, 0.04, 0.025), warp=20.0, weft=19.0,
                      streak=0.02, fade=0.06, seams={"hem": ("roll", 0.6, [0.4])}, grime={}, yellow=0.0,
                      dust=0.0),
}

PREVIEW_COLORS = {SHIRT: (0.85, 0.81, 0.72, 1), WAISTCOAT: (0.23, 0.16, 0.11, 1), TROUSERS: (0.18, 0.165, 0.15, 1),
                  APRON: (0.61, 0.56, 0.45, 1), NECKERCHIEF: (0.54, 0.23, 0.18, 1), BOOTS: (0.12, 0.09, 0.07, 1),
                  PENCIL: (0.55, 0.36, 0.2, 1)}

# reserved UV strip (v < D.STRIP) slots for small added parts (u0, u1, v0, v1)
STRIP_V = (0.004, 0.026, 0.030, 0.052)


def log(msg):
    CW.log(msg)


def gdir(name):
    return OUT / name.replace("SKM_", "").replace("SM_", "")


class WaistLine:
    """Stand-in for the heroine's cord belt: only used for top_params' last line; the clerk's
    hems are set explicitly from his landmarks afterwards."""
    radius = 0.0

    def z(self, t):
        return 0.0


def full(n, v):
    return np.full(n, float(v))


# ------------------------------------------------------------------- inputs

def load_inputs():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    arm, meshes = B.import_skeletal(BODY_FBX)
    arm.name = "root"
    body_obj = meshes[0]
    body_obj.name = "SKM_MHC_Clerk_BodyMesh"
    face_arm, fm = B.import_skeletal(FACE_FBX)
    face_arm.name = "FaceRig"
    face_obj = fm[0]
    face_obj.name = "SKM_MHC_Clerk_FaceMesh"
    body = B.Body(arm, body_obj, extra=[face_obj])
    return arm, body_obj, face_obj, body


# ---------------------------------------------------------------------- fit

def foot_hull_planes(body):
    """Outward face planes (normals, offsets, centre) of the convex hull of each foot below BOOT_HULL_Z."""
    import bmesh
    BV = body.V[:body.n_body]
    out = []
    for sgn in (1.0, -1.0):
        P = BV[(BV[:, 2] < BOOT_HULL_Z) & (BV[:, 0] * sgn > 0.02)]
        bm = bmesh.new()
        for p in P:
            bm.verts.new(p.tolist())
        bmesh.ops.convex_hull(bm, input=bm.verts, use_existing_faces=False)
        c = P.mean(0)
        Ns, Ds = [], []
        for f in bm.faces:
            if len(f.verts) < 3:
                continue
            fc = np.array(f.calc_center_median())
            nrm = np.array(f.normal)
            if np.linalg.norm(nrm) < 1e-9:
                continue
            if nrm @ (fc - c) < 0:
                nrm = -nrm
            Ns.append(nrm)
            Ds.append(nrm @ fc)
        bm.free()
        out.append((sgn, np.array(Ns), np.array(Ds)))
    return out


def hull_push(V, planes, off, w):
    """Push points inside a foot hull (closer than ``off`` to its surface) out along the nearest face."""
    V = V.copy()
    for sgn, Ns, Ds in planes:
        sel = np.where((V[:, 0] * sgn > 0) & (w > 0))[0]
        if not len(sel):
            continue
        S_ = V[sel] @ Ns.T - Ds
        k = np.argmax(S_, 1)
        s = S_[np.arange(len(sel)), k]
        need = np.clip(off - s, 0, None) * w[sel]
        V[sel] += Ns[k] * need[:, None]
    return V


def fit_boots(body, body_obj):
    import bmesh
    obj, used = S.boots_mesh(BOOTS, body_obj, z_top=BOOT_TOP_Z, push=BOOT_PUSH, smooth_iters=30)
    obj.matrix_world = body_obj.matrix_world.copy()
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0006)
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context="VERTS")
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=0.0002)
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    dec = obj.modifiers.new("Budget", "DECIMATE")
    dec.ratio = BOOT_DECIMATE
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=dec.name)
    bpy.context.view_layer.update()
    V, _, _ = B.mesh_arrays(obj, evaluated=False)
    # alternate smoothing with collision so the shell bridges between the toes into a boot last
    # (smoothing alone then one collide just re-forms the toes)
    me = obj.data
    E = np.array([e.vertices[:] for e in me.edges], np.int64)
    cnt = np.zeros(len(V))
    np.add.at(cnt, E[:, 0], 1)
    np.add.at(cnt, E[:, 1], 1)
    edge_users = {}
    for p in me.polygons:
        for ek in p.edge_keys:
            edge_users[ek] = edge_users.get(ek, 0) + 1
    pinned = np.zeros(len(V), bool)
    for (a, b), u in edge_users.items():
        if u < 2:
            pinned[a] = pinned[b] = True
    body_c = [(Ly.ClosedSurface(body.all), full(len(V), 0.004), None)]
    # boot last: the convex hull of each foot (bridges the toes into a toe box), pushed out to
    # BOOT_HULL_OFF and blended away towards the ankle; Laplacian smoothing between pushes
    planes = foot_hull_planes(body)
    wz = np.clip((BOOT_HULL_Z + 0.01 - V[:, 2]) / 0.03, 0.0, 1.0)
    for _ in range(BOOT_LAST_ROUNDS):
        for _ in range(3):
            acc = np.zeros_like(V)
            np.add.at(acc, E[:, 0], V[E[:, 1]])
            np.add.at(acc, E[:, 1], V[E[:, 0]])
            L = acc / np.maximum(cnt, 1)[:, None] - V
            L[pinned] = 0.0
            V = V + 0.6 * L * wz[:, None]
        V = hull_push(V, planes, BOOT_HULL_OFF, wz)
    V = Ly.collide_layers(V, body_c, rounds=3)
    Mi = np.array(obj.matrix_world.inverted())
    CW.set_verts(obj, V @ Mi[:3, :3].T + Mi[:3, 3])
    obj.data.update()
    mod = obj.modifiers.new("Leather", "SOLIDIFY")
    mod.thickness = BOOT_LEATHER
    mod.offset = 1.0
    mod.use_even_offset = False
    mod.use_quality_normals = True
    mod.use_rim = True
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=mod.name)
    gap = body.all.signed(B.mesh_arrays(obj, evaluated=False)[0])[0]
    log(f"{BOOTS}: {len(obj.data.vertices)} verts, z top {V[:, 2].max():.3f}, skin gap mm min {1000 * gap.min():.2f}")
    return obj


def fit_trousers(body, layers):
    p = Tr.trousers_params(body)
    p.update(name=TROUSERS, pitch=TROUSERS_PITCH, waist_front_z=body.lm["navel"][2] - 0.010)
    g = Tr.build_trousers(body, p)
    V, F = g.arrays()
    # the waist ring scan can catch the A-pose hands at the hips: pull those verts back in
    V, nsp = despike(V, F, zmax=1.25, thresh=0.03, rounds=12)
    log(f"{TROUSERS}: despiked {nsp} waist verts")
    F = CW.orient_out(V, F, body.all)
    loops = boundary_loops(F, len(V))
    fixed = np.zeros(len(V), bool)
    for lp in loops:
        fixed[lp] = True
    lab = Tr.trouser_labels(g, V)
    off = Tr.trouser_offsets(g, V, body)
    # fairly straight legs: ease grows from the knee to the ankle so the leg doesn't taper
    kz, hz = p["knee_z"], p["hem_z"]
    off = off + 0.015 * np.clip((kz - V[:, 2]) / (kz - hz), 0, 1) ** 1.2 * (lab > 0)
    # the hem falls over the boot shafts: clear boot leather (push + thickness) with room to spare
    off = np.where(V[:, 2] < BOOT_TOP_Z + 0.03, np.maximum(off, BOOT_PUSH + BOOT_LEATHER + 0.006), off)
    n = len(V)
    cons = [(CW.Labelled([body.torso, body.leg_l, body.leg_r], lab), off, None),
            (Tr.LegDrape(body, p, "l"), np.zeros(n), lab == 1),
            (Tr.LegDrape(body, p, "r"), np.zeros(n), lab == 2),
            (Tr.Plane(1.0, 0.0022), np.zeros(n), lab == 1),
            (Tr.Plane(-1.0, 0.0022), np.zeros(n), lab == 2)]
    for lay, loff in layers:
        cons.append((lay, full(n, loff), None))
    V, _ = Ly.relax_layers(V, F, fixed, cons, iters=170, log=log)
    V = CW.smooth_edges(V, F, loops, fixed, cons)
    V, nsp = despike(V, F, zmax=1.25, thresh=0.03, rounds=12)
    if nsp:
        log(f"{TROUSERS}: despiked {nsp} verts after the solve")
    gap = cons[0][0].signed(V)[0]
    g.extra.update(labels=lab, loops=loops, gap=gap, offsets=off, constraints=cons)
    log(f"{g.name}: {len(V)} verts, {len(F)} quads, gap mm min {1000 * gap.min():.2f} median {1000 * np.median(gap):.2f}")
    return g, V, F


def shirt_params(body):
    p = Tp.top_params(body, "longshirt", WaistLine())
    p.update(name=SHIRT, pitch=SHIRT_PITCH, slit=None, cuff=None, sleeve_end=0.33, sleeve_ease=0.012, hang=0.6, base=0.0055,
             zc=body.lm["jugular_z"] - 0.008, zcb=body.lm["jugular_z"] + 0.030,
             collar=dict(rows=2, dz=0.012, lean=0.002, off=0.004))
    p["hem"] = {t: SHIRT_HEM_Z + 0.010 * math.sin(math.radians(t)) ** 2 for t in range(-180, 181)}
    return p


def waistcoat_params(body):
    ap = body.lm["armpit_z"]
    p = Tp.top_params(body, "tee", WaistLine())
    p.update(name=WAISTCOAT, pitch=WAISTCOAT_PITCH, x_in=0.075, x_out=0.13, ua_z=ap - 0.045, hang=0.35, base=0.004, sleeve_end=0.06,
             sleeve_ease=0.012, opening=True, open_gap=0.003, facing=0.03, collar=None, slit=None, cuff=None)
    p["hem"] = {t: float(np.interp(abs(t), [0, 8, 25, 90, 180], [0.945, 0.955, 0.975, 0.985, 0.995]))
                for t in range(-180, 181)}
    return p


def layer_cons(layers, V, default_mask=None):
    """layers: (surface, offset[, mask_fn]); offset may be a scalar or a function of V."""
    out = []
    for entry in layers:
        lay, loff = entry[0], entry[1]
        o = loff(V) if callable(loff) else full(len(V), loff)
        m = entry[2](V) if len(entry) > 2 else default_mask
        out.append((lay, o, m))
    return out


def despike(V, F, zmax=1.30, thresh=0.007, rounds=8):
    """Pull isolated spike vertices (more than ``thresh`` from their neighbours' mean, below
    ``zmax``) back to that mean. The layer solve leaves a few at the hip where the shirt hem and
    the trousers' waist edge meet."""
    n = len(V)
    E = np.concatenate([F[:, [0, 1]], F[:, [1, 2]], F[:, [2, 3]], F[:, [3, 0]]]) if F.shape[1] == 4 else \
        np.concatenate([F[:, [0, 1]], F[:, [1, 2]], F[:, [2, 0]]])
    E = np.unique(np.sort(E, 1), axis=0)
    V = V.copy()
    total = 0
    for _ in range(rounds):
        acc = np.zeros((n, 3))
        cnt = np.zeros(n)
        np.add.at(acc, E[:, 0], V[E[:, 1]])
        np.add.at(acc, E[:, 1], V[E[:, 0]])
        np.add.at(cnt, E[:, 0], 1)
        np.add.at(cnt, E[:, 1], 1)
        mean = acc / np.maximum(cnt, 1)[:, None]
        bad = (np.linalg.norm(V - mean, axis=1) > thresh) & (V[:, 2] < zmax) & (cnt >= 3)
        if not bad.any():
            break
        total += int(bad.sum())
        V[bad] = mean[bad]
    return V, total


def fit_top(body, p, layers, prof, mask_low=True):
    g = Tp.build_top(body, p)
    V, F = g.arrays()
    F = CW.orient_out(V, F, body.all)
    loops = boundary_loops(F, len(V))
    fixed = np.zeros(len(V), bool)
    for lp in loops:
        fixed[lp] = True
    off = CW.top_offsets(g, V, body, prof)
    n = len(V)
    cons = [(Ly.ClosedSurface(body.all), off, None),
            (Ly.Drape(prof, p["hang"], p["ua_z"] + 0.03, p["base"]), np.zeros(n), g.extra["region"] == 0)]
    low = V[:, 2] < 1.16 if mask_low else None
    cons += layer_cons(layers, V, low)
    V, _ = Ly.relax_layers(V, F, fixed, cons, hem_axis=g.extra["hem_axis"], iters=170, log=log)
    V = CW.smooth_edges(V, F, loops, fixed, cons, hem_axis=g.extra["hem_axis"])
    gap = body.all.signed(V)[0]
    g.extra.update(loops=loops, gap=gap, offsets=off, constraints=cons)
    log(f"{g.name}: {len(V)} verts, {len(F)} quads, body gap mm min {1000 * gap.min():.2f} median {1000 * np.median(gap):.2f}")
    if p.get("opening"):
        n0 = len(V)
        V, F, le, ri = Tp.split_front(g, V, F, -1.0)
        V = Tp.spread_opening(V, le, ri, lambda v: p["open_gap"])
        g.extra["opening"] = (le, ri)
        src = np.concatenate([np.arange(n0), np.array(le, np.int64)])
        for k in ("region", "axial", "hem_axis", "gap", "offsets"):
            g.extra[k] = np.asarray(g.extra[k])[src]
        g.extra["constraints"] = [(s, np.asarray(o)[src], None if m is None else m[src]) for s, o, m in cons]
        g.extra["src_map"] = src
    g.extra["loops"] = boundary_loops(F, len(V))
    return g, V, F


def drop_faces(g, V, F, keep_face, keys=("region", "axial", "hem_axis", "gap", "offsets")):
    used = np.unique(F[keep_face])
    remap = -np.ones(len(V), np.int64)
    remap[used] = np.arange(len(used))
    F2 = remap[F[keep_face]]
    g.face_panel = [g.face_panel[i] for i in np.where(keep_face)[0]]
    g.uv_seams = {tuple(sorted((int(remap[a]), int(remap[b])))) for a, b in g.uv_seams
                  if remap[a] >= 0 and remap[b] >= 0}
    for k in keys:
        if k in g.extra:
            g.extra[k] = np.asarray(g.extra[k])[used]
    if "opening" in g.extra:
        le, ri = g.extra["opening"]
        g.extra["opening"] = ([int(remap[v]) for v in le if remap[v] >= 0], [int(remap[v]) for v in ri if remap[v] >= 0])
    return V[used].copy(), F2, remap


def fit_waistcoat(body, layers, prof):
    p = waistcoat_params(body)
    g, V, F = fit_top(body, p, layers, prof, mask_low=False)
    reg = g.extra["region"]
    cent = V[F].mean(1)
    cy = np.array([body.center_y(float(z)) for z in cent[:, 2]])
    z_cut = 1.27 + 0.19 * np.abs(cent[:, 0]) / 0.075
    front_v = (cent[:, 1] < cy) & (cent[:, 2] > z_cut)
    sleeve = (reg[F] != 0).any(1)
    keep = ~(front_v | sleeve)
    V, F, _ = drop_faces(g, V, F, keep)
    n = len(V)
    loops = boundary_loops(F, n)
    fixed = np.zeros(n, bool)
    for lp in loops:
        fixed[lp] = True
    off = np.maximum(np.asarray(g.extra["offsets"]), p["base"])
    cons = [(Ly.ClosedSurface(body.all), off, None)] + layer_cons(layers, V)
    V = CW.smooth_edges(V, F, loops, fixed, cons, iters=40)
    V = Ly.collide_layers(V, cons, rounds=3)
    V, nsp = despike(V, F)
    V = Ly.collide_layers(V, cons, rounds=2)
    V, nsp2 = despike(V, F, thresh=0.009)
    log(f"{WAISTCOAT}: despiked {nsp}+{nsp2} verts")
    g.extra.update(loops=loops, constraints=cons, gap=body.all.signed(V)[0], offsets=off)
    log(f"{WAISTCOAT}: trimmed to {n} verts, {len(F)} quads, loops {[len(l) for l in loops]}")
    return g, V, F


def neck_cloud(body, extra):
    return np.vstack([body.V] + list(extra))


def fit_neckerchief(body, shirt_V, waist_V, cons):
    V, F, G, th = S.neckerchief_ring(shirt_V, body, NK_LIFT, NK_HEIGHT, cols=48, rows=5,
                                     pad=0.004, skin=0.006, reach=0.015, z_front=NK_Z_FRONT)
    F = CW.orient_out(V, F, body.all)
    gap = body.all.signed(V)[0]
    log(f"{NECKERCHIEF}: ring {len(V)} verts, z {V[:, 2].min():.3f}-{V[:, 2].max():.3f}, skin gap min {1000 * gap.min():.1f} mm")
    return V, F, G


def hug_collar(g, V, F, body, loop_off=0.011, row_off=(0.009, 0.007, 0.006), fade=5):
    """Pull the shirt's neckline and stand collar in to a band close round the neck (the shared
    top builder grows the collar from wherever the neckline lands on the trapezius, which on this
    body makes a wide funnel). Radial moves only; the change fades over ``fade`` rings."""
    rows = g.extra["collar"]["rows"]
    delta = {}
    for k, row in enumerate(rows):
        off = loop_off if k == 0 else row_off[min(k - 1, len(row_off) - 1)]
        for i in row:
            v = V[i]
            c = S.neck_axis(body, float(v[2]))
            rel = v[:2] - c
            r = float(np.linalg.norm(rel))
            th = math.degrees(math.atan2(rel[0], -rel[1]))
            rt = S.neck_radius(body, float(v[2]), th) + off
            if k == 0:
                rt = min(r, rt)
            nv = v.copy()
            nv[:2] = c + rel / max(r, 1e-9) * rt
            delta[i] = nv - v
    for i, dv in delta.items():
        V[i] = V[i] + dv
    nbr = {}
    for f in F:
        for a, b in zip(f, list(f[1:]) + [f[0]]):
            nbr.setdefault(int(a), set()).add(int(b))
            nbr.setdefault(int(b), set()).add(int(a))
    ring = set(rows[0])
    seen = set(i for row in rows for i in row)
    for r in range(1, fade + 1):
        nxt = set()
        for i in ring:
            nxt |= nbr.get(int(i), set())
        nxt -= seen
        w = (1.0 - r / (fade + 1)) ** 1.5
        for j in nxt:
            ds = [delta[k] for k in nbr[j] if k in delta]
            if ds:
                delta[j] = w * np.mean(ds, axis=0)
                V[j] = V[j] + delta[j]
        seen |= nxt
        ring = nxt
    return V


def fit_apron(body, inner, cons_list):
    """inner: list of vertex arrays (shirt, waistcoat, trousers) under the apron."""
    cl = np.vstack([body.V] + inner)
    cl = cl[~((np.abs(cl[:, 0]) > 0.22) & (cl[:, 2] > 0.95))]
    cons = [(s, None, None) for s in cons_list]
    n_dummy = 1
    del n_dummy

    class Cons(list):
        pass

    # constraints sized lazily: build_apron collides the grid with these
    V, F, grid, zs, us = None, None, None, None, None
    nrows = int(round((1.36 - 0.33) / 0.012)) + 1
    nv = nrows * 27
    cc = [(s, full(nv, o), None) for s, o in cons_list]
    V, F, grid, zs, us = S.build_apron(cl, cc, 1.36, 0.33, ncols=27, pitch=0.012, gap=0.010, log=log)
    F = CW.orient_out(V, F, body.all)
    Vp, Fp, gp = S.pocket_patch(V, grid, zs, us, -0.15, -0.02, 0.86, 0.99, lift=0.005, cols=10, rows=10)
    Fp = CW.orient_out(Vp, Fp, body.all)
    del cons
    return V, F, grid, Vp, Fp, gp


# ------------------------------------------------------------------ preview

def quick_preview(show, path, views=None, scale=2.0):
    CW.COLORS.update(PREVIEW_COLORS)
    views = views or (("front", (0, -4, 0.95), (90, 0, 0)), ("side", (4, 0, 0.95), (90, 0, 90)),
                      ("back", (0, 4, 0.95), (90, 0, 180)), ("q34", (2.83, -2.83, 0.95), (90, 0, 45)))
    CW.preview(show, path, views=views, scale=scale)


def setup_render(res=(1200, 1200)):
    sc = bpy.context.scene
    sc.render.engine = "BLENDER_WORKBENCH"
    sc.display.shading.light = "STUDIO"
    sc.display.shading.color_type = "TEXTURE"
    sc.display.shading.show_cavity = True
    sc.display.shading.cavity_type = "WORLD"
    sc.display.shading.show_shadows = True
    sc.render.resolution_x, sc.render.resolution_y = res
    sc.render.film_transparent = False
    cam = bpy.data.objects.get("PreviewCam") or CW.link(bpy.data.objects.new("PreviewCam", bpy.data.cameras.new("PreviewCam")))
    cam.data.type = "ORTHO"
    sc.camera = cam
    return sc, cam


def skin_materials(body_obj, face_obj):
    skin = bpy.data.materials.get("_PreviewSkin") or bpy.data.materials.new("_PreviewSkin")
    skin.diffuse_color = (0.78, 0.58, 0.48, 1)
    for o in (body_obj, face_obj):
        o.data.materials.clear()
        o.data.materials.append(skin)


def render_views(show, prefix, views, scale, res=(1000, 1200)):
    sc, cam = setup_render(res)
    for o in sc.objects:
        if o.type == "MESH":
            o.hide_render = o.name not in show and not o.name.startswith("SKM_MHC")
    cam.data.ortho_scale = scale
    out = []
    for name, loc, rot in views:
        cam.location = loc
        cam.rotation_euler = [math.radians(a) for a in rot]
        sc.render.filepath = str(PREVIEWS / f"{prefix}_{name}.png")
        bpy.ops.render.render(write_still=True)
        out.append(sc.render.filepath)
    return out


FULL_VIEWS = (("front", (0, -4, 0.93), (90, 0, 0)), ("q34", (2.9, -2.9, 0.93), (90, 0, 45)),
              ("side", (4, 0, 0.93), (90, 0, 90)), ("back", (0, 4, 0.93), (90, 0, 180)))
CLOSE_VIEWS = (("torso_front", (0, -3, 1.30), (90, 0, 0)), ("torso_q34", (-2.1, -2.1, 1.30), (90, 0, -45)))
HEAD_VIEWS = (("head_right", (-3, 0, 1.64), (90, 0, -90)), ("head_q34", (-2.1, -2.1, 1.64), (90, 0, -45)))
FEET_VIEWS = (("feet_q34", (-2.1, -2.1, 0.12), (90, 0, -45)),)


# ---------------------------------------------------------------- build helpers

def uv_near(obj, p):
    me = obj.data
    V = np.array([v.co for v in me.vertices])
    i = int(np.argmin(np.linalg.norm(V - np.asarray(p), axis=1)))
    uvl = me.uv_layers.active.data
    for lp in me.loops:
        if lp.vertex_index == i:
            return tuple(uvl[lp.index].uv)
    return (0.5, 0.5)


def strip_slot(k, n=8):
    """k-th of 2*n rectangles in the reserved strip."""
    row, col = divmod(k, n)
    w = 0.98 / n
    u0 = 0.01 + col * w
    v0, v1 = (STRIP_V[0], STRIP_V[1]) if row == 0 else (STRIP_V[2], STRIP_V[3])
    return (u0 + 0.002, u0 + w - 0.002, v0, v1)


def textures(obj, fields, uvpm, short, R_out):
    tdir = OUT / short / "Textures"
    tdir.mkdir(parents=True, exist_ok=True)
    cached = WORK / f"tex_{short}.json"
    if REUSE["tex"] and cached.exists():
        prev = json.loads(cached.read_text())
        log(f"{short}: reusing textures")
        return {k: Path(v) for k, v in prev["paths"].items()}, prev["stats"]
    WT.STYLES[obj.name] = STYLES[obj.name]
    paths, stats = WT.compose_cloth(obj, fields, uvpm, tdir, short, R_out=R_out, supersample=2, seed=SEED, log=log)
    cached.write_text(json.dumps(dict(paths={k: str(v) for k, v in paths.items()}, stats=stats)))
    return paths, stats


def finish(name, obj, V, F, fields, edge_names, gap, blend, src, limb, arm, parts_fn=None, R_out=2048,
           weight_fn=None):
    """Texture, skin, thicken and join a fitted single-layer garment (quads)."""
    short = name.replace("SKM_", "")
    uvpm = D.unwrap(obj)
    paths, stats = textures(obj, fields, uvpm, short, R_out)
    parts = parts_fn(obj, paths) if parts_fn else []
    mat = CW.make_material("M_" + short, paths)
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    WR.skin(obj, src, limb, blend, gap, edges_of(F))
    if weight_fn:
        weight_fn(obj)
    edge = np.min([fields[n][0] for n in edge_names if n in fields], axis=0)
    CW.edge_group(obj, edge)
    thick = 1 - D._smooth(edge, 0.004, 0.011)
    base_t, hem_t = THICKNESS[name]
    WR.make_shell(obj, base_t * (1 - thick) + hem_t * thick)
    n_shell = len(obj.data.vertices)
    CW.join_parts(obj, parts, src)
    edge_final = CW.read_group(obj, "_edge", 0.05)
    CW.attach(obj, arm)
    me = obj.data
    me.calc_loop_triangles()
    info = dict(vertices=len(me.vertices), triangles=len(me.loop_triangles), bones_weighted=len(obj.vertex_groups),
                parts=len(parts), shell_vertices=n_shell, texel_uv_per_m=uvpm,
                textures={k: Path(v).name for k, v in paths.items()}, texture_stats=stats,
                air_gap_mm_median=round(1000 * float(np.median(gap)), 2), air_gap_mm_min=round(1000 * float(gap.min()), 2))
    log(f"{name}: built {info['vertices']} verts, {info['triangles']} tris, {info['bones_weighted']} bones")
    return info, edge_final


def plain_object(name, V, F, panel, seam_edges=()):
    me = bpy.data.meshes.new(name)
    me.from_pydata(np.asarray(V).tolist(), [], np.asarray(F).tolist())
    me.update()
    me.shade_smooth()
    pa = me.attributes.new("panel", "INT", "FACE")
    pa.data.foreach_set("value", np.asarray(panel, np.int32))
    seams = {tuple(sorted(e)) for e in seam_edges}
    for e in me.edges:
        e.use_seam = tuple(sorted(e.vertices)) in seams
    return CW.link(bpy.data.objects.new(name, me))


def polys_from_runs(runs):
    return {k: [list(map(int, r)) for r in v] for k, v in runs.items()}


# ------------------------------------------------------------------ garments

def build_shirt(g, V, F, body, layers, src, limb, arm):
    obj, F = CW.garment_object(g, V, F, body)
    V, fields, gap = CW.detail(g, V, F, obj, body, layers)
    reg, axial = g.extra["region"], g.extra["axial"]
    rolls = []

    def parts_fn(o, paths):
        src_uv = uv_near(o, V[np.argmin(np.abs(V[:, 2] - 1.15) + np.abs(V[:, 0]) + 5 * (V[:, 1] > 0))])
        parts = []
        k = 0
        for side, r_id in (("l", 1), ("r", 2)):
            ax = Tp.ArmAxis(body, side)
            m = reg == r_id
            s_end = float(axial[m].max())
            for turn in range(4):
                s = s_end - 0.004 - turn * 0.0105
                C, t, e1, e2 = S.arm_frame_at(ax, s)
                near = m & (np.abs(axial - s) < 0.012)
                bins, rad = S.sleeve_radius(V[near], C, e1, e2, t)
                tube = 0.0065 if turn < 3 else 0.0055
                rect = strip_slot(k)
                TX.fabric_into_rect(paths, src_uv, rect)
                parts.append(S.torus(f"_roll_{side}{turn}", C, t, e1, e2, bins, rad - 0.002, tube, rect,
                                     seed=SEED + k))
                k += 1
            rolls.append(dict(side=side, s_end=round(s_end, 4), elbow_s=round(float(ax.L1), 4)))
        return parts

    info, edge = finish(SHIRT, obj, V, F, fields, ("hem", "cuff", "collartop", "neck"), gap, CW.limb_blend(g, V),
                        src, limb, arm, parts_fn)
    info["rolled_sleeves"] = rolls
    return obj, info, edge


def build_trousers(g, V, F, body, layers, src, limb, arm):
    obj, F = CW.garment_object(g, V, F, body)
    V, fields, gap = CW.detail(g, V, F, obj, body, layers)
    info, edge = finish(TROUSERS, obj, V, F, fields, ("waist", "leghem"), gap, CW.limb_blend(g, V), src, limb, arm)
    return obj, info, edge


def build_waistcoat(g, V, F, body, src, limb, arm):
    obj, F = CW.garment_object(g, V, F, body)
    loops = boundary_loops(F, len(V))
    polys = {}
    for lp in loops:
        key = "armhole" if np.abs(V[lp, 0]).mean() > 0.1 else "edge"
        polys.setdefault(key, []).append(list(lp) + [lp[0]])
    fields = WD.fields_for(V, polys)
    gap = body.all.signed(V)[0]
    N = CW.vertex_normals(V, F)
    front = V[:, 1] < np.array([body.center_y(float(z)) for z in V[:, 2]])
    edge_d = fields["edge"][0]

    def parts_fn(o, paths):
        rect = strip_slot(15)
        TX.paint_rect(paths, rect, (0.10, 0.07, 0.045), 0.35, seed=SEED)
        parts = []
        for k, z in enumerate(np.linspace(1.25, 0.99, 6)):
            d = (V[:, 0] + 0.010) ** 2 + (V[:, 2] - z) ** 2 + 1e3 * (~front)
            i = int(np.argmin(d))
            t = THICKNESS[WAISTCOAT][1] if edge_d[i] < 0.011 else THICKNESS[WAISTCOAT][0]
            parts.append(S.button(f"_button{k}", V[i] + N[i] * (t + 0.0002), N[i], 0.0065, 0.003, rect))
        return parts

    info, edge = finish(WAISTCOAT, obj, V, F, fields, ("edge", "armhole"), gap, {}, src, limb, arm, parts_fn)
    return obj, info, edge


def build_neckerchief(V, F, G, body, shirt_obj_cons, src, limb, arm):
    rows, cols = G.shape
    seam = [(int(G[r, 0]), int(G[r + 1, 0])) for r in range(rows - 1)]
    obj = plain_object(NECKERCHIEF, V, F, np.zeros(len(F), np.int32), seam)
    polys = {"hem": [list(G[0]) + [int(G[0, 0])], list(G[-1]) + [int(G[-1, 0])]]}
    fields = WD.fields_for(V, polys)
    gap = body.all.signed(V)[0]
    front_c = cols // 2
    knot_c = V[G[:, front_c]].mean(0) + np.array([0, -0.010, -0.002])

    def parts_fn(o, paths):
        src_uv = uv_near(o, V[G[rows // 2, cols // 4]])
        parts = []
        r0 = strip_slot(0)
        TX.fabric_into_rect(paths, src_uv, r0)
        parts.append(S.ellipsoid("_knot", knot_c, (0.018, 0.012, 0.014), r0))
        for k, sx in enumerate((1.0, -1.0)):
            rect = strip_slot(1 + k)
            TX.fabric_into_rect(paths, src_uv, rect)
            P = np.array([knot_c + np.array([0.004 * sx, -0.004, -0.010]),
                          knot_c + np.array([0.012 * sx, -0.002, -0.030]),
                          knot_c + np.array([0.020 * sx, 0.002, -0.050]),
                          knot_c + np.array([0.026 * sx, 0.004, -0.066])])
            P = S.resample(P, 0.006)
            P = S.push_out(P, [(s, full(len(P), off + 0.002), None) for s, off in shirt_obj_cons])
            Nn = np.tile([0.0, -1.0, 0.0], (len(P), 1))
            taper = np.linspace(0.75, 1.0, len(P))
            parts.append(S.tape(f"_tail{k}", P, Nn, 0.035, 0.0022, rect, taper=taper))
        return parts

    info, edge = finish(NECKERCHIEF, obj, V, F, fields, ("hem",), gap, {}, src, limb, arm, parts_fn, R_out=1024)
    return obj, info, edge


def ring_path(cloud, body, thetas, zs, pad, max_r=None):
    P = []
    for t, z in zip(thetas, zs):
        tt = ((t + 180.0) % 360.0) - 180.0
        c = S.neck_axis(body, z) if z > 1.3 else np.array([0.0, body.center_y(z)])
        r = S.radial_ring(cloud, c, z, [tt], dz=0.012, dth=6.0, max_r=max_r)[0]
        P.append(S.ring_point(c, z, tt, r + pad))
    return np.array(P)


def build_apron(Va, Fa, grid, Vp, Fp, gp, body, strap_cloud, waist_cloud, cons, src, limb, arm):
    n_a = len(Va)
    V = np.vstack([Va, Vp])
    F = np.vstack([Fa, Fp + n_a])
    panel = np.r_[np.zeros(len(Fa), np.int32), np.ones(len(Fp), np.int32)]
    obj = plain_object(APRON, V, F, panel)
    gpp = gp + n_a
    polys = {"bibtop": [list(grid[0])], "hem": [list(grid[-1])], "side": [list(grid[:, 0]), list(grid[:, -1])],
             "pocketedge": [list(gpp[0])], "pocket": [list(gpp[:, 0]), list(gpp[-1]), list(gpp[:, -1])]}
    fields = WD.fields_for(V, polys)
    gap = body.all.signed(V)[0]
    tl, tr = Va[grid[0, -1]], Va[grid[0, 0]]  # +x (his left) / -x (his right) bib corners

    def parts_fn(o, paths):
        src_uv = uv_near(o, Va[grid[len(grid) // 2, 13]])
        parts = []
        # neck strap: bib corners -> up the chest -> round the back of the neck
        th = np.linspace(62, 298, 40)
        tz = np.array(S.NK_BASE_TABLE, float)
        zs = np.interp(np.abs(((th + 180.0) % 360.0) - 180.0), tz[:, 0], tz[:, 1]) - 0.010
        ring = ring_path(strap_cloud, body, th, zs, 0.006, max_r=0.15)
        P = np.vstack([tl + [0, 0.0, -0.004], (tl + ring[0]) / 2, ring, (tr + ring[-1]) / 2, tr + [0, 0.0, -0.004]])
        P = S.resample(P, 0.008)
        for _ in range(4):
            P[1:-1] = 0.5 * P[1:-1] + 0.25 * (P[:-2] + P[2:])
        P = S.push_out(P, [(s, full(len(P), off), None) for s, off in cons], rounds=3, smooth=2)
        c = np.array([S.neck_axis(body, min(z, 1.47)) for z in P[:, 2]])
        Nn = np.c_[P[:, 0] - c[:, 0], P[:, 1] - c[:, 1], np.zeros(len(P))]
        Nn /= np.maximum(np.linalg.norm(Nn, axis=1, keepdims=True), 1e-9)
        rect = strip_slot(0)
        TX.fabric_into_rect(paths, src_uv, rect)
        parts.append(S.tape("_neckstrap", P, Nn, 0.020, 0.002, rect))
        # waist ties: from the apron's sides at the waist round to a knot at the back
        iw = int(np.argmin(np.abs(Va[grid[:, 0], 2] - 1.06)))
        pl, pr = Va[grid[iw, -1]], Va[grid[iw, 0]]
        th = np.linspace(80, 280, 36)
        ring = ring_path(waist_cloud, body, th, np.full(len(th), 1.06), 0.005, max_r=0.26)
        for k, half in enumerate((ring[: len(ring) // 2 + 1], ring[len(ring) // 2:][::-1])):
            start = pl if k == 0 else pr
            P = S.resample(np.vstack([start, half]), 0.008)
            P = S.push_out(P, [(s, full(len(P), off), None) for s, off in cons], rounds=2, smooth=2)
            cc = np.array([body.center_y(float(z)) for z in P[:, 2]])
            Nn = np.c_[P[:, 0], P[:, 1] - cc, np.zeros(len(P))]
            Nn /= np.maximum(np.linalg.norm(Nn, axis=1, keepdims=True), 1e-9)
            rect = strip_slot(1 + k)
            TX.fabric_into_rect(paths, src_uv, rect)
            parts.append(S.tape(f"_tie{k}", P, Nn, 0.020, 0.002, rect))
        back = ring[len(ring) // 2]
        rect = strip_slot(3)
        TX.fabric_into_rect(paths, src_uv, rect)
        parts.append(S.ellipsoid("_tieknot", back + np.array([0, 0.008, 0]), (0.016, 0.010, 0.011), rect))
        for k, sx in enumerate((1.0, -1.0)):
            P = np.array([back + [0.006 * sx, 0.010, -0.008], back + [0.020 * sx, 0.012, -0.08],
                          back + [0.030 * sx, 0.012, -0.20]])
            P = S.resample(P, 0.01)
            P = S.push_out(P, [(s, full(len(P), off + 0.004), None) for s, off in cons], rounds=2, smooth=3)
            Nn = np.tile([0.0, 1.0, 0.0], (len(P), 1))
            rect = strip_slot(4 + k)
            TX.fabric_into_rect(paths, src_uv, rect)
            parts.append(S.tape(f"_tietail{k}", P, Nn, 0.020, 0.002, rect, taper=np.linspace(1, 0.85, len(P))))
        return parts

    def weight_fn(o):
        names, W = R.read_weights(o)
        for b in ("pelvis", "thigh_l", "thigh_r"):
            if b not in names:
                names.append(b)
                W = np.c_[W, np.zeros(len(W))]
        Vo = np.array([v.co for v in o.data.vertices])
        w = 0.75 * D._smooth(1.02 - Vo[:, 2], 0.0, 0.12)
        side = np.clip(Vo[:, 0] / 0.12, -1, 1)
        T = np.zeros_like(W)
        T[:, names.index("pelvis")] = 0.6
        T[:, names.index("thigh_l")] = 0.2 + 0.15 * side
        T[:, names.index("thigh_r")] = 0.2 - 0.15 * side
        W = W * (1 - w[:, None]) + T * w[:, None]
        R.write_weights(o, names, R.clean_weights(W))

    info, edge = finish(APRON, obj, V, F, fields, ("hem", "side", "bibtop", "pocketedge", "pocket"), gap, {},
                        src, limb, arm, parts_fn, weight_fn=weight_fn)
    return obj, info, edge


def build_boots(obj, src, arm):
    me = obj.data
    if not me.uv_layers:
        me.uv_layers.new(name="UVMap")
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.01)
    bpy.ops.object.mode_set(mode="OBJECT")
    short = BOOTS.replace("SKM_", "")
    paths = TX.leather_maps(OUT / short / "Textures", short, R=1024, seed=SEED)
    obj.data.materials.clear()
    obj.data.materials.append(CW.make_material("M_" + short, paths))
    for vg in list(obj.vertex_groups):
        obj.vertex_groups.remove(vg)
    R.transfer_weights(obj, src)
    names, W = R.read_weights(obj)
    R.write_weights(obj, names, R.clean_weights(W))
    CW.attach(obj, arm)
    me.calc_loop_triangles()
    return dict(vertices=len(me.vertices), triangles=len(me.loop_triangles), bones_weighted=len(obj.vertex_groups),
                textures={k: Path(v).name for k, v in paths.items()})


# ------------------------------------------------------------------- pencil

def build_pencil(body, arm):
    obj, uinfo = S.pencil_mesh(PENCIL)
    short = "ClerkPencil"
    paths = TX.pencil_maps(OUT / short / "Textures", short, uinfo["cone_u"], uinfo["lead_u"], R=1024, seed=SEED)
    obj.data.materials.append(CW.make_material("M_" + short, paths))
    # rest pose of the pencil behind his right ear (right = -x)
    fV = body.V[body.n_body:]
    box = (fV[:, 0] < -0.07) & (fV[:, 2] > 1.55) & (fV[:, 2] < 1.72) & (fV[:, 1] > -0.05) & (fV[:, 1] < 0.06)
    P = fV[box]
    ear = P[P[:, 0] < P[:, 0].min() + 0.012]
    top = ear[np.argmax(ear[:, 2])]
    def frame(tilt, yaw):
        X = np.array([-math.sin(math.radians(yaw)), -math.cos(math.radians(yaw)), -math.tan(math.radians(tilt))])
        X /= np.linalg.norm(X)
        Zv = np.array([-1.0, 0.0, 0.0])
        Zv = Zv - (Zv @ X) * X
        Zv /= np.linalg.norm(Zv)
        return X, np.cross(Zv, X), Zv

    ts = np.linspace(-0.085, 0.085, 35)
    # tucked in the crease between the top of the ear and the skull: for each back-shift / tilt /
    # yaw, the lowest, most inward centre that clears; the tip pokes forward past the temple
    best = None
    for dy in PENCIL_BACK:
        for tilt in PENCIL_TILTS:
            for yaw in PENCIL_YAWS:
                X, _, _ = frame(tilt, yaw)
                found = None
                for dz in np.arange(-0.004, 0.026, 0.002):
                    for dx in np.arange(0.010, -0.014, -0.002):
                        c = top + np.array([dx, dy, dz])
                        if float(body.all.signed(c + np.outer(ts, X))[0].min()) > PENCIL_CLEAR:
                            found = (dz, dx, c)
                            break
                    if found:
                        break
                if not found:
                    continue
                dz, dx, c = found
                score = dz + 0.5 * abs(dx - 0.002) + 0.0004 * yaw + 0.0002 * abs(tilt - PENCIL_TILT_DEG)
                if best is None or score < best[0]:
                    best = (score, c, tilt, yaw)
    if best:
        _, loc, tilt, yaw = best
    else:
        loc, tilt, yaw = top + np.array([-0.006, 0.025, 0.012]), PENCIL_TILT_DEG, 0.0
    X, Yv, Zv = frame(tilt, yaw)
    pts = lambda c: c + np.outer(ts, X)  # noqa: E731
    clearance = float(body.all.signed(pts(loc))[0].min())
    Mw = Matrix(np.r_[np.c_[X, Yv, Zv, loc], [[0, 0, 0, 1]]].tolist())
    obj.matrix_world = Mw
    Mh = arm.matrix_world @ arm.data.bones["head"].matrix_local
    rel = Mh.inverted() @ Mw
    loc_r, rot_r, _ = rel.decompose()
    eul = rot_r.to_euler("XYZ")
    fit = dict(
        attach_bone="head", units="cm / degrees",
        note=("Pencil (long axis +X, pivot at the middle, sharpened end +X) resting on top of his right ear "
              "between ear and skull, tip forward past the temple and tilted down. 'relative_to_head_bone' is in "
              "the Blender head bone's rest frame; verify the socket in the editor."),
        relative_to_head_bone=dict(location_cm=[round(100 * c, 3) for c in loc_r],
                                   rotation_euler_xyz_deg=[round(math.degrees(a), 2) for a in eul],
                                   rotation_quat_wxyz=[round(c, 5) for c in rot_r]),
        world_bind=dict(location_cm=[round(100 * float(c), 3) for c in loc],
                        axis_x=[round(float(c), 4) for c in X], axis_z=[round(float(c), 4) for c in Zv]),
        unreal_component_space_guess=dict(location_cm=[round(100 * float(loc[0]), 3), round(-100 * float(loc[1]), 3),
                                                       round(100 * float(loc[2]), 3)],
                                          pencil_axis=[round(float(X[0]), 4), round(-float(X[1]), 4), round(float(X[2]), 4)],
                                          note="Y negated for Unreal (verify in editor)"),
        ear_top_cm=[round(100 * float(c), 2) for c in top], min_clearance_mm=round(1000 * clearance, 2))
    pdir = OUT / short
    pdir.mkdir(parents=True, exist_ok=True)
    (pdir / "clerk_pencil_fit.json").write_text(json.dumps(fit, indent=2))
    # static export at the origin
    dup = obj.copy()
    dup.data = obj.data.copy()
    CW.link(dup)
    dup.matrix_world = Matrix.Identity(4)
    dup.name = PENCIL + "_export"
    bpy.ops.object.select_all(action="DESELECT")
    dup.select_set(True)
    bpy.context.view_layer.objects.active = dup
    fbx = pdir / f"{PENCIL}.fbx"
    nm = dup.name
    obj.name = PENCIL + "_placed"
    dup.name = PENCIL
    bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={"MESH"}, global_scale=1.0,
                             apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Y",
                             axis_up="Z", bake_anim=False, path_mode="STRIP", use_mesh_modifiers=True,
                             mesh_smooth_type="FACE", use_tspace=True, add_leaf_bones=False)
    me = dup.data
    bpy.data.objects.remove(dup)
    bpy.data.meshes.remove(me)
    obj.name = PENCIL
    del nm
    obj.data.calc_loop_triangles()
    log(f"{PENCIL}: fit {fit['relative_to_head_bone']}, clearance {1000 * clearance:.1f} mm")
    return obj, dict(vertices=len(obj.data.vertices), triangles=len(obj.data.loop_triangles), fit=fit,
                     textures={k: Path(v).name for k, v in paths.items()})


# -------------------------------------------------------------------- poses

def counter_pose(arm):
    """Arms forward, hands resting on a shop counter: upper arms ~35 deg down-forward, forearms
    forward, a little below horizontal."""
    R.reset_pose(arm)
    Mi = arm.matrix_world.to_3x3().inverted()
    for side, sx in (("l", 1.0), ("r", -1.0)):
        for bone, child, tgt in ((f"upperarm_{side}", f"lowerarm_{side}", (0.15 * sx, -0.57, -0.82)),
                                 (f"lowerarm_{side}", f"hand_{side}", (-0.10 * sx, -1.0, -0.30))):
            # the limb direction is head -> child head (imported bone tails don't follow the limb)
            pb = arm.pose.bones[bone]
            cur = (arm.pose.bones[child].head - pb.head).normalized()
            t = (Mi @ Vector(tgt)).normalized()
            ax = cur.cross(t)
            if ax.length > 1e-6:
                R.rotate_bone_world(arm, bone, ax.normalized(), math.degrees(cur.angle(t)))
    hands = {s: [round(c, 3) for c in arm.matrix_world @ arm.pose.bones[f"hand_{s}"].head] for s in ("l", "r")}
    log(f"counter pose hands {hands}")
    return hands


def set_pose(arm, name):
    if name == "bind":
        R.reset_pose(arm)
        return None
    if name == "counter":
        return counter_pose(arm)
    WR.apply_pose(arm, name)
    return None


# --------------------------------------------------------------------- main

REUSE = {"tex": False}


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    ap = argparse.ArgumentParser()
    ap.add_argument("--stage", choices=("fit", "all"), default="all")
    ap.add_argument("--reuse-textures", action="store_true")
    args = ap.parse_args(argv)
    REUSE["tex"] = args.reuse_textures
    CW.SEED = SEED
    WORK.mkdir(parents=True, exist_ok=True)
    PREVIEWS.mkdir(parents=True, exist_ok=True)

    arm, body_obj, face_obj, body = load_inputs()
    log(f"body matrix_world translation {tuple(body_obj.matrix_world.translation)}, lm jugular {body.lm['jugular_z']:.3f}")
    prof = Ly.RadialProfile(body, 0.98, 1.36)
    boots = fit_boots(body, body_obj)
    gt, Vt, Ft = fit_trousers(body, [])
    tr_layer = CW.shell_layer(Vt, Ft, THICKNESS[TROUSERS][0], TROUSERS)
    shirt_layers = [(tr_layer, 0.003)]
    gs, Vs, Fs = fit_top(body, shirt_params(body), shirt_layers, prof)
    Vs = hug_collar(gs, Vs, Fs, body)
    Vs, nsp = despike(Vs, Fs)
    Vs = Ly.collide_layers(Vs, [(Ly.ClosedSurface(body.all), full(len(Vs), 0.003), None)], rounds=2)
    log(f"{SHIRT}: despiked {nsp} verts")
    gs.extra["gap"] = body.all.signed(Vs)[0]
    log(f"{SHIRT}: collar hugged, body gap min {1000 * gs.extra['gap'].min():.2f} mm, z max {Vs[:, 2].max():.3f}")
    sh_layer = CW.shell_layer(Vs, Fs, THICKNESS[SHIRT][0], SHIRT)
    # near the shirt's open hem its shell normals are unreliable, so the waistcoat clears the
    # trousers by the shirt's thickness there instead
    wc_layers = [(sh_layer, 0.004, lambda V: V[:, 2] > SHIRT_HEM_Z + 0.035),
                 (tr_layer, lambda V: np.where(V[:, 2] < SHIRT_HEM_Z + 0.035, 0.009, 0.0035))]
    gw, Vw, Fw = fit_waistcoat(body, wc_layers, prof)
    wc_layer = CW.shell_layer(Vw, Fw, THICKNESS[WAISTCOAT][1], WAISTCOAT)
    nk_cons = [(Ly.ClosedSurface(body.all), 0.006), (sh_layer, 0.003), (wc_layer, 0.003)]
    Vn, Fn, Gn = fit_neckerchief(body, Vs, Vw, nk_cons)
    nk_layer = CW.shell_layer(Vn, Fn, THICKNESS[NECKERCHIEF][1], NECKERCHIEF)
    ap_cons = [(Ly.ClosedSurface(body.all), 0.006), (sh_layer, 0.004), (wc_layer, 0.004), (tr_layer, 0.004)]
    Va, Fa, grid, Vp, Fp, gp = fit_apron(body, [Vs, Vw, Vt], ap_cons)

    # fit previews (flat colours, the fitted surfaces only)
    objs = {}
    objs[TROUSERS], _ = CW.garment_object(gt, Vt, Ft, body)
    objs[SHIRT], _ = CW.garment_object(gs, Vs, Fs, body)
    objs[WAISTCOAT], _ = CW.garment_object(gw, Vw, Fw, body)
    objs[NECKERCHIEF] = plain_object(NECKERCHIEF, Vn, Fn, np.zeros(len(Fn)))
    objs[APRON] = plain_object(APRON, np.vstack([Va, Vp]), np.vstack([Fa, Fp + len(Va)]), np.zeros(len(Fa) + len(Fp)))
    quick_preview(list(GARMENTS), WORK / "fit.png")
    quick_preview(list(GARMENTS), WORK / "fit_torso.png",
                  views=(("front", (0, -4, 1.3), (90, 0, 0)), ("side", (4, 0, 1.3), (90, 0, 90))), scale=0.8)
    if args.stage == "fit":
        bpy.ops.wm.save_as_mainfile(filepath=str(WORK / "fit.blend"))
        log("fit stage done")
        return
    for o in objs.values():
        me = o.data
        bpy.data.objects.remove(o)
        bpy.data.meshes.remove(me)

    # ------------------------------------------------------------ build
    src = R.weight_source(body_obj, face_obj, arm)
    limb = WR.limb_sources(src)
    rng = np.random.default_rng(SEED)
    del rng
    infos, edges, built = {}, {}, {}
    built[BOOTS] = boots
    infos[BOOTS] = build_boots(boots, src, arm)
    edges[BOOTS] = np.full(len(boots.data.vertices), 0.05)
    built[TROUSERS], infos[TROUSERS], edges[TROUSERS] = build_trousers(gt, Vt, Ft, body, [], src, limb, arm)
    built[SHIRT], infos[SHIRT], edges[SHIRT] = build_shirt(gs, Vs, Fs, body, shirt_layers, src, limb, arm)
    built[WAISTCOAT], infos[WAISTCOAT], edges[WAISTCOAT] = build_waistcoat(gw, Vw, Fw, body, src, limb, arm)
    built[NECKERCHIEF], infos[NECKERCHIEF], edges[NECKERCHIEF] = build_neckerchief(
        Vn, Fn, Gn, body, [(Ly.ClosedSurface(body.all), 0.008), (sh_layer, 0.004), (wc_layer, 0.004)], src, limb, arm)
    strap_cloud = np.vstack([body.V, Vs, Vw, Vn])
    waist_cloud = np.vstack([body.V, Vs, Vw, Vt])
    built[APRON], infos[APRON], edges[APRON] = build_apron(
        Va, Fa, grid, Vp, Fp, gp, body, strap_cloud, waist_cloud,
        ap_cons + [(nk_layer, 0.003)], src, limb, arm)
    for o in list(limb.values()) + [src]:
        me = o.data
        bpy.data.objects.remove(o)
        bpy.data.meshes.remove(me)

    # exports + coverage masks
    exports = {}
    for name in GARMENTS:
        d = gdir(name)
        d.mkdir(parents=True, exist_ok=True)
        path = d / f"{name}.fbx"
        nb, bones = R.export_fbx(path, arm, [built[name]], log=log)
        exports[name] = dict(path=str(path.relative_to(ROOT)).replace("\\", "/"), bones=nb)
    coverage = {}
    for name in (SHIRT, WAISTCOAT, TROUSERS, APRON, BOOTS):
        short = name.replace("SKM_", "")
        mp = gdir(name) / f"BodyCoverageMask_{short}.png"
        try:
            cov, _ = R.coverage_mask(body_obj, [built[name]], {name: edges[name]}, mp, margin=0.012, max_dist=0.04,
                                     z_min=0.0)
            coverage[name] = dict(path=mp.name, **cov)
        except Exception as e:  # masks are optional
            log(f"coverage mask {name} failed: {e!r}")
            coverage[name] = dict(error=repr(e))

    # pencil
    pencil, pinfo = build_pencil(body, arm)

    # metrics in poses
    face_V = body.V[body.n_body:]
    face_N = body.N[body.n_body:] if hasattr(body, "N") else None
    face_T = body.T[(body.T >= body.n_body).all(1)] - body.n_body
    pose_metrics, hands = {}, None
    layer_pairs = ((WAISTCOAT, SHIRT), (APRON, WAISTCOAT), (APRON, TROUSERS), (APRON, SHIRT), (SHIRT, TROUSERS),
                   (WAISTCOAT, TROUSERS), (NECKERCHIEF, SHIRT), (TROUSERS, BOOTS))
    for pose in ("bind", "counter", "walk"):
        h = set_pose(arm, pose)
        hands = h or hands
        surf = WR.posed_surface(arm, body_obj, face_V, face_N, face_T)
        pm = {name: R.poke_metrics(built[name], surf, edges[name]) for name in GARMENTS}
        lm = {}
        for outer, inner in layer_pairs:
            lm[f"{outer}>{inner}"] = WR.layer_poke(built[outer], built[inner], surf)
        pose_metrics[pose] = dict(body=pm, layers=lm)
        log(f"pose {pose}: " + ", ".join(f"{k} {v['inside_pct']:.2f}% {v['max_depth_mm']:.1f}mm" for k, v in pm.items()))
        log("  layers: " + ", ".join(f"{k} {v['inside_pct']:.2f}%" for k, v in lm.items()))
        if pose == "counter":
            skin_materials(body_obj, face_obj)
            show = list(GARMENTS) + [PENCIL]
            render_views(show, "counter_pose", (("front", (0, -4, 1.0), (90, 0, 0)), ("q34", (2.6, -2.6, 1.05), (90, 0, 40)),
                                                 ("side", (4, -0.2, 1.05), (90, 0, 90))), 2.0)
    R.reset_pose(arm)

    # final previews
    skin_materials(body_obj, face_obj)
    pencil.parent = None
    show = list(GARMENTS) + [PENCIL]
    previews = render_views(show, "clerk", FULL_VIEWS, 2.0)
    previews += render_views(show, "clerk", CLOSE_VIEWS, 0.75, res=(1200, 1000))
    previews += render_views(show, "clerk", HEAD_VIEWS, 0.40, res=(1000, 1000))
    previews += render_views(show, "clerk", FEET_VIEWS, 0.55, res=(1200, 800))
    previews += [str(PREVIEWS / f"counter_pose_{v}.png") for v in ("front", "q34", "side")]

    total = sum(infos[n]["triangles"] for n in GARMENTS)
    files = {}
    for p in sorted(OUT.rglob("*")):
        if p.is_file() and p.suffix.lower() in (".fbx", ".png", ".json") and p.name != "report.json":
            files[str(p.relative_to(OUT)).replace("\\", "/")] = CW.sha256(p)
    report = dict(
        recipe="Scripts/Blender/Recipes/clerk_outfit.py", seed=SEED, date=DATE, blender=bpy.app.version_string,
        inputs={p.name: CW.sha256(p) for p in (BODY_FBX, FACE_FBX)},
        garments={n: infos[n] for n in GARMENTS}, total_garment_triangles=total,
        pencil=pinfo, exports=exports, coverage_masks=coverage, counter_pose_hands=hands,
        pose_metrics=pose_metrics, previews=previews, files=files)
    (OUT / "report.json").write_text(json.dumps(report, indent=1, default=lambda o: o.tolist() if hasattr(o, "tolist") else str(o)))
    log(f"total garment triangles {total}")
    bpy.ops.wm.save_as_mainfile(filepath=str(WORK / "build.blend"))
    log("done")


if __name__ == "__main__":
    main()
