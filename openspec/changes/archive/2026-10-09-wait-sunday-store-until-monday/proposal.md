# Proposal

## Why

Jenny's feedback `jenny-mus1kfmm-ayz6jx` asks to wait at the General Store on Sunday until Monday opening and then enter.

## What Changes

- Offer a Sunday wait naming Monday morning opening.
- Advance calendar, seasons, crops, energy/rest and ordinary world consequences through existing time logic.
- Preserve normal closed-night waiting and confirmation/cancel controls; open the door after waiting.
- First playable result: Sunday door interaction waits until Monday opening and permits entry. Jenny owns integrated acceptance; shop inventory/economy changes belong to Farming Fishing.

## Capabilities

### New Capabilities

- `closed-shop-wait`: authoritative waiting for the General Store to open.

### Modified Capabilities

## Impact

Reuse `NextShopOpening`, `WaitForShop`, `AdvanceGameHours`, doze/rest behavior and `HomesteadShopFlow`'s two-press confirmation. Remove only the policy restriction preventing a Sunday daytime wait and handle rough sleep without overshooting opening. No assets, external dependencies or save-format changes.
