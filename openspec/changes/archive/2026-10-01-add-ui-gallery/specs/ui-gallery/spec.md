# Spec Delta

## ADDED Requirements

### Requirement: Every UI surface can be put on screen and captured
The project SHALL provide a Development-only UI gallery. Each entry has a stable id and a one-line description of what she should see. An entry starts from an isolated fixture and opens exactly one UI surface. A capture run SHALL record each entry as she sees it, Slate included, at the requested resolution and input device, without touching a real save.

#### Scenario: Capturing the gallery
- **WHEN** `Scripts\Capture-UiGallery.ps1 -Res 720p,4K` runs
- **THEN** each resolution's folder holds `<id>.png` for every entry, viewable copies and a contact sheet, and `index.md` lists each id with its description

#### Scenario: A new surface without an entry
- **WHEN** a field-book tab, settings tab or notice style has no gallery entry
- **THEN** the capture run fails and names what is missing
