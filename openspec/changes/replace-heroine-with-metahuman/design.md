# Design

## Context

See proposal.md for motivation. Observed on 2026-09-25 in the live editor through the MCP tooling:

- `AHomesteadCharacter` is an `ACharacter` (CharacterMovement) with one custom 53-bone skeletal
  mesh, separate no-collision garment components, and a native `UHomesteadAnimInstance` that blends
  idle/walk by speed and plays work clips after simulation transactions. There's no foot IK, root
  motion or motion matching. Assets load by hard-coded paths in `LoadHeroineAssets`.
- MetaHuman Creator opens in 5.8 but runs "with limited features" until the engine's
  `Plugins/MetaHuman/MetaHumanCharacter/Content/Optional` is installed. Opening a MetaHuman flags
  missing `r.GPUSkin.Support16BitBoneIndex`, `r.GPUSkin.UnlimitedBoneInfluences` and
  `r.SkinCache.CompileShaders`.
- `DefaultEngine.ini` sets Lumen and Virtual Shadow Maps but targets no SM6 shader format, so
  VSM is inactive. The editor also warns that Lumen has no ray-tracing data, because neither
  distance fields nor hardware ray tracing are enabled.
- `MetaHumanCharacterEditorSubsystem` exposes creation, body constraints, face state, skin/eyes,
  `request_texture_sources`, `request_auto_rigging` and `build_meta_human`. Agents reach it through
  the opt-in `run_python` MCP tool (`Scripts/Start-EditorMcp.ps1 -AllowPython -ExtraPlugins ...`).
- Target PC: RTX 5080 (16 GB), Ryzen 7 3700X (8 cores), 32 GB RAM. Shader compilation will be
  the slowest step.

## Goals / Non-Goals

**Goals:**
- One approved photoreal heroine in ordinary play first, then motion quality, then appearance
  breadth.
- Reuse Epic's MetaHuman and GASP pipelines rather than re-inventing rigs, retargets or
  motion-matching databases.
- Keep simulation authority, input mappings, the menu and save behavior intact.

**Non-Goals:**
- Runtime face or body sliders and additional body presets (deferred until one heroine is
  approved).
- Netcode, the Mover plugin, traversal (vaulting/climbing), combat.
- Changing Homestead's material framework to Substrate.

## Decisions

1. **Author in MetaHuman Creator, driven by scripts plus Jenny's review.** Start from the preset
   closest to the face/hair reference. Set body constraints (about 160 cm, feminine, curvy, moderate
   muscularity) and eyes/skin through the subsystem. Iterate with Appearance-preview and woodland
   captures from the MCP tooling. Download 4K texture sources (8K costs VRAM and disk for little
   gain at game distance), create a full rig, and assemble with **UE Optimized / High** into
   `/Game/Characters/Heroine_MH/`. Keep the `MHC_` asset so the look stays editable. Alternative:
   Mesh-to-MetaHuman from the current MPFB head. Rejected because that face is the problem.
2. **Keep `AHomesteadCharacter`; swap its mesh stack.** The body becomes the leader
   skeletal mesh; face, grooms and garments follow it as leader-pose/attached components, matching
   the MetaHuman Blueprint's structure. Asset paths move from scattered `LoadObject` calls into one
   heroine data asset, so trials can swap without code edits. Alternative: adopt GASP's
   `CBP_SandboxCharacter` Blueprint wholesale. Rejected because it would replace working movement,
   camera, input and save code.
3. **Animation Blueprint on a C++ base.** The body uses a Blueprint Animation Blueprint whose
   parent class is `UHomesteadAnimInstance`, so C++ still supplies locomotion inputs and work-action
   state while the graph hosts GASP's Pose Search/Chooser nodes. Work actions become montages
   triggered after successful transactions, which keeps authority in the simulation. Add a
   `UCharacterTrajectoryComponent` to feed motion matching. The existing velocity blend remains the
   fallback until motion matching passes review.
4. **First playable uses retargeted existing clips.** An IK Rig for the 53-bone skeleton plus an
   IK Retargeter (auto chains/auto align) moves idle, walk, sprint and all work clips onto the
   MetaHuman skeleton. This gives an end-to-end playable slice before GASP is migrated. The
   retargeted clips are interim quality.
5. **GASP by migration, not dependency.** Jenny adds GASP from Fab and creates the sample project.
   Its animation content (databases, choosers, animation sequences, IK rigs, retargeters, ABP) is
   migrated into `/Game/Animation/GASP/` and trimmed to locomotion, idles and turns. Traversal and
   combat content is dropped.
6. **Clothing: fixed-size garments on the one body first.** Author the tunic, apron and footwear in
   Blender against the assembled MetaHuman body (via DCC export), weight them to the body skeleton,
   and import them as garment skeletal meshes with dye parameters. Use Chaos Cloth only where the
   apron or skirt hem needs motion. A Fab MetaHuman garment may stand in for the starter outfit if
   one fits the rustic brief. Parametric Chaos Outfits wait until more body presets return.
7. **Rendering baseline as its own first increment.** Add `PCD3D_SM6`, hardware ray tracing,
   Lumen hardware RT, mesh distance fields (for software fallback) and the three skinning settings
   to `DefaultEngine.ini`. Enable the MetaHuman, Pose Search, Chooser, Motion Warping, Hair Strands,
   RigLogic and Control Rig plugins in the `.uproject` when the heroine first ships in play. Measure
   the packaged build before and after at 720p and 4K.
8. **Hair:** strand grooms at LOD0 and near LODs, with the groom's card LOD at distance.
   Hairstyle and colour map to groom assets and their melanin/redness parameters at runtime.
9. **Player-configurable colour:** hair, eye and skin colour are Appearance options, not baked
   choices. Hair colour drives the groom materials' Melanin/Redness (brows and lashes follow,
   slightly darker). Eye colour drives the eye material's iris parameters, seeded from Creator's
   eye presets. Skin tone can't be a single runtime tint, because MetaHuman skin is synthesized
   into textures. Author a small set of skin-tone texture variants of the same face in Creator,
   then swap those texture sets on the face and body materials at runtime. The authored defaults
   are dark brown hair (Melanin 0.72, Redness 0.35), eye preset 8 (green) and the heroine's own
   skin. `Content/Python/homestead_agent/metahuman_look.py` records the authoring values.
   Alternative: separate assembled MetaHumans per skin tone. Rejected: that duplicates the rig
   and meshes for a texture-only difference.

Independent lanes and owned files:
- **A. Rendering:** `Config/DefaultEngine.ini`, `SurvivalGame.uproject` (plugins). Needs no other
  lane.
- **B. Authoring:** `/Game/Characters/Heroine_MH/**` content. Needs Jenny's optional-content
  install and sign-in.
- **C. Integration:** `HomesteadCharacter.*`, `HomesteadAnimInstance.*`, `HomesteadAppearance.*`,
  the new heroine data asset, and the Appearance/portrait code in `SHomesteadMenu.cpp`. Needs B's
  first assembly and lane A's skinning settings.
- **D. Animation:** IK rigs/retargeters, `/Game/Animation/**`, and the body AnimBP. The retarget
  part needs only B's skeleton; the GASP part needs Jenny's Fab step.
- **E. Clothing:** Blender sources under `Assets/Characters/`, garment content. Needs B's body.

The shared interface is the heroine data asset (mesh, groom, garment, anim class references) and
the anim-instance variables C++ exposes. Define both in C before D and E integrate.

## Risks / Trade-offs

- [Full shader recompile after SM6/ray tracing on an 8-core CPU may take hours] → run it once in
  lane A early, reuse the worktree DDC, and don't interleave other engine-heavy jobs.
- [Repository and package size grow by gigabytes (LFS)] → keep only the assembled assets actually
  used; 4K rather than 8K textures; record size in the build receipt.
- [MetaHuman tooling in 5.8 has known issues and some experimental parts] → use only
  Creator/Assembly (non-experimental), save the `MHC_` asset and assembled output separately, and
  keep the prototype heroine as rollback.
- [GASP's Blueprint AnimBP expects its sample character's variables] → port the needed
  inputs into `UHomesteadAnimInstance` and verify in ordinary PIE play before deleting the old blend.
- [Retargeted hand-keyed work clips may look worse on a realistic body] → accept them only as
  interim. Replacement mocap sources (Rokoko free packs, CMU, GASP subsets) are evaluated in lane D
  with licences recorded.
- [Cloud steps require Jenny's Epic login] → agents prepare everything else and stop at the
  prompt; credentials are never handled by agents.
- [Photoreal skin under the game's current bright/golden dawn grade may look worse than
  expected] → review in daylight, dusk and firelight as part of the acceptance scenarios, and
  adjust exposure/grading within lane A if needed.

## Open Questions

- Which MetaHuman preset is the closest starting face: chosen together with Jenny from captures
  once the optional content is installed.
- The mocap source for work actions beyond retargeted clips: decided in lane D after comparing
  candidates on the new body.
