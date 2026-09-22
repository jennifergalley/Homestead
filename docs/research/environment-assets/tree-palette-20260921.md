# Bounded woodland tree palette

Date: 2026-09-21. Source-only decision; no acquisition or in-game acceptance.
Current local inventory supports **two source families / three forms**, not the
requested four-to-six genuinely distinct trees:

1. Tree Small 02 (one 4.56 m airy broadleaf form).
2. Fir Sapling `a` (1.30 m) and `c` (0.74 m) young-conifer forms.

Those paths and hashes exist locally or in MAIN's prepared workspace. No other
tree FBX/glTF/GLB/Blend exists under the known project asset roots. Shrubs,
flowers and logs are not relabeled as trees. LODs of one tree are not species.

The recommended complete palette therefore has **three ready forms and four
acquisition-required additions**. Until acquisition and inspection, generator
roles remain stable silhouette/age strata and MAIN uses an explicit fallback;
missing assets must never renumber deterministic metadata.

## Deterministic role map

Role weights apply to accepted tree sites, not to gameplay yield or asset load
order: BroadleafMature 24%, BroadleafYoung 24%, ConiferMature 18%,
ConiferYoung 26%, WoodlandAccent 8%. Uniform scale is modest around each
variant's native size. Yaw/scale/variant use key-specific stable streams.

| Stable key | Generator role | Source/variant | Availability | Native form and allowed uniform scale | Canopy / collision / future yield |
| --- | --- | --- | --- | --- | --- |
| `broadleaf.pioneer.tree-small-02` | BroadleafYoung | Tree Small 02 | Ready: `Assets\Environment\TreeSmall02Prepared\v3\TreeSmall02_LOD2.fbx`, SHA-256 `03365d16...12e283`; MAIN also has native `SM_TreeSmall02_Woodland` | 4.564 m high; 2.921 x 4.296 m source-world footprint; scale 0.90-1.08 | Airy young/pioneer canopy. Use measured native lower-trunk capsule, never full canopy radius; placeholder `wood-medium-broadleaf`. |
| `conifer.young.fir-sapling-a` | ConiferYoung variant 0 | Fir Sapling `a` | Ready in MAIN's prepared `fir_sapling_selected.fbx`, SHA-256 `4CA95C6E...B24D390E` | Referenced raw 1.301 m high, about 0.861 x 0.822 m local footprint; scale 0.95-1.05 | Dense juvenile conifer. Measure lower trunk from branch-role vertices; camera-only crown if needed; placeholder `softwood-small`. |
| `conifer.young.fir-sapling-c` | ConiferYoung variant 1 | Fir Sapling `c` | Same ready prepared file | Referenced raw 0.736 m high, about 0.622 x 0.634 m local footprint; scale 0.95-1.05 | Smallest juvenile. Do not enlarge into a mature tree; placeholder `softwood-sapling`. |
| `broadleaf.mature.spreading` | BroadleafMature | Jacaranda Tree LOD0 source | **Not local; acquisition required** | Descriptor: one 19.47 m high spreading multi-trunk form, roughly 24.42 x 19.15 m union of its three role accessors; scale 0.88-1.02 | Dominant broadleaf roof and major silhouette break. Collision must be measured from trunk vertices below branch rise; no canopy collision; placeholder `wood-large-broadleaf`. |
| `broadleaf.accent.gnarled` | WoodlandAccent | Island Tree 02 LOD0 source | **Not local; acquisition required** | Descriptor: gnarled approximately 3.41 m high, roughly 4.21 x 4.07 m leaf/branch span; scale 0.90-1.06 | Rare low spreading accent/edge tree, not dominant species. Gnarled base may need two measured capsules or a simple convex lower-trunk proxy; placeholder `wood-medium-broadleaf`. |
| `conifer.mature.fir-c` | ConiferMature | Fir Tree 01 `c` LOD0 source | **Not local; acquisition required** | Descriptor: 14.52 m high, about 6.31 x 5.97 m twig span; 505,494 indexed triangles; scale 0.92-1.05 | Tall sparse mature conifer for vertical rhythm. One lower-trunk capsule derived from `trunk_c`, crown camera-only; placeholder `softwood-large`. |
| `conifer.pole.fir-medium-c` | ConiferYoung variant 2 | Fir Sapling Medium `c` LOD0 source | **Not local; acquisition required** | Descriptor: 5.91 m high, about 4.31 x 4.01 m twig span; 427,645 indexed triangles; scale 0.92-1.06 | Intermediate pole layer between 1.3 m saplings and mature fir; measured trunk capsule; placeholder `softwood-medium`. |

`variantIndex` is role-local. MAIN maps role+variant to an available mesh;
the generator never depends on package availability. Collision radii/heights
come from the imported mesh's measured lower trunk, not this table's canopy
span. Yield tags are placeholders only; ForestTree key/clear/save authority and
current economy remain unchanged.

## Ecological and art-direction truth

This is a **fictional warm mixed woodland silhouette palette**, not a European
forest reconstruction:

- Tree Small 02 is identified as African *Burkea africana* / wild syringa.
- Jacaranda is a subtropical South American genus and supplies a broad,
  compound-leaf spreading form. Do not add purple flowering or call it native
  temperate woodland without a later setting decision.
- Island Tree 02 is a wind-sculpted coastal broadleaf of unspecified species.
- Fir assets supply a coherent conifer age ladder.

The visual palette is coherent in color/scan treatment and strata, but botanical
provenance is mixed. If ecological specificity becomes more important than
immediate variety, replace broadleaf roles as a later licensed acquisition;
do not silently rewrite these identities.

Publisher previews were personally compared in a private contact sheet. Jacaranda
is the only large spreading broadleaf candidate; Island Tree 02 adds a strongly
different gnarled low silhouette; Fir Tree 01 and Fir Sapling Medium provide
distinct mature and pole conifers. They are not near-duplicate LODs.

## Ready-source contracts

### Tree Small 02

Rico Cilliers, CC0-1.0. Exact source/provenance/material/UV qualifications are
in `TreeSmall02Prepared\README.md`. Selected LOD2 has 231,785 triangles and
three roles: branches, leaves, trunk. Branch texture UV is channel 1; leaves/
trunk use channel 0. Existing native canopy work may add runtime LODs, but
source LOD files remain one source family.

### Fir Sapling

Rob Tuytel (photography), Rico Cilliers (modeling), CC0-1.0.
MAIN's prepared file is 12,427,404 bytes, SHA-256
`4CA95C6EB2ECE345B4EC368F1DA9D76F9DBC5F606E86E85A01532EE4B24D390E`.
Variants `a`/`c` have 157,402 / 124,743 source fan triangles; two ordered roles:
opaque branches and masked/two-sided twigs. Both use UV0 and have no flagged
near-zero role diagnostics. No source LOD chain is supplied.

## Acquisition-required exact sources

All four pages currently declare free access and CC0-1.0. Provider policy and
license URLs remain `https://polyhaven.com/about-contact`,
`https://polyhaven.com/license`, and
`https://creativecommons.org/publicdomain/zero/1.0/`.
No binary below exists locally; publisher MD5 is not a local SHA-256.
MAIN decides/acquires and creates actual receipts before import.

Acquisition status after MAIN review:

| Asset | Status |
| --- | --- |
| `jacaranda_tree` | Authorized for MAIN's existing size/hash/license-gated acquisition path |
| `island_tree_02` | Authorized for MAIN's existing size/hash/license-gated acquisition path |
| `fir_sapling_medium` | Authorized for MAIN's existing size/hash/license-gated acquisition path |
| `fir_tree_01` | **Conditional, not approved as the 249 MB FBX.** MAIN must first inspect a bounded Poly Haven archive/delivery alternative or choose a sub-128 MiB mature-conifer source. Raising the cap requires separate exact rationale. |

### `jacaranda_tree`

- Authors: Rob Tuytel (guidance), Rico Cilliers (all).
- Source: https://polyhaven.com/a/jacaranda_tree
- Metadata API: https://api.polyhaven.com/info/jacaranda_tree and
  https://api.polyhaven.com/files/jacaranda_tree; publisher files hash
  `2841c124ea9e28651baa3b47ca8828907e19aee0`.
- 1K FBX: https://dl.polyhaven.org/file/ph-assets/Models/fbx/1k/jacaranda_tree/jacaranda_tree_1k.fbx
  — 132,437,628 bytes, publisher MD5 `98f9827599dd42b18c1e9dfab3062d2f`.
- Public descriptor: https://dl.polyhaven.org/file/ph-assets/Models/gltf/1k/jacaranda_tree/jacaranda_tree_1k.gltf
  — 9,526 bytes, publisher MD5 `8fed93ff7cbdaac0f5c199ef1fd0f694`.
- Descriptor evidence: one `jacaranda_tree_LOD0`, 3,863,832 indexed triangles;
  roles in order branches / trunk / leaves. glTF is metre/Y-up, identity node
  transform. Role accessor union is the approximate native extent above.
- Desired 1K maps: for each `branches`, `trunk`, `leaves`, diffuse, DirectX
  normal, roughness and AO; add `leaves_alpha`. All exact files are under
  `.../Models/png/1k/jacaranda_tree/` using
  `jacaranda_tree_<role>_{diff,nor_dx,rough,ao}_1k.png`.
- Publisher `lods:true` is only a catalog flag. The public descriptor exposes
  LOD0 only; authored FBX LOD count and usable reduction must be inspected.
- Acquisition cost/risk: the FBX alone is about 126.3 MiB and exceeds the old
  palette's per-file scale, though it remains below 128 MiB. Near geometry is
  too expensive for mass placement until native/authored LOD evidence exists.

### `island_tree_02`

- Authors: Rob Tuytel (scanning/processing), Rico Cilliers (cleanup/processing).
- Source: https://polyhaven.com/a/island_tree_02
- Metadata API: https://api.polyhaven.com/info/island_tree_02 and
  https://api.polyhaven.com/files/island_tree_02; publisher files hash
  `4292954a33702fc0809a2b2f17a310bb36fd1aca`.
- 1K FBX: https://dl.polyhaven.org/file/ph-assets/Models/fbx/1k/island_tree_02/island_tree_02_1k.fbx
  — 32,258,924 bytes, publisher MD5 `52f6b7264dc026f9496d5cb7d51f7c04`.
- Public descriptor: https://dl.polyhaven.org/file/ph-assets/Models/gltf/1k/island_tree_02/island_tree_02_1k.gltf
  — 8,545 bytes, publisher MD5 `825057164e987894cbfc6c462977fa71`.
- Descriptor evidence: one `island_tree_02_LOD0`, 1,072,213 indexed triangles;
  roles in order trunk/base / leaves / branches, metre/Y-up, identity transform.
- Desired maps: trunk/base prefix `island_tree_02` plus `leaves` and `branches`;
  each gets diffuse, DX normal, roughness, AO; add `leaves_alpha`.
- Preview includes a sandy base disc. Source inspection/import must determine
  whether that geometry/material can be omitted without damaging the tree.
- `lods:true` remains unverified authored-LOD evidence. This is a rare accent,
  not a density workhorse.

### `fir_tree_01`

- Authors: Rob Tuytel (photography), Rico Cilliers (modeling).
- Source: https://polyhaven.com/a/fir_tree_01
- Metadata API: https://api.polyhaven.com/info/fir_tree_01 and
  https://api.polyhaven.com/files/fir_tree_01; publisher files hash
  `6b79f3da0de1d192fc78ab54efa5e8956af97749`.
- 1K FBX: https://dl.polyhaven.org/file/ph-assets/Models/fbx/1k/fir_tree_01/fir_tree_01_1k.fbx
  — 249,300,492 bytes, publisher MD5 `ab79788fc818ce7eadd40ccbfe987918`.
- Public descriptor: https://dl.polyhaven.org/file/ph-assets/Models/gltf/1k/fir_tree_01/fir_tree_01_1k.gltf
  — 30,279 bytes, publisher MD5 `0a184431efdba9e4bf76900d8d56504d`.
- Select **only** `fir_tree_01_c_LOD0`: 505,494 indexed triangles. Roles are
  bark / twig / dead branches / trunk_c (source descriptor lists trunk_c last).
  The model node has layout translation `(12,0,0)` metres; preparation must
  preserve source data but native placement must remove display-layout offset.
- Desired maps: `bark_{diff,nor_dx,rough,ao}`, `twig_{diff,nor_dx,rough,ao,alpha}`,
  and `trunk_c_{diff,nor_dx,rough,ao}`. Descriptor says trunk_c/dead branches
  reuse bark maps, but source inspection controls the native graph.
- The FBX exceeds the established 128 MiB per-file acquisition cap. Do not
  silently raise it. This exact FBX is **not acquisition-approved**. MAIN first
  checks a bounded publisher archive/delivery alternative or selects a verified
  sub-128 MiB mature conifer. Public glTF includes a large external bin and is
  not an automatic safer substitute or license/size bypass.

### `fir_sapling_medium`

- Authors: Rob Tuytel (photography), Rico Cilliers (modeling).
- Source: https://polyhaven.com/a/fir_sapling_medium
- Metadata API: https://api.polyhaven.com/info/fir_sapling_medium and
  https://api.polyhaven.com/files/fir_sapling_medium; publisher files hash
  `0029476c186325a03c000bb1349ec299e43f9331`.
- 1K FBX: https://dl.polyhaven.org/file/ph-assets/Models/fbx/1k/fir_sapling_medium/fir_sapling_medium_1k.fbx
  — 51,634,508 bytes, publisher MD5 `82d67e5714eab0c493c8c9260bba8d28`.
- Public descriptor: https://dl.polyhaven.org/file/ph-assets/Models/gltf/1k/fir_sapling_medium/fir_sapling_medium_1k.gltf
  — 17,059 bytes, publisher MD5 `19e16469d9f085b6f9a8034ddab1c010`.
- Select `fir_sapling_medium_c_LOD0`: 427,645 indexed triangles, 5.91 m high;
  roles branches / twigs / branches_dead. Model layout translation `(10,0,0)`
  metres must not become a gameplay offset.
- Desired maps: `branches_{diff,nor_dx,rough,ao}` and
  `twigs_{diff,nor_dx,rough,ao,alpha}`. Dead branches reuse branch maps.
- Again, catalog `lods:true` does not prove a supplied runtime LOD chain; the
  public descriptor exposes only LOD0.

## Preview evidence and stop

Reference-only previews, personally viewed:

- https://cdn.polyhaven.com/asset_img/thumbs/jacaranda_tree.png?width=630&quality=95&v=935ab3ba
- https://cdn.polyhaven.com/asset_img/thumbs/island_tree_02.png?width=630&quality=95&v=854bdc1f
- https://cdn.polyhaven.com/asset_img/thumbs/fir_tree_01.png?width=630&quality=95&v=55f25e61
- https://cdn.polyhaven.com/asset_img/thumbs/fir_sapling_medium.png?width=630&quality=95&v=e66bf476

The exact downloaded preview responses used for selection were stored only in
session reference space, not the repository or game. Their actual SHA-256 /
bytes were:

- Jacaranda: `1985D3566831F6F22D0CAF4A3EC8FF724167AF8195A6353DBCD26BA26316097C`
  / 529,864.
- Island Tree 02: `C09D51153794BAA50EEE54A41AF589923E637896367C4AA726036189C03D90B9`
  / 437,890.
- Fir Tree 01: `0E9F9F163DF752673B25FC2F190E827D5047582B7E64D35929EF79A3FF4DBDEA`
  / 511,876.
- Fir Sapling Medium: `8E700E671C26265AFE51B507F93707217433AA451DABE50F54DCE74F2952B3B9`
  / 375,779.

These hashes prove which preview bytes were inspected; they do not license
preview-image redistribution or establish the binary model's appearance.

These images support silhouette selection only and are not shipped. This is the
entire bounded palette. Pine Tree 01 was excluded for an even larger 17.4M
published polygon source and redundant tall-conifer role; Island Trees 01/03
were excluded as near-family variants; desert/quiver trees are incoherent.

No new source is import-ready. Acquisition, actual SHA-256, FBX model/LOD/UV/
normal/root inspection, guarded preparation, native materials/collision and
ordinary game screenshots are all future MAIN-owned gates. Source preparation
is not visual acceptance.
