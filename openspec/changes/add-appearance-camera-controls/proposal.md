# Proposal

## Why

Jenny: on the Appearance page she wants to drag on her to turn her, WASD to turn the view (without walking), the wheel to zoom, and to see her from the front even with a wall behind her. She also asked that the "Curly bob" hairstyle be called "Long bob".

## What Changes

- A left drag on the Appearance view (anywhere the page has no control of its own) orbits the camera around her, and the capture ends cleanly on release or focus loss.
- Held W/A/S/D orbit at a steady rate (90°/s yaw, 45°/s pitch), and the right stick does the same on a gamepad. The book owns these keys, so she doesn't walk; the D-pad still changes the highlighted choice.
- The wheel zooms between her face (90 cm) and full length (340 cm) wherever the pointer is. It doesn't scroll the page or change the hotbar tool while Appearance is open, and ordinary wheel behaviour returns when the book closes.
- Opening the page always frames her from the front: the camera arm stops testing collision while the page is open, and the existing near-clip removes anything between the lens and her. On close, the collision setting, arm length and view are restored.
- The hairstyle list shows "Long bob" for the style that said "Curly bob" (display text only; the id and saves are unchanged).
- The page hint mentions dragging, WASD, the right stick and the wheel.

## Impact

- `HomesteadCharacter.h/.cpp` (OrbitAppearance, ZoomAppearance, arm collision while open, zoom-aware framing), `HomesteadControllerAppearance.cpp` (new), `UI/SHomesteadMenu.h/.cpp` (drag capture, WASD and stick handling, wheel), `HomesteadController.cpp` (the hint), `HomesteadAppearance.cpp` (the label).
