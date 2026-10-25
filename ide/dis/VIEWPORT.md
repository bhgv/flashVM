# Channel-owned viewport and glyph hits

[Русский](VIEWPORT.ru.md) · [Viewport interface](bridge/ideviewport.m) ·
[Dis implementation](bridge/ideviewport.b) · [Controller](examples/graphical-editor/main.b) ·
[Native rendering/input](../../disVM/runtime/editor_view.c).

Ideviewport owns the visible Unicode interval in Dis. It scrolls by logical LF
lines, follows navigation/editing only when requested, and admits at most 4096
UTF-8 bytes without splitting a scalar. A long single line can select a suffix
around its caret. Scrolling never edits text, moves selection or changes history.
Native horizontal following is pixel geometry over the actual FlashVM layout,
not a second document-navigation implementation. Soft wrapping is disabled so
vertical movement and viewport scrolling share explicit logical-line units.

The graphical controller sends View/65540 before each full snapshot: flags is
content revision, first/second are source scalar boundaries, and the 24-byte
payload contains top logical line, total scalar count and monotonic view ticket
as three canonical hex words. The following snapshot and selection complete one
view. Tickets are not reused even when only scrolling changes; exhaustion fails
explicitly. Both snapshot and bounded interval are verified before TextField is
updated. The full source still belongs to Dis and is saved independently of the
currently displayed window.

The renderer retains the actual FlashVM glyph layout of the completed window.
Native mouse pixels are mapped to glyph-cluster insertion proposals and then to
source Unicode scalars. The proposed Place request (65542) contains content
revision, view ticket, scalar position and begin/update/end phase. Only Dis
validates bounds/current identity and chooses anchor/Shift/drag semantics.
Unstarted updates cannot invent a drag. Focus loss ends the gesture. Scroll
requests (65541) use the same identity and a bounded logical-line delta. Old
hits after scrolling are rejected even if the content revision stayed the same.

FlashVM exports unclipped contours, while its public raster calls bound input
coordinates. The renderer conservatively culls whole offscreen contours before
calling that raster API. Retained curves/control points are unchanged; this is
not destructive point clamping. Selection rectangles are clipped to the surface.
Horizontal following uses the TextField's actual clamped hscroll observation,
and hit-testing applies exactly the same translation.

Tests run actual Dis channels/controller and FlashVM raster/glyphs: Unicode drag
replacement, non-BMP offsets, wheel without cursor movement, stale viewport
rejection, page reveal, long lines and source tails. Independent Dis viewport
tests check line bounds, delta budgets and UTF-8 windows. Interpreter/JIT and
SDL dummy upload are checked separately. Physical mouse/display acceptance,
full bidi/grapheme editing, resize and syntax/completion widgets remain open.
