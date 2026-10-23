# Tiny cubic Bezier extrema

At sufficiently small coordinates the squared derivative coefficient and the product of the other coefficients rounded to zero. The discriminant then lost two interior roots; [`fvm_path_bounds`](../include/flashvm/path.h#L71) and round/round stroke bounds could underestimate extents. The native cubic with controls 0,12,-12,0 scaled by 2^-600 fails on v0.1464. Independent analytic extrema of the normalized curve are ±2√3.

The cubic branch selects the largest magnitude of its three derivative coefficients. If its square is below the minimum normal double, frexp supplies a shared exponent and ldexp increases all coefficients by the same binary power before evaluating the discriminant. On ordinary binary arithmetic this scales coefficients exactly without changing the mathematical roots. The existing stable q formula remains. Normal coefficients and the linear/quadratic branch retain their arithmetic order. No epsilon is introduced.

## Contract and limits

[`fvm_path_bounds`](../include/flashvm/path.h#L71) borrows an immutable native path and publishes four scalar coordinates into caller-owned [`FVM_PathBounds`](../include/flashvm/path.h#L42) on [FVM_OK](../include/flashvm/flashvm.h#L42). Invalid path/out yields INVALID_ARGUMENT without publication. There is no allocation, callback, VM, GC or device work. The caller owns the native path until [`fvm_path_destroy`](../include/flashvm/path.h#L52) and must ensure stable lifetime and no concurrent mutation. [`fvm_path_transform`](../include/flashvm/path.h#L65) and [`fvm_path_append`](../include/flashvm/path.h#L67) retain transactional validation and coordinate/matrix limits of ±1,000,000. Internal round/round geometry shares the helper and then expands by half the positive width; existing statuses and joint output requirements are preserved.

Cubic remains a native path command: managed MovieClip still admits quadratic. Pixel rasterization and native JIT receive no new algorithm. General nearly repeated/collinear double geometry and lost low bits when constructing coefficients from large nearby coordinates do not gain arbitrary-precision treatment. Ordinary binary arithmetic is required; FTZ/fast-math and physical platforms are not validated. All nine roadmap stages remain open; codecs, child streams and complete Flash reference acceptance are not completed by this series.

## Regression design and example

An independent oracle solves ordinary long-double quadratic roots on unscaled fixture controls and evaluates curves by de Casteljau. Exact analytic ±2√3 and 16/3 checks plus 1025 samples for each of seven shapes supplement it. Coverage includes axes, diagonal geometry, reflection, exponents through -1000, subnormal-square transitions, round strokes, quadratic/linear branches, repeated/negative discriminants, append/transform, multiple contours and four independent OS workers with 196 stroke checks. Oracle formulas are not extracted from the production helper.

The [executable C89 tutorial](../examples/embedding/cubic_extrema/README.md) creates a native path, checks ordinary bounds, scales the real public path by 2^-600, normalizes its new bounds and releases the owner without a VM. Every line is explained; ordinary errors replace assertions. The example also works without graphics. Its README details all five APIs, ownership and statuses. Public C89 ABI, callback IDs and formats are unchanged.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_path_append` | function | [include/flashvm/path.h:67](../include/flashvm/path.h#L67) |
| `fvm_path_bounds` | function | [include/flashvm/path.h:71](../include/flashvm/path.h#L71) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../include/flashvm/path.h#L52) |
| `fvm_path_transform` | function | [include/flashvm/path.h:65](../include/flashvm/path.h#L65) |
| `FVM_PathBounds` | struct | [include/flashvm/path.h:42](../include/flashvm/path.h#L42) |
<!-- source-links:end -->
