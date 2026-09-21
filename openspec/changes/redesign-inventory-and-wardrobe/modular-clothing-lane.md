# Modular clothing source lane

## Authorization and ownership

The 2026-09-20 coordinator kickoff explicitly authorizes independent source
implementation now, superseding the earlier environment-completion gate for
this isolated lane only. Central proposal/design/tasks remain owned by the UI
lane. Environment acceptance, Unreal import, character wiring and playable
acceptance are not established by this source handoff.

Owner: session 56c0f38f-6725-4888-a360-a369d9422d53, branch
`jennifergalley-homestead-modular-clothing`. Changes are confined to new modular
scripts in `Scripts\Characters`, `Assets\Characters\ModularClothing`, focused
source checks, and this addendum. Existing character asset sources, 18 joined
meshes, central launch/build scripts and other checkouts remain unchanged.

## Source and implementation contract

Use Blender 4.5.14 CPU/background with factory startup and process-local
configuration/temp directories. Reuse verified original local MPFB 2.0.17
sources where reconstruction is required. Do not switch to Blender 5.2, download
or execute asset scripts, open private portraits, change the shared profile,
launch Unreal tools or use GPU rendering.

Preserve the Preferred, Willow and Hazel adult shapes, three existing hairstyles,
face materials, original MakeHuman game_engine bind and existing animations.
Reconstruct any missing geometry from verified original data, not the
destructively masked/joined FBX. Each base must remain permanently modest with
complete feet when every removable item is absent.

Jenny's direct follow-up specifies a simple bra and underwear as two separate
pieces, not a union suit or long undergarments. Both are permanent opaque base
geometry with distinct `M_Modular_BaseBra` and `M_Modular_BaseBriefs` roles,
never wearable inventory items. Torso/waist skin between the pieces remains
visible; coverage checks target breast and pelvic regions, not the whole torso.

| Definition | Slots | Dye | Fit / output key |
| --- | --- | --- | --- |
| `linen-tunic` | Torso + Legs, one instance | Moss/Wine/Slate/Flax | `SK_Modular_{Body}_Tunic` |
| `linen-apron` | Apron, requires linen-tunic | Moss/Wine/Slate/Flax | `SK_Modular_{Body}_Apron` |
| `legacy-laceup-shoes` | Feet, includes socks | None | `SK_Modular_{Body}_Shoes` |
| `woven-footwraps` | Feet, excludes shoes | None | `SK_Modular_{Body}_Footwraps` |
| Permanent base | Not an item or slot | None | `SK_Modular_{Body}_Base_{Hair}` |

`{Body}` is Preferred/Willow/Hazel; `{Hair}` is LongWave/Bob/Ponytail.
Source output root is `Assets\Characters\ModularClothing`; intended Unreal root
is `/Game/SurvivalGame/Characters/ModularClothing/{Body}`. These paths are an
import contract, not existing cooked assets. All pieces use metres in Blender,
FBX_SCALE_UNITS and the existing -Y forward/+Z up export convention. Only
garment linen roles receive dye ColorTint; skin, hair, eyes, trim, leather, metal,
footwear and the permanent base never inherit garment dye.

## Lane checks and handoff

- [x] Inspect source geometry and reconstruct complete bodies where needed.
- [x] Build three permanently covered bases with retained hair/face choices.
- [x] Export four independently fitted garments per body and nine base FBXs.
- [x] Verify source counts, feet/coverage, units, bind names/transforms,
  normalized <=4 weights, material assignments and real FBX round trips.
- [x] Sample existing movement/action deformation and inexpensive CPU previews;
  document limits rather than claim gameplay or aesthetic acceptance.
- [x] Record source/license/output hashes, import/material mapping and commands.
- [x] Commit via existing LFS rules, verify private remote and push this branch.

Source delivery: `1425aff5ca585d2cc3d9d366922201784ffdfe5b`; runtime API
delivery: `11ba95c`. Both are on the verified private branch. The source commit
contains all three packed scenes, nine bra/briefs bases, twelve garments and
three CPU review sheets. Its 35 new LFS objects uploaded successfully. All 21
FBX round trips and 75 sampled body/action states passed; the read-only
`check_modular_manifest.py` hash/coverage/catalog check passes against the
persistent files. This records source delivery only, not Unreal acceptance.

## Added presentation-source ownership

The coordinator explicitly extended this lane on 2026-09-21 to own
`HomesteadCharacter.h/.cpp` and narrow new wardrobe-presentation helper files.
Controller/widgets, authority/save code and all Unreal tool execution remain
outside this lane. This extension does not authorize an import or build.

The Controller-facing API agreed with the UI owner is:

```cpp
bool PrepareEquipment(const Homestead::State& CandidateState,
    const FHomesteadAppearance& Look, FString& Error);
bool ApplyPreparedEquipment(FString& Error);
void ClearPreparedEquipment();
bool IsEquipmentPresentationReady() const;
const FHomesteadEquipmentPresentation* GetEquipmentPresentation() const;
```

Preparation is non-mutating with respect to authoritative ownership and visible
components. It validates the candidate's references/slots, all owned garment
definitions (including carried craft results), actual cooked mesh/material
availability, body fit and identical original rig; then retains meshes and
prepared dynamic materials. The UI synchronously executes candidate transaction,
prepare, identical real transaction and apply on the game thread, clearing a
prepared candidate after any rejected real transaction. Apply uses retained
references, performs no asset loads, and cannot ordinarily fail after prepare.
The exposed presentation snapshot is render data only, never item authority.

Modular components follow the existing body pose without new animation graphs,
collision or cloth physics. The existing mesh component, animation instance,
held-tool attachments and nonclothing tint/eye parameters remain authoritative.
Legacy joined appearance remains usable before wardrobe activation but is not
a fallback for missing modular equipment and cannot replace an active wardrobe.

- [x] Implement and hand off the prepared presentation API and follower parts.
- [x] Check catalog/path/material consistency and rejection behavior in source.

Unreal import/build, actual cooked admission and ordinary gameplay/visual
acceptance remain coordinator-owned pending work. Central 4.1/4.2/4.3 must not
be marked fully complete merely because this source lane passes.

## Native compiler incident and evidence limits

At 2026-09-20 23:03:05 Arizona the source lane invoked `cl` through
`vcvars64.bat`/`cmd.exe` for `HomesteadWardrobeSelectionTests.cpp` and
`HomesteadSimulation.cpp`, outside the coordinator's guarded compiler route.
The test executable passed its functional selection/rejection checks, but this
is not a clean admitted compiler run. Output timestamps strongly correlate the
launch with VCTIP PID 48608 (parent 45016); no retained parent-PID proof exists.
No historical network/upload assurance is inferred.

The coordinator later verified the exact helper process handle/path/creation
`2026-09-21T06:03:05.243905Z` and hash beginning `D591358C`, stopped only that
PID, observed its death and a fresh empty uploader enumeration. No unrelated
process was changed by this lane. Compiler work was paused; further compiler
execution requires the supplied unchanged guarded route and main coordination,
not direct `cl`, `vcvars`, or compiler-version probes. UE compilation remains
pending in the main-owned slot.
