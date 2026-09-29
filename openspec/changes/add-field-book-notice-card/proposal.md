# Proposal

## Why

Jenny: "When I move stuff or craft stuff, please don't have messages appear at the top of the menu / inventory screens, as they push things down and that's jarring. Instead have it appear as a modal above the menu screen that disappears shortly."

The field book showed every toast (transfers, crafting, sorting, eating, pinning, plans, saves) as a banner between the tabs and the page. It took layout space, so the whole page jumped down while it showed and back up when it went. She also asked for it to look Victorian: a period font, larger text, centred in a compact panel.

## What Changes

- Remove the banner from `SHomesteadMenu`'s layout. The page, its scroll position, its focus and the selected tile stay exactly where they are when a notice appears or goes.
- Show the latest toast as a notice card over the book. It's a small parchment slip with a double-ruled ink frame, EB Garamond text (the licensed display face the title card uses) centred and wrapped at a maximum width, and a soft shadow. Errors use a rust ink and frame.
- The card never takes focus, clicks or layout (`HitTestInvisible`), so keyboard, mouse and gamepad keep working. It sits at the bottom centre of the book, and moves under the tabs when the focused control (a tile, the held craft square, a dialog button) is where the card would be.
- It fades in, stays about 2.6 s (errors 3.8 s), and fades out. A new message replaces the one showing; nothing piles up. A notice from the moment the book opens (a save or a load) shows, but an older world toast doesn't.
- Real confirmation dialogs stay modal. World toasts with the book closed are unchanged, and so are the controller's toast state and timings that tests read.

## Capabilities

### New Capabilities

- `field-book-notice-card`: Brief notices over the field book that never move the page, take focus or cover the focused control.

### Modified Capabilities

None.

## Impact

- `Source/SurvivalGame/UI/SHomesteadMenu.h/.cpp`: the banner goes; the notice card, its style namespace (`MenuNoticeStyle`) and its placement.
- `Source/SurvivalGame/HomesteadController.h`: read-only accessors `ToastSecondsLeft()` and `NoticeCount()`.
- No simulation, save, input mapping or HUD change. The shop keeps its own footer status line.
