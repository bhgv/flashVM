# Cap and join aware shape hit testing

Selected straight-stroke bounds are now described in [STROKE_BOUNDS.md](STROKE_BOUNDS.md). Exact curves and nonuniform/shear metrics remain open; this earlier checkpoint does not certify them.

The point form of MovieClip.hitTest with a true shape flag now respects the
symmetric round, butt and square caps and round/bevel joins already restored
from SWF 8 DefineShape4 LINESTYLE2. It queries geometry rather than raster alpha:
solid, linear/focal gradient and bitmap strokes share the same coverage test.
Transparent paint and bitmap texels do not turn shape testing into a pixel-alpha
query. Bounds/object hit testing is unchanged.

## Geometry contract

The private [fvm_path_hit_test_style](../src/capi/path_hit_c.cpp#L421) uses destination-space coordinates and a
positive stroke width. Each nonzero straight segment contributes its finite
rectangular body. Butt caps add no extension, square caps extend by half the
width, and round caps add circular endpoints. Joins add only the outside bevel
triangle or circular sector. Closed contours join the final and first tangents;
open contours, including NoClose paths returning to their start, retain caps.
Moves finish the preceding contour. Zero-length segments do not create a tangent
or a stroked dot. Boundaries are included.

Quadratic/cubic curves retain adaptive chord subdivision with 0.0001 coordinate
unit flatness and depth limit 20. Selected joins connect these chords. This is
bounded geometry approximation, not a proof of exact Flash curve coverage at
every boundary, cusp or extreme scale. The private query allocates no temporary
geometry; managed path construction and graph copies can still allocate.

Width zero delegates to the existing fill algorithm and its even-odd/nonzero
rule. The public C89 [fvm_path_hit_test](../include/flashvm/path.h#L75) retains its legacy round-distance behavior,
including its degenerate segment policy. Public ABI and wire formats do not
change. Private selectors outside their supported ranges and invalid coordinates,
width, fill rule or output pointer return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before output is
written. Valid coordinates remain bounded to +/-2,000,000 and widths to 1,000,000.

## Managed metadata and lifetime

The shared stroke_selectors reads own lineCap/lineJoin numeric integral scalars
without invoking getters. Missing fields mean round defaults; fractions, NaN,
infinity, wrong types and unsupported selector values are invalid. Both drawing
and shape testing use this validator. A fill hit can short-circuit the stroke
query, as before. The hit method validates restored metadata only for strokes it
actually queries.

Native call barriers and copied display/paint edges retain the queried graph.
Binary/channel reconstruction needs no new callback IDs or native resource;
receivers query after sender teardown and collection. Timeline character
replacement and repeated seeks use the currently restored selectors. Stage
quality changes antialiasing during rendering, not geometric hit testing.
Independent worker contexts share no query state.

## Validation and remaining acceptance

The regression fixtures cover independent straight-line equations, analytic
corner probes, multiple/open/closed contours, degenerate edges, quadratic/cubic
curves, legacy round equivalence, invalid arguments, four SWF paint families,
hairline and UI16 width boundaries, restored metadata, uniform rotations and
reflections, stage quality, transfer, seek, isolated OS workers and managed
allocation failures. See the final checkpoint for measured build results.

Miter/asymmetric caps, non-scaling strokes, pixel hinting, exact stroke bounds,
Flash nonuniform/shear stroke metrics, a full independent reference-player
corpus, Cairo and physical-device acceptance remain open. The current managed
hit width still uses its existing maximum-axis scale rule, while drawing uses
the existing X-axis rule. This series does not resolve that earlier difference.
All nine roadmap stages and full SWF <= 8 compatibility remain open.

Build from a clean CMake directory when comparing private core/test interfaces.
Run tests with assertions enabled as configured by the project.

## Fixture index

| Fixture | Scope |
| --- | --- |
| [path_style_hit_caps](../tests/path_style_hit_caps.cpp) | Analytic endpoint extents |
| [path_style_hit_turns](../tests/path_style_hit_turns.cpp) | Outer corners, reversals and short edges |
| [path_style_hit_contours](../tests/path_style_hit_contours.cpp) | Open/closed/multiple and degenerate contours |
| [path_style_hit_curves](../tests/path_style_hit_curves.cpp) | Quadratic/cubic endpoints and interiors |
| [path_style_hit_errors](../tests/path_style_hit_errors.cpp) | Invalid arguments preserve output; fill parity |
| [path_style_hit_oracle](../tests/path_style_hit_oracle.cpp) | Independent dense straight-line equations |
| [path_style_hit_round](../tests/path_style_hit_round.cpp) | Legacy round equivalence on ordinary contours |
| [swf_stroke_hit](../tests/swf_stroke_hit.cpp) | 24 restored paint/cap/join combinations and GC |
| [swf_stroke_hit_metadata](../tests/swf_stroke_hit_metadata.cpp) | Invalid restored scalars and absent round defaults |
| [swf_stroke_hit_widths](../tests/swf_stroke_hit_widths.cpp) | 120 paint/style/width cases including hairlines |
| [swf_stroke_hit_transforms](../tests/swf_stroke_hit_transforms.cpp) | 96 uniform scale/rotation/reflection configurations |
| [swf_stroke_hit_quality](../tests/swf_stroke_hit_quality.cpp) | 96 paint/style/quality configurations |
| [swf_stroke_hit_transfer](../tests/swf_stroke_hit_transfer.cpp) | 24 graphs through binary and channel after sender teardown |
| [swf_stroke_hit_timeline](../tests/swf_stroke_hit_timeline.cpp) | Character replacement and sixteen alternating seeks |
| [swf_stroke_hit_threads](../tests/swf_stroke_hit_threads.cpp) | Four workers, 96 independent query/GC epochs |
| [swf_stroke_hit_allocation](../tests/swf_stroke_hit_allocation.cpp) | 24 native managed-query allocation-fault sweeps |


## Checkpoint v0.1284

Cap/join-aware shape hits v0.1267–v0.1284: MovieClip point hitTest with shape=true now queries finite stroke bodies, selected round/butt/square ends and round/bevel outer joins for restored solid, linear/focal-gradient and bitmap paints. One own-scalar validator serves drawing and querying; absent selectors remain round. Curves retain bounded adaptive chords. Sixteen new fixtures cover analytic equations, short-edge outer sectors, contour topology, curve queries, legacy round equivalence, argument/metadata rejection, 24 SWF paint/style combinations, 120 width cases, 96 uniform transformations, 96 quality cases, 24 graphs through binary and channel after sender destruction, replacement and sixteen seeks, four workers with 96 query/GC epochs and 24 managed allocation-fault sweeps. Validation passes 187 selected native regressions plus the separate legacy MovieClip shape-hit regression, all 16 fixtures in a fresh SDK-free no-graphics interpreter build, 15 ASan/UBSan fixtures and allocation with explicit --no-faults. Real allocation failures pass natively; sanitizer unwinding remains limited and LeakSanitizer is disabled. Public C89 ABI and serialization formats are unchanged. Exact bounds, nonuniform/shear width metrics, unsupported LINESTYLE2 options, independent full Flash reference acceptance, Cairo and physical platforms remain open. All nine stages remain open. [Contract](STROKE_HIT_TEST.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_path_hit_test` | function | [include/flashvm/path.h:75](../include/flashvm/path.h#L75) |
| `fvm_path_hit_test_style` | function | [src/capi/path_hit_c.cpp:421](../src/capi/path_hit_c.cpp#L421) |
<!-- source-links:end -->
