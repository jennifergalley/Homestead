# Tasks

## 1. Simulation

- [x] 1.1 `ShopClosedDay` (Sunday), `IsShopDay`, and `IsShopOpen` closed all of the calendar day. `NextShopOpening`/`HoursUntilOpen` skip it. `ClosedMessage(shop, hour)` and `ClosedSignText` name the day it reopens.
- [x] 1.2 `CanWaitForShop` allows only the ordinary night's closure, so `WaitForShop` refuses through Sunday with "The general store is closed on Sundays. It opens Monday at 8 AM." and passes no time.
- [x] 1.3 The travel summary warns when she'd arrive on a Sunday (`storeClosedAllDay`) and names Monday after a Saturday evening.
- [x] 1.4 Native `ShopsCloseOnSundays` covers:
  - Sunday closed at every quarter hour from 06:00 to 06:00;
  - Saturday 17:59 open and 18:00 shut;
  - Monday 07:59 shut and 08:00 open;
  - the message and sign texts;
  - waits allowed and refused, with the refusal text and no state change;
  - an old save loading unchanged;
  - travel warnings from Saturday 1 AM, Sunday noon, Monday morning and Saturday noon.

## 2. Unreal

- [x] 2.1 `HomesteadShopFlow.cpp`: the closed messages carry the hour, the door offers no wait key across Sunday and E names Monday, and the board's text follows `ClosedSignText` (`AHomesteadGeneralStore::SetClosedText`, only on a change). The shopkeeper is off duty, so hidden, whenever the shop is shut. (Source; uncompiled until the next Unreal slot.)
- [x] 2.2 The Native menu test's shop step skips a Sunday.
- [ ] 2.3 UBT and PIE in the next Unreal slot: the store door at Sunday 10:00 and Saturday 19:00 (focus line, board, no shopkeeper, E refusal), Monday 08:00 open, and the walk-to-town summary from a Saturday night.
- [ ] 2.4 UI gallery (Menu's lane): a `focus-door-sunday` entry, and shop entries pinned to a trading day.
