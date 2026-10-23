# Certified linear Bezier stroke bounds

Exact bounds now use segment geometry for quadratic/cubic commands with collinear controls inside the endpoint rectangle. For non-axis paths every coordinate must lie exactly on a 1/16 lattice within the original ±1,000,000 domain; DBL_MANT_DIG must be at least 53. Multiplication by 16 produces exact integers, differences are at most 32,000,000, products stay below 2^50 and cross differences below 2^51. Thus the cross-product test cannot round a nonzero deviation to zero. There is no epsilon. Public path admission is unchanged; its coordinate limit is shared through the [private header](../src/capi/path_geometry_constants.h).

Axis paths retain arbitrary admitted finite coordinates without a lattice requirement. Controls must lie inside the endpoint interval, but cubic controls may now be unordered. For normalized u,v in [0,1], the cubic coordinate derivative is affine in u,v; its four control-square vertices are proportional to t², 2t(1-t), (1-2t)² and (1-t)². All are nonnegative, so this entire class is monotone. Controls at opposite endpoints can produce an admitted stationary midpoint.

## Contract

[fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L454) retains its contract: an immutable borrowed path, a positive finite width up to 1,000,000, supported cap/join selectors and two required caller-owned outputs. Admission and traversal allocate nothing, invoke no callbacks/GC and need no context. Argument errors and unsupported curves preserve both outputs. Concurrent path mutation is forbidden. Existing finite bodies, round/none/square caps, round/bevel joins and Close geometry are reused. Constants and isolated Moves contribute no stroke.

Non-axis off-grid coordinates, noncollinear controls and controls outside the endpoint interval are not reduced; non-round styles retain [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) and the managed fallback. Round/round retains general analytic curves. Transforms can move a path off the lattice. The existing nonuniform width metric is unchanged.

Managed quadratic paths automatically share the new bounds through getBounds, dimensions and non-shape point hitTest. getRect and mixed fill paints retain the underlying path-box policy. Cubic remains native-only; MovieClip still rejects it. Pixels, shape-hit subdivision, C89 ABI, wire formats and callback IDs are unchanged. The [C89 tutorial](../examples/embedding/stroke_styles/README.md) adds a real diagonal curveTo with none/bevel and an ignored isolated moveTo.

General non-round curves, exact arbitrary off-grid collinearity, nonuniform/shear metrics, miter/hinting/non-scaling, full Flash reference acceptance, Cairo and devices remain open. All nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:454](../src/capi/path_hit_c.cpp#L454) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
