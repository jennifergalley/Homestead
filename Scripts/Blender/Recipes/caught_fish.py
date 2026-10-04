"""Six original British catch species, not a recoloured common fish mesh.

Usage: Scripts\\Blender\\New-Prop.ps1 caught_fish -Live -BeautySamples 192

Reference morphology (text only; no copied mesh/photo/texture):
https://animaldiversity.org/accounts/Salmo_trutta/ - olive/brown trout, haloed
red spots, large terminal mouth, dorsal and adipose fins.
https://www.fisheries.noaa.gov/species/atlantic-salmon - returning silver adult,
spindle body, relatively small head, dark back and white belly.
https://en.wikipedia.org/wiki/European_perch - deep/humped green body, 5-8
bars, red lower fins. Two distinct dorsals rather than a salmonid adipose.
https://animaldiversity.org/accounts/Cyprinus_carpio/ - deep scaled carp,
large scales and a long dorsal. Four mouth barbels are explicitly authored.
https://en.wikipedia.org/wiki/Atlantic_mackerel - slender blue/silver body,
upper wavy stripes, distant dorsals, five upper and lower finlets, forked tail.
https://www.nw-ifca.gov.uk/managing-sustainable-fisheries/species/fish/european-seabass/
- large head/mouth, silver scales, two dorsals, 8-9 leading spines.

Representative catch lengths are an art choice, not weight/size gameplay:
trout 34cm, salmon 62cm, perch 30cm, carp 42cm, mackerel 36cm, bass 46cm.
Meters/Z-up/-Y nose; bottom-centre pivots. Each species has independently
authored body, mouth, tail and fin proportions. Newly shared anatomy/material
helpers generate original geometry; no prior project fish or food asset is used.

Close-up proportion/skin studies (public-domain photos, observation only):
https://commons.wikimedia.org/wiki/File:Brown_Trout_19_10_08.JPG
Burnopfielder1, PD-self; https://commons.wikimedia.org/wiki/File:Perca_fluviatilis_2008_G1.jpg
George Chernilevsky, PD-self. Neither image is a texture or shipped asset.
"""
import math

import bpy
import homestead_materials as materials
from mathutils import Vector

NAME = "CaughtFish"
DESCRIPTION = "Six original anatomically distinct river, lake and coastal catches."
PROVENANCE = "Original parametric anatomy and procedural PBR; cited text and PD-self photos for observation only."
COLLISION = "none"
TRIANGLE_BUDGET = 65000
BAKE = {"size": 4096, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 78, 28), "meshes": {}}
BODY_RINGS = 201
BODY_SIDES = 80
ORAL_LINING_COLUMNS = 11
ROSTRUM_ROUNDING = .004
PROFILE_T = (0, 0.035, 0.075, 0.12, 0.18, 0.25, 0.36, 0.50, 0.65, 0.76, 0.85, 0.90)
FISH = (
    dict(key="RiverTrout", species="Salmo trutta", length=0.34, pattern="trout",
         width=(.014, .024, .034, .043, .054, .058, .058, .051, .037, .022, .011, .009),
         top=(.017, .031, .049, .064, .080, .097, .103, .090, .065, .041, .021, .015),
         bottom=(.018, .030, .041, .053, .071, .088, .090, .079, .055, .030, .017, .014),
         back=(.040, .047, .026), flank=(.29, .28, .19), belly=(.37, .37, .29),
         fin=(.067, .074, .050), eye=(.33, .22, .065), eye_u=.095, eye_size=.014,
         dorsals=((.35, .52, .067, 14),), adipose=(.74, .80, .026),
         tail_height=.090, tail_fork=.988, mouth_end=.135, mouth_gap=.020, scales=140),
    dict(key="RiverSalmon", species="Salmo salar", length=0.62, pattern="salmon",
         width=(.009, .016, .026, .035, .045, .054, .055, .046, .033, .021, .012, .008),
         top=(.014, .024, .039, .054, .068, .084, .095, .083, .061, .037, .020, .015),
         bottom=(.018, .027, .037, .051, .066, .080, .086, .074, .053, .031, .018, .014),
         back=(.030, .043, .041), flank=(.37, .40, .39), belly=(.50, .51, .47),
         fin=(.091, .102, .073), eye=(.29, .27, .13), eye_u=.096, eye_size=.011,
         dorsals=((.36, .51, .10, 13),), adipose=(.75, .80, .025),
         tail_height=.12, tail_fork=.945, mouth_end=.151, mouth_gap=.013, scales=155),
    dict(key="LakePerch", species="Perca fluviatilis", length=0.30, pattern="perch",
         width=(.012, .025, .039, .050, .060, .067, .065, .056, .042, .028, .014, .010),
         top=(.016, .039, .063, .078, .106, .151, .158, .141, .095, .054, .025, .016),
         bottom=(.028, .046, .059, .071, .087, .114, .123, .107, .079, .043, .021, .015),
         back=(.032, .045, .021), flank=(.28, .30, .21), belly=(.38, .38, .30),
         fin=(.27, .048, .016), eye=(.41, .27, .025), eye_u=.105, eye_size=.019,
         dorsals=((.29, .55, .123, 14), (.59, .76, .090, 13)), adipose=None,
         tail_height=.129, tail_fork=.947, mouth_end=.112, mouth_gap=.016, scales=78),
    dict(key="LakeCarp", species="Cyprinus carpio", length=0.42, pattern="carp",
         width=(.015, .029, .042, .054, .072, .090, .096, .087, .059, .038, .019, .012),
         top=(.023, .039, .060, .081, .118, .161, .183, .166, .122, .068, .031, .019),
         bottom=(.037, .052, .067, .081, .098, .124, .135, .122, .089, .053, .025, .018),
         back=(.082, .072, .032), flank=(.30, .22, .085), belly=(.42, .35, .20),
         fin=(.18, .105, .042), eye=(.32, .18, .040), eye_u=.10, eye_size=.012,
         dorsals=((.29, .74, .080, 20),), adipose=None,
         tail_height=.147, tail_fork=.923, mouth_end=.075, mouth_gap=.017, scales=46),
    dict(key="SeaMackerel", species="Scomber scombrus", length=0.36, pattern="mackerel",
         width=(.006, .014, .024, .033, .048, .058, .061, .051, .033, .017, .008, .006),
         top=(.009, .023, .037, .050, .062, .071, .070, .060, .040, .024, .013, .010),
         bottom=(.013, .026, .040, .051, .063, .066, .064, .054, .037, .022, .012, .009),
         back=(.020, .092, .101), flank=(.37, .43, .43), belly=(.52, .52, .46),
         fin=(.12, .14, .11), eye=(.31, .28, .12), eye_u=.108, eye_size=.018,
         dorsals=((.31, .46, .087, 11), (.64, .73, .045, 10)), adipose=None,
         tail_height=.137, tail_fork=.918, mouth_end=.13, mouth_gap=.010, scales=130),
    dict(key="SeaBass", species="Dicentrarchus labrax", length=0.46, pattern="bass",
         width=(.012, .029, .041, .049, .059, .068, .071, .061, .043, .028, .014, .011),
         top=(.018, .038, .056, .070, .087, .105, .118, .104, .077, .047, .024, .018),
         bottom=(.034, .046, .057, .066, .077, .089, .098, .087, .064, .037, .020, .016),
         back=(.040, .060, .062), flank=(.36, .40, .40), belly=(.48, .49, .43),
         fin=(.095, .12, .115), eye=(.32, .29, .11), eye_u=.102, eye_size=.015,
         dorsals=((.30, .48, .095, 9), (.53, .72, .075, 13)), adipose=None,
         tail_height=.121, tail_fork=.944, mouth_end=.145, mouth_gap=.020, scales=94),
)
REPORT = {"species": [{"item": f["key"], "scientific_name": f["species"],
                       "representative_length_cm": f["length"] * 100} for f in FISH],
          "wet_fish": {"coat_weight": materials.FISH_COAT_WEIGHT,
                       "coat_roughness": materials.FISH_COAT_ROUGHNESS}}


def profile(values: tuple, t: float) -> float:
    """Monotone cubic interpolation avoids faceted stations and negative radii."""
    index = min(len(PROFILE_T) - 2, max(0, next(
        (i - 1 for i, point in enumerate(PROFILE_T) if point > t), len(PROFILE_T) - 2)))
    slopes = [(b - a) / (tb - ta)
              for a, b, ta, tb in zip(values, values[1:], PROFILE_T, PROFILE_T[1:])]

    def tangent(i):
        if i == 0:
            return slopes[0]
        if i == len(values) - 1:
            return slopes[-1]
        a, b = slopes[i - 1], slopes[i]
        return 2 * a * b / (a + b) if a * b > 0 else 0.0

    span = PROFILE_T[index + 1] - PROFILE_T[index]
    u = min(1, max(0, (t - PROFILE_T[index]) / span))
    return ((2*u**3 - 3*u**2 + 1) * values[index]
            + (u**3 - 2*u**2 + u) * span * tangent(index)
            + (-2*u**3 + 3*u**2) * values[index + 1]
            + (u**3 - u**2) * span * tangent(index + 1))


def surface(fish: dict, u: float, angle: float, lift: float = 0) -> Vector:
    length = fish["length"]
    s = math.sin(angle)
    muzzle = .55 + .45 * math.sqrt(max(0, 1 - (1 - min(1, u / .028)) ** 2))
    z = profile(fish["top"] if s >= 0 else fish["bottom"], u) * s * muzzle
    centre_x = .005 * math.sin(math.pi * u) ** 2
    posterior = .187 + .065 * math.exp(-((s - .20) / .50) ** 2)
    gate = min(1, max(0, (abs(math.cos(angle)) - .35) / .30))
    plate_edge = min(1, max(0, (posterior - u) / .009))
    plate_front = min(1, max(0, (u - .13) / .040))
    operculum = (.0038 * plate_front * plate_edge * plate_edge * (3 - 2 * plate_edge)
                 - .0017 * math.exp(-((u - posterior) / .003) ** 2)) * gate * length
    bone = 0
    if u < .28:
        eye_angle = .35 if math.cos(angle) >= 0 else math.pi - .35
        delta = math.atan2(math.sin(angle - eye_angle), math.cos(angle - eye_angle))
        arc = max(profile(fish["top"], fish["eye_u"]), .035) * delta
        orbit = math.hypot((u - fish["eye_u"]) / fish["eye_size"], arc / fish["eye_size"])
        preoperculum = .137 + .068 * math.exp(-((s + .10) / .48) ** 2)
        bone = (.0032 * math.exp(-((orbit - 1.12) / .23) ** 2)
                + .0033 * math.exp(-((u - preoperculum) / .024) ** 2)
                * math.exp(-((s + .15) / .42) ** 2)
                - .0008 * math.exp(-((u - preoperculum + .024) / .005) ** 2)
                * math.exp(-((s + .05) / .48) ** 2)
                + .0035 * math.exp(-((u - .112) / .033) ** 2)
                * math.exp(-((s + .17) / .08) ** 2)
                + .0038 * math.exp(-((u - .095) / .055) ** 2)
                * math.exp(-((s + .62) / .10) ** 2)) * gate * length
    head_blend = max(0, min(1, (.285 - u) / .10))
    head_blend = head_blend * head_blend * (3 - 2 * head_blend)
    lateral = math.copysign(abs(math.cos(angle)) ** (1 - .35 * head_blend), math.cos(angle))
    return Vector((length * (centre_x + profile(fish["width"], u) * lateral * muzzle),
                   length * (u - .5), length * z)) + Vector(
                       (math.cos(angle), 0, math.sin(angle))) * (lift + operculum + bone)


def oral_angle(fish: dict, u: float) -> float:
    t = min(1, u / fish["mouth_end"])
    z = -.003 - .014 * t ** 1.3
    angle = math.asin(max(-.8, z / profile(fish["bottom"], min(u, fish["mouth_end"]))))
    fade = min(1, max(0, (u - fish["mouth_end"]) / .045))
    return angle * (1 - fade * fade * (3 - 2 * fade))


def oral_gap(fish: dict, u: float) -> float:
    t = min(1, max(0, u / fish["mouth_end"]))
    return fish["length"] * fish["mouth_gap"] * (1 - t * t * (3 - 2 * t))


def jaw_surface(fish: dict, u: float, angle: float, upper: bool) -> Vector:
    point = surface(fish, u, angle)
    nose = max(0, 1 - u / .028)
    cut = oral_angle(fish, u)
    distance = min(abs(math.atan2(math.sin(angle - cut), math.cos(angle - cut))),
                   abs(math.atan2(math.sin(angle - math.pi + cut), math.cos(angle - math.pi + cut))))
    rim = max(nose, math.exp(-(distance / .24) ** 2))
    point.z += oral_gap(fish, u) * rim * (.12 if upper else -.88)
    point.y += fish["length"] * ROSTRUM_ROUNDING * max(0, 1 - u / .028) ** 2
    if upper:
        point.y += fish["length"] * .005 * max(0, 1 - u / fish["mouth_end"]) ** 2
    lip = fish["length"] * .0028 * math.exp(-(distance / .075) ** 2)
    lip *= min(1, max(0, (fish["mouth_end"] - u) / .025))
    lip = min(lip, oral_gap(fish, u) * .20)
    point += Vector((math.cos(angle), -.28 * min(1, u / .015), -.55 if upper else .55)).normalized() * lip
    return point


def attribute(obj, name: str, values: list) -> None:
    data = obj.data.attributes.new(name, "FLOAT_VECTOR", "POINT")
    for entry, value in zip(data.data, values):
        entry.vector = value


def body(kit, fish: dict, material, cavity_material):
    vertices, faces, coords = [], [], []
    columns = BODY_SIDES // 2 + 1
    rims = {}
    for upper in (True, False):
        offset = len(vertices)
        for ring in range(BODY_RINGS):
            u = .90 * ring / (BODY_RINGS - 1)
            cut = oral_angle(fish, u)
            start, end = (cut, math.pi - cut) if upper else (math.pi - cut, 2 * math.pi + cut)
            for side in range(columns):
                angle = start + (end - start) * side / (columns - 1)
                vertices.append(jaw_surface(fish, u, angle, upper))
                coords.append((u, math.cos(angle), math.sin(angle)))
            rims[(upper, ring)] = (
                offset + ring * columns + (0 if upper else columns - 1),
                offset + ring * columns + (columns - 1 if upper else 0))
        for ring in range(BODY_RINGS - 1):
            for side in range(columns - 1):
                a = offset + ring * columns + side
                faces.append((a + columns, a + 1 + columns, a + 1, a))
    vertices.append(Vector((0, fish["length"] * .40, 0)))
    coords.append((.90, 0, 0))
    centre = len(vertices) - 1
    for upper in (True, False):
        offset = 0 if upper else BODY_RINGS * columns
        for side in range(columns - 1):
            a = offset + (BODY_RINGS - 1) * columns + side
            faces.append((centre, a + 1, a))
        right, left = rims[(upper, 0)]
        centre_point = (vertices[right] + vertices[left]) * .5
        previous = list(range(offset, offset + columns))
        for layer in range(1, 5):
            t = layer / 5
            current = []
            for side in range(columns):
                point = centre_point.lerp(vertices[offset + side], 1 - t)
                point.y -= fish["length"] * ROSTRUM_ROUNDING * math.sqrt(1 - (1 - t) ** 2)
                current.append(len(vertices))
                vertices.append(point)
                coords.append((0, math.cos(oral_angle(fish, 0) + side * math.pi / (columns - 1)) * (1 - t),
                               math.sin(oral_angle(fish, 0))))
            for side in range(columns - 1):
                faces.append((previous[side], previous[side + 1], current[side + 1], current[side]))
            previous = current
        nose_center = len(vertices)
        vertices.append(centre_point - Vector((0, fish["length"] * ROSTRUM_ROUNDING, 0)))
        coords.append((0, 0, math.sin(oral_angle(fish, 0))))
        for side in range(columns - 1):
            faces.append((nose_center, previous[side], previous[side + 1]))
    cavity_start = len(faces)
    head_rings = [ring for ring in range(BODY_RINGS)
                  if .90 * ring / (BODY_RINGS - 1) <= fish["mouth_end"]]
    linings = []
    for side in range(2):
        side_start = len(faces)
        wall = []
        for ring in head_rings:
            top, bottom = rims[(True, ring)][side], rims[(False, ring)][side]
            inner_top = len(vertices)
            inward = Vector(((-1 if side == 0 else 1) * fish["length"] * .006, 0, 0))
            vertices.extend((vertices[top] + inward, vertices[bottom] + inward))
            coords.extend((coords[top], coords[bottom]))
            wall.append((top, bottom, inner_top, inner_top + 1))
        for first, second in zip(wall, wall[1:]):
            top, bottom, inside_top, inside_bottom = first
            next_top, next_bottom, next_inside_top, next_inside_bottom = second
            faces.extend(((top, next_top, next_inside_top, inside_top),
                          (bottom, inside_bottom, next_inside_bottom, next_bottom),
                          (inside_top, next_inside_top, next_inside_bottom, inside_bottom)))
        if wall:
            top, bottom, inside_top, inside_bottom = wall[0]
            faces.append((top, inside_top, inside_bottom, bottom))
        if side == 0:
            faces[side_start:] = [tuple(reversed(face)) for face in faces[side_start:]]
        linings.append(wall)
    lining_columns = ORAL_LINING_COLUMNS
    lining_start = len(vertices)
    for upper in (True, False):
        offset = len(vertices)
        for ring, (right, left) in enumerate(zip(*linings)):
            a, b = vertices[right[2 if upper else 3]], vertices[left[2 if upper else 3]]
            u = .90 * head_rings[ring] / (BODY_RINGS - 1)
            for column in range(lining_columns):
                t = column / (lining_columns - 1)
                point = a.lerp(b, t)
                fullness = oral_gap(fish, u) * .20 * math.sin(math.pi * t) ** 2
                point.z += fullness * (-1 if upper else 1)
                vertices.append(point)
                coords.append((u, 1 - 2 * t, math.sin(oral_angle(fish, u))))
        for ring in range(len(head_rings) - 1):
            for column in range(lining_columns - 1):
                a = offset + ring * lining_columns + column
                quad = (a, a + 1, a + 1 + lining_columns, a + lining_columns)
                faces.append(quad if upper else tuple(reversed(quad)))
    obj = kit.mesh("AnatomicalBody", vertices, faces, material)
    obj.data.materials.append(cavity_material)
    for polygon in obj.data.polygons[cavity_start:]:
        polygon.material_index = 1
    attribute(obj, "fishcoord", coords)
    obj["oral_lining_start"] = lining_start
    obj["oral_lining_columns"] = lining_columns
    obj["oral_lining_rings"] = len(head_rings)
    girth = lambda u: math.sqrt((profile(fish["width"], u) ** 2
                                + ((profile(fish["top"], u) + profile(fish["bottom"], u)) / 2) ** 2) / 2)
    maximum_girth = max(girth(t) for t in PROFILE_T)
    girths = {u: girth(u) / maximum_girth for u, _, _ in coords}
    attribute(obj, "fishscale", [(u, math.atan2(up, side) * girths[u], girths[u])
                                 for u, side, up in coords])
    # The open oral sheets need explicit winding; volume-based repair can invert them.
    return obj


def membrane(kit, name: str, base, edge, rays: int, material, length: float,
             rib_material=None) -> list:
    columns, spans = rays * 3, 6
    adipose = name == "Adipose"
    thickness = max(.00035, length * (.006 if adipose else .0009))
    vertices, faces, coords = [], [], []

    def normal_at(s):
        tangent = base(min(1, s + .002)) - base(max(0, s - .002))
        return tangent.cross(edge(s) - base(s)).normalized()

    for layer in (-1, 1):
        for column in range(columns + 1):
            s = column / columns
            root, tip = base(s), edge(s)
            normal = normal_at(s)
            for step in range(spans + 1):
                v = step / spans
                depth = .10 if "Spiny" in name else .018
                scallop = 1 - depth * math.sin(math.pi * s * rays) ** 2 * v
                point = root.lerp(tip, v * scallop)
                bow = length * .0025 * math.sin(math.pi * v) * math.sin(math.pi * s)
                local_thickness = thickness * (
                    .10 + .90 * math.sin(math.pi * v) * math.sin(math.pi * s) if adipose
                    else (.30 + .70 * (1 - v) ** .60) * (.75 + .25 * math.sin(math.pi * s)))
                point += normal * (layer * local_thickness * .5 + bow)
                vertices.append(point)
                coords.append((s, v, rays))
    sheet = (columns + 1) * (spans + 1)
    for layer in range(2):
        offset = layer * sheet
        for column in range(columns):
            for step in range(spans):
                a = offset + column * (spans + 1) + step
                quad = (a, a + spans + 1, a + spans + 2, a + 1)
                faces.append(quad if layer else tuple(reversed(quad)))
    perimeter = ([column * (spans + 1) for column in range(columns + 1)]
                 + [columns * (spans + 1) + step for step in range(1, spans + 1)]
                 + [column * (spans + 1) + spans for column in range(columns - 1, -1, -1)]
                 + [step for step in range(spans - 1, 0, -1)])
    for a, b in zip(perimeter, perimeter[1:] + perimeter[:1]):
        faces.append((a, b, b + sheet, a + sheet))
    skin = kit.mesh(name, vertices, faces, material)
    attribute(skin, "fincoord", coords)
    kit.recalc_normals(skin)
    parts = [skin]
    if rib_material is not None:
        for ray in range(1, rays):
            s = ray / rays
            root, tip = base(s), edge(s)
            normal = normal_at(s)
            points = [root.lerp(tip, step / 7) + normal * (
                length * .0025 * math.sin(math.pi * step / 7) * math.sin(math.pi * s))
                for step in range(8)]
            radius = length * (.00065 if "Spiny" in name else .00045)
            obj = kit.tube(name + "Ray", points, sides=12, material=rib_material,
                           radii=[radius * (1 - .78 * step / 7) for step in range(8)])
            coords = [(s, step / 7, rays) for step in range(8) for _ in range(12)]
            coords.extend(((s, 0, rays), (s, 1, rays)))
            attribute(obj, "fincoord", coords)
            parts.append(obj)
    return parts


def dorsal(kit, fish: dict, spec: tuple, material, index: int) -> list:
    start, end, height, rays = spec
    length = fish["length"]
    base = lambda s: surface(fish, start + (end - start) * s, math.pi / 2)

    def edge(s):
        lift = height * max(.018, math.sin(math.pi * s) ** .55) * (1.23 - .62 * s)
        point = base(s) + Vector((length * .009 * math.sin(math.pi*s),
                                  length * .008 * math.sin(math.pi*s), length * lift))
        return point

    spiny = fish["pattern"] in {"perch", "bass", "carp", "mackerel"} and index == 0
    return membrane(kit, ("Spiny" if spiny else "Soft") + "Dorsal", base, edge,
                    rays, material, length, material)


def tail(kit, fish: dict, material) -> list:
    length = fish["length"]

    def base(s):
        return Vector((length * .002, length * (.875 - .5),
                       length * (s * 2 - 1) * .016))

    def edge(s):
        across = s * 2 - 1
        u = fish["tail_fork"] + (1 - fish["tail_fork"]) * abs(across) ** .7
        return Vector((length * (.004 + .012 * math.sin(math.pi * s)),
                       length * (u - .5), length * across * fish["tail_height"]))

    return membrane(kit, "CaudalFan", base, edge, 18, material, length, material)


def paired_fin(kit, fish: dict, material, side: int, pelvic: bool) -> list:
    length = fish["length"]
    start, angle = (.49, -1.05) if pelvic else (.245, -.25)

    def base(s):
        polar = angle - (.12 if pelvic else .15) * s
        return surface(fish, start + (.020 if pelvic else .008) * s,
                       polar if side > 0 else math.pi - polar)

    def edge(s):
        spread = max(.035, math.sin(math.pi * s) ** .60)
        return base(s) + Vector((side * length * (.026 if pelvic else .016) * spread,
                                 length * (.078 if pelvic else .140) * (1 - .25 * s) * spread,
                                 -length * ((.025 + .035 * s) if pelvic else (.015 + .085 * s)) * spread))

    return membrane(kit, "Pelvic" if pelvic else "Pectoral", base, edge,
                    10 if pelvic else 13, material, length, material)


def lower_fin(kit, fish: dict, material, start=.64, end=.77, height=.06, rays=11) -> list:
    length = fish["length"]
    base = lambda s: surface(fish, start + (end - start) * s, -math.pi / 2)
    edge = lambda s: base(s) + Vector((0, length * .012 * math.sin(math.pi * s),
                                       -length * height * max(.025, math.sin(math.pi*s)**.6)))
    return membrane(kit, "AnalFin", base, edge, rays, material, length, material)


def eye(kit, fish: dict, material, side: int):
    radius, columns, rings = fish["length"] * fish["eye_size"], 48, 20
    angle = .35 if side > 0 else math.pi - .35
    u = fish["eye_u"]
    head = lambda station, polar: jaw_surface(fish, station, polar, True)
    along = (head(u + .001, angle) - head(u - .001, angle)) / .002
    around = (head(u, angle + .001) - head(u, angle - .001)) / .002
    vertices, faces, coords = [], [], []
    for ring in range(rings + 1):
        radial = ring / rings
        for column in range(columns):
            theta = 2 * math.pi * column / columns
            du = radius * radial * math.cos(theta) / along.length
            da = radius * radial * math.sin(theta) / around.length
            a = (head(u + du + .001, angle + da)
                 - head(u + du - .001, angle + da))
            b = (head(u + du, angle + da + .001)
                 - head(u + du, angle + da - .001))
            normal = a.cross(b).normalized()
            depth = radius * .28 * (1 - radial * radial) + .00006 - .00012 * radial ** 8
            vertices.append(head(u + du, angle + da) + normal * depth)
            coords.append((math.sqrt(max(0, 1 - radial * radial)),
                           radial * math.cos(theta), radial * math.sin(theta)))
    for ring in range(rings):
        for column in range(columns):
            a, b = ring*columns + column, ring*columns + (column + 1) % columns
            faces.append((a, a + columns, b + columns, b))
    obj = kit.mesh("LenticularEye", vertices, faces, material)
    attribute(obj, "eyecoord", coords)
    kit.recalc_normals(obj)
    return obj


def mouth(kit, fish: dict, lip_material, cavity_material) -> list:
    length, parts = fish["length"], []
    if fish["pattern"] == "carp":
        for side in (-1, 1):
            for short in (False, True):
                u = .037 if short else .07
                start = surface(fish, u, 0 if side > 0 else math.pi)
                span = length * (.024 if short else .043)
                points = [start + Vector((side*span*t*.6, span*t,
                                         -span*(.3*t + .5*t*t))) for t in [i/16 for i in range(17)]]
                parts.append(kit.tube("SmallBarbel" if short else "CornerBarbel", points,
                                      radii=[length*.0015*(1-.8*i/16) for i in range(17)],
                                      sides=12, material=lip_material))
    return parts


def build_species(kit, fish: dict):
    key, length = fish["key"], fish["length"]
    girth = 2 * math.pi * math.sqrt((max(fish["width"]) ** 2
                                    + ((max(fish["top"]) + max(fish["bottom"])) / 2) ** 2) / 2)
    scale_rings = max(8, round(fish["scales"] * girth / 2) * 2)
    skin = kit.mats.fish_skin("M_" + key + "Skin", fish["back"], fish["flank"],
                              fish["belly"], fish["pattern"], fish["scales"], scale_rings,
                              eye_u=fish["eye_u"])
    fin = kit.mats.fish_fin("M_" + key + "Fin", fish["fin"])
    dorsal_color = (.18, .19, .13) if fish["pattern"] == "perch" else tuple(
        .60 * back + .40 * fin_color for back, fin_color in zip(fish["back"], fish["fin"]))
    dorsal_fin = kit.mats.fish_fin("M_" + key + "Dorsal", dorsal_color)
    pectoral_fin = kit.mats.fish_fin("M_" + key + "Pectoral", (.20, .16, .065)) if fish["pattern"] == "perch" else fin
    eye_mat = kit.mats.fish_eye("M_" + key + "Eye", fish["eye"])
    lip = kit.mats.fish_fin("M_" + key + "Lip", tuple(c*.65 for c in fish["flank"]))
    cavity = kit.mats.fish_fin("M_" + key + "Mouth", (.075, .044, .035), ray_detail=False)
    parts = [body(kit, fish, skin, cavity)]
    for index, spec in enumerate(fish["dorsals"]):
        parts.extend(dorsal(kit, fish, spec, dorsal_fin, index))
    if fish["adipose"] is not None:
        start, end, height = fish["adipose"]
        base = lambda s: surface(fish, start + (end-start)*s, math.pi/2)
        edge = lambda s: base(s) + Vector((0, 0, length*height*max(.03, math.sin(math.pi*s)**.6)))
        adipose = kit.mats.fish_fin("M_" + key + "Adipose", fish["back"], ray_detail=False)
        parts.extend(membrane(kit, "Adipose", base, edge, 5, adipose, length))
    parts.extend(tail(kit, fish, fin))
    for side in (-1, 1):
        parts.append(eye(kit, fish, eye_mat, side))
        parts.extend(paired_fin(kit, fish, pectoral_fin, side, False))
        parts.extend(paired_fin(kit, fish, fin, side, True))
    parts.extend(lower_fin(kit, fish, fin,
                           start=.665 if fish["pattern"] == "mackerel" else .64,
                           end=.745 if fish["pattern"] == "mackerel" else .77,
                           height=.037 if fish["pattern"] == "mackerel" else .065))
    if fish["pattern"] == "mackerel":
        for index in range(5):
            start = .765 + index * .026
            parts.extend(dorsal(kit, fish, (start, start+.016, .017, 3), dorsal_fin, index+2))
            parts.extend(lower_fin(kit, fish, fin, start, start+.016, .016, 3))
    parts.extend(mouth(kit, fish, lip, cavity))
    obj = kit.join(parts, "SM_" + key, pivot="base", smooth_angle=65, unwrap=True)
    # Welding the thin fin tips can move the lowest vertex after the kit sets its pivot.
    settle = min(vertex.co.z for vertex in obj.data.vertices)
    for vertex in obj.data.vertices:
        vertex.co.z -= settle
    shift = list(obj["homestead_shift"])
    shift[2] += settle
    obj["homestead_shift"] = shift
    obj.data.update()
    obj["fish_item"] = key
    obj["fish_species"] = fish["species"]
    obj["fish_original"] = True
    return obj


def build(kit) -> list:
    return [build_species(kit, fish) for fish in FISH]


def after_bake(kit, obj) -> None:
    material = obj.material_slots[0].material
    bsdf = next(node for node in material.node_tree.nodes if node.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Coat Weight"].default_value = materials.FISH_COAT_WEIGHT
    bsdf.inputs["Coat Roughness"].default_value = materials.FISH_COAT_ROUGHNESS


for fish in FISH:
    length = fish["length"]
    BEAUTY["meshes"]["SM_" + fish["key"]] = {
        "focus": [0, length * (.19 - .5), 0],
        "detail_distance": length * 1.0,
    }
