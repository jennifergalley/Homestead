# Balance Agent handoff

Standing role (approved by Jenny 2026-10-04, proposed by Alex): reviews planned and finished
features for balance (energy, coins, yields, timers, pacing, travel) and cohesion (design, flavor,
UI) against the pillars: cozy fun without grinding, then beauty (flowers everywhere), then
verisimilitude. Docs-only; never edits game code; holds no heavy-job slot.

**Start here (next Balance Agent):** read `docs/design/balance.md` and `docs/design/cohesion.md`,
then the "Open" list below. Lanes consult you at planning and before `[ready]`; answer within one
turn. Send docs `[ready]`s and backlog suggestions to the current Orchestrator (see the round page).

## Sessions

| Segment | Session | Branch / worktree | Model / configuration | Dates |
| --- | --- | --- | --- | --- |
| 1 | `81a547cd` (archived 2026-10-09) | `jennifergalley-balance-agent` / `...\jennifergalley-probable-funicular` | Claude Opus 5.5, high, default context | 2026-10-04 – 10-09 |
| 2 | `ceb1109b-f156-451c-b890-ca91fa92f1c1` (retired 2026-10-12) | `jennifergalley-balance-agent-2c8` / `E:\Repos\copilot-worktrees\SurvivalGame\jennifergalley-supreme-broccoli` | Claude Opus 5.5, high, default context (as launched; no model changes) | 2026-10-10 – 10-12 |

AI credits: segment 2's usage wasn't measured in-session. Integration's accounting export for the
round should attribute session `ceb1109b` to round 4 (map shrink stage 2, coast, beach, cave, mine,
polish) as review-only work. All work is docs commits; the last is the commit carrying this file.

## Owned docs

`docs/design/balance.md` (numbers sheet), `docs/design/cohesion.md` (flavor, copy, UI, beauty),
this file. Mine numbers also live in `openspec/changes/open-estate-mine-shallow-level/design.md` §7.

## Decisions this round (2026-10-10 – 10-12), all recorded in the docs

- **Map (§9 travel):** estate about 25 ha (Jenny's call). Farm 8–12 s, pond 44 s, mine 33 s. The
  cove is 80 s / 378 m on Jenny's pick of a mostly-slope path with two short stone flights (cap
  75 s). The village walk is about 90 s after Jenny's scheduled street-grade card. Neighbours
  Ashgrove, Brackenburn and Thornley; Hollin Wood is for sale; Gull Sands is the communal beach
  (about 600 m from the village, accepted); the road ends at "Marlbury 9 miles".
- **Names (cohesion §1–2):** no Cornish or Poldark names. Larkhollow, Mr. Callum Aldridge
  ("Aldridge's general store", sign "C. ALDRIDGE – PROVISIONS & SUNDRIES"), "Pasty", "the village".
- **Beach (§8a):** 40 finds (about 440 coins and 20 energy a day). Laver, not sea lettuce. Meals:
  steamed mussels 25, broth 35, fried laver 19. The campfire takes 3 driftwood and burns 4 h. Finds
  refill at the first 06:00 at least 12 h after picking.
- **Night:** 0.2 lux moonlight (Set A).
- **Mine (§8b, design §7):** 20 rock faces at 3 swings, 2.5 energy and about 6 stone each, back
  daily. 7 coal seams and 6 copper veins. No gem nodes: gems drop at 3% per face and 10% per vein,
  85/15 quartz/amethyst. Prices: coal 10, copper ore 25, quartz 35, amethyst 150. A 25-stone pile
  upstairs. Unlit tunnels at MineFill 0.1 (Jenny); mining needs a lit lamp within 4 m.
- **Manor (§8b):** 5 stages, 1,500 stone, 215 timber and 7,000 coins in total, about 12 game days.
  Jenny's D1: materials in estate chests plus a village mason; restored overnight.
- **Polish (§4–§6):** wild roots grow in 5 days; herbed roots 25 and roasted 19 energy; wild nodes
  give 1 seed and regrow in 72 h; "Wild root seeds". Meadow herbs halved (19 patches, still 10
  coins). Bare tilled plots revert at 20% per morning after 24 h bare.

## Open (for the next Balance Agent)

- **Mine house (Mine Art `029d7c8b`):** Balance OK for `[ready]`, on the condition that Jenny's
  review includes an RT-on or packaged exterior shot. If the walls still look olive there, card
  "[Balance] Lighter granite on the mine house" rather than hold the build.
- **Mine tunnels:** Mine Art sends the tunnel captures separately; they haven't been reviewed for
  cohesion yet. Check: timber sets, never pitch black at MineFill 0.1, a daylight pool at the ladder
  foot, no bare greybox.
- **Post-merge checks:** the store title reads "Aldridge's" (the beach shots still showed
  "Trethewey's"); the sea cave's 2 tideline nodes from the Beach branch sit at the cave mouth.
- **Manor restoration (`rebuild-manor-and-decorate`):** when it's planned, hold it to the §8b stage
  budget and Jenny's D1 (chests plus mason, overnight).
- **Cards sent this round** (the Orchestrator files them; check their current text in
  `docs/handoff/backlog-inbox.json` before suggesting retirement):
  - **Village and map:** village walk grade or time; bench viewpoint on the cove path; lit village
    windows at night; distances on fingerposts.
  - **Sea cave:** finish the rock face; clear pool water and a light shaft; planting and goblin's
    gold.
  - **Beach:** held laver and shell read better.
  - **Mine house:** yard dressing; homely props. The weathered-wood frames card was dropped
    (done).
  - **Older cards:** my first 12 suggestions (2026-10-04) are on the backlog as "[Balance]" /
    "[Balance UI]".

## Backlog suggestions rule (Jenny, 2026-10-04)

Every suggestion (balance, cohesion/UI, polish) becomes a backlog card tagged with its source. End
each review by sending the Orchestrator a "Backlog suggestions" list: one line per card with a
title, a one-sentence description and the doc section it came from. The Orchestrator adds them
through the planner; never edit the backlog files yourself. Withdraw a suggestion when the lane
adopts it before shipping.

## How lanes reach the Balance Agent

Order for new assets and animations (Jenny, 2026-10-04): the lane sends review media (screenshots
plus MP4) and gets a cohesion OK or blocking changes first. It then sends the batch to Jenny, who
approves it before `[ready]`. The Balance OK doesn't replace hers.

Lanes send the short proposal or final numbers/copy (plus a screenshot path for visible work) with
`send_session_message`, delivery mode immediate. Answer with approve / approve-with-numbers or
"balance OK" / blocking changes within one turn.

Lessons from this round:
- Check a timing rule against the whole waking day before setting it (the 20 h beach refill
  skipped a day for picks after 10:00).
- A playtested complaint from Jenny ("too far", "fine as is") outranks a Balance cap. When a fix
  trades against her stated preference, put the options to her through the Orchestrator rather than
  relaxing the cap yourself.
- Judge darkness and colour on the final surfaces and lighting (RT-on or packaged), not on greybox
  or RT-off captures.
