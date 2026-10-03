# Town measured build 01

- Build: `20261002-measured-01`; app `44e7062d-49af-4d90-bc81-c3ad396086c6`; runtime `d1334c5c-5b56-4cc8-90aa-dfe669b2b2e2`; branch `jennifergalley-town-agent`; worktree `jennifergalley-vigilant-broccoli`.
- Model/config: observed `gpt-6.1-sol` / high throughout; requested default context, actual context unobserved. No model changes or helpers/helper IDs. Base `6390d37f`; verified implementation SHA `964aecb37c6b387f2d194cf9938ee0aee8cdcf69`; delivery HEAD includes this handoff receipt and is reported with `[ready]`. No main push.
- State/next: both changes implemented and directly reviewed; targeted native suites and one editor compile passed. Integration can merge/package; Jenny's integrated shop/sign visual acceptance remains unchecked. Park after feature-branch delivery, no further scope.
- Files: `HomesteadItems.cpp`, `HomesteadShops.{h,cpp}`, `UI\SHomesteadShop.cpp`, `Tests\HomesteadEconomyTests.cpp`; road generator/public-road include/header and `Tests\HomesteadPublicRoadTests.cpp`; the two OpenSpec changes. No UI-owned theme/map/inventory files changed.
- Crops: roots/berries now use the existing General Store buyer mask at unchanged 4/6 coins. Six period prices remain 20/16/14/90/8/13. Wild roots/berries use identical Items with no provenance and are therefore also accepted, as Jenny authorized; no other forage/economy scope.
- Production path: `SHomesteadShop::BuildRows` uses tested `ShopSellableItems`, carried quantity and `SellPrice`; `Limit`/Max cap to carried quantity; `Confirm` passes selected Item/quantity with Sell tab true to `AHomesteadController::ShopTrade`, which calls `Sim.Sell` and updates wallet only on success; `Refresh` rebuilds rows after each result. This is source-path verification plus production row-source/native transactions, not an in-game UI click test.
- Crop regression: all eight kinds are actually planted and harvested (ripe-save fixture), appear exactly once in the production Sell list, and exercise partial/full sale, exact coins/shop stock/layout, over-quantity/duplicate refusal, buyback, closed trades and byte-identical reloads. Existing normal-buy, Sunday, purse/capacity and travel checks also pass.
- Sign: retained `ManorRoadSign` name/order/destinations; chainage 71.41 m, 26 m beyond the farm's farthest projected corner, 4.2 m off-road, local +Y board along the townward tangent. Minimum nearby resource-centre clearance about 3.6 m. No sign remains by the manor. Separate manor arrival remains exactly `(-26340,-64920,8652.3,309.7)`.
- Save safety: no enums, save fields/versions, bake versions, serialized placements or resource IDs changed. Generator diff is one sign row only; layout JSON/scenery/bridge/other signs unchanged. Initial sign candidate overlapped tree 510053; adjusted chainage instead of moving saved resources.
- Commands/results: targeted CMake Release builds for `HomesteadEconomyTests` and `HomesteadPublicRoadTests`; crop selector passed 303 checks; CTest `-R 'Homestead(Economy|PublicRoad)Tests'` passed 2/2; `python Scripts\Terrain\public_road.py` cleared zero scatter records; `Scripts\Invoke-UnrealBuild.ps1 -Target SurvivalGameEditor` succeeded (128 s, including Slate shop/controller and simulation compile). No UAT/package/live editor/game. TEMP/TMP on this runtime's E: scratch.
- Fixture/tool lessons: empty-estate reload fixture must retain the placement bake identity; standalone MSVC probe must use `/MD` to match CMake's runtime. Probe and transient logs are cleanup-only artifacts, not shipped tooling.

## Task and usage boundaries

Times below are on 2026-10-02, local (-07:00); cursors are sampled local `assistant_usage_events` IDs, not exact billing partitions. Early practical research covered both scopes; combined CTest is included at the tail of sign work.

| Segment | Start/end | Usage cursor boundary |
| --- | --- | --- |
| Startup / policy overhead | first recorded 21:36:29; before crop research | 68691-68721 |
| Crops research / implementation / first native pass | 21:38:28 to 21:45:26.9758702 | start sampled 68725; transition sampled 68803 |
| Sign implementation / geometry / combined native suites | after crop pass to 21:48:07.9746405 | 68803 to shared-validation sample 68826 |
| Shared review / editor compile | queue clear 21:48:29; compile complete 21:50:48.3151346 | 68826 to sample 68845 |
| Delivery / handoff / cleanup | after compile; receipt sampled 21:53:35 | 68845 through sample 68896; final messages follow |

Local recorded usage through cursor 68865: startup 32,293,110,000; crop 106,401,450,000; sign/CTest 18,655,560,000; shared editor/review 4,211,300,000; delivery-so-far 9,220,180,000 nano-AIU (55 recorded calls, none missing in that sample). Total 170,781,600,000 nano-AIU at that cursor only, not a final bill. Accounting owns final export, conversion and post-handoff usage; no unobserved cost is treated as zero.

Delivery receipt through cursor 68896 (`2026-10-03T04:53:35.038Z`): 59 recorded calls, 182,170,580,000 nano-AIU, zero missing usage records in the sample. Handoff commit/push/messages after this receipt belong to delivery overhead and require Accounting's final export.

No helpers, wakeups/automations, live processes or live editor were created. Named scratch is cleaned before delivery; ignored build caches and the worktree-local UBT verification log remain reproducible evidence. Jenny owns actual-game acceptance; Integration owns the measured Shipping build. Stop promptly on sign-off.
