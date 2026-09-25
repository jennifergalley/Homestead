# UI and Fiber playtest baseline (2026-09-23)

Selected `inventory-drop-v18`: `Build\Releases\20260923-150721-drop\drop-02-shipping`, Shipping executable SHA-256 `326E4E5C41197468E5EC97AED2717F21F581F622D7134B94EE3A020FBBAD247F`. No selected binary or personal save was modified.

| Fresh isolated Shipping QA route | 1280x720 | 3840x2160 |
|---|---|---|
| Native Settings, Inventory, Guidebook, Craft, Appearance | `Saved\Automation\ui-fiber-selected-native-720` | `Saved\Automation\ui-fiber-selected-native-4k` |
| Ordinary mapped spawn-to-creek crossing, no teleport or supply | `Saved\Automation\ui-fiber-selected-creek-720` | `Saved\Automation\ui-fiber-selected-creek-4k` |

Each output contains its exact `qa-launch.json`, independent `EngineUser`, `SmokeSave`, `Graphics`, `smoke-result.txt` and native framebuffer captures. At 720p, Settings fills a 1280x720 fixed virtual canvas; at 4K, `SScaleBox(ScaleToFit)` grows the same UI roughly threefold. The Guidebook shows an inert **Read** button although its selected row already displays the text. Native menu mean/p95/p99 were 53.81 FPS/17.49/17.68 ms at 720p and 53.62/17.47/17.75 ms at 4K, excluding startup and screenshot readback; these paused-menu samples are **not a world-boundary hitch benchmark**. The selected Creek test passed input/collision checks at both resolutions, but its minimum-duration walking steps overshot the creek by more than a kilometer before taking the alleged bank pictures; those frames are **not a valid matched reed visual baseline**. The revised source route stops at the real bank and gathers on foot. Task 1.1 remains open for a genuine selected-build bank/pointer comparison; no success was inferred from misleading screenshot names.

The reported Settings pointer bug is explained by the selected source path: the global menu input preprocessor forwards mouse-down as `InputKey(LeftMouseButton)`, which `SHomesteadMenu::HandleKey` treats as Activate while Session/Resume may be focused. This happens before the clicked `SSlider` can own the drag. The player report is a real 4K repro; the selected binary has no automated slider click fixture, so this source trace is **not** a passing pointer interaction test.

The generator seeds reeds on the two banks around 120 cm from the creek center (`HomesteadWorldGeneration.cpp`). The player-facing resource clump currently draws five `SM_GrassMedium01_mid_b` meshes (`HomesteadWorld.cpp`), visually similar to the surrounding bank grass; one patch already awards five Fiber via authoritative `Harvest` without a hatchet. Normal creek screenshots at the crossing do not demonstrate a conspicuous ready reed clump or that a first-time player can find one. Match later comparisons to the same route, time, lighting, graphic policy and fresh test-world seed, and judge foliage at actual game camera distance.

Prior world-performance work (`upgrade-woodland-environment-assets` task 7.5) lowered measured chunk-boundary preparation from 1128.699 to 690.414 ms; tasks 8.1-8.4 for incremental publication remain unchecked. Jenny reports a recurring half-second wandering hitch. The prior measurement motivates a fresh selected-build breakdown, but does not attribute every current stall or prove a fix.

## Unselected Shipping candidate

`Build\Releases\20260923-ui-fiber-01\Windows\SurvivalGame.exe`, native SHA-256
`4C6C1F1D2F3A89D1C29752887FFEB2E01A5D033A05B3F7A34C741270EF68BA0D`,
was cooked/staged from the uncommitted integrated source into its own archive.
It does **not** replace `Preview.json` or carry a commit-backed preview receipt.
Shipping QA at both 720p and 4K passed exact pointer sliders,
informational entries, earned-tool positions, normal mapped riverbank reed
gathering (five Fiber, no immediate repeat), 24-hour deadline and current-save
reload. Full survival, native menu, generated producer and separate consumer
routes passed; the ordinary non-QA Shipping startup probe restored an isolated
version-7 wardrobe fixture and saved/loaded without touching selected or
player profiles.

Raw frames: `Saved\Automation\ui-fiber-shipping-native-720`,
`ui-fiber-shipping-native-4k`, `ui-fiber-shipping-creek-720-r2` and
`ui-fiber-shipping-creek-4k`. The 4K Slate book fills the viewport through a
2560x1440 logical layout while labels, icons and hit targets have bounded
physical sizes. The first attempt at shrinking the whole 1280x720 book into
the middle was rejected after Jenny clarified the design; that miniature
is **not** in this candidate.

Matched generated boundary routes at 720p reported selected `CHUNK_PREP`
789.200 ms, candidate repeat 768.421 ms and screenshot-free p95 17.47/17.49
ms. The first candidate run was 1289.310 ms under another load; all three
numbers are retained rather than calling the hitch solved. The old selected
creek route overshot the bank; task 1.1 stays unchecked.
