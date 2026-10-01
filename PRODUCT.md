# Product

<!-- impeccable:product-schema 1 -->

## Platform

Windows-native Unreal Engine game. This is not a web, iOS, or Android product.

## Users and purpose

A personal, offline, mouse/keyboard-and-controller cozy life sim for Jenny, set on an
early-Victorian Cornish coast. As the daughter of a noble family returned to a derelict
estate, she clears the overgrown land, farms, ranches, fishes and reopens the family mine.
She hauls her goods to town to sell, rebuilds and decorates the manor, hires help, and
restores the family's name. The goal is a fortune and standing, not survival.

## Source of truth

`docs\game-plan.md` summarises the direction and round order.
`openspec\changes\pivot-to-cozy-estate-life-sim\design.md` records the approved interview
decisions, and they take precedence over inferred preferences. Each round is its own
OpenSpec change.

## Operating context

Mouse/keyboard or Xbox controller on a Windows PC with a Ryzen 7 3700X, RTX 5080,
32 GB RAM, and a 4K display. Mouse/keyboard is likely to dominate frequent
playtesting because it is immediately available; neither input path is secondary.
Target 60 FPS with adjustable quality/upscaling.
Menus and construction planning pause simulation. No online gameplay services.

## Product principles

- Cozy, never lethal. One gentle energy meter, like Coral Island, with no hunger, cold, death, predators or
  spoilage. Setbacks are small and recoverable, and she never loses the estate.
- Earning a fortune is the goal, with real money sinks (rates, wages, upkeep, materials, land
  and upgrades) so wealth is earned.
- Physical loops are pleasures in their own right: clearing, farming, ranching, fishing,
  mining, hauling goods to town, and building.
- One fixed, hand-authored world, grounded in real Cornish terrain and period
  verisimilitude. Modern custom clothing for the heroine and dollars as currency are
  deliberate exceptions.
- The heroine and environment must be appealing, not merely technically present.
- Coral Island is the primary comparative reference for the loop, economy, tool tiers, town
  and shops. Stardew Valley, Minecraft and Disney Dreamlight Valley inform mining, artisan
  goods, interaction, inventory and UI decisions.
- Characters are photorealistic, at MetaHuman quality.
- Preserve progress reliably.
- Keep repeated tool work immediate through a visible ten-slot carried-tool hotbar, with
  mouse/keyboard and controller parity.
- **Keep interaction and tools distinct.** E/the interact button only interacts—harvest, plant,
  pick up, open, talk, eat or sleep. Tools act only through click/the gamepad tool button. No
  crossover: E on an unripe crop never waters it. Hold-to-repeat binds only to tool input.
- **Bed sleep is one press.** Within its tight bed focus, E/A immediately sleeps until Energy is
  full, capped at 06:00 when sleeping overnight. If she is already rested at night it sleeps until
  06:00; a rested daytime bed has no sleep verb. There is no confirm dialog or nap-hours picker.
- **Estate Energy is never fatal.** Below roughly 25% Energy she cannot sprint; below roughly 10%
  she walks more slowly and tool work says `Too tired`. The Energy bar changes colour and pulses,
  with `Getting tired` and `Exhausted` warnings. Eating or sleeping restores her; she never faints,
  dies, fails, or loses progress from low Energy. Any recovery/checkpoint load chooses the newest
  valid save by timestamp and revision.
- **Respect farming-sim fluency.** Assume the player knows the genre: UI is concise rather than
  instructional. No toasts for obvious outcomes; focus cards show only a name and keyed verbs;
  details/tooltips show stats, requirements and price rather than rules explanations; settings show
  a label and value; refusals are short (about 4–6 words). Review new copy against this principle
  before it ships.
- Ship small playable increments. Don't build the entire roadmap at once.

## Evidence and constraints

The planning interview and supplied local portrait establish the visual brief:
grounded countryside, a petite adult heroine, long wavy brown hair, and a short
straight bob option. Do not upload the portrait to external services.
Free assets first; no purchases are authorized. Unreal and character asset
feasibility remain explicit gates. A technical stand-in must be labeled, not
presented as the approved heroine or finished graphics.

## Accessibility

Complete controller navigation, readable 4K UI, clear interaction targets,
separate audio controls, and warnings that do not rely on sound alone.
