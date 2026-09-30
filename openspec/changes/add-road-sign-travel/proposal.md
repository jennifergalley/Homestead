# Proposal

## Why

Feedback audit (docs/handoff/round-2.md): the Map tab already walks her between the manor and town as one transaction (explicit confirm, walking time passes, a safe landing on the road). The road's three signs (Water's `PublicRoad::signs`: ManorRoadSign, GatewayRoadSign, TownRoadSign) should offer the same walk where she stands, so the road itself invites the trip.

## What Changes

- Portable (`Simulation/HomesteadTravel.*`): `RoadSignDestinations` (the manor's sign points to town, town's home, the gateway's both ways), `RoadSignNear` (the sign within 280 cm) and `RoadSignLabel` ("To town", "To the manor", "Town / Manor"). Native scenario: every sign is found only within reach, every way it offers plans from its verge, and the manor sign's walk is `Simulation::WalkRoad` (time advances by the plan).
- World: `AHomesteadRoadSign` at each sign's anchor (position, ground height and facing from the road data), spawned on the fixed estate. Until Props' original sign mesh exists at `/Game/SurvivalGame/Environment/Props/RoadSign/SM_RoadSign` it is a clearly labelled stand-in: a post, a board and painted words with "(stand-in sign)".
- Focus: walking up to a sign focuses it ("Road sign | To town") with "[E] Walk to town (about 7 h 12 min)", or "[E] Choose a way" at the gateway.
- Interaction opens the field book's Map page with the Map tab's own confirm, centred: the plan summary (distance, time, arrival, closed-store warning) and one "Walk to ..." per way, plus "Stay here". Confirming calls the same `AHomesteadController::MenuTravel` (revision check, `PrepareWorldAt`, `Simulation::WalkRoad`, ground-snapped landing on the road facing the way she walked). A way that can't be walked is disabled, or refused with its reason when it is the only one: then only the notice shows, with no book and no pause (NativeMenu automation covers both the refusal and the two-way confirm).

## Impact

`Simulation/HomesteadTravel.*`, `Tests/HomesteadEconomyTests.cpp`, new `HomesteadRoadSign.*`, new `HomesteadControllerRoadSigns.cpp`, `HomesteadController.h/.cpp`, `HomesteadControllerFocus.cpp`, `HomesteadControllerInteraction.cpp`, `UI/SHomesteadMenu.h/.cpp`, `UI/SHomesteadMenuMap.cpp`, `UI/SHomesteadMenuDialog.cpp`. No save change. Props: the original sign mesh.
