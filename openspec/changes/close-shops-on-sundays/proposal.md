# Close the shops on Sundays

## Why

Jenny, on 2026-09-30: "Shops on Sundays - I think they should be closed." In 1851 Cornwall the shops kept the Sabbath. The estate calendar already names the weekdays (Monday, Spring 1, 1851), but the shops kept the same hours every day.

## What Changes

- Every shop is closed all of Sunday, the calendar day from 06:00 Sunday to 06:00 Monday. Trading, the shop screen, the shopkeeper (off duty, so hidden) and the counter focus all follow `IsShopOpen`.
- The refusals name the day it reopens:
  - "Closed today (Sunday) - opens Monday at 8 AM" on a Sunday;
  - "Closed - opens Monday at 8 AM" on a Saturday evening;
  - "Closed - opens at 8 AM" on other nights, as before.
- The board on the shut door says "CLOSED / on Sundays", "CLOSED / opens Mon 8 AM" or "CLOSED / opens at 8 AM". It's updated whenever the words change.
- She can wait at the door only through an ordinary night's closure (closing to opening, 14 h), never through Sunday. In that case the door offers no wait key, and E says "The general store is closed on Sundays. It opens Monday at 8 AM."
- The walk-to-town summary warns when she'd arrive on a Sunday, during what would be the store's hours: "You'd arrive on a Sunday, when the general store is closed all day (it opens Monday at 8 AM)." When she'd arrive after Saturday's closing, it names Monday as the day it opens.
- Nothing is saved: the rule comes from the clock alone, so old saves load unchanged.

## Impact

- Simulation: `HomesteadShops` (`ShopClosedDay`, `IsShopDay`, `NextShopOpening`, `CanWaitForShop`, `ClosedMessage(shop, hour)`, `ClosedSignText`) and `HomesteadTravel` (`TravelPlan::storeClosedAllDay`).
- Unreal: `HomesteadShopFlow.cpp` (messages, door prompt, sign text), `AHomesteadGeneralStore::SetClosedText`, and a Sunday skip in the Native menu test's shop step.
- The UI gallery (Menu's lane) needs a Sunday-closed entry, and its shop entries need a trading day.
