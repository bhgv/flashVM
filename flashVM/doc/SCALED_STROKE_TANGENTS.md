# Normalizing tiny stroke tangents

A nonzero segment no longer disappears when its squared length rounds to zero. Only that case divides components by their maximum magnitude before evaluating the norm. Ordinary segments retain their previous arithmetic. Certified linear Beziers now share this path without the former squared-norm rejection.

Private [`fvm_path_stroke_bounds_style`](../src/capi/path_hit_c.cpp#L494) and [`fvm_path_hit_test_style`](../src/capi/path_hit_c.cpp#L412) borrow an immutable path; callers own outputs. There is no allocation, VM callback or GC interaction. Concurrent mutation is forbidden. Existing argument validation and transactional bounds errors remain unchanged. The public legacy distance walker, curve subdivision, rasterizer and C89 ABI are unchanged.

Tests cover horizontal, vertical, diagonal and reversed segments across binary exponents -540 through -1000, all six cap/join combinations, midpoint hits and normalized independent bounds. The existing quadratic/cubic underflow regression now requires successful exact bounds at exponent -600.

Standard binary floating point with gradual underflow is required; fast-math and flush-to-zero are not acceptance configurations. General curved non-round bounds, transform metrics, reference raster and device/platform acceptance remain open. All nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_hit_test_style` | function | [src/capi/path_hit_c.cpp:412](../src/capi/path_hit_c.cpp#L412) |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:494](../src/capi/path_hit_c.cpp#L494) |
<!-- source-links:end -->
