# Adaptive exact vectors for linear Beziers

Non-round bounds no longer require a fixed absolute 1/16 lattice. Start coordinates are subtracted from the endpoint and controls with a Two-Diff residual check; a nonzero residual rejects reduction. Every intermediate is stored as volatile double rather than relying on extended registers. The arithmetic construction is described in [Shewchuk's work](https://www.cs.cmu.edu/~quake/robust.html); it requires ordinary round-to-nearest and standard binary arithmetic without fast-math or flush-to-zero.

The endpoint span supplies a frexp exponent; ldexp increases all exact differences so their magnitudes stay below 2^25. Every scaled component must be integral. Products then remain below 2^50 and cross differences below 2^51, exact with at least 53 mantissa bits. There is no epsilon. Binary-radix and precision checks remain. Exponent overflow is guarded with INT_MAX. The coordinate domain stays ±1,000,000, so scaling increases magnitude and cannot discard tiny components. Vectors with too much relative bit spread can remain unsupported.

## Contract and limits

[fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L481) still borrows an immutable path and requires positive finite width up to 1,000,000, valid selectors and two caller-owned outputs. Admission allocates nothing, invokes no callbacks/GC and needs no VM. Errors and unsupported geometry preserve both outputs; concurrent path mutation is forbidden. Axis paths retain their fast path. Non-axis reduction additionally requires a nonzero norm so tangent underflow in the existing segment walker cannot turn a previous fallback into an empty stroke.

Controls must lie inside the endpoint rectangle and be exactly collinear after scaling. Bounded unordered cubic controls remain admitted. Lost subtraction tails, fractional scaled components, nonzero orientation and unnormalizable tangents retain [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) for non-round styles. Round/round retains general analytic bounds. Finite bodies, caps, joins and Close are unchanged.

Managed quadratic paths share the result through getBounds, dimensions and non-shape point hitTest; getRect and mixed fills retain the underlying path box. Cubic remains native-only. Pixels, shape-hit subdivision, public C89 ABI, wire formats and callback IDs do not change. The [C89 tutorial](../examples/embedding/stroke_styles/README.md) uses a real compact curveTo outside the former lattice, explicitly replacing each result GC root.

This is a bounded exact predicate, not a complete adaptive-precision orientation implementation. General non-round curves, all arbitrary double vectors, new nonuniform/shear metrics, miter/hinting/non-scaling, full Flash/reference acceptance, Cairo and physical platforms remain open. All nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:481](../src/capi/path_hit_c.cpp#L481) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
