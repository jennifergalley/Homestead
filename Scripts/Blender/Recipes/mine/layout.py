"""Shared mine dimensions (metres, Unreal frame: +X east toward the yard path, +Y toward the sea, Z up).

House frame: origin on the levelled mine yard at ``Anchor::MineEntrance`` (``HOUSE_WORLD``), yaw 0.
Tunnel frame: origin on the tunnel floor directly below the trapdoor's centre (``tunnel_world``).

Real-object research (Jenny's D5: "an engine house in a similar style to a Cornish one, without
needing to be fully Cornish"): the small 19th-century winding and pumping houses of Cornwall,
Devon, Wales, Derbyshire and Shropshire were granite or local rubble boxes 9-12 m long and 6-8 m
wide, two or three storeys, with walls 0.6-0.9 m thick, round-arched (segmental or semicircular)
openings with dressed voussoirs, a slate roof on a gable, and a square stack built into one corner
that tapers and finishes in brick 3-5 m above the ridge. The thick end wall that carried the beam
(the bob wall) has few openings. Mine levels of the period ("drives", "levels") were small, 1.8 m by
1.2 m, but haulage levels with rails were 2-2.5 m; the game's are larger (about 3.6 m by 3.4 m) so a
third-person camera fits.
"""
import math

CM = 100.0

# --------------------------------------------------------------------------- world placement
HOUSE_WORLD = (-410.0, -722.0, 56.0)      # Anchor::MineEntrance (HomesteadEstate.cpp), metres
HOUSE_YAW = 0.0
TUNNEL_FLOOR_Z = 8.0                       # world metres: about 44 m under the house floor

# --------------------------------------------------------------------------- house shell
# Jenny (blockout review): "more engine-house: a bit taller and narrower (massive bob wall, stack
# tight to a corner)". Three storeys under a steep gable; the west end is the 1.3 m bob wall with
# the big arched opening high up where the beam once came out; the stack is built into the
# north-west corner and rises 5 m clear of the ridge.
OUT_X, OUT_Y = 9.0, 6.0                    # outer footprint
WALL = 0.70                                # granite rubble side and door-end walls
BOB_WALL = 1.30                            # the west end
IN_W, IN_E = -OUT_X / 2 + BOB_WALL, OUT_X / 2 - WALL   # interior x range (-3.2 .. 3.8)
IN_Y = OUT_Y / 2 - WALL                    # interior half width (2.3)
IN_CX = (IN_W + IN_E) / 2
FLOOR_Z = 0.18                             # ground-floor boards above the yard
PLINTH = 0.35                              # footing course visible above the yard
CEILING_Z = 3.70                           # underside of the first-floor joists
UPPER_FLOOR_Z = 3.95
EAVES_Z = 9.40
ROOF_PITCH = 40.0                          # degrees
EAVES_OVERHANG, VERGE_OVERHANG = 0.25, 0.15
RIDGE_Z = EAVES_Z + (OUT_Y / 2) * math.tan(math.radians(ROOF_PITCH))

# The stack is built into the north-west corner, flush with both outer faces.
STACK = {"centre": (-OUT_X / 2 + 0.80, OUT_Y / 2 - 0.80), "half": 0.80, "top_z": 17.0,
         "brick_from_z": 13.6, "brick_half": 0.52}

# Openings: (wall, centre along the wall, sill z, width, spring z, arch) - wall is "+X" (east
# gable, door end), "-X" (west, bob wall), "+Y" (sea side) or "-Y". Arch "round" is semicircular.
DOOR = {"wall": "+X", "centre": 0.0, "sill": FLOOR_Z, "width": 1.50, "spring": 2.35, "arch": "round"}
WINDOWS = [
    {"wall": "+X", "centre": 0.0, "sill": 4.90, "width": 1.10, "spring": 6.40, "arch": "round"},
    {"wall": "+X", "centre": 0.0, "sill": 8.60, "width": 0.70, "spring": 9.30, "arch": "round"},
    {"wall": "+Y", "centre": 1.60, "sill": 1.05, "width": 1.00, "spring": 2.65, "arch": "round"},
    {"wall": "-Y", "centre": 1.60, "sill": 1.05, "width": 1.00, "spring": 2.65, "arch": "round"},
    {"wall": "+Y", "centre": 1.60, "sill": 4.85, "width": 1.00, "spring": 6.30, "arch": "round"},
    {"wall": "-Y", "centre": 1.60, "sill": 4.85, "width": 1.00, "spring": 6.30, "arch": "round"},
    {"wall": "+Y", "centre": -1.20, "sill": 4.85, "width": 1.00, "spring": 6.30, "arch": "round"},
    {"wall": "-Y", "centre": -1.20, "sill": 4.85, "width": 1.00, "spring": 6.30, "arch": "round"},
    {"wall": "-Y", "centre": 0.20, "sill": 7.50, "width": 0.90, "spring": 8.45, "arch": "round"},
    # The bob wall's great arched opening, boarded up (the beam's way out to the shaft).
    {"wall": "-X", "centre": -0.30, "sill": 6.40, "width": 1.70, "spring": 8.10, "arch": "round", "boarded": True},
]

# --------------------------------------------------------------------------- furnishing (house frame)
# Jenny (second review): "Desk: move it off the wall, more centred in the room. Trapdoor: move it
# significantly closer to the door, still between the door and the desk."
# Entering through the east door facing west: the trapdoor is a couple of paces in, the desk stands free
# in the middle of the room beyond it with its chair behind, facing the door; the rock pile fills the
# far left-hand corner (+Y), and crates and a sack stand by the door.
# The opening is centred on a landscape vertex so the 2 x 2 landscape quads round it can be holed
# (Scripts/Mine/paint_mine_holes.py): the shaft is seen through the boards, not the yard under them.
TRAPDOOR = {"centre": (2.00, 0.00), "size": (1.20, 1.20), "hinge": "+Y"}   # opening in the boards
TRAPDOOR_LAP = 0.05                        # the lid overlaps the opening this much all round, in a rebate
SHAFT_WALL = 0.40                          # the shaft lining round the opening (fills the holed quads)
SHAFT_DEPTH_SHOWN = 6.0                    # the shaft drawn under the trapdoor before it fades to black
DESK = {"centre": (-0.80, 0.0), "size": (0.80, 1.40, 0.78), "yaw": 0.0}   # free-standing, long side across the room
LAMP_ON_DESK = (-0.98, 0.45, FLOOR_Z + 0.78)    # base of the desk lamp, at the pile's end so it lights the heap (Balance)
CHAIR = {"centre": (-1.55, 0.25), "yaw": 15.0}   # behind the desk, facing the door
ROCK_PILE = {"centre": (IN_W + 0.75, IN_Y - 0.70), "radius": 0.85, "height": 1.15}   # a clear heap (Balance)
ROCK_PILE_PICKUP = (-1.45, 1.15)          # where she stands to take the stone (walkable)

def house_to_world(x, y, z=0.0):
    """House-frame metres to world metres."""
    yaw = math.radians(HOUSE_YAW)
    c, s = math.cos(yaw), math.sin(yaw)
    return (HOUSE_WORLD[0] + c * x - s * y, HOUSE_WORLD[1] + s * x + c * y, HOUSE_WORLD[2] + z)


def tunnel_world():
    """World metres of the tunnel frame's origin (the ladder foot, under the trapdoor)."""
    x, y, _ = house_to_world(*TRAPDOOR["centre"])
    return (x, y, TUNNEL_FLOOR_Z)


def tunnel_to_world(x, y, z=0.0):
    ox, oy, oz = tunnel_world()
    yaw = math.radians(HOUSE_YAW)
    c, s = math.cos(yaw), math.sin(yaw)
    return (ox + c * x - s * y, oy + s * x + c * y, oz + z)


# --------------------------------------------------------------------------- the shallow level (tunnel frame)
# The ladder chamber sits under the shaft; a cross-cut (the adit) runs east to junction A on the north
# lode. The main gallery is a long loop like a real level's: a drive east along the north lode
# (A - N1 - N2 - B), a cross-cut south at the far end (B - C), a drive back west along the south lode
# (C - S1 - S2 - D) and a cross-cut home (D - A). The drives wander a little, as hand-cut levels
# following a lode do. Four side drifts follow branch veins and end blind; a short alcove off the
# ladder chamber holds the sump. Ladder to the far end is about 60 m: 33 s at a walk. Ground over the
# whole network is 40-70 m above its roof.
GALLERY_HALF, GALLERY_WALL_H, GALLERY_H = 1.80, 2.30, 3.40     # haulage level: 3.6 m wide, 3.4 m high
DRIFT_HALF, DRIFT_WALL_H, DRIFT_H = 1.60, 2.10, 3.10
CHAMBER = {"centre": (0.5, 0.0), "half": (3.6, 3.1), "height": 4.4}  # the ladder chamber
SHAFT = {"centre": (0.0, 0.0), "half": 0.75, "height": 9.0}         # rises from the chamber roof
LADDER_SIDE = "-X"                                                    # ladder on the shaft's west wall

POINTS = {
    "L": (0.0, 0.0), "A": (14.0, 0.0), "N1": (30.0, 2.5), "N2": (46.0, 1.5), "B": (60.0, 3.5),
    "C": (61.0, -14.0), "S1": (45.0, -16.5), "S2": (29.0, -15.0), "D": (15.0, -16.5),
    "E1": (31.5, 15.0), "E2": (73.0, 5.5), "E3": (28.0, -29.0), "E4": (5.5, -26.0), "K": (-2.6, -6.4),
}
# (from, to, kind): "gallery" (haulage level), "drift" (blind side drift), "alcove" (sump)
EDGES = [
    ("L", "A", "gallery"),
    ("A", "N1", "gallery"), ("N1", "N2", "gallery"), ("N2", "B", "gallery"), ("B", "C", "gallery"),
    ("C", "S1", "gallery"), ("S1", "S2", "gallery"), ("S2", "D", "gallery"), ("D", "A", "gallery"),
    ("N1", "E1", "drift"), ("B", "E2", "drift"), ("S2", "E3", "drift"), ("D", "E4", "drift"),
    ("L", "K", "alcove"),
]
SUMP = {"centre": (-2.3, -5.6), "half": (1.0, 0.9), "depth": 0.6}  # kerbed pool at the alcove's end
# Jenny (second review): no rails or ore carts ("In Poldark they hauled it out by hand"): broken rock went
# out in baskets and wooden barrows, so a barrow and baskets stand about as dressing.

# Node sockets: (id, kind, edge, t along it, side) - side +1 is the left wall looking from the
# edge's first point to its second, -1 the right; t = 1 with side 0 is the drift's blind end.
ROCK, COAL, COPPER, GEM = 0, 1, 2, 3
ROCK_ROUTE = [("L", "A"), ("A", "N1"), ("N1", "N2"), ("N2", "B"), ("B", "C"), ("C", "S1"), ("S1", "S2"),
              ("S2", "D"), ("D", "A")]
ROCK_IDS = range(588010, 588030)
JUNCTION_CLEAR = 3.2        # no rock face within this of a junction or the chamber


def _rock_faces():
    """The 20 rock faces evenly round the adit and the loop, alternating walls (Balance: spread
    through the main gallery so she mines her way round rather than standing at one wall)."""
    spans = []
    for edge in ROCK_ROUTE:
        length = math.dist(POINTS[edge[0]], POINTS[edge[1]])
        lo = (CHAMBER["half"][0] + 1.0) if edge[0] == "L" else JUNCTION_CLEAR
        spans.append((edge, length, lo, length - JUNCTION_CLEAR))
    usable = sum(hi - lo for _, _, lo, hi in spans)
    step = usable / len(ROCK_IDS)
    out, walked = [], step * 0.5
    for i, nid in enumerate(ROCK_IDS):
        target = step * 0.5 + i * step
        for edge, length, lo, hi in spans:
            if target <= hi - lo:
                out.append((nid, 0, edge, (lo + target) / length, 1 if i % 2 == 0 else -1))
                break
            target -= hi - lo
    return out


NODES = _rock_faces() + [    (588030, COAL, ("N1", "E1"), 0.38, 1), (588031, COAL, ("B", "E2"), 0.32, -1), (588032, COAL, ("B", "E2"), 0.58, 1),
    (588033, COAL, ("S2", "E3"), 0.36, 1), (588034, COAL, ("D", "E4"), 0.34, -1), (588035, COAL, ("D", "E4"), 0.62, 1),
    (588040, COPPER, ("N1", "E1"), 0.68, -1), (588041, COPPER, ("B", "E2"), 1.0, 0),
    (588042, COPPER, ("S2", "E3"), 0.66, -1), (588043, COPPER, ("D", "E4"), 1.0, 0),
    # Jenny (2026-10-11): "we actually don't need gems to have a dedicated spot where you mine them. I'd rather
    # they just be something you get at random when mining stone or an ore vein." The three gem-pocket spots
    # (588050-588052, never shipped; not reused) became ore veins.
    (588044, COPPER, ("N1", "E1"), 1.0, 0), (588036, COAL, ("B", "E2"), 0.84, -1), (588045, COPPER, ("S2", "E3"), 1.0, 0),
]
NODE_PROUD = 0.10           # a node's ground point sits this far in front of the wall line
ROCK_PILE_ID, TRAPDOOR_ID, LADDER_FOOT_ID = 588000, 588001, 588002


def edge_frame(edge):
    """(start, end, unit direction, left normal, length) of an edge in the tunnel frame."""
    a, b = POINTS[edge[0]], POINTS[edge[1]]
    dx, dy = b[0] - a[0], b[1] - a[1]
    length = math.hypot(dx, dy)
    ux, uy = dx / length, dy / length
    return a, b, (ux, uy), (-uy, ux), length


def edge_half(edge):
    kind = next(k for f, t, k in EDGES if (f, t) == tuple(edge))
    return DRIFT_HALF if kind == "drift" else (1.1 if kind == "alcove" else GALLERY_HALF)


def node_sockets():
    """[(id, kind, (x, y) tunnel frame, yaw deg, (nx, ny) into the gallery)] for every node."""
    out = []
    for nid, kind, edge, t, side in NODES:
        a, b, (ux, uy), (lx, ly), length = edge_frame(edge)
        half = edge_half(edge)
        if side == 0:   # blind end: the face across the drift's end, facing back down it
            x, y = b[0] + ux * (half - NODE_PROUD - 0.05), b[1] + uy * (half - NODE_PROUD - 0.05)
            nx, ny = -ux, -uy
        else:
            x = a[0] + ux * length * t + lx * side * (half - NODE_PROUD)
            y = a[1] + uy * length * t + ly * side * (half - NODE_PROUD)
            nx, ny = -lx * side, -ly * side
        out.append((nid, kind, (x, y), math.degrees(math.atan2(ny, nx)), (nx, ny)))
    return out


# --------------------------------------------------------------------------- dressing placements
PROPS = "/Game/SurvivalGame/Environment/Props"
RAIL_SECTION = 4.0
# Jenny: "Far fewer timber sets, spaced much further apart (only where the rock needs support)": one
# where the adit leaves the shaft's broken ground, and one at the mouth of each of the two drifts
# driven through the softer, jointed rock along the lodes.
TIMBER_SETS = [(("L", "A"), 0.36), (("N1", "E1"), 0.16), (("S2", "E3"), 0.16)]
HAUL = [("MineBarrow", "SM_MineBarrow", ("L", "A"), 0.68, -1.05, 12.0),
        ("MineBasketA", "SM_MineBasket", ("A", "N1"), 0.55, 1.15, 0.0),
        ("MineBasketB", "SM_MineBasket", ("C", "S1"), 0.40, -1.2, 35.0),
        ("MineBasketC", "SM_MineBasket", ("D", "E4"), 0.45, 1.0, 70.0)]
LADDER_LENGTH = 10.0

def _edge_point(edge, t):
    a, b, (ux, uy), _, length = edge_frame(edge)
    return (a[0] + ux * length * t, a[1] + uy * length * t), math.degrees(math.atan2(uy, ux))


def dressing():
    """[(label, asset path, frame, (x, y, z) m, yaw deg)] for every kit piece and prop the placement
    script spawns; frame is "house" or "tunnel"."""
    out = []
    kit = f"{PROPS}/MineKit"
    for edge, t in TIMBER_SETS:
        (x, y), yaw = _edge_point(edge, t)
        mesh = "SM_MineTimberSet" if edge == ("L", "A") else "SM_MineTimberSetDrift"
        out.append((f"MineSet{edge[1]}", f"{kit}/{mesh}", "tunnel", (x, y, 0.0), yaw))
    for label, mesh, edge, t, side, turn in HAUL:
        (x, y), yaw = _edge_point(edge, t)
        _, _, _, (lx, ly), _ = edge_frame(edge)
        out.append((label, f"{kit}/{mesh}", "tunnel", (x + lx * side, y + ly * side, 0.0), yaw + turn))
    out.append(("MineSumpWater", f"{kit}/SM_MineSumpWater", "tunnel", (*SUMP["centre"], -0.21), 0.0))
    sx, sy = SHAFT["centre"]
    out.append(("MineLadderTunnel", f"{kit}/SM_MineLadder", "tunnel", (sx - SHAFT["half"] + 0.13, sy, 0.0), 0.0))
    tx, ty = TRAPDOOR["centre"]
    out.append(("MineLadderHouse", f"{kit}/SM_MineLadder", "house",
                (tx - TRAPDOOR["size"][0] / 2 + 0.12, ty, FLOOR_Z - 0.08 - LADDER_LENGTH), 0.0))   # tops out under the closed lid
    # The desk lamp (the estate's own oil lamp meshes), lit for good by MineDeskLampLight.
    lx, ly, lz = LAMP_ON_DESK
    for part in ("SM_OilLamp", "SM_OilLampGlass", "SM_OilLampFlame"):
        out.append((f"MineDesk{part[3:]}", f"{PROPS}/OilLamp/{part}", "house", (lx, ly, lz), 200.0))
    fur = f"{PROPS}/MineFurnishing"
    out.append(("MineDesk", f"{fur}/SM_MineDesk", "house", (*DESK["centre"], FLOOR_Z), 180.0))
    out.append(("MineChair", f"{fur}/SM_MineChair", "house", (*CHAIR["centre"], FLOOR_Z), CHAIR["yaw"]))
    out.append(("MineLedger", f"{fur}/SM_MineLedger", "house", (DESK["centre"][0] - 0.18, -0.25, FLOOR_Z + DESK["size"][2]), 176.0))
    out.append(("MineRockPile", f"{fur}/SM_MineRockPile", "house", (*ROCK_PILE["centre"], FLOOR_Z), 0.0))
    out.append(("MineTrapdoorCollar", f"{fur}/SM_MineTrapdoorCollar", "house", (*TRAPDOOR["centre"], FLOOR_Z), 0.0))
    # Store props by the door: a crate and a sack (shared with the general store's kit). The barrel went: its baked
    # wood read pale and swirled against the floor (Jenny, review 10), and it can't take the plank photoscan.
    out.append(("MineCrate", f"{PROPS}/StoreCrate/SM_Store_Crate", "house", (IN_E - 0.55, IN_Y - 0.45, FLOOR_Z), 8.0))
    out.append(("MineSack", f"{PROPS}/StoreSack/SM_Store_Sack", "house", (IN_E - 0.55, -IN_Y + 0.45, FLOOR_Z), 120.0))
    return out
