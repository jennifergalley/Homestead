# Design

## Context

See proposal and spec. Focus/toast cards already use parchment; pickup gains still float beside the heroine. Focus titles cap their measured width but draw unbounded text, and action rows never wrap. Resource success clears any outstanding notice.

## Goals / Non-Goals

**Goals:** one existing parchment treatment and one bounded top-centre stack for focus, explicit notices and existing pickup gains.

**Non-Goals:** new messages, tutorial copy, changed rewards, inputs, save data, notice durations or gain detection.

## Decisions

Reuse Canvas `NoticeCard`, `WrappedLines`, font measurements and the controller's `RecentPickups`. Render gains through the same HUD layout rather than maintaining an independently positioned Slate popup. Keep explicit notice text/error colour and five/eight-second clocks; retain gain aggregation and 2.6-second lifetime/fades. Routine success must not erase a still-live refusal, save or Travel unlock notice. Track quiet action success independently so hint learning continues without restarting book notices. Remove redundant one-second hotbar-name toasts: selection and audio remain, while existing notices keep their lifetimes.

Measure the available band left of the vitals, wrap titles/actions/gains, then stack notices beneath the focus card. Reuse the existing theme and gallery; no new infrastructure/assets.

## Lanes and ownership

HUD owns `HomesteadHUD*`, removal of the old pickup rendering widget, the notice-success clearing hunk in `HomesteadControllerFocus.cpp`, and its feature artifacts. Farming owns seed/fishing action text and retirement logic; preserve its `249229b0` checkpoint. Travel owns exact first-visit unlock copy. Fishing's bottom-centre card is separate from this top-centre stack.

## Risks / Trade-offs

Long labels and 720p font floors grow cards: measured wrapping and the shared available-width bound replace unsafe width-only caps. Unreal compilation and visual acceptance are explicitly held while Jenny plays; no ready claim before those checks.
