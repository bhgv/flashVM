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


## v0.1524 checkpoint

All 360 selected no-graphics Release executables and 182 selected no-graphics ASan/UBSan Debug executables exit zero with assertions enabled. Both profiles include the seventeen new fixtures, previous tiny/translated curve and selected-stroke regressions, public path queries, affine/Point geometry and the existing C89 tutorials. The Release run also includes additional sound/stream/mix regressions; these are regression evidence, not new audio functionality. LeakSanitizer is disabled; sanitizer executable-name lookup warnings can limit symbolization. No device, Cairo or new native-JIT acceptance is claimed.

The twenty-commit format-patch series is checked by git am --3way against a clean published v0.1504 base, with final trees, consecutive versions and both identities compared. Public headers are unchanged. Source links and frontend tutorial checks pass; generated logs remain outside Git. All nine roadmap stages remain open.

| Fixture / Проверка | Source / Исходник |
| --- | --- |
| analytic | [analytic](../tests/path_public_round_scale_analytic.cpp) |
| vertical | [vertical](../tests/path_public_round_scale_vertical.cpp) |
| diagonal | [diagonal](../tests/path_public_round_scale_diagonal.cpp) |
| negative | [negative](../tests/path_public_round_scale_negative.cpp) |
| zero segment | [zero_segment](../tests/path_public_round_scale_zero_segment.cpp) |
| scales | [scales](../tests/path_public_round_scale_scales.cpp) |
| transition | [transition](../tests/path_public_round_scale_transition.cpp) |
| normal | [normal](../tests/path_public_round_scale_normal.cpp) |
| tiny zero | [tiny_zero](../tests/path_public_round_scale_tiny_zero.cpp) |
| transform | [transform](../tests/path_public_round_scale_transform.cpp) |
| append | [append](../tests/path_public_round_scale_append.cpp) |
| caps | [caps](../tests/path_public_round_scale_caps.cpp) |
| far | [far](../tests/path_public_round_scale_far.cpp) |
| thin | [thin](../tests/path_public_round_scale_thin.cpp) |
| wide | [wide](../tests/path_public_round_scale_wide.cpp) |
| matrix | [matrix](../tests/path_public_round_scale_matrix.cpp) |
| threads | [threads](../tests/path_public_round_scale_threads.cpp) |

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
