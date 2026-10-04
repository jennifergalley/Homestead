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
