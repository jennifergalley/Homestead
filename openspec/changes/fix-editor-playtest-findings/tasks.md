# Tasks

## Current state and next visible goal

Recorded from the 2026-09-25 editor MCP playtest; nothing implemented yet. Next visible goal: the
Settings list steps one row per D-pad press. Reproduce every bug with the editor MCP play toolset
(`.github/skills/unreal-editor-mcp/SKILL.md`) before changing code, and verify with the same route.

## 1. Settings row skipping

- [ ] 1.1 Make Settings navigation use one logical column, matching its single drawn column, and confirm page 6's two-column use is intentional before touching it; verify through the MCP play toolset that one D-pad Down from Save focuses Load latest save and that repeated Down visits every Settings row in order.
- [ ] 1.2 Verify the change doesn't regress other pages: rerun the native menu and directional-navigation automation tests, and capture Settings stepping at 1280x720 and 3840x2160 in the packaged build.

## 2. Menu feedback reflow

- [ ] 2.1 Show field-book feedback in reserved space or an overlay instead of a collapsing row; verify by eating one berry from the Inventory in PIE that grid, details, action buttons and portrait keep their positions in before/after/expired captures.
- [ ] 2.2 Verify the banner remains fully readable and doesn't cover tabs, content or dialogs at 1280x720 and 3840x2160 with a long error message (for example a failed craft).

## 3. Competing directional lights

- [ ] 3.1 Give the sun and moonlight explicit forward-shading priority with a deterministic day/night owner; verify that a PIE session started at dawn shows no competing-light warning on screen or in the log.
- [ ] 3.2 Verify the dusk/dawn crossover by advancing through at least one transition in play and capturing before/after frames with no lighting pop and no warning.

## 4. Felled trees vanish

- [ ] 4.1 Keep the fell transaction and collision removal immediate, but hide the tree visual at the chop impact and present a short topple or stump using existing meshes; verify with PIE captures of a real RT fell that the trunk is present at impact and then visibly falls or leaves a stump.
- [ ] 4.2 Verify persistence and blocking: after felling, the player can walk through the former trunk position immediately, and after save and reload the tree does not reappear and any stump matches the cleared state.

## 5. Flat creek water

- [ ] 5.1 Diagnose the flat blue creek: capture the creek in a packaged Shipping build and in PIE after shader compilation finishes, and record whether it's a fallback-material artifact or the real water look.
- [ ] 5.2 If it's real, adjust the existing creek water material/presentation for depth tint and light response within the creek presentation rules; verify with daylight captures along the creek and an unchanged watering-can refill. If 5.1 shows an artifact only, fix readiness gating or record it as not reproducible in Shipping.

## 6. Smooth sun shadows

- [x] 6.1 Stop the visible shadow steps without VSM re-render stalls. Verify frame pacing on the 4K presentation route. (2026-09-25: the sun/moon use hardware ray-traced shadows and rotate every refresh, via `homestead.RayTracedSun`, default 1. The VSM fallback keeps the 0.5° steps. 4K 60-cap p99 is 18.2 ms vs 20.7 ms stepped; uncapped 85 vs 78 FPS. See `docs/research/rendering-baseline/README.md`.)
- [ ] 6.2 Jenny compares the ray-traced look (softer shadows, less low sun on inner leaves and hair) with `homestead.RayTracedSun 0` at dawn, midday and dusk, and accepts it or asks for tuning. Record her verdict here.

## 7. Integrated acceptance

- [ ] 7.1 Run one ordinary packaged playtest covering Settings navigation, eating from the Inventory, felling a tree, viewing the creek and watching shadows move through dawn; verify each fixed behavior in captures and record the evidence and any remaining limits in this change.
