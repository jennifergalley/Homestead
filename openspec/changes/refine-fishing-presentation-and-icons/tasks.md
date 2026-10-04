# Tasks

## 1. Fishing presentation
- [x] 1.1 Author and integrate the clip, tackle, bite cue and icon glyphs; publish the timing contract (commits 256ce7e5, dab028ac, e32d4d15, 7345bae1, 871d7055, 630cb0fe).
- [ ] 1.2 Jenny checks the integrated build: cast at water, then hook and land a fish (cast, float bob, strike and lift all visible); the pole and fish icons read clearly in the hotbar and pack.

## Handoff (Fishing art agent, Oct 4 afternoon/evening batch)
- Session `ac7339b4-84f7-49c0-8ec2-3d51d2b86730`, branch `jennifergalley-fishing-art-agent`, worktree `jennifergalley-bookish-pancake`. Pushed; not on main. Gameplay is merging it into its 9 PM candidate.
- Model: Claude Opus 5.5, high reasoning, default context (user-directed tier) for the whole batch. The usage export and registry/build ID come from Integration; anything not recorded stays unknown.
- Done:
  - Fishing clip, tackle, bite cue and glyphs (task 1.1).
  - 4 PM seed-packet gate on Gameplay 4f8d8f9d: PASS. All 7 crops show distinct non-stacking packets in the hotbar and pack.
  - Fence post body restored (`3e994653`; the recipe's mortise boolean had collapsed it).
  - Fish glyphs tilted 22° nose-up at 1.12x so they fill the slot like the tools and crops; salmon spots are dots (`69115cdc`, UBT clean). Gameplay candidate `e735bcb4` has everything through `6e2c6334`; asked it to merge `69115cdc`.
- Next: PIE-gate Gameplay's 9 PM candidate once it contains `69115cdc` and the lane editor slot is free (after the 4 PM smoke). Check cast/bite/strike/catch/miss/cancel and the pole plus six fish glyphs, then send `[ready]`.
- Re-author the clip with `Content/Python/homestead_agent/fish_cast.py` (MetaHuman Control Rig, 30 fps). The phase constants live in `HomesteadFishingPresentation.h`; change both together.
- Evidence and scratch are in `E:\CopilotScratch\ac7339b4-...\` (gate4f8_*.png, fence_pie.png). No editor is running.
