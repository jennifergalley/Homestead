# Spec Delta

## Purpose

Field-book notices are brief, readable and never disturb the page the player is working in.

## ADDED Requirements

### Requirement: Notices never move the page
A notice raised while the field book is open SHALL NOT change the size or position of the book's tabs, page content, scroll position, focus or selection, either when it appears or when it goes.

#### Scenario: Move an item into a chest
- **WHEN** the player transfers a stack with the book open and a notice appears
- **THEN** every tile and control stays exactly where it was, and the same tile stays selected

### Requirement: Notices float over the book without taking input
The notice SHALL be a compact card over the book that takes no focus, clicks or navigation. Keyboard, mouse and gamepad input SHALL keep reaching the page while it shows.

#### Scenario: Keep navigating
- **WHEN** a notice is showing and the player presses a D-pad direction or clicks a tile
- **THEN** the selection moves or the tile responds as if no notice were showing

### Requirement: Notices keep clear of the focused control
The card SHALL sit at the bottom centre of the book, and SHALL move below the tabs whenever the focused control would lie under it, including a recipe square being held to craft and a dialog's focused button.

#### Scenario: Craft a recipe on the bottom row
- **WHEN** the player holds a recipe square that lies where the card would go
- **THEN** the card shows below the tabs, and the square's white fill stays fully visible

### Requirement: Notices are brief and do not pile up
A notice SHALL fade in, stay about 2.6 seconds (about 3.8 seconds for errors) and fade out. A new notice SHALL replace the one showing. A world toast raised before the book opened SHALL NOT reappear in the book, except a notice raised as it opened.

#### Scenario: Several transfers in a row
- **WHEN** the player makes several transfers within a second
- **THEN** one card shows the latest message, then fades within a few seconds of the last one

### Requirement: Notices read as period notes
The card SHALL use the project's licensed period display face at a size larger than the page's body text, centred, wrapping long messages within a maximum width. Errors SHALL be distinguishable by colour. Real confirmation dialogs SHALL keep their modal look and behaviour.

#### Scenario: A long error
- **WHEN** an error notice longer than one line appears at 1080p or 4K
- **THEN** it wraps inside the card in rust ink and stays inside the book
