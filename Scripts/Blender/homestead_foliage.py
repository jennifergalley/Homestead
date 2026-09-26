"""Foliage toolkit for Homestead underbrush recipes (shrubs, brambles, ferns, forbs).

Two halves:

* ``Atlas``: paints one shared texture atlas per asset set directly with numpy. Leaf, petal and
  berry *tiles* carry an alpha-masked silhouette (serrations, holes, curled dead tips); stem and
  bark *columns* span the atlas height and tile seamlessly along V, so tubes of any length map
  into them with ``v = arclength / column_length``. ``Atlas.save`` writes
  ``T_<Name>_basecolor.png`` (sRGB RGB + alpha mask), ``T_<Name>_normal.png`` (tangent space,
  OpenGL +Y; flip green for Unreal) and ``T_<Name>_roughness.png`` (R = roughness,
  G = translucency/subsurface mask, B = ambient occlusion). ``Atlas.material`` builds the
  single two-sided, alpha-clipped Blender material that the review renders use.

* ``Batch``: accumulates leaf cards, swept tubes, cones and berries into one mesh with per-loop
  UVs and the ``Wind`` vertex colour (BYTE_COLOR, exported to FBX):
    R = height above ground / plant height (0 at the root, 1 at the top) - main sway weight
    G = per-branch random phase 0..1 (so branches do not sway in lockstep)
    B = flutter weight: 0 on stems/canes, rising 0 -> 1 from a leaf's base to its tip
    A = 1
  ``finish_lods`` joins the batches for every LOD with one shared origin (bottom centre of LOD0).

Everything is seeded and deterministic. Colours are painted in linear space and converted to
sRGB when saved; keep leaf albedo muted (linear green channel roughly 0.06-0.16).
"""
import math
from pathlib import Path

import numpy as np
import bpy
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parents[2]


# ------------------------------------------------------------------ utilities

def props_folder(name):
    return ROOT / "Assets" / "Props" / name


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def to_srgb(lin):
    lin = np.clip(lin, 0.0, 1.0)
    return np.where(lin <= 0.0031308, lin * 12.92, 1.055 * np.power(lin, 1.0 / 2.4) - 0.055)


def lerp(a, b, t):
    t = np.asarray(t)[..., None] if np.ndim(t) else t
    return np.asarray(a) * (1.0 - t) + np.asarray(b) * t


def noise(shape, rng, freq=8.0, beta=2.2, aniso=(1.0, 1.0)):
    """Periodic fractal noise in 0..1 (tileable on both axes). ``freq`` is the feature
    frequency in cycles per tile; ``aniso`` stretches (x, y) features."""
    h, w = shape
    white = rng.standard_normal((h, w))
    fy = np.fft.fftfreq(h)[:, None] * h / aniso[1]
    fx = np.fft.fftfreq(w)[None, :] * w / aniso[0]
    r = np.sqrt(fx * fx + fy * fy)
    amp = 1.0 / np.power(1.0 + (r / max(freq, 0.01)) ** 2, beta * 0.5)
    amp[0, 0] = 0.0
    field = np.real(np.fft.ifft2(np.fft.fft2(white) * amp))
    lo, hi = np.percentile(field, (0.5, 99.5))
    return np.clip((field - lo) / max(hi - lo, 1e-9), 0.0, 1.0)


def ridged(field):
    return 1.0 - np.abs(field * 2.0 - 1.0)


def polyline_distance(X, Y, pts, max_dist=None):
    """Distance from grid points to a polyline and the normalized arclength 0..1 at the
    nearest point. With ``max_dist`` only points inside the polyline's padded bounding box are
    evaluated (others report 1e9), which keeps thin vein networks cheap."""
    pts = [tuple(p) for p in pts]
    lengths = [0.0]
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        lengths.append(lengths[-1] + math.hypot(x1 - x0, y1 - y0))
    total = max(lengths[-1], 1e-9)
    if max_dist is not None:
        xs = [p[0] for p in pts]
        ys = [p[1] for p in pts]
        sel = ((X > min(xs) - max_dist) & (X < max(xs) + max_dist) &
               (Y > min(ys) - max_dist) & (Y < max(ys) + max_dist))
        dist = np.full(X.shape, 1e9)
        along = np.zeros(X.shape)
        if sel.any():
            d, a = polyline_distance(X[sel], Y[sel], pts)
            dist[sel] = d
            along[sel] = a
        return dist, along
    dist = np.full(X.shape, 1e9)
    along = np.zeros(X.shape)
    for k, ((x0, y0), (x1, y1)) in enumerate(zip(pts, pts[1:])):
        dx, dy = x1 - x0, y1 - y0
        l2 = dx * dx + dy * dy + 1e-12
        t = np.clip(((X - x0) * dx + (Y - y0) * dy) / l2, 0.0, 1.0)
        d = np.hypot(X - (x0 + t * dx), Y - (y0 + t * dy))
        closer = d < dist
        dist = np.where(closer, d, dist)
        along = np.where(closer, (lengths[k] + t * math.sqrt(l2)) / total, along)
    return dist, along


def normal_from_height(height, px_size, strength=1.0, wrap=False):
    """OpenGL tangent-space normal (+Y up the texture) from a height field in meters."""
    if wrap:
        dx = (np.roll(height, -1, 1) - np.roll(height, 1, 1)) * 0.5
        dy = (np.roll(height, -1, 0) - np.roll(height, 1, 0)) * 0.5
    else:
        dy, dx = np.gradient(height)
    nx = -dx / px_size * strength
    ny = -dy / px_size * strength
    nz = np.ones_like(height)
    n = np.stack([nx, ny, nz], -1)
    n /= np.linalg.norm(n, axis=-1, keepdims=True)
    return n * 0.5 + 0.5


# ------------------------------------------------------------------ layers

class Layer:
    """One painted surface on a coordinate grid: linear colour, coverage, height (m),
    roughness, translucency and AO."""

    def __init__(self, shape):
        self.color = np.zeros(shape + (3,))
        self.alpha = np.zeros(shape)
        self.height = np.zeros(shape)
        self.rough = np.full(shape, 0.55)
        self.trans = np.zeros(shape)
        self.ao = np.ones(shape)

    def over(self, top):
        """Composite ``top`` over this layer (alpha-over on every channel)."""
        a = top.alpha
        a3 = a[..., None]
        self.color = self.color * (1 - a3) + top.color * a3
        # Height stacks so overlapping leaflets keep their relief.
        self.height = self.height * (1 - a) + top.height * a
        self.rough = self.rough * (1 - a) + top.rough * a
        self.trans = self.trans * (1 - a) + top.trans * a
        self.ao = self.ao * (1 - a) + top.ao * a
        self.alpha = np.maximum(self.alpha, a)
        return self


# ------------------------------------------------------------------ leaf painting

def serration(y, side, count, depth, double=0.0, phase=0.0, forward=0.78):
    """Forward-pointing teeth along a margin parameter y (0 base .. 1 tip): each tooth rises
    over ``forward`` of its period and drops back over the rest."""
    def teeth(ph):
        saw = ph - np.floor(ph)
        tri = np.where(saw < forward, saw / forward, (1.0 - saw) / (1.0 - forward))
        return np.power(tri, 1.25) - 0.5
    ph = y * count + phase + np.where(side > 0, 0.37, 0.0)
    tooth = teeth(ph)
    if double:
        tooth = tooth + double * teeth(ph * 3.0 + 0.2)
    return tooth * depth


def ovate(width=0.32, base=0.03, tip=0.98, widest=0.40, tip_sharp=1.4, base_round=0.6,
          teeth=0, tooth_depth=0.0, double=0.0, cordate=0.0, phase=0.0):
    """Simple leaf/leaflet silhouette in tile units (x across, y 0 base .. 1 tip). Returns
    ``shape(X, Y) -> (inside, blade_y, halfwidth)``; inside > 0 within the blade."""

    def profile(y):
        s = np.clip((y - base) / (tip - base), 0.0, 1.0)
        # Asymmetric bump peaking at ``widest``: rounded base, tapering (acuminate) tip.
        lower = np.power(np.clip(s / widest, 0, 1), base_round)
        upper = np.power(np.clip((1 - s) / (1 - widest), 0, 1), tip_sharp)
        hw = width * np.where(s < widest, np.sin(lower * math.pi * 0.5), np.sin(upper * math.pi * 0.5))
        if cordate:
            hw = hw + cordate * width * np.exp(-((s - 0.04) / 0.06) ** 2)
        return s, hw

    def shape(X, Y):
        s, hw = profile(Y)
        if teeth:
            fade = smoothstep(0.04, 0.16, s) * (1 - smoothstep(0.93, 1.0, s))
            hw = hw * (1 + serration(s, X, teeth, tooth_depth, double, phase) * fade)
        inside = np.minimum(hw - np.abs(X), np.minimum(Y - base, tip - Y) * 3.0)
        return inside, s, hw

    return shape


def pinnate_veins(count=7, angle=0.9, curve=0.5, reach=0.86, base=0.03, tip=0.98, width=0.32,
                  widest=0.40, start=0.08, stop=0.86, rng=None, midrib_bend=0.0):
    """Midrib plus ``count`` pairs of secondary veins curving toward the tip. Returns a list of
    (polyline, width_scale) in tile units."""
    rng = rng or np.random.default_rng(0)
    shape = ovate(width=width, base=base, tip=tip, widest=widest)
    mid = [(midrib_bend * math.sin(math.pi * t), base + (tip - base) * t) for t in np.linspace(0, 1, 18)]
    veins = [(mid, 1.0)]
    for k in range(count):
        s0 = start + (stop - start) * k / max(count - 1, 1) + rng.uniform(-0.015, 0.015)
        for side in (-1, 1):
            s_off = s0 + (0.018 if side > 0 else 0.0)
            y0 = base + (tip - base) * s_off
            pts = []
            for t in np.linspace(0, 1, 10):
                yy = y0 + (tip - base) * (angle * 0.22 * t + curve * 0.10 * t * t)
                _, _, hw = shape(np.zeros(1), np.array([min(yy, tip)]))
                pts.append((side * reach * hw[0] * t + midrib_bend * math.sin(math.pi * s_off), yy))
            veins.append((pts, 0.55 - 0.2 * k / max(count, 1)))
    return veins


def paint_blade(X, Y, rng, shape, veins, pal, px, *, vein_width=0.006, vein_depth=0.00035,
                puff=0.00025, tertiary=0.5, damage=0.0, holes=0.0, yellow=0.0, dry=0.0,
                edge_burn=0.0, gloss=0.0, hair=0.0, stalk=0.0, trans=0.55):
    """Paint one blade onto coordinate grids X, Y (tile units). ``pal`` keys: base, tip,
    vein, margin, and optionally yellow, brown, dry. ``px`` is the tile-unit size of a pixel.
    Returns a Layer."""
    shp = X.shape
    layer = Layer(shp)
    inside, s, hw = shape(X, Y)
    gy, gx = np.gradient(inside)
    grad = np.sqrt(gx * gx + gy * gy) / px + 1e-9
    sdf_px = inside / (grad * px)
    alpha = np.clip(sdf_px + 0.5, 0.0, 1.0)
    if stalk:
        d_stalk = np.abs(X) - stalk * (1 - 0.3 * Y)
        stalk_mask = np.clip(-d_stalk / px + 0.5, 0, 1) * (Y < 0.2) * (Y > 0.0)
    else:
        stalk_mask = np.zeros(shp)

    vein_mask = np.zeros(shp)
    for pts, scale in veins:
        w0 = vein_width * scale
        d, along = polyline_distance(X, Y, pts, max_dist=max(w0, px) * 4.0)
        w = w0 * (1.0 - 0.65 * along)
        vein_mask = np.maximum(vein_mask, np.exp(-(d / np.maximum(w, px * 0.7)) ** 2))
    n_lo = noise(shp, rng, freq=5.0, beta=2.4)
    n_mid = noise(shp, rng, freq=22.0, beta=2.0)
    n_hi = noise(shp, rng, freq=90.0, beta=1.6)
    net = smoothstep(0.82, 0.97, ridged(noise(shp, rng, freq=55.0, beta=2.6))) * tertiary

    edge = 1.0 - smoothstep(0.0, 0.035, inside)
    color = lerp(pal["base"], pal["tip"], smoothstep(0.1, 0.95, s))
    color = color * (0.86 + 0.28 * n_lo)[..., None]
    color = lerp(color, color * 0.82, smoothstep(0.35, 0.75, n_mid) * 0.5)
    color = lerp(color, pal["margin"], edge * 0.55)
    color = lerp(color, pal["vein"], np.clip(vein_mask * 0.6 + net * 0.12, 0, 1))
    rough = 0.50 - gloss * 0.12 + 0.08 * n_mid + 0.05 * vein_mask
    tr = np.full(shp, trans) * (1 - 0.35 * vein_mask)

    if yellow:
        ymask = np.clip(yellow * (0.55 + 0.9 * (n_lo - 0.5)) + 0.25 * smoothstep(0.6, 1.0, s) * yellow, 0, 1)
        color = lerp(color, pal.get("yellow", (0.30, 0.26, 0.06)), ymask)
    if damage:
        spot_field = noise(shp, rng, freq=26.0, beta=2.4)
        spots = smoothstep(0.86 - 0.10 * damage, 0.90 - 0.10 * damage, spot_field)
        halo = smoothstep(0.78 - 0.10 * damage, 0.88 - 0.10 * damage, spot_field)
        color = lerp(color, pal.get("yellow", (0.30, 0.26, 0.06)), halo * 0.35)
        color = lerp(color, pal.get("brown", (0.11, 0.05, 0.02)), spots)
        rough = rough + spots * 0.2
    if edge_burn:
        burn = np.clip((1.0 - smoothstep(0.0, 0.07 * edge_burn, inside + (n_mid - 0.5) * 0.05))
                       + smoothstep(1.0 - 0.25 * edge_burn, 1.0, s), 0, 1)
        color = lerp(color, pal.get("brown", (0.11, 0.05, 0.02)), burn)
        tr = tr * (1 - 0.6 * burn)
    if dry:
        brown = np.asarray(pal.get("brown", (0.06, 0.03, 0.012)))
        dcol = lerp(pal.get("dry", (0.16, 0.085, 0.035)), brown, smoothstep(0.3, 0.8, n_lo) * 0.75)
        dcol = dcol * (0.82 + 0.3 * n_mid)[..., None]
        dcol = lerp(dcol, np.clip(np.asarray(pal.get("dry", (0.16, 0.085, 0.035))) * 1.45, 0, 1),
                    np.clip(vein_mask * 0.7 + net * 0.2, 0, 1))
        dcol = lerp(dcol, brown * 0.7, edge * 0.8)
        color = lerp(color, dcol, np.clip(dry, 0, 1))
        rough = rough + 0.22 * dry
        tr = tr * (1 - 0.7 * dry)
    if holes:
        hole_field = noise(shp, rng, freq=9.0, beta=3.0)
        cut = smoothstep(1.0 - 0.12 * holes, 1.0 - 0.10 * holes, hole_field)
        rim = smoothstep(1.0 - 0.17 * holes, 1.0 - 0.12 * holes, hole_field) * (1 - cut)
        color = lerp(color, pal.get("brown", (0.11, 0.05, 0.02)), rim * 0.8)
        alpha = alpha * (1 - cut)
    if hair:
        color = lerp(color, (0.30, 0.34, 0.26), smoothstep(0.7, 1.0, n_hi) * hair)

    stalk_col = np.asarray(pal.get("stalk", pal["vein"]))
    color = lerp(color, stalk_col, stalk_mask * (1 - alpha))
    alpha = np.maximum(alpha, stalk_mask)

    # Relief: veins sunk, lamina puffed between them, fine grain.
    between = 1.0 - np.clip(vein_mask + net * 0.35, 0, 1)
    height = puff * smoothstep(0.0, 1.0, between) - vein_depth * vein_mask + 0.00006 * n_hi
    height = height + 0.0001 * edge
    layer.color, layer.alpha, layer.height = color, alpha, height
    layer.rough = np.clip(rough, 0.3, 0.95)
    layer.trans = np.clip(tr, 0.0, 1.0)
    layer.ao = 1.0 - 0.25 * vein_mask - 0.15 * edge
    return layer


# ------------------------------------------------------------------ atlas

class Atlas:
    def __init__(self, name, size=4096, folder=None, seed=1, padding=12):
        self.name = name
        self.size = size
        self.folder = Path(folder or props_folder(name) / "Textures")
        self.rng = np.random.default_rng(seed)
        self.pad = padding
        s = size
        f32 = np.float32
        self.color = np.zeros((s, s, 3), f32)
        self.color[...] = (0.045, 0.065, 0.028)
        self.alpha = np.zeros((s, s), f32)
        self.normal = np.zeros((s, s, 3), f32)
        self.normal[...] = (0.5, 0.5, 1.0)
        self.rough = np.full((s, s), 0.6, f32)
        self.trans = np.zeros((s, s), f32)
        self.ao = np.ones((s, s), f32)
        self.rects = {}
        self._right = s          # columns are allocated from the right edge leftwards
        self._shelf_x = 0
        self._shelf_y = 0
        self._shelf_h = 0

    # --- allocation (pixel boxes: x0, y0, w, h; rows start at the bottom)
    def column(self, key, width):
        x0 = self._right - width
        if x0 < self._shelf_x:
            raise RuntimeError("atlas full (column " + key + ")")
        self._right = x0
        box = (x0, 0, width, self.size)
        self.rects[key] = box
        return box

    def tile(self, key, width, height):
        if self._shelf_x + width > self._right:
            self._shelf_x = 0
            self._shelf_y += self._shelf_h
            self._shelf_h = 0
        if self._shelf_y + height > self.size or width > self._right:
            raise RuntimeError("atlas full (tile " + key + ")")
        box = (self._shelf_x, self._shelf_y, width, height)
        self._shelf_x += width
        self._shelf_h = max(self._shelf_h, height)
        self.rects[key] = box
        return box

    def uv(self, key, inset=True):
        """Tile rect in UV space (u0, u1, v0, v1). Tiles are inset by the padding so the
        card's edge stays transparent; columns are inset only horizontally."""
        x0, y0, w, h = self.rects[key]
        p = self.pad if inset else 0
        s = float(self.size)
        if h == self.size:
            return ((x0 + p) / s, (x0 + w - p) / s, 0.0, 1.0)
        return ((x0 + p) / s, (x0 + w - p) / s, (y0 + p) / s, (y0 + h - p) / s)

    def grid(self, key):
        """Coordinates for painting inside a tile: X in [-a, a], Y in [0, 1] over the inset
        area (a = aspect/2), plus the pixel size in those units."""
        x0, y0, w, h = self.rects[key]
        p = self.pad
        iw, ih = w - 2 * p, h - 2 * p
        px = 1.0 / ih
        a = iw / (2.0 * ih)
        ys = (np.arange(h) - p + 0.5) * px
        xs = (np.arange(w) - p + 0.5) * px - a
        X, Y = np.meshgrid(xs, ys)
        return X, Y, px

    def put(self, key, layer, normal_strength=1.0, meters_per_px=None, wrap=False, opaque=False):
        x0, y0, w, h = self.rects[key]
        sl = (slice(y0, y0 + h), slice(x0, x0 + w))
        self.color[sl] = np.clip(layer.color, 0, 1)
        self.alpha[sl] = 1.0 if opaque else layer.alpha
        mpp = meters_per_px or (0.08 / h)
        self.normal[sl] = normal_from_height(layer.height, mpp, normal_strength, wrap=wrap)
        self.rough[sl] = layer.rough
        self.trans[sl] = layer.trans
        self.ao[sl] = layer.ao

    def column_grid(self, key):
        x0, y0, w, h = self.rects[key]
        U, V = np.meshgrid((np.arange(w) + 0.5) / w, (np.arange(h) + 0.5) / h)
        return U, V

    # --- output
    def cached(self):
        """True when HOMESTEAD_REUSE_TEXTURES=1 and this atlas's maps already exist (geometry-only
        iteration); the existing files are then used as-is."""
        import os
        roles = ("basecolor", "normal", "roughness")
        paths = {r: self.folder / f"T_{self.name}_{r}.png" for r in roles}
        if os.environ.get("HOMESTEAD_REUSE_TEXTURES") == "1" and all(p.exists() for p in paths.values()):
            self.paths = paths
            print("HOMESTEAD_ATLAS_REUSED", self.name)
            return True
        return False

    def save(self):
        self.folder.mkdir(parents=True, exist_ok=True)
        paths = {}

        def write(role, rgb, alpha=None, srgb=False):
            s = self.size
            img = bpy.data.images.new(f"T_{self.name}_{role}", s, s, alpha=alpha is not None,
                                      float_buffer=False)
            img.colorspace_settings.name = "sRGB" if srgb else "Non-Color"
            rgba = np.ones((s, s, 4), np.float32)
            rgba[..., :3] = to_srgb(rgb) if srgb else np.clip(rgb, 0, 1)
            if alpha is not None:
                rgba[..., 3] = np.clip(alpha, 0, 1)
            img.pixels.foreach_set(rgba.ravel())
            path = self.folder / f"T_{self.name}_{role}.png"
            img.filepath_raw = str(path)
            img.file_format = "PNG"
            img.save()
            bpy.data.images.remove(img)
            paths[role] = path

        write("basecolor", self.color, self.alpha, srgb=True)
        write("normal", self.normal)
        write("roughness", np.stack([self.rough, self.trans, self.ao], -1))
        self.paths = paths
        return paths

    def material(self, translucent=(1.15, 1.2, 0.55)):
        """Two-sided alpha-clipped material using the saved maps (for Blender review
        renders; Unreal uses a masked two-sided foliage material with the same maps)."""
        name = "M_" + self.name
        mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
        mat.use_nodes = True
        mat.use_backface_culling = False
        try:
            mat.surface_render_method = "DITHERED"
        except AttributeError:
            pass
        tree = mat.node_tree
        tree.nodes.clear()
        out = tree.nodes.new("ShaderNodeOutputMaterial")
        bsdf = tree.nodes.new("ShaderNodeBsdfPrincipled")

        def tex(role, colorspace):
            node = tree.nodes.new("ShaderNodeTexImage")
            node.image = bpy.data.images.load(str(self.paths[role]), check_existing=True)
            node.image.colorspace_settings.name = colorspace
            node.image.alpha_mode = "STRAIGHT"
            node.interpolation = "Cubic"
            return node

        base = tex("basecolor", "sRGB")
        nrm = tex("normal", "Non-Color")
        rgh = tex("roughness", "Non-Color")
        split = tree.nodes.new("ShaderNodeSeparateColor")
        tree.links.new(rgh.outputs["Color"], split.inputs["Color"])
        tree.links.new(base.outputs["Color"], bsdf.inputs["Base Color"])
        clip = tree.nodes.new("ShaderNodeMath")
        clip.operation = "GREATER_THAN"
        clip.inputs[1].default_value = 0.5
        tree.links.new(base.outputs["Alpha"], clip.inputs[0])
        tree.links.new(split.outputs["Red"], bsdf.inputs["Roughness"])
        nmap = tree.nodes.new("ShaderNodeNormalMap")
        tree.links.new(nrm.outputs["Color"], nmap.inputs["Color"])
        tree.links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])

        def multiply(a, b):
            node = tree.nodes.new("ShaderNodeMix")
            node.data_type = "RGBA"
            node.blend_type = "MULTIPLY"
            node.inputs["Factor"].default_value = 1.0
            sockets = [s for s in node.inputs if s.type == "RGBA"]
            for socket, value in zip(sockets, (a, b)):
                if isinstance(value, bpy.types.NodeSocket):
                    tree.links.new(value, socket)
                else:
                    socket.default_value = (*value, 1.0)
            return next(s for s in node.outputs if s.type == "RGBA")

        # AO darkens albedo slightly (as the Unreal material will).
        tree.links.new(multiply(base.outputs["Color"], split.outputs["Blue"]), bsdf.inputs["Base Color"])
        # Thin-leaf translucency: mix a Translucent BSDF tinted from the albedo by the G mask.
        tl = tree.nodes.new("ShaderNodeBsdfTranslucent")
        tree.links.new(multiply(base.outputs["Color"], translucent), tl.inputs["Color"])
        tree.links.new(nmap.outputs["Normal"], tl.inputs["Normal"])
        fac = tree.nodes.new("ShaderNodeMath")
        fac.operation = "MULTIPLY"
        fac.inputs[1].default_value = 0.45
        tree.links.new(split.outputs["Green"], fac.inputs[0])
        mix = tree.nodes.new("ShaderNodeMixShader")
        tree.links.new(fac.outputs[0], mix.inputs["Fac"])
        tree.links.new(bsdf.outputs["BSDF"], mix.inputs[1])
        tree.links.new(tl.outputs["BSDF"], mix.inputs[2])
        transparent = tree.nodes.new("ShaderNodeBsdfTransparent")
        cut = tree.nodes.new("ShaderNodeMixShader")
        tree.links.new(clip.outputs[0], cut.inputs["Fac"])
        tree.links.new(transparent.outputs["BSDF"], cut.inputs[1])
        tree.links.new(mix.outputs["Shader"], cut.inputs[2])
        tree.links.new(cut.outputs["Shader"], out.inputs["Surface"])
        bsdf.inputs["Specular IOR Level"].default_value = 0.42
        mat["homestead_foliage"] = True
        return mat


# ------------------------------------------------------------------ geometry

def frame(direction, up_hint=Vector((0, 0, 1))):
    d = Vector(direction).normalized()
    side = d.cross(up_hint)
    if side.length < 1e-4:
        side = d.cross(Vector((1, 0, 0)))
    side.normalize()
    return d, side, side.cross(d).normalized()


class Batch:
    """Collects foliage geometry with per-loop UVs and Wind vertex colours."""

    def __init__(self, height=1.0):
        self.verts, self.faces, self.uvs, self.wind = [], [], [], []
        self.height = max(height, 0.05)

    @property
    def triangles(self):
        return sum(len(f) - 2 for f in self.faces)

    def _vert(self, co, phase, flutter):
        self.verts.append(tuple(co))
        r = min(max(co[2] / self.height, 0.0), 1.0)
        self.wind.append((r, phase % 1.0, min(max(flutter, 0.0), 1.0), 1.0))
        return len(self.verts) - 1

    def card(self, base, direction, up, length, width, rect, rows=3, cols=2, fold=0.25, curl=0.0,
             droop=0.15, twist=0.0, lift=0.0, phase=0.0, flutter=1.0, flutter_base=0.0):
        """Leaf card along ``direction`` whose upper face points toward ``up``. The UV rect
        spans the card (u across, v base -> tip). ``fold`` raises the margins (V crease at the
        midrib), ``curl`` rolls them, ``droop`` bends the tip down, ``lift`` curves it up."""
        d, side, n = frame(direction, Vector(up))
        u0, u1, v0, v1 = rect
        cols = max(cols, 1)
        idx = []
        pos = Vector(base)
        heading = d.copy()
        step = length / rows
        spine = [pos.copy()]
        for i in range(rows):
            t = (i + 0.5) / rows
            bend = Matrix.Rotation(-(droop - lift) * 1.6 / rows * (0.6 + t), 3, side)
            heading = (bend @ heading).normalized()
            pos = pos + heading * step
            spine.append(pos.copy())
        for i in range(rows + 1):
            t = i / rows
            tw = Matrix.Rotation(twist * t, 3, d)
            sd = tw @ side
            nn = tw @ n
            for j in range(cols + 1):
                s = j / cols * 2 - 1
                x = s * width * 0.5
                off = sd * x + nn * (fold * abs(x) + curl * (s * s) * width * 0.5)
                idx.append(self._vert(spine[i] + off, phase, flutter_base + (flutter - flutter_base) * t))
        for i in range(rows):
            for j in range(cols):
                a = i * (cols + 1) + j
                b, c, e = a + 1, a + cols + 2, a + cols + 1
                self.faces.append((idx[a], idx[b], idx[c], idx[e]))
                self.uvs.append([(u0 + (u1 - u0) * (jj / cols), v0 + (v1 - v0) * (ii / rows))
                                 for ii, jj in ((i, j), (i, j + 1), (i + 1, j + 1), (i + 1, j))])
        return idx

    def flat(self, center, normal, size, rect, spin=0.0, cup=0.0, phase=0.0, flutter=0.5, segs=2):
        """Small square card centred on ``center`` facing ``normal`` (flowers, rosettes)."""
        n = Vector(normal).normalized()
        a = n.orthogonal().normalized()
        a = Matrix.Rotation(spin, 3, n) @ a
        b = n.cross(a).normalized()
        u0, u1, v0, v1 = rect
        idx = []
        for i in range(segs + 1):
            for j in range(segs + 1):
                x = (j / segs * 2 - 1) * size * 0.5
                y = (i / segs * 2 - 1) * size * 0.5
                r2 = (x * x + y * y) / (size * size * 0.25)
                idx.append(self._vert(Vector(center) + a * x + b * y + n * (cup * r2 * size), phase,
                                      flutter * min(r2, 1.0)))
        for i in range(segs):
            for j in range(segs):
                p = i * (segs + 1) + j
                self.faces.append((idx[p], idx[p + 1], idx[p + segs + 2], idx[p + segs + 1]))
                self.uvs.append([(u0 + (u1 - u0) * (jj / segs), v0 + (v1 - v0) * (ii / segs))
                                 for ii, jj in ((i, j), (i, j + 1), (i + 1, j + 1), (i + 1, j))])

    def tube(self, points, radii, sides, rect, v_length=1.0, v_offset=0.0, phase=0.0, flutter=0.0,
             cap=False, roll=0.0, flatten=1.0):
        """Swept tube with parallel-transport frames. ``rect`` is a column (u0, u1, ...): u runs
        around, v = (arclength / v_length + v_offset) so the column tiles along the tube."""
        pts = [Vector(p) for p in points]
        count = len(pts)
        lengths = [0.0]
        for a, b in zip(pts, pts[1:]):
            lengths.append(lengths[-1] + (b - a).length)
        tangents = [(pts[min(i + 1, count - 1)] - pts[max(i - 1, 0)]).normalized() for i in range(count)]
        seed = Vector((0, 0, 1)) if abs(tangents[0].z) < 0.9 else Vector((1, 0, 0))
        normal = tangents[0].cross(seed).normalized()
        u0, u1 = rect[0], rect[1]
        rings = []
        for i in range(count):
            if i:
                normal = (tangents[i - 1].rotation_difference(tangents[i]) @ normal).normalized()
            binormal = tangents[i].cross(normal).normalized()
            ring = []
            for j in range(sides):
                ang = 2 * math.pi * j / sides + roll
                off = normal * math.cos(ang) + binormal * math.sin(ang) * flatten
                ring.append(self._vert(pts[i] + off * radii[i], phase, flutter))
            rings.append(ring)
        for i in range(count - 1):
            va, vb = lengths[i] / v_length + v_offset, lengths[i + 1] / v_length + v_offset
            for j in range(sides):
                k = (j + 1) % sides
                self.faces.append((rings[i][j], rings[i][k], rings[i + 1][k], rings[i + 1][j]))
                ua, ub = u0 + (u1 - u0) * j / sides, u0 + (u1 - u0) * (j + 1) / sides
                self.uvs.append([(ua, va), (ub, va), (ub, vb), (ua, vb)])
        if cap:
            tip = self._vert(pts[-1] + tangents[-1] * radii[-1] * 0.6, phase, flutter)
            v = lengths[-1] / v_length + v_offset
            for j in range(sides):
                k = (j + 1) % sides
                self.faces.append((rings[-1][j], rings[-1][k], tip))
                self.uvs.append([(u0 + (u1 - u0) * j / sides, v), (u0 + (u1 - u0) * (j + 1) / sides, v),
                                 ((u0 + u1) * 0.5, v + 0.002)])
        return lengths[-1]

    def cone(self, base, direction, length, radius, rect, sides=3, phase=0.0, hook=0.0, up=None):
        """Thorn/prickle: open cone from ``base`` along ``direction``; ``hook`` bends the tip."""
        d, side, n = frame(direction, Vector(up) if up is not None else Vector((0, 0, 1)))
        u0, u1, v0, v1 = rect
        ring = []
        for j in range(sides):
            ang = 2 * math.pi * j / sides
            ring.append(self._vert(Vector(base) + (side * math.cos(ang) + n * math.sin(ang)) * radius, phase, 0))
        tip_pos = Vector(base) + d * length - n * hook * length
        tip = self._vert(tip_pos, phase, 0)
        for j in range(sides):
            k = (j + 1) % sides
            self.faces.append((ring[j], tip, ring[k]))
            self.uvs.append([(u0 + (u1 - u0) * j / sides, v0), ((u0 + u1) * 0.5, v1),
                             (u0 + (u1 - u0) * (j + 1) / sides, v0)])

    def sphere(self, center, radius, rect, segs=6, rings=4, stretch=1.0, axis=Vector((0, 0, 1)),
               phase=0.0):
        """Low-poly UV sphere (berries); u around, v from the attached pole to the free pole."""
        d, side, n = frame(axis)
        u0, u1, v0, v1 = rect
        pole_a = self._vert(Vector(center) - d * radius * stretch, phase, 0.2)
        grid = []
        for i in range(1, rings):
            th = math.pi * i / rings
            row = []
            for j in range(segs):
                ph = 2 * math.pi * j / segs
                p = (Vector(center) - d * math.cos(th) * radius * stretch +
                     (side * math.cos(ph) + n * math.sin(ph)) * math.sin(th) * radius)
                row.append(self._vert(p, phase, 0.2))
            grid.append(row)
        pole_b = self._vert(Vector(center) + d * radius * stretch, phase, 0.2)

        def uv(i, j):
            return (u0 + (u1 - u0) * j / segs, v0 + (v1 - v0) * i / rings)
        for j in range(segs):
            k = (j + 1) % segs
            self.faces.append((pole_a, grid[0][j], grid[0][k]))
            self.uvs.append([((u0 + u1) * 0.5, v0), uv(1, j), uv(1, j + 1)])
        for i in range(rings - 2):
            for j in range(segs):
                k = (j + 1) % segs
                self.faces.append((grid[i][j], grid[i + 1][j], grid[i + 1][k], grid[i][k]))
                self.uvs.append([uv(i + 1, j), uv(i + 2, j), uv(i + 2, j + 1), uv(i + 1, j + 1)])
        for j in range(segs):
            k = (j + 1) % segs
            self.faces.append((grid[-1][j], pole_b, grid[-1][k]))
            self.uvs.append([uv(rings - 1, j), ((u0 + u1) * 0.5, v1), uv(rings - 1, j + 1)])

    def build(self, name, material):
        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata(self.verts, [], self.faces)
        mesh.update()
        uv = mesh.uv_layers.new(name="UVMap")
        flat = [c for face in self.uvs for loop in face for c in loop]
        uv.data.foreach_set("uv", flat)
        attr = mesh.attributes.new("Wind", "BYTE_COLOR", "POINT")
        attr.data.foreach_set("color_srgb", [c for w in self.wind for c in w])
        mesh.materials.append(material)
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        return obj


def finish_lods(kit, batches, base_name, material, smooth_angle=65.0):
    """Build one SM_ mesh per LOD batch (LOD0 first) with a shared origin at the bottom
    centre of LOD0, smooth shading and the Wind colour attribute."""
    objects = []
    for lod, batch in enumerate(batches):
        name = base_name if lod == 0 else f"{base_name}_LOD{lod}"
        obj = batch.build(name, material)
        obj = kit.join([obj], name, pivot=None, unwrap=False, reshade=True, smooth_angle=smooth_angle)
        objects.append(obj)
    lo, hi = kit.bounds(objects[0])
    shift = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, 0.0))
    for obj in objects:
        obj.data.transform(Matrix.Translation(-shift))
        obj["homestead_shift"] = list(shift)
        obj.data.color_attributes.active_color = obj.data.color_attributes["Wind"]
        obj.data.update()
    return objects


def lod_report(objects):
    tris = [sum(len(p.vertices) - 2 for p in o.data.polygons) for o in objects]
    return {o.name: {"triangles": t, "ratio": round(t / max(tris[0], 1), 3)} for o, t in zip(objects, tris)}
