"""Focused supplementary-scatter tests: python -m unittest discover -s Tests -p test_estate_wildflowers.py."""
import sys
import unittest
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Scripts" / "Terrain"))
import estate_wildflowers as flowers


class EstateWildflowersTests(unittest.TestCase):
    def test_marker_only_claims_owned_flower_records(self):
        records = np.zeros(4, dtype=flowers.lake_features.RECORD)
        records["k"] = [42, 43, 13, 48]
        records["pad"] = [b"\0\0\0", flowers.TAG, flowers.TAG, b"\0\0\0"]
        self.assertEqual(flowers.supplemental(records).tolist(), [False, True, False, False])
        original = records[~flowers.supplemental(records)]
        self.assertEqual(original.tobytes(), records[[0, 2, 3]].tobytes())

    def test_river_exclusion_follows_variable_bank_width(self):
        layout = {"river": [[0, 0], [10, 0], [20, 0]], "riverHalfWidth": [1, 3, 5]}
        points = np.array([[0, 2], [5, 2], [10, 2], [15, 2], [20, 2]], float)
        np.testing.assert_allclose(flowers.river_bank_distance(layout, points), [1, 0, -1, -2, -3])

    def test_separate_bank_legs_use_the_nearest_segment(self):
        layout = {"river": [[0, 0], [10, 0], [10, 10]], "riverHalfWidth": [1, 1, 3]}
        points = np.array([[5, 3], [8, 8]], float)
        np.testing.assert_allclose(flowers.river_bank_distance(layout, points), [2, -0.6])


if __name__ == "__main__":
    unittest.main()
