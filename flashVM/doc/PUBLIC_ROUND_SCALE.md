# Tiny public round-stroke queries

## Corrected behavior

The public [`fvm_path_hit_test`](../include/flashvm/path.h#L75) round-distance calculation squared tiny tangent lengths and radii. For a line from (0,0) to (10s,0), width 2s, s=2^-600, the point (5s,2s) incorrectly hit: both squared distance and squared radius rounded to zero. A separate executable linked against v0.1504 reproduces the assertion failure before the fix.

Only public positive-radius round queries whose squared tangent norm or squared radius is below DBL_MIN now use a scaled path. The tangent is normalized by its maximum component, the signed along-distance is clamped to the finite segment, and the residual distance is compared using bounded residual/radius ratios. Component rejection precedes division. Constant stroked segments retain their existing disk interpretation. Ordinary normal-square queries, fills, adaptive curve subdivision and selected private cap/join queries retain their existing paths.

## Public API contract

[`fvm_path_hit_test`](../include/flashvm/path.h#L75) borrows a native immutable path and writes an integer boolean to caller storage. The query coordinates must be finite within +/-2000000, width finite in 0..1000000, even_odd either 0 or 1, and path/output non-null. Invalid arguments return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); accepted queries return [FVM_OK](../include/flashvm/flashvm.h#L42). Width zero selects implicitly closed fills with the documented boundary tolerance; positive widths select round strokes. Curves retain adaptive chord tolerance 0.0001 and maximum depth 20. No allocation, callbacks, VM execution, GC or ownership transfer occurs. Independent paths can be queried concurrently; mutation of a queried path requires caller synchronization. Other traversal errors propagate without publishing a successful hit.

The existing [C89 cubic example](../examples/embedding/cubic_extrema/README.md) demonstrates native path ownership and bounds. It does not demonstrate this hit query; the dedicated fixtures below provide its numerical regression coverage.

## Limits

Widths whose half-width itself rounds to zero are outside this extension. Near-boundary rounding, lost coordinate differences and the least subnormal projection distances are not claimed to have exact predicate behavior. No epsilon is added. This does not change rasterization, SWF version visibility, JIT emission, public headers or serialized formats. Reference/device/Cairo acceptance and all nine roadmap stages remain open.

## Fixtures

Seventeen independent executable fixtures cover horizontal/vertical/diagonal segments, reflections, constant segments, scales down to 2^-1000, the normal/subnormal square transition, ordinary ranges, public transactional transform/append, both ends, distant queries, a tiny radius on an ordinary segment, a wide disk around a tiny segment, a cross-product matrix and four native OS workers using independent paths. Expected hits follow analytic perpendicular and endpoint distances; they do not call the private selected-stroke query as an oracle.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_path_hit_test` | function | [include/flashvm/path.h:75](../include/flashvm/path.h#L75) |
<!-- source-links:end -->
