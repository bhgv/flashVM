# Adaptive exact vectors for linear Beziers

Current tiny-tangent extension / Расширение для коротких касательных: [EN](SCALED_STROKE_TANGENTS.md) / [RU](SCALED_STROKE_TANGENTS.ru.md). The squared-norm restriction below is historical.

Non-round bounds no longer require a fixed absolute 1/16 lattice. Start coordinates are subtracted from the endpoint and controls with a Two-Diff residual check; a nonzero residual rejects reduction. Every intermediate is stored as volatile double rather than relying on extended registers. The arithmetic construction is described in [Shewchuk's work](https://www.cs.cmu.edu/~quake/robust.html); it requires ordinary round-to-nearest and standard binary arithmetic without fast-math or flush-to-zero.

The endpoint span supplies a frexp exponent; ldexp increases all exact differences so their magnitudes stay below 2^25. Every scaled component must be integral. Products then remain below 2^50 and cross differences below 2^51, exact with at least 53 mantissa bits. There is no epsilon. Binary-radix and precision checks remain. Exponent overflow is guarded with INT_MAX. The coordinate domain stays ±1,000,000, so scaling increases magnitude and cannot discard tiny components. Vectors with too much relative bit spread can remain unsupported.

## Contract and limits

[fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L503) still borrows an immutable path and requires positive finite width up to 1,000,000, valid selectors and two caller-owned outputs. Admission allocates nothing, invokes no callbacks/GC and needs no VM. Errors and unsupported geometry preserve both outputs; concurrent path mutation is forbidden. Axis paths retain their fast path. Non-axis reduction additionally requires a nonzero norm so tangent underflow in the existing segment walker cannot turn a previous fallback into an empty stroke.

Controls must lie inside the endpoint rectangle and be exactly collinear after scaling. Bounded unordered cubic controls remain admitted. Lost subtraction tails, fractional scaled components, nonzero orientation and unnormalizable tangents retain [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) for non-round styles. Round/round retains general analytic bounds. Finite bodies, caps, joins and Close are unchanged.

Managed quadratic paths share the result through getBounds, dimensions and non-shape point hitTest; getRect and mixed fills retain the underlying path box. Cubic remains native-only. Pixels, shape-hit subdivision, public C89 ABI, wire formats and callback IDs do not change. The [C89 tutorial](../examples/embedding/stroke_styles/README.md) uses a real compact curveTo outside the former lattice, explicitly replacing each result GC root.

This is a bounded exact predicate, not a complete adaptive-precision orientation implementation. General non-round curves, all arbitrary double vectors, new nonuniform/shear metrics, miter/hinting/non-scaling, full Flash/reference acceptance, Cairo and physical platforms remain open. All nine roadmap stages remain open.

## Verified checkpoint v0.1421

21 local commits v0.1401–v0.1421 on published master v0.1400. Validation passes 325 selected native AGG/JIT tests, 79 no-graphics tests and 79 ASan/UBSan tests with the final C89 tutorial and vector_paths; LeakSanitizer is disabled. Eighteen new fixtures cover both curve degrees, six styles, direction, origins up to 999000, binary scales and normalized geometry at scale 2^-400, transactional residual-loss/underflow/precision rejection, contours, managed geometry/hairline/mixed/dimensions/hitTest, 96 quality/paint/style combinations, 24 graphs through binary/channel after sender teardown, and four OS workers with 96 bounds/GC epochs. Existing JIT/timeline/allocation tests are regressions rather than new complete reference acceptance of those subsystems. Source/example/diff checks pass. Other double representations, rounding modes, Cairo and physical devices were not tested; all nine stages remain open.

| Fixture | Coverage |
| --- | --- |
| [path_adaptive_curve_quadratic](../tests/path_adaptive_curve_quadratic.cpp) | Measure compact quadratic curves beyond the fixed lattice |
| [path_adaptive_curve_cubic](../tests/path_adaptive_curve_cubic.cpp) | Measure compact unordered cubic segment bounds |
| [path_adaptive_curve_reverse](../tests/path_adaptive_curve_reverse.cpp) | Retain adaptive vector bounds in reverse traversal |
| [path_adaptive_curve_translation](../tests/path_adaptive_curve_translation.cpp) | Use exact local vectors after large and fractional translations |
| [path_adaptive_curve_scales](../tests/path_adaptive_curve_scales.cpp) | Normalize segment bounds across binary scale exponents |
| [path_adaptive_curve_small](../tests/path_adaptive_curve_small.cpp) | Retain normalized geometry for very small nonzero tangents |
| [path_adaptive_curve_subtraction](../tests/path_adaptive_curve_subtraction.cpp) | Preserve outputs when origin subtraction loses a residual |
| [path_adaptive_curve_underflow](../tests/path_adaptive_curve_underflow.cpp) | Retain fallback when the segment tangent norm underflows |
| [path_adaptive_curve_precision](../tests/path_adaptive_curve_precision.cpp) | Reject excessive vector precision without epsilon |
| [path_adaptive_curve_contours](../tests/path_adaptive_curve_contours.cpp) | Exclude isolated moves from compact curve extents |
| [movieclip_adaptive_curve_geometry](../tests/movieclip_adaptive_curve_geometry.cpp) | Use certified adaptive curve bounds through managed geometry queries |
| [movieclip_adaptive_curve_hairline](../tests/movieclip_adaptive_curve_hairline.cpp) | Measure one-unit adaptive curve hairlines |
| [movieclip_adaptive_curve_mixed](../tests/movieclip_adaptive_curve_mixed.cpp) | Union filled path boxes with certified adaptive strokes |
| [movieclip_adaptive_curve_dimensions](../tests/movieclip_adaptive_curve_dimensions.cpp) | Expose tight adaptive curve width and height observers |
| [movieclip_adaptive_curve_hitbox](../tests/movieclip_adaptive_curve_hitbox.cpp) | Apply tight adaptive curve boxes to non-shape point hits |
| [movieclip_adaptive_curve_quality](../tests/movieclip_adaptive_curve_quality.cpp) | Retain adaptive bounds across paints and stage qualities |
| [movieclip_adaptive_curve_transfer](../tests/movieclip_adaptive_curve_transfer.cpp) | Retain adaptive curves after binary and channel sender teardown |
| [movieclip_adaptive_curve_threads](../tests/movieclip_adaptive_curve_threads.cpp) | Measure and collect adaptive curves on isolated worker runtimes |

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
