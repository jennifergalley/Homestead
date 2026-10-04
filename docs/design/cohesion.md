# Homestead cohesion guide

A one-page guide maintained by the Balance Agent. It covers flavor, naming, UI and copy conventions,
beauty, and a prioritized UI polish audit. A lane checks a feature against it before `[ready]`. The
Balance Agent's balance numbers are in [balance.md](balance.md).

**Pillars, in priority order:** (1) cozy, casual, fun; (2) beauty; (3) verisimilitude. When they
conflict, the cozier, more fun or more beautiful option wins.

## 1. Flavor

- **Where and when:** the Trevennor estate, a neglected manor on the Cornish coast, spring 1851 on.
  The heroine, Eleanor Cavendish, has inherited it and is bringing it back to life. The world is
  coves, river valleys, hedge banks, granite, slate, engine-house ruins far off, a market town with
  Trethewey's general store (Mr. Josiah Trethewey, a shopkeeper in his fifties; renamed from
  Pascoe, a Poldark surname, on 2026-10-04).
- **Mood:** warm, unhurried, hopeful. Work is satisfying, never punishing; she doesn't starve,
  faint or die on the estate. Setbacks are gentle (a doze in the field, a missed fish).
- **Period feel without pastiche:** coins, posies, pasties, lamp oil, hemp twine, a field book.
  Prefer things a Cornish household of 1851 would know. Avoid modern words (sushi, upgrade,
  inventory in player copy, XP, quest) unless no period word reads clearly; clarity wins over
  period accuracy (pillar 1 beats pillar 3).
- **Names:** Cornish surnames and places (Trethewey, Trevennor, Penhallow, Tregarthen, Polwhele…)
  and plain Victorian given names. **Never reference *Poldark***: no Ross, Demelza, Nampara,
  Trenwith, Wheal Leisure/Grace, Warleggan, Pascoe, Hoskin(g), Jud, Prudie, Verity or close
  variants. If a name sounds like the show, pick another.

## 2. Copy conventions

| Surface | Rule | Example |
| --- | --- | --- |
| Interaction hint (focus card) | Name, then keyed verbs. Title-case the name, sentence-case the verb. | `[E] Harvest Turnips` · `[E] Eat Cornish pasty` |
| Tool use | Tools act on click / gamepad tool button; `E` only interacts. | `[LMB] Strike!` |
| Refusal | 4–6 words, no "You can't". | "Needs a fishing pole." · "The pack is full." |
| HUD notice (toast) | Only for non-obvious outcomes; one line; no exclamation spam. | "The fish have stopped biting here today." |
| Item description | One or two sentences, what it is then what it's for. Never mention implementation ("deferred", "placeholder", "TODO"). | "Mackerel sliced thin and eaten fresh. No fire needed." |
| Numbers | Thousands commas; "coins", not "c" or "$". | "1,500 coins" |
| Ranges | En dash, no spaces for numbers; spaced en dash for clauses. | "Open 8 AM–6 PM" · "Hotbar – the first row" |
| Dates | `Mon, Spring 1, 1851` style everywhere. Weather words match the HUD. | |
| Store name | Full: **Trethewey's general store**; short: **the general store**. Sentence case unless it's a title heading. | |

Assume the player knows farming games: don't toast obvious outcomes (picked up 1 hay) and don't
explain standard verbs.

## 3. UI conventions

- **Theme:** `UI/HomesteadPalette.h` + `HomesteadUITheme`. Default is light **Parchment** (paper,
  iron-gall ink, rust-brown accent); **Dark** ("book by candlelight") and **Classic** (pine) are
  options. Every new colour comes from the palette and must read in all three themes. No
  widget-local `FLinearColor` literals.
- **Type:** EB Garamond via `Font()`; key glyphs use `KeyFont`. Headings in small caps or title
  case; body in sentence case.
- **Icons:** code-drawn vectors in `SHomesteadIcon.cpp` on a 56-unit grid, ink outline plus muted
  fill (see `docs/ui-icon-provenance.md`). One style for every icon on screen.
- **Frames:** panels that float over the world (notices, focus card, HUD readouts) use the
  double-rule frame with drop shadow; nothing is a bare flat rectangle.
- **Accent use:** rust/brass marks selection, focus and "act now" cues. Green/red are reserved for
  nothing; good/bad use sage and rust tints plus an icon, never colour alone.

## 4. Beauty

- **Flowers everywhere:** verges, hedge banks, cottage beds, window boxes. Spring primroses,
  bluebells and wild garlic; summer foxgloves, sea pinks, honeysuckle. Cleared ground should bloom
  back, not stay bare.
- **Golden light:** warm mornings and evenings, soft fog in the valleys, rain that's pretty, not
  grim. Night is blue and lamp-lit, never pitch black.
- **Lived-in detail:** paths worn, stone walls mossed, the manor recovering visibly as she works.
  Progress should be seen in the world, not just in numbers.
- **Restraint in UI:** the world is the picture; HUD stays small, inked and warm, and fades when
  nothing changes.

## 5. UI polish audit (2026-10-04)

Jenny's ask: icons and menu/HUD should look like a polished, stylized game; for example, the plain
rectangles behind the clock, energy and money. Evidence: `docs/ui-gallery/2026-09-30/parchment-1080p`
(hud-overview, book-pack, book-craft, shop-buy, toast-success) and the UI source. Ordered by
player-visible impact per effort. Size: **S** ≤ half a day, **M** 1–2 days, **L** 3+ days. These are
suggestions for Jenny's backlog; nothing is scheduled.

| # | What's wrong | Improvement | Size |
| --- | --- | --- | --- |
| 1 | **HUD corner cluster.** Clock, energy and coins sit in three flat rectangles with a faint 1 px border, while notices and the focus card already have a framed look, the compass is a pill and the minimap a ring. It reads as debug UI. | One shared framed **almanac plate**: paper fill, double ink rule, small brass corner ornaments, soft shadow. Clock as a pocket-watch/sundial medallion with the day arc around it; coins on a little purse tag with stamped numerals; energy as the inked capsule from #3. | M |
| 2 | **No shared frame language.** Every panel is a `WhiteBrush` flat fill; hotbar, shop, book and HUD each improvise borders. | A procedural Slate frame (`SHomesteadFrame`: double rule, corner flourish, optional deckled edge, faint paper grain drawn in code), used by HUD plates, notices, focus card, hotbar, shop and book. Code-drawn keeps the no-image-asset icon rule. | M–L |
| 3 | **Energy bar** shows a bed icon, no label or value, and a saturated green fill outside the palette (`SHomesteadVitals.cpp` 35–43). | Inked capsule with tick marks at 25 (sprint) and 10 (slow walk), moss/sage fill turning rust when low, a loaf or heart glyph; value on hover/in book. | S |
| 4 | **Selection outline is blue** in the pack grid and craft list, outside the palette. | Rust/brass double outline, matching the accent. | S |
| 5 | **Craft requirement rows** use saturated green/red fills and `[+]`/`[-]` text. | Ink tick / cross glyphs with muted sage and rust tints; "2 / 3 branches" text. | S |
| 6 | **Icon style split:** craft tool icons are grey line art, hotbar icons coloured flat art. Locked recipes look like broken icons. | One style: ink outline + muted palette fill everywhere. Locked recipes desaturated with a small padlock glyph. | M |
| 7 | **Field book chrome:** tabs, Sell/Buy and Leave are flat blocks. | Bookmark-ribbon or thumb-index tabs, a spine shadow down the middle, a flourish rule under headings, buttons as stamped labels with an inked border. | M |
| 8 | **Hotbar selected slot** is a full rust fill that hides the icon's colour. | Paper cell with a brass frame and lift shadow when selected; stamped slot numerals. | S |
| 9 | **Compass and minimap:** sans letters on a pill; minimap a plain ring. | Serif small-caps cardinal letters on a ribbon; minimap with a brass bezel and a tiny compass-rose ornament. | S |
| 10 | **Wording drift** (see §2): the store is "Pascoe's General Store", "General store" and "General Store" in different places; "Open 8 AM - 6 PM" and "Hotbar - the first row…" use hyphens; the book tab says "Spring / Day 1 … Clear" while the HUD says "Mon, Spring 1 … Sunny"; focus card "Eat cornish pasty" (lowercase C); Legs slot shows "Linen tunic"; pail showed "Water 0 / 6" though source holds 15 (verify on a fresh capture); pole source "1500 coins" lacks a comma. | One copy pass against §2. | S |
| 11 | **Key chips** are pine/teal squares that don't match the accent. | Small ink keycaps (paper fill, ink border, slight bevel) in every theme. | S |
| 12 | **Dark theme** has no gallery captures, so nobody has checked it. | Capture a dark set with the next gallery run and review against this list. | S |
| 13 | **Day arc** sky blue is outside the palette. Optional. | Warm dawn-to-dusk wash from the palette, or fold into the #1 clock medallion. | S |

Top three for impact: **#1 HUD plate**, **#3 energy capsule**, **#2 shared frame** (they compound:
build #2, then #1 and #3 use it). Fast wins that need no new art: #4, #5, #8, #10, #11.

## Changelog

- 2026-10-04: first page and UI audit (Balance Agent).
