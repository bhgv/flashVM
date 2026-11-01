# Brush splines and the independent page viewport

[Русский](VECTOR-VIEWPORT.ru.md) · [Object/node editing](VECTOR-EDITING.md)

After release, the existing Limbo fitting worker compares dense contour samples
against the complete filled figure's width/40 and height/40. Both coordinate
differences must be strictly below their respective threshold. Run span is also
bounded so a long chain of short edges cannot collapse the whole contour.
A dense run is replaced by a two-part quadratic spline with two editable controls.
Its start, middle knot and end are interpolated; other nearby samples are fitted,
not claimed to be interpolated exactly. Both pieces share a tangent at the middle
knot unless a control must be clamped to the canvas boundary. Open drawing samples
are still native/raw; fitting never runs in the mouse-motion handler.

The Nodes arrow selects the spline endpoints and both controls. Nearest-handle
picking disambiguates densely spaced handles. Existing Ctrl/marquee group editing,
Undo/Cancel and SVG Q commands apply; no cubic SVG command is invented.
The algorithm is separate in `dis/bridge/idevectorbrushfit.b`.

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
