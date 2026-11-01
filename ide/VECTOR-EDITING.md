[Dense picking and crash correction](VECTOR-PICKING.md)

# Object and contour editing

[Русский](VECTOR-EDITING.ru.md) · [Vector editor](VECTOR-EDITOR.md)

The first two buttons on the vector pane's left toolbar select **Objects** and
**Nodes**. Keyboard alternatives are V and N. Existing Brush, Pen, Rectangle,
Ellipse and connected Line tools follow below. Colours stay on the right.

Objects selects the topmost touched figure. A filled interior or a stroke can be
picked. Drag the figure to translate it; the four square bounding handles scale
it around the opposite corner. The handle above the frame rotates around the
figure centre. A figure is one existing Shape, regardless of whether it came
from Brush union, Pen, primitives or an imported compound path. Other Shapes
retain their identities and geometry. Empty-space click clears object selection.

Nodes selects one touched contour and shows its path, square endpoint handles
and quadratic control handles/guides. Click a handle to select it; Ctrl-click
adds/removes a handle. Drag a selected handle to move the selected group. Drag
empty canvas to select handles in a rectangular marquee; Ctrl preserves the
existing selection. Selecting another contour clears the old handle selection.
The current model uses quadratic curves; this does not invent cubic SVG support.

Every drag transforms an immutable pointer-down snapshot. Motion never compounds
rounding from the preceding motion. Release commits one Undo item and revision;
Escape/focus cancellation restores the exact snapshot. Out-of-canvas transforms
are refused as a whole. Geometry and quadratic verbs persist in SVG; selection,
frames and handles do not. Save/Open and Undo/Redo retain existing owner channels.
Pending ink fitting completes before an edit gesture is admitted.

The algorithm lives in `dis/bridge/idevectoredit.b`, separately from drawing,
serialization, native capture and window composition. Selection and its mask are
owned by the pane. Render workers receive only immutable serialized geometry;
editing tools bypass the native brush/primitive sample capture route.

The presentation overlay has a 4096-command budget. Very dense contours display
an initial bounded set of handles; the complete source contour remains editable
and marquee selection considers all its nodes. This first editing implementation
has no insert/delete/join/split node commands or multi-object selection.

Desktop scaling now marks old and new extents when a node is resized. Changes to
the linear affine transform recompose the root once from cached local images;
ordinary translation retains old/new damaged-tile composition. This avoids old
control/card strips surviving a scale change without repainting each bitmap.

Checks exercise immutable move, corner scale, rotation, single/group nodes,
marquee, quadratic control points, Undo/Cancel and real toolbar/mouse/SVG routing
with both interpreter and JIT. Retained raster checks compare resized/zoomed
incremental output byte-for-byte against a fresh render.
