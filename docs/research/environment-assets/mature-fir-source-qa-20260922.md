# Mature Fir 01 c source-quality check

Date: 2026-09-22. Read-only QA supporting MAIN's current integration.
This is not an in-game visual or performance acceptance.

## Inputs and identity

The first section records the rejected FBX route. The accepted replacement
contract uses MAIN's exact `candidate02` files:

- Blend-source manifest SHA-256
  `ED00C13141D954731A8C010686C785BE585E94FFDECF1779E8BB9FBC8891888E`.
- Blend-source receipt SHA-256
  `38DF4AFD4E4C2EF4E03975F733DD7F1A5ADB01BF3ED119CFB6338006C227BA89`.
- Provider Blend original SHA-256
  `153D20AAE6D7C63A4C52689D91E8BEAD78F6D17A70E11EF4D03D19953383DE00`.
- Accepted prepared provenance SHA-256
  `0327C21332E75B0C549A44F6D034125702FE8D06A57BB56C162B77C1DE9EFEDB`.

Rejected route evidence was read from:

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

The first generated-reduction repair route was abandoned: its fresh-import LOD1
had 498 zero-normal corners despite valid UVs and geometric area. Repeating
smoothing/export switches did not repair it.

The Blend file also contains provider-authored objects
`fir_tree_01_c_LOD1` and `fir_tree_01_c_LOD2`. MAIN selected those instead and
completed a fresh `Prepared-UV07` export. Its exact provenance is:

`Assets\Environment\MatureFir20260921\candidate02\Prepared-UV07\provenance.json`

- SHA-256 `0327C21332E75B0C549A44F6D034125702FE8D06A57BB56C162B77C1DE9EFEDB`.
- Current production tool SHA-256
  `BFA4CBDD4CC3C3DB2E8A3334306226E3EB0E85B4319677529171959433A2A373`.

The source repair contract passes:

- All three exact c objects have location `(12,0,0)` m, identity rotation/scale;
  display layout is removed once.
- Each has a provider-native `CORNER` / `FLOAT_VECTOR` `UVMap` attribute whose
  Z value is zero. The attribute length equals mesh loops.
- On a copy, the source attribute is renamed, exact XY values are copied into
  active/render UV layer 0, and every-corner source/recovered SHA-256 matches:
  LOD0 `9B47C514...B1F779`, LOD1 `7F26F33B...661DF1`,
  LOD2 `A9814DEF...CE7AA`.
- Positions, loop vertex indices, polygon starts/totals, source/recovered material
  indices and corner normals are hash-bound. They remain exactly equal except
  LOD0's documented empty leading material slot removal.
- Each exported FBX contains exactly one `LayerElementUV`; fresh reimport reports
  zero per-role degenerate UV triangles, zero near-zero tangent/normal corners at
  `1e-4`, zero invalid binormal signs and zero tiny geometry.

This is preservation/conversion of publisher UV data, not a generated unwrap.

## LOD and canopy-retention risks

The rejected generated levels above are no longer the source contract. The
accepted provider-authored levels are:

| LOD / proposed screen size | Source model | Total triangles | Green twig triangles | Geometry-centroid occupancy retention XZ / YZ |
| --- | ---: | ---: | ---: | --- |
| 0 / 1.0 | `fir_tree_01_c_LOD0` | 505,494 | 446,074 | 100% / 100% |
| 1 / 0.30 | `fir_tree_01_c_LOD1` | 68,879 | 9,459 | 54.20% / 51.02% |
| 2 / 0.08 | `fir_tree_01_c_LOD2` | 31,512 | 5,016 | 35.57% / 33.19% |

Accepted FBX SHA-256 values are LOD0
`FF9A5E8BF8673370F8225D6879008D7060ED0C340994C47B4A36AE28F4A24F9A`,
LOD1 `032622AEE17276978FD7B27B92C881942E7AC9F66BF650051B96948540650E9B`,
and LOD2 `496CC17FE04AD16CFB52E29EC2A331376D74DB439B367830B5341E996F7D05BA`.

The occupancy proxy bins projected triangle centroids into 256x256 XZ/YZ grids.
It ignores the alpha mask and is not a rendered silhouette, but it is useful
for relative source-geometry loss.

The first green twig geometry remains elevated and stable:

- LOD0 minimum Z 6.9085 m; 1st/5th centroid percentiles 7.166/8.322 m.
- Authored LOD1 minimum Z about 6.90 m; 1st/5th percentiles 7.18/8.31 m.
- Authored LOD2 minimum Z about 6.90 m; 1st/5th percentiles 7.17/8.31 m.
- No twig triangle in any LOD has any vertex below 6 m.

Twig triangles touching the lower crown below 8 m retain only 2.28% / 1.27%
of the LOD0 triangle count in authored LOD1/2. Because these are provider-authored
larger-card levels, triangle/centroid loss alone does not establish visible
silhouette loss; alpha-masked card area matters. It does establish a focused
review requirement: compare ordinary-camera lower-crown/horizon transitions at
0.30 and especially 0.08. Do not assume authored automatically means acceptable.

Bounds/root remain coherent. LOD1/2 retain minimum Z `-0.093823` m; maximum Z
changes by at most 3.18 cm and horizontal extrema by roughly 3 cm from LOD0.
There is no meaningful ground-anchor or pivot jump between levels.

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

Status: **Blend-source provider-authored LOD source contract passed**.
The previous FBX/generated-level candidate remains rejected and must not be
silently reused. The new source is ready for fresh native import with the
enhanced UV/basis verifier. Authored LOD lower-crown retention and the 50 cm
collision radius remain visual/gameplay review items. Runtime package use,
performance and Jenny's aesthetic acceptance are not established by this check.

Full machine-readable read-only measurements are in this session's
`mature-fir-qa.json`; the script and logs remain session artifacts, not repo
pipeline code. No source/prepared/native file was changed.
