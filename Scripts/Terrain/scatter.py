"""Scatter the Estate map's decorative scenery and bake the world lane's interactive placements.

Outputs (both committed):
  Content/SurvivalGame/Estate/Runtime/EstateScenery.bin   decorative instances read by AHomesteadWorld
  Source/SurvivalGame/Simulation/HomesteadEstateWorldPlacements.inc   world-lane EstatePlacement rows (500100+)

Deterministic (fixed seed). Kind bytes must match EstateSceneryKinds in HomesteadWorld.cpp.
"""
import json, os, struct
import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter
from scipy.spatial import cKDTree
from skimage.measure import points_in_poly

import mvp_woodland
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
WORK = os.environ.get("HOMESTEAD_TERRAIN_WORK", r"E:\TerrainSource\work")
H = 2016
BROADLEAF, FIR, HAZEL, BRACKEN, YARROW, LEDGE, BOULDER, ERRATIC, DOME, FERN, GRASS_TALL, GRASS_MID, SHRUB, OAK, BEECH, SYCAMORE, HAWTHORN, HOLLY, HAZEL_COPPICE = range(19)
# LEDGE (5) was loose granite cobbles; it's now a block of granite sunk into the slope, so no scenery
# looks like the hand stones she can pick up.
# The Hawthorn mesh streams toward +X; Cornwall's prevailing wind is south-westerly, so thorns lean
# north-east (+X north, +Y east): yaw 45 +- WIND_SPREAD.
WIND_YAW, WIND_SPREAD = 45.0, 25.0
# 19+ are the MVP woodland's kinds (mvp_woodland.py).

def weights():
    out = {}
    for n in ["Pasture", "WoodlandFloor", "Moorland", "DuneSand", "Beach", "CliffRock", "DirtRoad"]:
        a = np.asarray(Image.open(os.path.join(WORK, "weights", f"{n}.png"))).astype(np.float32) / 255
        out[n] = a.T[::-1, :]            # back to [row = north..south, col = west..east]
    return out

def sample(grid, x, y):
    r = np.clip(np.round(H - x).astype(int), 0, grid.shape[0] - 1)
    c = np.clip(np.round(H + y).astype(int), 0, grid.shape[1] - 1)
    return grid[r, c]

def densify(p, step=2.0):
    p = np.asarray(p, float)
    s = np.r_[0, np.cumsum(np.linalg.norm(np.diff(p, axis=0), axis=1))]
    t = np.arange(0, s[-1], step)
    return np.c_[np.interp(t, s, p[:, 0]), np.interp(t, s, p[:, 1])]

def main():
    rng = np.random.default_rng(1851)
    z = np.load(os.path.join(WORK, "game_reshaped_4033.npy"))
    W = weights()
    gy, gx = np.gradient(gaussian_filter(z, 1.0))
    slope = np.degrees(np.arctan(np.hypot(gx, gy)))
    L = json.load(open(os.path.join(HERE, "estate_layout.json")))
    mvp = mvp_woodland.Region(os.path.join(HERE, "mvp_woodland.json"))
    lm = {k: np.array(v[:2]) for k, v in L["landmarks"].items()}
    road = cKDTree(densify(L["road"]))
    river = cKDTree(densify(L["river"]))
    boundary = np.array(L["polygons"]["EstateBoundary"])
    manor = np.array(L["polygons"]["ManorFootprint"])
    town = lm["TownSquare"]

    def candidates(per_m2, area=(-2000, 2000, -2000, 2000)):
        x0, x1, y0, y1 = area
        n = rng.poisson(per_m2 * (x1 - x0) * (y1 - y0))
        return np.c_[rng.uniform(x0, x1, n), rng.uniform(y0, y1, n)]

    def common_mask(p, road_clear=6.0, river_clear=5.0):
        ok = sample(z, p[:, 0], p[:, 1]) > 1.5
        ok &= road.query(p)[0] > road_clear
        ok &= river.query(p)[0] > river_clear
        ok &= np.linalg.norm(p - town, axis=1) > 180
        ok &= np.linalg.norm(p - lm["StandingRoomOrigin"], axis=1) > 28
        ok &= np.linalg.norm(p - lm["MineEntrance"], axis=1) > 20
        ok &= np.linalg.norm(p - lm["MillSite"], axis=1) > 14
        return ok

    def keep_common(p, road_clear=6.0, river_clear=5.0):
        return p[common_mask(p, road_clear, river_clear)]

    def by_weight(p, layer, lo=0.35, jitter=True):
        w = sample(W[layer], p[:, 0], p[:, 1])
        if jitter:
            return p[rng.random(len(p)) < np.clip((w - lo) / (1 - lo), 0, 1)]
        return p[w > lo]

    # ---- interactive placements (world lane, 500100+) -------------------------------------
    rows = []
    next_id = [500100]
    inside = lambda p: points_in_poly(p, boundary) & ~points_in_poly(p, manor)

    def place(kind, pts, limit, min_gap, taken):
        chosen = []
        for pt in pts:
            if len(chosen) >= limit: break
            if taken and min(np.linalg.norm(np.array(taken) - pt, axis=1)) < min_gap: continue
            chosen.append(pt); taken.append(pt)
        for pt in chosen:
            rows.append((next_id[0], kind, pt))
            next_id[0] += 1
        return chosen

    taken = [lm["StandingRoomOrigin"].tolist()]
    wood = keep_common(candidates(1 / 60.0, (-800, 200, -1200, 0)), 7, 7)
    wood = wood[inside(wood)]
    wood_trees = by_weight(wood, "WoodlandFloor", 0.4, False)
    interactive_trees = place("ForestTree", wood_trees, 180, 7.0, taken)
    near = keep_common(candidates(1 / 25.0, (-450, 0, -950, -300)), 3, 4)
    near = near[inside(near)]
    near = near[np.argsort(np.linalg.norm(near - lm["StandingRoomOrigin"], axis=1))]
    in_wood = sample(W["WoodlandFloor"], near[:, 0], near[:, 1]) > 0.3
    place("Branches", np.r_[near[in_wood][:400], near[~in_wood][:200]], 45, 6, taken)
    place("Stones", near[slope[np.clip(np.round(H - near[:, 0]).astype(int), 0, 4032), np.clip(np.round(H + near[:, 1]).astype(int), 0, 4032)] > 8], 30, 8, taken)
    place("Flowers", near[~in_wood], 35, 7, taken)
    place("BerryBush", near[in_wood][40:], 22, 9, taken)
    place("Roots", near[in_wood][80:], 22, 8, taken)
    # Reeds are retired by add-overgrown-estate-clearing; the river banks get no interactive reeds.
    tree_tree = cKDTree(np.array(interactive_trees)) if interactive_trees else None

    # ---- the MVP woodland (add-mvp-woodland-biome): its own rng, so the rest stays as it was ----
    def mvp_ground(p):
        sv = slope[np.clip(np.round(H - p[:, 0]).astype(int), 0, 4032), np.clip(np.round(H + p[:, 1]).astype(int), 0, 4032)]
        return sv < 35
    mvp_rows, mvp_recs = mvp_woodland.build(mvp, lambda p: common_mask(p, 4.0, 6.0), mvp_ground,
                                            np.array([pt for _, _, pt in rows]))
    rows += mvp_rows

    # ---- decorative scenery -------------------------------------------------------------
    recs = []
    def emit(kind, p, smin, smax):
        # Inside the MVP woodland the estate's own scenery gives way (the draws still happen, so
        # everything outside the region is unchanged).
        emit_yawed(kind, p, smin, smax, 0.0, 360.0)
    def emit_windswept(kind, p, smin, smax):
        # Same two draws per point as emit, so swapping a share to a windswept kind keeps the sequence.
        emit_yawed(kind, p, smin, smax, WIND_YAW - WIND_SPREAD, WIND_YAW + WIND_SPREAD)
    def emit_yawed(kind, p, smin, smax, yaw_lo, yaw_hi):
        blocked = mvp.cornish_blocked(p) if len(p) else []
        for i, (x, y) in enumerate(p):
            yaw, s = rng.uniform(yaw_lo, yaw_hi), rng.uniform(smin, smax)
            if not blocked[i]:
                recs.append((kind, x, y, yaw, s))
    def clear_of_interactive(p, gap):
        pts = np.array(taken)
        return p[cKDTree(pts).query(p)[0] > gap] if len(p) else p
    # Where the real hand stones are: the Stones forage, the manor clear-out (clearout.py) and the
    # grounds round the house. Rock scenery keeps its distance, so the two aren't confused.
    stone_pts = [pt for _, kind, pt in rows if kind == "Stones"]
    clearout_inc = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation", "HomesteadEstateClearoutPlacements.inc")
    if os.path.exists(clearout_inc):
        for line in open(clearout_inc):
            if line.startswith("clearout("):
                parts = line[line.index("(") + 1:line.rindex(")")].split(",")
                stone_pts.append((float(parts[2]) / 100.0, float(parts[3]) / 100.0))
    stone_tree = cKDTree(np.array(stone_pts)) if stone_pts else None
    def clear_of_stones(p, gap):
        if not len(p):
            return np.zeros(0, bool)
        ok = np.linalg.norm(p - lm["StandingRoomOrigin"], axis=1) > 60.0
        ok &= road.query(p)[0] > 15.0
        # The derelict farm (X -222..-162 m, Y -705..-645 m) and a 15 m margin.
        ok &= (np.abs(p[:, 0] + 192.0) > 45.0) | (np.abs(p[:, 1] + 675.0) > 45.0)
        if stone_tree is not None:
            ok &= stone_tree.query(p)[0] > gap
        return ok

    all_wood = keep_common(candidates(1 / 45.0), 7, 6)
    wt = by_weight(all_wood, "WoodlandFloor", 0.3)
    wt = clear_of_interactive(wt, 6.5)
    firs = rng.random(len(wt)) < 0.12
    emit(BROADLEAF, wt[~firs], 0.85, 1.25)
    emit(FIR, wt[firs], 0.8, 1.15)
    under = keep_common(candidates(1 / 22.0, (-1000, 700, -1500, 700)), 3, 3)
    under = clear_of_interactive(by_weight(under, "WoodlandFloor", 0.3), 2.0)
    pick = rng.random(len(under))
    emit(HAZEL, under[pick < 0.18], 0.8, 1.15)
    emit(BRACKEN, under[(pick >= 0.18) & (pick < 0.55)], 0.75, 1.25)
    emit(FERN, under[(pick >= 0.55) & (pick < 0.85)], 0.8, 1.3)
    emit(SHRUB, under[pick >= 0.85], 0.8, 1.2)
    # Wild woods: after decades of neglect the estate and the country round it have gone back to
    # oak-and-hazel woodland, leaving the manor's grounds, the drive, the old farm and the fields
    # open. A two-scale noise field picks the woods; its fringe is a belt of scrub.
    step = 4.0
    axis = np.arange(-2000.0, 2000.0, step)
    gx_, gy_ = np.meshgrid(axis, axis, indexing="ij")
    field_noise = gaussian_filter(rng.normal(0, 1, gx_.shape), 60.0 / step)
    field_noise /= field_noise.std()
    fine_noise = gaussian_filter(rng.normal(0, 1, gx_.shape), 18.0 / step)
    fine_noise /= fine_noise.std()
    score = 0.8 * field_noise + 0.35 * fine_noise
    def kept_open(p, road_clear):
        # The grounds round the house stay open (the lawns, now rough) with a scrubby edge.
        s = np.clip((90.0 - np.linalg.norm(p - lm["StandingRoomOrigin"], axis=1)) / 25.0, 0, 3)
        # The derelict farm (manor lane: X -222..-162 m, Y -705..-645 m) plus a 10 m margin.
        fx = np.maximum(np.abs(p[:, 0] + 192.0) - 40.0, 0)
        fy = np.maximum(np.abs(p[:, 1] + 675.0) - 40.0, 0)
        s += np.clip((6.0 - np.hypot(fx, fy)) / 3.0, 0, 3)
        # The drive keeps a verge each side, and the river its banks.
        if road_clear > 0:
            s += np.clip((road_clear - road.query(p)[0]) / 3.0, 0, 3)
        s += np.clip((9.0 - river.query(p)[0]) / 3.0, 0, 3)
        s += np.clip((230.0 - np.linalg.norm(p - town, axis=1)) / 40.0, 0, 3)
        return s
    def wood_score(p):
        i = np.clip(((p[:, 0] + 2000.0) / step).astype(int), 0, len(axis) - 1)
        j = np.clip(((p[:, 1] + 2000.0) / step).astype(int), 0, len(axis) - 1)
        # Denser on the estate, where nobody has cut anything for twenty years, and closing in on the
        # grounds: the woods stand round the house's rough lawns about 90-200 m out.
        d_house = np.linalg.norm(p - lm["StandingRoomOrigin"], axis=1)
        return (score[i, j] + np.where(points_in_poly(p, boundary), 0.45, 0.0)
                + 0.7 * np.exp(-((d_house - 140.0) / 55.0) ** 2) - kept_open(p, 11.0))
    def woodable(p):
        ok = sample(W["Pasture"], p[:, 0], p[:, 1]) + sample(W["WoodlandFloor"], p[:, 0], p[:, 1]) > 0.45
        sv = slope[np.clip(np.round(H - p[:, 0]).astype(int), 0, 4032), np.clip(np.round(H + p[:, 1]).astype(int), 0, 4032)]
        return p[ok & (sv < 32)]
    WOOD_AT = 0.55
    ww = woodable(keep_common(candidates(1 / 32.0), 9, 7))
    ww = ww[wood_score(ww) > WOOD_AT]
    # Beyond the estate the woods thin with distance: a stand every so often towards the horizon.
    out_d = np.linalg.norm(ww - boundary.mean(axis=0), axis=1)
    ww = ww[points_in_poly(ww, boundary) | (rng.random(len(ww)) < np.clip(1.3 - out_d / 1100.0, 0.18, 1.0))]
    ww = clear_of_interactive(ww, 6.5)
    # Mature oak, beech and sycamore (14-17 m crowns) make the canopy; young broadleaf and the odd
    # fir fill between them.
    pick = rng.random(len(ww))
    emit(OAK, ww[pick < 0.16], 0.8, 1.15)
    emit(BEECH, ww[(pick >= 0.16) & (pick < 0.26)], 0.8, 1.1)
    emit(SYCAMORE, ww[(pick >= 0.26) & (pick < 0.34)], 0.8, 1.1)
    emit(BROADLEAF, ww[(pick >= 0.34) & (pick < 0.92)], 0.9, 1.4)
    emit(FIR, ww[pick >= 0.92], 0.85, 1.2)
    # Undergrowth under the new woods, thickest on the estate itself.
    wu = woodable(keep_common(candidates(1 / 7.0, (-1100, 600, -1500, 700)), 3.5, 3))
    wu = wu[wood_score(wu) > WOOD_AT]
    wu = clear_of_interactive(wu, 2.0)
    # Holly and hazel coppice stools take part of the old hazel and shrub shares.
    pick = rng.random(len(wu))
    emit(HAZEL, wu[pick < 0.07], 0.8, 1.25)
    emit(HAZEL_COPPICE, wu[(pick >= 0.07) & (pick < 0.14)], 0.75, 1.1)
    emit(BRACKEN, wu[(pick >= 0.14) & (pick < 0.5)], 0.8, 1.35)
    emit(FERN, wu[(pick >= 0.5) & (pick < 0.75)], 0.85, 1.4)
    emit(HOLLY, wu[(pick >= 0.75) & (pick < 0.86)], 0.7, 1.15)
    emit(SHRUB, wu[pick >= 0.86], 0.85, 1.35)
    # The woods' ragged fringe: windswept hawthorn, hazel and thorn scrub with the odd young tree.
    fringe = woodable(keep_common(candidates(1 / 14.0, (-1100, 600, -1500, 700)), 4, 4))
    fs = wood_score(fringe)
    fringe = clear_of_interactive(fringe[(fs > WOOD_AT - 0.22) & (fs <= WOOD_AT)], 2.0)
    pick = rng.random(len(fringe))
    emit_windswept(HAWTHORN, fringe[pick < 0.14], 0.7, 1.05)
    emit(SHRUB, fringe[(pick >= 0.14) & (pick < 0.45)], 0.8, 1.3)
    emit(HAZEL, fringe[(pick >= 0.45) & (pick < 0.7)], 0.7, 1.1)
    emit(BRACKEN, fringe[(pick >= 0.7) & (pick < 0.96)], 0.8, 1.2)
    emit(BROADLEAF, fringe[pick >= 0.96], 0.55, 0.8)
    # Overgrown hedges: the estate's boundary hedge and a broken hedge down one side of the drive,
    # gone to shrubs, hazel and the odd standard tree.
    hedge = []
    ring = np.r_[boundary, boundary[:1]]
    edge = densify(ring, 1.7)
    centre_b = boundary.mean(axis=0)
    inward = centre_b - edge
    inward /= np.linalg.norm(inward, axis=1, keepdims=True)
    hedge.append(edge + inward * 3.0 + rng.normal(0, 0.5, edge.shape))
    lane = densify(L["road"], 1.7)
    lt = np.gradient(lane, axis=0)
    lt /= np.linalg.norm(lt, axis=1, keepdims=True)
    ln = np.c_[-lt[:, 1], lt[:, 0]]
    along = np.arange(len(lane)) * 1.7
    gaps = (along % 45.0) > 6.0
    hedge.append((lane + ln * 7.5 + rng.normal(0, 0.4, lane.shape))[gaps])
    hedge = np.concatenate(hedge)
    hedge = keep_common(hedge, 5.5, 5)
    hedge = woodable(hedge)
    hedge = clear_of_interactive(hedge[kept_open(hedge, 0.0) < 0.3], 2.0)
    pick = rng.random(len(hedge))
    emit_windswept(HAWTHORN, hedge[pick < 0.12], 0.75, 1.0)
    emit(SHRUB, hedge[(pick >= 0.12) & (pick < 0.5)], 0.9, 1.4)
    emit(HAZEL, hedge[(pick >= 0.5) & (pick < 0.85)], 0.9, 1.3)
    emit(BRACKEN, hedge[(pick >= 0.85) & (pick < 0.97)], 0.8, 1.1)
    emit(OAK, hedge[pick >= 0.97], 0.75, 1.05)
    # Lone field trees and hedgerow shrubs on the pasture.
    field = clear_of_interactive(by_weight(keep_common(candidates(1 / 2500.0)), "Pasture", 0.6), 10)
    # Parkland field trees: mostly broad old oaks.
    pick = rng.random(len(field))
    emit(OAK, field[pick < 0.6], 0.9, 1.25)
    emit(SYCAMORE, field[(pick >= 0.6) & (pick < 0.75)], 0.85, 1.1)
    emit(BROADLEAF, field[pick >= 0.75], 1.0, 1.35)
    # Grass tufts and yarrow close to where she walks: round the manor and along the road.
    close = candidates(1 / 5.0, (-420, -80, -900, -400))
    close = np.r_[close, densify(L["road"], 1.0)[::1] + rng.normal(0, 9, (len(densify(L["road"], 1.0)), 2))]
    close = keep_common(close, 3.2, 3.5)
    close = by_weight(close, "Pasture", 0.4)
    close = clear_of_interactive(close, 1.2)
    pick = rng.random(len(close))
    emit(GRASS_TALL, close[pick < 0.45], 0.8, 1.25)
    emit(GRASS_MID, close[(pick >= 0.45) & (pick < 0.9)], 0.8, 1.3)
    emit(YARROW, close[pick >= 0.9], 0.8, 1.3)
    # Road verges: tufts along both shoulders of the cart track and a sparse line on its grass crown,
    # matching the ruts M_EstateLandscape draws from road_ruts.py (tracks at +-0.8 m, shoulders 1 m+).
    centre = densify(L["road"], 0.8)
    tang = np.gradient(centre, axis=0)
    tang /= np.linalg.norm(tang, axis=1, keepdims=True)
    normal = np.c_[-tang[:, 1], tang[:, 0]]
    verge = []
    for side in (-1.0, 1.0):
        off = rng.uniform(1.2, 2.7, len(centre))
        keep = rng.random(len(centre)) < 0.85
        verge.append((centre + normal * (side * off)[:, None] + tang * rng.uniform(-0.4, 0.4, (len(centre), 1)))[keep])
    crown = centre[rng.random(len(centre)) < 0.12]
    verge = clear_of_interactive(np.r_[verge[0], verge[1]], 1.0)
    crown = clear_of_interactive(crown, 1.0)
    is_wood = sample(W["WoodlandFloor"], verge[:, 0], verge[:, 1]) > 0.35
    pick = rng.random(len(verge))
    emit(GRASS_TALL, verge[(pick < 0.4) & ~is_wood], 1.8, 2.6)
    emit(GRASS_MID, verge[(pick >= 0.4) & (pick < 0.75) & ~is_wood], 2.0, 3.0)
    emit(YARROW, verge[(pick >= 0.75) & ~is_wood], 0.7, 1.1)
    emit(FERN, verge[(pick < 0.35) & is_wood], 0.6, 1.0)
    emit(GRASS_MID, verge[(pick >= 0.35) & is_wood], 1.8, 2.6)
    emit(GRASS_MID, crown, 1.3, 1.9)
    # Moor: bracken, boulders, erratics and a few tors on the high ground.
    moor = keep_common(candidates(1 / 70.0))
    moor = by_weight(moor, "Moorland", 0.4)
    pick = rng.random(len(moor))
    emit(BRACKEN, moor[pick < 0.7], 0.8, 1.3)
    emit(BOULDER, moor[(pick >= 0.7) & (pick < 0.95)], 0.8, 1.4)
    emit(ERRATIC, moor[pick >= 0.985], 0.85, 1.2)
    high = np.argwhere(z > 170)
    if len(high):
        for r, c in high[rng.choice(len(high), size=min(5, len(high)), replace=False)]:
            recs.append((DOME, float(H - r), float(c - H), rng.uniform(0, 360), rng.uniform(0.9, 1.05)))
    # Cliffs and steep valley sides: granite ledges breaking through the slope, and boulders. (Loose
    # cobble piles here read as the hand stones she can pick up, so the ground shows bedrock instead.)
    steep = keep_common(candidates(1 / 90.0))
    sv = slope[np.clip(np.round(H - steep[:, 0]).astype(int), 0, 4032), np.clip(np.round(H + steep[:, 1]).astype(int), 0, 4032)]
    steep = steep[(sv > 22) & (sv < 45)]
    pick = rng.random(len(steep))
    ledges = steep[(pick < 0.6) & (rng.random(len(steep)) < 0.45)]
    ledges = clear_of_interactive(ledges[clear_of_stones(ledges, 12.0)], 4.0)
    emit(LEDGE, ledges, 1.2, 1.9)
    emit(BOULDER, steep[pick >= 0.6], 0.8, 1.3)
    recs += mvp_recs

    # ---- write -------------------------------------------------------------------------
    runtime = os.path.join(ROOT, "Content", "SurvivalGame", "Estate", "Runtime")
    with open(os.path.join(runtime, "EstateScenery.bin"), "wb") as fh:
        fh.write(b"HSC1" + struct.pack("<I", len(recs)))
        for k, x, y, yaw, s in recs:
            fh.write(struct.pack("<B3x4f", k, x * 100.0, y * 100.0, yaw, s))
    # The estate lake (lake_basin.py): clear the water and its path and plant the margin, after the
    # scatter so its random draws are unchanged.
    if L.get("lake"):
        import lake_features
        lake_features.apply_to_scenery_file(os.path.join(runtime, "EstateScenery.bin"), L["lake"])
    inc = os.path.join(ROOT, "Source", "SurvivalGame", "Simulation", "HomesteadEstateWorldPlacements.inc")
    with open(inc, "w", newline="\n") as fh:
        fh.write("// Generated by Scripts/Terrain/scatter.py; do not edit by hand.\n")
        fh.write("// World-lane interactive placements: {id, kind, {x, y} cm}.\n")
        for pid, kind, (x, y) in rows:
            fh.write(f"world({pid}, ResourceKind::{kind}, {x * 100.0:.1f}, {y * 100.0:.1f});\n")
    counts = {}
    for k, *_ in recs: counts[k] = counts.get(k, 0) + 1
    kinds = {}
    for _, k, _p in rows: kinds[k] = kinds.get(k, 0) + 1
    print("scenery", len(recs), counts)
    print("placements", len(rows), kinds)

if __name__ == "__main__":
    main()
