# Lush woodland: direct resource replacement map

Baseline: main `5ccddea` / directional-navigation-06. Selection date: 2026-09-21.
This is the source lane; main owns world composition and engine execution.
Jenny rejected the current sparse/prototype appearance. No repeat license
survey or new marketplace pack is needed.

## Reuse now

The current world caps the authored grove at 16 and spaces it by twice the
full canopy radius, then omits every unselected primitive tree. Main identified
15 actual trees. Restore layered enclosure and overlapping crowns through
composition, not an unrelated new biome. Existing prepared Tree Small 02
has measured 231,785 triangles in the selected near mesh; frozen LOD3 is
170,289, only a modest cheaper tier, not a solution for 60+ dense trees.
Near/mid/far selection must respond to camera distance, not permanently assign
a coarse tree that can later be approached.

Already imported grass meshes have 1,257 / 653 / 290 / 79 triangles, not the
publisher's million-polygon scatter estimate. Reuse those, the four Fern 02
meshes, Grass Ground blend, Brown Mud Leaves and moss rocks. Dense grass/fern
decoration alone will not replace the cylinder/sphere resource plants.

## Four selected acquisitions

All are Poly Haven CC0-1.0, public no-account assets. Existing policy evidence
is S04/S05; asset-specific free/CC0 metadata was checked for the new shrub,
branches and flower on this date. Fir Sapling reuses S28 admission and its
current file manifest. No source binaries had been fetched at selection time.
Exact proposed URLs/bytes/MD5 are in
`Assets\Environment\WoodlandResources\candidate01\asset-manifest.json`.

| Priority and game role | Exact asset / author | Visual evidence and adaptation | Source implications |
| --- | --- | --- | --- |
| 1: persistent berry-bush base | [Shrub 04](https://polyhaven.com/a/shrub_04), Rico Cilliers | Preview shows four healthy broadleaf woody shoots. Use measured shoots in a compact multi-stem base, not another sphere. Keep berry produce separately removable; no berries or edible species are supplied. | Aggregate layout 0.747 x 0.187 x 0.281 m, 47,813 published polygons, advertised LODs. 1K FBX 852,540 bytes; diffuse/DX normal/roughness/AO/alpha/mask. Actual per-part dimensions, slots, LOD chain and active UV channels await source inspection. |
| 2: gatherable branch group | [Dry Branches Medium 01](https://polyhaven.com/a/dry_branches_medium_01), Rico Cilliers | Preview shows three irregular bark-covered branches. Replace the three cylinder produce pieces; the entire group disappears on gathering and follows existing renewal. | Aggregate 1.059 x 1.299 x 0.341 m, 16,803 published polygons; 1K FBX 699,532 bytes. Opaque diffuse/DX normal/roughness/AO; no alpha or LOD chain established. Select actual branch geometry rather than combine an arrangement blindly. |
| 3: small gatherable tree | [Fir Sapling](https://polyhaven.com/a/fir_sapling), Rob Tuytel photography / Rico Cilliers modeling | Preview shows three young dense conifers, not a mature canopy. Replace cone crowns and stick trunks. Main preserves produce/base/clearing behavior and camera collision deliberately. | Aggregate layout 2.743 x 0.826 x 1.301 m, 433,021 published polygons; 1K FBX 19,037,468 bytes. Branch/twig PBR families, twig alpha/mask. Actual per-variant geometry/slots/UV/LOD/wind unknown; do not read layout width as one tree's width. |
| 4: visible flower resource and restrained accents | [Flower Empodium](https://polyhaven.com/a/flower_empodium), Jenelle van Heerden photography / Rico Cilliers modeling | Preview shows green narrow leaves with small yellow star flowers. Replaces conspicuous cone/cylinder/sphere flowers, not merely scattered decoration. Preserve a depleted plant base if retained by gameplay. | Aggregate layout 1.323 x 0.284 x 0.268 m, 5,820 published polygons, advertised LODs; 1K FBX 139,500 bytes. Diffuse/DX normal/roughness/AO/alpha. Whether blossoms are separable by geometry/material/atlas is unverified, so do not promise independent flower removal until inspected. |

All dimensions above come from publisher millimetre metadata and can describe
multi-object display layouts. Published polygons are neither triangle counts
nor a runtime budget. Source FBXs must be inspected before placement/scaling.
PBR/alpha inventory does not prove material-slot count or a working LOD chain.
Do not repeat the earlier tree mistake: active UV can differ by material role.

## Remaining roles without more acquisition

- **Roots:** a modest low-leaf cluster derived from selected leafy shoots or
  existing natural plant geometry can replace flattened leaf spheres, with
  the root produce mesh kept separate. Label this an original generic forage
  plant, not a botanically accurate root crop. Avoid turning a tall tree into
  a tiny herb by arbitrary scaling.
- **Reeds:** use the admitted tall grass clump as a starting bank-grass silhouette,
  preserving grouped stalk/produce state, rather than calling a fern a reed.
  This is generic fiber-bearing bank grass, not a verified cattail asset.
  Geometry height and repetition must be judged in game; a large uniform scale
  cannot be assumed to make a 32 cm clump convincing.
- **Loose stones:** reuse moss-rock geometry/material at measured natural small
  sizes instead of gray spheres where the source variant supports it.
- **Fallen log:** [Dead Tree Trunk](https://polyhaven.com/a/dead_tree_trunk),
  Rob Tuytel, was previewed and fits weathered/mossy woodland. It is a roughly
  3.05 m log, not a bundle of small sticks. Its 101,802 published polygons and
  7,165,036-byte 1K FBX are not needed to unblock the four priorities; acquisition
  is explicitly deferred. No new log resource kind is proposed.

## Preview evidence

Publisher previews were downloaded only into session reference storage and
viewed. They are not shipped or assumed CC0 images:

- Shrub 04: https://cdn.polyhaven.com/asset_img/thumbs/shrub_04.png?width=630&quality=95&v=8bfaf1f2
- Dry branches: https://cdn.polyhaven.com/asset_img/thumbs/dry_branches_medium_01.png?width=630&quality=95&v=5fecfa26
- Fir Sapling: https://cdn.polyhaven.com/asset_img/thumbs/fir_sapling.png?width=630&quality=95&v=bb4db69e
- Flower Empodium: https://cdn.polyhaven.com/asset_img/thumbs/flower_empodium.png?width=630&quality=95&v=0ea6897e

Shrub 02 was inspected and rejected for this berry-base role: its sparse
lanceolate/twiggy appearance is less suitable than Shrub 04's healthy broadleaf
shoots. This is a visual selection, not a claim that it is an inferior asset.
An exact `tree_small_01` lookup returned 404; no alternative tree was invented.

## Source admission and handoff

Main reserved these four exact assets to this isolated lane and prohibited
duplicate downloads. Reuse explicit hosts, no redirects, expected byte counts,
publisher MD5 and actual SHA-256 receipts, fresh ignored source paths and live
stop controls. Texture choices are a small 1K starting set: diffuse, DirectX
normal, roughness, AO and needed alpha/mask, not source .blend scenes, GL/EXR
auto-dependencies, displacement or a full resolution catalog. The raw files
remain unchanged.

Main executes any source authoring using the admitted official tool/guard and
its reserved slot. This lane supplies source facts and a narrow preparation
script, not a new authoring system. Generated assets require measured per-role
UVs, normals/tangents, referenced bounds/root and separable harvest parts before
a final contract. Do not call imported components or thumbnails a beauty pass:
the next ordinary gameplay screenshots must show a coherent living woodland
and authored resource silhouettes.
