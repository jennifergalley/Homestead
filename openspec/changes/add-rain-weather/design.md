# Design

1. **Schedule in the simulation.** `RainAmount` is 0 outside `IsRainingAt`. Inside, it's a 0.3 drizzle
   swelling towards 1 in showers (two slow sines, eased), easing in over 15 minutes and out over 10.
   `Overcast` is 1 through the window and ramps over `OvercastLeadHours` (0.5) either side. Both are
   pure and native-tested.
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
   and the fog's density, falloff and colour. The constants are in `HomesteadWorld.h`.
6. **Indoors.** The camera is indoors under a building piece's roof (from the state) or when a line
   trace straight up hits something within 25 m, checked every 0.25 s. Under other cover the streaks
   fade out. Either way the sound drops to 35% behind a 900 Hz low-pass.
