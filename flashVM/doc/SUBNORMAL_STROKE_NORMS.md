# Subnormal squared-length precision

The previous correction scaled tangents only when squared length became zero. A nonzero subnormal square also loses relative precision: near binary exponent -538 the direction of a (3,4) segment ceases to be a unit vector. Now every squared length below the minimum normal double uses maximum-component scaling before evaluating its norm. Normal squared lengths retain the previous arithmetic order.

## Contract

The private [`fvm_path_stroke_bounds_style`](../src/capi/path_hit_c.cpp#L733) borrows an immutable path, requires positive finite width within the existing limit, validates cap/join selectors, and publishes caller-owned bounds and presence together on success. [`fvm_path_hit_test_style`](../src/capi/path_hit_c.cpp#L633) borrows the same path and writes a caller-owned integer point result under its existing validation contract. Neither function allocates, invokes callbacks or interacts with GC. Read-only concurrent queries require stable path lifetime; concurrent mutation is forbidden. Unsupported curves and malformed selectors retain their existing status and transactional bounds behavior.

No public API, wire format or callback IDs change. Only styled segment tangents change in the subnormal squared-norm range. Ordinary arithmetic, legacy public distance tests, rasterization and curve subdivision are unchanged. Managed getBounds, dimensions and selected shape hits share the segment walker. General curve bounds, nonuniform metrics and miter/hinting/non-scaling remain open. Gradual binary underflow and ordinary rounding are required; FTZ/fast-math configurations are not validated.

## Regression design

Fourteen fixtures cover diagonal/shallow/steep/reversed independent cap equations, quadratic and bounded unordered cubic reductions in both directions, open joins, closed contours, multiple contours, zero-length/retraced segments, selected body/cap point hits and managed bounds/dimensions. Scale covariance compares contour geometry against normal-sized paths; independent straight-edge equations prevent relying exclusively on that metamorphic oracle. Tests span the normal/subnormal squared-norm transition and zero underflow. The diagonal fixture fails against v0.1427 before the runtime correction.

The existing [C89 stroke tutorial](../examples/embedding/stroke_styles/README.md) remains executable; these adversarial tiny coordinates are regression fixtures rather than a raster demonstration. All nine roadmap stages remain open. Physical devices, Cairo and complete Flash reference acceptance remain separate.


## Round caps and bevel wedges

When the radius square is subnormal, round-cap tests first reject any distance component larger than the radius, then divide both components by the radius and compare the sum of bounded squares against one. This prevents zero/zero and reduced-precision squared-distance false hits without changing normal-radius arithmetic.

Tiny bevel triangles use the largest coordinate difference as a local scale whenever its square is subnormal. A coordinate-box rejection bounds the query ratios before orientation products are evaluated. Original coordinates and bounds remain unchanged. This corrects uniform tiny-scale underflow; it is not arbitrary-precision orientation for all near-collinear double inputs. Independent cap inclusion/rejection and sampled wedge classification from exponents -515 through -1000 supplement the original twelve fixtures.


Widths whose half-width itself underflows to zero are outside this validation range; this series does not establish their geometry contract.


## Validation v0.1445

343 native, 97 no-graphics and 97 ASan/UBSan tests pass. LeakSanitizer is disabled. Documentation, source links and the C89 tutorial checks pass. Fourteen new fixtures cover the three corrections. Physical devices and complete Flash acceptance are not claimed.


## Public round-distance extension

The subsequent [public round query correction](PUBLIC_ROUND_SCALE.md) avoids tiny squared distances in the public legacy round-distance path. The earlier selected cap/join policy remains unchanged. Half-width underflow and general exact predicates remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_hit_test_style` | function | [src/capi/path_hit_c.cpp:633](../src/capi/path_hit_c.cpp#L633) |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:733](../src/capi/path_hit_c.cpp#L733) |
<!-- source-links:end -->
