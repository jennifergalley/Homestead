# Tasks

## 1. Simulation

- [x] 1.1 `CropCare::DailyWeeds`, `Crops::WeedDay`, `Crops::GrowDailyWeeds`; drop the hourly creep in `Step`
- [x] 1.2 Pass at 6 AM when awake (`Step`), on waking from `Sleep` and `DozeOff`
- [x] 1.3 Native test `DailyWeedPass`: 18 h awake, night sleep, nap, sleep across 6 AM, woken before 6 AM,
      the book's split long sleep, doze, three days awake, the cap, reload stability; `GameplayWalkthrough`
      and `FarmingAndRain` expect no weeds within a day

## 2. Verification

- [ ] 2.1 PIE: weed a plot in the evening, sleep, see tufts at dawn; weed again, play until evening, none
- [ ] 2.2 HomesteadFullLoop still passes (it sleeps to 06:45 and expects weeds after)
