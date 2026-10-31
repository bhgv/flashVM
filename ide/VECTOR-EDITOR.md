# MDI vector editor: drawing and SVG

[Русский](VECTOR-EDITOR.ru.md) · [Entry points](ENTRYPOINTS.md) ·
[Desktop](DESKTOP-WORKSPACE.md) · [Example](dis/examples/vector/README.md)

Implemented in v0.1696, expanded with screen controls and SVG interoperability in v0.1697: a new independently owned vector document type inside
menu **8**, the existing MovieClip IDE. Click **Draw** on the right, or press
**Ctrl+Shift+N**. Each command creates another MDI window, up to the existing
combined eight-window limit. Text editors, vector documents, Help and console
coexist. Window movement, max/normal, desktop zoom, thumbnails, docking and
revealing use the existing layout owner. A thumbnail contains the actual paths.

There is no separate mock browser/canvas application. Authoring logic and Undo/
Redo run in portable Limbo/Dis. The FlashVM context owner receives copied path
commands and calls actual MovieClip moveTo/lineTo/curveTo/lineStyle/beginFill.
Detached contours then go through the existing raster workers. No worker uses a
VM/font/document pointer. This is scheduled Dis ownership, not parallel VM/GC.

![Actual MovieClip vector window and thumbnail](assets/vector-v0.1697.png)

## Drawing

Focus the vector window and use its two rows of rounded child MovieClips:

| Row | Left to right |
| --- | --- |
| Upper | Brush, rectangle, ellipse, line, Undo, Redo, narrower stroke, wider stroke |
| Lower | Six colour swatches, fill toggle, pen with numeric stroke width |

The last lower-row tile selects Pen and displays the current width. Selected tools/colours/fill are highlighted;
unavailable history and width steps are dimmed. Clicks carry a copied cell
proposal through the same ticket/owner/revision admission as the canvas. Each
pane and miniature owns its own16 child MovieClips. The model remains on Dis.
Controls hide in very narrow panes; keyboard shortcuts remain available.
The owner also accepts explicit keys and asynchronous/batched SDL text commands.
Node selection/editing and a full Flash-style toolbox remain future work.

| Key | Action |
| --- | --- |
| B | Brush: filled round nib |
| P | Pen: stroked centerline |
| R | Rectangle from opposite corners |
| E | Ellipse from opposite corners |
| L | Straight line |
| 1–6 | Blue, red, green, ochre, black, purple |
| F | Toggle same-colour interior fill for closed figures |
| + / - | Stroke width 1..32 in document units |
| Ctrl+Z / Ctrl+Y | Undo/Redo the whole committed gesture |
| Escape | Cancel an uncommitted gesture |
| Ctrl+S | Save SVG; choose a path if untitled |
| Ctrl+Shift+S | Choose another SVG path |
| Ctrl+O | Read supported SVG into the active clean vector document |

The page has logical coordinates 0..1000 by0..600. A uniform scale fits it below
the toolbar, with a logical 20-unit stroke margin. Mouse hit proposals use the
actual composed MovieClip matrices, including table/window zoom and pan. The
Limbo app independently admits the completed view ticket, frontmost pane and
source revision before sending the gesture to its owner. Two/three-contact
window/desktop gestures cancel a pending brush gesture and retain existing policy.
After an admitted canvas press, the gesture captures that pane. Continuations use
its current composed matrix and clamp to the page, rather than re-hit another
widget. Escape cancels; ordinary commands/focus changes finish at the last admitted
sample. Existing multi-contact window/table gestures retain their cancellation policy.

Brush and Pen keep raw samples while the pointer moves: no curve fitting or
sample decimation in the live path. Captured native motion is not coalesced away
and does not wait for the previous raster to complete. Pen becomes a stroked
midpoint-quadratic centerline after release, preserving the final endpoint.
Brush sweeps a round nib of width1..32, then traces and simplifies the **filled
boundary** after release. Visible connected same-colour filled shapes/brush areas
unite, including transitive connections; enclosed holes remain compound subpaths.
Different-colour overlays are respected; covered older paint is not resurrected.
The implementation uses a bounded1000×600 packed coverage grid, then polygonal
approximation with a0.75-unit distance threshold. This is quantized vectorization,
not an exact analytic Boolean operation. Imported curves use eight subdivisions
for union coverage. Pressure, erasing, node editing, layers and animation are future work.

Live panes use a separate spawned Limbo process for final geometry, independent
of the Flash frame clock. This is scheduled Dis work, not a claim of parallel
native VM/GC execution. The owner first commits visible raw ink and one Undo unit,
then sends immutable shapes to the worker. One job plus one latest queued snapshot
bounds the queue. Only matching revisions are published; stale jobs cannot replace
new edits. Save explicitly waits for current work; ordinary input does not.
Worker failure or geometry quota refusal retains raw ink and reports a diagnostic.
No periodic10-second fitting is enabled yet: refinement currently runs on release.

Rectangles use lines and close. Ellipses use eight quadratic arcs with tangent
intersections at 22.5 degrees (an approximation, not exact conics). All paths use
the same generic commands for preview, committed drawing, history and SVG export.
A preview changes no committed path/history; release commits once. Sixteen Undo
and sixteen Redo states retain immutable path references. Undo/Redo creates a new
revision and conservatively marks the document dirty even if geometry matches an
older saved state. Dirty title/close/quit protection is shared with text windows.

## SVG files and refusal policy

A launcher document ending in `.svg` starts as a vector owner automatically:

```sh
sh out/disvm-editor/run-desktop-ide.sh --document /absolute/path/to/drawing.svg
```

The first importer deliberately accepts a **strict subset**, not arbitrary SVG:

- One `svg` root, optional standard xmlns, width1000, height600 and
  viewBox=`0 0 1000 600` (omitted dimensions use that same logical page).
  A self-closing empty root is accepted too.
- Optional UTF-8 BOM, XML1.0 declaration with UTF-8 encoding/standalone metadata,
  and well-formed inter-element XML comments. These non-rendering annotations
  are normalized away by canonical Save; they never execute/load resources.
- Self-closing `path`, `rect`, `ellipse`, `line` elements, with unique quoted
  supported attributes and whitespace between attributes.
- Absolute explicit `M`, `L`, `Q`, `Z` path commands, closed compound subpaths (each new M follows Z), integer
  coordinates inside the page. Exactly integral decimals such as100.000 are
  accepted without rounding. Each repeated segment needs its command letter;
  separators are whitespace. No nonzero fraction/exponent/relative/comma shorthand.
- RGB colours `#rrggbb` or `none`; fill and stroke must share the same RGB when
  both are present. Default fill is black, default stroke absent. Stroke width
  is 1..32; fill-only paths disable their native stroke.
- For primitives: x/y/width/height, cx/cy/rx/ry, or x1/y1/x2/y2 respectively.

Unsupported elements/attributes, DOCTYPE/entities/unknown processing instructions,
CSS, transforms, different fill/stroke colours, arbitrary viewBoxes, nonintegral
numbers, cubic/arc commands and unterminated extra subpaths are **rejected transactionally**.
They are not silently dropped or converted. This prevents an apparently successful
read/save from losing unsupported artwork. The SVG reader is a bounded data-format
reader, not a replacement compiler parser; AS/Limbo recognition stays on Coco/R.

Export writes deterministic ordinary SVG paths; every file exported by this
implementation roundtrips through its reader. Reading builds a new document
before adopting it. Dirty documents refuse Open. A successful Open advances the
pane revision, resets history and marks the adopted document clean. Save adopts
the destination and saved revision only after a successful complete write.
Filesystem writes may block the VM owner and are checked overwrites, **not atomic
replacement/fsync**: an I/O failure can leave a partial destination file. The
original in-memory artwork remains available for another Save.

## Bounds and presentation protocol

Limits are local resource policy, not SVG/Dis64 wire requirements:32 paths,
4096 commands/path,16384 commands/document,1 MiB complete UTF-8 SVG input.
Authoring reserves4096 commands before starting a gesture, so admission is
conservative. At the sample bound, the retained raw stroke commits as one Undo unit with
a Limit status; new samples are refused and earlier points are never decimated. The complete drawing stream is bounded by660000 bytes.
Raw contour tracing also has an explicit65536-vertex intermediate budget.

`Idedeskpane.Reply.drawing` carries a complete copied snapshot. Local UI codes
65576/65577 stage Begin/data fragments inside the ordinary desktop Begin/End:
Begin identifies pane/revision/exact bytes and explicit private version2. The
complete stream starts with tool/colour/width/fill/Undo/Redo and path count;
data has the same view ticket and
exact next byte offset, at most1024 bytes. The native adapter validates the whole
stream, every style, range, opcode and extent before scene commit. Missing,
reordered, duplicate, overlong or malformed fragments cannot commit partial paths.
These are private UI codes, **not shared Dis64 format/opcode IDs**. Old unversioned
vector messages are rejected. Hit metadata uses signed int32 cells -2..-17;
raw uint32 parse results are explicitly sign-extended on NC64. Exported module
signatures changed: rebuild/install the matching .dis/.m application together.

## Verification and sources

- [Model](dis/bridge/idevector.b) / [interface](dis/bridge/idevector.m): real gesture,
  raw samples, post-release fitting, dot, figure, immutable history and SVG logic.
- [Model regression](dis/tests/idevector_test.b): roundtrips, common primitives,
  malformed/unsupported/truncated input and atomic quota refusals.
- [Paint regression](dis/tests/idepaint_test.b): Pen centerline, Brush fill/hole,
  queued raw strokes, connected union and immutable Undo masks.
- [Owner regression](dis/tests/idevectorpane_test.b): independent vector panes,
  revision-bound preview, dirty file refusals and acknowledged shutdown.
- [Native admission](../disVM/runtime/test_desktop_vector.c): fragment order,
  tickets, bounds, command checks and refusal of incomplete commit.
- [Actual UI replay](../disVM/runtime/test_desktop_vector.py): real Dis input,
  MovieClip curve/fill pixels, Undo/Redo, SVG Save/Open and direct `.svg` launch.

Interpreter and JIT dispatch are checked separately. Replay/pixel tests are not
physical-device acceptance or a measured input-latency benchmark. The existing
lazy Coco/R text-colour service remains independent of this vector document model.
