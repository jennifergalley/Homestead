# Native UI icon provenance

`Source\SurvivalGame\UI\SHomesteadIcon.h` and `.cpp` contain original,
code-authored vector geometry for Homestead. The coordinates, silhouettes,
arrangements, and details were authored for this project, not traced or copied
from Coral Island or any other reference. No downloaded images, icon packs,
reference assets, external fonts, textures, or generated image files are used.

Rendering uses standard `FSlateDrawElement::MakeLines` and `MakeBox` with
`FCoreStyle`'s `WhiteBrush`. Filled contours are locally scan-converted into
one-design-unit box bands and finished with antialiased lines; there are no
custom Slate vertices, engine-specific polygon helpers, or asset loads.
Only the unknown-key `?` uses `MakeText` and the engine's built-in CoreStyle
font. None of the supported icons uses text or a font glyph as artwork.

## Public API

```cpp
#include "UI/SHomesteadIcon.h"

SNew(SHomesteadIcon)
    .Kind(FName(TEXT("pack")))
    .Tint(FLinearColor(0.92f, 0.74f, 0.43f, 1.0f))
```

`SHomesteadIcon : public SLeafWidget` exports
`void Construct(const FArguments& InArgs)`.
Its `SLATE_BEGIN_ARGS` declares `SLATE_ATTRIBUTE(FName, Kind)` (default `pack`)
and `SLATE_ATTRIBUTE(FLinearColor, Tint)` (default Gold above).
`Kind` is stored as `TAttribute<FName>` and evaluated with `.Get()` during
`OnPaint`. Existing `.Kind(FName(...))` calls remain supported, as do dynamic
bindings such as `.Kind_Lambda([this]() { return SelectedIconKey; })` where
`SelectedIconKey` is an `FName`. Bound kinds make the widget volatile so cached
paint does not freeze a changing detail icon. Tint also accepts `.Tint_Lambda`
and is evaluated at paint time; a bound tint participates in volatility.
Normal inherited Slate widget arguments remain available.

Desired size is **56 x 56 Slate units**. Painting uniformly fits and centers
the 56-unit artwork inside the actual allocation, including non-square slots.
Zero-size allocations emit no elements. Paint layers advance in drawing order
and `OnPaint` returns the highest layer used.

Tint RGB replaces Gold accents and the tunic/apron cloth fill, not the rest of
the material palette. Carried/worn garment rows use the existing authored
MossLinen base RGB and shared tunic dye multipliers as a symbolic color cue,
with a cream silhouette outline and explicit dye name. This is not a rendered
material swatch or a promise of identical lighting. Tint alpha fades the
entire illustration. Inherited widget color/opacity multiplies all colors;
disabled state uses Slate's standard `DisabledEffect`. The icon is decorative,
not an interactive control; the host owns labels, accessible descriptions,
selection, hover, and controller navigation.

## Stable keys and authored silhouettes

Keys are explicit `FName` lookups, with normal case-insensitive `FName`
comparison. All 32 keys below remain explicitly resolved on each paint.
Unsupported names, including `NAME_None`, display a charcoal `?` on a
Gold/accent backing for contrast. Other icons contain no text; menu label
colors remain the host's responsibility.

| Key | Illustration |
| --- | --- |
| `pack` | Flapped rucksack, carry loop and buckle |
| `craft` | Crossed mallet and eyed sewing needle |
| `build` | Gabled house outline with chimney and door |
| `guide` | Open field book with page marks and botanical leaf |
| `settings` | Three vertical sliders at different heights |
| `credits` | Portrait inside an open leafy wreath |
| `appearance` | Faceted hand mirror with reflected highlights |
| `knife` | Pointed diagonal blade, crossguard and riveted handle |
| `branch` | Forked woody branch with one leaf |
| `stone` | Broad asymmetric rock with light and dark facets |
| `fiber` | Splayed bundle of strands with binding |
| `berries` | Three round red berries below paired leaves |
| `roots` | Two tapered roots with standing foliage |
| `flowers` | Two distinct blossoms, stems and leaves |
| `seeds` | Sprout-marked seed packet and three loose seeds |
| `hatchet` | Long handle with broad bound stone axe head |
| `digging-stick` | Looped grip, diagonal shaft and pointed wooden end |
| `watering-can` | Banded vessel with loop handles, spout and drops |
| `water` | Single broad water drop with reflected edge |
| `roasted-roots` | Roasted roots on a shallow platter with rising steam |
| `herbed-roots` | Roots and tall herb sprig in a deep bowl |
| `foundation` | Low isometric plank platform with visible slab thickness |
| `wall` | Upright planked panel, posts and diagonal brace |
| `doorway` | Open timber frame with tied-back curtains and threshold |
| `roof` | Wide pitched roof with eaves, seams and short supports |
| `fire` | Asymmetric flame above crossed logs |
| `bed` | Side-view bed with pillow, blanket and upright posts |
| `chest` | Low lidded chest with straps and central lock |
| `linen-tunic` | Sleeved tunic with neckline and tied belt |
| `linen-apron` | Sleeveless bib apron, neck loop, ties and patch pocket |
| `leather-shoes` | Two offset low shoes with laces and soles |
| `woven-footwraps` | Two tall wraps with diagonal woven bindings |

The palette preserves `DESIGN.md`'s linear Pine `(0.055, 0.09, 0.075)`,
cream/Ink `(0.93, 0.93, 0.84)`, and Gold `(0.92, 0.74, 0.43)`.
Pine is opaque in internal details, not a panel background. Additional
wood, leaf, berry, root, stone, and water colors are original material accents.
Identity is carried by geometry and details rather than color alone.

## Integration and validation boundary

This isolated contribution changes only the two icon source files and this
document. The main writer owns menu/controller integration and module
dependencies; the hosting Unreal module needs `Slate` and `SlateCore`.
No build configuration, menu, controller, or reference document is changed here.

Static checks cover explicit key/enum/paint-case parity, documented coverage,
source whitespace, and use of the intended draw primitives. **Uncompiled**:
no Unreal, UBT, UAT, MSVC, build, or Blender process was launched. No tools were
installed or modified. Runtime rendering, DPI/disabled appearance, small-size
legibility, and performance still require native integration review.
