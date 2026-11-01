# Window chrome and finite-strip overview

[Русский](CHROME-ATLAS.ru.md) · [Desktop gestures](DESKTOP-WORKSPACE.md) ·
[AS sources](as/chrome) · [Release](../docs/releases/v0.1719.md)

Window cards now use a diffuse edge shadow, opaque paper, an outer selected/grey
rim and an inner highlight/bevel. Shadows occupy the existing object footprint;
they do not require an enlarged dirty region or a second full-screen redraw.
Blue title strips have rounded corners, a bright lip and a darker lower edge.
Toolbar, vector and tab buttons have pill-shaped faces, a contact shadow and a
small highlight. Inputs use recessed paper; command bubbles use a raised dark
surface. Active windows retain a blue outline and paper-coloured contents.

Each view stays in its own executable AS file under `as/chrome/`. The existing
Coco/R AS2 → FIR → SWO → SWF build produces the action headers actually executed
by FlashVM. C exposes generic drawing capabilities and detached raster jobs;
it does not choose theme colours or widget layout. Unchanged chrome and associated
object bitmaps remain cached. Raster workers never access a VM/document object.

## Overview geometry and ownership

`Ideatlas` is a separate pure Limbo module. The table is seven logical pages,
each `height - 48` pixels tall before camera zoom. Normal window geometry is
bounded vertically to that strip; physical scroll is clamped to
`max(0, 7 * (height - 48) * zoom / 100 - (height - 48))`.

The overview is fixed at the bottom of the right shelf with an eight-pixel margin.
Its width is 44 pixels. Height follows the full strip aspect ratio, capped at half
the IDE height. At the smallest admitted window sizes it is further capped so
one whole toolbar button remains usable. The strip/map transforms use the same
rounded integer policy for miniatures, viewport and navigation.

`idedesktopatlas.b` composes ordinary copied `Item` values. Map 60 is the backing,
62 is the transparent viewport frame, 89..95 mirror source windows1..7 and61
mirrors source 8. Hidden windows stay on the left shelf; inactive MDI children do
not duplicate the visible container. A maximized overlay is represented at the
current logical viewport. The ordinary mirror path shows actual text/vector
contents, rather than placeholder rectangles. It owns no additional document.

Tap/click centres the viewport on the map point, clamped at the strip boundaries.
A held single-pointer map drag also navigates. Wheel events over the map do not
scroll the toolbar or table. The backing, live miniatures and viewport are separate
fixed right-shelf objects; they never inherit the toolbar's scroll offset.
Buttons outside the remaining viewport are omitted from the scene and cannot be
clicked invisibly. A popup closes if its invoking button scrolls out of view.

## Gesture admission

The first contact of a native touch session selects its surface. A three-contact
begin uses that original point, with later movement measured by centroid delta.
Thus a first touch on the left/right shelf continues to scroll that shelf even
when the other fingers land over the table. A first touch on the table/window
pans the table. The capture remains latched across multiple motion events.

Right-shelf commands activate on release, not provisional touch-down. Adding a
second contact emits private pointer subcode 11 (`ContactCancel`), cancelling the
pending tap before a triple pan starts. Lifting remaining contacts cannot create
a new tap. A vertical single-pointer drag over toolbar buttons starts scrolling
after an eight-pixel threshold; middle-button and Ctrl+Shift+left pan continue
through the existing route. The fixed map is excluded from button scrolling.
Map and toolbar captures clear on release, cancellation and stale admission.

These are private IDE event/scene roles, not shared Dis64 wire IDs. Rebuild the
native adapter and matching `.dis/.m` modules together. `idedesktoptools.b` owns
the admitted release policy; C only tracks/copies native contacts.

## Generic additive rounded primitive

`desktop_round_primitive.h` implements the native `ideDraw.round` capability used
by these AS views. It appends a contour without clearing existing layers.
The same-context target is pinned across allocations on the FlashVM owner.
No pointer, heap object or drawing call crosses into Dis or a raster worker.

| Property | Contract |
| --- | --- |
| `x`, `y` | Finite local coordinates, absolute value at most 16777215 |
| `w`, `h` | Nonnegative local extents, at most 1048576 |
| `radius` |0..1048576, reduced to half the smaller extent |
| `color`, `border` | Integer RGB24 |
| `stroke` |0..32 local pixels; zero disables the outline |
| `alpha` |0..100 percent |
| `filled` |1 paints the interior;0 leaves it transparent |
| `round = true` | Append the checked filled/outlined rounded contour |

An executable AS recipe, with every scalar explicit:

```actionscript
ideDraw.clear = true;       // Replace this control's old geometry once.
ideDraw.x = 0;              // Owner-local left edge.
ideDraw.y = 0;              // Owner-local top edge.
ideDraw.w = chromeWidth;    // Copied admitted widget width.
ideDraw.h = chromeHeight;   // Copied admitted widget height.
ideDraw.radius = 8;         // Requested corner radius, clamped by the primitive.
ideDraw.color = 15922161;   // Opaque paper RGB #f2f3f1.
ideDraw.border = 8227470;   // Grey rim; the real view substitutes selected blue.
ideDraw.stroke = 1;         // One local pixel of outline.
ideDraw.alpha = 100;        // Paper remains opaque over the desktop.
ideDraw.filled = 1;         // Paint the paper interior as well as the rim.
ideDraw.round = true;      // Execute through the owner-local native capability.
```

For a shadow, append a black rounded layer with a small offset and low alpha
*before* the paper. For a bevel, append a thin unfilled highlight afterwards.
The complete commented layers are in `window-card.as`, `window-header.as`,
`tool-scroll.as`, `vector-control.as`, `tab-button.as`, `miniature-card.as`,
`input-field.as` and `tool-palette.as`. Map backing/frame/miniature views are three
separate files. This capability belongs to the IDE host; it is not a new standard
Flash8 method or a separately loaded C-module ABI.

## Verification

Real Limbo owner tests check seven-page bounds, proportional/capped geometry,
live mirror references, map navigation, hidden-control refusal, multi-motion
shelf capture and post-release hover. Native SDL tests check first-contact
coordinates, added-contact cancellation and lifting without phantom taps.
Interpreter/JIT replays inspect actual map/source pixels, fixed rail positions,
viewport motion, cancelled Files taps, document isolation and native-sized max
windows. The max-size pixel oracle excludes the translucent bottom decoration
and permits at most 64 one-level antialias channel differences on rounded bevels;
glyph/control displacement or scaling still fails. These are dummy SDL/headless
checks, not physical-device latency or FPS measurements.
