# S16 - Epic Virtual Shadow Maps

URL: https://dev.epicgames.com/documentation/en-us/unreal-engine/virtual-shadow-maps-in-unreal-engine
Accessed: 2026-09-20 by the counterevidence worker.
Type: primary engine reference.

## Short verbatim evidence

- "Any light movement or rotation will invalidate all cached pages for that light"
- "Geometry that can be deformed using Skeletal animation, or materials using World Position Offset or Pixel Depth Offset always invalidates cached pages every frame."
- "It is important that non-Nanite meshes have LODs setup or else they become extremely expensive to render into small pages."
- "corruption will occur"

## Interpretation and limits

The last quote concerns physical page-pool overflow. Moving sunlight, wind
deformation and distant shadow detail must be measured together. A frozen-sun
comparison can conceal real gameplay cost. Distance-disable deformation where
visually acceptable; do not suppress cache invalidation with Rigid/Static flags
while continuing to animate the mesh and call the result correct.

The source also says Nanite geometry renders more efficiently into VSMs:
this is counterevidence to automatically preferring conventional meshes.
The shadow texture pool consumes memory separately from Nanite geometry.

## Assessment

Credibility: high for mechanisms and debugging guidance. Recency: rolling
reference, no Homestead measurement. Bias: vendor optimization guidance.
