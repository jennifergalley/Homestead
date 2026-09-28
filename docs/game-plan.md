# Homestead: a cozy Cornish estate life sim

Homestead is a single-player, offline life sim for Jenny, in the tradition of Coral Island
and inspired by *Poldark*. It's set on the Cornish coast of the early Victorian era, around
1851. The daughter of a noble family comes home to her family's derelict, overgrown estate.
She clears it, farms, ranches, fishes and reopens the family mine. She hauls her goods to
town and sells them, and with the money she rebuilds and decorates the manor, hires help, and
restores the family's name. The goal is a fortune and standing, not survival. Courtship and
family come later.

The authoritative design (setting, world, loops, economy, tools and tiers, round order, and
the disposition of earlier work) is
`openspec\changes\pivot-to-cozy-estate-life-sim\design.md`, with product rules in its
`specs\estate-life-sim-direction\spec.md`. Each round is its own OpenSpec change. The
survival-era plan it replaced (2026-09-27) is kept for history in
`docs\archive\survival-prototype-game-plan.md`. Don't treat its requirements as current.

## Working policy: playable iteration first

Jenny's instructions of 2026-09-20 govern current development and supersede
older planning constraints that conflict with this section.

- **Playtesting, not a protected playthrough.** Until Jenny says otherwise,
  existing game saves are disposable test data. Prefer the latest playable
  version over preserving or migrating old test progress. Reset incompatible
  test saves when needed and report the reset; do not delete unrelated data.
  Reliable saving remains a game feature, but legacy test-save migration,
  profile snapshots and recovery infrastructure are not release prerequisites.
- **Completion-driven autonomy.** Continue around the clock through agreed
  scope and plans, working toward the quality and feature completeness of an
  indie game on Steam. Cost and credit use are not constraints for this project.
  Do not impose arbitrary time, spending or iteration caps. Honor explicit
  stops and actual failures; do not confuse a quiet long-running build with
  a failure or repeat a failed approach without diagnosis.
- **Research reuse before planning.** First check the project, existing research,
  Unreal facilities and available free assets/libraries for work we can reuse.
  Research external options only where needed. Verify the actual license,
  attribution/distribution terms, source, access requirements, compatibility and
  integration effort; "free to download" alone is not enough. Prefer suitable
  existing work over hand-authoring its equivalent. Plan custom work only for
  the concrete gaps, and record why reuse does not meet those needs. Keep this
  research actionable rather than an exhaustive survey or repeated review of
  unchanged admitted assets.
- **OpenSpec before each improvement round.** After reuse research, orient the
  proposal and design around the selected reusable pieces and necessary custom
  gaps. Keep proposal, design and tasks
  current, including changes to scope or acceptance. Plan enough to implement
  coherently, not to eliminate all future debugging or refactoring.
- **Deliver playable features, not an infrastructure project.** Prefer direct
  integration and frequent playtests to elaborate authoring, testing or proof
  systems. Build, exercise the changed behavior, inspect actual visuals where
  relevant, and fix blocking defects. Modest disclosed bugs and later refactors
  are acceptable. Essential security, licensing and honest error reporting
  remain required; do not claim untested behavior or failed checks as passing.
- **Make the latest usable build easy to playtest.** Update the normal preview
  selection after appropriate build and gameplay checks, with concise notes
  about changes, known issues and any save reset. Do not require another
  approval solely to preserve obsolete test progress. Keep cheap code/build
  rollback where useful, without turning compatibility into the main task.
- **Only the integration session packages in multi-session rounds.** Lanes implement,
  verify in the editor, run native tests, compile-check, commit and push, then
  notify the orchestrator, which forwards the work to the integration session.
  That session merges, packages once, runs packaged tests and reports back;
  the orchestrator only coordinates and relays to Jenny. The Interactive Loop's
  "package, commit, push" applies to the integration session (or a lone
  session). The procedure is in `docs\handoff\README.md` ("Delivering lane work").
- **Parallelize independent work.** Use isolated sub-sessions/worktrees whenever
  concrete file and artifact ownership makes later integration practical.
  Agree shared interfaces early, coordinate merges, and serialize only genuinely
  shared resources such as engine authoring, packaging and final integration.
  Do not limit useful parallel work merely to save credits or agent count.

Delivery now follows the round order of the cozy-estate pivot (below). Independent
implementation and asset preparation may proceed in parallel branches, integrated in round
order. Jenny will give further direction as she playtests. Don't invent unrelated features
simply to keep agents occupied.

These preferences do not authorize purchases, account/security/firewall changes,
private-reference uploads, or interference with unrelated applications.
Current progress and the latest usable launch path are recorded in
`docs\development-status.md` and `Preview.cmd`; the prototype snapshot below
is historical, not the current build/authorization status.

### Apply these lessons to every feature round

1. Start with focused reuse research, consulting existing findings before
   searching again. For player-facing design, include the applicable primary
   comparative references above and identify the specific convention or hierarchy
   being adapted without copying its branded expression. Select suitable free
   assets, code, libraries and native engine features; identify only the gaps
   requiring custom work. Then define
   the smallest observable end-to-end improvement in OpenSpec, its first
   playable demonstration and each parallel lane's file/resource ownership.
   Separate that first delivery from full-round acceptance.
2. Integrate into the existing game early. Do not make a complete asset palette,
   isolated showcase or generalized authoring framework a prerequisite for
   demonstrating one useful change in the real clearing or menu.
3. Reuse supported engine workflows, working helpers and compatible caches.
   Do not repeatedly clear shader caches, rebuild unaffected code or reopen
   unchanged tool-admission decisions. Resolve genuine new permission boundaries
   explicitly, without treating ordinary implementation bugs as permission issues.
4. Make infrastructure changes only for a reproduced blocker on the path to the
   feature. Choose the smallest supported fix, verify the actual production
   invocation, then return to feature integration. Do not build speculative
   future-proofing or custom substitutes for existing engine facilities.
5. Verify the changed behavior with the smallest relevant build/test route and
   real visual/play evidence. Broaden regression coverage when risk or a failure
   justifies it, not automatically before every increment. Preserve useful raw
   diagnostic images/logs even when acceptance fails; never label them a pass.
   A requested screenshot means the change integrated into the actual game,
   captured during our ordinary playtest, as with the earlier animation work.
   Isolated asset renders and test scenes are internal diagnostics, not that
   deliverable. Capture and share in-game views when naturally available; do
   not derail integration to manufacture a screenshot. Until then, report the
   remaining integration/playtest steps and any timing uncertainty honestly.
6. Log meaningful phases before expensive operations, keep reusable work after
   failures, and diagnose before retrying. Neither quiet logs nor CPU activity
   alone prove failure or useful progress. Do not kill healthy work solely
   because an arbitrary estimate expired.
7. Merge coherent parallel increments frequently. Coordinate shared interfaces
   early and serialize only overlapping files, outputs or genuinely shared
   execution resources, rather than whole independent workstreams.
8. Report what changed in the playable game, what Jenny can try, and known
   limitations. Keep the active task view concise, with history linked elsewhere.
   Imported assets, passing tools and screenshots are supporting milestones,
   not substitutes for integration, and task counts must remain honest.

## Direction at a glance (2026-09-27 interview)

- **World:** one fixed, hand-authored map of about 4 × 4 km, built from real Environment
  Agency LIDAR of the St Agnes coast and reshaped:
  - The estate, about 1 km across, runs down a south-facing slope to its own cove, with a
    clifftop mine, a river valley and a mill site.
  - A dirt road runs about 1.5 km to an estuary-head harbour town.
  - Moorland and tors lie to the north, and dunes and beaches along the far coast.
  - The estate boundary is shown on a minimap and world map. There are no pre-built fences.
- **Start:** Spring day 1, in the one standing room of a ruined manor, on an estate buried in
  bramble, weeds, stumps and rubble.
- **Meters:** energy and hunger only, both gentle. There's no cold, death or spoilage. Food
  refills hunger and energy.
- **Calendar:** days of about 30 real minutes, from 6 AM to 2 AM, and four 28-day seasons.
  It's sunny with occasional rain.
- **Tools:** axe, hoe, pail or watering can, scythe, billhook and pickaxe, then a fishing
  pole. The first tools are hafted from salvaged heads, and a blacksmith takes them through
  Coral Island-style tiers that gate what each can clear.
- **Loops:**
  - Clearing, and farming (period crops).
  - Gentle ranching with no slaughter.
  - Fishing, then boat fishing.
  - Mining: a fixed maze, lantern and chalk, and the Coral Island ore and gem ladders.
  - Artisan processing and the water mill.
  - Hauling by hand, then handcart, then horse and wagon, to town shops with supply-sensitive
    prices.
- **Money:** US dollars and cents. Sinks include rates and taxes, wages, upkeep, materials,
  land and upgrades. She never loses the estate.
- **Manor:** a freeform wall-by-wall rebuild on the old footprint, and interior décor.
- **Later:** hired workers, family reputation, town events, land purchase, a dog companion
  (pit bull or Great Dane), and courtship.

## Round order

| # | Round | OpenSpec change(s) |
| --- | --- | --- |
| 1 | Walk your estate | `author-fixed-cornish-estate-map`, `add-estate-boundary-map-and-minimap`, `add-overgrown-estate-clearing`, `add-ruined-manor-and-arrival`, `add-dollars-and-general-store` |
| 2 | Farming calendar, seed shop and period crafting | `rework-farming-calendar-and-period-crafting` |
| 3 | Handcart, more shops and dynamic prices | `add-handcart-hauling-and-dynamic-prices` |
| 4 | Shore and river fishing | `add-shore-and-river-fishing` |
| 5 | Manor rebuild, décor and the dog | `rebuild-manor-and-decorate` |
| 6 | Ranching | `add-ranching-and-livestock` |
| 7 | Mine: shallow level | `open-estate-mine-shallow-level` |
| 8 | Horse and wagon | `add-horse-and-wagon` |
| 9 | Water mill and artisan goods | `add-watermill-and-artisan-goods` |
| 10 | Workers, wages and upkeep | `add-hired-workers-and-estate-upkeep` |
| 11 | Mine pumping and deep levels | `restore-mine-pumping-and-deep-levels` |
| 12 | Reputation, events and land | `add-family-reputation-and-land-purchase` |
| 13 | Boat fishing | `add-boat-fishing` |
| 14 | Courtship and family | `add-courtship-and-family` |

Round 1 is fully specified. Rounds 2–14 are proposal-only, and each is fleshed out with fresh
reuse research and Jenny's latest playtest feedback when it's reached. The order may change
as she playtests.

## Comparative references

- **Coral Island** is the primary reference for the loop, the economy, tool tiers, the town
  and shops, ranching and mining.
- **Stardew Valley** is a reference for mine levels, artisan goods, no-slaughter livestock
  and the opening farm clutter.
- **Minecraft** and **Disney Dreamlight Valley** remain interaction, inventory and daily-
  tending references.
- **Poldark** sets the tone for the setting, the mine as fortune, and a family's name
  restored.

Before designing a HUD, menu, tool flow, shop, decoration system or daily rhythm, inspect
reputable screenshots and official or well-supported descriptions of the applicable games,
and record which convention is being adapted. Adapt principles, not expression: Homestead
uses original artwork, icons, typography, animation, copy and sounds. Link or cite external
reference pages, and don't commit copyrighted screenshots or extracted assets. References
never override Jenny's playtest feedback, verisimilitude, accessibility, performance or
licensing.

Photorealistic characters at MetaHuman quality remain the bar for the heroine and
townsfolk.

## Technical approach and guardrails

### Target machine and initial tooling inspection

- Windows PC: AMD Ryzen 7 3700X, 8 cores / 16 logical processors.
- NVIDIA GeForce RTX 5080, approximately 16 GB VRAM; 32 GB system RAM.
- Current display resolution: 3840 x 2160. Target 60 FPS; internal rendering
  resolution, upscaling, and quality settings can be adjusted after profiling.
  Display resolution is not a promise of native-4K rendering performance.
- E: has approximately 1,408 GB free; C: approximately 81 GB free at inspection.
  Prefer E: for engine, content, and large build/shader caches where configurable.
- Initial inspection found no game engine and an incomplete old VS Preview.
  Visual Studio Build Tools 2022 17.14.41 and Windows SDK 10.0.26100.0 are now
  installed; the new toolchain is on E:\Tools\VSBuildTools.
- Epic Launcher is installed and signed in. Unreal 5.8.2 installation at
  E:\Program Files\UE_5.8 is complete, verified by Epic metadata and successful
  editor compilation, bootstrap, gameplay launch, and packaging.

### Engine and implementation decisions

Use the preferred Unreal route for a feasibility gate, not an unconditional
promise that its stock character tools satisfy the complete game.

- Select and pin a compatible, non-preview Unreal release and supported compiler/
  Windows SDK after verifying the release-specific requirements.
- Prove a representative rigged character, environment, controller, and packaged
  build before committing to substantial content production.
- Prototype runtime complete-character preset switching, color controls, hairstyles, and
  outfit swaps with animation and save/load. A creator that only works inside the
  development editor does not meet the in-game customization requirement.
- Treat MetaHuman as a candidate for authored presets, not a ready-made shipped
  face/body-slider system. Epic's documented workflow assembles rigged characters
  from editor-authored assets. Deep runtime shape customization needs a separate
  feasibility investigation before committing to a long-term character pipeline.
- If the candidate pipeline cannot support the intended progression to detailed
  runtime customization and compatible clothes, stop and present alternatives
  before producing a large wardrobe. Code abstraction alone does not make
  incompatible character art interchangeable.
- Guided setup may require Jenny to sign into Epic, acquire permitted assets, and
  perform creator/editor steps. Document which steps are manual; do not assume a
  headless tool can accept licenses or complete every asset-authoring operation.
- Use original or appropriately licensed assets; track origin and permitted use.
  Purchases and major downloads require Jenny's approval.
- Keep item, recipe, crop, price, clothing, and seasonal values data-driven, starting from
  the single item catalogue added in round 1.
- Use stable object identifiers for cleared resources, placed structures, crops,
  inventory, character appearance, and saved progression.
- Keep simulation state separate from presentation and input.
- Route controller and keyboard/mouse through shared gameplay actions.
- Save with a schema version and recoverable writes. Proposed defaults: manual
  save slots, three rotating autosaves, and a separately retained recovery
  checkpoint. Preserve prior saves on write failure and show a clear error.
  Resume/failure screens expose earlier checkpoints rather than trapping the
  player in the latest failing state. Do not introduce cloud-save dependencies.
- Profile frame times in a packaged build on the target PC, aiming for 60 FPS
  (approximately 16.7 ms per frame). Record average and low-percentile performance
  in the character/forest scene and again after the homestead systems are active.
  Do not claim success from editor FPS or an empty map.
- Introduce repository/build conventions after engine choice; do not scaffold
  several competing implementations.
- No gameplay accounts, services, online economy, or AI-generated NPC runtime.
  Epic/asset accounts may be needed during development; the packaged game must
  work offline as a single-player game.

### Proposed project organization

The single Unreal project lives in `E:\Repos\SurvivalGame`, and agents work in worktrees of it.
Prefer testable C++ gameplay/state code and limited Blueprint asset/presentation
wiring; avoid an opaque all-Blueprint implementation that is difficult to review
and maintain with text-based tools. Do not build a general-purpose game framework.

- SurvivalGame.uproject and Config: pinned engine/plugins, inputs, rendering,
  defaults, and packaging.
- Source\SurvivalGame: player/camera, interactions, inventory/crafting, building,
  plant foraging/renewal, farming, time/vitals, appearance selections, and persistence.
- Content\SurvivalGame: maps, character/animation setup, item/recipe data,
  construction pieces, environments, audio, and controller UI.
- Content\SurvivalGame\Audio\Music: approved imported score assets and playlist/
  playback configuration; keep source-audio provenance with the asset manifest.
- Content\ThirdParty: licensed content kept identifiable where package structure
  permits; do not break references merely to enforce a folder convention.
- Scripts: repeatable editor setup/build helpers where the engine supports them.
- docs: design, MVP, roadmap, technical setup, playtest feedback, and an asset
  source/license manifest. Reference art remains local, not in a public upload.

Use Unreal's input actions for both input methods, a shared simulation clock, and
versioned save records. Prefer engine-native collision, animation, UI, audio, and
serialization rather than introducing extra frameworks. Keep tuning in data
assets so feedback changes do not require redesigning system code.

### Documented engine constraints

Official Epic sources inspected during planning:

- https://dev.epicgames.com/documentation/en-us/metahuman/getting-started-with-metahuman-creator-in-unreal-engine
  MetaHuman Creator requires additional Core Data and a plugin; auto-rigging and
  texture synthesis use cloud services.
- https://dev.epicgames.com/documentation/en-us/metahuman/creating-a-metahuman-character-in-unreal-engine
  Character creation/assembly is an editor workflow. Once rigged, head/body and
  material editing in that workflow requires removing the rig; hair/clothing
  remain editable. This does not establish a ready-made runtime creator.
- https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine
  Compiler compatibility varies by engine release. Current documentation lists
  newer supported toolchains than the registered local VS 17.9 Preview for UE 5.8.
  Verify the chosen release rather than mixing requirements from different versions.

Asset-specific license checks, a free character/clothing shortlist, engine release
selection, and exact disk requirements remain outstanding. Search summaries alone
are not sufficient evidence for license or toolchain decisions.

## Documentation ownership

Detailed specifications and durable plans live in this repository, not Notion: this
document, the OpenSpec changes under `openspec\changes`, and the focused docs under
`docs`. Record bugs and deferred fixes from playtests as OpenSpec changes.
