# Night brightness A/B/C, 2026-09-30

`sheet-1.jpg`: town square 21:00, meadow 21:00, meadow 00:00. `sheet-2.jpg`: manor forecourt 19:00, held lamp
on the drive 22:00, the manor room by the hearth with the lamp held 22:00.

Columns (`homestead.NightMoonLux` / `NightSky` / `NightMinExposure`; ground key = moonlit ground relative to an
adapted daylight grey):
- **Old**: 2.0 / 0.6 / -2, ground key 1.00, with the old lamp (`homestead.LampLegacy 1`). This is main e5877da8, Jenny's current build.
- **A**: 0.2 / 0.3 / -1, ground key 0.16. The next build's default.
- **B**: 0.3 / 0.4 / -1.25, ground key 0.29.
- **C**: 0.3 / 0.5 / -1.5, ground key 0.34.

Editor PIE with ray tracing on, clear weather, fixed cameras, 10 s of exposure settling per set. Built from
water-slot-1001 plus the Menu tip a37d693d. Scripts are in the Water lane's scratch (`night_ab.py`, `night_sheet.py`).
Rain doesn't fall at night yet; that's add-rain-weather section 4. See openspec/changes/fix-night-brightness task 2.3.
