## Why

Jenny's 2026-09-29 playtest: the hearth is too quiet, the woodland birds and creek play at full level
inside the standing room, and the room's west doorway is an open 130 x 220 cm gap with no door.

## What Changes

- The hearth crackle rises from 0.2 to 0.32 (about +4 dB). A roofed hearth heard from outside its room
  keeps 0.3 of that, so the fire stays in its room even with the door open.
- The non-spatial woodland ambience and the creek duck to 0.4 of the Ambience setting indoors, behind a
  1.8 kHz low-pass. They follow the same eased roof check the rain uses, which now runs in sun as well.
- The standing room's heritage stone doorway gets an oak ledged-and-braced leaf. The world owns it.
  It swings out west into the ruin's south range as she comes within 2.2 m and shuts once she is 3 m
  away. It never collides with her or the camera. Its boards block sight lines, so a shut door also
  silences the hearth outside.
- No save changes: the door's swing is presentation only.

## Impact

- Simulation: `HomesteadRoomAudio`, `HomesteadDoor` (new), with native `HomesteadRoomTests`.
- Unreal: `UHomesteadWeather` (public indoor mix, checked in any weather), `HomesteadControllerAudio.cpp`,
  `HomesteadWorldStructures.cpp` (hearth mix, door hook), new `HomesteadWorldDoors.cpp`.
- Props' oak-plank door mesh can replace the cube-built leaf later without changing the swing.
