# Dis-admitted editor canvas resizing

[Русский](RESIZE.ru.md) · [Host boundary](bridge/idehost.m) ·
[Controller](examples/graphical-editor/main.b) ·
[Actual surface/input tests](../../disVM/runtime/test_graphical_resize.py).

The SDL window is resizable. Native size events propose copied pixel dimensions;
Dis admits width 240..2048 and height 160..2048. A surface is at most 16 MiB RGBA,
with fresh allocation/texture staged before retiring the previous storage. The
TextFields, capture dimensions, upload stride and hit/footer bounds follow the
admitted canvas. GUI operations stay on the native owner thread; document,
selection, page and viewport policy stay on Dis. No native address crosses a VM.

Pointer/65552 carries proposed width/height in first/second. The native owner
measures row capacity at the current font and proposed height without changing
the current view, then emits Request/65552: flags=revision, first/second=pixels,
text=two canonical eight-digit words (completed-view ticket, measured rows).
Dis rejects stale/malformed identity or dimensions/rows outside bounds. Accepted
resize resets drag state, updates page rows and reveals the current caret.
Source text, revision, saved state, history and modal field values do not change.

Each completed view now begins Canvas/65553 (admitted pixels), then Style/65548
(font/rows), Panel, Viewport, snapshot fragments and selection. Renderer checks
canvas/style/panel/viewport/content revision consistency and verifies the actual
font row count. A forged in-range measurement that disagrees with the renderer
is a terminal protocol error, not a successful view. Allocation/font/SDL errors
are visible failures. This is a matched experimental host/controller extension;
ship/rebuild both together. The fvd1 envelope and Dis/heap ABI are unchanged.

Ctrl+wheel/pinch measures against the current height; PageUp/Down uses the
admitted row count. Hit coordinates remain native canvas pixels converted to
twips at the input boundary. Text and caret keep the shared leading/kerning/tab
contract described in [CARET](CARET.md). The pinned footer is excluded from
source hit testing at every size.

Real tests shrink/grow through the minimum and maximum extents, inspect actual
PPM allocation dimensions, compare caret with independently scanned ink, derive
mouse hits from that ink, check page/reveal behavior and preserve a Find field
through resize/zoom. Stale/malformed/negative/oversize proposals do not allocate.
Both Dis dispatch modes and optional real SDL texture uploads with the dummy
video driver pass. Dummy/replay acceptance does not certify physical window
managers, high-DPI mapping or Windows/macOS native runtime ports.
