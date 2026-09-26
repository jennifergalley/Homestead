"""Generic multi-stem woody shrub grower for Homestead foliage recipes.

Builds on homestead_foliage.py (atlas painting, Batch geometry, Wind colours, LODs). A recipe
supplies a spec (envelope, stem and branch levels, leaf arrangement) plus its own painted leaf,
spray and bark tiles; ``grow`` makes every random decision once and ``emit`` turns them into one
Batch per LOD, so all LODs share the same plant.

Growth: stems leave the crowns and steer with an upward tropism and an outward pull until they
reach the envelope (a height profile of horizontal radius). Each branch level spawns children
along its parent with a golden-angle spiral. Leaves grow on the last ``leaf_zone`` metres of
every branch at or above ``leaf_level``; the outermost twigs (what the eye sees) get individual
leaf cards, the rest a painted leaf-spray card. LOD1 keeps individual leaves on the outer twigs
with coarser cards; LOD2 uses one spray card per twig and drops the finest branches.
"""
import math

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F

UP = Vector((0, 0, 1))
GOLDEN = 2.39996


def rotated(X, Y, origin, theta, scale, squash=1.0):
    """Blade coordinates for a leaf whose base is at ``origin`` and whose axis leans ``theta``
    from +Y toward +X, ``scale`` tile units long; ``squash`` < 1 fakes a leaf tilted away."""
    dx, dy = X - origin[0], Y - origin[1]
    c, s = math.cos(theta), math.sin(theta)
    return (dx * c - dy * s) / (scale * squash), (dx * s + dy * c) / scale


def interp(profile, t):
    if t <= profile[0][0]:
        return profile[0][1]
    for (t0, v0), (t1, v1) in zip(profile, profile[1:]):
        if t <= t1:
            return v0 + (v1 - v0) * (t - t0) / max(t1 - t0, 1e-9)
    return profile[-1][1]


def tile_aspect(atlas, key):
    x0, y0, w, h = atlas.rects[key]
    return (w - 2 * atlas.pad) / float(h - 2 * atlas.pad)


class Envelope:
    """Crown silhouette: horizontal half-widths rx, ry scaled by ``profile`` (height fraction ->
    radius fraction), flat-topped at ``height``. ratio() > 1 means outside."""

    def __init__(self, height, rx, ry, profile, center=(0.0, 0.0), lobes=()):
        self.H, self.rx, self.ry, self.profile = height, rx, ry, profile
        self.c = Vector((center[0], center[1], 0.0))
        self.lobes = lobes      # (k, amplitude, phase, height_coupling): irregular, lumpy crowns

    def _lobe(self, p):
        if not self.lobes:
            return 1.0
        th = math.atan2((p.y - self.c.y) / self.ry, (p.x - self.c.x) / self.rx)
        z = p.z / self.H
        return 1.0 + sum(a * math.sin(k * th + ph + hz * z * 6.0) for k, a, ph, hz in self.lobes)

    def radial(self, p):
        t = min(max(p.z / self.H, 0.0), 1.0)
        prof = max(interp(self.profile, t), 0.03) * self._lobe(p)
        ex = (p.x - self.c.x) / (self.rx * prof)
        ey = (p.y - self.c.y) / (self.ry * prof)
        return ex, ey

    def ratio(self, p):
        ex, ey = self.radial(p)
        return max(math.hypot(ex, ey), p.z / (self.H * (0.94 + 0.06 * self._lobe(p))))

    def outward(self, p):
        o = Vector((p.x - self.c.x, p.y - self.c.y, 0.0))
        return o.normalized() if o.length > 1e-3 else Vector((1, 0, 0))

    def normal(self, p):
        ex, ey = self.radial(p)
        t = min(max(p.z / self.H, 0.0), 1.0)
        n = Vector((ex / self.rx, ey / self.ry, 0.0))
        n = n.normalized() * min(math.hypot(ex, ey), 1.0) if n.length > 1e-6 else Vector()
        return (n + UP * (0.25 + 1.2 * t ** 3)).normalized()


def _rand_unit(rng):
    v = Vector((rng.gauss(0, 1), rng.gauss(0, 1), rng.gauss(0, 1)))
    return v.normalized() if v.length > 1e-6 else Vector((1, 0, 0))


def grow_path(rng, env, start, direction, length, step, tropism, outward, wander, stop=1.0,
              gravity=0.0, origin=None):
    """Branch axis from ``start``. Per metre the heading turns toward UP by ``tropism``, away
    from the crown centre by ``outward`` and wanders smoothly; ``gravity`` pulls long branches
    down with length. Growth stops at ``length`` or when leaving the envelope (ratio > stop)."""
    p = Vector(start)
    d = Vector(direction).normalized()
    pts = [p.copy()]
    w = _rand_unit(rng)
    s = 0.0
    while s < length:
        w = (w * 0.85 + _rand_unit(rng) * 0.15).normalized()
        if origin is not None:
            o = Vector((p.x - origin.x, p.y - origin.y, 0.0))
            o = o.normalized() if o.length > 1e-3 else env.outward(p)
        else:
            o = env.outward(p)
        d = (d + (UP * tropism + o * outward + w * wander - UP * gravity * s) * step).normalized()
        p = p + d * step
        s += step
        if p.z < 0.012:
            p.z = 0.012
            d.z = abs(d.z) + 0.1
            d.normalize()
        pts.append(p.copy())
        if env.ratio(p) > stop and s > step * 3:
            break
    return pts


def arclength(pts):
    acc = [0.0]
    for a, b in zip(pts, pts[1:]):
        acc.append(acc[-1] + (b - a).length)
    return acc


def at(pts, acc, s):
    for i in range(1, len(pts)):
        if acc[i] >= s:
            f = (s - acc[i - 1]) / max(acc[i] - acc[i - 1], 1e-9)
            return pts[i - 1].lerp(pts[i], f), (pts[i] - pts[i - 1]).normalized(), i
    return pts[-1], (pts[-1] - pts[-2]).normalized(), len(pts) - 1


def grow(spec, rng):
    """Every random decision for one shrub. Returns dict(branches, twigs, env, height)."""
    H = spec["height"]
    lobes = []
    if spec.get("lobes"):
        amp = spec["lobes"]
        for k in (2, 3, 5, 7):
            lobes.append((k, amp * rng.uniform(0.4, 1.0) / (k ** 0.5), rng.uniform(0, math.tau), rng.uniform(-0.5, 0.5)))
    env = Envelope(H, spec["rx"], spec["ry"], spec["profile"], spec.get("center", (0.0, 0.0)), lobes)
    crowns = [Vector((x, y, -0.01)) for x, y in spec["crowns"]]
    levels = spec["levels"]
    branches, twigs = [], []
    step = spec.get("step", 0.03)

    def add_branch(level, pts, r0, phase, parent=None):
        if len(pts) < 2:
            return None
        acc = arclength(pts)
        L = acc[-1]
        lv = levels[level]
        tip = lv.get("tip_radius", 0.3)
        radii = [max(r0 * (tip + (1 - tip) * (1 - s / max(L, 1e-6)) ** 0.9), spec.get("min_radius", 0.0012))
                 for s in acc]
        br = dict(level=level, pts=pts, acc=acc, radii=radii, phase=phase % 1.0, length=L,
                  bark=lv.get("bark", spec.get("bark", "bark")), parent=parent)
        branches.append(br)
        return br

    # Level 0: stems from the crowns.
    stems = []
    lv0 = levels[0]
    for i in range(lv0["count"]):
        crown = crowns[i % len(crowns)] + Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), 0)) * spec.get("crown_jitter", 0.04)
        az = rng.uniform(0, math.tau)
        out = env.outward(crown) if (crown.xy - env.c.xy).length > 0.05 else Vector((math.cos(az), math.sin(az), 0))
        horiz = (Vector((math.cos(az), math.sin(az), 0)) * 0.6 + out * 0.4).normalized()
        low = rng.random() < lv0.get("low_frac", 0.0)
        elev = math.radians(rng.uniform(*lv0.get("low_elev" if low else "elev", lv0["elev"])))
        d = horiz * math.cos(elev) + UP * math.sin(elev)
        length = H * rng.uniform(*lv0.get("low_length" if low else "length", lv0["length"]))
        pts = grow_path(rng, env, crown, d, length, step, lv0.get("tropism", 0.3) * (0.4 if low else 1.0),
                        lv0.get("outward", 0.2), lv0.get("wander", 0.5), stop=lv0.get("stop", 0.95),
                        gravity=lv0.get("gravity", 0.0), origin=crown if spec.get("clumps") else None)
        br = add_branch(0, pts, spec["stem_radius"] * rng.uniform(0.75, 1.2) * (0.7 if low else 1.0), rng.random())
        if br:
            br["origin"] = crown
            stems.append(br)

    # Higher levels: children spiralling along each parent.
    parents = stems
    for level in range(1, len(levels)):
        lv = levels[level]
        children = []
        for par in parents:
            L = par["length"]
            s = L * lv.get("start", 0.3) + rng.uniform(0, 1.0 / lv["per_m"])
            az = rng.uniform(0, math.tau)
            made = 0
            while s < L * lv.get("end", 0.97) and made < lv.get("max", 99):
                node, tan, i = at(par["pts"], par["acc"], s)
                az += GOLDEN + rng.uniform(-0.3, 0.3)
                ratio = env.ratio(node)
                if ratio < lv.get("inner", 0.0) and rng.random() < lv.get("inner_skip", 0.7):
                    s += rng.uniform(0.6, 1.4) / lv["per_m"]
                    continue
                radial = Matrix.Rotation(az, 3, tan) @ tan.orthogonal().normalized()
                ang = math.radians(rng.uniform(*lv["angle"]))
                d = tan * math.cos(ang) + radial * math.sin(ang)
                o = env.outward(node)
                if spec.get("clumps") and par.get("origin") is not None:
                    oc = Vector((node.x - par["origin"].x, node.y - par["origin"].y, 0.0))
                    o = oc.normalized() if oc.length > 1e-3 else o
                d = (d + UP * lv.get("up", 0.2) + o * lv.get("out", 0.3)).normalized()
                remain = L - s
                if lv.get("abs_length"):
                    length = rng.uniform(*lv["abs_length"])
                else:
                    length = max(remain, L * 0.3) * rng.uniform(*lv["length"])
                pts = grow_path(rng, env, node, d, length, lv.get("step", step * 0.7), lv.get("tropism", 0.3),
                                lv.get("outward", 0.3), lv.get("wander", 0.8), stop=lv.get("stop", 1.02),
                                gravity=lv.get("gravity", 0.0),
                                origin=par.get("origin") if spec.get("clumps") else None)
                r0 = par["radii"][i] * rng.uniform(*lv.get("radius", (0.45, 0.65)))
                br = add_branch(level, pts, r0, par["phase"] + rng.uniform(0.02, 0.12), parent=par)
                if br:
                    br["origin"] = par.get("origin")
                    children.append(br)
                    made += 1
                s += rng.uniform(0.6, 1.4) / lv["per_m"]
        parents = children

    # Short leafy spurs along the outer branches thicken the foliage shell cheaply.
    sp = spec.get("spurs")
    if sp:
        for par in [b for b in branches if b["level"] >= sp.get("level", 1)]:
            L = par["length"]
            s = L * sp.get("start", 0.3) + rng.uniform(0, 1.0 / sp["per_m"])
            az = rng.uniform(0, math.tau)
            while s < L * 0.9:
                node, tan, i = at(par["pts"], par["acc"], s)
                az += GOLDEN + rng.uniform(-0.3, 0.3)
                if env.ratio(node) >= sp.get("inner", 0.5):
                    radial = Matrix.Rotation(az, 3, tan) @ tan.orthogonal().normalized()
                    ang = math.radians(rng.uniform(*sp["angle"]))
                    d = (tan * math.cos(ang) + radial * math.sin(ang) + UP * sp.get("up", 0.3) +
                         env.normal(node) * 0.3).normalized()
                    ln = rng.uniform(*sp["length"])
                    pts = [node + d * ln * t + UP * (-0.01 * t * t) for t in np.linspace(0, 1, 4)]
                    br = add_branch(min(par["level"] + 1, len(levels) - 1), pts, par["radii"][i] * 0.5,
                                    par["phase"] + rng.uniform(0.02, 0.1), parent=par)
                    if br:
                        br["level"] = par["level"] + 1
                s += rng.uniform(0.6, 1.4) / sp["per_m"]

    # Shell fill: leafy twigs from the nearest branch out to points spread through the outer
    # layer of the envelope, so the foliage mass is dense and even where the eye sees it.
    sh = spec.get("shell")
    if sh:
        anchors, owners = [], []
        for br in branches:
            if br["level"] > sh.get("max_level", 2):
                continue
            s = br["length"] * sh.get("from", 0.15)
            while s < br["length"]:
                node, _, i = at(br["pts"], br["acc"], s)
                anchors.append(tuple(node))
                owners.append((br, i))
                s += 0.05
        A = np.asarray(anchors) if anchors else np.zeros((0, 3))
        made, tries = 0, 0
        while made < sh["count"] and tries < sh["count"] * 30 and len(A):
            tries += 1
            q = Vector((env.c.x + rng.uniform(-1, 1) * env.rx * 1.05, env.c.y + rng.uniform(-1, 1) * env.ry * 1.05,
                        H * rng.uniform(sh.get("z_min", 0.04), 1.0)))
            r = env.ratio(q)
            if not (sh.get("depth", (0.78, 1.0))[0] <= r <= sh.get("depth", (0.78, 1.0))[1]):
                continue
            d2 = ((A - np.asarray(tuple(q))) ** 2).sum(1)
            j = int(np.argmin(d2))
            dist = math.sqrt(d2[j])
            if dist > sh.get("max_len", 0.5) or dist < 0.06:
                continue
            par, i = owners[j]
            p = Vector(anchors[j])
            bow = rng.uniform(0.01, 0.04)
            pts = [p + (q - p) * t + UP * (bow * math.sin(math.pi * t)) for t in np.linspace(0, 1, max(3, int(dist / 0.03)))]
            br = add_branch(len(levels) - 1, pts, min(par["radii"][i] * 0.5, sh.get("radius", 0.0035)),
                            par["phase"] + rng.uniform(0.02, 0.1), parent=par)
            if br:
                br["origin"] = par.get("origin")
                made += 1

    # Leaves on the outer end of every branch at or above leaf_level.
    leaf = spec["leaf"]
    arrangement = leaf.get("arrangement", "alternate")
    for br in branches:
        if br["level"] < leaf.get("level", 1):
            continue
        L = br["length"]
        zone = min(L, leaf["zone"] * (1.0 + 0.3 * (len(levels) - 1 - br["level"])))
        s0 = max(L - zone, 0.0)
        mid, _, _ = at(br["pts"], br["acc"], (s0 + L) * 0.5)
        outer = env.ratio(mid)
        density = 1.0 if outer >= leaf.get("inner", 0.55) else leaf.get("inner_density", 0.3)
        tw_leaves = []
        s = s0 + rng.uniform(0, leaf["spacing"][0])
        az = rng.uniform(0, math.tau)
        k = 0
        while s < L - 0.004:
            node, tan, _ = at(br["pts"], br["acc"], s)
            pair = (0.0, math.pi) if arrangement == "opposite" else (0.0,)
            for extra in pair:
                if rng.random() > density:
                    continue
                a = az + extra + rng.uniform(-0.25, 0.25)
                radial = Matrix.Rotation(a, 3, tan) @ tan.orthogonal().normalized()
                ang = math.radians(rng.uniform(*leaf["angle"]))
                d = tan * math.cos(ang) + radial * math.sin(ang)
                surf = env.normal(node)
                d = (d + surf * leaf.get("face_out", 0.35) - UP * leaf.get("hang", 0.1)).normalized()
                jitter = _rand_unit(rng) * leaf.get("jitter", 0.35)
                up = (UP * leaf.get("face_up", 0.9) + surf * 0.5 + jitter).normalized()
                if abs(up.dot(d)) > 0.95:
                    up = (up + surf).normalized()
                from_tip = L - s
                size = leaf["size"] * rng.uniform(0.85, 1.15)
                size *= leaf.get("tip_min", 0.55) + (1 - leaf.get("tip_min", 0.55)) * min(1.0, from_tip / leaf.get("tip_len", 0.10))
                if outer < leaf.get("inner", 0.55):
                    size *= 0.85
                pet = leaf.get("petiole", 0.0) * size
                base = node + d * pet
                if leaf.get("flat"):
                    # Blade held level on its petiole (large shade leaves).
                    flat = Vector((d.x, d.y, d.z * (1 - leaf["flat"]))) + UP * rng.uniform(-0.12, 0.08)
                    d = flat.normalized() if flat.length > 1e-3 else d
                    up = (UP * 1.5 + jitter * 0.6).normalized()
                tw_leaves.append(dict(base=base, node=node, dir=d, up=up, length=size, outer=outer,
                                      from_tip=from_tip, k=k, fold=rng.uniform(*leaf.get("fold", (0.1, 0.3))),
                                      droop=rng.uniform(*leaf.get("droop", (0.05, 0.3))),
                                      curl=rng.uniform(*leaf.get("curl", (0.0, 0.08))),
                                      twist=rng.uniform(-0.3, 0.3), phase=br["phase"]))
                k += 1
            az += math.pi / 2 if arrangement == "opposite" else leaf.get("divergence", GOLDEN)
            s += rng.uniform(*leaf["spacing"])
        if not tw_leaves:
            continue
        start, _, _ = at(br["pts"], br["acc"], s0)
        end = br["pts"][-1]
        chord = end - start
        if chord.length < 1e-3:
            chord = br["pts"][-1] - br["pts"][-2]
        surf = env.normal(mid)
        up = (UP * 0.8 + surf * 0.6).normalized()
        twigs.append(dict(branch=br, start=start, end=end, dir=chord.normalized(), up=up, zone=L - s0,
                          outer=outer, leaves=tw_leaves, phase=br["phase"], full=False,
                          spray=rng.randrange(1 << 16), spin=rng.uniform(-0.4, 0.4)))
        br["twig"] = twigs[-1]
    ranked = sorted(twigs, key=lambda t: -t["outer"])
    nfull = int(len(ranked) * spec.get("full", 0.3))
    for i, tw in enumerate(ranked):
        tw["full"] = i < nfull
    return dict(branches=branches, twigs=twigs, env=env, height=H)


def emit(desc, atlas, lod, spec, leaf_key, spray_key):
    """Geometry for one LOD. ``leaf_key(leaf, twig)`` and ``spray_key(twig)`` pick atlas tiles."""
    b = F.Batch(height=desc["height"])
    stats = {"wood": 0, "leaves": 0, "sprays": 0}
    max_level = spec.get("lod_levels", (9, 9, 1))[lod]
    sides_by_level = spec.get("sides", [(6, 4, 3), (4, 3, 3), (3, 3, 3), (3, 3, 3)])
    seg = spec.get("lod_step", (0.03, 0.08, 0.18))[lod]
    v_bark = spec.get("v_bark", 0.6)
    for br in desc["branches"]:
        if br["level"] > max_level:
            continue
        t0 = b.triangles
        pts, radii = br["pts"], br["radii"]
        tw = br.get("twig")
        lod2_leaves = spec.get("lod2_leaves", 0.0)
        if tw is not None and ((lod == 2 and not (lod2_leaves and tw["full"])) or not tw["full"]):
            # The spray card paints this twig's leafy end; only model the bare part below it.
            cut = next((i for i, s in enumerate(br["acc"]) if s >= br["length"] - tw["zone"] + 0.02), len(pts))
            pts, radii = pts[:cut], radii[:cut]
            if len(pts) < 2:
                continue
        spacing = br["length"] / max(len(br["pts"]) - 1, 1)
        stride = max(1, int(round(seg * (1.0 if br["level"] == 0 else 1.3) / max(spacing, 1e-4))))
        idx = list(range(0, len(pts), stride))
        if idx[-1] != len(pts) - 1:
            idx.append(len(pts) - 1)
        sides = sides_by_level[min(br["level"], len(sides_by_level) - 1)][lod]
        flutter = 0.0 if br["level"] < 2 else 0.12
        b.tube([pts[i] for i in idx], [radii[i] for i in idx], sides, atlas.uv(br["bark"]), v_length=v_bark,
               v_offset=br["phase"] * 3.1, phase=br["phase"], flutter=flutter, cap=lod == 0 and br["level"] == 0,
               roll=br["phase"] * 6.28)
        stats["wood"] += b.triangles - t0
    leaf_grid = spec.get("leaf_grid", ((2, 2), (1, 2), (1, 1)))
    spray_grid = spec.get("spray_grid", ((2, 2), (1, 1), (1, 1)))
    for tw in desc["twigs"]:
        t0 = b.triangles
        keep = spec.get("lod2_leaves", 0.0)
        if tw["full"] and (lod < 2 or keep) and not (lod == 1 and spec.get("lod1_sprays_only")):
            rows, cols = leaf_grid[lod]
            pet_key = spec["leaf"].get("petiole_tube") if lod < 2 else None
            for li, lf in enumerate(tw["leaves"]):
                gscale = 1.0
                if lod == 2:
                    # Far LOD with few large leaves: keep a share of real leaves, enlarged to hold coverage.
                    if ((li * 0.618 + tw["phase"]) % 1.0) >= keep:
                        continue
                    gscale = min(1.3, keep ** -0.5)
                key = leaf_key(lf, tw)
                width = lf["length"] * gscale * tile_aspect(atlas, key)
                if pet_key and (lf["base"] - lf["node"]).length > 0.01:
                    r = spec["leaf"].get("petiole_radius", 0.0018)
                    mid = lf["node"].lerp(lf["base"], 0.5) + UP * 0.25 * (lf["base"] - lf["node"]).length
                    b.tube([lf["node"], mid, lf["base"]] if lod == 0 else [lf["node"], lf["base"]],
                           [r, r * 0.85, r * 0.7] if lod == 0 else [r, r * 0.7], 3, atlas.uv(pet_key),
                           v_length=spec.get("v_bark", 0.6), phase=tw["phase"], flutter=0.3)
                b.card(lf["base"], lf["dir"], lf["up"], lf["length"] * gscale, width, atlas.uv(key), rows=rows, cols=cols,
                       fold=lf["fold"], curl=lf["curl"] if lod == 0 else 0.0, droop=lf["droop"],
                       twist=lf["twist"] if lod == 0 else 0.0, phase=tw["phase"], flutter=1.0,
                       flutter_base=0.25)
            stats["leaves"] += b.triangles - t0
            continue
        key = spray_key(tw)
        length = (tw["zone"] + spec["leaf"]["size"] * spec.get("spray_overhang", 0.7)) * spec.get("spray_scale", 1.0)
        length *= (1.0, 1.05, 1.12)[lod]
        width = length * tile_aspect(atlas, key)
        base = tw["start"] - tw["dir"] * spec["leaf"]["size"] * 0.15
        rows, cols = spray_grid[lod]
        up = Matrix.Rotation(tw["spin"], 3, tw["dir"]) @ tw["up"]
        b.card(base, tw["dir"], up, length, width, atlas.uv(key), rows=rows, cols=cols, fold=0.18 if lod < 2 else 0.0,
               droop=0.12, phase=tw["phase"], flutter=0.8, flutter_base=0.2)
        if lod == 0 and spec.get("spray_cross"):
            up2 = Matrix.Rotation(1.2, 3, tw["dir"]) @ up
            b.card(base, tw["dir"], up2, length * 0.9, width * 0.9, atlas.uv(key), rows=1, cols=cols,
                   fold=0.1, droop=0.1, phase=tw["phase"], flutter=0.8, flutter_base=0.2)
        stats["sprays"] += b.triangles - t0
    print("HOMESTEAD_TRIS", lod, stats)
    return b


# ------------------------------------------------------------------ painting helpers

def over_window(layer, X, Y, center, radius, paint):
    """Composite ``paint(Xs, Ys, window_slices)`` (a Layer for the sub-grid) onto ``layer`` only
    inside the square window of ``radius`` tile units around ``center``; much faster than
    painting every small leaf over the whole tile."""
    xs, ys = X[0], Y[:, 0]
    i0 = int(np.searchsorted(ys, center[1] - radius))
    i1 = int(np.searchsorted(ys, center[1] + radius))
    j0 = int(np.searchsorted(xs, center[0] - radius))
    j1 = int(np.searchsorted(xs, center[0] + radius))
    if i1 - i0 < 4 or j1 - j0 < 4:
        return
    sl = (slice(i0, i1), slice(j0, j1))
    sub = F.Layer((i1 - i0, j1 - j0))
    sub.color, sub.alpha, sub.height = layer.color[sl], layer.alpha[sl], layer.height[sl]
    sub.rough, sub.trans, sub.ao = layer.rough[sl], layer.trans[sl], layer.ao[sl]
    sub.over(paint(X[sl], Y[sl]))
    layer.color[sl], layer.alpha[sl], layer.height[sl] = sub.color, sub.alpha, sub.height
    layer.rough[sl], layer.trans[sl], layer.ao[sl] = sub.rough, sub.trans, sub.ao


def paint_spray(atlas, key, nrng, rng, blade, count, *, leaf_len=0.30, angle=(40, 60), arrangement="alternate",
                twig_color=(0.07, 0.05, 0.03), twig_width=0.010, twig_top=0.80, tip_leaf=True,
                keys=None, squash=(0.65, 1.0), taper=0.55):
    """A twig seen from above with ``count`` leaves, painted into tile ``key``. ``blade(Xl, Yl,
    pxl, leaf_index, key)`` paints one leaf in its own blade coordinates and returns a Layer."""
    X, Y, px = atlas.grid(key)
    layer = F.Layer(X.shape)
    layer.color[...] = twig_color
    bend = rng.uniform(-0.04, 0.04)
    twig = [(bend * math.sin(math.pi * t), 0.02 + twig_top * t) for t in np.linspace(0, 1, 12)]
    d, along = F.polyline_distance(X, Y, twig)
    w = twig_width * (1 - 0.6 * along)
    stalk = F.Layer(X.shape)
    stalk.color = F.lerp(np.asarray(twig_color) * 0.8, np.asarray(twig_color) * 1.3,
                         F.noise(X.shape, nrng, freq=30.0, beta=2.0))
    stalk.alpha = np.clip(-(d - w) / px + 0.5, 0, 1)
    stalk.height = 0.0004 * np.sqrt(np.clip(1 - (d / np.maximum(w, px)) ** 2, 0, 1))
    stalk.trans[...] = 0.05
    layer.over(stalk)
    side = 1
    for i in range(count):
        t = (i + 0.6) / (count + (0.3 if tip_leaf else 0.8))
        y = 0.02 + twig_top * t
        x = bend * math.sin(math.pi * t)
        if arrangement == "opposite":
            sides = (-1, 1)
        else:
            sides = (side,)
            side = -side
        for sd in sides:
            th = sd * math.radians(rng.uniform(*angle)) * (1.0 - 0.35 * t)
            ln = leaf_len * (1.0 - taper * t ** 1.5) * rng.uniform(0.9, 1.08)
            sq = rng.uniform(*squash)
            k = keys[i % len(keys)] if keys else None
            ctr = (x + 0.5 * ln * math.sin(th), y + 0.5 * ln * math.cos(th))
            over_window(layer, X, Y, ctr, ln * 0.72,
                        lambda Xs, Ys, o=(x, y), th=th, ln=ln, sq=sq, i=i, k=k:
                        blade(*rotated(Xs, Ys, o, th, ln, sq), px / ln, i, k))
    if tip_leaf:
        ln = leaf_len * (1.0 - taper * 0.9) * rng.uniform(0.95, 1.1)
        Xl, Yl = rotated(X, Y, twig[-1], rng.uniform(-0.1, 0.1), ln, 0.95)
        layer.over(blade(Xl, Yl, px / ln, count, keys[0] if keys else None))
    return layer


def paint_bark(atlas, key, nrng, base, alt, *, blotch=0.5, stri=0.35, aniso=12.0, lenticels=0.0,
               lent_color=(0.20, 0.18, 0.15), cracks=0.0, crack_color=(0.03, 0.025, 0.02), rough=0.62,
               lichen=0.0, v_len=0.6, relief=0.00025):
    """Tileable bark column (u around, v along) with blotches, striations, lenticels, cracks."""
    U, V = atlas.column_grid(key)
    shp = U.shape
    st = F.noise(shp, nrng, freq=40.0, beta=1.8, aniso=(1.0, aniso))
    bl = F.noise(shp, nrng, freq=6.0, beta=2.6, aniso=(1.0, 3.0))
    fine = F.noise(shp, nrng, freq=140.0, beta=1.2)
    color = F.lerp(base, alt, F.smoothstep(0.35, 0.8, bl) * blotch)
    color = color * (1 - stri * 0.5 + stri * st)[..., None]
    height = relief * st
    r = rough + 0.1 * st
    if lenticels:
        lent = F.smoothstep(0.88, 0.95, F.noise(shp, nrng, freq=45.0, beta=1.4, aniso=(3.0, 1.0)))
        color = F.lerp(color, lent_color, lent * lenticels)
        height = height + 0.0002 * lent * lenticels
    if cracks:
        cr = F.smoothstep(0.90, 0.98, F.ridged(F.noise(shp, nrng, freq=22.0, beta=2.2, aniso=(1.0, 6.0))))
        color = F.lerp(color, crack_color, cr * cracks)
        height = height - 0.0003 * cr * cracks
        r = r + 0.1 * cr * cracks
    if lichen:
        li = F.smoothstep(0.78, 0.9, F.noise(shp, nrng, freq=18.0, beta=2.3))
        color = F.lerp(color, (0.20, 0.22, 0.16), li * lichen)
    color = F.lerp(color, np.asarray(color) * 1.15, F.smoothstep(0.9, 1.0, fine) * 0.4)
    layer = F.Layer(shp)
    layer.color, layer.height, layer.rough = color, height, np.clip(r, 0.3, 0.95)
    layer.trans[...] = 0.03
    layer.ao = 0.9 + 0.1 * st
    atlas.put(key, layer, meters_per_px=v_len / shp[0], wrap=True, opaque=True)
    return layer
