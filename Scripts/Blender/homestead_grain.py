"""Shared builder for the grain crop plots (wheat and barley), used by crop_wheat.py / crop_barley.py.

    import homestead_grain as G
    def build(kit): return G.build(kit, CONFIG)

Real-plant research (written before modelling):
- Nineteenth-century Cornish farms grew wheat and barley in drills; kitchen-garden patches were
  sown the same way. A plant tillers into 3-5 culms (hollow jointed stems) with long, narrow,
  parallel-veined strap leaves, the top one the flag leaf just under the ear.
- Stages: a green "brairding" of grass-like blades; a tillered tuft that arches over; stem
  extension with leaves along the culm; heading, when green ears push out of the flag-leaf sheath;
  and ripening, when stems, leaves and ears turn straw-gold and the ears nod.
- Wheat of the period (Red Lammas, Talavera, Browick) was tall (1-1.5 m) and mostly awnless: a
  fat, square-ish ear 8-10 cm long of alternating spikelets on two faces, ripening russet-gold.
- Barley for malting (Chevalier) is two-rowed and "bearded": a flatter, thinner ear 7-9 cm long
  with long stiff awns 10-15 cm that fan up from it; the ripe ear hooks over and hangs.

Game build: the plot's stems and leaves are the five stage meshes; each ear is one instance of
SM_<Name>_Produce placed on the culm tops from the report anchors, so the game swells the ears
and ripens them green -> gold through M_CropProduce. The ear mesh stands on +Z with its lean baked
toward +X (the anchor yaw turns it to the culm's lean). The Harvest mesh is a small tied sheaf,
gripped at the band (origin), ears hanging down -Z.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_crop as C
import homestead_foliage as F
import homestead_shrub as S

UP = Vector((0, 0, 1))


class Grain:
    def __init__(self, **kw):
        self.__dict__.update(kw)


# ------------------------------------------------------------------ painting

def _blade_layer(X, Y, px, nrng, pal, *, width, dry=0.0, yellow=0.0, burn=0.0, trans=0.5):
    shape = F.ovate(width=width, base=0.0, tip=0.995, widest=0.10, tip_sharp=1.15, base_round=0.25)
    veins = [([(0.0, 0.01), (0.0, 0.97)], 1.0)]
    for k in (-3, -2, -1, 1, 2, 3):
        x0 = width * 0.28 * k
        veins.append(([(x0, 0.02), (x0 * 0.55, 0.60), (x0 * 0.10, 0.96)], 0.35))
    return F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=0.0030, vein_depth=0.00008,
                         puff=0.00006, tertiary=0.0, dry=dry, yellow=yellow, edge_burn=burn,
                         gloss=0.10, trans=trans)


def _paint_ear(atlas, key, cfg, nrng):
    """Column for the ear tube: u around, v base -> tip. Stacked spikelets on alternating faces,
    ripe-coloured (the produce material tints it green while unripe)."""
    U, V = atlas.column_grid(key)
    layer = F.Layer(U.shape)
    count = cfg.spikelets
    col = np.zeros(U.shape + (3,))
    col[...] = cfg.ear_dark
    height = np.zeros(U.shape)
    n = F.noise(U.shape, nrng, freq=40.0, beta=1.8)
    for face in range(4):
        uc = 0.125 + 0.25 * face
        for k in range(count):
            vc = (k + 0.5 + 0.5 * (face % 2)) / (count + 0.5)
            du = (U - uc) / 0.105
            dv = (V - vc) / (0.62 / count)
            r = du * du + dv * dv
            body = 1 - F.smoothstep(0.55, 1.0, r)
            rim = F.smoothstep(0.35, 0.95, r) * body
            spik = F.lerp(cfg.ear_light, cfg.ear_mid, np.clip(0.35 + 0.45 * dv + 0.2 * n, 0, 1))
            spik = F.lerp(spik, cfg.ear_dark, rim * 0.55)
            col = F.lerp(col, spik, body)
            height = np.maximum(height, 0.0009 * np.sqrt(np.clip(1 - r, 0, 1)))
    # Glume keels: a fine pale line down each spikelet.
    keel = np.exp(-(((U * 8.0) % 1.0 - 0.5) / 0.06) ** 2)
    col = F.lerp(col, cfg.ear_light, keel * 0.25)
    layer.color = col * (0.9 + 0.2 * n)[..., None]
    layer.height = height + 0.00004 * n
    layer.alpha[...] = 1
    layer.rough = 0.62 + 0.1 * n
    layer.trans[...] = 0.10
    atlas.put(key, layer, meters_per_px=cfg.ear_len / U.shape[0], wrap=True, opaque=True)


def _paint_stem(atlas, key, base, hi, nrng, node_every=0.18):
    U, V = atlas.column_grid(key)
    n = F.noise(U.shape, nrng, freq=90.0, beta=1.8, aniso=(1.0, 8.0))
    streak = F.ridged(F.noise(U.shape, nrng, freq=30.0, beta=2.2, aniso=(1.0, 12.0)))
    layer = F.Layer(U.shape)
    layer.color = F.lerp(base, hi, np.clip(0.35 * n + 0.35 * streak + 0.15, 0, 1))
    # Nodes: darker swollen rings, v_length maps 1 v to ~0.35 m of stem.
    node = np.exp(-(((V / node_every) % 1.0 - 0.5) / 0.03) ** 2)
    layer.color = F.lerp(layer.color, np.asarray(base) * 0.62, node * 0.6)
    layer.height = 0.00005 * n + 0.00012 * node
    layer.alpha[...] = 1
    layer.rough = 0.55 + 0.1 * n
    layer.trans[...] = 0.25
    atlas.put(key, layer, meters_per_px=0.35 / U.shape[0], wrap=True, opaque=True)


def _paint_awns(atlas, key, cfg, nrng):
    """Barley beard: fine stiff bristles fanning up from the ear, on a clear background."""
    X, Y, px = atlas.grid(key)
    layer = F.Layer(X.shape)
    rng = random.Random(cfg.seed + 55)
    alpha = np.zeros(X.shape)
    col = np.zeros(X.shape + (3,))
    col[...] = cfg.ear_mid
    a = X.max()
    for _ in range(36):
        x0 = rng.uniform(-0.30, 0.30) * a
        spread = x0 * rng.uniform(1.05, 1.45)
        y0 = rng.uniform(0.0, 0.25)
        pts = [(x0, y0), (x0 + (spread - x0) * 0.5, (y0 + 1.0) * 0.5), (spread, rng.uniform(0.86, 0.99))]
        d, along = F.polyline_distance(X, Y, pts, max_dist=px * 6)
        w = px * (1.35 - 0.8 * along)
        line = np.exp(-(d / np.maximum(w, px * 0.6)) ** 2) * (Y > y0 - 0.005)
        tone = rng.uniform(0.0, 1.0)
        col = F.lerp(col, F.lerp(cfg.ear_light, cfg.ear_dark, tone * 0.6), line)
        alpha = np.maximum(alpha, line)
    layer.color = col
    layer.alpha = F.smoothstep(0.25, 0.65, alpha)
    layer.height = 0.00003 * alpha
    layer.rough[...] = 0.6
    layer.trans[...] = 0.35
    atlas.put(key, layer, meters_per_px=cfg.awn_len / X.shape[0])


def paint_atlas(cfg):
    atlas = F.Atlas(cfg.name, size=2048, seed=cfg.seed, padding=10)
    atlas.column("stem", 48)
    atlas.column("straw", 48)
    atlas.column("ear", 160)
    tile_w = int(2 * cfg.blade_halfwidth * 980 * 1.15) + 20
    for key in ("blade", "blade_pale", "blade_young", "blade_turning", "blade_dry"):
        atlas.tile(key, tile_w, 1000)
    atlas.tile("band", 150, 500)
    for key in ("tuft", "tuft_young", "tuft_dry"):
        atlas.tile(key, 440, 560)
    if cfg.awns:
        atlas.tile("awns", 300, 1000)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(cfg.seed)
    w = cfg.blade_halfwidth
    pals = {
        "blade": dict(base=cfg.leaf, tip=tuple(c * 1.10 for c in cfg.leaf), vein=cfg.leaf_vein, margin=cfg.leaf_vein),
        "blade_pale": dict(base=cfg.leaf_pale, tip=cfg.leaf_pale, vein=cfg.leaf_vein, margin=cfg.leaf_vein),
        "blade_young": dict(base=cfg.leaf_young, tip=tuple(c * 1.08 for c in cfg.leaf_young),
                            vein=cfg.leaf_pale, margin=cfg.leaf_pale),
        "blade_turning": dict(base=cfg.leaf, tip=cfg.straw_hi, vein=cfg.leaf_vein, margin=cfg.straw,
                              yellow=cfg.straw_hi, brown=cfg.straw_dark, dry=cfg.straw),
        "blade_dry": dict(base=cfg.straw, tip=cfg.straw_hi, vein=cfg.straw_hi, margin=cfg.straw_dark,
                          yellow=cfg.straw_hi, brown=cfg.straw_dark, dry=cfg.straw),
    }
    for key, extra in (("blade", {}), ("blade_pale", {}), ("blade_young", dict(trans=0.65)),
                       ("blade_turning", dict(yellow=0.55, burn=0.35, trans=0.4)),
                       ("blade_dry", dict(dry=0.85, burn=0.4, trans=0.25))):
        X, Y, px = atlas.grid(key)
        atlas.put(key, _blade_layer(X, Y, px, nrng, pals[key], width=w, **extra),
                  meters_per_px=cfg.leaf_len / X.shape[0])
    # Tuft cards: a fan of seven blades from one base, so a tillering plant reads as a dense
    # clump from a few cards.
    trng = random.Random(cfg.seed + 71)
    for key, pal_key, extra in (("tuft", "blade", {}), ("tuft_young", "blade_young", dict(trans=0.6)),
                                ("tuft_dry", "blade_dry", dict(dry=0.8, burn=0.4, trans=0.25))):
        X, Y, px = atlas.grid(key)
        layer = F.Layer(X.shape)
        for k in range(7):
            theta = -0.62 + 1.24 * k / 6 + trng.uniform(-0.08, 0.08)
            length = trng.uniform(0.78, 0.98) * (1.0 - 0.25 * abs(theta))
            Xl, Yl = S.rotated(X, Y, (0.0, 0.02), theta, length)
            # Blades arch outward: shift their far half sideways.
            Xl = Xl - 0.10 * np.sign(theta) * np.clip(Yl, 0, 1) ** 2
            p = pals[pal_key] if k % 3 else pals["blade_pale" if pal_key == "blade" else pal_key]
            layer.over(_blade_layer(Xl, Yl, px / length, nrng, p, width=w * 1.25, **extra))
        atlas.put(key, layer, meters_per_px=cfg.leaf_len / X.shape[0])
    # Twisted-straw band for the sheaf.
    X, Y, px = atlas.grid("band")
    band = F.Layer(X.shape)
    n = F.noise(X.shape, nrng, freq=30.0, beta=1.8, aniso=(6.0, 1.0))
    twist = 0.5 + 0.5 * np.sin((Y * 40.0 + X * 90.0) * math.tau / 4)
    band.color = F.lerp(cfg.straw_dark, cfg.straw_hi, np.clip(0.3 + 0.5 * twist + 0.2 * n, 0, 1))
    band.alpha[...] = 1
    band.height = 0.0003 * twist
    band.rough[...] = 0.7
    atlas.put("band", band, meters_per_px=0.1 / X.shape[0], opaque=True)
    _paint_stem(atlas, "stem", cfg.stem, cfg.stem_hi, nrng)
    _paint_stem(atlas, "straw", cfg.straw, cfg.straw_hi, nrng)
    _paint_ear(atlas, "ear", cfg, nrng)
    if cfg.awns:
        _paint_awns(atlas, "awns", cfg, nrng)
    atlas.save()
    return atlas


# ------------------------------------------------------------------ layout

def _plants(cfg):
    out = []
    rng = random.Random(cfg.seed + 3)
    xs = np.linspace(-0.40, 0.40, cfg.plants_per_ridge)
    for j, y in enumerate(C.RIDGES):
        for i, x in enumerate(xs):
            out.append(dict(pos=Vector((x + rng.uniform(-0.025, 0.025), y + rng.uniform(-0.035, 0.035), C.BASE_Z)),
                            az=rng.uniform(0, math.tau), phase=rng.random(), idx=j * 100 + i))
    return out


def _culms(cfg, stage):
    """The culm (stem) of each tiller for a stage: base, top point list, lean azimuth. Stable
    across LODs so every LOD's stems end at the produce anchors."""
    st = cfg.stages[stage]
    culms = []
    for plant in _plants(cfg):
        rng = C.plant_rng(cfg.seed, "culm", stage, plant["idx"])
        for t in range(cfg.tillers):
            az = plant["az"] + t * math.tau / cfg.tillers + rng.uniform(-0.5, 0.5)
            lean = math.radians(rng.uniform(*st.get("lean", (0, 3))))
            if stage == "Ripe" and rng.random() < 0.10:
                lean += math.radians(rng.uniform(10, 18))  # a few stems half-lodged
            # Young has no culm yet; its (hidden) anchors sit in the tuft.
            H = st.get("culm", st.get("blade_len", 0.1) * 0.4) * rng.uniform(0.86, 1.06)
            base = plant["pos"] + C.heading(az) * rng.uniform(0.004, 0.014)
            hd = C.heading(az)
            pts = []
            n = 6
            for k in range(n):
                s = k / (n - 1)
                horiz = math.sin(lean) * H * (s ** 1.6)
                pts.append(base + hd * horiz + UP * (math.cos(lean * s) * H * s))
            top = pts[-1]
            # Keep the ear inside the plot square.
            if abs(top.x) > C.EDGE or abs(top.y) > C.EDGE:
                sx = min(1.0, (C.EDGE - abs(base.x)) / max(abs(top.x - base.x), 1e-4))
                sy = min(1.0, (C.EDGE - abs(base.y)) / max(abs(top.y - base.y), 1e-4))
                pts = [Vector((base.x + (p.x - base.x) * sx, base.y + (p.y - base.y) * sy, p.z)) for p in pts]
            culms.append(dict(pts=pts, az=az, phase=plant["phase"], seed=rng.randrange(1 << 20),
                              scale=rng.uniform(0.86, 1.10), plant=plant))
    return culms


def _anchors(cfg):
    rows = {}
    for stage in C.PRODUCE_STAGES:
        rows[stage] = [(c["pts"][-1], math.degrees(c["az"]), c["scale"]) for c in _culms(cfg, stage)]
    return C.anchors_report(f"SM_{cfg.name}_Produce", rows)


# ------------------------------------------------------------------ geometry

def _leaf(b, atlas, key, node, az, length, phase, lod, rng, elev_deg, droop):
    hd = C.heading(az)
    d = (hd * math.cos(math.radians(elev_deg)) + UP * math.sin(math.radians(elev_deg))).normalized()
    up = (UP - hd * 0.5).normalized()
    rect = atlas.uv(key)
    if rng.random() < 0.5:
        rect = (rect[1], rect[0], rect[2], rect[3])
    length = length * C.edge_scale(node, hd, length * 0.8)
    C.strap(b, rect, node, d, length, length * S.tile_aspect(atlas, key), phase, up=up,
            rows=(5, 3, 2)[lod], droop=droop, twist=rng.uniform(-0.9, 0.9) if lod < 2 else 0.0,
            fold=0.25 if lod == 0 else 0.0, flutter=1.0, flutter_base=0.10)


def _tuft(b, atlas, cfg, plant, stage, lod):
    st = cfg.stages[stage]
    rng = C.plant_rng(cfg.seed, "tuft", stage, plant["idx"])
    # Crossed tuft cards give the clump its mass; single blades break up its outline.
    tufts = st.get("tufts", 0)
    for k in range(tufts):
        az = plant["az"] + k * math.pi / max(tufts, 1) + rng.uniform(-0.2, 0.2)
        length = st["tuft_len"] * rng.uniform(0.85, 1.1)
        lean = C.heading(az + math.pi * 0.5) * rng.uniform(-0.25, 0.25)
        d = (UP + lean).normalized()
        if lod == 2 and k:
            continue
        b.card(plant["pos"] - UP * 0.004, d, C.heading(az), length, length * S.tile_aspect(atlas, st["tuft_key"]),
               atlas.uv(st["tuft_key"]), rows=(3, 2, 1)[lod], cols=(2, 1, 1)[lod], fold=0.05 if lod == 0 else 0.0,
               droop=rng.uniform(0.1, 0.3), phase=plant["phase"], flutter=0.9, flutter_base=0.05)
    count = st.get("blades", 0)
    keep = (1.0, 0.6, 0.34)[lod]
    for k in range(count):
        az = plant["az"] + k * 2.39996 + rng.uniform(-0.3, 0.3)
        length = st["blade_len"] * rng.uniform(0.75, 1.1)
        elev = rng.uniform(*st["blade_elev"])
        droop = rng.uniform(*st["blade_droop"])
        key = st["blade_key"] if rng.random() > 0.25 or stage == "Sprout" else "blade_pale"
        if k >= max(1, int(round(count * keep))):
            continue
        base = plant["pos"] + C.heading(az) * 0.004 + UP * 0.002
        _leaf(b, atlas, key, base, az, length, plant["phase"], lod, rng, elev, droop)


def _emit_culm(b, atlas, cfg, culm, stage, lod):
    st = cfg.stages[stage]
    rng = random.Random(culm["seed"])
    pts = culm["pts"]
    if lod == 1:
        pts = [pts[0], pts[2], pts[4], pts[5]]
    elif lod == 2:
        pts = [pts[0], pts[3], pts[5]]
    r0 = cfg.culm_radius
    radii = [r0 * (1.0 - 0.35 * i / (len(pts) - 1)) for i in range(len(pts))]
    stem_key = "straw" if stage == "Ripe" else "stem"
    b.tube(pts, radii, 3, atlas.uv(stem_key), v_length=0.35, phase=culm["phase"],
           flutter=0.05)
    # Leaves at the nodes: blade sheaths wrap the culm, then the blade leaves at an angle; the top
    # one is the flag leaf just below the ear.
    leaves = st["leaves"]
    for k in range(leaves):
        t = 0.14 + 0.62 * k / max(leaves - 1, 1)
        az = culm["az"] + math.pi * (k % 2) + rng.uniform(-0.5, 0.5)
        length = cfg.leaf_len * rng.uniform(0.8, 1.05) * (0.75 if k == leaves - 1 else 1.0)
        elev = rng.uniform(*st["leaf_elev"])
        droop = rng.uniform(*st["leaf_droop"])
        key = st["leaf_keys"][min(k, len(st["leaf_keys"]) - 1)]
        if lod == 1 and k % 2 == 1 and k != leaves - 1:
            continue
        if lod == 2 and k != leaves - 1:
            continue
        seg = t * (len(culm["pts"]) - 1)
        i = min(int(seg), len(culm["pts"]) - 2)
        node = culm["pts"][i].lerp(culm["pts"][i + 1], seg - i)
        _leaf(b, atlas, key, node, az, length, culm["phase"], lod, rng, elev, droop)


def emit(cfg, atlas, stage, lod):
    st = cfg.stages[stage]
    b = F.Batch(height=C.BASE_Z + max(st.get("culm", 0.0), st.get("blade_len", 0.0), st.get("tuft_len", 0.0)) + 0.05)
    if st.get("blades") or st.get("tufts"):
        for plant in _plants(cfg):
            _tuft(b, atlas, cfg, plant, stage, lod)
    if st.get("culm"):
        for n, culm in enumerate(_culms(cfg, stage)):
            if lod == 2 and n % 3 == 2 and stage != "Ripe":
                continue
            _emit_culm(b, atlas, cfg, culm, stage, lod)
    print("HOMESTEAD_TRIS", stage, lod, b.triangles)
    return b


def _ear(b, atlas, cfg, base, axis, side, phase, *, sides=8, rings=12, awns=True, awn_lod=0):
    """One ear along ``axis`` from ``base``: a flattened, lumpy tube with a pointed tip, plus the
    barley beard as two crossed awn cards."""
    pts, radii = [], []
    for i in range(rings):
        t = i / (rings - 1)
        bow = side * (cfg.ear_bow * math.sin(t * math.pi * 0.5))
        pts.append(base + axis * (cfg.ear_len * t) + bow)
        if t < 0.2:
            env = 0.72 + 0.28 * math.sin(math.pi * 0.5 * t / 0.2)
        elif t < 0.78:
            env = 1.0
        else:
            env = 0.12 + 0.88 * (1 - (t - 0.78) / 0.22) ** 0.7
        lump = 1.0 + 0.16 * abs(math.sin(t * math.pi * cfg.spikelets * 0.5))
        radii.append(cfg.ear_radius * env * lump)
    b.tube(pts, radii, sides, atlas.uv("ear", inset=False), v_length=cfg.ear_len, phase=phase,
           flutter=0.05, cap=True, flatten=cfg.ear_flatten)
    if cfg.awns and awns:
        for k in range(2 if awn_lod == 0 else 1):
            n = Matrix.Rotation(math.radians(60) * k, 3, axis) @ side
            b.card(base + axis * cfg.ear_len * 0.15, axis, n.cross(axis), cfg.awn_len,
                   cfg.awn_len * S.tile_aspect(atlas, "awns"), atlas.uv("awns"), rows=2, cols=1,
                   fold=0.0, droop=-0.05, phase=phase, flutter=0.2, flutter_base=0.05)
    return pts


def emit_produce(cfg, atlas):
    b = F.Batch(height=0.3)
    lean = math.radians(cfg.ear_lean)
    axis = Vector((math.sin(lean), 0, math.cos(lean))).normalized()
    side = axis.cross(Vector((0, 1, 0))).normalized()
    # A short bare neck of stem (the peduncle) so the ear joins the culm top cleanly.
    neck = [Vector((0, 0, -0.01)), Vector((0, 0, 0.005)), axis * 0.012]
    b.tube(neck, [cfg.culm_radius * 0.9, cfg.culm_radius * 0.85, cfg.culm_radius * 0.8], 6,
           atlas.uv("straw"), v_length=0.35, phase=0.0, flutter=0.0)
    _ear(b, atlas, cfg, axis * 0.010, axis, side, 0.0, sides=7, rings=10)
    print("HOMESTEAD_TRIS Produce", b.triangles)
    return b


def emit_harvest(cfg, atlas):
    """A small tied sheaf gripped at the straw band (origin): butts above, ears hanging down."""
    rng = random.Random(cfg.seed + 909)
    b = F.Batch(height=0.6)
    stalks = 22
    top_z = 0.10
    length = cfg.sheaf_len
    for k in range(stalks):
        r = 0.022 * math.sqrt(rng.random())
        a = rng.uniform(0, math.tau)
        at = Vector((math.cos(a) * r, math.sin(a) * r, 0))
        splay_b = at.normalized() * rng.uniform(0.015, 0.035) if at.length > 1e-4 else Vector((0.01, 0, 0))
        splay_e = at.normalized() * rng.uniform(0.04, 0.09) if at.length > 1e-4 else Vector((0.03, 0, 0))
        pts = [at * 1.4 + splay_b + UP * top_z, at + UP * 0.03, at * 0.9 + UP * -0.03,
               at * 1.4 + splay_e * 0.5 + UP * (-length * 0.55), at * 1.6 + splay_e + UP * (-length)]
        b.tube(pts, [cfg.culm_radius] * len(pts), 4, atlas.uv("straw"), v_length=0.35, phase=0.2, flutter=0.0)
        axis = (pts[-1] - pts[-2]).normalized()
        side = axis.cross(Vector((0, 0, 1)) if abs(axis.z) < 0.95 else Vector((1, 0, 0))).normalized()
        _ear(b, atlas, cfg, pts[-1], axis, side, 0.2, sides=6, rings=8, awn_lod=1)
    # The twisted straw band round the middle.
    ring = [Vector((math.cos(a) * 0.030, math.sin(a) * 0.030, 0.0)) for a in np.linspace(0, math.tau, 13)]
    for dz in (-0.006, 0.006):
        b.tube([p + UP * dz for p in ring], [0.006] * len(ring), 5, atlas.uv("band"), v_length=0.2,
               phase=0.2, flutter=0.0)
    print("HOMESTEAD_TRIS Harvest", b.triangles)
    return b


def build(kit, cfg):
    atlas = paint_atlas(cfg)
    material = atlas.material(translucent=(1.1, 1.15, 0.6))
    objs = C.stage_meshes(kit, cfg.name, material, lambda stage, lod: emit(cfg, atlas, stage, lod))
    cfg.report["produce"] = _anchors(cfg)
    produce_material = material.copy()
    produce_material.name = f"M_{cfg.name}Produce"
    objs.append(C.single(kit, emit_produce(cfg, atlas), f"SM_{cfg.name}_Produce", produce_material, still_wind=True))
    objs.append(C.single(kit, emit_harvest(cfg, atlas), f"SM_{cfg.name}_Harvest", material, still_wind=True))
    return objs
