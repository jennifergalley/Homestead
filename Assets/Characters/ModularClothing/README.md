# Modular clothing source handoff

**Source-ready; Unreal import/build/play acceptance pending.** These are new
assets, not replacements already installed in the game. Existing joined
character meshes and source scenes remain unchanged.

## Contents and interface

Three packed authoring scenes, nine complete body/hair base FBXs and twelve
independent garment FBXs. `manifest.json` maps every export to its intended
Unreal object path under
`/Game/SurvivalGame/Characters/ModularClothing/{Preferred|Willow|Hazel}`.
Body indices are 0 Preferred, 1 Willow, 2 Hazel; hair indices remain 0 LongWave,
1 Bob, 2 Ponytail. The later hairstyle-refinement round is a separate change.

| Definition | FBX suffix | Slots / dye |
| --- | --- | --- |
| `linen-tunic` (0) | `Tunic` | One instance in Torso(0)+Legs(1); dye 0..3 |
| `linen-apron` (1) | `Apron` | Apron(2); requires tunic; dye 0..3 |
| `legacy-laceup-shoes` (2) | `Shoes` | Feet(3), includes socks; authored dye 0 only |
| `woven-footwraps` (3) | `Footwraps` | Feet(3), excludes shoes; authored dye 0 only |
| Non-item permanent base | `Base_{Hair}` | Never owned, removed, crafted or recolored as clothing |

Names are `SK_Modular_{Body}_{Suffix}`. All source meshes use the original
MakeHuman game_engine 53-bone bind, metre authoring coordinates and the incumbent
FBX_SCALE_UNITS/-Y-forward/+Z-up exporter. Import into the original
`/Game/SurvivalGame/Characters/Heroine/SK_Heroine_LongWave_Skeleton`; do not
create a separate garment skeleton or physics asset. Scene units convert to cm.
Keep all bones/parent order/rest transforms for leader-pose compatibility.
Explicitly include the new content root in the coordinator's cook.

## Recovery and permanent coverage

The incumbent `.blend` bodies already lacked feet because their shoe masks
were applied before saving. The new builder reconstructs each full adult body
using its exact saved MPFB macro/detail recipe, verifies every retained original
vertex is within 0.1 mm, and adopts the unchanged original rig. It does not
reverse-engineer deleted regions from the joined meshes.

A simple opaque bra and briefs are built into each base as two separate pieces,
with an exposed waist and clean-cut seams, per Jenny's follow-up. Each has its
own source mesh/material; bra straps stay attached to the bra. Cloth replaces
the covered body surface rather than hiding a separate naked mesh; the pieces
cannot be removed or traded. The complete feet remain present.
There are no equipped-dependent body deletion masks. Original head/face and
hair direction remain unchanged in this clothing round.

Tunic/apron retain the existing original fitted geometry. Shoes/socks are the
verified CC0 source. Footwraps are original per-body closed cloth slippers,
ankle cuffs and crossed binding geometry derived from foot cross-sections.

## Materials

Each body's `materials.json` is the import contract. Preserve exact slot order
from its source report; the presentation adapter rejects missing/reordered slots.
For textured fixed base/footwrap materials, multiply the texture by `base_color`
and apply `uv_scale`; other legacy textures replace base color as before.
Use DefaultLit, two-sided rendering, masked hair/brows/lashes/eyes and opaque
body/clothing. Permanent bra/briefs are never masked.

`ColorTint` applies to skin/hair appearance and to the equipped instance's
`M_Heroine_MossLinen` or `M_Heroine_ApronLinen`. The four dye multipliers are
recorded explicitly. Neutral trim, belt, brass, shoes, wraps and base retain
their authored colors. Eyes preserve iris-only `IrisColor`/`IrisMix`. Do not tint
all slots, duplicate socks as a separate item, or recolor held tools.

## Reproduction and checks

```powershell
.\Scripts\Characters\Build-ModularClothing.ps1 -Render
.\Scripts\Characters\Build-ModularClothing.ps1 -VerifyOnly
python .\Scripts\Characters\check_modular_manifest.py
```

After a deliberate rebuild and refreshed review sheets, regenerate the binary
inventory with `check_modular_manifest.py --write-inventory`, then run the
read-only check again. `asset-hashes.json` covers packed scenes, FBXs, textures
and retained review PNGs.

The runner copies the existing verified MPFB 2.0.17 add-on into an ignored
lane-local profile only when needed. Blender 4.5.14 runs factory-startup,
offline, disabled auto-execution, two CPU threads and lane-local temp paths.
The shared profile is never changed. No new downloads or asset scripts run.

`validation.json` records real import round trips for all 21 FBXs, geometry and
slot counts, original bind errors, normalized <=4 weights, metre bounds,
permanent-region coverage and complete feet. Existing relaxed idle, grounded
walk, gather/weeding, watering and clearing clips each get five deformation
samples per body (75 samples). `Review` contains CPU contact sheets, ordered
base front, base back, base with wraps, tunic/shoes, apron walking, apron
gathering. These are source review, not game screenshots.

Nearest-surface overlap numbers are diagnostics, not collision proofs (open
edges and the tunic's double shell distort signs). Source samples retain modest
coverage and show no exploded skinning, but they do not prove every intermediate
pose is free of clipping. Existing shoulder-strap geometry and action contact
remain modest prototype art. Main must inspect ordinary in-game movement.

## Runtime source API

`HomesteadCharacter::PrepareEquipment(State, Look, Error)` resolves and retains
the complete body plus all currently owned garment definitions, including
carried craft results. It checks actual cooked content, exact bind/material
roles, shading, opacity, parameters and bounds without changing visible
equipment or inventory. Missing assets reject explicitly; legacy joined meshes
are not a fallback.

Controller runs candidate transaction -> prepare -> identical real transaction
-> `ApplyPreparedEquipment` synchronously. After rejected commit call
`ClearPreparedEquipment`. Apply uses retained materials/meshes with no loads,
keeps the existing body component/animation/tool attachments, and uses three
leader-pose followers. `GetEquipmentPresentation()` exposes the same retained
surface data for the inventory preview, never another inventory.

Native `HomesteadWardrobeSelectionTests.cpp` exercises valid fits, all-owned
craft admission, base-only, duplicates, invalid dye/definition/fit, two-slot
reference mismatches, conflicting footwear and dependent apron rejection.
Its initial functional pass used an unguarded compiler invocation; the scoped
OpenSpec lane addendum records the correlated helper-process incident and
coordinator cleanup. It is not clean compiler-route evidence.
UE source compilation, cooked material admission, preview wiring and actual
gameplay verification remain main-owned and pending.
