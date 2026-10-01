# Tasks

## 1. Simulation

- [x] 1.1 `CropCare::DailyWeeds`, `Crops::WeedDay`, `Crops::GrowDailyWeeds`; drop the hourly creep in `Step`
- [x] 1.2 Pass at 6 AM when awake (`Step`), on waking from `Sleep` and `DozeOff`
- [x] 1.3 Native test `DailyWeedPass`: 18 h awake, night sleep, nap, sleep across 6 AM, woken before 6 AM,
      the book's split long sleep, doze, three days awake, the cap, reload stability; `GameplayWalkthrough`
      and `FarmingAndRain` expect no weeds within a day

## 2. Verification

- [x] 2.1 PIE (2026-10-01, awake path): a fresh plot tilled at 14:40 shows no weeds after 12 awake hours (02:41),
      then [F] Pull weeds at 07:31 after crossing 06:00; pulled at 07:40, none by 19:42. The sleep and doze paths
      are native-tested (DailyWeedPass); the PIE world had no bed to sleep in.
- [ ] 2.2 HomesteadFullLoop still passes (it sleeps to 06:45 and expects weeds after)
