"""Skeleton of a yearling mule-deer buck (~60 kg) lying on its right side, as signed-
distance bones (skull, jaw, vertebrae, pelvis, scapula, limb bones, hooves) and swept
ribs and antlers.

World frame (before the recipe centres it): +X toward the tail, +Y dorsal (the back),
-Y ventral (where the legs lie), +Z is the animal's left side (facing the sky); the
ground is z = 0. Each bone is authored in its own frame, meshed, tagged with those
part-local coordinates (``pcoord``) and moved into place.

Reference dimensions (mule deer, Odocoileus hemionus): skull condylobasal length
~26 cm, 11 cm across the orbits; 7 cervical, 13 thoracic, 6 lumbar vertebrae, a
4-piece sacrum; thoracic spines up to 9 cm at the withers; lumbar transverse processes
5-6 cm; 13 rib pairs, the longest ~30 cm; humerus 18.5, radius 21, metacarpus 19,
femur 23, tibia 27, metatarsus 25 cm; hoof toes ~5 cm.
"""
import math

import numpy as np

import homestead_sdf as sdf
from deer import common as C

rng = np.random.default_rng(20260927)

# ------------------------------------------------------------------ spine

SPINE_POINTS = [(-0.425, 0.05, 0.05), (-0.43, 0.13, 0.05), (-0.395, 0.19, 0.048), (-0.32, 0.215, 0.045),
                (-0.15, 0.212, 0.04), (0.03, 0.2, 0.045), (0.2, 0.19, 0.056), (0.33, 0.18, 0.062),
                (0.42, 0.17, 0.056), (0.49, 0.15, 0.04), (0.57, 0.12, 0.025)]
SPINE = C.Curve(C.catmull_rom(SPINE_POINTS, 600))

# (name, kind, centrum length); gaps of 3 mm between them.
VERTEBRAE = ([("Atlas", "atlas", 0.030), ("Axis", "axis", 0.062)]
             + [(f"C{i}", "cervical", L) for i, L in zip(range(3, 8), (0.048, 0.046, 0.043, 0.039, 0.033))]
             + [(f"T{i}", "thoracic", 0.0255 + 0.0004 * i) for i in range(1, 14)]
             + [(f"L{i}", "lumbar", 0.034 + 0.0004 * i) for i in range(1, 7)]
             + [("Sacrum", "sacrum", 0.105)]
             + [(f"Cd{i}", "caudal", L) for i, L in zip(range(1, 6), (0.021, 0.020, 0.019, 0.017, 0.015))])
GAP = 0.003


def spine_frame(s):
    """(tangent toward the tail, dorsal, left-lateral) at arclength s."""
    t = SPINE.tangent(s)
    d = C.unit(np.cross(C.ZUP, t))
    return np.stack([t, d, np.cross(t, d)], axis=1)


def vertebra_layout():
    out, s = [], 0.0
    for name, kind, length in VERTEBRAE:
        centre = s + length / 2
        out.append((name, kind, length, centre))
        s += length + GAP
    return out


LAYOUT = vertebra_layout()
S_OF = {name: s for name, _, _, s in LAYOUT}


# ---------------------------------------------------------------- helpers

def _rot(axis, degrees):
    return sdf.axis_angle(axis, math.radians(degrees))


def tapered(x0, x1, profile, rounding=0.8):
    """Rounded box along X whose half width, half height and centre height follow
    ``profile`` = [(u, half_w, half_h, centre_z), ...] for u in 0..1 (u = (x-x0)/(x1-x0))."""
    prof = np.asarray(profile, dtype=np.float64)

    def dist(p):
        u = np.clip((p[:, 0] - x0) / (x1 - x0), 0.0, 1.0)
        w = np.interp(u, prof[:, 0], prof[:, 1])
        h = np.interp(u, prof[:, 0], prof[:, 2])
        c = np.interp(u, prof[:, 0], prof[:, 3])
        r = np.minimum(w, h) * rounding
        qy = np.abs(p[:, 1]) - w + r
        qz = np.abs(p[:, 2] - c) - h + r
        d2 = np.hypot(np.maximum(qy, 0), np.maximum(qz, 0)) + np.minimum(np.maximum(qy, qz), 0) - r
        dx = np.maximum(x0 - p[:, 0], p[:, 0] - x1)
        return np.hypot(np.maximum(d2, 0), np.maximum(dx, 0)) + np.minimum(np.maximum(d2, dx), 0)
    w, h, c = prof[:, 1].max(), prof[:, 2].max(), prof[:, 3]
    return sdf._primitive(dist, (x0, -w, c.min() - h), (x1, w, c.max() + h))


def mirrored(fn):
    """Union of ``fn(side)`` for both sides (side = +1 left, -1 right)."""
    return [fn(1.0), fn(-1.0)]


# ------------------------------------------------------------------ skull

def skull_node():
    """Skull in its own frame: +X forward along the snout, +Y left, +Z dorsal, origin
    between the occipital condyles."""
    braincase = sdf.smooth_union([
        sdf.ellipsoid((0.052, 0, 0.034), (0.056, 0.035, 0.034)),
        sdf.ellipsoid((0.02, 0, 0.032), (0.022, 0.033, 0.034)),
        sdf.ellipsoid((0.03, 0, 0.0), (0.032, 0.013, 0.011)),                     # basioccipital
        sdf.ellipsoid((0.014, 0, 0.058), (0.007, 0.03, 0.007)),                   # nuchal crest
    ] + mirrored(lambda s: sdf.ellipsoid((0.001, s * 0.016, 0.005), (0.008, 0.008, 0.012), _rot((0, 1, 0), 20)))
      + mirrored(lambda s: sdf.round_cone((0.008, s * 0.028, 0.0), (0.014, s * 0.034, -0.028), 0.0052, 0.0026))
      + mirrored(lambda s: sdf.ellipsoid((0.031, s * 0.021, -0.006), (0.012, 0.009, 0.0105))), k=0.006)
    frontal = sdf.smooth_union([
        sdf.ellipsoid((0.104, 0, 0.043), (0.058, 0.049, 0.022)),
        sdf.ellipsoid((0.085, 0, 0.052), (0.03, 0.035, 0.012)),
    ] + mirrored(lambda s: sdf.ellipsoid((0.12, s * 0.047, 0.012), (0.016, 0.008, 0.013)))           # malar
      + mirrored(lambda s: sdf.round_cone((0.119, s * 0.051, 0.006), (0.064, s * 0.041, 0.002), 0.0055, 0.0045))
      + mirrored(lambda s: sdf.round_cone((0.088, s * 0.028, 0.057), (0.083, s * 0.035, 0.074), 0.0105, 0.0098)),
      k=0.007)
    rostrum = sdf.smooth_union([
        tapered(0.118, 0.247, [(0.0, 0.037, 0.027, 0.022), (0.35, 0.03, 0.022, 0.017),
                               (0.7, 0.021, 0.016, 0.011), (1.0, 0.013, 0.01, 0.004)]),
        tapered(0.125, 0.232, [(0.0, 0.02, 0.006, 0.045), (0.5, 0.017, 0.0055, 0.034),
                               (1.0, 0.011, 0.004, 0.021)], rounding=0.9),          # nasal bones
        tapered(0.085, 0.19, [(0.0, 0.03, 0.009, 0.0), (0.6, 0.029, 0.008, 0.001),
                              (1.0, 0.024, 0.007, 0.004)], rounding=0.9),            # alveolar ridge
    ] + mirrored(lambda s: sdf.round_cone((0.205, s * 0.012, 0.001), (0.264, s * 0.0055, 0.003), 0.0062, 0.0036)),
        k=0.008)
    skull = sdf.smooth_union([braincase, frontal, rostrum], k=0.012)
    cutters = [sdf.ellipsoid((-0.005, 0, 0.012), (0.012, 0.0105, 0.009)),              # foramen magnum
               sdf.ellipsoid((0.238, 0, 0.0125), (0.034, 0.05, 0.0068)),                # naso-incisive notch
               sdf.ellipsoid((0.214, 0, 0.012), (0.05, 0.0085, 0.0085))]                # nasal cavity
    cutters += mirrored(lambda s: sdf.sphere((0.115, s * 0.059, 0.029), 0.0212))          # orbits
    cutters += mirrored(lambda s: sdf.ellipsoid((0.143, s * 0.037, 0.033), (0.013, 0.02, 0.0078)))  # ethmoid gap
    cutters += mirrored(lambda s: sdf.sphere((0.172, s * 0.031, 0.012), 0.0032))          # infraorbital foramen
    skull = sdf.subtract(skull, cutters, k=0.0025)
    skull = sdf.subtract(skull, mirrored(lambda s: sdf.sphere((0.133, s * 0.047, 0.017), 0.0085)), k=0.004)
    rims = mirrored(lambda s: sdf.torus((0.113, s * 0.0452, 0.029), 0.0196, 0.0028,
                                        sdf.rotation(axis_z=(0.3, s * 1.0, 0.22))))
    skull = sdf.smooth_union([skull] + rims, k=0.0085)
    # Sutures (shallow grooves), the lacrimal pits in front of the orbits, the temporal
    # fossae behind them and a frontal ridge between the pedicles: without these the
    # skull reads as a smooth cast.
    grooves = [sdf.round_cone((0.068, -0.03, 0.061), (0.068, 0.03, 0.061), 0.0012),               # coronal
               sdf.round_cone((0.068, 0.0, 0.061), (0.15, 0.0, 0.05), 0.0011),                      # frontal
               sdf.round_cone((0.15, -0.018, 0.047), (0.15, 0.018, 0.047), 0.001),                   # fronto-nasal
               sdf.round_cone((0.15, 0.0, 0.049), (0.232, 0.0, 0.024), 0.0009)]                      # internasal
    grooves += mirrored(lambda s: sdf.round_cone((0.03, s * 0.036, 0.04), (0.066, s * 0.034, 0.061), 0.0011))
    grooves += mirrored(lambda s: sdf.round_cone((0.152, s * 0.022, 0.044), (0.205, s * 0.024, 0.02), 0.0009))
    pits = mirrored(lambda s: sdf.ellipsoid((0.142, s * 0.05, 0.024), (0.009, 0.006, 0.0065)))
    pits += mirrored(lambda s: sdf.ellipsoid((0.05, s * 0.04, 0.02), (0.03, 0.009, 0.016)))
    skull = sdf.subtract(skull, grooves, k=0.0008)
    skull = sdf.subtract(skull, pits, k=0.004)
    ridge = sdf.round_cone((0.07, 0.0, 0.063), (0.1, 0.0, 0.063), 0.0035, 0.0025)
    skull = sdf.smooth_union([skull, ridge], k=0.006)
    return sdf.displaced(skull, lambda p: 0.00035 * sdf.fbm(p * 520.0, 2, 3), 0.00035)


UPPER_TEETH_X = [0.1, 0.1135, 0.127, 0.1395, 0.1505, 0.1605, 0.1695]


def upper_teeth_node():
    teeth = []
    for side in (1.0, -1.0):
        for i in range(6):
            x0, x1 = UPPER_TEETH_X[i], UPPER_TEETH_X[i + 1]
            molar = i < 3
            half = ((x1 - x0) / 2 - 0.0004, 0.0062 if molar else 0.0052, 0.0062)
            c = ((x0 + x1) / 2, side * (0.0255 - 0.0006 * i), -0.005)
            tooth = sdf.box(c, half, rounding=0.0034)
            cusps = [sdf.ellipsoid((c[0] + dx, c[1] + side * 0.0015, c[2] - half[2] - 0.0006),
                                   (half[0] * 0.42, 0.0022, 0.0022)) for dx in ((-0.0032, 0.0032) if molar else (0.0,))]
            teeth.append(sdf.subtract(tooth, cusps, k=0.001))
    return sdf.union(teeth)


def mandible_node():
    """Left hemi-mandible: +X forward, +Y lateral (outer face), +Z up; origin at the angle."""
    body = tapered(0.035, 0.205, [(0.0, 0.0058, 0.018, 0.014), (0.45, 0.0055, 0.015, 0.014),
                                   (0.75, 0.0048, 0.0105, 0.015), (1.0, 0.004, 0.0075, 0.016)], rounding=0.9)
    ramus = sdf.prism([(0.0, -0.002), (0.05, 0.0), (0.044, 0.07), (0.032, 0.1), (0.012, 0.09), (-0.004, 0.035)],
                      0.0032, rotation=sdf.rotation(axis_x=(1, 0, 0), axis_y=(0, 0, 1)), rounding=0.002)
    parts = [body, ramus,
             sdf.ellipsoid((0.008, 0.0, 0.087), (0.007, 0.0095, 0.005)),                  # condyle
             sdf.round_cone((0.034, 0, 0.095), (0.044, 0, 0.114), 0.004, 0.0022),          # coronoid
             sdf.ellipsoid((0.012, 0.0, 0.012), (0.018, 0.0052, 0.02))]                     # angle
    jaw = sdf.smooth_union(parts, k=0.006)
    jaw = sdf.subtract(jaw, [sdf.sphere((0.16, 0.009, 0.013), 0.0028)], k=0.001)           # mental foramen
    return jaw


def lower_teeth_node():
    teeth = []
    xs = [0.052, 0.068, 0.083, 0.097, 0.108, 0.118, 0.127]
    for i in range(6):
        x0, x1 = xs[i], xs[i + 1]
        c = ((x0 + x1) / 2, 0.0, 0.034)
        tooth = sdf.box(c, ((x1 - x0) / 2 - 0.0004, 0.0048, 0.006), rounding=0.0032)
        teeth.append(sdf.subtract(tooth, [sdf.ellipsoid((c[0], 0.0015, 0.04), ((x1 - x0) * 0.3, 0.002, 0.002))],
                                  k=0.001))
    for i in range(4):
        y = -0.0015 + 0.0012 * i
        teeth.append(sdf.round_cone((0.196 + 0.002 * i, y, 0.02), (0.222 + 0.001 * i, y, 0.028), 0.0022, 0.0028))
    return sdf.union(teeth)


# -------------------------------------------------------------- vertebrae

def vertebra_node(kind, length, index):
    """Vertebra in its own frame: +X toward the tail, +Y dorsal, +Z left lateral."""
    h = length / 2
    parts, cutters = [], []
    canal_r, rc = 0.0078, 0.0115
    if kind == "atlas":
        ring = sdf.torus((0, 0.009, 0), 0.0135, 0.0052, sdf.rotation(axis_z=(1, 0, 0)))
        parts = [ring] + mirrored(lambda s: sdf.ellipsoid((0.002, 0.005, s * 0.03), (0.0145, 0.0055, 0.026)))
        parts += mirrored(lambda s: sdf.ellipsoid((-0.01, 0.004, s * 0.013), (0.007, 0.011, 0.008)))
        cutters = mirrored(lambda s: sdf.ellipsoid((-0.017, 0.004, s * 0.012), (0.006, 0.009, 0.007)))
        cutters += mirrored(lambda s: sdf.sphere((0.0, 0.007, s * 0.03), 0.0032))
        return sdf.subtract(sdf.smooth_union(parts, k=0.005), cutters, k=0.0015)
    if kind == "caudal":
        scale = length / 0.021
        parts = [sdf.round_cone((-h, 0, 0), (h, 0, 0), 0.0075 * scale, 0.0068 * scale),
                 sdf.ellipsoid((-h + 0.002, 0, 0), (0.004, 0.0082 * scale, 0.0085 * scale)),
                 sdf.ellipsoid((h - 0.002, 0, 0), (0.004, 0.0078 * scale, 0.008 * scale))]
        if index < 2:
            parts.append(sdf.ellipsoid((0, 0.001, 0), (h * 0.7, 0.0045, 0.015 * scale)))
        return sdf.smooth_union(parts, k=0.003)
    if kind == "sacrum":
        parts = [sdf.round_cone((-h, 0, 0), (h, 0.004, 0), 0.0135, 0.0075),
                 sdf.ellipsoid((0.0, 0.004, 0), (h * 0.95, 0.0078, 0.02)),
                 sdf.box((0.0, 0.019, 0), (h * 0.85, 0.009, 0.0024), rounding=0.002)]
        parts += mirrored(lambda s: sdf.ellipsoid((-h + 0.012, 0.004, s * 0.027), (0.016, 0.0095, 0.026)))
        cutters = [sdf.round_cone((-h - 0.01, 0.012, 0), (-h + 0.02, 0.011, 0), 0.0072, 0.005)]
        cutters += [sdf.sphere((x, 0.011, s * 0.012), 0.0031) for x in (-0.03, -0.006, 0.018) for s in (1, -1)]
        return sdf.subtract(sdf.smooth_union(parts, k=0.005), cutters, k=0.001)

    if kind in ("axis", "cervical"):
        rc = 0.0112
    elif kind == "lumbar":
        rc, canal_r = 0.0128, 0.0072
    centrum = [sdf.round_cone((-h, 0, 0), (h, 0, 0), rc, rc * 1.04),
               sdf.sphere((-h + rc * 0.25, 0, 0), rc * 0.95)]
    if kind == "axis":
        centrum.append(sdf.round_cone((-h, 0.002, 0), (-h - 0.014, 0.003, 0), 0.007, 0.0045))       # dens
    arch_y = rc + canal_r * 0.85
    parts = centrum + [sdf.box((0, arch_y, 0), (h * (0.85 if kind != "thoracic" else 0.75),
                                               canal_r + 0.0048, canal_r + 0.0052), rounding=0.004)]
    cutters = [sdf.round_cone((-h - 0.03, arch_y - 0.0005, 0), (h + 0.03, arch_y - 0.0005, 0), canal_r, canal_r),
               sdf.sphere((h + rc * 0.8, 0, 0), rc * 0.95)]
    top = arch_y + canal_r + 0.004

    if kind == "axis":
        parts.append(sdf.box((0.004, top + 0.008, 0), (h * 0.92, 0.01, 0.0026), _rot((0, 0, 1), -8), 0.002))
        parts += mirrored(lambda s: sdf.ellipsoid((-h + 0.004, 0.004, s * 0.014), (0.008, 0.009, 0.0095)))
        parts += mirrored(lambda s: sdf.round_cone((h - 0.01, 0.002, s * 0.011), (h + 0.004, -0.006, s * 0.019),
                                                   0.0042, 0.003))
    elif kind == "cervical":
        c7 = index == 4
        spine_h = 0.04 if c7 else 0.006 + 0.002 * index
        parts.append(sdf.box((0.002, top + spine_h / 2, 0), (0.0045 if c7 else h * 0.3, spine_h / 2 + 0.002, 0.0026),
                             rounding=0.002))
        parts += mirrored(lambda s: sdf.ellipsoid((0.0, -0.003, s * 0.019), (h * 0.8, 0.0115, 0.0058),
                                                  _rot((1, 0, 0), s * 25)))
        for sx in (-1, 1):
            parts += mirrored(lambda s: sdf.round_cone((sx * h * 0.55, top - 0.004, s * 0.009),
                                                       (sx * (h + 0.006), top - 0.002, s * 0.0125), 0.0048, 0.004))
    elif kind == "thoracic":
        i = index                                                     # 0..12
        spine_h = [0.064, 0.084, 0.09, 0.088, 0.081, 0.072, 0.062, 0.052, 0.044, 0.038, 0.033, 0.03, 0.028][i]
        tilt = math.radians([30, 35, 38, 40, 42, 44, 45, 44, 38, 28, 18, 8, 0][i])
        direction = np.array([math.sin(tilt), math.cos(tilt), 0.0])
        base = np.array([0.0, top - 0.004, 0.0])
        frame = sdf.rotation(axis_y=direction, axis_z=(0, 0, 1))
        width = 0.0072 if i < 9 else 0.009
        parts.append(sdf.box(base + direction * spine_h / 2, (width, spine_h / 2 + 0.003, 0.0021), frame, 0.0018))
        parts.append(sdf.sphere(base + direction * spine_h, 0.0042 if i < 8 else 0.0032))
        parts += mirrored(lambda s: sdf.round_cone((0.0, arch_y, s * 0.008), (0.002, arch_y + 0.003, s * 0.023),
                                                   0.0058, 0.0048))
    elif kind == "lumbar":
        parts.append(sdf.box((0.001, top + 0.011, 0), (h * 0.62, 0.013, 0.0028), rounding=0.0025))
        parts += mirrored(lambda s: sdf.box((-0.004, 0.0, s * 0.033), (0.0078, 0.0022, 0.024),
                                            _rot((0, 1, 0), s * 14) @ _rot((1, 0, 0), s * -6), 0.0018))
        for sx in (-1, 1):
            parts += mirrored(lambda s: sdf.round_cone((sx * h * 0.5, top - 0.003, s * 0.008),
                                                       (sx * (h + 0.005), top, s * 0.011), 0.0045, 0.004))
    return sdf.subtract(sdf.smooth_union(parts, k=0.0045), cutters, k=0.0012)


VERTEBRA_TRIS = {"atlas": 620, "axis": 600, "cervical": 460, "thoracic": 400, "lumbar": 460, "sacrum": 800,
                 "caudal": 120}


def vertebra_placement(name, kind, length, s, index):
    frame = spine_frame(s)
    jitter = rng.normal(0, 1, 3)
    roll = {"lumbar": 16.0, "sacrum": 10.0}.get(kind, 4.0) + jitter[0] * 5
    yaw = jitter[1] * 4
    local = _rot((1, 0, 0), roll) @ _rot((0, 0, 1), yaw)
    return frame @ local, SPINE.at(s)


# ------------------------------------------------------------------- ribs

RIB_LENGTH = [0.12, 0.16, 0.2, 0.245, 0.27, 0.29, 0.3, 0.3, 0.29, 0.275, 0.25, 0.215, 0.17]
# Upper (left) ribs, arching over; None = gone (scavenged); ("broken", f) = snapped at f.
UPPER = [1, 1, 1, 1, 1, 1, 1, 1, ("broken", 0.55), 1, None, 1, None]
LOOSE = [("RibLoose12", 12, (0.16, -0.21, 0.0), 150.0), ("RibLoose9", 9, (-0.02, -0.26, 0.0), 35.0)]


def _bezier(p0, p1, p2, p3, n):
    t = np.linspace(0.0, 1.0, n)[:, None]
    return ((1 - t) ** 3) * p0 + 3 * ((1 - t) ** 2) * t * p1 + 3 * (1 - t) * t * t * p2 + t ** 3 * p3


PEAK = [0.07, 0.1, 0.12, 0.138, 0.148, 0.152, 0.15, 0.145, 0.136, 0.125, 0.11, 0.092, 0.07]
# Per-rib (height, reach, caudal lean) jitter: a collapsed chest is never regular.
RIB_JITTER = [(1.0 + 0.1 * a, 1.0 + 0.08 * b, 0.05 * c) for a, b, c in
              np.random.default_rng(1313).normal(0.0, 1.0, (13, 3))]
RIB_JITTER[4] = (0.78, 1.05, 0.06)
RIB_JITTER[7] = (0.84, 0.94, -0.03)


def rib_points(i, upper, cut=1.0, samples=16):
    """World points of rib ``i`` (0-based) from its head at the spine. Upper (left) ribs
    arch up over the collapsed chest and come down to the ground ventrally; lower
    (right) ribs lie pressed flat on the ground under the body."""
    s = S_OF[f"T{i + 1}"] + 0.004
    frame = spine_frame(s)
    t, d, l = frame[:, 0], frame[:, 1], frame[:, 2]
    base = SPINE.at(s) + d * 0.004 + (0.0, 0.0, VSHIFT.get(f"T{i + 1}", 0.0))
    L = RIB_LENGTH[i]
    jitter = RIB_JITTER[i]
    if upper:
        drop = -(base[2] + 0.012 - 0.0055)
        prof = _bezier(np.array([0.0, 0.0]), np.array([0.04 * L, 0.95 * PEAK[i] * jitter[0]]),
                       np.array([0.5 * L, 1.18 * PEAK[i] * jitter[0]]), np.array([0.74 * L * jitter[1], drop]), samples)
    else:
        drop = -(base[2] - 0.012 - 0.004)
        prof = _bezier(np.array([0.0, 0.0]), np.array([0.05 * L, drop * 0.9]),
                       np.array([0.35 * L, drop]), np.array([0.86 * L * jitter[1], drop]), samples)
    pts = []
    for k, (u, h) in enumerate(prof):
        f = k / (samples - 1)
        drift = L * (0.16 + 0.02 * i + jitter[2]) * f ** 1.3
        if upper:
            lateral = l * (0.012 + h)
        else:
            # Pressed flat, the lower ribs keep their curve in the ground plane.
            lateral = -l * 0.012 + C.ZUP * h + t * (0.22 * L * math.sin(math.pi * f) * (1 + jitter[0] - 1.0))
        pts.append(base + lateral - d * u + t * drift)
    pts = np.array(pts)
    pts[:, 2] = np.maximum(pts[:, 2], 0.0055 if upper else 0.0025)
    n = max(3, int(round(samples * cut)))
    return pts[:n], L * cut

def rib(kit, name, i, upper, material, cut=1.0, points=None):
    pts, length = (points, RIB_LENGTH[i] * cut) if points is not None else rib_points(i, upper, cut)
    tangent_axis = spine_frame(S_OF[f"T{i + 1}"])[:, 0]
    width = 0.0072 + 0.0022 * math.sin(math.pi * min(i / 8, 1.0))

    def half_w(t):
        head = math.exp(-(t / 0.05) ** 2)
        return width * (0.75 + 0.35 * t) * (1 - 0.25 * head) + 0.0006

    def half_h(t):
        head = math.exp(-(t / 0.06) ** 2)
        return 0.0028 * (1 - 0.25 * t) + 0.0034 * head

    return C.sweep(kit, name, pts, half_w, half_h, tangent_axis, material, sides=8, power=0.75)


def loose_rib_points(i, centre, heading, samples=16):
    """A rib dragged off and dropped on its side: its arch lies flat on the ground."""
    L = RIB_LENGTH[i]
    prof = _bezier(np.array([0.0, 0.0]), np.array([0.04 * L, 0.95 * PEAK[i]]), np.array([0.5 * L, 1.18 * PEAK[i]]),
                   np.array([0.74 * L, -0.04]), samples)
    a = math.radians(heading)
    rot = np.array([[math.cos(a), -math.sin(a)], [math.sin(a), math.cos(a)]])
    flat = (prof - prof.mean(0)) @ rot.T + np.asarray(centre[:2])
    z = 0.0055 + 0.012 * np.sin(np.linspace(0, math.pi, samples)) ** 2
    return np.column_stack([flat, z]), L

# -------------------------------------------------------------- limb bones

def long_bone(kind, L):
    """Limb bone along +Z from its proximal (0) to distal (L) end; +X cranial."""
    r = {"humerus": 0.0105, "radius": 0.0098, "cannon_f": 0.0088, "femur": 0.0118, "tibia": 0.0102,
         "cannon_h": 0.0086, "p1": 0.0068, "p2": 0.006}[kind]
    parts = [sdf.round_cone((0, 0, L * 0.12), (0, 0, L * 0.88), r * 1.08, r * 0.96)]
    if kind == "humerus":
        parts += [sdf.sphere((-0.009, 0, 0.014), 0.0185), sdf.ellipsoid((0.011, 0.004, 0.006), (0.012, 0.014, 0.017)),
                  sdf.ellipsoid((0.007, 0.006, L * 0.36), (0.006, 0.007, 0.022)),
                  sdf.ellipsoid((0.002, 0, L - 0.012), (0.012, 0.02, 0.011))]
        parts += mirrored(lambda s: sdf.sphere((0.001, s * 0.012, L - 0.011), 0.0125))
        cutters = [sdf.ellipsoid((-0.012, 0, L - 0.02), (0.006, 0.009, 0.012))]
    elif kind == "radius":
        parts[0] = sdf.ellipsoid((0.001, 0, L * 0.5), (0.0078, 0.0125, L * 0.46))
        parts += [sdf.ellipsoid((0.0, 0, 0.007), (0.011, 0.018, 0.008)),
                  sdf.ellipsoid((0.002, 0, L - 0.008), (0.012, 0.018, 0.01)),
                  sdf.round_cone((-0.012, 0, -0.045), (-0.011, 0, 0.012), 0.0085, 0.008),
                  sdf.round_cone((-0.011, 0, 0.012), (-0.008, 0, L * 0.62), 0.006, 0.0022)]
        cutters = []
    elif kind in ("cannon_f", "cannon_h"):
        parts[0] = sdf.ellipsoid((0, 0, L * 0.5), (r * 0.95, r * 1.12, L * 0.47))
        parts += [sdf.ellipsoid((0, 0, 0.007), (0.011, 0.0145, 0.009))]
        parts += mirrored(lambda s: sdf.ellipsoid((0, s * 0.0085, L - 0.006), (0.0098, 0.0078, 0.0085)))
        cutters = [sdf.box((0.009, 0, L * 0.5), (0.0024, 0.0016, L * 0.4), rounding=0.0014)]    # dorsal groove
    elif kind == "femur":
        parts += [sdf.sphere((0.0, -0.021, 0.012), 0.0175), sdf.round_cone((0, -0.02, 0.012), (0, 0, 0.03), 0.01, 0.012),
                  sdf.ellipsoid((0.0, 0.012, 0.0), (0.014, 0.012, 0.022)),
                  sdf.ellipsoid((-0.004, 0, L * 0.35), (0.008, 0.012, 0.02))]
        parts += mirrored(lambda s: sdf.sphere((-0.008, s * 0.014, L - 0.016), 0.0158))
        parts += mirrored(lambda s: sdf.round_cone((0.012, s * 0.009, L - 0.04), (0.015, s * 0.008, L - 0.01), 0.005, 0.006))
        cutters = [sdf.ellipsoid((-0.018, 0, L - 0.012), (0.008, 0.005, 0.02))]
    elif kind == "tibia":
        parts += [sdf.ellipsoid((-0.004, 0, 0.012), (0.02, 0.026, 0.012)),
                  sdf.round_cone((0.02, 0, 0.018), (0.009, 0, 0.1), 0.0065, 0.004),
                  sdf.ellipsoid((0, 0, L - 0.009), (0.013, 0.016, 0.01))]
        cutters = mirrored(lambda s: sdf.ellipsoid((0.0, s * 0.006, L + 0.003), (0.012, 0.0035, 0.006)))
    else:                                                                # phalanges
        parts += [sdf.ellipsoid((0, 0, 0.004), (r * 1.25, r * 1.35, 0.006)),
                  sdf.ellipsoid((0, 0, L - 0.004), (r * 1.15, r * 1.25, 0.005))]
        cutters = []
    bone = sdf.smooth_union(parts, k=0.006)
    return sdf.subtract(bone, cutters, k=0.002) if cutters else bone


def calcaneus_node():
    return sdf.smooth_union([sdf.round_cone((0, 0, 0.0), (-0.052, 0, -0.004), 0.009, 0.0075),
                             sdf.ellipsoid((-0.058, 0, -0.004), (0.009, 0.0085, 0.011)),
                             sdf.ellipsoid((0.006, 0.01, 0.004), (0.012, 0.009, 0.014))], k=0.005)


def hoof_node():
    """Pair of cloven hoof toes (+X toward the toe tips, +Z up, sole at z = 0), dewclaws behind."""
    toes = []
    for s in (1.0, -1.0):
        toe = sdf.smooth_union([sdf.round_cone((0.0, s * 0.011, 0.016), (0.047, s * 0.0065, 0.004), 0.0135, 0.0032),
                                sdf.ellipsoid((0.016, s * 0.01, 0.012), (0.024, 0.0105, 0.014))], k=0.006)
        toe = sdf.intersect(toe, sdf.halfspace((0, 0, 0.0005), (0, 0, -1), (-0.03, -0.04, -0.01), (0.07, 0.04, 0.05)))
        toe = sdf.intersect(toe, sdf.halfspace((0, s * 0.0014, 0), (0, -s, 0), (-0.03, -0.04, -0.01), (0.07, 0.04, 0.05)),
                            k=0.0015)
        toes.append(toe)
        toes.append(sdf.round_cone((-0.028, s * 0.014, 0.032), (-0.036, s * 0.016, 0.022), 0.0052, 0.0028))
    return sdf.union(toes)


# ---------------------------------------------------------- scapula, pelvis

def scapula_node():
    """Left scapula: +X from the glenoid toward the dorsal border, +Y cranial, +Z lateral."""
    blade = sdf.prism([(0.03, -0.018), (0.175, -0.05), (0.19, 0.0), (0.18, 0.052), (0.03, 0.016)], 0.0022,
                      rounding=0.0016)
    blade = sdf.smooth_union([blade, sdf.prism([(0.0, -0.016), (0.06, -0.022), (0.06, 0.02), (0.0, 0.016)], 0.0055,
                                               rounding=0.004)], k=0.012)
    spine = sdf.prism([(0.04, -0.003), (0.17, -0.004), (0.17, 0.004), (0.04, 0.003)], 0.0022,
                      rotation=sdf.rotation(axis_x=(1, 0, 0), axis_z=(0, 1, 0)), rounding=0.0015)
    spine = sdf.transformed(spine, np.eye(3), (0, -0.004, 0.006))
    parts = [blade, spine, sdf.ellipsoid((0.005, 0, 0.0), (0.013, 0.019, 0.013)),
             sdf.sphere((0.004, 0.017, 0.002), 0.0065)]
    bone = sdf.smooth_union(parts, k=0.005)
    return sdf.subtract(bone, [sdf.sphere((-0.013, 0, 0.0), 0.0145)], k=0.002)


def pelvis_node():
    """Both coxae: +X toward the tail, +Y dorsal, +Z left lateral; origin between the
    acetabula."""
    def side(s):
        z = s * 0.045
        # Ilium: a blade that fans out cranio-dorsally from the acetabulum toward the sacrum,
        # its gluteal face turned laterally; tuber coxae and sacrale are its corners.
        normal = np.array([0.0, -0.5 * s, 0.87 * s])
        wing = sdf.rotation(axis_x=(0.87, -0.5, 0.0), axis_z=normal)
        blade = sdf.smooth_union([sdf.ellipsoid((-0.028, 0.0, 0.0), (0.032, 0.012, 0.0072)),
                                  sdf.ellipsoid((-0.075, 0.004, 0.0), (0.036, 0.022, 0.0062)),
                                  sdf.ellipsoid((-0.118, 0.008, 0.0), (0.035, 0.036, 0.0056)),
                                  sdf.ellipsoid((-0.146, -0.028, 0.0), (0.013, 0.013, 0.0085)),
                                  sdf.ellipsoid((-0.14, 0.042, 0.0), (0.011, 0.01, 0.0065))], k=0.012)
        parts = [sdf.sphere((0, 0, z), 0.019),
                 sdf.transformed(blade, wing, (0.0, 0.004, z * 0.97)),
                 sdf.round_cone((0.004, -0.002, z * 0.92), (0.076, 0.002, s * 0.031), 0.0112, 0.0085),
                 sdf.ellipsoid((0.05, -0.012, s * 0.031), (0.042, 0.018, 0.0072)),
                 sdf.ellipsoid((0.086, 0.004, s * 0.04), (0.012, 0.011, 0.0095)),
                 sdf.round_cone((0.006, -0.012, s * 0.036), (0.022, -0.036, s * 0.006), 0.0072, 0.0065),
                 sdf.ellipsoid((0.054, -0.031, s * 0.016), (0.042, 0.0078, 0.02))]
        bone = sdf.smooth_union(parts, k=0.008)
        return sdf.subtract(bone, [sdf.sphere((0, 0, s * 0.062), 0.0165),
                                   sdf.ellipsoid((0.036, -0.024, s * 0.022), (0.02, 0.03, 0.0125))], k=0.002)
    return sdf.union([side(1.0), side(-1.0)])


# ---------------------------------------------------------------- antlers

def antler_path(side=1.0):
    """Forked-horn antler in skull frame, from the burr: main beam and the two tines."""
    beam = [(0.084, 0.035, 0.077), (0.079, 0.047, 0.1), (0.08, 0.062, 0.128), (0.089, 0.075, 0.153),
            (0.103, 0.083, 0.172)]
    front = [(0.101, 0.082, 0.17), (0.121, 0.084, 0.19), (0.139, 0.08, 0.207), (0.152, 0.074, 0.221)]
    back = [(0.101, 0.083, 0.171), (0.107, 0.093, 0.196), (0.109, 0.098, 0.222), (0.106, 0.098, 0.245)]
    flip = np.array([1.0, side, 1.0])
    return [[np.array(p) * flip for p in branch] for branch in (beam, front, back)]


def antler(kit, name, material, side=1.0):
    beam, front, back = antler_path(side)
    parts = []
    radii = ([0.0112, 0.0104, 0.0094, 0.0084, 0.0076], [0.0074, 0.0064, 0.0048, 0.0026],
             [0.0074, 0.0066, 0.0052, 0.0028])
    for label, path, rad in (("Beam", beam, radii[0]), ("Front", front, radii[1]), ("Back", back, radii[2])):
        pts = C.catmull_rom(path, 22)
        rr = np.interp(np.linspace(0, 1, len(pts)), np.linspace(0, 1, len(rad)), rad)
        tube = kit.tube(f"{name}{label}", pts, radii=list(rr), sides=14, material=material)
        tip = label != "Beam"

        def ridges(co, pco, tip=tip, length=float(np.sum(np.linalg.norm(np.diff(pts, axis=0), axis=1)))):
            a = math.atan2(pco.y, pco.x)
            u = min(max(pco.z / length, 0.0), 1.0)
            groove = -0.0009 * max(0.0, math.sin(7 * a + 3.0 * math.sin(pco.z * 40))) ** 3 * (1 - u) ** 0.5
            pearl = 0.0007 * max(0.0, 1 - pco.z / 0.05) * (0.5 + 0.5 * math.sin(a * 11 + pco.z * 300)) if not tip else 0
            return groove + pearl
        kit.displace(tube, ridges)
        parts.append(tube)
    burr = kit.tube(f"{name}Burr", [np.array(beam[0]) + (np.array(beam[1]) - beam[0]) * f for f in (-0.08, 0.1)],
                    radii=[0.0138, 0.0128], sides=16, material=material)
    kit.displace(burr, lambda co, pco: 0.0012 * max(0.0, math.sin(math.atan2(pco.y, pco.x) * 13)) ** 2)
    parts.append(burr)
    return parts


# ------------------------------------------------------------------- legs

def _bone_frame(a, b, cranial_hint):
    z = C.unit(np.asarray(b) - np.asarray(a))
    x = np.asarray(cranial_hint, dtype=np.float64)
    x = C.unit(x - z * z.dot(x))
    return np.stack([x, np.cross(z, x), z], axis=1)


# Joint chains (world, before centring). Left (upper) legs lie on the right ones.
LEGS = {
    "ForeL": dict(joints=[(-0.2, 0.03, 0.045), (-0.125, -0.09, 0.028), (-0.195, -0.272, 0.02),
                          (-0.072, -0.335, 0.015), (-0.032, -0.35, 0.013)], hint=(0, 0, 1), kind="fore"),
    "ForeR": dict(joints=[(-0.215, 0.025, 0.02), (-0.16, -0.098, 0.016), (-0.275, -0.24, 0.013),
                          (-0.18, -0.318, 0.011), (-0.143, -0.338, 0.01)], hint=(0, 0, 1), kind="fore"),
    "HindL": dict(joints=[(0.45, 0.14, 0.07), (0.3, -0.05, 0.03), (0.5, -0.2, 0.02),
                          (0.69, -0.33, 0.015), (0.735, -0.35, 0.013)], hint=(0, 0, 1), kind="hind"),
    "HindR": dict(joints=[(0.45, 0.14, 0.02), (0.32, -0.075, 0.016), (0.53, -0.19, 0.013),
                          (0.715, -0.27, 0.011), (0.76, -0.285, 0.01)], hint=(0, 0, 1), kind="hind"),
}
BONE_NAMES = {"fore": ("humerus", "radius", "cannon_f"), "hind": ("femur", "tibia", "cannon_h")}


def leg_parts(name, spec):
    """[(part name, node, voxel, tris, category, rotation, translation)] for one leg."""
    j = [np.asarray(p, dtype=np.float64) for p in spec["joints"]]
    out = []
    kinds = BONE_NAMES[spec["kind"]]
    for k, kind in enumerate(kinds):
        a, b = j[k], j[k + 1]
        L = float(np.linalg.norm(b - a))
        out.append((f"{name}_{kind}", long_bone(kind, L), 0.0009, 480 if k < 2 else 420, "bone",
                    _bone_frame(a, b, spec["hint"]), a))
    # Pastern: two phalanges from the fetlock toward the hoof.
    a, b = j[3], j[4]
    mid = a + (b - a) * 0.58
    for label, p0, p1, kind in (("p1", a, mid, "p1"), ("p2", mid, b, "p2")):
        L = float(np.linalg.norm(p1 - p0))
        out.append((f"{name}_{label}", long_bone(kind, L), 0.0007, 200, "bone", _bone_frame(p0, p1, spec["hint"]), p0))
    if spec["kind"] == "hind":
        hock = j[2]
        x = C.unit(j[2] - j[1])
        frame = np.stack([x, np.cross(C.ZUP, x), C.ZUP], axis=1)
        out.append((f"{name}_calcaneus", calcaneus_node(), 0.0008, 300, "bone", frame, hock))
    # Hoof: toes forward along the pastern, sole resting on its side on the ground.
    fwd = C.unit(j[4] - j[3])
    up = C.unit(np.cross(fwd, C.ZUP)) if spec["hint"][2] > 0 else C.ZUP
    side = np.cross(up, fwd)
    frame = np.stack([fwd, side, up], axis=1)
    out.append((f"{name}_hoof", hoof_node(), 0.0007, 640, "hoof", frame, j[4] - fwd * 0.012 - up * 0.013))
    return out


# ------------------------------------------------------------------ skull

SKULL_ORIGIN = np.array([-0.458, 0.034, 0.061])
SKULL_FORWARD = C.unit((-0.36, -0.93, -0.05))
SKULL_ROLL = -12.0


def skull_frame():
    x = SKULL_FORWARD
    y = C.unit(np.cross(np.cross(x, C.ZUP), x))          # left side, toward the sky
    frame = np.stack([x, y, np.cross(x, y)], axis=1)
    return frame @ _rot((1, 0, 0), SKULL_ROLL)


MANDIBLE_AT = (-0.64, -0.06, 30.0)          # x, y, heading (deg)
SHED_ANTLER_AT = (-0.585, 0.215, 145.0)
SCAPULA_AT = (-0.075, -0.2, 118.0)
VSHIFT = {}                                  # vertebra name -> z shift applied when settling
RAISED = []                                  # upper left limb bones still held up by the hide
CLEARANCE = {"thoracic": 0.0075, "cervical": 0.003, "axis": 0.003, "atlas": 0.002, "lumbar": 0.002,
             "sacrum": 0.004, "caudal": 0.002}


def pelvis_placement():
    s = S_OF["Sacrum"] - 0.012
    frame = spine_frame(s) @ _rot((1, 0, 0), 12)
    return frame, SPINE.at(s) + (0.0, -0.028, 0.002)


def parts_list(kit, mats):
    """Every skeleton part as a Blender object with its material. Returns
    [(object, category, group)]: category picks the material family (bone, antler, hoof,
    teeth); group says where it lies (head, neck, body, tail, leg, loose, shed), which
    the hide uses to decide what it covers."""
    out = []

    def add(name, node, voxel, tris, category, rotation, translation, group):
        obj = C.sdf_part(name, node, voxel, tris, mats[category], rotation, translation)
        out.append((obj, category, group))
        return obj

    # Head: skull, upper teeth and the attached left antler rest together.
    frame = skull_frame()
    head = [add("Skull", skull_node(), 0.0007, 11500, "bone", frame, SKULL_ORIGIN, "head"),
            add("TeethUpper", upper_teeth_node(), 0.0005, 1800, "teeth", frame, SKULL_ORIGIN, "head")]
    for part in antler(kit, "AntlerL", mats["antler"], 1.0):
        C.place(part, frame, SKULL_ORIGIN, tag=False)
        out.append((part, "antler", "head"))
        head.append(part)
    C.settle(head, 0.0005)

    jaw = [add("Mandible", mandible_node(), 0.0006, 2800, "bone", np.eye(3), (0, 0, 0), "head"),
           add("TeethLower", lower_teeth_node(), 0.0005, 1000, "teeth", np.eye(3), (0, 0, 0), "head")]
    C.lay_flat(jaw, MANDIBLE_AT[:2] + (0.0,), MANDIBLE_AT[2])

    shed = antler(kit, "AntlerShed", mats["antler"], -1.0)
    out += [(part, "antler", "shed") for part in shed]
    C.lay_flat(shed, SHED_ANTLER_AT[:2] + (0.0,), SHED_ANTLER_AT[2])

    for index, (name, kind, length, s) in enumerate(LAYOUT):
        region = "neck" if kind in ("atlas", "axis", "cervical") else ("tail" if kind == "caudal" else "body")
        rotation, translation = vertebra_placement(name, kind, length, s, index)
        local_index = index - {"cervical": 2, "thoracic": 7, "lumbar": 20, "caudal": 27}.get(kind, 0)
        obj = add(name, vertebra_node(kind, length, local_index), 0.0006 if kind != "sacrum" else 0.0007,
                  VERTEBRA_TRIS[kind], "bone", rotation, translation, region)
        VSHIFT[name] = C.settle([obj], CLEARANCE[kind])

    for i in range(13):
        out.append((rib(kit, f"RibR{i + 1}", i, False, mats["bone"]), "bone", "body"))
        state = UPPER[i]
        if state is None:
            continue
        cut = state[1] if isinstance(state, tuple) else 1.0
        out.append((rib(kit, f"RibL{i + 1}", i, True, mats["bone"], cut=cut), "bone", "body"))
    for name, number, centre, heading in LOOSE:
        pts, _ = loose_rib_points(number - 1, centre, heading)
        out.append((rib(kit, name, number - 1, True, mats["bone"], points=pts), "bone", "loose"))

    frame, origin = pelvis_placement()
    pelvis = add("Pelvis", pelvis_node(), 0.0008, 4000, "bone", frame, origin, "body")
    dz = C.settle([pelvis], 0.0008)
    acetabula = {side: origin + frame @ np.array([0, 0, side * 0.045]) + (0, 0, dz) for side in (1, -1)}

    scapula = add("ScapulaL", scapula_node(), 0.0006, 1600, "bone", np.eye(3), (0, 0, 0), "loose")
    C.lay_flat([scapula], SCAPULA_AT[:2] + (0.0,), SCAPULA_AT[2])

    for leg, spec in LEGS.items():
        spec = dict(spec)
        joints = [np.array(j, dtype=np.float64) for j in spec["joints"]]
        if spec["kind"] == "hind":
            joints[0] = acetabula[1 if leg.endswith("L") else -1]
        spec["joints"] = joints
        for name, node, voxel, tris, category, rotation, translation in leg_parts(leg, spec):
            group = "leg" if any(k in name for k in ("cannon", "p1", "p2", "hoof", "calcaneus")) else "body"
            obj = add(name, node, voxel, tris, category, rotation, translation, group)
            upper = any(k in name for k in ("humerus", "femur"))
            if upper and leg.endswith("L"):
                RAISED.append(obj)
            else:
                C.settle([obj], 0.001)
    return out