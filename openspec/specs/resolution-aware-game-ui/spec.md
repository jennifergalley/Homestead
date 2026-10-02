# resolution-aware-game-ui Specification

## Purpose
Keep the Windows game interface readable and proportionate across 720p and 4K while making Settings controls honor direct mouse interaction.

## Requirements

### Requirement: 4K UI does not grow with every display pixel
The game SHALL use the available display area for its field-book, Settings, inventory and guidebook at high resolutions while bounding the physical size of text, icons, row heights and hit targets. It MUST NOT merely shrink the whole 720p menu into a small central island. HUD and hotbar shall remain proportionate, the 720p layout SHALL remain usable, and UI size MUST be independent of 3D render resolution without clipping core actions, focus cues or item counts.

#### Scenario: Compare menus at 720p and 4K
- **WHEN** the same field-book page is opened at 1280x720 and 3840x2160
- **THEN** the 4K field book still spans the available viewport with reflowed columns and rows, while text, icons and individual controls remain comfortable rather than triple-size; every tab, details panel, slider and close control remains available

#### Scenario: Adjust 3D render scale
- **WHEN** the player changes only 3D resolution scale
- **THEN** the UI layout, click targets and native crispness remain unchanged

### Requirement: Audio sliders honor pointer drag without navigation
Clicking, dragging or releasing each Music, Ambience or Effects slider SHALL change only that setting, keep Settings open, and commit once on release. Keyboard/controller changes and read-only/save-error recovery MUST retain their existing behavior.

#### Scenario: Click and drag a volume slider
- **WHEN** a player presses and drags an audio slider to another value and releases over or outside the row
- **THEN** the preview and saved level reflect the chosen position, Settings stays visible throughout, and no unrelated row is activated

#### Scenario: Unable to save audio settings
- **WHEN** a settings write is rejected after an audio drag
- **THEN** the previous persisted/runtime value is restored and the existing error is shown without closing Settings
