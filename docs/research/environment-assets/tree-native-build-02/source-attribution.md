# Frozen LOD2 normal and UV attribution

Read-only measurement on 2026-09-21. Full results and exact definitions:
`tree-lod2-attribution.json`; measurement script: `tree-lod2-attribution.py`.

Source: `TreeSmall02_LOD2.fbx`, SHA-256
`03365d16ad77535e28c8f3b7243a8183d30768af3c719589cef58a44ed12e283`.
Hash checked before and after. CPython 3.13/NumPy reused the installed official
standalone FBX binary reader. No bpy, Blender process, Unreal, export or repair.
Only these session-file analysis artifacts were written.

## Referenced normal attribution

347,049 direct normal vectors exist and all are referenced. Direct index 0 is
exactly `(0,0,0)`; it is the only vector with Euclidean length <= 0.0001
(also the only vector <= 0, 1e-12, 1e-8 and 1e-6).
FBX normal mapping is ByPolygonVertex / IndexToDirect.

| Material slot / role | Triangles | Corners using zero normal | Distinct triangles using zero normal | Zero geometric-area triangles | Exact-zero UV0 determinants |
| --- | ---: | ---: | ---: | ---: | ---: |
| 0 branches | 23,702 | 338 | 262 | 0 | 23,702 |
| 1 leaves | 193,938 | 1,041 | 1,041 | 0 | 0 |
| 2 trunk | 14,145 | 0 | 0 | 0 | 0 |
| Total | 231,785 | 1,379 | 1,303 | 0 | 23,702 |

The zero normal is **not unused data**. There are 695,355 referenced corners.
Control points are a separate table: 424,817 total, 384,193 referenced, 40,624
unused. No bounds conclusions are drawn here; main owns that investigation.

Geometric area is half the length of the edge cross product in raw local metre
coordinates, evaluated in float64. Both exact-zero area and area <= 1e-12 square
metres produce zero cases for every role. This does not reproduce UE's welding,
degeneracy threshold, normal conversion or tangent-generation algorithm.

## UV material-channel finding and erratum

Every branch corner in UV0 (`UVMap`) is exactly `(0,0)`. All 23,702 branch
triangles therefore have zero UV0 determinant. Branch UV1 (`UV_map_01`) contains
nondegenerate coordinates instead. Conversely, all leaf/trunk corners in UV1
are `(0,0)` and their nondegenerate data is in UV0.

| Layer | Branch degenerate determinants | Leaf degenerate determinants | Trunk degenerate determinants |
| --- | ---: | ---: | ---: |
| UV0 / UVMap | 23,702 | 0 | 0 |
| UV1 / UV_map_01 | 0 | 193,938 | 14,145 |

For this table, exact zero and absolute determinant <= 1e-12 give identical
counts. The determinant is the absolute 2D edge determinant, twice UV triangle
area, dimensionless. No normalization/welding/clamping was performed.
Branch UV1 extends from `(-0.0006629583076573908,-0.6426932215690613)` to
`(1.0000296831130981,1.211830735206604)`; do not infer that coordinates outside
0..1 are a lightmap or should be clamped.

**Erratum to the frozen README:** its blanket "Material maps use channel 0"
statement is incorrect for branches. The measured source mapping is branch
slot 0 -> UV1, leaf slot 1 -> UV0, trunk slot 2 -> UV0. Main must verify the
native import's retained channel order. No frozen file was edited to make this
correction; this report supplies the corrected source facts for integration.
Earlier unlit source comparisons used default UV0 and therefore did not validate
branch texture mapping or tangent-space shading.

## Limits

The measurements attribute malformed normal references and degenerate UV0
coordinates to roles in this frozen derived LOD2. They do not establish whether
those conditions originated upstream or during reduction, nor which individual
UE warnings they explain. No tangent vectors, repair strategy, UE bounds/frame
conversion or new source version were investigated or implemented.
