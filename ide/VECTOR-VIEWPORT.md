[Latest-only fitting / Отмена устаревшей векторизации](VECTOR-CANCELLATION.md)

[v0.1726: cubic model, updated fitting/picking / Безье и новые правила](../docs/releases/v0.1726.md)

# Brush splines and the independent page viewport

[Русский](VECTOR-VIEWPORT.ru.md) · [Object/node editing](VECTOR-EDITING.md)

After release, the separate Limbo worker applies the stroke to the current paint.
It constructs dots and straight brush sweeps from four cubic arcs and preserves
an existing outer path when an interior cut can append a reverse-wound hole.
For a simple forward-moving stroke, the worker simplifies the sampled centerline
within one logical unit and joins its offsets with tangent cubic arcs. The vector
operation uses SVG nonzero winding to distinguish filled rings, holes and redundant
subpaths. It isolates transverse intersections by subdividing the Bézier control
hulls, retaining only the segments that bound the new fill. A union computes crossings
for all participating outlines before rounding to the editor's SVG grid. New crossings
are cusp nodes. Unchanged source anchors keep their join mode. A symmetric anchor
becomes smooth when one of its adjoining segments is shortened.

For shared straight or curved intervals, the operation compares the fill on
both sides. It proves partial coincidence by mapping source endpoints to the
other curve and comparing the restricted Bézier controls on a common monotone
axis. Collinear Q and C spans also take part and keep their command type on
surviving portions. A line meets a Bézier at roots of signed distance. When
the transverse solver cannot resolve two curves, the worker looks for a
separating normal using extrema of their scalar projections. It accepts a
point contact only when both support positions coincide. Local directions at
the contact keep the two fill faces independently editable.

For self-intersecting input, the worker finds events within the source along
with events between fills. At each surviving span it counts signed crossings
on a ray perpendicular to the span. The jump at the ray origin includes all
coincident source edges. Nonzero winding on both sides discards an internal
edge, while zero winding leaves an empty face even when surrounded by paths.
A loop within one cubic is split at its two visits to the crossing, with
further exact subdivision where needed. The quick interior punch also checks
that no source edge enters the brush's control box before appending a hole.

For loops, sharp turns, retracing and page edges, the worker sweeps a single
four-cubic nib along the simplified centerline. Each span contributes a strip
between the two tangential support points, and each distinct endpoint supplies
a full stamp. Overlapping collinear intervals merge before construction. The
worker unions the strips and stamps and intersects the result with the page
when needed, all before rounding. Subtracting an annular stroke preserves an
independently editable island of the underlying fill in its empty center.

Dense samples are simplified with an explicit stack, to at most 128 centerline
knots. The arrangement caches control hulls and sweeps the less crowded axis,
so only pairs whose bounds overlap in both dimensions reach curve intersection.
Each edge has its own contact list. The worker skips rings outside a winding
query and uses spatial buckets to keep vertex matching separate from SVG
rounding. Later cuts and unions accept long saved paths within the shared
limit of 1,200 prepared edges, including subdivisions where applicable.

Unresolved contacts and inputs outside the vector budget use the bounded mask.
The worker traces its coverage and fits the closed
contours with cubic Bézier segments using chord parameters and interval least
squares. Every sample is checked against the rounded curve with per-axis extent/100 tolerances
based on the whole compound figure. Failed intervals subdivide; short intervals
retain lines. A bounded reverse sampling check rejects bows between knots.
This is a discrete fitting bound, not a continuous Hausdorff proof. Motion keeps
raw native ink. The owner replaces geometry only for a matching completion.

Nodes edits endpoints and both cubic controls independently, with Ctrl/marquee,
Undo/Cancel and SVG C round trips. Legacy Q paths remain editable. The algorithm
lives in `dis/bridge/idevectorbrushfit.b`; the linked release gives full limitations.
Merge policy and topology live in `dis/bridge/idevectormergedrawing.b` and
`dis/bridge/idevectorpainttopology.b`. The sampled nib is in
`dis/bridge/idevectorbrushsweep.b`. Intersection isolation and contour assembly
are in `dis/bridge/idevectorcutintersections.b` and `dis/bridge/idevectorcutgraph.b`.
Compound input and winding are handled by `dis/bridge/idevectorcutoutline.b`.
Contact detection and path assembly are in `dis/bridge/idevectorcutcontacts.b`
and `dis/bridge/idevectorcutboundary.b`. The [contact tests](dis/tests/idepaint_contacts.b)
exercise partial seams, subtraction and line/curve and curve/curve tangency.
The partial seam and separating tangent checks are in
`dis/bridge/idevectorcutcoincidence.b` and `dis/bridge/idevectorcutsupport.b`.
Within-source winding and cubic subdivision are in `dis/bridge/idevectorcutwinding.b`
and `dis/bridge/idevectorcutspans.b`. The [winding tests](dis/tests/idepaint_winding.b)
cover crossed and repeated paths, opposite windings and a loop within one cubic.
General nib sweeps live in `dis/bridge/idevectorbrushunion.b`. The
[sweep tests](dis/tests/idepaint_sweep.b) exercise closed loops, reversal, oblique
crossing, clipping and a move that exposes the cut around a central island.
They also cover a 640-sample stroke and a long stepped ring that can be moved
and undone after cutting an imported cubic fill. Intersection indexes are in
`dis/bridge/idevectorcutindex.b`.

The magnifier button below the two selection arrows toggles viewport mode.
While active, wheel or two-finger pinch over the page changes magnification;
a left-button drag frames a page rectangle, then fits and centres it on release.
Double-click/double-tap the magnifier restores the initial 1:1 viewport camera.
Native SDL classifies that double gesture; Limbo owns the reset command.
Other tools can be used at the retained magnification. Exiting magnifier mode
restores the previous tool. Zoom is bounded to 25..1600 percent relative to the
page's initial fitted scale; 100% restores that baseline, not a physical-DPI claim.

Camera state and rubber-band preview never enter SVG/history or change source
revision. Owner policy lives in `idevectorviewport.b`. Painting, native live ink,
node picking and inverse pointer mapping use the same copied camera; the page is
clipped independently of the fixed toolbar. Controls/title text do not zoom with
the page. The default six-word wire header stays byte-identical. Nondefault
cameras use private tool bit0x10000 plus zoom/centreX/centreY words before geometry;
unknown bits, short headers and out-of-range camera values are refused. This is
not a shared SWF/Dis64 format extension. Matching native/Dis products must rebuild.

Checks cover relative fitting and individually movable controls, camera-only
transactions, wheel/pinch routing, fitted marquee, unchanged SVG, actual clipped
pixels and double reset in both dispatch modes. Native SDL checks double-tap
classification separately. Existing editing/ink/history checks remain active.
