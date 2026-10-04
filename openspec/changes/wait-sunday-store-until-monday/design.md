# Design

## Context

`NextShopOpening` already skips Sunday. `CanWaitForShop` refuses intervals longer than the ordinary night and `WaitForShop` refuses any trial doze. The controller already has a two-press confirmation with cancellation.

## Goals / Non-Goals

Goals: reuse calendar/time advancement and existing confirmation, finish at opening despite normal rough sleep, enable the existing store entry path.
Non-goals: free energy grants, skipped simulation consequences, trade/catalogue changes, new save schema or live-save access.

## Decisions

- Permit the selected Sunday interval through `CanWaitForShop`; retain ordinary overnight behavior and finite/calendar/reach validation.
- Advance on a trial copy with existing authoritative stepping. Account for its rough-sleep behavior rather than setting `hour` or energy directly; ensure dozing cannot silently overshoot the agreed opening.
- Name Monday in the focus/confirmation text, refresh the existing open-door presentation after success and preserve normal entry movement.
- Travel Rest owns wait functions and controller wait presentation. Farming Fishing owns non-wait ShopGoods/trade edits in the same file. Integration coordinates compile; Jenny performs in-game acceptance.

## Risks / Trade-offs

- Automatic rough sleep adds time -> use bounded existing time/rest stepping, with native low-energy and day/season boundary checks.
- Actor state refresh lags -> explicitly refresh door state after a successful wait without teleporting the heroine.
