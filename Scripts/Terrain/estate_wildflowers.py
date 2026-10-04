"""Extend original lake-trail flowers into natural Estate habitat drifts; no interactable rows change.

    python Scripts\\Terrain\\estate_wildflowers.py [--verify] [--scenery PATH]

Run after lake_path_plants.py when rebaking that trail. HSC1 padding marks only this supplemental
scatter, so repeat bakes replace it without touching original lake flowers or other scenery bytes.
Requires existing HOMESTEAD_TERRAIN_WORK inputs (see scatter.py); no assets are authored/imported.
"""
import argparse
import json
import os
import struct
from pathlib import Path

import numpy as np
from PIL import Image
from scipy.spatial import cKDTree

import lake_features
from lake_path_plants import FARM, FARM_CLEAR_M, FLOWER_KINDS, FLOWER_SCALE, TRUNK_KINDS
from scatter import HERE, ROOT, WORK, densify, sample

SEED = 20261004
TAG = b"WF3"
SCENERY = Path(ROOT) / "Content" / "SurvivalGame" / "Estate" / "Runtime" / "EstateScenery.bin"
PATCH_AREA_M2 = 1800.0       # Sparse drifts with broad unflowered gaps, not a continuous carpet.
PATCH_RADIUS_M = (2.0, 5.5)
PATCH_CLUMPS = (10, 23)
MIN_CLUMP_GAP_M = 0.65
BAKE_FOOTPRINT_M = 1.5      # Covers the existing clumps at their largest authored lake-trail scale.
MAX_SLOPE_DEG = 24.0
DRY_BANK_M = 1.0
BANK_WIDTH_M = 18.0
WATER_PATCH_STEP_M = 24.0
MIN_HEIGHT_M = 3.0
ROAD_CLEAR_M = 3.0
TRUNK_CLEAR_M = 0.8


def read_records(path: Path) -> np.ndarray:
    raw = path.read_bytes()
    if len(raw) < 8 or raw[:4] != b"HSC1":
        raise ValueError(f"{path}: not HSC1 scenery")
    count = struct.unpack_from("<I", raw, 4)[0]
    if len(raw) != 8 + count * lake_features.RECORD.itemsize:
        raise ValueError(f"{path}: scenery count/length mismatch")
    return np.frombuffer(raw[8:], lake_features.RECORD).copy()


def supplemental(records: np.ndarray) -> np.ndarray:
    return np.isin(records["k"], FLOWER_KINDS) & (records["pad"] == np.void(TAG))


def river_bank_distance(layout: dict, points: np.ndarray) -> np.ndarray:
    """Signed distance from the variable-width dry-bank edge, not just the centreline."""
    river = np.asarray(layout["river"], float)
    widths = np.asarray(layout["riverHalfWidth"], float)
    nearest = np.full(len(points), np.inf)
    bank = np.full(len(points), np.inf)
    for i, (a, b) in enumerate(zip(river[:-1], river[1:])):
        delta = b - a
        t = np.clip(((points - a) @ delta) / max(float(delta @ delta), 1e-9), 0.0, 1.0)
        distance = np.linalg.norm(points - (a + t[:, None] * delta), axis=1)
        better = distance < nearest
        width = widths[min(i, len(widths) - 1)] * (1.0 - t) + widths[min(i + 1, len(widths) - 1)] * t
        bank[better] = (distance - width)[better]
        nearest[better] = distance[better]
    return bank


class Habitat:
    def __init__(self, layout: dict, records: np.ndarray):
        self.layout = layout
        self.height = np.load(os.path.join(WORK, "game_reshaped_4033.npy"), mmap_mode="r")
        self.weights = {}
        for name in ("Pasture", "WoodlandFloor", "Moorland", "DirtRoad"):
            with Image.open(os.path.join(WORK, "weights", f"{name}.png")) as image:
                self.weights[name] = np.asarray(image, np.float32).T[::-1, :] / 255.0
        trunks = records[np.isin(records["k"], TRUNK_KINDS)]
        self.trunks = cKDTree(np.c_[trunks["x"], trunks["y"]] / 100.0)
        original = records[np.isin(records["k"], FLOWER_KINDS)]
        self.original = cKDTree(np.c_[original["x"], original["y"]] / 100.0)
        self.road = cKDTree(densify(layout["road"], 1.0))
        self.paths = [layout["lake"]["path"]] + [p["points"] for p in layout.get("footpaths", [])]
        self.polygons = [polygon for name, polygon in layout["polygons"].items()
                         if name == "EstateBoundary" or name.startswith("ForSale.")]

    def allowed(self, points: np.ndarray) -> np.ndarray:
        x, y = points.T
        inside = np.zeros(len(points), bool)
        for polygon in self.polygons:
            inside |= lake_features._signed_distance(polygon, x, y) < -BAKE_FOOTPRINT_M
        manor = self.layout["polygons"]["ManorFootprint"]
        farm = ((x >= FARM[0] - FARM_CLEAR_M - BAKE_FOOTPRINT_M)
                & (x <= FARM[1] + FARM_CLEAR_M + BAKE_FOOTPRINT_M)
                & (y >= FARM[2] - FARM_CLEAR_M - BAKE_FOOTPRINT_M)
                & (y <= FARM[3] + FARM_CLEAR_M + BAKE_FOOTPRINT_M))
        dx = (sample(self.height, x + 1, y) - sample(self.height, x - 1, y)) * 0.5
        dy = (sample(self.height, x, y + 1) - sample(self.height, x, y - 1)) * 0.5
        ground_weight = sum(sample(self.weights[n], x, y) for n in ("Pasture", "WoodlandFloor", "Moorland"))
        ok = inside & ~farm & (ground_weight > 0.35) & (sample(self.height, x, y) > MIN_HEIGHT_M)
        ok &= np.degrees(np.arctan(np.hypot(dx, dy))) < MAX_SLOPE_DEG
        ok &= lake_features._signed_distance(manor, x, y) > BAKE_FOOTPRINT_M
        ok &= lake_features._signed_distance(self.layout["lake"]["shore"], x, y) > DRY_BANK_M + BAKE_FOOTPRINT_M
        ok &= river_bank_distance(self.layout, points) > DRY_BANK_M + BAKE_FOOTPRINT_M
        ok &= self.road.query(points)[0] > ROAD_CLEAR_M + BAKE_FOOTPRINT_M
        ok &= sample(self.weights["DirtRoad"], x, y) < 0.2
        ok &= self.trunks.query(points)[0] > TRUNK_CLEAR_M + BAKE_FOOTPRINT_M
        ok &= self.original.query(points)[0] > BAKE_FOOTPRINT_M
        for path in self.paths:
            ok &= lake_features._path_distance(path, x, y) > lake_features.CLEAR_PATH_M + BAKE_FOOTPRINT_M
        return ok

    def habitat(self, points: np.ndarray) -> np.ndarray:
        lake = lake_features._signed_distance(self.layout["lake"]["shore"], *points.T)
        water = np.minimum(lake, river_bank_distance(self.layout, points))
        wood = sample(self.weights["WoodlandFloor"], *points.T) > 0.3
        return np.where(water < BANK_WIDTH_M, "bank", np.where(wood, "wood", "field"))


def generate(base: np.ndarray, habitat: Habitat) -> tuple[np.ndarray, dict]:
    rng = np.random.default_rng(SEED)
    all_corners = np.concatenate(habitat.polygons)
    lo, hi = all_corners.min(axis=0), all_corners.max(axis=0)
    count = int(np.prod(hi - lo) / PATCH_AREA_M2)
    centres = rng.uniform(lo, hi, (count, 2))
    # Extra pockets follow both river banks and the lake's entire dry perimeter.
    river = densify(habitat.layout["river"][:habitat.layout["riverEnd"]], WATER_PATCH_STEP_M)
    shore = np.asarray(habitat.layout["lake"]["shore"], float)
    shore = densify(np.r_[shore, shore[:1]], WATER_PATCH_STEP_M)
    water_centres = np.repeat(np.r_[river, shore], 3, axis=0) + rng.normal(0.0, 10.0, ((len(river) + len(shore)) * 3, 2))
    centres = np.r_[centres, water_centres]
    centres = centres[habitat.allowed(centres)]
    labels = habitat.habitat(centres)
    choices = {"bank": (44, 44, 48, 46, 43), "wood": (42, 42, 45, 45, 44, 43, 47),
               "field": (43, 46, 46, 48, 47)}
    candidates, kinds, habitats = [], [], []
    for centre, label in zip(centres, labels):
        kind = int(rng.choice(choices[label]))
        number = int(rng.integers(*PATCH_CLUMPS))
        if kind == 47:
            number = max(3, number // 3)
        radius = rng.uniform(*PATCH_RADIUS_M)
        points = centre + rng.normal(0.0, radius * 0.5, (number, 2))
        candidates.extend(points)
        kinds.extend([kind] * number)
        habitats.extend([label] * number)
    if not candidates:
        raise ValueError("No Estate flower candidates; verify existing terrain/layout inputs")
    candidates = np.asarray(candidates)
    ok = habitat.allowed(candidates)
    candidates, kinds, habitats = candidates[ok], np.asarray(kinds)[ok], np.asarray(habitats)[ok]
    flowers, cells = [], {}
    summary = {"bank": 0, "wood": 0, "field": 0}
    for point, kind, label in zip(candidates, kinds, habitats):
        cell = tuple(np.floor(point / MIN_CLUMP_GAP_M).astype(int))
        neighbours = [p for dx in (-1, 0, 1) for dy in (-1, 0, 1)
                      for p in cells.get((cell[0] + dx, cell[1] + dy), [])]
        if any(np.linalg.norm(point - previous) < MIN_CLUMP_GAP_M for previous in neighbours):
            continue
        cells.setdefault(cell, []).append(point)
        flowers.append((kind, TAG, point[0] * 100.0, point[1] * 100.0,
                        rng.uniform(0, 360), FLOWER_SCALE[int(kind)] * rng.uniform(0.85, 1.2)))
        summary[str(label)] += 1
    extra = np.array(flowers, dtype=lake_features.RECORD)
    return np.r_[base, extra], summary


def bake(path: Path, verify: bool = False) -> dict:
    records = read_records(path)
    base = records[~supplemental(records)]
    with open(os.path.join(HERE, "estate_layout.json"), encoding="utf-8") as file:
        layout = json.load(file)
    habitat = Habitat(layout, base)
    result, summary = generate(base, habitat)
    extra = result[len(base):]
    if not np.array_equal(result[:len(base)], base):
        raise AssertionError("Original scenery records changed")
    if not habitat.allowed(np.c_[extra["x"], extra["y"]] / 100.0).all():
        raise AssertionError("A float32 flower record violates terrain/landmark exclusions")
    if any(count == 0 for count in summary.values()):
        raise AssertionError(f"A requested habitat has no flowers: {summary}")
    if verify:
        repeated, repeated_summary = generate(result[~supplemental(result)], habitat)
        if repeated.tobytes() != result.tobytes() or repeated_summary != summary:
            raise AssertionError("Supplemental bake is not byte-repeatable")
        if supplemental(records).any() and records.tobytes() != result.tobytes():
            raise AssertionError("Committed supplemental flowers differ from the current bake")
    else:
        temporary = path.with_suffix(path.suffix + ".flowers.tmp")
        temporary.write_bytes(b"HSC1" + struct.pack("<I", len(result)) + result.tobytes())
        temporary.replace(path)
    return {"preserved_records": len(base), "supplemental_clumps": len(extra),
            "habitats": summary, "total_records": len(result), "mode": "verify" if verify else "bake"}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--scenery", type=Path, default=SCENERY)
    parser.add_argument("--verify", action="store_true", help="Check exclusions and deterministic rebake without writing")
    args = parser.parse_args()
    print(json.dumps(bake(args.scenery, args.verify), sort_keys=True))


if __name__ == "__main__":
    main()
