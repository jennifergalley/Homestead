# Spec Delta

## ADDED Requirements

### Requirement: The chosen book palette applies to every screen, with readable text
Every HUD element, interaction hint, toast, notice, book page, dialog, shop, chest and naming screen SHALL draw in the chosen palette (Light or Dark), switching straight away when the palette changes. Every text and background pair SHALL have a contrast of at least 4.5:1 in both palettes, and notice text SHALL be centred on its card.

#### Scenario: Dark HUD
- **WHEN** she chooses Dark and closes the book
- **THEN** the clock, meters, coins, hotbar and hint card are dark, and their text is at least 4.5:1 against its background
