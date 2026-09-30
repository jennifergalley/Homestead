"""The cove route: an on-foot path from the manor's fallen south front door down to the sand at the head of
the cove, with granite steps where the valley side is too steep for a path (add-cove-route).

The route is designed, not searched, each run: CONTROL holds its turning points and what kind of way leaves
each one. They were found with `--search` (a grade-aware least-cost search over the heightfield: paths at
1 in 7 or gentler, steps where steeper, a penalty on turns so it makes long traverses and a few real
switchbacks, and 2.6 m clear of the interactive trees and big stones) and then eased by hand.

- A "path" leg is a graded earth path, no steeper than MAX_PATH_GRADE, following the ground as closely as
  that allows (the least-worst profile between its ends, as road_grade.py does for the road).
- A "stairs" leg is straight and becomes flights of granite steps (Props' add-cove-route-kit: rise 15-17 cm,
  going 30-35 cm, at most 12 risers, the flight pitched within a degree of the kit's 26/28/30 degree raked
  rails) with landings of at least 1.2 m between them and a square landing where two stair legs meet.
- The heights where legs meet are solved together (least squares to the ground, within each leg's limits),
  so the steps take the steep valley side and the paths never exceed 1 in 7: every drop steeper than a path
  is on steps with a rail (the final descent to the beach is never an exposed dirt slope).
- The heightfield is cut to the design: the path's bed, and a few centimetres under every tread and landing
  (TREAD_CLEARANCE_M), blending back to the ground over a short verge.
- Kerbs stand on a path's edge where the ground falls away beside it, and oak rails where it falls further,
  and on the drop side of every flight; fingerposts stand at the front door and at the head of the cliff
  steps.

    python Scripts\\Terrain\\cove_route.py            # grade once, then write the route data and bakes' inputs
    python Scripts\\Terrain\\cove_route.py --dry      # design and report only (Saved/CoveRoute/), write nothing
    python Scripts\\Terrain\\cove_route.py --search   # re-run the least-cost search and print candidate CONTROL

Writes (in step): Content/SurvivalGame/Estate/Runtime/EstateHeightfield.r16, Scripts/Terrain/
Estate_Heightmap_4033.png and <work>/game_reshaped_4033.npy (once: "coveRoute.graded" in the layout; a
later run leaves the heightfield alone and only brings a stale work npy into step), estate_layout.json
("coveRoute", and "footpaths" for the map), Source/SurvivalGame/Simulation/HomesteadEstateCoveRoute.inc (the
runtime builder's and the native tests' data), and clears the baked scatter off the route
(EstateScenery.bin; scatter.py re-applies clear_scenery_records after a fresh scatter). Then run
bake_ground.py and bake_estate_map.py, and in the editor ApplyEstateHeightfield over the rectangle it
prints, build_ground.py and ImportEstateMap. The route's plan and profile go to Saved/CoveRoute/.
"""
import json
import math
import os
import sys

import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter, gaussian_filter1d
from scipy.optimize import minimize
from scipy.spatial import cKDTree

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
R16 = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateHeightfield.r16")
PNG = os.path.join(HERE, "Estate_Heightmap_4033.png")
LAYOUT = os.path.join(HERE, "estate_layout.json")
SCENERY = os.path.join(REPO, "Content", "SurvivalGame", "Estate", "Runtime", "EstateScenery.bin")
INC = os.path.join(REPO, "Source", "SurvivalGame", "Simulation", "HomesteadEstateCoveRoute.inc")
DATA = os.path.join(HERE, "cove_route.json")     # the full design (generated; the layout keeps a summary)
SAVED = os.path.join(REPO, "Saved", "CoveRoute")
SIZE, H = 4033, 2016

# (x, y) metres (+X north, +Y east), and the kind of way that leaves the point toward the next one.
CONTROL = [
    (-260.5, -654.5, "path"),     # outside the fallen front door on the ruin's south front
    (-280.0, -647.0, "path"),
    (-290.0, -637.0, "path"),
    (-342.0, -611.0, "path"),     # first switchback
    (-344.0, -651.0, "path"),
    (-363.0, -673.0, "path"),
    (-372.0, -676.0, "path"),     # second switchback
    (-389.0, -662.0, "path"),
    (-404.0, -632.0, "path"),
    (-431.0, -607.0, "path"),
    (-460.0, -580.0, "path"),
    (-461.0, -541.0, "path"),     # third switchback, at the head of the valley side
    (-493.0, -557.0, "stairs"),   # head of the cliff steps (fingerpost)
    (-511.0, -548.0, "stairs"),
    (-526.0, -533.0, "stairs"),
    (-536.0, -511.0, "path"),     # a bench above the valley floor
    (-524.0, -487.0, "stairs"),
    (-521.0, -480.0, "path"),     # the valley floor, 7 m off the river
    (-538.0, -487.0, "stairs"),
    (-543.0, -492.0, "path"),     # the foot of the last steps, onto the sand
    (-556.0, -521.0, None),       # the sand at the head of the cove, west of the river mouth
]

MAX_PATH_GRADE = 1.0 / 7.0       # a steep Cornish coast path, still walked without steps
DESIGN_MARGIN = 0.97             # paths are designed this far inside the limit (chords on curves read steeper)
STAIR_GRADE = (0.20, 0.42)       # a stair leg's mean fall per metre (flights and landings together)
RISE_M = (0.15, 0.17)            # Props' kit (add-cove-route-kit design.md)
GOING_M = (0.30, 0.35)
RAIL_PITCHES_DEG = (26.0, 28.0, 30.0)
PITCH_TOLERANCE_DEG = 1.0        # a flight's pitch is within this of a raked rail bay's
MAX_RISERS = 12
MIN_RISERS = 3
LANDING_M = (1.2, 3.0)           # between flights
MAX_HEAD_LANDING_M = 4.0         # a flat landing at the head of a stair leg takes any length left over
CORNER_HALF_M = 0.75             # a corner landing where two stair legs meet: 1.5 m square
CLEAR_HALF_M = 0.70              # 1.4 m clear width
TREAD_HALF_M = 0.75              # 1.5 m treads
KERB_OFFSET_M = CLEAR_HALF_M     # kerb pivot: the path-side top edge
KERB_PROUD_M = 0.20
RAIL_OFFSET_M = CLEAR_HALF_M + 0.10
RAIL_BAY_M = (1.6, 1.8)
RAIL_CLEAR_M = 0.0               # a rail stands outside another leg's clear width too
TREAD_CLEARANCE_M = 0.05         # the ground stays at least this far under a tread's or landing's top
PATH_BED_HALF_M = 1.2            # heightfield cells set to the path's bed this far out
STAIR_BED_HALF_M = 2.2           # ... and to the stairs' (a tread edge's bilinear cell reaches 0.75 + 1.42 m)
VERGE_M = 2.5                    # then blended back to the ground
RIVER_MARGIN_M = 2.0             # corridor cells never touch the river channel (its half width plus this)
KERB_DROP_M = 0.45               # the ground 2 m out from the path's edge this far below its bed: a kerb
RAIL_DROP_M = 1.1                # ... this far: a rail
SIDE_PROBE_M = 2.0
MIN_RUN_M = 3.0                  # kerb and rail runs shorter than this are dropped, gaps shorter bridged
CLEAR_SCENERY_M = 2.4            # half width of the scatter cleared along the route
STATION_M = 0.25
CHAIKIN_PASSES = 3
SMOOTH_GROUND_M = 1.5            # the heightfield's lumps the path's bed evens out


# ---- heightfield ------------------------------------------------------------------------------------
def load_r16():
    raw = np.fromfile(R16, "<u2").reshape(SIZE, SIZE)
    return raw, (raw.astype(np.float64) - 32768.0) / 128.0     # [row = y + H, col = x + H]


def bilinear(z, x, y):
    """z [row = y + H, col = x + H] at world metres (x north, y east)."""
    c = np.asarray(x, np.float64) + H
    r = np.asarray(y, np.float64) + H
    c0 = np.clip(np.floor(c).astype(int), 0, SIZE - 2)
    r0 = np.clip(np.floor(r).astype(int), 0, SIZE - 2)
    fc, fr = c - c0, r - r0
    return ((z[r0, c0] * (1 - fc) + z[r0, c0 + 1] * fc) * (1 - fr)
            + (z[r0 + 1, c0] * (1 - fc) + z[r0 + 1, c0 + 1] * fc) * fr)


# ---- plan -------------------------------------------------------------------------------------------
def chaikin(points, passes):
    p = np.asarray(points, np.float64)
    for _ in range(passes):
        q = [p[0]]
        for a, b in zip(p[:-1], p[1:]):
            q += [0.75 * a + 0.25 * b, 0.25 * a + 0.75 * b]
        q.append(p[-1])
        p = np.array(q)
    return p


def resample(points, step):
    p = np.asarray(points, np.float64)
    seg = np.hypot(*np.diff(p, axis=0).T)
    s = np.r_[0.0, np.cumsum(seg)]
    n = max(2, int(math.ceil(s[-1] / step)) + 1)
    t = np.linspace(0.0, s[-1], n)
    return np.c_[np.interp(t, s, p[:, 0]), np.interp(t, s, p[:, 1])], t


def pieces():
    """The route as runs: ("path", smoothed polyline) or ("stairs", straight leg), joined at nodes."""
    runs = []
    k = 0
    while k < len(CONTROL) - 1:
        kind = CONTROL[k][2]
        j = k
        if kind == "path":
            while j < len(CONTROL) - 1 and CONTROL[j][2] == "path":
                j += 1
            pts = chaikin([c[:2] for c in CONTROL[k:j + 1]], CHAIKIN_PASSES)
        else:
            j = k + 1
            pts = np.array([CONTROL[k][:2], CONTROL[j][:2]], np.float64)
        runs.append({"kind": kind, "points": pts, "from": k, "to": j})
        k = j
    return runs


# ---- profile ----------------------------------------------------------------------------------------
def stair_allowances(runs, i):
    """Flat length reserved at a stair leg's start and end (a corner landing where it meets another)."""
    before = i > 0 and runs[i - 1]["kind"] == "stairs"
    after = i + 1 < len(runs) and runs[i + 1]["kind"] == "stairs"
    return (CORNER_HALF_M if before else 0.0), (CORNER_HALF_M if after else 0.0)


def solve_nodes(runs, ground):
    """Heights at the runs' ends: nearest the ground in least squares, within every run's limits."""
    nodes = [runs[0]["from"]] + [r["to"] for r in runs]
    target = np.array([ground(*CONTROL[n][:2]) for n in nodes])
    cons = []
    for i, r in enumerate(runs):
        length = float(np.hypot(*np.diff(r["points"], axis=0).T).sum())
        if r["kind"] == "path":
            lim = MAX_PATH_GRADE * DESIGN_MARGIN * length
            cons += [{"type": "ineq", "fun": (lambda z, i=i, lim=lim: lim - (z[i + 1] - z[i]))},
                     {"type": "ineq", "fun": (lambda z, i=i, lim=lim: lim + (z[i + 1] - z[i]))}]
        else:
            a, b = stair_allowances(runs, i)
            run = length - a - b
            sign = 1.0 if target[i + 1] > target[i] else -1.0          # the steps climb toward the higher end
            lo, hi = STAIR_GRADE[0] * run, STAIR_GRADE[1] * run
            cons += [{"type": "ineq", "fun": (lambda z, i=i, s=sign, lo=lo: s * (z[i + 1] - z[i]) - lo)},
                     {"type": "ineq", "fun": (lambda z, i=i, s=sign, hi=hi: hi - s * (z[i + 1] - z[i]))}]
    # The front door's forecourt and the beach keep their levels.
    pins = [0, len(nodes) - 1]
    weight = np.ones(len(nodes))
    weight[pins] = 100.0
    res = minimize(lambda z: float((weight * (z - target) ** 2).sum()), target, constraints=cons, method="SLSQP",
                   options={"maxiter": 500, "ftol": 1e-10})
    if not res.success:
        raise SystemExit(f"cove route: node heights not solvable ({res.message})")
    return res.x, target


def pinned_lipschitz(target, ds, grade, za, zb):
    """The profile within `grade` whose worst deviation from target is smallest, through za and zb."""
    n = len(target)
    step = grade * ds
    lo, hi = target.copy(), target.copy()
    for k in range(1, n):
        lo[k] = min(lo[k], lo[k - 1] + step)
        hi[k] = max(hi[k], hi[k - 1] - step)
    for k in range(n - 2, -1, -1):
        lo[k] = min(lo[k], lo[k + 1] + step)
        hi[k] = max(hi[k], hi[k + 1] - step)
    mid = (lo + hi) / 2.0
    s = np.arange(n) * ds
    mid = gaussian_filter1d(mid, 2.0 / ds, mode="nearest")       # vertical curves (keeps the grade)
    below = np.maximum(za - grade * s, zb - grade * (s[-1] - s))
    above = np.minimum(za + grade * s, zb + grade * (s[-1] - s))
    return np.clip(mid, below, above)


def flights_for(drop, run):
    """Flights and landings for a straight stair leg: (risers per flight, rise, going, landing, head landing,
    pitch of the nearest raked rail). None if nothing fits."""
    best = None
    for n in range(int(math.ceil(drop / RISE_M[1] - 1e-9)), int(math.floor(drop / RISE_M[0] + 1e-9)) + 1):
        if n < MIN_RISERS:
            continue
        rise = drop / n
        for pitch in RAIL_PITCHES_DEG:
            going = rise / math.tan(math.radians(pitch))
            going = min(max(going, GOING_M[0]), GOING_M[1])
            actual = math.degrees(math.atan2(rise, going))
            if abs(actual - pitch) > PITCH_TOLERANCE_DEG:
                continue
            for f in range(int(math.ceil(n / MAX_RISERS)), n // MIN_RISERS + 1):
                spare = run - n * going
                if f == 1:
                    landing, head = 0.0, spare
                else:
                    landing = min(max(spare / (f - 1), LANDING_M[0]), LANDING_M[1])
                    head = spare - landing * (f - 1)
                if head < -1e-6 or head > MAX_HEAD_LANDING_M:
                    continue
                split = [n // f + (1 if q < n % f else 0) for q in range(f)]
                # Prefer the least left over at the head (steps spread along the leg), then the gentlest
                # pitch, then fewer flights.
                score = (round(head / 0.5), pitch, f)
                if best is None or score < best[0]:
                    best = (score, (split, rise, going, landing, head, pitch))
                break
    return best[1] if best else None


def design(runs, ground_fn):
    zs, target = solve_nodes(runs, ground_fn)
    stations = []        # x, y, bed z, walk z, kind, run index
    flights, landings = [], []
    for i, r in enumerate(runs):
        za, zb = zs[i], zs[i + 1]
        if r["kind"] == "path":
            pts, s = resample(r["points"], STATION_M)
            tgt = gaussian_filter1d(ground_fn(pts[:, 0], pts[:, 1]), 3.0 / STATION_M, mode="nearest")
            prof = pinned_lipschitz(tgt, s[1] - s[0], MAX_PATH_GRADE * DESIGN_MARGIN, za, zb)
            for k, ((x, y), z) in enumerate(zip(pts, prof)):
                if k == 0 and stations:
                    continue                                      # the join is the previous run's last station
                stations.append((x, y, z, z, "path", i))
            r["length"] = float(s[-1])
            continue
        a, b = r["points"]
        length = float(np.hypot(*(b - a)))
        t = (b - a) / length
        cut0, cut1 = stair_allowances(runs, i)
        climbing = zb > za
        r["descends"] = not climbing
        foot, head = (a, b) if climbing else (b, a)          # steps are laid from the foot up
        zf, zh = (za, zb) if climbing else (zb, za)
        up = t if climbing else -t
        e_foot, e_head = (cut0, cut1) if climbing else (cut1, cut0)
        run = length - e_foot - e_head
        plan = flights_for(zh - zf, run)
        if plan is None:
            raise SystemExit(f"cove route: no flights fit stair leg {i} (fall {zh - zf:.2f} m over {run:.2f} m)")
        split, rise, going, landing, head_landing, pitch = plan
        yaw = math.degrees(math.atan2(up[1], up[0]))
        r.update(length=length, rise=rise, going=going, risers=sum(split), flights=len(split),
                 landing=landing, headLanding=head_landing, pitch=pitch, yaw=yaw, footZ=zf, headZ=zh)
        # Walk the leg from its foot: (s from the foot, walk z, bed z).
        prof = []
        s0, z0 = e_foot, zf
        if e_foot > 0:
            prof.append((0.0, e_foot, zf, zf - TREAD_CLEARANCE_M))                 # corner landing
        for q, n in enumerate(split):
            start = foot + up * s0
            flights.append({"x": start[0], "y": start[1], "z": z0 + rise, "yaw": yaw, "rise": rise,
                            "going": going, "treads": n, "pitch": pitch, "leg": i})
            prof.append(("flight", s0, z0, n, rise, going))
            s0 += n * going
            z0 += n * rise
            if q < len(split) - 1:
                c = foot + up * s0
                landings.append({"x": c[0], "y": c[1], "z": z0, "yaw": yaw, "length": landing, "leg": i})
                prof.append((s0, s0 + landing, z0, z0 - TREAD_CLEARANCE_M))
                s0 += landing
        # A flat landing at the head takes what's left over; where another stair leg starts here it runs on
        # through the corner (CORNER_HALF_M past the turning point, under the next leg's foot).
        top = head_landing + (e_head + CORNER_HALF_M if e_head > 0 else 0.0)
        if top > 0.05:
            c = foot + up * s0
            # "turn": how far along it the route turns onto the next stair leg (None: no turn on it).
            landings.append({"x": c[0], "y": c[1], "z": z0, "yaw": yaw, "length": top, "leg": i,
                             "turn": head_landing + e_head if e_head > 0 else None})
        if head_landing > 0.05:
            prof.append((s0, s0 + head_landing, z0, z0 - TREAD_CLEARANCE_M))
            s0 += head_landing
        if e_head > 0:
            prof.append((s0, s0 + e_head, z0, z0 - TREAD_CLEARANCE_M))            # corner landing
        assert abs(z0 - zh) < 1e-6 and abs(s0 + e_head - length) < 1e-6, (i, z0, zh, s0, length)

        def at(s):
            for item in prof:
                if item[0] == "flight":
                    _, f0, fz, n, rr, gg = item
                    if f0 - 1e-9 <= s <= f0 + n * gg + 1e-9:
                        k = min(int((s - f0) / gg), n - 1)
                        walk = fz + (k + 1) * rr                              # tread k's top
                        line = fz + (s - f0) / gg * rr - rr - TREAD_CLEARANCE_M
                        return walk, max(fz, line) if s - f0 < gg else line
                elif item[0] - 1e-9 <= s <= item[1] + 1e-9:
                    return item[2], item[3]
            return (zf, zf) if s <= 0 else (zh, zh)

        n_st = max(2, int(math.ceil(length / STATION_M)) + 1)
        order = np.linspace(0.0, length, n_st)
        pts = []
        for s in order:
            walk, bed = at(s)
            p = foot + up * s
            pts.append((p[0], p[1], bed, walk))
        if not climbing:
            pts.reverse()
        for k, (x, y, bed, walk) in enumerate(pts):
            if k == 0 and stations:
                continue
            stations.append((x, y, bed, walk, "stairs", i))
    return zs, target, stations, flights, landings


# ---- edges, kerbs, rails, signs ---------------------------------------------------------------------
def headings(xy):
    d = np.gradient(xy, axis=0)
    return np.degrees(np.arctan2(d[:, 1], d[:, 0]))


def runs_of(mask, s):
    """[start, end) index ranges where mask holds, gaps under MIN_RUN_M bridged, runs under it dropped."""
    out = []
    k = 0
    n = len(mask)
    while k < n:
        if not mask[k]:
            k += 1
            continue
        j = k
        while j < n and mask[j]:
            j += 1
        if out and s[k] - s[out[-1][1] - 1] < MIN_RUN_M:
            out[-1] = (out[-1][0], j)
        else:
            out.append((k, j))
        k = j
    return [(a, b) for a, b in out if s[b - 1] - s[a] >= MIN_RUN_M]


def edges(stations, flights, landings, runs, ground_fn):
    xy = np.array([(st[0], st[1]) for st in stations])
    bed = np.array([st[2] for st in stations])
    walk = np.array([st[3] for st in stations])
    kind = np.array([st[4] for st in stations])
    s = np.r_[0.0, np.cumsum(np.hypot(*np.diff(xy, axis=0).T))]
    hd = headings(xy)
    # A piece's +Y at yaw = heading: the walker's right going down the route ("left"/"right" below name the
    # +Y and -Y sides).
    left = np.c_[-np.sin(np.radians(hd)), np.cos(np.radians(hd))]
    drop = {}
    for side, sign in (("left", 1.0), ("right", -1.0)):
        probe = xy + left * sign * (CLEAR_HALF_M + SIDE_PROBE_M)
        drop[side] = walk - ground_fn(probe[:, 0], probe[:, 1])
    kerbs, rails = [], []
    # Kerbs on paths where the ground falls away beside them.
    for side, sign in (("left", 1.0), ("right", -1.0)):
        want = (kind == "path") & (drop[side] > KERB_DROP_M)
        for a, b in runs_of(want, s):
            n = max(1, int(round(s[b - 1] - s[a])))
            for q in range(n):
                t = s[a] + q + 0.5
                k = min(np.searchsorted(s, t), len(s) - 1)
                p = xy[k] + left[k] * sign * KERB_OFFSET_M
                yaw = hd[k] if sign > 0 else hd[k] + 180.0
                kerbs.append({"x": p[0], "y": p[1], "z": walk[k] + KERB_PROUD_M, "yaw": yaw % 360.0})
    # Level rails on paths and landings where the drop is worse; raked rails on every flight's drop side.
    def level_bays(a, b, sign):
        span = s[b - 1] - s[a]
        n = max(1, int(round(span / RAIL_BAY_M[1] + 0.49)))
        bay = span / n
        for q in range(n):
            k = min(np.searchsorted(s, s[a] + q * bay), len(s) - 1)
            p = xy[k] + left[k] * sign * RAIL_OFFSET_M
            yaw = hd[k] if sign > 0 else hd[k] + 180.0
            rails.append({"x": p[0], "y": p[1], "z": walk[k], "yaw": yaw % 360.0, "pitch": 0.0,
                          "length": bay, "mirror": 0})

    for side, sign in (("left", 1.0), ("right", -1.0)):
        for a, b in runs_of((kind == "path") & (drop[side] > RAIL_DROP_M), s):
            level_bays(a, b, sign)
    def flight_frame(f):
        yaw = math.radians(f["yaw"])
        return np.array([-math.sin(yaw), math.cos(yaw)]), np.array([math.cos(yaw), math.sin(yaw)])

    def flight_bays(f):
        span = f["treads"] * f["going"]
        n = max(1, int(round(span / RAIL_BAY_M[1] + 0.49)))
        return n, span / n

    def flight_drop(f, sgn):
        """How far the ground 2 m out on this side lies below the flight's nosing line (mean, m)."""
        lft, up = flight_frame(f)
        span = f["treads"] * f["going"]
        drops = []
        for u in (0.25, 0.5, 0.75):
            m = np.array([f["x"], f["y"]]) + up * span * u
            drops.append(f["z"] + f["rise"] * f["treads"] * u - ground_fn(*(m + lft * sgn * (CLEAR_HALF_M + SIDE_PROBE_M))))
        return float(np.mean(drops))

    def flight_clashes(f, sgn, other):
        """Bays on this side that would stand in another leg's clear width (a hairpin at a flight's foot)."""
        lft, up = flight_frame(f)
        n, bay = flight_bays(f)
        out = []
        for q in range(n):
            ends = [np.array([f["x"], f["y"]]) + up * (q + u) * bay + lft * sgn * RAIL_OFFSET_M for u in (0.0, 0.5, 1.0)]
            d = min(other.query(e)[0] for e in ends) if other is not None else np.inf
            out.append(d < CLEAR_HALF_M + RAIL_CLEAR_M)
        return out

    # One rail side per stair leg, so it never swaps sides partway down: the side clear of the other legs
    # (where the path doubles back at a flight's foot it takes the other side), then the bigger drop.
    for leg in sorted({f["leg"] for f in flights}):
        mine = [f for f in flights if f["leg"] == leg]
        others = xy[np.array([st[5] != leg for st in stations])]
        other = cKDTree(others) if len(others) else None
        score = {}
        for sgn in (1.0, -1.0):
            score[sgn] = (sum(sum(flight_clashes(f, sgn, other)) for f in mine), -sum(flight_drop(f, sgn) for f in mine))
        sign = min((1.0, -1.0), key=lambda s: score[s])
        for f in mine:
            lft, up = flight_frame(f)
            n, bay = flight_bays(f)
            blocked = flight_clashes(f, sign, other)
            f["railSide"] = "left" if sign > 0 else "right"
            f["sideDrop"] = max(flight_drop(f, 1.0), flight_drop(f, -1.0))
            for q in range(n):
                if blocked[q]:
                    continue
                base = np.array([f["x"], f["y"]]) + up * q * bay + lft * sign * RAIL_OFFSET_M
                # Posts stand on the nosing line: the rail's pivot rises with the flight.
                rails.append({"x": base[0], "y": base[1], "z": f["z"] + q * bay / f["going"] * f["rise"],
                              "yaw": f["yaw"] % 360.0, "pitch": f["pitch"], "length": bay,
                              "mirror": 0 if sign > 0 else 1})
    def level_rail(p, q, z, drop_dir):
        """A level bay from p to q (plan, m) whose +Y faces drop_dir."""
        d = q - p
        yaw = math.degrees(math.atan2(d[1], d[0]))
        if -math.sin(math.radians(yaw)) * drop_dir[0] + math.cos(math.radians(yaw)) * drop_dir[1] < 0:
            p, yaw = q, yaw + 180.0
        rails.append({"x": p[0], "y": p[1], "z": z, "yaw": yaw % 360.0, "pitch": 0.0,
                      "length": float(np.hypot(*d)), "mirror": 0})

    for lnd in landings:
        # Carry the flight's rail across the landing on the same side; on a corner landing only as far as the
        # turn, then across to the next leg's rail (no gap to slip through, nothing in its clear width).
        leg = [f for f in flights if f["leg"] == lnd["leg"]]
        sign = 1.0 if leg and leg[0]["railSide"] == "left" else -1.0
        yaw = math.radians(lnd["yaw"])
        up = np.array([math.cos(yaw), math.sin(yaw)])
        lft = np.array([-math.sin(yaw), math.cos(yaw)])
        base = np.array([lnd["x"], lnd["y"]])
        span = lnd["length"] if lnd.get("turn") is None else lnd["turn"]
        p0, p1 = base + lft * sign * RAIL_OFFSET_M, base + up * span + lft * sign * RAIL_OFFSET_M
        if span > 0.05:
            level_rail(p0, p1, lnd["z"], lft * sign)
        if lnd.get("turn") is not None:
            # The other leg of the corner: its flight whose foot is nearest the turn, on the same drop side.
            turn = base + up * span
            nxt = min((f for f in flights if f["leg"] != lnd["leg"]),
                      key=lambda f: math.hypot(f["x"] - turn[0], f["y"] - turn[1]))
            if nxt["railSide"] == (leg[0]["railSide"] if leg else None):
                ny = math.radians(nxt["yaw"])
                nlft = np.array([-math.sin(ny), math.cos(ny)])
                q = np.array([nxt["x"], nxt["y"]]) + nlft * sign * RAIL_OFFSET_M
                level_rail(p1, q, lnd["z"], lft * sign)
    return kerbs, rails, s


def estate_placements():
    """Every interactive placement in the generated Simulation tables: (kind, x m, y m)."""
    import glob
    import re
    out = []
    for f in sorted(glob.glob(os.path.join(REPO, "Source", "SurvivalGame", "Simulation", "*.inc"))):
        for m in re.finditer(r"\((\d+), ResourceKind::(\w+), (-?[\d.]+), (-?[\d.]+)\)", open(f).read()):
            out.append((m.group(2), float(m.group(3)) / 100.0, float(m.group(4)) / 100.0))
    return out


FINGERPOST_SIDE_M = 1.4
FINGERPOST_CLEAR_M = 1.6     # from any interactive placement (the forecourt's clear-out)


def fingerposts(stations, runs, flights):
    xy = np.array([(st[0], st[1]) for st in stations])
    walk = np.array([st[3] for st in stations])
    s = np.r_[0.0, np.cumsum(np.hypot(*np.diff(xy, axis=0).T))]
    hd = headings(xy)
    placed = cKDTree(np.array([(x, y) for _, x, y in estate_placements()]))
    posts = []

    def spot(at_s, side_m):
        k = min(np.searchsorted(s, at_s), len(s) - 1)
        yaw = math.radians(hd[k])
        p = xy[k] + np.array([-math.sin(yaw), math.cos(yaw)]) * side_m
        return {"x": p[0], "y": p[1], "z": walk[k], "yaw": hd[k] % 360.0, "s": float(s[k])}

    def post(candidates):
        for at_s, side_m in candidates:
            c = spot(at_s, side_m)
            if placed.query((c["x"], c["y"]))[0] >= FINGERPOST_CLEAR_M:
                posts.append(c)
                return
        raise SystemExit(f"cove route: no clear spot for a fingerpost near {candidates[0][0]:.0f} m")

    # Past the thin bramble outside the front door, where the path leaves the forecourt.
    post([(at, side) for at in (6.0, 8.0, 10.0, 12.0, 14.0) for side in (-FINGERPOST_SIDE_M, FINGERPOST_SIDE_M)])
    first = next(i for i, r in enumerate(runs) if r["kind"] == "stairs")
    head = min(k for k, st in enumerate(stations) if st[5] == first)
    # At the head of the cliff steps, on the side away from their rail. The route runs down the flight, so
    # the flight's +Y (its "left" railSide) is the route's -Y.
    rail_plus_y = next(f for f in flights if f["leg"] == first)["railSide"] == "left"
    side = FINGERPOST_SIDE_M if rail_plus_y else -FINGERPOST_SIDE_M
    post([(max(0.0, s[head] - at), side) for at in (1.0, 2.0, 3.0)])
    # Its arm points down the steps (the flights' +X runs up them).
    down = (next(f for f in flights if f["leg"] == first)["yaw"] + (180.0 if runs[first]["descends"] else 0.0)) % 360.0
    posts[-1]["yaw"] = down
    return posts


# ---- heightfield grade ------------------------------------------------------------------------------
def corridor_change(stations, z, layout):
    xy = np.array([(st[0], st[1]) for st in stations])
    bed = np.array([st[2] for st in stations])
    stair = np.array([st[4] == "stairs" for st in stations])
    flat = np.where(stair, STAIR_BED_HALF_M, PATH_BED_HALF_M)
    reach = STAIR_BED_HALF_M + VERGE_M
    x0, x1 = int(math.floor(xy[:, 0].min() - reach)), int(math.ceil(xy[:, 0].max() + reach))
    y0, y1 = int(math.floor(xy[:, 1].min() - reach)), int(math.ceil(xy[:, 1].max() + reach))
    gx, gy = np.meshgrid(np.arange(x0, x1 + 1), np.arange(y0, y1 + 1))
    pts = np.c_[gx.ravel(), gy.ravel()].astype(np.float64)
    d, i = cKDTree(xy).query(pts, distance_upper_bound=reach)
    keep = np.isfinite(d)
    pts, d, i = pts[keep], d[keep], i[keep]
    t = np.clip((d - flat[i]) / VERGE_M, 0.0, 1.0)
    w = 1.0 - t * t * (3 - 2 * t)
    # Never into the river's channel.
    riv = np.asarray(layout["river"][:layout.get("riverEnd", len(layout["river"]))], np.float64)
    half = np.asarray(layout["riverHalfWidth"][:len(riv)], np.float64)
    dr, j = cKDTree(riv).query(pts)
    w *= np.clip((dr - half[j] - RIVER_MARGIN_M) / 1.0, 0.0, 1.0)
    rows = (pts[:, 1] + H).astype(int)
    cols = (pts[:, 0] + H).astype(int)
    change = (bed[i] - z[rows, cols]) * w
    return rows, cols, change


# ---- scenery ----------------------------------------------------------------------------------------
RECORD = np.dtype([("k", "u1"), ("pad", "V3"), ("x", "<f4"), ("y", "<f4"), ("yaw", "<f4"), ("s", "<f4")])


def clear_scenery_records(rec, route):
    """Keep-mask for scatter records (cm) off the route's centreline (layout["coveRoute"]["centreline"])."""
    line = np.asarray(route["centreline"], np.float64)[:, :2]
    dense, _ = resample(line, 0.5)
    d, _ = cKDTree(dense).query(np.c_[rec["x"] / 100.0, rec["y"] / 100.0])
    return d >= CLEAR_SCENERY_M


def apply_to_scenery_file(path, route):
    raw = open(path, "rb").read()
    count = int(np.frombuffer(raw[4:8], np.uint32)[0])
    rec = np.frombuffer(raw[8:8 + count * 20], RECORD)
    keep = clear_scenery_records(rec, route)
    if (~keep).any():
        kept = rec[keep]
        with open(path, "wb") as fh:
            fh.write(raw[:4] + np.uint32(len(kept)).tobytes() + kept.tobytes() + raw[8 + count * 20:])
    return int((~keep).sum())


# ---- outputs ----------------------------------------------------------------------------------------
FOOTPATH_STEP_M = 4.0        # the map's dashed footpath (layout "footpaths")
DATA_STATION_M = 0.5          # centreline spacing kept in cove_route.json


def write_data(route):
    """cove_route.json: one record per line, so a regenerated route diffs by the record."""
    out = dict(route)
    step = max(1, int(round(DATA_STATION_M / STATION_M)))
    c = route["centreline"]
    if route.get("stationStep") is None:
        out["centreline"] = c[::step] + ([c[-1]] if (len(c) - 1) % step else [])
        out["stationStep"] = DATA_STATION_M
    lines = ["{"]
    keys = list(out.keys())
    for n, k in enumerate(keys):
        v = out[k]
        tail = "," if n < len(keys) - 1 else ""
        if isinstance(v, list) and v and isinstance(v[0], (list, dict)):
            lines.append(f" {json.dumps(k)}: [")
            lines += [f"  {json.dumps(item, separators=(', ', ': '))}{',' if q < len(v) - 1 else ''}" for q, item in enumerate(v)]
            lines.append(f" ]{tail}")
        else:
            lines.append(f" {json.dumps(k)}: {json.dumps(v)}{tail}")
    lines.append("}")
    with open(DATA, "w", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")


def write_inc(route):
    L = []
    L.append("// Generated by Scripts/Terrain/cove_route.py from estate_layout.json; do not edit by hand.")
    L.append(f"// The cove route from the manor's south front door to the cove: {route['length']:.1f} m, "
             f"{len(route['flights'])} flights ({sum(f['treads'] for f in route['flights'])} steps).")
    L.append("// Unreal cm, +X north, +Y east; yaw in degrees from +X toward +Y. Props' kit pivots (add-cove-route-kit).")
    stations = route["centreline"]
    step = max(1, int(round(1.0 / route["stationStep"])))
    L.append("// station(x, y, walk z, bed z, metres along, on steps): the centreline every "
             f"{route['stationStep'] * step:g} m; walk z is what she stands on, bed z the cut ground")
    for k in range(0, len(stations), step):
        x, y, walk, bed, s, stair = stations[k]
        L.append(f"station({x * 100:.1f}, {y * 100:.1f}, {walk * 100:.1f}, {bed * 100:.1f}, {s:.2f}, {int(stair)});")
    if (len(stations) - 1) % step:
        x, y, walk, bed, s, stair = stations[-1]
        L.append(f"station({x * 100:.1f}, {y * 100:.1f}, {walk * 100:.1f}, {bed * 100:.1f}, {s:.2f}, {int(stair)});")
    L.append("// flight(x, y, z, yaw, rise, going, treads, rail pitch deg): tread i's pivot (top, front nosing centre)")
    L.append("// is (x, y, z) + i * (going along yaw, rise); +X runs up the flight")
    for f in route["flights"]:
        L.append(f"flight({f['x'] * 100:.1f}, {f['y'] * 100:.1f}, {f['z'] * 100:.1f}, {f['yaw']:.2f}, "
                 f"{f['rise'] * 100:.2f}, {f['going'] * 100:.2f}, {f['treads']}, {f['pitch']:.0f});")
    L.append("// landing(x, y, z, yaw, length): pivot at its front (downhill) edge's centre, top; +X up the steps")
    for lnd in route["landings"]:
        L.append(f"landing({lnd['x'] * 100:.1f}, {lnd['y'] * 100:.1f}, {lnd['z'] * 100:.1f}, {lnd['yaw']:.2f}, "
                 f"{lnd['length'] * 100:.1f});")
    L.append("// kerb(x, y, z, yaw): a 1 m piece; pivot on its path-side top edge, +X along the path, +Y to the drop")
    for k in route["kerbs"]:
        L.append(f"kerb({k['x'] * 100:.1f}, {k['y'] * 100:.1f}, {k['z'] * 100:.1f}, {k['yaw']:.2f});")
    L.append("// rail(x, y, z, yaw, pitch deg, length cm along plan, mirrored): a bay; pivot at its downhill post's")
    L.append("// foot on the path (or the nosing line), +X along the path (uphill when raked), +Y to the drop;")
    L.append("// mirrored bays have the drop on -Y (scale Y by -1: a raked bay can't be turned round)")
    for r in route["rails"]:
        L.append(f"rail({r['x'] * 100:.1f}, {r['y'] * 100:.1f}, {r['z'] * 100:.1f}, {r['yaw']:.2f}, "
                 f"{r['pitch']:.0f}, {r['length'] * 100:.1f}, {r['mirror']});")
    L.append("// fingerpost(x, y, z, yaw): pivot at the post's foot; the arm points along yaw (\"To the Cove\")")
    for p in route["fingerposts"]:
        L.append(f"fingerpost({p['x'] * 100:.1f}, {p['y'] * 100:.1f}, {p['z'] * 100:.1f}, {p['yaw']:.2f});")
    with open(INC, "w", newline="\n") as fh:
        fh.write("\n".join(L) + "\n")


def leg_summary(runs, zs, flights):
    legs = []
    for i, r in enumerate(runs):
        a = r["points"][0]
        leg = {"kind": r["kind"], "x": round(float(a[0]), 2), "y": round(float(a[1]), 2),
               "length": round(float(r["length"]), 2), "fromZ": round(float(zs[i]), 3), "toZ": round(float(zs[i + 1]), 3)}
        if r["kind"] == "stairs":
            fl = [f for f in flights if f["leg"] == i]
            leg.update(flights=len(fl), risers=sum(f["treads"] for f in fl), rise=round(r["rise"], 4),
                       going=round(r["going"], 4), pitch=r["pitch"], landing=round(r["landing"], 3),
                       headLanding=round(r["headLanding"], 3))
        legs.append(leg)
    return legs


def report(route):
    os.makedirs(SAVED, exist_ok=True)
    rows = ["| # | Way | From (x, y) m | Length m | Top z m | Foot z m | Fall m | Grade | Steps |",
            "| --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for i, g in enumerate(route["legs"]):
        za, zb = g["fromZ"], g["toZ"]
        steps = ""
        if g["kind"] == "stairs":
            steps = (f"{g['flights']} flight{'s' if g['flights'] > 1 else ''}, {g['risers']} risers at "
                     f"{g['rise'] * 100:.1f} / {g['going'] * 100:.1f} cm, {g['pitch']:.0f} deg rail")
            if g["flights"] > 1:
                steps += f", landings {g['landing']:.2f} m"
            if g["headLanding"] > 0.05:
                steps += f", head landing {g['headLanding']:.2f} m"
        rows.append(f"| {i + 1} | {g['kind']} | ({g['x']:.0f}, {g['y']:.0f}) | {g['length']:.1f} | {max(za, zb):.2f} | "
                    f"{min(za, zb):.2f} | {abs(za - zb):.2f} | 1 in {g['length'] / max(abs(za - zb), 1e-6):.1f} | {steps} |")
    table = "\n".join(rows)
    with open(os.path.join(SAVED, "cove_route_table.md"), "w") as fh:
        fh.write(table + "\n")
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        return table
    c = np.asarray(route["centreline"])
    fig, ax = plt.subplots(1, 2, figsize=(16, 9), gridspec_kw={"width_ratios": [1, 1.4]})
    _, z = load_r16()
    X0, X1 = int(c[:, 0].min() - 25), int(c[:, 0].max() + 25)
    Y0, Y1 = int(c[:, 1].min() - 25), int(c[:, 1].max() + 25)
    sub = z[Y0 + H:Y1 + H + 1, X0 + H:X1 + H + 1].T
    cs = ax[0].contour(np.arange(Y0, Y1 + 1), np.arange(X0, X1 + 1), sub, levels=np.arange(0, 100, 2.5),
                       colors="k", linewidths=0.3)
    ax[0].clabel(cs, fontsize=6)
    stair = c[:, 5] > 0
    ax[0].plot(c[:, 1], c[:, 0], color="#8a6", lw=2)
    ax[0].plot(np.where(stair, c[:, 1], np.nan), np.where(stair, c[:, 0], np.nan), color="#555", lw=3)
    for k in route["kerbs"]:
        ax[0].plot(k["y"], k["x"], ".", color="#a86", ms=2)
    for r in route["rails"]:
        ax[0].plot(r["y"], r["x"], "|", color="#630", ms=4)
    for p in route["fingerposts"]:
        ax[0].plot(p["y"], p["x"], "r^", ms=7)
    ax[0].set_aspect(1)
    ax[0].set_title("Cove route: path (green), steps (grey), kerbs, rails, fingerposts (red)")
    ax[1].plot(c[:, 4], c[:, 2], color="#555", lw=1)
    ax[1].fill_between(c[:, 4], c[:, 3], c[:, 2], where=stair, color="#999", alpha=0.6)
    ax[1].set_xlabel("metres along the route")
    ax[1].set_ylabel("z (m)")
    ax[1].grid(alpha=0.3)
    ax[1].set_title("Profile (walk height; steps shaded)")
    plt.tight_layout()
    plt.savefig(os.path.join(SAVED, "cove_route.png"), dpi=80)
    return table


def search():
    """Grade-aware least-cost route (1 m cells, 16 headings), printed as candidate CONTROL points."""
    import heapq
    import glob
    import re
    _, z = load_r16()
    layout = json.load(open(LAYOUT))
    X0, X1, Y0, Y1 = -600, -240, -720, -470
    sub = gaussian_filter(z[Y0 + H:Y1 + H + 1, X0 + H:X1 + H + 1], 1.5)
    ny, nx = sub.shape
    gy, gx = np.gradient(sub)
    yy, xx = np.mgrid[Y0:Y1 + 1, X0:X1 + 1]
    pts = np.c_[xx.ravel(), yy.ravel()]
    riv = np.asarray(layout["river"], np.float64)
    near_river = cKDTree(riv).query(pts)[0].reshape(sub.shape) < 7.0
    solid = []
    for f in glob.glob(os.path.join(REPO, "Source", "SurvivalGame", "Simulation", "*.inc")):
        for m in re.finditer(r"\((\d+), ResourceKind::(ForestTree|Boulder|StumpLarge)\w*, (-?[\d.]+), (-?[\d.]+)\)",
                             open(f).read()):
            solid.append((float(m.group(3)) / 100, float(m.group(4)) / 100))
    near_solid = cKDTree(np.array(solid)).query(pts)[0].reshape(sub.shape) < 2.6
    ok = (sub > 1.2) & ~near_river & ~near_solid
    moves = [(dx, dy) for dx in range(-2, 3) for dy in range(-2, 3) if (dx, dy) != (0, 0) and math.gcd(abs(dx), abs(dy)) == 1]
    ang = [math.atan2(dy, dx) for dx, dy in moves]
    start = (int(CONTROL[0][0]) - X0, int(CONTROL[0][1]) - Y0)
    goal = (int(CONTROL[-1][0]) - X0, int(CONTROL[-1][1]) - Y0)
    dist, prev, pq = {}, {}, []
    for h in range(len(moves)):
        dist[(*start, h)] = 0.0
        heapq.heappush(pq, (0.0, *start, h))
    best = None
    while pq:
        d, x, y, h = heapq.heappop(pq)
        if d > dist.get((x, y, h), 1e18):
            continue
        if (x, y) == goal:
            best = (x, y, h)
            break
        for h2, (dx, dy) in enumerate(moves):
            x2, y2 = x + dx, y + dy
            if not (0 <= x2 < nx and 0 <= y2 < ny) or not ok[y2, x2]:
                continue
            ln = math.hypot(dx, dy)
            g = abs(sub[y2, x2] - sub[y, x]) / ln
            cross = abs(-dy / ln * gx[y, x] + dx / ln * gy[y, x])
            c = ln * (1 + 0.8 * max(0.0, cross - 0.45))
            if g > MAX_PATH_GRADE:
                if g > 0.5:
                    continue
                c *= 4.0 * (1 + g)
            turn = abs((ang[h2] - ang[h] + math.pi) % (2 * math.pi) - math.pi)
            c += 18.0 * turn / (math.pi / 2)
            nd = d + c
            if nd < dist.get((x2, y2, h2), 1e18):
                dist[(x2, y2, h2)] = nd
                prev[(x2, y2, h2)] = (x, y, h)
                heapq.heappush(pq, (nd, x2, y2, h2))
    path = [best]
    while path[-1] in prev:
        path.append(prev[path[-1]])
    p = np.array([(q[0] + X0, q[1] + Y0, sub[q[1], q[0]]) for q in path[::-1]], np.float64)

    def rdp(P, eps):
        if len(P) < 3:
            return [0, len(P) - 1]
        a, b = P[0], P[-1]
        d = b - a
        dist_ = np.abs(d[0] * (P[:, 1] - a[1]) - d[1] * (P[:, 0] - a[0])) / np.hypot(*d)
        i = int(np.argmax(dist_))
        if dist_[i] > eps:
            return rdp(P[:i + 1], eps)[:-1] + [x + i for x in rdp(P[i:], eps)]
        return [0, len(P) - 1]

    V = p[rdp(p[:, :2], 1.5)]
    for a, b in zip(V[:-1], V[1:]):
        g = abs(b[2] - a[2]) / np.hypot(*(b[:2] - a[:2]))
        print(f"    ({a[0]:.1f}, {a[1]:.1f}, \"{'stairs' if g > MAX_PATH_GRADE + 0.02 else 'path'}\"),   # z {a[2]:.1f}")
    print(f"    ({V[-1][0]:.1f}, {V[-1][1]:.1f}, None),")


def main():
    if "--search" in sys.argv:
        search()
        return
    dry = "--dry" in sys.argv          # design and report only; nothing in the repo is written
    layout = json.load(open(LAYOUT))
    raw_r16, z = load_r16()
    npy_path = os.path.join(WORK, "game_reshaped_4033.npy")
    prior = layout.get("coveRoute")
    if prior and prior.get("graded"):
        # Graded already: the design stands (it was made over the ground before the cut). Re-emit its data.
        if [list(c[:2]) for c in CONTROL] != [list(c) for c in prior["control"]]:
            raise SystemExit("cove route: CONTROL differs from the graded route in estate_layout.json; restore the "
                             "heightfield from before the cove route and remove \"coveRoute\" to re-grade")
        route = json.load(open(DATA))
        print("cove route: heightfield already graded; route data re-emitted")
    else:
        runs = pieces()
        # The ground the route is designed over, lightly smoothed (the bed evens out its lumps).
        cx = [c[0] for c in CONTROL]
        cy = [c[1] for c in CONTROL]
        pad = 16
        x0, y0 = int(math.floor(min(cx))) - pad, int(math.floor(min(cy))) - pad
        x1, y1 = int(math.ceil(max(cx))) + pad, int(math.ceil(max(cy))) + pad
        smooth = gaussian_filter(z[y0 + H:y1 + H + 1, x0 + H:x1 + H + 1], SMOOTH_GROUND_M)

        def ground_fn(x, y):
            c = np.asarray(x, np.float64) - x0
            r = np.asarray(y, np.float64) - y0
            c0 = np.clip(np.floor(c).astype(int), 0, smooth.shape[1] - 2)
            r0 = np.clip(np.floor(r).astype(int), 0, smooth.shape[0] - 2)
            fc, fr = c - c0, r - r0
            return ((smooth[r0, c0] * (1 - fc) + smooth[r0, c0 + 1] * fc) * (1 - fr)
                    + (smooth[r0 + 1, c0] * (1 - fc) + smooth[r0 + 1, c0 + 1] * fc) * fr)

        zs, _, stations, flights, landings = design(runs, ground_fn)
        kerbs, rails, s = edges(stations, flights, landings, runs, ground_fn)
        posts = fingerposts(stations, runs, flights)
        centreline = [[round(float(st[0]), 3), round(float(st[1]), 3), round(float(st[3]), 3), round(float(st[2]), 3),
                       round(float(si), 3), int(st[4] == "stairs")] for st, si in zip(stations, s)]
        rows_, cols_, change_ = corridor_change(stations, z, layout)
        rnd = lambda items: [{k: (round(float(v), 4) if isinstance(v, (float, np.floating)) else v) for k, v in d.items()}
                             for d in items]
        route = {"graded": True, "control": [list(c[:2]) for c in CONTROL], "kinds": [c[2] for c in CONTROL[:-1]],
                 "length": round(float(s[-1]), 2), "clearWidth": 2 * CLEAR_HALF_M, "maxPathGrade": MAX_PATH_GRADE,
                 "nodes": [round(float(v), 3) for v in zs], "legs": leg_summary(runs, zs, flights),
                 "centreline": centreline, "flights": rnd(flights), "landings": rnd(landings), "kerbs": rnd(kerbs),
                 "rails": rnd(rails), "fingerposts": rnd(posts)}
        rows, cols, change = rows_, cols_, change_
        z[rows, cols] += change
        u16 = np.clip(np.round(32768 + z * 128.0), 0, 65535).astype(np.uint16)
        moved = np.argwhere(u16 != raw_r16)
        if not dry:
            u16.astype("<u2").tofile(R16)
            Image.fromarray(u16).save(PNG)
        z = (u16.astype(np.float64) - 32768.0) / 128.0
        (r0, c0), (r1, c1) = moved.min(0), moved.max(0)
        print(f"cove route: {len(moved)} heightfield vertices changed; max cut {change.min():+.2f} m, max fill "
              f"{change.max():+.2f} m; ApplyEstateHeightfield rows {r0}-{r1}, cols {c0}-{c1}")
    # Bring the work npy ([row = H - x, col = H + y]) into step where it differs from the graded heightfield
    # round the route (a work folder that missed the cut).
    if dry:
        print(report(route))
        return
    if os.path.exists(npy_path):
        zn = np.load(npy_path)
        c = np.asarray(route["centreline"], np.float64)
        xs = np.arange(int(c[:, 0].min()) - 8, int(c[:, 0].max()) + 9)
        ys = np.arange(int(c[:, 1].min()) - 8, int(c[:, 1].max()) + 9)
        X, Y = np.meshgrid(xs, ys, indexing="ij")
        near = cKDTree(c[:, :2]).query(np.c_[X.ravel(), Y.ravel()])[0].reshape(X.shape) <= STAIR_BED_HALF_M + VERGE_M + 1
        graded = z[Y + H, X + H]
        stale = near & (np.abs(zn[H - X, H + Y] - graded) > 0.01)
        if stale.any():
            zn[H - X[stale], H + Y[stale]] = graded[stale].astype(zn.dtype)
            np.save(npy_path, zn)
            print(f"cove route: work npy brought into step at {int(stale.sum())} cells ({npy_path})")

    write_data(route)
    route = json.load(open(DATA))           # everything below reads what's committed, first run or re-run
    layout["coveRoute"] = {k: route[k] for k in ("graded", "control", "kinds", "length", "clearWidth", "maxPathGrade",
                                                 "nodes", "legs")}
    layout["coveRoute"]["data"] = "Scripts/Terrain/cove_route.json"
    paths = [p for p in layout.get("footpaths", []) if p.get("name") != "Cove"]
    dense, _ = resample(np.array([c[:2] for c in route["centreline"]]), FOOTPATH_STEP_M)
    paths.append({"name": "Cove", "points": np.round(dense, 2).tolist()})
    layout["footpaths"] = paths
    with open(LAYOUT, "w") as fh:
        json.dump(layout, fh, indent=1)
    cleared = apply_to_scenery_file(SCENERY, route)
    write_inc(route)
    print(report(route))
    fl = route["flights"]
    print(f"cove route: {route['length']:.0f} m, {len(fl)} flights, {sum(f['treads'] for f in fl)} steps, "
          f"{len(route['landings'])} landings, {len(route['kerbs'])} kerbs, {len(route['rails'])} rail bays, "
          f"{len(route['fingerposts'])} fingerposts; {cleared} scenery records cleared")


if __name__ == "__main__":
    main()
