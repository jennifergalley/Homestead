"""Chopping block: a bark-ringed stump round with a hatchet biting into the top."""
import math

NAME = "ChoppingBlock"
DESCRIPTION = "Homestead yard prop: stump chopping block with an embedded hatchet."
COLLISION = "box"
TRIANGLE_BUDGET = 3000


def build(kit):
    bark = kit.material("M_ChoppingBlockBark", (0.20, 0.13, 0.08), roughness=0.95)
    wood = kit.material("M_ChoppingBlockWood", (0.55, 0.40, 0.24), roughness=0.85)
    handle = kit.material("M_ChoppingBlockHandle", (0.42, 0.27, 0.14), roughness=0.7)
    steel = kit.material("M_ChoppingBlockSteel", (0.30, 0.31, 0.33), roughness=0.45)

    radius, height = 0.30, 0.46
    stump = kit.cylinder("Stump", radius, height, (0, 0, height / 2), material=bark, sides=18)
    kit.roughen(stump, strength=0.022, scale=7.0, seed=3, subdivide=2)
    kit.taper(stump, top_scale=0.93)

    # Lighter end grain sitting just proud of the bark, slightly tilted like a real cut.
    top = kit.cylinder("EndGrain", radius * 0.9, 0.03, (0.005, 0, height + 0.004),
                       rotation=(1.2, -0.8, 0), material=wood, sides=18, bevel=0.006)
    kit.roughen(top, strength=0.006, scale=9.0, seed=5)

    # Hatchet. In head-local space the poll (eye) is at the origin, the cutting
    # edge points down -Z and the haft leaves along -X. Rotating by `lean` about
    # Y raises the haft `lean` degrees above horizontal and buries the bit.
    lean = 40.0
    bit_length, bury = 0.12, 0.05
    head_vertices = [
        (-0.03, -0.014, 0), (0.03, -0.014, 0), (0.03, 0.014, 0), (-0.03, 0.014, 0),
        (-0.045, -0.0015, -bit_length), (0.045, -0.0015, -bit_length),
        (0.045, 0.0015, -bit_length), (-0.045, 0.0015, -bit_length),
    ]
    head_faces = [(0, 1, 2, 3), (7, 6, 5, 4), (0, 4, 5, 1), (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0)]
    poll = (0.06, -0.02, height + bit_length * math.cos(math.radians(lean)) - bury)
    head = kit.mesh("Head", head_vertices, head_faces, steel, location=poll, rotation=(0, lean, 0))
    kit.recalc_normals(head)

    haft_length = 0.40
    up = (-math.cos(math.radians(lean)), 0, math.sin(math.radians(lean)))
    reach = haft_length / 2 - 0.015
    haft = kit.cylinder("Haft", 0.016, haft_length,
                        tuple(p + u * reach for p, u in zip(poll, up)),
                        rotation=(0, lean - 90, 0), material=handle, sides=8, radius_top=0.013)
    return kit.join([stump, top, haft, head], "SM_ChoppingBlock")
