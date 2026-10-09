# Tasks

## 1. Fishing presentation
- [x] 1.1 Author and integrate the clip, tackle, bite cue and icon glyphs; publish the timing contract (commits 256ce7e5, dab028ac, e32d4d15, 7345bae1, 871d7055, 630cb0fe).
- [ ] 1.2 Jenny checks the integrated build: cast at water, then hook and land a fish (cast, float bob, strike and lift all visible); the pole and fish icons read clearly in the hotbar and pack.

## Handoff (Fishing art agent, Oct 4 afternoon/evening batch)
- Session `ac7339b4-84f7-49c0-8ec2-3d51d2b86730`, branch `jennifergalley-fishing-art-agent`, worktree `jennifergalley-bookish-pancake`. Pushed; not on main. Delivered through Gameplay's 9 PM candidate `9958a359`.
- Model: Claude Opus 5.5, high reasoning, default context (user-directed tier) for the whole batch. The usage export and registry/build ID come from Integration; anything not recorded stays unknown.
- Done:
  - Fishing clip, tackle, bite cue and glyphs (task 1.1).
  - 4 PM seed-packet gate on Gameplay 4f8d8f9d: PASS. All 7 crops show distinct non-stacking packets in the hotbar and pack.
  - Fence post body restored (`3e994653`; the recipe's mortise boolean had collapsed it).
  - Fish glyphs tilted 22° nose-up at 1.12x so they fill the slot like the tools and crops; salmon spots are dots (`69115cdc`).
  - Hitch no longer loses the catch: strike overshoot carries into the catch segment (`433759a8`). The cue keeps fixed colours in every theme, with a gilt closing ring and pips (`d4e8c0fb`).
  - 9 PM PIE gate on Gameplay `9958a359` (merged here): PASS. Clean catch; worst-case hitch (three 0.27 s stalls from strike2 +0.36 s) still lands; a small move (~17 cm) during strike or catch before the 6.4 s lift pays nothing ("The fish escaped the hook"), while one after the lift pays; immediate recast after a cancel catches normally; the gilt ring reads at "Strike!"; pole plus six species glyphs read in the hotbar and pack.
- Limits: the cancel timing was driven over MCP, so it landed at about clip 5.8–6.2 s rather than exactly 6.30 s (Gameplay native tests cover 6.30 s). PIE only, not Shipping.
- Next: nothing for this lane until Jenny's playtest (task 1.2).
- Re-author the clip with `Content/Python/homestead_agent/fish_cast.py` (MetaHuman Control Rig, 30 fps). The phase constants live in `HomesteadFishingPresentation.h`; change both together.
- Evidence and scratch are in `E:\CopilotScratch\ac7339b4-...\` (sheet2.png ring/cue, c_05.png cast, c_10.png catch toast, glyph_row.png, glyph_pack.png). No editor is running.
