# Design

1. **Schedule in the simulation** (`Simulation/HomesteadRain`; rain at any hour, Jenny 2026-09-30). Each
   calendar day (06:00 to 06:00) may draw one spell from a SplitMix64 hash of the day: whether it rains
   (by season: 20% of spring days, 13% summer, 25% autumn, 28% winter), when it starts (any hour), how
   long it lasts (1-8 h, `8 - 7u^2`, mean about 5.7 h) and how long its cloud builds before and clears after
   (0.5-1.5 h each). A spell may run past midnight and the next 06:00. A drawn spell whose cloud would come
   within an hour of the previous day's is dropped, so spells never merge. Day 0 is dry. Over twenty years
   it's wet 5.0% of spring hours (the old schedule's share), 3.2% summer, 6.0% autumn, 6.8% winter and 5.3%
   of the year (5% before), and about 43% of spells start between 19:00 and 06:00. Inside a spell
   `RainAmount` is a 0.3 drizzle swelling towards 1 in showers (two slow sines, eased, with a per-spell
   phase), easing in over 15 minutes and out over 10. `Overcast` ramps over the spell's build-up and
   clearing. `GroundWetness` wets over the first half hour and dries over four hours after. `Step` stops at
   `NextRainChange`, so a step never spans a spell's start or end. All are pure functions of the hour (no
   seed and nothing saved, so a reload brings back the same weather) and native-tested.
2. **Rain as one mesh.** `SM_RainStreaks` holds 8,000 near and 5,000 far quads, 26k triangles. Each is
   a 1 cm square at its home in its layer's box, with its layer and rank packed into UV0.x (OBJ carries
   one UV set). `M_Rain`'s vertex shader:
   - moves each streak by its fall velocity and wraps it in a lattice anchored to the world round the
     camera, so walking passes through the rain with true parallax;
   - billboards it along its fall direction and widens it with distance, lowering opacity to match;
   - hides it when its rank is above the rain's strength, or when it falls inside one of the eight
     roofs nearest the camera (MPC vectors, from `State.structures` roof pieces).
   The component sits at the camera each frame. It has no shadows, collision, distance field or ray
   tracing.
3. **Screen-constant brightness.** The streaks and clouds are unlit and divided by eye adaptation, so
   they look the same through any exposure. They're scaled by Daylight so they dim towards evening.
4. **Clouds over sky only.** A large sphere round the camera, drawn translucent where `SceneDepth`
   exceeds 100 km (only sky pixels), so no land at any distance is covered. The noise layer is
   projected onto a plane overhead, drifts with the wind and closes to full cover as overcast rises.
   Towards the horizon it greys completely.
5. **Lighting.** All in `UpdateLighting` from `Weather->GetOvercast()` and `GetRain()`: the sun's
   intensity and source angle, the sky light's intensity and colour, the exposure bias and saturation,
   and the fog's density, falloff and colour. The constants are in `HomesteadWorld.h`. At night cloud
   hides the moon down to `OvercastMoonScale` (35%). The sky light's overcast lift and the exposure's
   overcast bias apply by daylight only, so a rainy night keeps the night sky floor and the night
   exposure floor: darker than a clear night, never black, with the lamp and hearth carrying it.
6. **Indoors.** The camera is indoors under a building piece's roof (from the state) or when a line
   trace straight up hits something within 25 m, checked every 0.25 s. Under other cover the streaks
   fade out. Either way the sound drops to 35% behind a 900 Hz low-pass.
