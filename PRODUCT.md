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

- Cozy, never lethal. Energy and hunger are gentle, with no cold, death, predators or
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
