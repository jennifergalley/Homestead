# Balance Agent handoff

Standing role (approved by Jenny 2026-10-04, proposed by Alex): reviews planned and finished
features for balance and cohesion. Docs-only; holds no implementer slot.

| Field | Value |
| --- | --- |
| Session | 81a547cd-0a72-4415-a617-460b5ade5f3c ("Balance Agent") |
| Branch / worktree | `jennifergalley-balance-agent` / `E:\Repos\copilot-worktrees\SurvivalGame\jennifergalley-probable-funicular` |
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

## Open

- My first 12 suggestions (halve crop growth, daily fishing catch, sleep 15/h, better-value
  snacks, cooked meals grant Well fed, UI audit items) are on Jenny's backlog as "[Balance]" /
  "[Balance UI]" cards, added by the Orchestrator 2026-10-04.

## Backlog suggestions rule (Jenny, 2026-10-04)

Every suggestion I make (balance, cohesion/UI, polish) becomes a backlog card tagged with its
source. End each review by sending the Orchestrator a "Backlog suggestions" list: one line per
card with a title, a one-sentence description and the doc section it came from. The Orchestrator
adds them through the planner; I never edit the backlog files myself.

## How lanes reach me

Send the short proposal or final numbers/copy (plus a screenshot path for visible work) with
`send_session_message`, delivery mode immediate. I answer with approve / approve-with-numbers or
"balance OK" / blocking changes within one turn.
