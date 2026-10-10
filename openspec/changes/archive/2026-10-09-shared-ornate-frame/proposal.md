## Why
Panels used mismatched borders. One code-drawn ornate frame (double rule, corner flourish of 8 px or less at 1080p, paper grain of 6% opacity or less) gives HUD, hotbar, shop and field book the same look.

## What changes
- `SHomesteadFrame` (Slate) and `DrawFrame` (Canvas HUD) share `HomesteadFrameStyle.h`; colours come from `HomesteadPalette.h` in all three themes.
- Frames go on panels and the HUD cluster only, not on rows.
