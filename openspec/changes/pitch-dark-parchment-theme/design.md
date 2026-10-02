# Design: the dark (candlelit) book

Only the colours change. Layout, type (EB Garamond), shapes, icons, borders and spacing are the
parchment book's. It is `HomesteadUITheme::ETheme::Dark`, chosen in Settings › Book colours
(Light / Dark, saved in GameUserSettings), with `homestead.UITheme dark` or `-HomesteadUITheme=dark`
for testing. Light parchment stays the default until Jenny approves.

## Tokens (sRGB)

| Token | Hex | Used for |
|---|---|---|
| UmberDeep | #2A1F17 | page and panels (the classic pine panels) |
| UmberRaised | #4A3A2C | slots, selected rows and tabs (raised fills sit between the two) |
| Cream | #EFE2C6 | body text |
| MutedCream | #C8B696 | secondary text |
| Gilt | #D4AA62 | accent: titles, icons, the selected option's fill |
| OnAccent | #24190F | text and icons on the gilt fill |
| Terracotta | #E8906F | warnings and refusals |

`HomesteadUITheme::DarkOf` maps every classic colour onto these tokens, the same way `ParchmentOf`
maps it for the light book. Coloured meanings keep their own colours: the Energy and Food bars keep
their classic fills, and the Requires rows are olive (met) or oxblood (missing). Notice cards
(toasts, the hint/focus card and the book's notice) are umber with cream ink, a dim-gilt frame
(#9C7A45) and terracotta errors (`HomesteadNoticeStyle::Card*()`, paper #3A2C21); key glyphs sit on
a gilt stamp in umber ink. In the light book they stay paper (apply-dark-theme-everywhere).

Every Slate HUD widget (hotbar, meters, coins, clock, pickups) and the open book are rebuilt when
the palette changes; theme-dependent Slate brushes (notice card, naming screen, sliders) are cached
per theme.

## Contrast (WCAG, composited over the dimmed world or the map)

| Pair | Light fg / bg | Light | Dark fg / bg | Dark |
|---|---|---|---|---|
| Body text on page panel | #30291F / #D6CAAF | 8.9:1 | #EFE2C6 / #30251C | 11.7:1 |
| Secondary text on page panel | #594B38 / #D6CAAF | 5.2:1 | #C8B696 / #30251C | 7.5:1 |
| Body text on pack or hotbar slot | #30291F / #D0C3A6 | 8.2:1 | #EFE2C6 / #3B2E23 | 10.3:1 |
| Count badge on its chip | #30291F / #D6CAAF | 8.9:1 | #EFE2C6 / #30251C | 11.7:1 |
| Body text on selected row or tab | #30291F / #C8BA9C | 7.5:1 | #EFE2C6 / #45372A | 9.0:1 |
| Accent text (selected tab) on selected | #6F3519 / #C8BA9C | 5.0:1 | #D4AA62 / #45372A | 5.3:1 |
| Accent text (titles, icons) on panel | #6F3519 / #D6CAAF | 5.9:1 | #D4AA62 / #30251C | 6.9:1 |
| Text on accent fill (selected option, Feet row) | #D8CCB0 / #6F3519 | 6.0:1 | #24190F / #D4AA62 | 8.0:1 |
| Warning text on panel | #813021 / #D6CAAF | 5.4:1 | #E8906F / #30251C | 6.2:1 |
| Popup text on popup | #30291F / #D6CAAF | 8.9:1 | #EFE2C6 / #30251C | 11.7:1 |
| Map label on its chip | #30291F / #D8CCB0 | 9.0:1 | #EFE2C6 / #3B3127 | 9.9:1 |
| Selected map label on its chip | #6F3519 / #D8CCB0 | 6.0:1 | #D4AA62 / #3B3127 | 5.9:1 |
| Clock and calendar text on calendar panel | #30291F / #D0C4A7 | 8.3:1 | #EFE2C6 / #3E3328 | 9.5:1 |
| Clock PM (accent) on calendar panel | #6F3519 / #D0C4A7 | 5.5:1 | #D4AA62 / #3E3328 | 5.7:1 |
| Coins and meter label (accent) on vitals panel | #6F3519 / #D0C4A7 | 5.5:1 | #D4AA62 / #3E3328 | 5.7:1 |
| Warning delta on vitals panel | #813021 / #D0C4A7 | 5.1:1 | #E8906F / #3E3328 | 5.0:1 |
| Gain (+N, +coins) on vitals panel | #305019 / #D0C4A7 | 5.3:1 | #DDF3C5 / #3E3328 | 10.3:1 |
| Notice and hint card text | #30291F / #CEC2A6 | 8.1:1 | #EFE2C6 / #3A2C21 | 10.5:1 |
| Hint card title (secondary) | #594B38 / #CEC2A6 | 4.8:1 | #C8B696 / #3A2C21 | 6.8:1 |
| Error notice text | #813021 / #CEC2A6 | 5.0:1 | #E8906F / #3A2C21 | 5.5:1 |
| Key glyph on its stamp | #F6DFAF / #42554D | 6.1:1 | #24190F / #D4AA62 | 8.0:1 |
| Requires [+] on met row | #304510 / #BCBC95 | 5.4:1 | #D4AA62 / #3C3C20 | 5.2:1 |
| Requires [-] on missing row | #6F190D / #D4AA95 | 5.4:1 | #E8906F / #522219 | 5.4:1 |
| Requires text on met or missing row | #30291F / #D4AA95 | 6.8:1 | #EFE2C6 / #522219 | 10.2:1 |

Every pair is 4.5:1 or better in both palettes. Text on the accent fill always uses the accent's own
ink (`PineInk`: paper on the light book's rust, umber on the dark book's gilt), which fixes the
unreadable dark-on-rust selected rows (polish-parchment-ui item 6) in both.
