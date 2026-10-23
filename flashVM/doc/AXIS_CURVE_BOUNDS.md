# Exact bounds for monotone axis Bezier commands

Admission is implemented by [axis_curve_segment](../src/capi/path_hit_c.cpp#L236).

The private [fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L432) accepts quadratic/cubic commands with every supported cap/join when controls progress from start to endpoint on one horizontal or vertical axis. Such a curve describes exactly the endpoint segment: its changing-axis derivative has one sign. Admission uses exact double comparisons without epsilon and creates no approximate polyline. Controls at either endpoint are accepted, including zero endpoint derivatives. A completely constant command contributes no extent.

The segment uses the existing finite-body, cap and join walker. Round/none/square caps and round/bevel joins retain their meanings; adjacent segments and Close use the same normals. Moves without strokes add no extent. Returning, non-axis or unordered controls do not qualify, even where a more general analysis could prove monotonicity. For non-round styles these still return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) while preserving both outputs; managed traversal uses its previous conservative fallback. Round/round retains its general analytic curve support.

## Contract and integration

[fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L432) borrows an immutable path, a positive finite width up to 1,000,000 destination units, selectors and two required caller-owned outputs. There is no allocation, context, callback or GC. Argument errors and unsupported geometry preserve both outputs. Concurrent path mutation is forbidden. Limits live in [path_geometry_constants.h](../src/capi/path_geometry_constants.h). The [internal walker](../src/capi/path_hit_c.cpp) publishes only after the entire path succeeds.

Managed quadratic paths automatically share the precise box through getBounds, MovieClip dimensions and non-shape point hitTest. Mixed fill/stroke paints retain the union with the underlying path box; getRect is unchanged. Hairlines use effective width one; positive widths keep the maximum-axis transform metric. Cubic remains a native path capability; MovieClip still rejects cubic commands. Rasterization and shape-hit subdivision are unchanged. Public C89 ABI, wire formats and callback IDs stay unchanged.

Arbitrary rotations can move an axis path outside the admitted class. General non-round curves, nonuniform/shear metrics, miter/hinting/non-scaling, full Flash reference acceptance, Cairo and physical devices remain open. All nine roadmap stages remain open. The executable [C89 tutorial](../examples/embedding/stroke_styles/README.md) demonstrates actual curveTo(8,8,14,8), none/bevel and an ignored isolated moveTo.

## Verified checkpoint v0.1379

Series v0.1359–v0.1379 on published master v0.1358 contains 21 local commits. Validation passes 289 selected native AGG/JIT tests, 43 no-graphics interpreter tests and 43 ASan/UBSan tests, including the updated C89 tutorial and existing vector_paths. LeakSanitizer is disabled. Eighteen new fixtures cover two curve degrees, six styles, both axes/directions, endpoint controls, constants, contours, joins/Close, four unsupported cases preserving both outputs, five widths, managed getBounds/getRect/hairline/mixed/dimensions/hitTest, 96 quality/paint/style combinations, 24 graphs through binary and channel after sender teardown, and four OS workers with 96 bounds/GC epochs. Existing native tests include JIT, timeline and allocation failures; these are regressions rather than complete new reference acceptance for every subsystem. Source/example/diff checks pass. Cairo and physical platforms were not tested; all nine stages remain open.

| Fixture | Coverage |
| --- | --- |
| [path_axis_curve_quadratic](../tests/path_axis_curve_quadratic.cpp) | Measure all supported quadratic axis stroke caps and joins |
| [path_axis_curve_cubic](../tests/path_axis_curve_cubic.cpp) | Measure all supported cubic axis stroke caps and joins |
| [path_axis_curve_vertical](../tests/path_axis_curve_vertical.cpp) | Apply exact stroke geometry to vertical axis curves |
| [path_axis_curve_reverse](../tests/path_axis_curve_reverse.cpp) | Retain axis stroke extents for both traversal directions |
| [path_axis_curve_endpoint_controls](../tests/path_axis_curve_endpoint_controls.cpp) | Accept endpoint controls and vanishing endpoint derivatives |
| [path_axis_curve_constant](../tests/path_axis_curve_constant.cpp) | Suppress constant axis Bezier stroke extents |
| [path_axis_curve_contours](../tests/path_axis_curve_contours.cpp) | Exclude isolated moves and constant secondary contours |
| [path_axis_curve_joins](../tests/path_axis_curve_joins.cpp) | Compose axis Bezier joins and closing segments like straight geometry |
| [path_axis_curve_errors](../tests/path_axis_curve_errors.cpp) | Preserve output sentinels on non-axis reversing and unordered controls |
| [path_axis_curve_widths](../tests/path_axis_curve_widths.cpp) | Measure axis stroke bodies at finite fractional and maximum widths |
| [movieclip_axis_curve_geometry](../tests/movieclip_axis_curve_geometry.cpp) | Use exact axis curve bounds without isolated move inflation |
| [movieclip_axis_curve_hairline](../tests/movieclip_axis_curve_hairline.cpp) | Measure effective one-unit axis curve hairlines |
| [movieclip_axis_curve_mixed](../tests/movieclip_axis_curve_mixed.cpp) | Union filled path boxes with exact axis stroke extents |
| [movieclip_axis_curve_dimensions](../tests/movieclip_axis_curve_dimensions.cpp) | Read scaled axis stroke dimensions through MovieClip observers |
| [movieclip_axis_curve_hitbox](../tests/movieclip_axis_curve_hitbox.cpp) | Use axis stroke boxes for non-shape point hit queries |
| [movieclip_axis_curve_quality](../tests/movieclip_axis_curve_quality.cpp) | Retain axis bounds across paint kinds and stage quality policies |
| [movieclip_axis_curve_transfer](../tests/movieclip_axis_curve_transfer.cpp) | Preserve axis curves through binary and channel sender teardown |
| [movieclip_axis_curve_threads](../tests/movieclip_axis_curve_threads.cpp) | Measure and collect axis curves on isolated worker runtimes |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:432](../src/capi/path_hit_c.cpp#L432) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
