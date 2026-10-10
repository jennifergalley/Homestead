"""Dress the village: a cobbled plaza with bloom beds, door lanes, a well, a noticeboard, benches, store clutter and cottage gardens.

Usage: python Scripts/Terrain/village_dress.py [--check]

Everything is derived from estate_layout.json (town.square, town.buildings, town.store, the street polyline) and
the baked heightfield, so the map-shrink stage 2 rebake carries it over by re-running this script after
town_layout.py. It writes Source/SurvivalGame/HomesteadVillageDressing.inc (props) and HomesteadVillagePaving.inc
(the paved square and lanes as shapes), which HomesteadWorldVillage.cpp lays out at runtime (positions are world cm;
the game puts each piece on the ground itself, and builds the paving as one mesh that follows the terrain).
--check prints the counts and rejected pieces and writes nothing.
"""
import argparse
import json
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
LAYOUT = HERE / "estate_layout.json"
HEIGHTFIELD = ROOT / "Content" / "SurvivalGame" / "Estate" / "Runtime" / "EstateHeightfield.r16"
OUT = ROOT / "Source" / "SurvivalGame" / "HomesteadVillageDressing.inc"
PAVING_OUT = ROOT / "Source" / "SurvivalGame" / "HomesteadVillagePaving.inc"
SEED = 20261005
GRID = 4033
HALF_GRID = 2016
R16_ZERO, R16_PER_M = 32768.0, 128.0

DOOR_HALF_M = 0.55 + 0.6           # TownStyle::DoorWidth / 2 + the door-side margin in HomesteadTownBuilding.cpp
GARDEN_COTTAGES = ("NorthCottage", "EastCottage", "SouthCottage", "WestCottage")
GARDEN_DEPTH_M = 7.0
GARDEN_GAP_M = 1.6                 # between the back wall and the first bed
BED_PITCH_M = 1.45
FENCE_BAY_M = 2.75                 # post centres, as the derelict farm
HEDGE_PITCH_M = 3.3
STREET_CLEAR_M = 2.2
BUILDING_MARGIN_M = 0.6
MAX_SLOPE = 0.28                   # rise over run across a piece's span; steeper spots are skipped
PAVE_CORNER_M = 3.0                # corner radius of the paved plaza
BED_RADIUS_M = 3.0                 # a flower bed is a round hole in the paving
BED_FLOWER_REACH_M = 2.3           # flowers stay this close to a bed's centre
LANE_HALF_M = 1.1                  # half width of a door lane
MOUTH_LANE_HALF_M = 1.7            # half width of the lane from the street to the plaza
STORE_LANE_HALF_M = 1.5
LANE_OVERLAP_M = 0.8               # a lane runs this far into the plaza and into the house wall, so no gap shows
PLAZA_INSET_M = np.array([2.5, 2.0])   # unpaved flower strip between the plaza and the house fronts, E/W and N/S
# Planting beds left bare inside the plaza, as tile-grid cell centres relative to the square centre (metres).
FLOWER_BEDS_M = ((-10.0, 10.0), (10.0, 10.0), (-10.0, -10.0), (10.0, -2.0))
FRONT_FLOWER_PITCH_M = 1.15
DOOR_CLEAR_M = 2.0                 # no front-strip flowers this close to a door; the lane runs there
NOTICEBOARD_OFFSET_M = np.array([8.0, -12.0])   # from the square centre; faces the arrival from the road
WELL_OFFSET_M = np.array([-1.0, -1.0])   # from the square centre, off the lane to the store
BENCH_RING_M = 3.9

CROPS = ("CropBroadBean", "CropCabbage", "CropCarrot", "CropPotato", "CropStrawberry", "CropTurnip")
CROP_STAGES = ("Sprout", "Young", "Growing", "Mature", "Ripe")
# (folder, mesh, drawn scale) for the clumps that fill the borders; the village reads in bloom.
BLOOMS = (("Bluebell", "SM_BluebellClump", 2.3), ("Foxglove", "SM_Foxglove", 2.2), ("RedCampion", "SM_RedCampionClump", 2.4),
          ("Primrose", "SM_PrimroseClump", 2.4))
DAFFODIL = ("WildDaffodil", "SM_WildDaffodilClump", 2.4)


class Heights:
    def __init__(self):
        raw = np.fromfile(HEIGHTFIELD, dtype="<u2").reshape(GRID, GRID)
        self.metres = (raw.astype(np.float32) - R16_ZERO) / R16_PER_M

    def at(self, x, y):
        row = int(round(y)) + HALF_GRID
        col = int(round(x)) + HALF_GRID
        return float(self.metres[row, col])

    def interp(self, x, y):
        fx, fy = x + HALF_GRID, y + HALF_GRID
        c0, r0 = int(np.floor(fx)), int(np.floor(fy))
        tx, ty = fx - c0, fy - r0
        m = self.metres
        return float((m[r0, c0] * (1 - tx) + m[r0, c0 + 1] * tx) * (1 - ty) + (m[r0 + 1, c0] * (1 - tx) + m[r0 + 1, c0 + 1] * tx) * ty)

    def slope(self, p, radius=1.0):
        dx = self.at(p[0] + radius, p[1]) - self.at(p[0] - radius, p[1])
        dy = self.at(p[0], p[1] + radius) - self.at(p[0], p[1] - radius)
        return float(np.hypot(dx, dy) / (2 * radius))


def facades(layout):
    """Per building: name, front centre, out (toward the square), side, width, depth, door centre."""
    rows = {}
    for b in layout["town"]["buildings"]:
        yaw = np.radians(b["yaw"])
        out = np.array([-np.cos(yaw), -np.sin(yaw)])
        side = np.array([-out[1], out[0]])
        # The actor's local +Y is -side; the door sits at DoorAt * (half width - door half - margin) along it.
        door_offset = b["doorAt"] * max(0.0, b["width"] / 2 - DOOR_HALF_M)
        front = np.array([b["x"], b["y"]])
        rows[b["name"]] = dict(front=front, out=out, side=side, width=b["width"], depth=b["depth"], yaw=b["yaw"],
                               door=front - side * door_offset)
    store = np.array(layout["town"]["store"]["footprint"])
    lo, hi = store.min(axis=0), store.max(axis=0)
    door = np.array(layout["landmarks"]["GeneralStoreDoor"][:2])
    rows["GeneralStore"] = dict(front=door + np.array([0.0, 0.5]), out=np.array([0.0, 1.0]), side=np.array([1.0, 0.0]),
                                width=float(hi[0] - lo[0]), depth=float(hi[1] - lo[1]), yaw=-90.0, door=door)
    return rows


def densify(points, step=0.5):
    out = []
    pts = np.asarray(points, dtype=float)
    for a, b in zip(pts[:-1], pts[1:]):
        n = max(1, int(np.ceil(np.hypot(*(b - a)) / step)))
        out.extend(a + (b - a) * (i / n) for i in range(n))
    out.append(pts[-1])
    return np.array(out)


class Dresser:
    def __init__(self, layout):
        self.layout = layout
        self.town = layout["town"]
        self.rng = np.random.default_rng(SEED)
        self.heights = Heights()
        self.rows = facades(layout)
        self.street = densify(self.town["street"])
        self.centre = np.array(self.town["square"]["centre"], dtype=float)
        self.lines = []
        self.paving = []
        self.skipped = {}
        self.plaza_shape()
        self.benches = []

    def plaza_shape(self):
        """The paved plaza is one rounded rectangle inside the house-front strips, with a round hole per flower bed."""
        self.half_paved = np.array([self.town["square"]["halfX"], self.town["square"]["halfY"]], dtype=float) - PLAZA_INSET_M
        self.bed_centres = [self.centre + np.array(b) for b in FLOWER_BEDS_M]

    def paved(self, p, margin=0.0):
        rel = np.abs(np.asarray(p) - self.centre)
        if np.any(rel > self.half_paved + margin):
            return False
        return not any(np.hypot(*(np.asarray(p) - b)) < BED_RADIUS_M + margin for b in self.bed_centres)

    def inside_building(self, p, margin=BUILDING_MARGIN_M):
        for b in self.rows.values():
            rel = p - b["front"]
            u, v = float(rel @ b["out"]), float(rel @ b["side"])
            if -b["depth"] - margin < u < margin and abs(v) < b["width"] / 2 + margin:
                return True
        return False

    def near_street(self, p, clear=STREET_CLEAR_M):
        return float(np.hypot(*(self.street - p).T).min()) < clear

    def skip(self, why):
        self.skipped[why] = self.skipped.get(why, 0) + 1

    def ok(self, p, why, street=True, slope=True, margin=BUILDING_MARGIN_M):
        if self.inside_building(p, margin):
            self.skip(why + ":building")
            return False
        if street and self.near_street(p):
            self.skip(why + ":street")
            return False
        if slope and self.heights.slope(p) > MAX_SLOPE:
            self.skip(why + ":slope")
            return False
        return True

    def put(self, mode, folder, mesh, p, yaw, scale):
        self.lines.append('V({}, TEXT("{}"), TEXT("{}"), {:.1f}f, {:.1f}f, {:.1f}f, {:.3f}f)'.format(
            mode, folder, mesh, p[0] * 100.0, p[1] * 100.0, yaw % 360.0, scale))

    def lane(self, a, b, half):
        self.paving.append('PAVE_LANE({:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f)'.format(
            a[0] * 100.0, a[1] * 100.0, b[0] * 100.0, b[1] * 100.0, half * 100.0))

    def lane_to_plaza(self, start, direction, half, limit=14.0):
        """A lane from start along direction until it meets the paved plaza, and a little into it and the wall behind."""
        direction = np.asarray(direction, dtype=float) / np.hypot(*direction)
        reach = 0.0
        while reach < limit and not self.paved(start + direction * reach):
            reach += 0.25
        self.lane(start - direction * LANE_OVERLAP_M, start + direction * (reach + LANE_OVERLAP_M), half)

    # --- the square -------------------------------------------------------------------------------------------

    def well(self):
        p = self.centre + WELL_OFFSET_M
        self.well_at = p
        self.put("Solid", "VillageWell", "SM_VillageWell", p, 0.0, 1.0)
        # Two benches facing the well from the sides clear of the store lane, the sitter's back to the cottages.
        for a_deg in (200.0, 340.0):
            a = np.radians(a_deg)
            direction = np.array([np.cos(a), np.sin(a)])
            bench = p + direction * BENCH_RING_M
            if self.ok(bench, "bench", street=True, slope=True):
                facing = np.degrees(np.arctan2(-direction[1], -direction[0]))
                self.put("Solid", "VillageBench", "SM_VillageBench", bench, facing, 1.0)
                self.benches.append(bench)

    def plaza(self):
        """The whole square is one continuous paved surface; the runtime lays it as a mesh that follows the ground."""
        self.paving.append('PAVE_RECT({:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f)'.format(
            self.centre[0] * 100.0, self.centre[1] * 100.0, self.half_paved[0] * 100.0, self.half_paved[1] * 100.0,
            PAVE_CORNER_M * 100.0))
        for c in self.bed_centres:
            self.paving.append('PAVE_HOLE({:.1f}f, {:.1f}f, {:.1f}f)'.format(c[0] * 100.0, c[1] * 100.0, BED_RADIUS_M * 100.0))

    def beds(self):
        """Daffodil and bloom beds set into the paving."""
        for c in self.bed_centres:
            for gx in range(4):
                for gy in range(4):
                    q = c + (np.array([gx, gy]) - 1.5) * 0.95 + self.rng.normal(0.0, 0.12, 2)
                    if np.hypot(*(q - c)) > BED_FLOWER_REACH_M:
                        continue
                    if self.rng.random() < 0.5:
                        folder, mesh, scale = DAFFODIL
                    else:
                        folder, mesh, scale = BLOOMS[int(self.rng.integers(len(BLOOMS)))]
                    self.put("Upright", folder, mesh, q, self.rng.uniform(0, 360), scale * self.rng.uniform(0.85, 1.15))

    def noticeboard(self):
        """A village noticeboard on the plaza facing the arrival; a placeholder for quests and village requests."""
        p = self.centre + NOTICEBOARD_OFFSET_M
        arrival = np.array(self.layout["landmarks"]["TownArrival"][:2])
        d = arrival - p
        self.put("Solid", "VillageNoticeboard", "SM_VillageNoticeboard", p, np.degrees(np.arctan2(d[1], d[0])), 1.0)

    def front_flowers(self):
        """A bloom strip along each house front, broken at the door where the lane runs."""
        for name, b in self.rows.items():
            if name == "GeneralStore":
                continue
            door_v = float((b["door"] - b["front"]) @ b["side"])
            count = int((b["width"] - 1.4) // FRONT_FLOWER_PITCH_M)
            for i in range(count):
                v = (i - (count - 1) / 2) * FRONT_FLOWER_PITCH_M
                if abs(v - door_v) < DOOR_CLEAR_M:
                    continue
                p = b["front"] + b["side"] * v + b["out"] * self.rng.uniform(0.8, 1.2)
                if self.paved(p, 0.3) or any(np.hypot(*(p - q)) < 1.1 for q in self.benches):
                    continue
                if self.ok(p, "frontflower"):
                    folder, mesh, scale = BLOOMS[int(self.rng.integers(len(BLOOMS)))]
                    self.put("Upright", folder, mesh, p, self.rng.uniform(0, 360), scale * self.rng.uniform(0.85, 1.15))

    def square_lanes(self):
        """Cobbles from the street mouth to the plaza, and the store's threshold with its barrels and crates."""
        arrival = np.array(self.layout["landmarks"]["TownArrival"][:2])
        entry = np.array(self.town["street"][-1])
        door = self.rows["GeneralStore"]["door"]
        self.lane(entry, arrival, MOUTH_LANE_HALF_M)
        self.lane_to_plaza(arrival, self.centre - arrival, MOUTH_LANE_HALF_M)
        self.lane_to_plaza(door, self.rows["GeneralStore"]["out"], STORE_LANE_HALF_M)
        for dx, dy, kind in ((-2.7, 1.5, "barrel"), (-2.9, 2.5, "crate"), (-1.2, 1.4, "sack"), (2.2, 1.5, "barrel"),
                             (2.9, 1.6, "sack"), (2.5, 2.5, "crate")):
            p = door + np.array([dx, dy]) + self.rng.normal(0.0, 0.05, 2)
            if not self.ok(p, "store", street=False, margin=0.0):
                continue
            if kind == "barrel":
                self.put("Solid", "StoreBarrel", "SM_Store_Barrel", p, self.rng.uniform(0, 360), 1.0)
            elif kind == "crate":
                self.put("Solid", "StoreCrate", "SM_Store_Crate", p, self.rng.uniform(-12, 12), 1.0)
            else:
                self.put("Solid", "StoreSack", "SM_Store_Sack", p, self.rng.uniform(0, 360), 1.0)

    def thresholds(self):
        """A cobbled lane across the front strip to every door, and a bench against the wall by three of them."""
        for b in self.rows.values():
            if b is not self.rows["GeneralStore"]:
                self.lane_to_plaza(b["door"], b["out"], LANE_HALF_M)
        for name, offset in (("Inn", 2.8), ("Baker", -2.6), ("Chemist", 2.4)):
            b = self.rows.get(name)
            if not b:
                continue
            p = b["door"] + b["side"] * offset + b["out"] * 0.4
            if self.ok(p, "wallbench", street=True, margin=0.0):
                self.put("Solid", "VillageBench", "SM_VillageBench", p, np.degrees(np.arctan2(b["out"][1], b["out"][0])), 1.0)
                self.benches.append(p)

    # --- cottage gardens --------------------------------------------------------------------------------------

    def garden(self, name):
        b = self.rows[name]
        back, away, side = b["front"] - b["out"] * b["depth"], -b["out"], b["side"]
        half = b["width"] / 2 + 0.6
        yaw_along = np.degrees(np.arctan2(side[1], side[0]))
        near, far = GARDEN_GAP_M, GARDEN_GAP_M + GARDEN_DEPTH_M

        def at(u, v):
            return back + away * u + side * v

        # Beds: two rows, crop stages mixed so it reads as a cottage plot rather than a farm field.
        rows = ((near + 1.0, True), (near + 2.9, False))
        for u, full in rows:
            count = int((2 * half - 0.5) // BED_PITCH_M)
            for i in range(count):
                v = (i - (count - 1) / 2) * BED_PITCH_M
                p = at(u, v)
                if not self.ok(p, "bed"):
                    continue
                # Some beds are borders of flowers instead of crops.
                if self.rng.random() < (0.28 if full else 0.4):
                    folder, mesh, scale = BLOOMS[int(self.rng.integers(len(BLOOMS)))]
                    for _ in range(2):
                        q = p + self.rng.normal(0.0, 0.3, 2)
                        self.put("Upright", folder, mesh, q, self.rng.uniform(0, 360), scale * self.rng.uniform(0.85, 1.15))
                    continue
                crop = CROPS[int(self.rng.integers(len(CROPS)))]
                stage = CROP_STAGES[int(self.rng.integers(len(CROP_STAGES)))]
                yaw = yaw_along + (180.0 if self.rng.random() < 0.5 else 0.0)
                self.put("Flat", "TilledBed", "SM_TilledBed", p, yaw, 1.0)
                self.put("Flat", crop, "SM_{}_{}".format(crop, stage), p, yaw, 1.0)
        # A bloom border along the back fence.
        for i in range(int(2 * half // 1.3)):
            v = -half + 0.65 + i * 1.3
            p = at(far - 0.9, v + self.rng.uniform(-0.2, 0.2))
            if self.ok(p, "border"):
                folder, mesh, scale = BLOOMS[int(self.rng.integers(len(BLOOMS)))]
                self.put("Upright", folder, mesh, p, self.rng.uniform(0, 360), scale * self.rng.uniform(0.9, 1.2))
        # Fence on the sides and the back, with a hazel hedge behind the back rail.
        self.fence(at(near, -half), at(far, -half))
        self.fence(at(near, half), at(far, half))
        self.fence(at(far, -half), at(far, half))
        for i in range(int(2 * half // HEDGE_PITCH_M) + 1):
            v = -half + i * HEDGE_PITCH_M + self.rng.uniform(-0.3, 0.3)
            p = at(far + 0.9, v)
            if self.ok(p, "hedge"):
                self.put("Upright", "Hazel", "SM_Hazel", p, self.rng.uniform(0, 360), self.rng.uniform(0.8, 1.0))

    def fence(self, a, b):
        """Posts at bay spacing and three rails per bay, between two points (the derelict farm's pieces)."""
        length = float(np.hypot(*(b - a)))
        bays = max(1, int(round(length / FENCE_BAY_M)))
        direction = (b - a) / max(length, 1e-6)
        yaw = np.degrees(np.arctan2(direction[1], direction[0]))
        bay = length / bays
        for i in range(bays + 1):
            p = a + direction * bay * i
            if self.ok(p, "post", street=True):
                self.put("Post", "FarmFence", "SM_FarmFencePost", p, yaw, 1.0)
        for i in range(bays):
            p = a + direction * bay * i
            if self.ok(p, "rail", street=True):
                self.put("Rail", "FarmFence", "SM_FarmFenceRail", p, yaw, bay * 100.0)

    def run(self):
        self.well()
        self.plaza()
        self.beds()
        self.noticeboard()
        self.square_lanes()
        self.thresholds()
        self.front_flowers()
        for name in GARDEN_COTTAGES:
            self.garden(name)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="print counts and skipped pieces, write nothing")
    args = parser.parse_args()
    layout = json.loads(LAYOUT.read_text(encoding="utf-8"))
    dresser = Dresser(layout)
    dresser.run()
    kinds = {}
    for line in dresser.lines:
        key = line.split("TEXT(")[2].split(")")[0]
        kinds[key] = kinds.get(key, 0) + 1
    print("{} pieces".format(len(dresser.lines)), kinds)
    print("skipped", dresser.skipped)
    if args.check:
        return
    header = ("// Generated by Scripts/Terrain/village_dress.py - do not edit by hand.\n"
              "// V(Mode, Folder, Mesh, X cm, Y cm, Yaw degrees, Scale); Rail's scale is its bay length in cm.\n")
    OUT.write_text(header + "\n".join(dresser.lines) + "\n", encoding="utf-8", newline="\r\n")
    print("wrote", OUT)
    header = ("// Generated by Scripts/Terrain/village_dress.py - do not edit by hand.\n"
              "// The paved square and lanes, world cm: PAVE_RECT(centre X, centre Y, half X, half Y, corner radius),\n"
              "// PAVE_HOLE(centre X, centre Y, radius) and PAVE_LANE(X0, Y0, X1, Y1, half width).\n")
    PAVING_OUT.write_text(header + "\n".join(dresser.paving) + "\n", encoding="utf-8", newline="\r\n")
    print("wrote", PAVING_OUT, len(dresser.paving), "shapes")


if __name__ == "__main__":
    main()
