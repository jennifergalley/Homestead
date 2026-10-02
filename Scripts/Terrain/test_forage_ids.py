"""Self-test for forage_ids.py: a rebake never gives a committed id to a different kind or position.

    python Scripts\\Terrain\\test_forage_ids.py
"""
import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import forage_ids as F  # noqa: E402


def row(pid, kind, x, y, macro="forage"):
    return F.Row(macro, pid, kind, x, y)


class ForageIds(unittest.TestCase):
    def setUp(self):
        # Two brambles then two roots, as forage.py writes them (roots numbered after the brambles).
        self.committed = [row(582100, "BerryBush", 0.0, 0.0), row(582101, "BerryBush", 5000.0, 0.0),
                          row(582102, "Roots", 0.0, 5000.0), row(582103, "Roots", 5000.0, 5000.0)]
        self.targets = {"BerryBush": 2, "Roots": 2}

    def ids(self, rows):
        return {r.id: (r.kind, r.x, r.y) for r in rows}

    def test_same_picks_change_nothing(self):
        picks = [(r.kind, r.x, r.y) for r in self.committed]
        rows, added = F.allocate_pool(self.committed, picks, self.targets, "forage", 582100, 582300)
        self.assertEqual(added, [])
        self.assertEqual([r.line for r in rows], [r.line for r in self.committed])

    def test_omitted_bramble_keeps_every_id(self):
        # The terrain changed: the generator no longer picks the first bramble and finds another instead.
        picks = [("BerryBush", 5000.0, 0.0), ("BerryBush", 9000.0, 9000.0),
                 ("Roots", 0.0, 5000.0), ("Roots", 5000.0, 5000.0)]
        rows, added = F.allocate_pool(self.committed, picks, self.targets, "forage", 582100, 582300)
        self.assertEqual(added, [])                      # the kind is at its target: nothing new
        self.assertEqual(self.ids(rows), self.ids(self.committed))  # roots 582102/582103 did not shift

    def test_added_pick_gets_a_fresh_id(self):
        picks = [(r.kind, r.x, r.y) for r in self.committed] + [("Roots", 20000.0, 20000.0)]
        rows, added = F.allocate_pool(self.committed, picks, {"BerryBush": 2, "Roots": 3}, "forage", 582100, 582300)
        self.assertEqual([r.id for r in added], [582104])
        self.assertEqual(self.ids(rows[:4]), self.ids(self.committed))

    def test_retired_id_is_a_hole(self):
        picks = [(r.kind, r.x, r.y) for r in self.committed] + [("BerryBush", 30000.0, 0.0)]
        rows, added = F.allocate_pool(self.committed, picks, self.targets, "forage", 582100, 582300, retired={582101})
        self.assertNotIn(582101, [r.id for r in rows])
        self.assertEqual([r.id for r in added], [582104])  # past every used id, the retired ones included
        self.assertEqual(self.ids(rows)[582102], ("Roots", 0.0, 5000.0))

    def test_gap_keeps_new_picks_off_committed_rows(self):
        picks = [("Roots", 300.0, 5000.0)]
        _, added = F.allocate_pool(self.committed, picks, {"Roots": 3}, "forage", 582100, 582300, gap_cm=1400.0)
        self.assertEqual(added, [])

    def test_roadside_slots(self):
        committed = [row(581000, "BerryBush", 0.0, 0.0, "roadside"), row(581001, "Flowers", 6300.0, 0.0, "roadside"),
                     row(581002, "Roots", 18900.0, 0.0, "roadside")]    # slot 2 was a hole
        slot_of = lambda r: int(round(r.x / 6300.0))
        # Slot 1 loses its verge, slot 2 gains one, slot 4 is new.
        picks = {0: ("BerryBush", 0.0, 0.0), 2: ("BerryBush", 12600.0, 0.0), 3: ("Roots", 18900.0, 0.0),
                 4: ("BerryBush", 25200.0, 0.0)}
        rows, added = F.allocate_slots(committed, slot_of, picks, "roadside", 581000, 581100)
        self.assertEqual({r.id: (r.kind, r.x) for r in rows[:3]}, {r.id: (r.kind, r.x) for r in committed})
        self.assertEqual([(r.id, r.x) for r in added], [(581003, 12600.0), (581004, 25200.0)])

    def test_read_rows_refuses_damage(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "x.inc")
            with self.assertRaises(SystemExit):
                F.read_rows(path, "forage", 582100, 582300)          # missing
            open(path, "w").write("// header\nforage(582100, ResourceKind::Roots, 1.0, 2.0);\nforage(582100, ResourceKind::Roots, 3.0, 4.0);\n")
            with self.assertRaises(SystemExit):
                F.read_rows(path, "forage", 582100, 582300)          # duplicate id
            open(path, "w").write("roadside(581000, ResourceKind::Roots, 1.0, 2.0);\n")
            with self.assertRaises(SystemExit):
                F.read_rows(path, "forage", 582100, 582300)          # wrong macro / range
            open(path, "w").write("// header\nforage(582100, ResourceKind::Roots, 1.0, 2.0);\n")
            header, rows = F.read_rows(path, "forage", 582100, 582300)
            out = os.path.join(tmp, "y.inc")
            F.write_rows(out, header, rows)
            self.assertEqual(open(out).read(), open(path).read())       # round trip is byte-identical


if __name__ == "__main__":
    unittest.main()
