# Crafting interaction

The Craft page uses the portable `Homestead::Simulation` as its only gameplay
authority. `AssessRecipe` is a pure view of the same costs, retained-tool,
cookfire, failure, and post-transaction capacity rules used by `Craft`; it does
not mutate state or advance the simulation revision.

Each core recipe tile contains only its icon. Unavailable recipes remain
selectable but render in grayscale. The details pane shows the output and
description, then separate status rows for every consumed ingredient, retained
tool, nearby cookfire, and pack capacity. Ingredient rows show exact carried
counts as `Have N / Need M`; chest contents do not satisfy a recipe. Current
source hints are:

| Ingredient | Primary source |
|---|---|
| Branch | Fallen branches |
| Stone | Loose stones |
| Fiber | Reeds near water |
| Roots | Wild roots |
| Meadow herb | Meadow herb patches |
| Timber | Mature trees with a carried Hatchet |

Pressing a recipe only selects it. Holding the pointer, Enter, Space, E, or
controller A fills the icon from bottom to top over 1.2 real-time seconds. A
complete cycle re-assesses and requests one authoritative craft. Continued
holds repeat complete cycles until a requirement fails; releasing, changing
focus/page, closing the menu, loading, or entering recovery cancels incomplete
progress without queuing output.

Each complete cycle schedules three restrained impacts from Kenney's CC0
Impact Sounds archive:

| Asset | Source member | SHA-256 |
|---|---|---|
| CraftStrikeA | `Audio/impactMetal_light_000.ogg` | `33B5E6E37C6E9D54E07BF5A89B12C76E879F40C1EA83CDD82714DF1D6F9FEC6D` |
| CraftStrikeB | `Audio/impactMetal_light_001.ogg` | `FBA69C467DDD85DA7B1D82E259F936B876D9BA6A3D6F1B24BDD10468E46BFE08` |
| CraftStrikeC | `Audio/impactMetal_light_002.ogg` | `25A96F90A9A1F88A531E824E126F0519504625E5635E65A72E4F31611428DB29` |

The sounds use the Effects volume path and produce no playback when Effects is
muted. License and download provenance remain in `Assets/asset-manifest.json`,
`Assets/download-receipt.json`, and `docs/asset-credits.md`.

Focused acceptance runs through `Scripts/Test-Game.ps1 -Crafting`. It covers
ready, active-progress, and blocked visuals; quick and partial presses;
keyboard, controller, and pointer holds; repeated exhaustion; and exact output
for all six core recipes. Use `-WithAudio` to capture the game-only master
submix for waveform validation.
