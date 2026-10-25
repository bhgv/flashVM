# Bundled Noto and channel-owned font scaling

[Русский](ZOOM.ru.md) · [Asset/license](../assets/fonts/README.md) ·
[Dis controller](examples/graphical-editor/main.b) ·
[Host values](bridge/idehost.m) · [Native geometry](../../disVM/runtime/editor_view.c) ·
[Real regression](../../disVM/runtime/test_graphical_zoom.py).

The editor starts with bundled OFL Noto Sans Mono at 11 pixels. Neither the build
menu nor launcher asks where to find a font. The default resource is resolved
beside the entry module, so relocated packages need no source checkout or system
font lookup. Explicit `--font`/`FVM_EDITOR_FONT` is an optional override only.

Ctrl+wheel and a two-finger pinch propose one +/-1 size step. Native code measures
actual row capacity from the proposed face size without mutating the current
view. Dis checks content revision and current view ticket, clamps size to 6..48,
admits rows in 1..128 and publishes Style/65548 before its viewport/snapshot.
The renderer applies the admitted size to both TextFields and the actual glyph
hit face and verifies the row metric before accepting the complete view. Page
movement and logical viewport rows change together. No text, history or saved
revision changes when zooming, including inside an active search/path field.

The raw input extension Pointer/65550 carries a normalized +/-1 pinch step.
Zoom/65547 requests contain the existing revision/ticket binding and measured
row count. SDL_MULTIGESTURE handles two-or-more-finger normalized distance changes
without an Android-only guard; Ctrl+wheel uses the same pipeline. Finger changes
and focus loss reset gesture accumulation. Synthesized touch mouse events are
ignored to avoid duplicate drag/zoom effects. This SDL adapter can serve desktop
Windows/macOS/Linux touch-capable frontends; physical device behavior and full
Dis runtime ports on those systems remain separate acceptance work. The current
native runtime acceptance remains Linux AMD64.

The public envelope's first/second values are signed int32. The wider NC64 cell
must sign-extend those decoded fields rather than treating -1 as 4294967295.
The codec now does so explicitly and rejects wider values before encoding. This
also fixes upward wheel scrolling; unsigned operation/flag fields retain their
32-bit bit-pattern contract. The wire header/byte layout did not change.

Tests use the bundled font with no override, verify its pinned digest/OFL notice,
compare actual raster changes/equivalence for wheel and pinch, exercise 6/48
bounds and actual row counts, reject stale/malformed proposals and preserve
prompt/document state. Independent real Dis codec tests exercise all signed
boundary pairs, Unicode and NC64 width rejection. Physical touchscreen tests are
not replaced by replay or SDL dummy upload.
