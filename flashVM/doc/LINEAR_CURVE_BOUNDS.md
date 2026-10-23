# Certified linear Bezier stroke bounds

Current extension: [adaptive vectors](ADAPTIVE_CURVE_BOUNDS.md). The former absolute-lattice requirement below is historical.

Exact bounds now use segment geometry for quadratic/cubic commands with collinear controls inside the endpoint rectangle. For non-axis paths every coordinate must lie exactly on a 1/16 lattice within the original ±1,000,000 domain; FLT_RADIX must be 2 and DBL_MANT_DIG at least 53. Multiplication by 16 produces exact integers, differences are at most 32,000,000, products stay below 2^50 and cross differences below 2^51. Thus the cross-product test cannot round a nonzero deviation to zero. There is no epsilon. Public path admission is unchanged; its coordinate limit is shared through the [private header](../src/capi/path_geometry_constants.h).

Axis paths retain arbitrary admitted finite coordinates without a lattice requirement. Controls must lie inside the endpoint interval, but cubic controls may now be unordered. For normalized u,v in [0,1], the cubic coordinate derivative is affine in u,v; its four control-square vertices are proportional to t², 2t(1-t), (1-2t)² and (1-t)². All are nonnegative, so this entire class is monotone. Controls at opposite endpoints can produce an admitted stationary midpoint.

## Contract

[fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L503) retains its contract: an immutable borrowed path, a positive finite width up to 1,000,000, supported cap/join selectors and two required caller-owned outputs. Admission and traversal allocate nothing, invoke no callbacks/GC and need no context. Argument errors and unsupported curves preserve both outputs. Concurrent path mutation is forbidden. Existing finite bodies, round/none/square caps, round/bevel joins and Close geometry are reused. Constants and isolated Moves contribute no stroke.

Non-axis off-grid coordinates, noncollinear controls and controls outside the endpoint interval are not reduced; non-round styles retain [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) and the managed fallback. Round/round retains general analytic curves. Transforms can move a path off the lattice. The existing nonuniform width metric is unchanged.

Managed quadratic paths automatically share the new bounds through getBounds, dimensions and non-shape point hitTest. getRect and mixed fill paints retain the underlying path-box policy. Cubic remains native-only; MovieClip still rejects it. Pixels, shape-hit subdivision, C89 ABI, wire formats and callback IDs are unchanged. The [C89 tutorial](../examples/embedding/stroke_styles/README.md) adds a real diagonal curveTo with none/bevel and an ignored isolated moveTo.

General non-round curves, exact arbitrary off-grid collinearity, nonuniform/shear metrics, miter/hinting/non-scaling, full Flash reference acceptance, Cairo and devices remain open. All nine roadmap stages remain open.

## Verified checkpoint v0.1400

21 local commits v0.1380–v0.1400 on published master v0.1379. Validation passes 307 selected native AGG/JIT tests, 61 no-graphics interpreter tests and 61 ASan/UBSan tests, including the final C89 tutorial and vector_paths. LeakSanitizer is disabled. Eighteen new fixtures cover quadratic/cubic, six cap/join styles, direction, unordered axis controls, fractional grid, rotation/scale, coordinate/width limits, three failures preserving both outputs, contours, joins/Close, managed geometry/hairline/mixed/dimensions/hitTest, 96 quality/paint/style combinations, 24 graphs through binary/channel after sender teardown, and four OS workers with 96 bounds/GC epochs. Existing tests retain JIT, timeline and allocation regressions without implying complete new reference acceptance of each subsystem. Source/example/diff checks pass. Cairo and devices were not tested; all nine stages remain open.

| Fixture | Coverage |
| --- | --- |
| [path_linear_curve_quadratic](../tests/path_linear_curve_quadratic.cpp) | Measure arbitrary-direction quadratic segment caps and joins |
| [path_linear_curve_cubic](../tests/path_linear_curve_cubic.cpp) | Measure unordered collinear cubic segment caps and joins |
| [path_linear_curve_reverse](../tests/path_linear_curve_reverse.cpp) | Retain linear Bezier extents in reverse traversal |
| [path_linear_curve_unordered_axis](../tests/path_linear_curve_unordered_axis.cpp) | Accept stationary-midpoint axis cubics with opposite endpoint controls |
| [path_linear_curve_grid](../tests/path_linear_curve_grid.cpp) | Certify translated fractional dyadic lattice coordinates |
| [path_linear_curve_rotation](../tests/path_linear_curve_rotation.cpp) | Measure rotated and uniformly scaled linear curves |
| [path_linear_curve_limits](../tests/path_linear_curve_limits.cpp) | Exercise exact collinearity at path and stroke limits |
| [path_linear_curve_errors](../tests/path_linear_curve_errors.cpp) | Preserve outputs on noncollinear off-grid and out-of-interval controls |
| [path_linear_curve_contours](../tests/path_linear_curve_contours.cpp) | Exclude isolated contours from linear Bezier bounds |
| [path_linear_curve_joins](../tests/path_linear_curve_joins.cpp) | Compose linear curve joins and closing edges as segment geometry |
| [movieclip_linear_curve_geometry](../tests/movieclip_linear_curve_geometry.cpp) | Use certified linear curve bounds through managed geometry queries |
| [movieclip_linear_curve_hairline](../tests/movieclip_linear_curve_hairline.cpp) | Measure one-unit linear curve hairlines |
| [movieclip_linear_curve_mixed](../tests/movieclip_linear_curve_mixed.cpp) | Union filled path boxes with certified linear strokes |
| [movieclip_linear_curve_dimensions](../tests/movieclip_linear_curve_dimensions.cpp) | Expose tight linear curve width and height observers |
| [movieclip_linear_curve_hitbox](../tests/movieclip_linear_curve_hitbox.cpp) | Apply tight linear curve boxes to non-shape point hits |
| [movieclip_linear_curve_quality](../tests/movieclip_linear_curve_quality.cpp) | Retain linear bounds across paints and stage qualities |
| [movieclip_linear_curve_transfer](../tests/movieclip_linear_curve_transfer.cpp) | Retain linear curves after binary and channel sender teardown |
| [movieclip_linear_curve_threads](../tests/movieclip_linear_curve_threads.cpp) | Measure and collect linear curves on isolated worker runtimes |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:503](../src/capi/path_hit_c.cpp#L503) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
