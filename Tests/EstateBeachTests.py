"""Checks the graded estate beach (Scripts/Terrain/beach_belt.py) in the committed heightfield.

    python -m unittest Tests/EstateBeachTests.py

The belt along the foot of the south cliffs must be dry sand the whole way from the west boundary to the
cove's headland, one walkable stretch from end to end, free of scarps across the swash zone, and leave the
river mouth open to the sea (openspec/changes/widen-estate-beach).
"""
import json
import unittest
from pathlib import Path

import numpy as np
from scipy.ndimage import label, maximum_filter

ROOT = Path(__file__).resolve().parents[1]
R16 = ROOT / "Content" / "SurvivalGame" / "Estate" / "Runtime" / "EstateHeightfield.r16"
LAYOUT = ROOT / "Scripts" / "Terrain" / "estate_layout.json"
H = 2016
X_WINDOW = (-860, -440)            # the south coast lies in here (beach_belt.py)
DRY_MIN_M = 15.0                   # dry sand across every 10 m of coast (the belt is 17-38 m; margin for the grid)
WALK_SLOPE = 0.35                  # walkable sand (1 in 3; the berm is 1 in 10 to 1 in 30)
SCARP_SLOPE = 0.35                 # no bank steeper than this over 1 m in the sand and swash zone (review: 0.68-0.95)
CLIFF_CLEAR_M = 3                  # cells this near ground higher than the berm are the cliff foot or a stack


class EstateBeachTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.layout = json.loads(LAYOUT.read_text())
        raw = np.fromfile(R16, "<u2").reshape(4033, 4033)
        cls.z = (raw.astype(np.float64) - 32768.0) / 128.0            # [row = y + H, col = x + H]
        beach = cls.layout.get("beach", {})
        cls.graded = bool(beach.get("graded"))
        cls.y_west, cls.y_east = beach.get("yWest", -1150.0), beach.get("yEast", -615.0)
        cls.top, cls.swash = beach.get("top", 1.7), beach.get("swash", 0.3)
        r0, r1 = int(cls.y_west) + H, int(cls.y_east) + H
        c0, c1 = X_WINDOW[0] + H, X_WINDOW[1] + H
        cls.rows = slice(r0, r1 + 1)
        cls.cols = slice(c0, c1 + 1)
        cls.sub = cls.z[cls.rows, cls.cols]                               # [y, x] over the belt's coast
        gy, gx = np.gradient(cls.sub)
        cls.slope = np.hypot(gx, gy)
        # Sand: the berm's heights, away from the cliff foot and the sea stacks standing on the beach.
        high = maximum_filter(cls.sub, size=2 * CLIFF_CLEAR_M + 1) > cls.top + 0.6
        cls.sand = (cls.sub >= cls.swash - 0.02) & (cls.sub <= cls.top + 0.5) & ~high

    def test_graded(self):
        self.assertTrue(self.graded, "estate_layout.json has no graded beach")

    def test_dry_all_along(self):
        """Every 10 m of coast from the west boundary to the headland has 15 m or more of dry, flat sand."""
        short = []
        for j in range(0, self.sub.shape[0], 10):
            line = self.sand[j] & (self.slope[j] < WALK_SLOPE)
            best = run = 0
            for v in line:
                run = run + 1 if v else 0
                best = max(best, run)
            if best < DRY_MIN_M:
                short.append((int(self.y_west) + j, best))
        self.assertFalse(short, f"stretches with less dry sand than {DRY_MIN_M} m (y, m): {short}")

    def test_walkable_end_to_end(self):
        """One walkable stretch of sand runs from the cove's headland to the west boundary."""
        walk = self.sand & (self.slope < WALK_SLOPE)
        parts, _ = label(walk, structure=np.ones((3, 3)))
        east = set(np.unique(parts[-12:][walk[-12:]]))                     # the headland end
        west = set(np.unique(parts[:12][walk[:12]]))                       # the west boundary end
        self.assertTrue(east & west, "no walkable sand connects the headland to the west boundary")

    def test_cove_to_beach(self):
        """Walkable sand runs from the foot of the cove steps round the bay onto the long beach (shrink-estate-map)."""
        cx, cy = self.layout["landmarks"]["CoveBeach"][:2]
        y0, y1 = int(self.y_west), int(cy) + 40
        sub = self.z[y0 + H:y1 + H + 1, X_WINDOW[0] + H:X_WINDOW[1] + H + 1]
        gy, gx = np.gradient(sub)
        high = maximum_filter(sub, size=3) > self.top + 0.6
        walk = (sub >= self.swash - 0.02) & (sub <= self.top + 0.5) & ~high & (np.hypot(gx, gy) < WALK_SLOPE)
        parts, _ = label(walk, structure=np.ones((3, 3)))
        r, c = int(cy) - y0, int(cx) - X_WINDOW[0]
        foot = set(np.unique(parts[r - 6:r + 7, c - 6:c + 7][walk[r - 6:r + 7, c - 6:c + 7]]))
        west = set(np.unique(parts[:12][walk[:12]]))
        self.assertTrue(foot & west, "no walkable sand from the cove steps' foot to the west end of the beach")

    def test_no_scarps(self):
        """No bank steeper than 1 in 2.9 in the sand or across the swash zone (above 1.5 m under the sea)."""
        zone = (self.sub >= -1.5) & (self.sub <= self.top + 0.5)
        high = maximum_filter(self.sub, size=2 * CLIFF_CLEAR_M + 1) > self.top + 0.6
        zone &= ~high
        steep = zone & (self.slope > SCARP_SLOPE)
        ys, xs = np.nonzero(steep)
        self.assertEqual(len(ys), 0, f"{len(ys)} scarp cells, e.g. (x, y) "
                         f"{[(int(x) + X_WINDOW[0], int(y) + int(self.y_west)) for y, x in list(zip(ys, xs))[:6]]}")

    def test_river_mouth_open(self):
        """The river's channel stays below its surface down to the sea (the belt keeps clear of it)."""
        end = self.layout["riverEnd"]
        river = np.asarray(self.layout["river"][:end], np.float64)
        surface = np.asarray(self.layout["riverSurface"], np.float64)
        for k in range(end - 15, end):
            x, y = river[k]
            ground = self.z[int(round(y)) + H, int(round(x)) + H]
            self.assertLess(ground, surface[k] - 0.1, f"river point {k} at ({x:.0f}, {y:.0f}): the bed at "
                            f"{ground:.2f} m is not under its surface {surface[k]:.2f} m")
        x, y = river[end - 1]
        self.assertLess(self.z[int(round(y)) + H, int(round(x)) + H], 0.0, "the river's last point is not under the sea")


if __name__ == "__main__":
    unittest.main()
