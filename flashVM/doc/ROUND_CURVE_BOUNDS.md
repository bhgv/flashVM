# Analytic round Bezier stroke bounds

Current monotone axis commands: [contract](AXIS_CURVE_BOUNDS.md). Earlier blanket non-round curve rejection below now has this exception.

The private [fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L494) now accepts quadratic and cubic contours with round/round styles. It measures analytic centerline extrema of each nonconstant segment and expands the combined box by half the effective width. For an entirely round stroke this is the radius-neighborhood bound of the centerline, subject to ordinary double arithmetic. It is geometric, not the antialias pixel rectangle or a certification of complete Flash equivalence.

Move only changes the contour origin and contributes no stroke. Constant line/quadratic/cubic segments contribute no extent; coincident curve endpoints do not make a curve constant when a control point differs. Close includes its actual closing segment. Multiple contours are unioned without adding intervening moves. Empty geometry returns zero bounds and has_segments=0.

## Contract and ownership

[fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L494) borrows an immutable path, accepts a finite positive width up to 1,000,000 destination units, valid cap/join selectors, and two required caller-owned output pointers. It allocates nothing, executes no callbacks or GC and needs no context. Errors preserve both outputs. Invalid selectors/widths/pointers return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Curves with butt/square caps or bevel joins still return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) without publishing outputs. Concurrent path mutation is forbidden.

Derivative roots use a [shared internal math module](../src/render/curve_bounds.hpp) extracted from the existing [fvm_path_bounds](../src/capi/path_c.cpp#L195). The public function retains its formulas and existing inclusion of isolated Moves. The new round walker deliberately excludes moves without strokes. Zero authored width still becomes one destination unit in managed traversal; positive width keeps the existing maximum-axis transform metric. A complete positive-width collapse creates no stroke.

## MovieClip and remaining limits

The existing MovieClip bounds traversal automatically consumes the new quadratic round/round result: getBounds, dimensions and non-shape point hitTest no longer include distant isolated moveTo commands in curve-only stroke bounds. A filled paint still retains the underlying path box; mixed paint unions it with the stroke. getRect retains its previous geometry. Managed path conversion and snapshots still allocate and use the existing native GC barrier.

Cubic is available to the native path API; managed MovieClip intentionally rejects cubic commands. This does not add cubic AS2/SWF semantics. Shape point hits and rasterization are unchanged and retain their own curve subdivision. Butt/square/bevel curves retain their conservative fallback. Exact non-round offset bounds, nonuniform/shear metrics, full reference rasterization, miter/hinting/non-scaling, Cairo and physical devices remain open. Public C89 ABI, callback IDs and wire formats do not change. All nine roadmap stages remain open.

The executable C89 [stroke_styles tutorial](../examples/embedding/stroke_styles/README.md) is extended in this series with a separate round curve and explicit replacement of its result GC root.

## Verified checkpoint v0.1358

Series v0.1338–v0.1358 on published v0.1337 contains 21 local commits. Validation passes 275 selected native AGG/JIT regressions plus the separate existing C89 vector_paths regression (276 total), 22 no-graphics tests and 22 ASan/UBSan tests, including the final C89 tutorial. LeakSanitizer is disabled. Eighteen new fixtures cover native quadratic/cubic, returning and constant curves, separate contours, rotation/scale, coordinate limits, transactional outputs, managed quadratic and deliberate cubic rejection, mixed paint, dimensions, bounding hitTest, four qualities/four paints, snapshots, four graphs through binary and channel after sender teardown, and four OS workers with 96 bounds/GC epochs. Existing native tests include JIT, timeline, channels and allocation failures; this is not a separate full reference acceptance of the new curves in each subsystem. Source links, examples and formatting checks pass. Cairo and physical platforms were not tested; all nine stages remain open.

| Fixture | Coverage |
| --- | --- |
| [path_round_curve_quadratic](../tests/path_round_curve_quadratic.cpp) | Check analytic quadratic round extents against centerline bounds |
| [path_round_curve_cubic](../tests/path_round_curve_cubic.cpp) | Verify symmetric and two-extremum cubic round bounds |
| [path_round_curve_reversal](../tests/path_round_curve_reversal.cpp) | Retain nonconstant returning curves despite coincident endpoints |
| [path_round_curve_degenerate](../tests/path_round_curve_degenerate.cpp) | Exclude constant quadratic and cubic segments from stroke extents |
| [path_round_curve_contours](../tests/path_round_curve_contours.cpp) | Union active curve contours without including isolated moves |
| [path_round_curve_rotation](../tests/path_round_curve_rotation.cpp) | Measure analytic round curves after uniform rotation and scaling |
| [path_round_curve_limits](../tests/path_round_curve_limits.cpp) | Exercise round curve bounds at coordinate and width limits |
| [path_round_curve_outputs](../tests/path_round_curve_outputs.cpp) | Preserve both output sentinels on unsupported and invalid curve queries |
| [movieclip_round_curve_isolated](../tests/movieclip_round_curve_isolated.cpp) | Exclude distant moveTo commands from managed round curve paint bounds |
| [movieclip_round_curve_degenerate](../tests/movieclip_round_curve_degenerate.cpp) | Suppress constant managed curves while preserving getRect geometry |
| [movieclip_round_curve_cubic](../tests/movieclip_round_curve_cubic.cpp) | Reject managed cubic metadata and recover a valid quadratic path |
| [movieclip_round_curve_mixed](../tests/movieclip_round_curve_mixed.cpp) | Union filled path geometry with analytic round curve stroke bounds |
| [movieclip_round_curve_dimensions](../tests/movieclip_round_curve_dimensions.cpp) | Read hairline curve dimensions without isolated move inflation |
| [movieclip_round_curve_hitbox](../tests/movieclip_round_curve_hitbox.cpp) | Apply analytic curve boxes to non-shape point hit queries |
| [movieclip_round_curve_quality](../tests/movieclip_round_curve_quality.cpp) | Keep analytic curve bounds stable across stage quality and paints |
| [movieclip_round_curve_snapshots](../tests/movieclip_round_curve_snapshots.cpp) | Retain curve style snapshots across later styles clear and collection |
| [movieclip_round_curve_transfer](../tests/movieclip_round_curve_transfer.cpp) | Preserve round curve bounds through binary and channel sender teardown |
| [movieclip_round_curve_threads](../tests/movieclip_round_curve_threads.cpp) | Measure and collect round curve graphs on isolated worker runtimes |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_path_bounds` | function | [include/flashvm/path.h:71](../include/flashvm/path.h#L71) |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:494](../src/capi/path_hit_c.cpp#L494) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
