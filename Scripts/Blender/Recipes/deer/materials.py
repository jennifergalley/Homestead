"""Procedural PBR materials for the deer remains (baked by the pipeline).

Colours are linear albedo. Weathered bone in open woodland (Behrensmeyer stages 1-3):
greasy ivory-tan in the first year, then bleached grey-white on the sky side with fine
longitudinal cracks and flaking, green algae and moss where it touches damp ground and
soil staining underneath. Mule-deer winter coat: grizzled grey-brown guard hairs (dark
tips over a pale band), a darker dorsal line, cream belly, white rump patch and a white
tail with a black tip. Dried rawhide under the fur is dark tan and translucent at thin
edges. None of these use subsurface: the baked set inherits the largest weight."""
import homestead_materials as M

Graph = M.Graph


def _geometry(g):
    node = g.node("ShaderNodeNewGeometry")
    return node.outputs["Position"], node.outputs["Normal"]


def bone(name, bleach=0.5, moss=0.3, cracks=0.5, seed=0.0):
    """Weathered bone on part-local pcoord (long bones have z along the shaft), with
    world-space ground staining, algae and moss near the ground and on shaded faces."""
    g = Graph(name)
    p = g.coord()
    seeded = g.vmath("ADD", p, (seed * 0.37, seed * 0.21, seed * 0.13))
    pos, nrm = _geometry(g)
    _, _, wz = g.separate(pos)
    _, _, nz = g.separate(nrm)
    fresh = (0.235, 0.195, 0.14)
    bleached = (0.265, 0.255, 0.232)
    base = tuple(f + (b - f) * bleach for f, b in zip(fresh, bleached))
    grey = tuple(c * 0.55 for c in (0.3, 0.285, 0.26))
    tone = g.noise(seeded, scale=18.0, detail=5.0, roughness=0.6).outputs["Fac"]
    color = g.mix(grey, base, g.remap(tone, 0.35, 0.65))
    sun = g.remap(nz, -0.2, 0.8, 0.0, bleach * 0.7)
    color = g.mix(color, (0.3, 0.292, 0.272), g.math("MULTIPLY", sun, 0.5))
    stain = g.noise(g.vmath("ADD", seeded, (3.0, 7.0, 1.0)), scale=7.0, detail=4.0).outputs["Fac"]
    color = g.mix(color, (0.22, 0.16, 0.095), g.remap(stain, 0.5, 0.72, 0.0, 0.55 * (1.0 - bleach) + 0.12))
    # Fine longitudinal weathering cracks (along pcoord z) and flaked cortex patches.
    stretched = g.vmath("MULTIPLY", seeded, (1.0, 1.0, 0.28))
    edges = g.voronoi(stretched, scale=70.0, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack_zone = g.remap(g.noise(seeded, scale=9.0, detail=3.0).outputs["Fac"], 0.4, 0.55, 0.0, cracks)
    crack = g.math("MULTIPLY", g.remap(edges, 0.05, 0.0), crack_zone)
    tone2 = g.noise(seeded, scale=60.0, detail=6.0, roughness=0.65).outputs["Fac"]
    color = g.mix(color, tuple(c * 0.78 for c in base), g.remap(tone2, 0.52, 0.7, 0.0, 0.7))
    flake = g.math("MULTIPLY", g.remap(g.noise(g.vmath("ADD", seeded, (9.0, 2.0, 5.0)), scale=26.0,
                                               detail=4.0).outputs["Fac"], 0.62, 0.66), cracks)
    color = g.mix(color, tuple(c * 1.07 for c in base), g.math("MULTIPLY", flake, 0.6))
    pores = g.voronoi(seeded, scale=1400.0, feature="F1").outputs["Distance"]
    pit = g.remap(pores, 0.12, 0.03)
    color = g.mix(color, (0.13, 0.11, 0.085), g.math("MAXIMUM", g.math("MULTIPLY", pit, 0.35),
                                                        g.math("MULTIPLY", crack, 0.85)))
    cavity = g.math("SUBTRACT", 1.0, g.ao(distance=0.012, samples=16))
    color = g.mix(color, (0.07, 0.055, 0.04), g.remap(cavity, 0.2, 0.7, 0.0, 0.5))
    # Soil stain from the damp ground, and algae/moss on the ground side and shaded faces.
    low = g.remap(wz, 0.012, 0.001)
    grime = g.noise(pos, scale=45.0, detail=5.0).outputs["Fac"]
    color = g.mix(color, (0.085, 0.066, 0.045), g.math("MULTIPLY", low, g.remap(grime, 0.3, 0.6, 0.35, 0.8)))
    damp = g.math("MAXIMUM", g.remap(wz, 0.009, 0.0015), g.remap(nz, -0.1, -0.7, 0.0, 0.7))
    patches = g.noise(pos, scale=22.0, detail=6.0, roughness=0.7).outputs["Fac"]
    growth = g.math("MULTIPLY", g.remap(g.math("ADD", patches, g.math("MULTIPLY", damp, 0.35)),
                                        0.68 - 0.18 * moss, 0.76 - 0.18 * moss), moss)
    algae = g.math("MULTIPLY", g.remap(patches, 0.45, 0.62), g.math("MULTIPLY", damp, 0.5 * moss))
    color = g.mix(color, (0.07, 0.085, 0.035), algae)
    tips = g.noise(pos, scale=900.0, detail=2.0).outputs["Fac"]
    moss_col = g.mix((0.028, 0.05, 0.012), (0.085, 0.11, 0.03), g.remap(tips, 0.35, 0.7))
    color = g.mix(color, moss_col, growth)
    g.set("Base Color", color)
    rough = g.remap(tone, 0.3, 0.7, 0.66, 0.8)
    rough = g.math("MAXIMUM", rough, g.math("MULTIPLY", g.math("ADD", growth, crack), 0.92))
    g.set("Roughness", rough)
    fine = g.noise(seeded, scale=260.0, detail=4.0).outputs["Fac"]
    height = g.math("ADD", g.math("MULTIPLY", fine, 0.25), g.math("MULTIPLY", flake, 0.3))
    height = g.math("SUBTRACT", height, g.math("ADD", g.math("MULTIPLY", crack, 0.7), g.math("MULTIPLY", pit, 0.25)))
    height = g.math("ADD", height, g.math("MULTIPLY", growth, g.math("ADD", 0.6, g.math("MULTIPLY", tips, 0.8))))
    height = g.math("ADD", height, g.math("MULTIPLY", tone2, 0.35))
    g.set("Normal", g.bump(height, strength=0.85, distance=0.0012))
    return g.mat


def antler(name, bleach=0.3, seed=0.0):
    """Antler on tube pcoord (x, y around the beam, z along it): brown with deep
    longitudinal guttering and pearled ridges, rubbed ivory tips (thin radius), paler and
    greyer on the sky side."""
    g = Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    pos, nrm = _geometry(g)
    _, _, nz = g.separate(nrm)
    radius = g.vmath("LENGTH", g.combine(x, y, 0.0))
    angle = g.math("ARCTAN2", y, x)
    wob = g.noise(g.vmath("ADD", p, (seed, seed, seed)), scale=40.0, detail=3.0).outputs["Fac"]
    groove = g.math("ABSOLUTE", g.math("SINE", g.math("ADD", g.math("MULTIPLY", angle, 7.0),
                                                      g.math("MULTIPLY", wob, 5.0))))
    ridge = g.remap(groove, 0.3, 1.0)
    pearls = g.voronoi(g.vmath("MULTIPLY", p, (1.0, 1.0, 0.5)), scale=520.0, feature="F1").outputs["Distance"]
    pearl = g.math("MULTIPLY", g.remap(pearls, 0.35, 0.05), g.remap(z, 0.06, 0.0))
    tip = g.remap(radius, 0.0052, 0.0028)
    color = g.mix((0.05, 0.034, 0.022), (0.2, 0.135, 0.075), ridge)
    color = g.mix(color, (0.27, 0.2, 0.13), g.math("MULTIPLY", pearl, 0.7))
    color = g.mix(color, (0.42, 0.38, 0.31), tip)
    color = g.mix(color, (0.3, 0.27, 0.23), g.remap(nz, 0.0, 0.9, 0.0, bleach))
    cavity = g.math("SUBTRACT", 1.0, g.ao(distance=0.01, samples=16))
    color = g.mix(color, (0.03, 0.022, 0.015), g.remap(cavity, 0.1, 0.5, 0.0, 0.6))
    g.set("Base Color", color)
    g.set("Roughness", g.math("SUBTRACT", g.remap(ridge, 0.0, 1.0, 0.82, 0.62), g.math("MULTIPLY", tip, 0.18)))
    fine = g.noise(p, scale=700.0, detail=3.0).outputs["Fac"]
    height = g.math("ADD", g.math("MULTIPLY", ridge, 0.8), g.math("ADD", pearl, g.math("MULTIPLY", fine, 0.2)))
    g.set("Normal", g.bump(height, strength=0.6, distance=0.0012))
    return g.mat


def hoof(name, seed=0.0):
    """Dry keratin toe shells: near-black grey-brown with growth lines parallel to the
    coronary band, scuffed paler toward the tip, caked soil on the sole."""
    g = Graph(name)
    p = g.coord()
    x, _, z = g.separate(p)
    lines = g.noise(g.combine(g.math("MULTIPLY", x, 40.0), g.math("MULTIPLY", z, 900.0), seed), scale=1.0,
                    detail=5.0, roughness=0.6).outputs["Fac"]
    color = g.mix((0.024, 0.021, 0.018), (0.075, 0.064, 0.05), g.remap(lines, 0.35, 0.7))
    color = g.mix(color, (0.13, 0.11, 0.085), g.remap(x, 0.03, 0.05, 0.0, 0.5))
    sole = g.remap(z, 0.004, 0.0006)
    color = g.mix(color, (0.09, 0.07, 0.048), sole)
    g.set("Base Color", color)
    g.set("Roughness", g.math("ADD", g.remap(lines, 0.3, 0.7, 0.5, 0.62), g.math("MULTIPLY", sole, 0.3)))
    g.set("Normal", g.bump(lines, strength=0.3, distance=0.0006))
    return g.mat


def teeth(name):
    """Worn selenodont cheek teeth: brown-grey enamel crests, cementum and black
    staining in the cusp hollows."""
    g = Graph(name)
    p = g.coord()
    enamel = g.noise(p, scale=300.0, detail=4.0).outputs["Fac"]
    color = g.mix((0.13, 0.11, 0.08), (0.27, 0.24, 0.19), g.remap(enamel, 0.3, 0.7))
    cavity = g.math("SUBTRACT", 1.0, g.ao(distance=0.004, samples=16))
    color = g.mix(color, (0.018, 0.015, 0.012), g.remap(cavity, 0.1, 0.5))
    g.set("Base Color", color)
    g.set("Roughness", g.remap(enamel, 0.3, 0.7, 0.38, 0.55))
    g.set("Normal", g.bump(enamel, strength=0.2, distance=0.0004))
    return g.mat


def fur(name, zones=True, seed=0.0, rump_from=None, length=1.0, dark=(0.03, 0.021, 0.014),
        pale=(0.175, 0.125, 0.078), tip_from=None):
    """Dried winter deer coat. pcoord = (across, 0 or around, along the hair flow) in
    meters: grizzled strands and clumps run along z. ``zones``: pcoord x is the distance
    from the dorsal line (dark stripe near 0, cream belly beyond ~0.22 m) and z the
    arclength from the neck (white rump patch past ``rump_from``). ``tip_from`` darkens
    the hair beyond that z to black (the mule deer's tail tip)."""
    g = Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 0.31, seed * 0.17, seed * 0.07))
    pos, _ = _geometry(g)
    _, _, wz = g.separate(pos)
    warp = g.noise(g.vmath("MULTIPLY", seeded, (1.0, 1.0, 0.5)), scale=18.0, detail=3.0).outputs["Color"]
    flow = g.vmath("ADD", seeded, g.vmath("MULTIPLY", g.vmath("SUBTRACT", warp, (0.5, 0.5, 0.5)),
                                         (0.012, 0.012, 0.004)))
    clumps = g.voronoi(g.vmath("MULTIPLY", flow, (1.0, 1.0, 0.36)), scale=120.0)
    clump_tone = g.channel(clumps.outputs["Color"], 0)
    clump_d = clumps.outputs["Distance"]
    strands = g.noise(g.vmath("MULTIPLY", flow, (1.0, 1.0, 0.06)), scale=1300.0, detail=3.0).outputs["Fac"]
    coarse = g.noise(g.vmath("MULTIPLY", flow, (1.0, 1.0, 0.12)), scale=380.0, detail=3.0).outputs["Fac"]
    grizzle = g.noise(g.vmath("MULTIPLY", flow, (1.0, 1.0, 0.25)), scale=420.0, detail=2.0).outputs["Fac"]
    coat = g.mix(dark, pale, g.remap(g.math("ADD", g.math("MULTIPLY", grizzle, 0.7),
                                                g.math("MULTIPLY", clump_tone, 0.3)), 0.36, 0.66))
    if zones:
        stripe = g.remap(g.math("ABSOLUTE", x), 0.05, 0.015)
        coat = g.mix(coat, g.mix(dark, (0.07, 0.052, 0.038), grizzle), g.math("MULTIPLY", stripe, 0.75))
        belly = g.remap(x, 0.2, 0.28)
        coat = g.mix(coat, g.mix((0.2, 0.17, 0.13), (0.34, 0.3, 0.24), strands), belly)
        if rump_from is not None:
            rump = g.math("MULTIPLY", g.remap(z, rump_from, rump_from + 0.06), g.remap(x, 0.2, 0.08))
            coat = g.mix(coat, g.mix((0.26, 0.24, 0.2), (0.38, 0.35, 0.3), strands), rump)
    if tip_from is not None:
        coat = g.mix(coat, g.mix((0.012, 0.01, 0.009), (0.04, 0.034, 0.028), strands), g.remap(z, tip_from, tip_from + 0.03))
    coat = g.mix(coat, tuple(c * 0.55 for c in pale), g.remap(clump_d, 0.35, 0.8, 0.0, 0.5))
    # Matted, dirty and bald patches (the rawhide shows grey-brown through).
    mat_zone = g.noise(seeded, scale=5.0 / length, detail=4.0).outputs["Fac"]
    matted = g.remap(mat_zone, 0.5, 0.62)
    coat = g.mix(coat, (0.055, 0.042, 0.03), g.math("MULTIPLY", matted, 0.35))
    bald_n = g.noise(g.vmath("ADD", seeded, (4.0, 1.0, 9.0)), scale=9.0, detail=5.0, roughness=0.65).outputs["Fac"]
    bald = g.remap(bald_n, 0.71, 0.745)
    leather = g.mix((0.075, 0.052, 0.034), (0.13, 0.095, 0.062), g.noise(seeded, scale=90.0).outputs["Fac"])
    coat = g.mix(coat, leather, bald)
    dirt_n = g.noise(g.vmath("ADD", pos, (2.0, 5.0, 3.0)), scale=16.0, detail=5.0).outputs["Fac"]
    dirt = g.math("MAXIMUM", g.remap(wz, 0.02, 0.002, 0.0, 0.85),
                  g.remap(dirt_n, 0.64, 0.72, 0.0, 0.55))
    coat = g.mix(coat, (0.075, 0.058, 0.04), dirt)
    cavity = g.math("SUBTRACT", 1.0, g.ao(distance=0.01, samples=16))
    coat = g.mix(coat, (0.02, 0.016, 0.012), g.remap(cavity, 0.1, 0.55, 0.0, 0.7))
    g.set("Base Color", coat)
    rough = g.math("SUBTRACT", g.remap(strands, 0.3, 0.7, 0.86, 0.7), g.math("MULTIPLY", matted, 0.12))
    g.set("Roughness", g.math("MAXIMUM", rough, g.math("MULTIPLY", dirt, 0.9)))
    g.set("Sheen Weight", 0.35)
    g.set("Sheen Roughness", 0.45)
    dome = g.remap(clump_d, 0.0, 0.85, 1.0, 0.0)
    coat = g.mix(coat, tuple(c * 0.5 for c in pale), g.remap(strands, 0.55, 0.3, 0.0, 0.55))
    g.set("Base Color", coat)
    height = g.math("ADD", g.math("MULTIPLY", dome, g.math("SUBTRACT", 1.0, g.math("MULTIPLY", matted, 0.7))),
                    g.math("ADD", g.math("MULTIPLY", strands, 1.1), g.math("MULTIPLY", coarse, 0.6)))
    height = g.math("MULTIPLY", height, g.math("SUBTRACT", 1.0, g.math("MULTIPLY", bald, 0.8)))
    g.set("Normal", g.bump(height, strength=1.0, distance=0.0016))
    return g.mat


def leather(name, seed=0.0):
    """Dried rawhide (flesh side and torn edges): dark tan, mottled, finely wrinkled,
    paler where it is thinnest, faint fibrous texture."""
    g = Graph(name)
    p = g.coord()
    seeded = g.vmath("ADD", p, (seed * 0.4, seed * 0.2, seed * 0.3))
    mottle = g.noise(seeded, scale=40.0, detail=5.0).outputs["Fac"]
    wrinkle = g.noise(g.vmath("MULTIPLY", seeded, (1.0, 1.0, 2.5)), scale=160.0, detail=3.0, distortion=0.5).outputs["Fac"]
    fibre = g.noise(g.vmath("MULTIPLY", seeded, (3.0, 3.0, 1.0)), scale=600.0, detail=2.0).outputs["Fac"]
    color = g.mix((0.06, 0.042, 0.028), (0.16, 0.115, 0.072), g.remap(mottle, 0.3, 0.7))
    color = g.mix(color, (0.21, 0.16, 0.1), g.remap(fibre, 0.62, 0.7, 0.0, 0.4))
    color = g.mix(color, (0.07, 0.055, 0.04), g.remap(g.noise(seeded, scale=12.0).outputs["Fac"], 0.6, 0.7, 0.0, 0.5))
    g.set("Base Color", color)
    g.set("Roughness", g.remap(wrinkle, 0.3, 0.7, 0.48, 0.66))
    g.set("Normal", g.bump(g.math("ADD", wrinkle, g.math("MULTIPLY", fibre, 0.3)), strength=0.45, distance=0.0009))
    return g.mat


def leaf(name):
    """Dry fallen oak leaves on the leaf-mesh pcoord (u across 0..1 with the midrib at
    0.5, v base 0 -> tip 1, z = per-leaf random seed): tan to dark brown per leaf,
    paler midrib and veins, darker margins and decay spots."""
    g = Graph(name)
    p = g.coord()
    u, v, seed = g.separate(p)
    per_leaf = g.noise(g.combine(seed, seed, 0.3), scale=1.0, detail=1.0).outputs["Fac"]
    base = g.ramp(per_leaf, [(0.25, (0.05, 0.032, 0.018)), (0.5, (0.105, 0.066, 0.034)), (0.75, (0.17, 0.115, 0.062))])
    across = g.math("ABSOLUTE", g.math("SUBTRACT", u, 0.5))
    midrib = g.remap(across, 0.025, 0.0)
    veins = g.math("ABSOLUTE", g.math("SINE", g.math("MULTIPLY", g.math("SUBTRACT", v, g.math("MULTIPLY", across, 0.9)),
                                                     42.0)))
    vein = g.math("MULTIPLY", g.remap(veins, 0.12, 0.0), g.remap(across, 0.48, 0.3))
    color = g.mix(base, (0.22, 0.165, 0.105), g.math("MAXIMUM", g.math("MULTIPLY", midrib, 0.6), g.math("MULTIPLY", vein, 0.35)))
    edge = g.remap(across, 0.4, 0.5)
    color = g.mix(color, (0.05, 0.03, 0.017), g.math("MULTIPLY", edge, 0.6))
    spots_v = g.voronoi(g.combine(g.math("MULTIPLY", u, 9.0), g.math("MULTIPLY", v, 14.0), g.math("MULTIPLY", seed, 7.0)),
                        scale=1.0, feature="F1").outputs["Distance"]
    color = g.mix(color, (0.045, 0.028, 0.016), g.remap(spots_v, 0.2, 0.1, 0.0, 0.6))
    grey = g.remap(g.noise(g.combine(seed, 0.7, 1.9), scale=1.0).outputs["Fac"], 0.62, 0.68)
    color = g.mix(color, (0.19, 0.175, 0.15), g.math("MULTIPLY", grey, 0.7))
    g.set("Base Color", color)
    g.set("Roughness", 0.72)
    height = g.math("SUBTRACT", g.math("MULTIPLY", g.math("ADD", midrib, vein), 0.6),
                    g.math("MULTIPLY", g.noise(p, scale=80.0).outputs["Fac"], 0.2))
    g.set("Normal", g.bump(height, strength=0.3, distance=0.0005))
    return g.mat


def moss(name, seed=0.0):
    """Cushion moss on world-ish pcoord: dense 1 mm stems in olive to yellow-green
    tufts, browned in the gaps."""
    g = Graph(name)
    p = g.coord()
    seeded = g.vmath("ADD", p, (seed, seed * 0.5, seed * 0.3))
    stems = g.voronoi(seeded, scale=1100.0, feature="F1").outputs["Distance"]
    tufts = g.noise(seeded, scale=120.0, detail=5.0, roughness=0.7).outputs["Fac"]
    tip = g.remap(stems, 0.45, 0.05)
    color = g.mix((0.022, 0.036, 0.01), (0.07, 0.095, 0.024), g.remap(tufts, 0.35, 0.7))
    color = g.mix(color, (0.12, 0.13, 0.04), g.math("MULTIPLY", tip, 0.45))
    color = g.mix(color, (0.05, 0.038, 0.018), g.remap(tufts, 0.32, 0.22))
    g.set("Base Color", color)
    g.set("Roughness", 0.86)
    g.set("Normal", g.bump(g.math("ADD", g.math("MULTIPLY", tip, 0.7), tufts), strength=0.8, distance=0.0015))
    return g.mat
