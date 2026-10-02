"""Shared painting and report helpers for small wildflower recipes (wood anemone, red campion, foxglove,
cow parsley). Leaves use homestead_foliage.paint_blade like the bluebell and primrose; this adds:
- paint_flower_face: an open flower seen from the front (radial petals or tepals, notches, an eye),
  for Batch.flat cards;
- paint_stem: a stem column, optionally hairy or flushed red;
- paint_umbellet: a tight dome of tiny white florets seen from above (cow parsley's umbellets);
- report(): the shared REPORT dict (wind vertex colours, one masked two-sided material).
"""
import math

import numpy as np

import homestead_foliage as F


def report(name):
    return {
        "blocking": False,
        "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
                 "G": "per-stem random phase", "B": "flutter 0 at leaf base -> 1 at leaf tip", "A": "1"},
        "material_notes": (f"One material M_{name}: T_{name}_basecolor (sRGB, alpha = opacity mask, clip 0.5), "
                           "_normal (OpenGL, flip green for Unreal), _roughness (R roughness, G translucency mask, "
                           "B AO). Two-sided foliage, masked."),
    }


def paint_flower_face(atlas, key, nrng, *, petals, inner, outer, eye, centre, size_m, notch=0.0, notch_width=0.06,
                      rim=0.92, waist=0.18, gap=0.0, veins=0.0, eye_radius=0.2, centre_radius=0.08, dots=0):
    """An open flower filling tile ``key`` (centred), ``petals`` radial lobes.
    ``inner``/``outer``: petal colour at the eye and at the rim; ``eye``: a ring of colour round the centre;
    ``centre``: the boss (stamens, anthers); ``notch``: depth of the cleft at each petal tip (campion's
    deeply bilobed petals ~0.35); ``waist``: how much the petals narrow between lobes; ``gap``: how far the
    petals separate toward the centre; ``dots``: that many anther dots round the boss."""
    X, Y, px = atlas.grid(key)
    Xc, Yc = X, Y - 0.5
    r = np.hypot(Xc, Yc) / 0.5
    th = np.arctan2(Xc, Yc)
    lobe = np.mod(th / (2 * math.pi) * petals + 0.5, 1.0) - 0.5
    edge = rim - waist * np.abs(lobe) * 2 - notch * np.exp(-(lobe / notch_width) ** 2)
    if gap:
        edge = np.where(np.abs(lobe) > 0.5 - gap * (1 - r), -1.0, edge)
    n = F.noise(X.shape, nrng, freq=24.0, beta=1.8)
    inside = edge + 0.02 * (n - 0.5) - r
    layer = F.Layer(X.shape)
    layer.alpha = np.clip(inside / (px * 2.0) + 0.5, 0, 1)
    col = F.lerp(np.array(inner), np.array(outer), F.smoothstep(0.15, 0.9, r))
    col = F.lerp(col, col * 0.92, n)
    if veins:
        v = np.exp(-((np.mod(th / (2 * math.pi) * petals * 5, 1.0) - 0.5) / 0.12) ** 2) * F.smoothstep(0.15, 0.8, r)
        col = F.lerp(col, col * 0.8, v * veins)
    ring = np.clip(1.0 - np.abs(r - eye_radius) / 0.08, 0, 1)
    col = F.lerp(col, np.array(eye), ring * 0.8)
    boss = np.clip(1.0 - (r - centre_radius) / 0.03, 0, 1)
    col = F.lerp(col, np.array(centre), boss)
    if dots:
        a = np.mod(th / (2 * math.pi) * dots, 1.0) - 0.5
        dot = np.exp(-(a / 0.18) ** 2) * np.exp(-((r - centre_radius * 1.4) / 0.025) ** 2)
        col = F.lerp(col, np.array(centre), np.clip(dot * 1.5, 0, 1))
    layer.color = col
    layer.height = 0.0001 * (1 - F.smoothstep(0.0, 0.5, r)) + 0.00003 * n
    layer.rough = 0.5 + 0.1 * n
    layer.trans = 0.55 * np.ones(X.shape)
    atlas.put(key, layer, meters_per_px=size_m / X.shape[0])


def paint_stem(atlas, key, nrng, base, top, *, hair=0.0, length_m=0.5, trans=0.3):
    """A stem column, ``base`` colour at the bottom shading to ``top``; ``hair`` pales it with fine down."""
    U, V = atlas.column_grid(key)
    st = F.noise(U.shape, nrng, freq=60.0, beta=1.4, aniso=(1.0, 8.0))
    layer = F.Layer(U.shape)
    layer.color = F.lerp(np.array(base), np.array(top), F.smoothstep(0.0, 0.6, V))
    if hair:
        fuzz = F.smoothstep(0.75, 0.95, F.noise(U.shape, nrng, freq=200.0, beta=0.8))
        layer.color = F.lerp(layer.color, np.array((0.30, 0.32, 0.26)), fuzz * hair)
    layer.height = 0.00004 * st
    layer.rough = 0.55 + 0.08 * st
    layer.trans[...] = trans
    atlas.put(key, layer, meters_per_px=length_m / U.shape[0], opaque=True)


def paint_umbellet(atlas, key, nrng, rng, *, florets=26, colour=(0.80, 0.80, 0.74), size_m=0.025):
    """A small domed head of tiny five-petalled white florets seen from above, alpha-cut round the edge."""
    X, Y, px = atlas.grid(key)
    Xc, Yc = X, Y - 0.5
    layer = F.Layer(X.shape)
    alpha = np.zeros(X.shape)
    shade = np.zeros(X.shape)
    for k in range(florets):
        # Sunflower spiral packing out to the rim; outer florets a little larger (they are on cow parsley).
        rr = 0.42 * math.sqrt((k + 0.5) / florets)
        a = k * 2.39996 + rng.uniform(-0.1, 0.1)
        cx, cy = rr * math.sin(a), rr * math.cos(a)
        radius = 0.055 + 0.03 * rr / 0.42
        d = np.hypot(Xc - cx, Yc - cy) / radius
        t = np.arctan2(Xc - cx, Yc - cy)
        petal = 1.0 - 0.25 * np.abs(np.cos(t * 2.5)) - d
        alpha = np.maximum(alpha, np.clip(petal / (px * 3 / radius) + 0.5, 0, 1))
        shade = np.maximum(shade, np.clip(1.0 - d / 0.25, 0, 1))
    n = F.noise(X.shape, nrng, freq=30.0, beta=1.6)
    layer.alpha = alpha
    layer.color = F.lerp(np.array(colour), np.array(colour) * 0.85, n)
    layer.color = F.lerp(layer.color, np.array((0.55, 0.58, 0.40)), shade * 0.6)
    layer.height = 0.00006 * alpha + 0.00002 * n
    layer.rough = 0.6 + 0.08 * n
    layer.trans = 0.5 * np.ones(X.shape)
    atlas.put(key, layer, meters_per_px=size_m / X.shape[0])
