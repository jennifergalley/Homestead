# Balance Agent handoff

Standing role (approved by Jenny 2026-10-04, proposed by Alex): reviews planned and finished
features for balance and cohesion. Docs-only; holds no implementer slot.

| Field | Value |
| --- | --- |
| Session | ceb1109b-f156-451c-b890-ca91fa92f1c1 ("Balance Agent", from 2026-10-10; replaces archived 81a547cd) |
| Branch / worktree | `jennifergalley-balance-agent-2c8` / `E:\Repos\copilot-worktrees\SurvivalGame\jennifergalley-supreme-broccoli` |
| Model / configuration | Claude Opus 5.5, reasoning high, default context (as launched) |
| Owned docs | `docs/design/balance.md`, `docs/design/cohesion.md`, this file |

## Done

- 2026-10-04: balance sheet and cohesion guide (with UI polish audit) written from `main` `b77c5c38`.
- 2026-10-04: fishing (9 PM) review sent to Gameplay UI and Fishing Art, cc Orchestrator. Numbers
  and pole price approved; blocking copy fixes adopted by Gameplay UI along with a 0.7 s strike
  window.

- 2026-10-04: fishing final balance OK (numbers, copy, visuals) on `9958a359` / Art `abead265`.
  Non-blocking polish sent: one catch toast "Caught a lake carp.", cue panel off the heroine,
  larger fish glyphs, shorter refusal/escape lines.

- 2026-10-10: map-shrink stage 2 budget sent to the Map Agent (4ff1041f) ahead of its consult:
  cove and mine 35–50 s (cap 60 s), manor core/pond/village unchanged, estate about 35–50 ha,
  neighbour and communal sizing (`balance.md` §9).
- 2026-10-10: Map Agent layout draft reviewed (cove 50 s, mine 38 s, estate 36 ha). OK on travel,
  with 3 blocking items: keep tool-head salvage and about 1,500 coins of clearing on her land; no
  path running into the invisible edge; rename "Wheal Woods".
- 2026-10-10: revised layout Balance OK. All clearing stays on her land (about 4,900 coins); the
  road ends at a gate and milestone ("Truro 14 miles"); Carn Wood; forage topped up to about 65;
  cliff landings with rails. Still due: in-game sprint timings and a cliff-step check before the
  Map Agent's [ready].
- 2026-10-10: in-game evidence Balance OK (cove 58.2 s down / 58.9 s up, mine 33.4 s; landings,
  rails, mine on her land). Asked for grass/heath and flowers on the bare cliff face and steps cut
  before [ready] if cheap; otherwise card it as "[Balance] Dress the cove cliff and cut".
- 2026-10-10: Names Agent (72b7d774) rename table OK with small changes (Gull Sands, "my dear",
  en dashes, pasty/Cornish sweep). After Jenny picks the names, rewrite `cohesion.md` §1–2 to
  neutral rural English (estate, clerk, store, neighbours, places) and update `balance.md` names.
- 2026-10-10: heath fix in (`babfe357e`): the cut reads green and 73 flower clumps dress the cove
  slopes. The steepest upper bay wall stays smeared bare earth (planar UVs), carded as
  "[Balance] Rock face for the cove cliff". Map Agent shipped [ready]. Nothing open.

## Open

- My first 12 suggestions (halve crop growth, daily fishing catch, sleep 15/h, better-value
  snacks, cooked meals grant Well fed, UI audit items) are on Jenny's backlog as "[Balance]" /
  "[Balance UI]" cards, added by the Orchestrator 2026-10-04.

## Backlog suggestions rule (Jenny, 2026-10-04)

Every suggestion I make (balance, cohesion/UI, polish) becomes a backlog card tagged with its
source. End each review by sending the Orchestrator a "Backlog suggestions" list: one line per
card with a title, a one-sentence description and the doc section it came from. The Orchestrator
adds them through the planner; I never edit the backlog files myself. Before suggesting a card be
retired, check its current text in `docs/handoff/backlog-inbox.json` / `priority.json`: Jenny may
have rewritten it (she turned "sleep 15/h" into "Sleep always resets the energy bar entirely").

## How lanes reach me

Order for new assets and animations (Jenny, 2026-10-04): the lane sends me review media
(screenshots plus MP4) and I give a cohesion OK or blocking changes. The lane then sends the batch
to Jenny, and she approves it before [ready]. My OK comes first; it doesn't replace hers.

Send the short proposal or final numbers/copy (plus a screenshot path for visible work) with
`send_session_message`, delivery mode immediate. I answer with approve / approve-with-numbers or
"balance OK" / blocking changes within one turn.
