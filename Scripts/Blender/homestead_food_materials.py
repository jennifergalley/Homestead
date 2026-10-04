"""Original prepared-food source shaders, isolated from held fish materials.

Usage: import homestead_food_materials as food; food.potato_skin(name, seed)
No meal baking or Unreal material admission is enabled here.
"""
import hashlib
from pathlib import Path

import bpy

from homestead_materials import Graph

SOURCE_SHA256 = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
SKIN_NOISE_SCALES = (170, 65, 1600, 850)
FLESH_NOISE_SCALES = (230, 1300)
CARROT_FIBRE_RELIEF_M = .00015
RAW_MYOMERE_SPACING_M = .0054
RAW_MYOMERE_SLOPE = .45
RAW_MYOMERE_CURVE_M = .0008
RAW_FASCIA_WIDTH_M = .00016


def potato_skin(name: str, seed: float) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    coordinates = graph.coord()
    _, _, height = graph.separate(coordinates)
    point = graph.vmath("ADD", coordinates, (seed * .037, seed * .019, seed * .071))
    roasting = graph.noise(point, scale=SKIN_NOISE_SCALES[0], detail=3, roughness=.67).outputs["Fac"]
    colour = graph.ramp(roasting, [
        (.15, (.080, .040, .014)), (.46, (.150, .080, .027)),
        (.75, (.20, .120, .040)), (.92, (.13, .066, .020)),
    ], "EASE")
    contact = graph.remap(height, -.006, -.024)
    ember = graph.noise(point, scale=SKIN_NOISE_SCALES[1], detail=2).outputs["Fac"]
    scorch = graph.math("MAXIMUM",
                        graph.math("MULTIPLY", contact, graph.remap(ember, .39, .64)),
                        graph.remap(ember, .58, .75, 0, .82))
    colour = graph.mix(colour, (.009, .006, .004), scorch)
    pores = graph.noise(point, scale=SKIN_NOISE_SCALES[2], detail=2).outputs["Fac"]
    wrinkles = graph.noise(graph.scale(point, (1, .16, 1)),
                           scale=SKIN_NOISE_SCALES[3], detail=3, roughness=.65).outputs["Fac"]
    graph.set("Base Color", colour)
    graph.set("Roughness", graph.remap(roasting, .25, .75, .88, .74))
    relief = graph.math("ADD", graph.math("MULTIPLY", pores, .3), wrinkles)
    graph.set("Normal", graph.bump(relief, strength=.24, distance=.00012))
    return graph.mat


def potato_flesh(name: str, seed: float) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.vmath("ADD", graph.coord(), (seed * .023, seed * .041, seed * .017))
    crumb = graph.noise(point, scale=FLESH_NOISE_SCALES[0], detail=3).outputs["Fac"]
    colour = graph.ramp(crumb, [
        (.15, (.40, .33, .20)), (.43, (.55, .47, .31)),
        (.70, (.61, .54, .39)), (.87, (.56, .48, .32)),
    ], "EASE")
    toast = graph.remap(crumb, .69, .85, 0, .65)
    graph.set("Base Color", graph.mix(colour, (.24, .12, .035), toast))
    graph.set("Roughness", graph.remap(crumb, .25, .75, .75, .59))
    graph.set("Subsurface Weight", .09)
    graph.set("Subsurface Radius", (.003, .0018, .0008))
    graph.set("Subsurface Scale", .05)
    fines = graph.noise(point, scale=FLESH_NOISE_SCALES[1], detail=2).outputs["Fac"]
    graph.set("Normal", graph.bump(fines, strength=.30, distance=.00016))
    return graph.mat


def roasted_turnip(name: str, seed: float, skin: bool) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.coord()
    _, shoulder, _ = graph.separate(point)
    shifted = graph.vmath("ADD", point, (seed * .029, seed * .047, seed * .013))
    cooking = graph.noise(shifted, scale=140, detail=3).outputs["Fac"]
    colour = graph.ramp(cooking, [
        (.15, (.40, .30, .14)), (.50, (.47, .37, .21)),
        (.85, (.52, .43, .27)),
    ], "EASE")
    heat = graph.noise(shifted, scale=35, detail=2).outputs["Fac"]
    sear = graph.remap(heat, .27, .69, .45, .95)
    crust = graph.ramp(cooking, [
        (.18, (.13, .052, .012)), (.50, (.27, .135, .034)),
        (.82, (.35, .205, .075)),
    ], "EASE")
    colour = graph.mix(colour, crust, sear)
    if skin:
        crown = graph.remap(shoulder, .008, .027)
        colour = graph.mix(colour, (.065, .022, .026), crown)
        scorch = graph.remap(heat, .63, .79, 0, .70)
    else:
        scorch = graph.remap(heat, .67, .84, 0, .72)
    graph.set("Base Color", graph.mix(colour, (.075, .023, .005), scorch))
    fibres = graph.noise(graph.scale(shifted, (1, .24, 1)),
                        scale=1400, detail=2).outputs["Fac"]
    graph.set("Roughness", graph.remap(cooking, .25, .75, .65, .44) if not skin else .74)
    graph.set("Subsurface Weight", .055)
    graph.set("Subsurface Radius", (.002, .0014, .0007))
    graph.set("Subsurface Scale", .05)
    graph.set("Normal", graph.bump(fibres, strength=.16, distance=.000045))
    return graph.mat


def earthenware(name: str, seed: float) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.vmath("ADD", graph.coord(), (seed * .13, seed * .07, 0))
    firing = graph.noise(point, scale=75, detail=3).outputs["Fac"]
    graph.set("Base Color", graph.ramp(firing, [
        (.15, (.11, .029, .010)), (.45, (.23, .075, .031)),
        (.80, (.31, .116, .050)),
    ], "EASE"))
    grit = graph.noise(point, scale=1800, detail=2).outputs["Fac"]
    graph.set("Roughness", .79)
    graph.set("Normal", graph.bump(grit, strength=.19, distance=.000065))
    return graph.mat


def stewed_carrot(name: str, seed: float, skin: bool) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.coord()
    x, _, z = graph.separate(point)
    shifted = graph.vmath("ADD", point, (seed * .031, seed * .053, seed * .017))
    pigment = graph.noise(shifted, scale=350, detail=3).outputs["Fac"]
    colour = graph.ramp(pigment, [
        (.18, (.24, .069, .008)), (.50, (.35, .115, .020)),
        (.83, (.42, .170, .044)),
    ], "EASE")
    if not skin:
        radial = graph.vmath("LENGTH", graph.combine(x, 0, z))
        radial = graph.math("ADD", radial,
                            graph.math("MULTIPLY", graph.math("SUBTRACT", pigment, .5), .0013))
        core = graph.remap(radial, .004, .007, .68, 0)
        colour = graph.mix(colour, (.42, .200, .068), core)
    graph.set("Base Color", colour)
    fibres = graph.noise(graph.scale(shifted, (1, .12, 1)),
                        scale=1900, detail=2).outputs["Fac"]
    graph.set("Roughness", graph.remap(pigment, .25, .75, .44, .29))
    graph.set("Subsurface Weight", .10)
    graph.set("Subsurface Radius", (.003, .0015, .0005))
    graph.set("Subsurface Scale", .05)
    graph.set("Normal", graph.bump(fibres, strength=.25, distance=CARROT_FIBRE_RELIEF_M))
    return graph.mat


def cooking_liquid(name: str) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    variation = graph.noise(graph.coord(), scale=90, detail=2).outputs["Fac"]
    graph.set("Base Color", graph.ramp(variation, [
        (.20, (.058, .024, .005)), (.80, (.12, .052, .014)),
    ]))
    graph.set("Roughness", .19)
    graph.set("IOR", 1.333)
    graph.set("Normal", graph.bump(variation, strength=.08, distance=.00002))
    return graph.mat


def cooked_broad_bean(name: str, seed: float) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.coord()
    shifted = graph.vmath("ADD", point, (seed * .037, seed * .019, seed * .061))
    pigment = graph.noise(shifted, scale=180, detail=3).outputs["Fac"]
    colour = graph.ramp(pigment, [
        (.15, (.16, .21, .057)), (.48, (.29, .34, .13)),
        (.85, (.39, .41, .22)),
    ], "EASE")
    colour = graph.mix(colour, (.17, .23, .065), (seed % 5) * .045)
    scar_point = graph.scale(graph.vmath("SUBTRACT", point, (-.0061, .0006, .0016)),
                             (850, 220, 550))
    scar = graph.remap(graph.vmath("LENGTH", scar_point), .40, 1.2, .95, 0)
    graph.set("Base Color", graph.mix(colour, (.42, .39, .24), scar))
    wrinkles = graph.noise(graph.scale(shifted, (1, .35, 1)),
                          scale=1100, detail=2).outputs["Fac"]
    pores = graph.noise(shifted, scale=2600, detail=2).outputs["Fac"]
    relief = graph.math("ADD", wrinkles, graph.math("MULTIPLY", pores, .16))
    graph.set("Roughness", graph.remap(pigment, .2, .8, .54, .39))
    graph.set("Subsurface Weight", .12)
    graph.set("Subsurface Radius", (.002, .0017, .0007))
    graph.set("Subsurface Scale", .05)
    graph.set("Normal", graph.bump(relief, strength=.24, distance=.00010))
    return graph.mat


def chopped_meadow_herb(name: str) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.coord()
    pigment = graph.noise(point, scale=650, detail=2).outputs["Fac"]
    graph.set("Base Color", graph.ramp(pigment, [
        (.2, (.018, .038, .005)), (.8, (.055, .091, .018)),
    ]))
    veins = graph.noise(graph.scale(point, (1, .15, 1)),
                       scale=2200, detail=2).outputs["Fac"]
    graph.set("Roughness", .58)
    graph.set("Normal", graph.bump(veins, strength=.10, distance=.00002))
    return graph.mat


def stewed_cabbage(name: str, seed: float) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.coord()
    x, _, _ = graph.separate(point)
    shifted = graph.vmath("ADD", point, (seed * .019, seed * .037, seed * .041))
    pigment = graph.noise(shifted, scale=240, detail=3).outputs["Fac"]
    colour = graph.ramp(pigment, [
        (.16, (.12, .16, .047)), (.5, (.20, .25, .096)),
        (.84, (.31, .34, .17)),
    ], "EASE")
    midrib = graph.remap(graph.math("ABSOLUTE", x), .0005, .0022, .6, 0)
    graph.set("Base Color", graph.mix(colour, (.40, .42, .25), midrib))
    fines = graph.noise(graph.scale(shifted, (1, .23, 1)),
                       scale=1700, detail=2).outputs["Fac"]
    graph.set("Roughness", graph.remap(pigment, .2, .8, .49, .32))
    graph.set("Subsurface Weight", .14)
    graph.set("Subsurface Radius", (.0015, .0010, .0004))
    graph.set("Subsurface Scale", .05)
    graph.set("Normal", graph.bump(fines, strength=.15, distance=.000035))
    return graph.mat


def stewed_potato(name: str, seed: float) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.vmath("ADD", graph.coord(), (seed * .029, seed * .047, seed * .013))
    crumb = graph.noise(point, scale=310, detail=3).outputs["Fac"]
    graph.set("Base Color", graph.ramp(crumb, [
        (.15, (.39, .33, .21)), (.5, (.52, .47, .32)),
        (.85, (.59, .55, .41)),
    ], "EASE"))
    pores = graph.noise(point, scale=1900, detail=2).outputs["Fac"]
    graph.set("Roughness", graph.remap(crumb, .2, .8, .61, .44))
    graph.set("Subsurface Weight", .12)
    graph.set("Subsurface Radius", (.0027, .0016, .0007))
    graph.set("Subsurface Scale", .05)
    graph.set("Normal", graph.bump(pores, strength=.23, distance=.00010))
    return graph.mat


def vegetable_broth(name: str) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    variation = graph.noise(graph.coord(), scale=120, detail=2).outputs["Fac"]
    graph.set("Base Color", graph.ramp(variation, [
        (.2, (.12, .115, .053)), (.8, (.22, .21, .12)),
    ]))
    graph.set("Roughness", .22)
    graph.set("IOR", 1.333)
    graph.set("Transmission Weight", .45)
    graph.set("Normal", graph.bump(variation, strength=.08, distance=.000025))
    return graph.mat


def maple_eating_spoon(name: str) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.scale(graph.coord(), (1, .045, 1))
    grain = graph.noise(point, scale=720, detail=3).outputs["Fac"]
    graph.set("Base Color", graph.ramp(grain, [
        (.18, (.12, .067, .025)), (.48, (.23, .15, .064)),
        (.82, (.31, .22, .11)),
    ], "EASE"))
    fibres = graph.noise(point, scale=2300, detail=2).outputs["Fac"]
    graph.set("Roughness", graph.remap(grain, .2, .8, .57, .41))
    graph.set("Normal", graph.bump(fibres, strength=.15, distance=.000035))
    return graph.mat


def reduced_blackberry(name: str, seed: float, juice: bool = False,
                      pulp: bool = False) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.vmath("ADD", graph.coord(), (seed * .019, seed * .031, seed * .047))
    pigment = graph.noise(point, scale=480, detail=3).outputs["Fac"]
    colour = graph.ramp(pigment, [
        (.16, (.008, .0015, .005)), (.50, (.025, .004, .017)),
        (.84, (.048, .008, .028)),
    ])
    graph.set("Base Color", graph.mix(colour, (.058, .009, .029), .45) if pulp else colour)
    graph.set("Roughness", .34 if pulp else
              graph.remap(pigment, .2, .8, .20, .29) if not juice else .18)
    graph.set("IOR", 1.38)
    graph.set("Subsurface Weight", .10 if not juice else .05)
    graph.set("Subsurface Radius", (.0012, .0006, .0008))
    graph.set("Subsurface Scale", .05)
    fines = graph.noise(point, scale=3200, detail=2).outputs["Fac"]
    graph.set("Normal", graph.bump(fines, strength=.13, distance=.000025))
    return graph.mat


def fruit_bowl_glaze(name: str) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.coord()
    firing = graph.noise(point, scale=170, detail=3).outputs["Fac"]
    graph.set("Base Color", graph.ramp(firing, [
        (.15, (.095, .112, .097)), (.5, (.16, .18, .151)),
        (.85, (.205, .224, .180)),
    ]))
    graph.set("Roughness", graph.remap(firing, .2, .8, .22, .31))
    graph.set("Coat Weight", .2)
    graph.set("Coat Roughness", .22)
    pores = graph.noise(point, scale=2100, detail=2).outputs["Fac"]
    graph.set("Normal", graph.bump(pores, strength=.12, distance=.000018))
    return graph.mat


def stewed_strawberry(name: str, seed: float, flesh: bool = False) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.vmath("ADD", graph.coord(), (seed * .017, seed * .043, seed * .031))
    pigment = graph.noise(point, scale=450, detail=3).outputs["Fac"]
    colour = graph.ramp(pigment, [
        (.16, (.085, .006, .004)), (.5, (.19, .017, .009)),
        (.84, (.29, .045, .020)),
    ])
    if flesh:
        x, _, z = graph.separate(graph.coord())
        axis = graph.math("ADD", x, graph.math("MULTIPLY", graph.math("SUBTRACT", pigment, .5), .0012))
        core = graph.remap(graph.math("ABSOLUTE", axis), .0005, .0018, .6, 0)
        core = graph.math("MULTIPLY", core, graph.remap(z, -.012, -.008))
        core = graph.math("MULTIPLY", core, graph.remap(pigment, .2, .8, .85, .55))
        colour = graph.mix(colour, (.25, .075, .045), core)
    graph.set("Base Color", colour)
    graph.set("Roughness", .29 if flesh else graph.remap(pigment, .2, .8, .26, .37))
    graph.set("Subsurface Weight", .12)
    graph.set("Subsurface Radius", (.0024, .001, .0006))
    graph.set("Subsurface Scale", .05)
    pulp = graph.noise(point, scale=2200, detail=2).outputs["Fac"]
    graph.set("Normal", graph.bump(pulp, strength=.16, distance=.00006))
    return graph.mat


def strawberry_achene(name: str) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    variation = graph.noise(graph.coord(), scale=1700, detail=2).outputs["Fac"]
    graph.set("Base Color", graph.ramp(variation, [
        (.2, (.077, .038, .009)), (.8, (.17, .10, .026)),
    ]))
    graph.set("Roughness", .47)
    return graph.mat


def stewed_root(name: str, seed: float, flesh: bool, turnip: bool = False) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.coord()
    shifted = graph.vmath("ADD", point, (seed * .021, seed * .039, seed * .053))
    tissue = graph.noise(shifted, scale=180, detail=3).outputs["Fac"]
    if turnip:
        colours = [(.18, (.39, .35, .24)), (.5, (.48, .44, .33)),
                   (.82, (.54, .50, .39))]
    elif flesh:
        colours = [(.18, (.24, .18, .086)), (.5, (.32, .25, .13)),
                   (.82, (.39, .32, .19))]
    else:
        colours = [(.18, (.045, .023, .009)), (.5, (.095, .055, .023)),
                   (.82, (.14, .087, .040))]
    graph.set("Base Color", graph.ramp(tissue, colours, "EASE"))
    fibres = graph.noise(graph.scale(shifted, (1, .10, 1)),
                        scale=1250, detail=3).outputs["Fac"]
    graph.set("Roughness", graph.remap(tissue, .2, .8, .48, .32))
    graph.set("Subsurface Weight", .065 if flesh or turnip else .02)
    graph.set("Subsurface Radius", (.0014, .0010, .0005))
    graph.set("Subsurface Scale", .05)
    graph.set("Normal", graph.bump(fibres, strength=.23, distance=.000065))
    return graph.mat


def raw_mackerel_flesh(name: str, seed: int) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.coord()
    x, y, _ = graph.separate(point)
    shifted = graph.vmath("ADD", point, (seed * .021, seed * .039, seed * .053))
    tissue = graph.noise(shifted, scale=220, detail=3).outputs["Fac"]
    colour = graph.ramp(tissue, [
        (.18, (.30, .235, .20)), (.5, (.36, .29, .25)), (.82, (.405, .335, .29)),
    ], "EASE")
    phase = graph.math("ADD", y, graph.math("MULTIPLY", graph.math("ABSOLUTE", x), RAW_MYOMERE_SLOPE))
    phase = graph.math("ADD", phase, graph.math("MULTIPLY",
                        graph.math("SINE", graph.math("MULTIPLY", x, 260)), RAW_MYOMERE_CURVE_M))
    phase = graph.math("ADD", phase, .001 * (seed % 5))
    phase = graph.math("DIVIDE", phase, RAW_MYOMERE_SPACING_M)
    fraction = graph.math("PINGPONG", phase, .5)
    fascia = graph.remap(fraction, 0, RAW_FASCIA_WIDTH_M / RAW_MYOMERE_SPACING_M, .30, 0)
    colour = graph.mix(colour, (.53, .46, .39), fascia)
    blood_axis = graph.math("ADD", x, graph.math("ADD", .005, graph.math("MULTIPLY",
                           graph.math("SINE", graph.math("ADD", graph.math("MULTIPLY", y, 740), seed)), .0007)))
    bloodline = graph.remap(graph.math("ABSOLUTE", blood_axis), .0004, .0017, .65, 0)
    colour = graph.mix(colour, (.085, .023, .016), bloodline)
    fibres = graph.noise(graph.scale(shifted, (1, .12, 1)),
                        scale=2200, detail=3).outputs["Fac"]
    graph.set("Base Color", colour)
    graph.set("Roughness", graph.remap(tissue, .2, .8, .34, .23))
    graph.set("Subsurface Weight", .16)
    graph.set("Subsurface Radius", (.002, .0013, .0009))
    graph.set("Subsurface Scale", .05)
    graph.set("Normal", graph.bump(fibres, strength=.18, distance=.000035))
    return graph.mat


def prepared_mackerel_skin(name: str) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.coord()
    x, y, _ = graph.separate(point)
    variation = graph.noise(point, scale=190, detail=3).outputs["Fac"]
    stripe = graph.math("SINE", graph.math("ADD",
                        graph.math("ADD", graph.math("MULTIPLY", y, 620), graph.math("MULTIPLY", x, 180)),
                        graph.math("MULTIPLY", variation, 2)))
    silver = graph.mix((.25, .30, .28), (.045, .070, .070),
                       graph.remap(stripe, .55, .96, 0, .72))
    fines = graph.noise(point, scale=3100, detail=2).outputs["Fac"]
    graph.set("Base Color", silver)
    graph.set("Metallic", .13)
    graph.set("Roughness", .31)
    graph.set("Coat Weight", .20)
    graph.set("Coat Roughness", .18)
    graph.set("Normal", graph.bump(fines, strength=.10, distance=.000018))
    return graph.mat


def raw_fish_plate(name: str) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.coord()
    firing = graph.noise(point, scale=60, detail=2).outputs["Fac"]
    graph.set("Base Color", graph.ramp(firing, [
        (.2, (.11, .14, .15)), (.8, (.145, .18, .188)),
    ], "EASE"))
    grit = graph.noise(point, scale=1400, detail=2).outputs["Fac"]
    graph.set("Roughness", .35)
    graph.set("Normal", graph.bump(grit, strength=.12, distance=.000035))
    return graph.mat


def grilled_trout_flesh(name: str, seed: int) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.vmath("ADD", graph.coord(), (seed * .017, seed * .037, seed * .051))
    tissue = graph.noise(point, scale=160, detail=3).outputs["Fac"]
    colour = graph.ramp(tissue, [
        (.18, (.39, .295, .21)), (.50, (.48, .385, .29)),
        (.82, (.55, .46, .355)),
    ], "EASE")
    heat = graph.noise(point, scale=45, detail=3).outputs["Fac"]
    browning = graph.remap(heat, .25, .62, 0, .72)
    colour = graph.mix(colour, (.14, .048, .012), browning)
    fines = graph.noise(graph.scale(point, (1, .13, 1)),
                        scale=2100, detail=3).outputs["Fac"]
    graph.set("Base Color", colour)
    graph.set("Roughness", graph.remap(tissue, .2, .8, .54, .38))
    graph.set("Subsurface Weight", .07)
    graph.set("Subsurface Radius", (.0015, .0010, .0006))
    graph.set("Subsurface Scale", .05)
    graph.set("Normal", graph.bump(fines, strength=.27, distance=.000065))
    return graph.mat


def grilled_trout_skin(name: str, seed: int) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.vmath("ADD", graph.coord(), (seed * .031, seed * .047, seed * .019))
    heat = graph.noise(point, scale=75, detail=3).outputs["Fac"]
    colour = graph.ramp(heat, [
        (.18, (.039, .027, .012)), (.48, (.10, .070, .025)),
        (.82, (.19, .132, .052)),
    ], "EASE")
    spots = graph.noise(point, scale=350, detail=2).outputs["Fac"]
    colour = graph.mix(colour, (.009, .007, .004), graph.remap(spots, .64, .79, 0, .68))
    scales = graph.noise(graph.scale(point, (1, .38, 1)),
                         scale=1300, detail=2).outputs["Fac"]
    graph.set("Base Color", colour)
    graph.set("Roughness", graph.remap(heat, .2, .8, .60, .39))
    graph.set("Normal", graph.bump(scales, strength=.22, distance=.000055))
    return graph.mat


def grilled_trout_platter(name: str) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    firing = graph.noise(graph.coord(), scale=55, detail=2).outputs["Fac"]
    graph.set("Base Color", graph.ramp(firing, [
        (.2, (.15, .092, .047)), (.8, (.185, .121, .069)),
    ], "EASE"))
    grit = graph.noise(graph.coord(), scale=1600, detail=2).outputs["Fac"]
    graph.set("Roughness", .43)
    graph.set("Normal", graph.bump(grit, strength=.12, distance=.000035))
    return graph.mat


def grilled_white_fish_flesh(name: str, seed: int) -> bpy.types.Material:
    material = grilled_trout_flesh(name, seed)
    ramps = [node for node in material.node_tree.nodes if node.type == "VALTORGB"]
    if len(ramps) != 1 or len(ramps[0].color_ramp.elements) != 3:
        raise ValueError("Cooked white-fish variant requires the authored three-stop tissue palette")
    for element, colour in zip(ramps[0].color_ramp.elements,
                               ((.41, .385, .31), (.51, .49, .415), (.57, .55, .48))):
        element.color = (*colour, 1)
    return material


def grilled_perch_skin(name: str, seed: int) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.vmath("ADD", graph.coord(), (seed * .037, seed * .029, seed * .053))
    _, y, _ = graph.separate(graph.coord())
    heat = graph.noise(point, scale=85, detail=3).outputs["Fac"]
    bands = graph.math("SINE", graph.math("ADD", graph.math("MULTIPLY", y, 235),
                                         graph.math("MULTIPLY", heat, 1.5)))
    colour = graph.ramp(heat, [
        (.2, (.049, .033, .014)), (.5, (.125, .085, .031)),
        (.8, (.18, .132, .056)),
    ], "EASE")
    colour = graph.mix(colour, (.012, .009, .005), graph.remap(bands, .10, .85, 0, .80))
    blisters = graph.noise(point, scale=410, detail=2).outputs["Fac"]
    fines = graph.noise(graph.scale(point, (1, .42, 1)),
                        scale=1700, detail=2).outputs["Fac"]
    graph.set("Base Color", colour)
    graph.set("Roughness", graph.remap(heat, .2, .8, .78, .62))
    relief = graph.math("ADD", fines, graph.math("MULTIPLY", blisters, .4))
    graph.set("Normal", graph.bump(relief, strength=.25, distance=.00007))
    return graph.mat


def grilled_perch_platter(name: str) -> bpy.types.Material:
    material = grilled_trout_platter(name)
    ramps = [node for node in material.node_tree.nodes if node.type == "VALTORGB"]
    if len(ramps) != 1 or len(ramps[0].color_ramp.elements) != 2:
        raise ValueError("Perch platter variant requires the authored two-stop ceramic palette")
    for element, colour in zip(ramps[0].color_ramp.elements,
                               ((.115, .126, .115), (.15, .16, .144))):
        element.color = (*colour, 1)
    return material


def grilled_mackerel_flesh(name: str, seed: int) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.coord()
    x, y, _ = graph.separate(point)
    shifted = graph.vmath("ADD", point, (seed * .017, seed * .031, seed * .043))
    muscle = graph.noise(shifted, scale=190, detail=3).outputs["Fac"]
    colour = graph.ramp(muscle, [
        (.2, (.37, .315, .24)), (.5, (.47, .415, .335)),
        (.8, (.54, .49, .40)),
    ], "EASE")
    axis = graph.math("ADD", x, graph.math("ADD", .004, graph.math("MULTIPLY",
                      graph.math("SINE", graph.math("MULTIPLY", y, 470)), .0006)))
    dark_muscle = graph.remap(graph.math("ABSOLUTE", axis), .0002, .0013, .60, 0)
    colour = graph.mix(colour, (.085, .038, .019), dark_muscle)
    heat = graph.noise(shifted, scale=50, detail=3).outputs["Fac"]
    colour = graph.mix(colour, (.17, .068, .022), graph.remap(heat, .38, .72, 0, .42))
    fibres = graph.noise(graph.scale(shifted, (1, .14, 1)),
                         scale=2200, detail=3).outputs["Fac"]
    graph.set("Base Color", colour)
    graph.set("Roughness", graph.remap(muscle, .2, .8, .53, .37))
    graph.set("Subsurface Weight", .065)
    graph.set("Subsurface Radius", (.0015, .0011, .0007))
    graph.set("Subsurface Scale", .05)
    graph.set("Normal", graph.bump(fibres, strength=.27, distance=.000065))
    return graph.mat


def grilled_mackerel_skin(name: str, seed: int) -> bpy.types.Material:
    graph = Graph(name)
    graph.mat["food_shader_source_sha256"] = SOURCE_SHA256
    point = graph.coord()
    x, y, _ = graph.separate(point)
    heat = graph.noise(point, scale=110, detail=3).outputs["Fac"]
    wave = graph.math("SINE", graph.math("ADD",
                       graph.math("ADD", graph.math("MULTIPLY", y, 640),
                                  graph.math("MULTIPLY", x, 180)),
                       graph.math("MULTIPLY", heat, 2.3)))
    colour = graph.mix((.16, .148, .118), (.040, .031, .019),
                       graph.remap(wave, .64, .96, 0, .68))
    scorch = graph.noise(graph.vmath("ADD", point, (seed * .037, seed * .019, seed * .041)),
                         scale=60, detail=2).outputs["Fac"]
    colour = graph.mix(colour, (.055, .029, .009), graph.remap(scorch, .28, .65, 0, .72))
    colour = graph.mix(colour, (.006, .004, .002), graph.remap(scorch, .62, .78, 0, .80))
    fines = graph.noise(point, scale=1800, detail=3).outputs["Fac"]
    graph.set("Base Color", colour)
    graph.set("Metallic", 0)
    graph.set("Roughness", graph.remap(heat, .2, .8, .70, .49))
    graph.set("Normal", graph.bump(fines, strength=.25, distance=.000065))
    return graph.mat


def grilled_mackerel_platter(name: str) -> bpy.types.Material:
    material = grilled_trout_platter(name)
    ramps = [node for node in material.node_tree.nodes if node.type == "VALTORGB"]
    if len(ramps) != 1 or len(ramps[0].color_ramp.elements) != 2:
        raise ValueError("Mackerel platter requires the authored two-stop ceramic palette")
    for element, colour in zip(ramps[0].color_ramp.elements,
                               ((.075, .11, .105), (.095, .14, .13))):
        element.color = (*colour, 1)
    return material
