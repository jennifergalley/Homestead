# Design: oil lamp

## Items and prices

| Item | Category | Base | Store price | Notes |
| --- | --- | --- | --- | --- |
| Oil lamp | Tool | 150¢ | not sold | A tin-and-brass hurricane lantern: glass chimney, wire bail. |
| Oil flask | Supply | 12¢ | 15¢ | A stoppered tin flask of lamp oil; refills the lamp once. |

The store's 125% markup gives 15¢ a flask, about a fifth of a Cornish pasty ($1.00), which feels
right for a few hours of light. The store doesn't buy either back. A spare lamp isn't sold in this
change (see "One lamp" below).

## One lamp, one reservoir

She owns one lamp, so the oil is one number in `State`: `lampOilHours` (0 to `LampCapacityHours`
= 6 game hours, about 15 real minutes; a night is 11 game hours, so a full night takes a refill).
Wherever the lamp is, pack or ground, the reservoir goes with it. If a second lamp ever turns up
(the `HomesteadGive` console command), the two share the reservoir; that's an accepted
simplification until lamps are sold.

The lamp burns oil while it's lit:
- **In hand:** lit while it's the selected hotbar tool, she isn't asleep, and there's oil
  (`Simulation::SetLampInHand`, set by the controller every frame).
- **On the ground:** a lamp set down is lit while there's oil, day or night.

Burning never double-counts: one lamp, one flame. At 0 it goes out.

## Setting it down and picking it up

A set-down lamp is an ordinary **world drop** of `Item::OilLamp`: it reuses the drop rules (dry
ground within reach, clear of structures and plots), the drop persistence in the save, and the
existing "Pick up" focus and `PickUpDrop`. `Simulation::SetDownLamp(position, player)` wraps
`DropGroup` for the lamp's carried stack. The game renders lamp drops with the lamp mesh and a
light instead of the generic drop bundle.

## Refilling

`Simulation::RefillLamp()`: needs a lamp in the pack and a flask; spends one flask and fills the
reservoir. It refuses when the lamp is already nearly full. In game: the flask's pack menu
("Fill the lamp") and, with the lamp in hand, the secondary action (F / gamepad X).

## Saves

- A tagged trailing section `lamp <oilHours> <kitGranted>` is written after the manor section.
  Saves without it load with an empty reservoir and `kitGranted = false`.
- On load, if `kitGranted` is false, she gets the kit once (lamp, full, and three flasks) if her pack
  has room, then `kitGranted` is true. New games start with it granted.
- No `SimulationSaveVersion` change. The new Items are appended to the enum; the v13 save
  hardening (count-prefixed stocks, on main at 8c723e87) makes that safe for older saves.
- The hotbar's layout version (`UHomesteadSave::CurrentHotbarLayout`) goes up by one so existing
  hotbars get the lamp in a free slot.

## Held pose and light

- **Pose:** `AN_HeroineMH_LampRaised` is a one-frame upper-body pose (the craft-hands layer
  pattern: a `LayeredBlendPerBone` from `spine_02`) blended over locomotion. The right arm is
  raised and slightly bent, the fist at about head height, 35 cm ahead and a little right. The
  head is tilted a touch to peer past the lamp; the left arm hangs relaxed. This is the "Filch in
  the library" hold.
- **Prop:** `SM_OilLamp` hangs from her fist by the bail. The bail's top is the mesh pivot, so it
  hangs straight below the fist; the game keeps it hanging plumb (world-up) rather than following
  wrist roll, with a little lag-sway when she walks.
- **Light:** a point light at the flame. Warm colour (1.0, 0.62, 0.3), intensity flickered with the
  hearth's Perlin mix at a faster, smaller amplitude, attenuation radius 1000 cm (10 m), source
  radius 2 cm, shadows on. In daylight it barely registers, as a real lamp would.
- **Glass and flame:** the chimney is translucent glass; the flame is an emissive card.
- **Another slot selected:** she lowers her arm, and the lamp goes out and is hidden, like other
  tools.

## Set-down and pick-up animation

`AN_HeroineMH_LampSetDown`: she kneels, lowers the lamp by its bail to the ground 45 cm ahead,
lets go and rises. Pick-up reverses it with the same clip played backwards. Both are authored with
`rig_authoring.Session` (`lamp_pose.py`), like `craft_hands.py`. Lab commands: `LabHold Lamp` and
`LabAction LampDown|LampUp`.

## Judgment calls

- A hurricane lantern hangs from its bail, so she holds the bail in her fist, with the lamp below
  her hand at chin to head height. That reads better and more truthfully than gripping the glass.
- No lamp is sold in the store yet. A second lamp would need its own reservoir.
- When she sleeps with the lamp selected, it doesn't burn oil. She'd put it out.
