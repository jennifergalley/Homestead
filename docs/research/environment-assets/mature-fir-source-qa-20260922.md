# Mature Fir 01 c source-quality check

Date: 2026-09-22. Read-only QA supporting MAIN's current integration.
This is not an in-game visual or performance acceptance.

## Inputs and identity

Read only from MAIN's exact current files:

- Prepared provenance:
  `Assets\Environment\MatureFir20260921\candidate01\Prepared\provenance.json`,
  SHA-256 `711A850106941676F6604044FFCD972BE1DCD232B7BD35503AF6791E8AC11650`.
- Source inventory SHA-256
  `D98D37854FD6C90F82F0D345D54628912A7C4CA9618483D8ED3EEF5F9D6F8F0C`.
- Source receipt SHA-256
  `93EFA5244A355EC0E395562F18E505A5CE44D23C8078359DF7EA8772218D0655`.
- Native package receipt SHA-256
  `2F5C3EA156B3B9DB3A5FA355579990AED75CA90ACCA5454135F8FEE7153B5A9B`.

Selected source identity is model ID `357290555`,
`fir_tree_01_c_LOD0`, geometry ID `816415551`.
Its four source slots are:
`fir_tree_01_bark`, `fir_tree_01_twig`,
`fir_tree_01_dead_branches`, `fir_tree_01_trunk_c`.
The source display-layout translation `(1200,0,0)` in centimetre-unit FBX data
was removed once; the imported `-90 degree X` axis conversion and scale 100
were baked once. Prepared root/origin remains `(0,0,0)`.

Prepared bounds are `[-2.747031,-2.969963,-0.093823]` to
`[3.558908,2.996274,14.426759]` metres:
6.306 x 5.966 x 14.521 m. Native package bounds agree after x100 conversion.
About 9.38 cm of root geometry sits below the anchor, which is appropriate
when the instance origin is placed at the ground height.

## Blocking source/material issue: no UV layer

The exact selected source geometry's declared layer types are
`LayerElementNormal` and `LayerElementMaterial`; **there is no
`LayerElementUV`**. Independent parsing of all three prepared FBXs likewise
finds zero UV layers.

The current native inventory verifies materials, texture objects, slot names,
triangles, screen sizes, bounds and collision, but it does not verify
MeshDescription UV-channel count, UV determinants, normals or tangents.
Successful package references therefore do not prove mapped PBR correctness.

The Unreal import log independently reports degenerate tangent bases and
nearly-zero binormals for LOD0, both imported lower LODs and the final mesh.
Read-only prepared-array attribution finds:

| LOD | Near-zero normal corners | Roles |
| --- | ---: | --- |
| 0 | 0 | none |
| 1 | 456 | 453 twig, 3 dead-branch |
| 2 | 630 | 621 twig, 9 dead-branch |

MAIN subsequently enhanced the native verifier and definitively measured
**505,494 / 505,494 LOD0 triangles with degenerate texture UVs**. The commandlet
rejected the persisted asset with exit 7 and shut down normally. The current
FBX-derived candidate is therefore blocked from runtime use, not merely awaiting
evidence. Do not hide the failure with tint, exposure, generated UVs or a tangent
setting.

The narrow provider-native repair source is:

- https://dl.polyhaven.org/file/ph-assets/Models/blend/1k/fir_tree_01/fir_tree_01_1k.blend
- 218,986,261 bytes; publisher MD5 `a08031ea8ffb49711b294e1c8213a909`.

MAIN approved acquisition of only this Blend file under a specific bounded
allowance, with no texture reacquisition or global file-cap relaxation. The
provider glTF descriptor contains `TEXCOORD_0` on all selected c primitives,
but its geometry dependency `fir_tree_01.bin` is 478,462,204 bytes. USD is
443,961,681 bytes. Neither is a smaller practical repair path.

The Blend path must still prove selected `fir_tree_01_c` model identity,
authored UV0 per material role, valid normals/tangents, source transforms and
material order before a new export. Acquisition alone is not repair acceptance.

## LOD and canopy-retention risks

| LOD / screen size | Total triangles | Green twig triangles | Twig retention | Geometry-centroid occupancy retention XZ / YZ |
| --- | ---: | ---: | ---: | --- |
| 0 / 1.0 | 505,494 | 446,074 | 100% | 100% / 100% |
| 1 / 0.30 | 128,247 | 111,518 | 25% | 84.96% / 84.23% |
| 2 / 0.08 | 34,015 | 26,764 | 6% | 47.92% / 48.12% |

The occupancy proxy bins projected triangle centroids into 256x256 XZ/YZ grids.
It ignores the alpha mask and is not a rendered silhouette, but it is useful
for relative source-geometry loss.

The first green twig geometry is elevated:

- LOD0 minimum Z 6.9085 m; 1st/5th centroid percentiles 7.166/8.322 m.
- LOD1 minimum Z 6.9091 m; 1st/5th percentiles 7.226/8.349 m.
- LOD2 minimum Z 6.9254 m; 1st/5th percentiles 7.942/8.383 m.
- No twig triangle in any LOD has any vertex below 6 m.

Twig triangles touching the lower crown below 8 m fall from 14,469 to 3,038
to 292: 21.0% and 2.02% retention. LOD2 therefore has a concrete risk of
thinning the lower edge and reopening distant horizon gaps at screen size 0.08.
LOD1 also needs an ordinary-camera transition comparison at 0.30.

This tree is appropriate as a tall mature-conifer silhouette, not as low
enclosure. It must be layered with poles, saplings, shrubs and/or the background
forest belt. Increasing its count cannot fill its 6.9 m foliage-free base.

## Scale, anchor and collision

Recommended runtime uniform scale remains **0.92-1.05**:

- Resulting height: approximately 13.36-15.25 m.
- Approximate full canopy envelope: 5.80-6.62 m by 5.49-6.26 m.
- Green foliage floor: approximately 6.36-7.27 m.

Do not scale this tree down into a pole or up into a landmark; the palette has
separate age strata. Preserve the root origin at analytic ground height.

The prepared source-derived capsule is centered at
`(-0.022,0.033,2.406)` m with radius 0.692 m and cylinder length 3.615 m.
Independent lower-trunk vertices through Z=5 m have radial percentiles from
that center of 0.358 m median, 0.610 m p90, 0.641 m p95 and 0.702 m max.

The native 0.50 m radius / 4.00 m cylinder centered at
`(-0.022,0.033,2.50)` m correctly spans ground-to-5 m and is deliberately
gameplay-conservative, but it under-covers more than the outer 10% of measured
lower-trunk radial samples. Accept it only if ordinary walking does not visibly
clip through trunk flare/low limbs. A measured 0.60 m radius with approximately
3.82 m cylinder length and center Z about 2.42 m is a better source-fit
compromise if clipping is seen; the full canopy radius must never be collision.
Uniform component scale must affect visual and collision together.

## Palette role comparison

- Tree Small 02: 4.56 m airy broadleaf pioneer.
- Fir Sapling: 0.74/1.30 m juvenile conifers.
- Fir Sapling Medium pole: planned 5.91 m intermediate conifer.
- Jacaranda: planned 19.47 m broad spreading mature broadleaf.
- Mature Fir Tree 01 c: 14.52 m narrow/tall mature conifer.

The mature fir fills a genuine missing vertical stratum and is not a duplicate
of the ready trees. Its elevated foliage makes the pole/sapling and midstory
layers essential rather than optional.

## Evidence limits and disposition

Source/prepared identity, geometry counts, slots, bounds and texture hashes are
consistent. Native import and fresh inventory passed, but both supervised
processes later hung during shutdown and were terminated by owned PID; that is
recorded in MAIN's receipt and is not erased here.

Status: **current FBX path rejected; Blend-source replacement in progress**.
If the new source/export passes UV/basis proof, LOD2 lower-crown retention and
the 50 cm collision
radius remain visual/gameplay review items. Runtime package use, performance
and Jenny's aesthetic acceptance are not established by this check.

Full machine-readable read-only measurements are in this session's
`mature-fir-qa.json`; the script and logs remain session artifacts, not repo
pipeline code. No source/prepared/native file was changed.
