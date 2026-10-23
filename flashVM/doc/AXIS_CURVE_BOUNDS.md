# Exact bounds for monotone axis Bezier commands

Admission is implemented by [axis_curve_segment](../src/capi/path_hit_c.cpp#L236).

The private [fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L432) accepts quadratic/cubic commands with every supported cap/join when controls progress from start to endpoint on one horizontal or vertical axis. Such a curve describes exactly the endpoint segment: its changing-axis derivative has one sign. Admission uses exact double comparisons without epsilon and creates no approximate polyline. Controls at either endpoint are accepted, including zero endpoint derivatives. A completely constant command contributes no extent.

The segment uses the existing finite-body, cap and join walker. Round/none/square caps and round/bevel joins retain their meanings; adjacent segments and Close use the same normals. Moves without strokes add no extent. Returning, non-axis or unordered controls do not qualify, even where a more general analysis could prove monotonicity. For non-round styles these still return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) while preserving both outputs; managed traversal uses its previous conservative fallback. Round/round retains its general analytic curve support.

## Contract and integration

[fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L432) borrows an immutable path, a positive finite width up to 1,000,000 destination units, selectors and two required caller-owned outputs. There is no allocation, context, callback or GC. Argument errors and unsupported geometry preserve both outputs. Concurrent path mutation is forbidden. Limits live in [path_geometry_constants.h](../src/capi/path_geometry_constants.h). The [internal walker](../src/capi/path_hit_c.cpp) publishes only after the entire path succeeds.

Managed quadratic paths automatically share the precise box through getBounds, MovieClip dimensions and non-shape point hitTest. Mixed fill/stroke paints retain the union with the underlying path box; getRect is unchanged. Hairlines use effective width one; positive widths keep the maximum-axis transform metric. Cubic remains a native path capability; MovieClip still rejects cubic commands. Rasterization and shape-hit subdivision are unchanged. Public C89 ABI, wire formats and callback IDs stay unchanged.

Arbitrary rotations can move an axis path outside the admitted class. General non-round curves, nonuniform/shear metrics, miter/hinting/non-scaling, full Flash reference acceptance, Cairo and physical devices remain open. All nine roadmap stages remain open. The executable [C89 tutorial](../examples/embedding/stroke_styles/README.md) demonstrates actual curveTo(8,8,14,8), none/bevel and an ignored isolated moveTo.

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
