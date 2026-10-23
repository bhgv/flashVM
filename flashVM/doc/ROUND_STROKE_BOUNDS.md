# Round stroke bounds and hairlines

Current round/round curves: [analytic bounds](ROUND_CURVE_BOUNDS.md). Earlier blanket curve fallback descriptions below are historical for the entirely round style.

The managed bounds traversal now sends every straight stroke through the existing private cap/join walker, including absent selectors (round caps and round joins). This removes the separate legacy round rectangle policy. An isolated move does not enlarge a stroke-only extent; a path containing only zero-length edges contributes no stroke extent. A filled paint still retains its underlying path bounds and unions any nonempty stroke.

Zero authored width uses one destination unit for both straight bounds and the conservative curve envelope. Positive width uses the existing maximum-axis transform metric; complete positive-width collapse contributes no stroke extent. getBounds, MovieClip dimensions and non-shape point hitTest share this traversal. getRect retains the underlying path geometry. These are geometric bounds, not antialias pixel extents.

The implementation is in [bounds](../src/standard/movieclip.cpp); straight geometry uses [fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L481). The latter remains allocation-free, context-free and transactional, with caller-owned outputs and an immutable borrowed path. Managed traversal still allocates snapshots and converted paths, uses its existing native GC barrier and propagates allocation errors. No API, serialization field or callback ID changes.

Quadratic/cubic paths continue to use the path bounding box expanded by half the effective width, multiplied by sqrt(2) for square caps. This is deliberately conservative, not an exact offset-curve bound. Existing nonuniform/shear width metric differences, raster/reference acceptance, unsupported LINESTYLE2 styles, Cairo and physical devices remain open. All nine roadmap stages remain open.

## Regression coverage

Ten focused host fixtures exercise SWF6/7/8 hairlines, diagonal round ends, isolated moves, degenerate paths, contours returning to their start, curved envelopes, dimensions, non-shape hits, all four quality policies and 32 GC epochs. Existing SWF width, transform and mixed-fill fixtures now include the corrected default round expectations. The existing executable [C89 stroke tutorial](../examples/embedding/stroke_styles/README.md) remains applicable; its selected square-cap geometry is unchanged.
The straight-stroke walker accepts positive effective widths up to 1,000,000 destination units; larger values return an argument error. Concurrent mutation of one context or path is not supported.


| Fixture | Coverage |
| --- | --- |
| [hairline](../tests/movieclip_round_bounds_hairline.cpp) | SWF6/7/8, bounds versus getRect |
| [diagonal](../tests/movieclip_round_bounds_diagonal.cpp) | Independent diagonal cap extents |
| [isolated_moves](../tests/movieclip_round_bounds_isolated_moves.cpp) | Stroke-only extents exclude an isolated move |
| [degenerate](../tests/movieclip_round_bounds_degenerate.cpp) | Zero-length edges versus underlying path |
| [closed](../tests/movieclip_round_bounds_closed.cpp) | Round contour returning to its start |
| [curves](../tests/movieclip_round_bounds_curves.cpp) | Conservative nonzero hairline envelope |
| [dimensions](../tests/movieclip_round_bounds_dimensions.cpp) | Width and height observers |
| [hitbox](../tests/movieclip_round_bounds_hitbox.cpp) | Non-shape point hit at hairline reach |
| [quality](../tests/movieclip_round_bounds_quality.cpp) | Four stage policies |
| [gc](../tests/movieclip_round_bounds_gc.cpp) | 32 explicit collection epochs |


## Verified checkpoint v0.1337

Series v0.1325–v0.1337 on published v0.1324 contains 13 local commits. Validation passes 257 selected native AGG/JIT tests, 15 no-graphics interpreter tests and 15 ASan/UBSan tests, including the C89 tutorial. LeakSanitizer is disabled. New fixtures exercise host-context geometry; hairlines additionally run in loaded SWF6/7/8 contexts. The native set retains existing JIT/transfer/thread/timeline/allocation regressions, without implying a new complete reference acceptance pass. Source/example checks pass. Cairo and physical platforms were not tested.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:481](../src/capi/path_hit_c.cpp#L481) |
<!-- source-links:end -->
