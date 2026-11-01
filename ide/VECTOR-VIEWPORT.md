[v0.1726: cubic model, updated fitting/picking / Безье и новые правила](../docs/releases/v0.1726.md)

# Brush splines and the independent page viewport

[Русский](VECTOR-VIEWPORT.ru.md) · [Object/node editing](VECTOR-EDITING.md)

After release, the separate Limbo worker fits closed brush contours with cubic
Bézier segments, using chord parameters and interval least squares. Every input
sample is checked against the rounded curve with per-axis extent/40 tolerances
based on the whole compound figure. Failed intervals subdivide; short intervals
retain lines. A bounded reverse sampling check rejects bows between knots.
This is a discrete fitting bound, not a continuous Hausdorff proof. Motion keeps
raw native ink. The owner replaces geometry only for a matching completion.

Nodes edits endpoints and both cubic controls independently, with Ctrl/marquee,
Undo/Cancel and SVG C round trips. Legacy Q paths remain editable. The algorithm
lives in `dis/bridge/idevectorbrushfit.b`; the linked release gives full limitations.

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
