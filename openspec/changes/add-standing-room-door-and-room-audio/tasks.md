# Tasks

- [x] 1.1 `Homestead::RoomAudio`: ambience and hearth mixes; native tests.
- [x] 1.2 Weather's indoor mix is public and checked in sun; ambience and creek duck in `UpdateCreekAudio`; hearth gain and roofed containment.
- [x] 1.3 `Homestead::Door`: leaf geometry from the doorway piece, hysteresis, swing and easing; native tests (opens west, never swings into the room, nothing saved).
- [x] 1.4 `HomesteadWorldDoors.cpp`: cube-built oak leaf on a hinge pivot, sight-blocking boards, no pawn or camera collision; `UpdateDoors` each tick.
- [x] 2.1 Editor and Game build: both passed (on a verification merge with the lake trail and the rain recurrence), and the Editor built first time with no fix-ups.
- [x] 2.2 PIE, no teleports: wake, walk to the door (it opens west), through it (no snag), away (it shuts); walk back in. Listen in and out with the door open and shut; compare the birds indoors and out; capture stills of the door shut, half open and open.
- [ ] 2.3 Swap in Props' oak-plank door mesh when it lands.
  _PIE 2026-09-30:_
  - _Door: shut from inside; she walked out west through the open leaf (swung past square outward) with no snag; it was shut again behind her at 5.5 m and open from outside at 1.9 m._
  - _Audio (component values): birds 0.70 outdoors and 0.28 indoors behind a 1.8 kHz low-pass; creek 0.245 and 0.098; hearth 0.32 in the room._
  - _Outside, the hearth is silent with the door open or shut: the listener (the camera, 4.7 m behind her) is already past its 9 m reach._
  - _Fix made in PIE: Unreal's low-pass setters don't update the component's own fields, so the change check re-sent them every tick. They now record what was sent; re-verified at 1800 Hz indoors, off outdoors._
  - _Captures: E:\\CopilotScratch\\89914e30-d8b6-4605-8635-5735406c97a2\\captures\\verify4pm (door_*.png)._
