"""Map shrink stage 2: write the approved compact-map geometry into estate_layout.json.

Usage: python Scripts/Terrain/compact_map.py [--dry]

The geometry is Jenny's approved layout v3 (2026-10-10, openspec/changes/shrink-estate-map): a skinnier home
estate with the village outside it, two for-sale plots beside it, three neighbouring estates and the
communal land as named outlines, and the playable rectangle the map sheet and the world edge use. It
replaces stage 1's boundary (with its slit and pocket round the village) and the three old for-sale
parcels, moves the mine ruin to the clifftop west of the cove and the cove anchor to the foot of the
new cove route's steps (cove_route.py). Mirror the polygons and anchors in HomesteadEstate.cpp (cm) by hand.
A rerun changes nothing. Metres, +X north, +Y east.
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
LAYOUT = os.path.join(HERE, "estate_layout.json")

ESTATE = [(60, -880), (60, -600), (-40, -505), (-70, -470), (-380, -470), (-560, -450), (-720, -450), (-720, -880)]
FOR_SALE = {
    "TopField": [(70, -1030), (190, -1030), (190, -600), (70, -600)],
    "CarnWood": [(-520, -1130), (60, -1130), (60, -890), (-520, -890)],
}
NEIGHBOURS = {   # Outline only; kept off her routes and east of the river (Polwhele), so the river stays common.
    "Penhallow": [(200, -1130), (540, -1130), (540, -600), (200, -600)],
    "Tregarthen": [(-520, -1350), (200, -1350), (200, -1140), (-520, -1140)],
    "Polwhele": [(-640, -340), (-380, -340), (-380, 150), (-640, 150)],
}
COMMONS = {
    "VillageGreen": [(-140, -255), (-60, -255), (-60, -195), (-140, -195)],
    "Allotments": [(-265, -385), (-195, -385), (-195, -300), (-265, -300)],
    "ChapelSands": [(-770, -390), (-650, -390), (-650, -280), (-770, -280)],
    "VillageGrowth": [(-215, -455), (15, -455), (15, -205), (-215, -205)],   # room for the village to grow
}
PLAYABLE = [(-900, -1360), (560, -1360), (560, 160), (-900, 160)]   # the map sheet and the world's edge
MINE = (-410.0, -722.0)        # the mine ruin's yard on the clifftop west of the cove
COVE = (-476.0, -616.0, 1.8, 200.0)   # the sand at the foot of the cove steps, facing the sea
OLD_FOR_SALE = ("ForSale.Woodland", "ForSale.MoorField", "ForSale.WestCove")


def ring(points):
    return [[float(x), float(y)] for x, y in points]


def main():
    dry = "--dry" in sys.argv
    layout = json.load(open(LAYOUT))
    before = json.dumps(layout, sort_keys=True)
    polygons = {k: v for k, v in layout["polygons"].items()
                if k not in OLD_FOR_SALE and not k.startswith(("Neighbour.", "Common."))}
    polygons["EstateBoundary"] = ring(ESTATE)
    for name, pts in FOR_SALE.items():
        polygons["ForSale." + name] = ring(pts)
    for name, pts in NEIGHBOURS.items():
        polygons["Neighbour." + name] = ring(pts)
    for name, pts in COMMONS.items():
        polygons["Common." + name] = ring(pts)
    polygons["PlayableBounds"] = ring(PLAYABLE)
    layout["polygons"] = polygons
    mine = layout["landmarks"]["MineEntrance"]
    layout["landmarks"]["MineEntrance"] = [MINE[0], MINE[1], mine[2], mine[3]]
    layout["landmarks"]["CoveBeach"] = list(COVE)
    changed = json.dumps(layout, sort_keys=True) != before
    print("compact map:", "changed" if changed else "already applied")
    if changed and not dry:
        with open(LAYOUT, "w") as fh:
            json.dump(layout, fh, indent=1)


if __name__ == "__main__":
    main()
