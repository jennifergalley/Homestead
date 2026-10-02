# Spec Delta

## ADDED Requirements

### Requirement: The Appearance view can be turned and zoomed
While Appearance is open, dragging on the view, holding W/A/S/D or moving the right stick SHALL orbit the camera around her, and the wheel SHALL zoom within face and full-length limits. None of these SHALL move her, scroll the page or change the hotbar tool, and ordinary controls SHALL return when the book closes.

#### Scenario: Turn her with the keyboard
- **WHEN** she holds D on the Appearance page
- **THEN** the view circles her at a steady rate and she stays where she is

#### Scenario: Zoom in
- **WHEN** she scrolls the wheel up
- **THEN** the view moves in toward her face, stopping at the closest limit

### Requirement: The Appearance view faces her
Opening Appearance SHALL show her from the front, whatever her facing and even with a wall behind the camera, and closing it SHALL restore the camera and its collision.

#### Scenario: In the standing room
- **WHEN** she opens Appearance with a wall close behind the camera
- **THEN** she's shown from the front with nothing in between, and the camera returns to normal when the book closes
