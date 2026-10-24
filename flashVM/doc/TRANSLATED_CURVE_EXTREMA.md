# Translated Bezier extrema

## Defect and numerical policy

A small curve translated close to a large coordinate lost significant derivative bits through subtraction of weighted absolute controls. For origin 524288, unit 2^-33 and cubic controls [14,53,1,23] in local units, the old upper bound was 27 units; the independent local oracle is approximately 28.995047089778474. The representable corrected result is 29 units.

The shared `curve_bounds` helper includes the endpoint first. When the start is nonzero and all active controls have the same sign and magnitudes between half and twice its magnitude, Sterbenz subtraction supplies exact local coordinates under ordinary IEEE binary arithmetic with gradual underflow. Derivatives and Bernstein evaluation use those coordinates, and the origin is restored once per extremum. Other control configurations retain the previous arithmetic. Tiny derivative discriminants still use the previous binary exponent scaling.

This affects public [`fvm_path_bounds`](../include/flashvm/path.h#L71) and private round/round stroke bounds. It allocates no storage and retains no state. Public headers, ownership, return statuses, callback identities and wire formats are unchanged. Cubics remain native paths; this does not add cubic MovieClip commands or change JIT emission.

## API contract and example

[`fvm_path_bounds`](../include/flashvm/path.h#L71) borrows an immutable non-null path and writes four owned scalar coordinates to a non-null output. It returns [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) for null inputs and [FVM_OK](../include/flashvm/flashvm.h#L42) for an accepted measurement, including an empty zero box. It neither transfers ownership nor invokes the VM, GC or host callbacks. Concurrent independent paths are permitted; mutation of the same path during measurement needs caller synchronization. Coordinate admission and the existing treatment of move-only contours are unchanged. Native path allocation and edits can still fail through their own APIs.

The existing executable [C89 cubic example](../examples/embedding/cubic_extrema/README.md) remains applicable and is rerun in every selected profile. No new public function is introduced.

## Limits

The fix is conditional local-coordinate arithmetic, not an interval enclosure proof or a complete general root solver. Coordinate quantization, near-multiple roots, mixed-sign widely separated controls, non-round arbitrary curves and raster/transform metric agreement remain separate concerns. No physical devices, Cairo, FTZ/fast-math or non-binary platform acceptance is claimed. All nine roadmap stages remain open.

## Regression cases

Seventeen fixtures compare normalized output against an independent long-double derivative oracle on small integer controls before translation. A 0.51-unit tolerance permits rounding of the restored absolute coordinate; it rejects the old nearly two-unit error. The matrix varies eight shapes, three axes, signs and binary origins. Other fixtures isolate quadratic, linear and repeated-root cases, both cubic humps, round strokes, native affine transforms and tiny origins down to 2^-900. Existing tiny-cubic and stroke regression families are also rerun.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_path_bounds` | function | [include/flashvm/path.h:71](../include/flashvm/path.h#L71) |
<!-- source-links:end -->
