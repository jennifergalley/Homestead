---
name: homestead-add-hud-element
description: Recipe for adding an on-screen HUD element or panel to Homestead (SurvivalGame) - a meter, readout, prompt, badge or small overlay - as a Slate widget that lines up with the Canvas HUD at any resolution. Use when a feature needs something new on screen outside the field book, or when changing where HUD elements sit.
---

# Adding a HUD element

The HUD is two layers (`docs/architecture.md`, section 2):

- **Canvas** (`AHomesteadHUD::DrawHUD`, `HomesteadHUD.cpp`): the calendar, key hints, toasts and
  focus cues, drawn every frame in 1080-line logical units.
- **Slate viewport widgets** owned by `AHomesteadController`: the hotbar, the vitals stack, the
  field book, the shop, the new-game names.

**New elements are Slate widgets.** Model yours on `UI/SHomesteadVitals.{h,cpp}` (a small,
self-contained example). Don't add more drawing to `DrawHUD` unless you're changing the calendar,
hints or toasts themselves.

## Steps

1. **Widget files:** `UI/SHomestead<Name>.h/.cpp`, in `namespace HomesteadMenus`:

   ```cpp
   class SHomestead<Name> : public SCompoundWidget
   {
   public:
       SLATE_BEGIN_ARGS(SHomestead<Name>) {}
           SLATE_ARGUMENT(TWeakObjectPtr<AHomesteadController>, Controller)
       SLATE_END_ARGS()
       void Construct(const FArguments& Args);
       // Where it sits in the Canvas HUD's 1080-line logical units, for layout checks.
       static FBox2D LogicalBox(float ViewWidth);
       static constexpr float Right = 30, Top = 300, Width = 460, Height = 54;
   private:
       TWeakObjectPtr<AHomesteadController> Controller;
   };
   ```

2. **Scale:** wrap the content in `SHomesteadHudScale` so one Slate unit is one Canvas HUD unit
   (the vitals and hotbar do this). Then positions and sizes in your constants mean the same thing
   as the calendar's, at 1080p and at 4K.
3. **Style:** colours from `UI/HomesteadPalette.h`; the widget's own shades, sizes and fonts in a
   named `<Name>Style` namespace (not an anonymous one, which can clash in unity builds). Fonts are
   `FCoreStyle::GetDefaultFontStyle("Bold"|"Regular", size)`; the period display face is used for
   titles only (`SHomesteadArrival`).
4. **Data:** read the controller in `*_Lambda` attributes (`Text_Lambda`, `ColorAndOpacity_Lambda`,
   `Visibility_Lambda`, `WidthOverride_Lambda`). Lambdas run every paint: read a value or two from
   `Controller->State()` or a small controller accessor; don't rebuild lists or format large
   strings per paint. If you need derived data, add a `const` accessor on the controller (in your
   feature's `HomesteadController<Feature>.cpp`, not the main file). Guard every lambda with
   `Controller.IsValid()`.
5. **Show and hide:** the controller adds HUD widgets in `AHomesteadController::ShowHotbar()` and
   removes them in `HideHotbar()` (`HomesteadController.cpp`). Add a `TSharedPtr<SWidget>
   <Name>Root` member, create it with an `SBox` whose `Visibility_Lambda` states when it's visible
   (copy the vitals' conditions: `bWorldReady && !bBookOpen && !IsFailed() && !ShopScreen.IsValid()
   && !HasNativeMenu()` plus yours), add it with `AddViewportWidgetContent(Root, 50)` and remove it
   in `HideHotbar`. Use `HitTestInvisible` unless it takes clicks. Z-orders: 50 for HUD widgets, 100
   for the field book.
6. **Layout:** keep clear of the fixed regions (Jenny's layout): key hints top-left, calendar and
   vitals top-right, minimap bottom-right, hotbar bottom-centre. If a
   Canvas element must avoid yours, have `AHomesteadHUD` read your `LogicalBox` (as it does for
   the vitals).
7. **Gamepad and 4K:** match the existing text sizes (hotbar counts 13 pt, the purse 20 pt, in HUD\n   units), and don't rely on hover: she may be on a controller.

## Verify

- Editor module build, then PIE: capture with `shot` (`CaptureEditorImage`). `hshot` /
  `HighResShot` doesn't draw Slate, so your widget won't appear in those.
- Check it at a 4K viewport too if it has text (`r.SetRes 3840x2160w` in a standalone `-game`
  window), and that it hides when the book, shop or new-game setup is open.
