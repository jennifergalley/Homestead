# Design

## Context

See `proposal.md` and the four capability specs. The selected `inventory-drop-v18` game has a 53-bone CC0 MPFB heroine with three body presets, three hair selectors, modular owned garments and original icons. It is a valid prototype, but Jenny explicitly dislikes its visual result. Its locomotion proxy plays one authored in-place one-second walk at a velocity-scaled rate, then overlays gathering/chop/till/water; neither a dedicated sprint nor a held Knife exists. The Knife already starts in inventory and is required for low-growth clearing, gathering, garment crafting and ordinary tool recipes; low-growth `Clear` currently plays the Hatchet chop.

Craft already has a pure authoritative recipe assessment, but Slate renders every requirement as a large colored three-line text card. The saved hotbar preassigns Knife, Hatchet, Digging Stick and Watering Can and paints missing tools as ghost icons. These are presentation gaps, not missing inventory/crafting rules.

## Goals / Non-Goals

**Goals:** A replacement-first heroine selection, more natural walk/sprint and real Knife presentation, backed by quieter icon/count Craft and truthful earned-tool icons. Preserve current save/ownership and selected-preview rollback until the replacement passes ordinary play.

**Non-Goals:** Replicating MetaHuman's facial rig/groom quality, buying assets, new accounts, combat, new Knife yields, a new stamina meter, adopting a full motion-matching framework without evidence, or tying character improvement to the separate far-world streaming/time-card scope.

## Decisions

### 1. Select a replacement through licensed, representative trials

The current MPFB character remains a known-good rollback, not the preferred art target. Shortlist only freely accessible candidates whose exact per-item rights permit inclusion in a packaged Unreal game. Epic's [modular character guidance](https://dev.epicgames.com/documentation/en-us/unreal-engine/working-with-modular-characters-in-unreal-engine) names City Sample Crowds as an example and documents Leader Pose, Copy Pose and mesh-merge trade-offs. Its actual Fab listing/license, engine compatibility, download access, render cost and suitable female head/body/clothing coverage are **not verified** by this planning pass. Do not request credentials, create an account, purchase content, or redistribute unapproved source art.

Trial assets import into fresh, isolated content paths and never overwrite the current skeletal meshes. Compare an actual normal-game heroine with a baseline at the same camera, daylight, weather, outfit and locomotion route. Require face and body improvement alongside real hair-color, supported hairstyles, adult body variants, owned garment identity/dye and clothed seams. If a candidate cannot cover those combinations, reject it rather than silently fall back per style. Reuse the existing Leader Pose wardrobe only if skeleton topology and render cost remain suitable; otherwise retarget the retained clips and adapt garments in a scoped trial before promotion.

If no available external candidate clears legal and visual gates, substantially re-sculpt/re-author the retained CC0 face/body and improve skin/eyes/hair/fabric materials on the existing Blender/Unreal path. A relight or recolor without a visibly better face and silhouette is insufficient. Preserve the baseline and rejected trials for honest A/B review. At ordinary third-person distance an appealing, coherent indie-realistic heroine is attainable without MetaHuman; close-up digital-human skin, hair groom, detailed facial expression and photoreal facial animation are not promised.

**Alternative:** import the first attractive Fab thumbnail or keep polishing only the old material. Rejected because store renders do not prove fit or rights and Jenny has rejected the current face/body.

### 2. Build movement on the chosen character, not an isolated mannequin

The near-term movement slice reuses the existing 180 cm/s walk/CharacterMovement authority and work-action proxy. Retarget or re-author a better walk with planted starts/stops and turn response plus a distinct sprint clip on the final trial skeleton. The pending `improve-contextual-feedback-and-sprint` spec sets toggle Shift/L3, 300 cm/s, zero sprint-specific Energy cost and a 25% Energy admission/auto-off threshold; below 10% it slows walking and refuses tool work without an Estate failure state. Preserve that contract rather than inventing another stamina system. The pending `polish-locomotion-view-distance-and-time-hud` spec owns the broader walk/turn/slope set. Reconcile overlapping task ownership before implementing; do not mark those plans complete from source or planning evidence.

Jenny rejected the fitted Vitruvian face and neck seam after playing the trial. The licensed CMU slow/normal gait improved the arms and legs but tilted the head backward. Retain the useful movement as a separate original-heroine trial, correct the measured head/neck pitch in the animation without changing arm/leg steps, and compare moving side views. Do not attach future movement trials to the rejected face or treat preserving the old face as completing the replacement-art goal. Jenny preferred the trial's menu scaling; retain it independently of heroine acceptance.

Epic's [Game Animation Sample](https://dev.epicgames.com/documentation/en-us/unreal-engine/game-animation-sample-project-in-unreal-engine) is a documented high-fidelity motion/retargeting reference; the current Fab listing's licensing, availability and skeleton fit have not been verified here. A reversible, narrow retargeting trial may compare an admitted walk/start/stop/sprint subset to the original Blender-authoring path. Prefer the best in-game result; do not migrate the entire Motion Matching/Chooser/traversal stack just to address one walk and sprint. Both methods keep root travel and survival transactions out of animation.

**Alternative:** multiply the existing walk clip rate. Rejected because a faster stiff loop still lacks weight transfer, stopping and turning motion.

### 3. Use the carried Knife in the existing work flow

Add one small original noncolliding Knife component at the right-hand attachment, preserving the current project icon/prop material language. Derive visible state from live carried count and transient hotbar hover/selection; mouse hover can preview the held Knife without changing the selected slot or allowing an action. When the Knife is selected, show it in idle/walk/sprint only if grip, clothing and camera remain clean. Menus/planning, loss of inventory ownership, load/appearance replacement and other selected actions clear or rebind it deterministically.

Separate Knife-specific cutting from Hatchet felling in the current single-action presentation coordinator. A successful existing low-growth `Clear` requests one short cut/recover; successful Knife-dependent reed gathering may request a restrained assisted gather. Ordinary berries/roots can retain the admitted bare-hand gather where appropriate. All yields, timing, reach and removal remain Simulation-owned and commit **before** animation. A failed, interrupted or distant action never awards anything or queues an animation replay.

**Alternative:** merely attach a Knife while continuing to play Hatchet chopping. Rejected because the work would still look wrong.

### 4. Represent earned tools without changing hotbar capacity or saves

Retain the saved ten-slot array and `1-9/0` positions so Jenny's earlier hotbar request and existing saves remain meaningful. Keep stable canonical references for the four tools; drive icon visibility from `HotbarSnapshot().Available`, not `Assigned`. Uncrafted, stored or dropped tools have no ghost icon, though an empty numbered slot remains for muscle memory. On acquisition the assigned icon appears automatically; when a tool leaves/re-enters the pack, its icon disappears/returns without changing the assigned item or granting possession. Mouse hover and selected tool are separate presentation states.

**Alternative:** dynamically shrink the hotbar or delete absent assignments. Rejected because ten-key positions and saved slots would jump and stored tools could lose their intended slot.

### 5. Compress crafting details around the existing assessment

Keep the icon-only recipe tiles, 1.2-second hold and authoritative `RecipeAssessment`. Replace the stacked `Have / Need` cards with narrow item-icon, one-word name and `have / need` rows. Use a distinct check/shortage shape and color; short acquisition hints appear only for a focused/hovered unmet row. A retained Knife/Hatchet carries a clear `kept` marker, a cookfire shows its true ready state, and pack space is emphasized only when actually blocking. Full item/count/source status stays available to accessible navigation. Test all six recipes, blocker combinations and a 720p maximum-details case against the actual assessment rather than reproducing crafting logic in Slate.

**Alternative:** remove requirement detail entirely. Rejected because blocked players still need to discover Fiber from Reeds and distinguish kept tools from consumed materials.

### 6. Order small playable increments and coordinate ownership

1. Record baseline real-game character walk/face/Knife and recipe/hotbar screenshots at 720p/4K. Establish free-character legal/access shortlist and one normal-game candidate/fallback comparison before committing to a new base.
2. Ship a character-first integrated trial with the chosen/re-authored heroine plus improved walk, start/stop, sprint and one Knife action. The selectable `inventory-drop-v18` build remains intact.
3. Add compact Craft requirements and earned-tool icon behavior, then inspect them in the same ordinary play route; ship an intermediate candidate if visual quality already clears the first slice.
4. Complete body/hair/outfit variants, idle/portrait coordination and terrain/turn refinements against the existing related OpenSpec plans. World streaming and time/weather card remain separately staged.
5. Run full portable/source/native/ordinary 720p/4K, save/reload, generated-world, full-loop and matched cadence gates. Seal only a genuinely better immutable Shipping candidate and isolate a fresh preview profile.

Independent lanes can research candidate rights/geometry/garment fit without launching the shared Editor, and build icon-first Craft from the existing assessment. A character-animation lane owns source clips and their verification. `HomesteadCharacter`, `HomesteadAnimInstance`, `HomesteadController`, project icons, Editor/import/cook/package, final integration and promotion are serialized behind agreed interfaces. Reuse current DDC, cooked-compatible assets and current release evidence; do not rebuild unrelated art or install new tools without a demonstrated gap.

**Overlap resolution:** This change implements the character-first walk/stop and
Shift/L3 sprint slice once, preserving the numerical sprint contract in
`improve-contextual-feedback-and-sprint`. That change retains contextual HUD
and its wider sprint acceptance; its tasks remain unchecked until their own
specified checks pass. `polish-locomotion-view-distance-and-time-hud` retains
directional turn/slope, distant view and time card, with no streaming/time-card
dependency on this slice. `refine-equipment-preview-and-idle` retains the
breathing idle and transparent portrait/slot UI. `refine-playable-heroine-hairstyles`
retains the rejected long-wave-end and all-body appearance acceptance; its
previously completed Bob work is preserved. See
`docs/research/character-assets/heroine-replacement-trial.md` for the evidence
and explicit unchecked-task mapping. No overlapping task is marked done from
the new plan alone.

## Risks / Trade-offs

- **[Free asset cannot legally or practically be used]** -> Require a per-item license and download/access receipt before importing; fall back to substantive re-authoring of the admitted CC0 source, not an unlicensed import.
- **[Better face breaks clothing/hair or saved ownership]** -> Trial every current body/hair/garment identity on an isolated path, retain existing IDs, validate save/reload, and reject unsupported combinations.
- **[A beautiful static face moves badly]** -> Review chronological walk, sprint, turns and work at normal camera before promotion.
- **[Sprinting drains Energy during pause or action]** -> Keep one simulation-owned exertion rule and explicit cancel/low-Energy checks; do not infer cost from animation frames.
- **[Hover seems like equipping or using]** -> Hover changes presentation only; pointer click remains the existing selection action and world use stays on its own input.
- **[Icons obscure missing ingredients or accessibility]** -> Use numeric counts, unique silhouettes, shape cues and full focused descriptions, with the pre-hatchet Fiber source prominent on demand.
- **[The large visual goal delays a playable result]** -> Gate one demonstrably better in-game candidate first, preserve `inventory-drop-v18`, and defer unsupported close-up/face-rig ambitions rather than call an import complete.

## Migration Plan

No existing preview or personal save is overwritten. Any actual equipment/skeleton/save schema change creates a current-version test profile with explicit reset disclosure; style/asset replacements cannot silently remap owned garment IDs. Promoted candidate metadata pins exact source, package, license provenance, gameplay visuals and prior selection. If a new character fails normal-play review, keep the current heroine in the selected candidate and return to a bounded candidate or CC0 re-authoring path.
