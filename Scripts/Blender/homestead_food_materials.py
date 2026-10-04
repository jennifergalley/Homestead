"""Original prepared-food source shaders, isolated from held fish materials.

Usage: import homestead_food_materials as food; food.potato_skin(name, seed)
No meal baking or Unreal material admission is enabled here.
"""
import bpy

from homestead_materials import Graph


def potato_skin(name: str, seed: float) -> bpy.types.Material:
    graph = Graph(name)
    point = graph.vmath("ADD", graph.coord(), (seed * .037, seed * .019, seed * .071))
    roasting = graph.noise(point, scale=85, detail=2, roughness=.62).outputs["Fac"]
    colour = graph.ramp(roasting, [
        (.31, (.009, .006, .003)), (.40, (.027, .015, .006)),
        (.49, (.095, .047, .015)), (.59, (.18, .093, .028)),
        (.71, (.24, .15, .060)), (.83, (.15, .074, .024)),
    ], "EASE")
    pores = graph.noise(point, scale=1100, detail=2).outputs["Fac"]
    graph.set("Base Color", graph.mix(colour, (.11, .060, .019),
                                     graph.math("MULTIPLY", pores, .16)))
    graph.set("Roughness", graph.remap(roasting, .25, .75, .83, .64))
    graph.set("Normal", graph.bump(pores, strength=.18, distance=.000065))
    return graph.mat


def potato_flesh(name: str, seed: float) -> bpy.types.Material:
    graph = Graph(name)
    point = graph.vmath("ADD", graph.coord(), (seed * .023, seed * .041, seed * .017))
    crumb = graph.noise(point, scale=410, detail=2).outputs["Fac"]
    graph.set("Base Color", graph.ramp(crumb, [
        (.15, (.34, .28, .16)), (.43, (.53, .45, .29)),
        (.70, (.66, .59, .42)), (.87, (.55, .47, .30)),
    ], "EASE"))
    graph.set("Roughness", .81)
    graph.set("Subsurface Weight", .035)
    graph.set("Subsurface Radius", (.003, .0018, .0008))
    graph.set("Subsurface Scale", .05)
    graph.set("Normal", graph.bump(crumb, strength=.12, distance=.00007))
    return graph.mat
