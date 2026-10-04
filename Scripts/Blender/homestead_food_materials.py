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
