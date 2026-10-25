# Stable secondary-axis stroke bounds

Bounds now use the existing `vector_length` for both affine columns. The former Y formula sqrt(xy*xy+yy*yy) could erase a nonzero scale or lose relative precision through squared-norm underflow. The X column and shape hitTest already used the stable norm. This changes numerical robustness, not the selected metric: positive authored width multiplies the maximum column length; a hairline remains one destination-space unit.

The underflow regression fails on v0.1445 before this correction. Admitted larger-scale scenarios inversely scale source coordinates and authored width. Affine components remain bounded by the existing path admission limit of 1,000,000. Matrices with exponents 510, 600 and 900 are still rejected before stroke metric evaluation, even when transformed points would be small. No limits are expanded.

## Contract and validation limits

[`fvm_movieclip_set_matrix`](../include/flashvm/movieclip.h#L138) accepts finite affine matrices under its existing checks and updates clip-owned state; the clip belongs to its context. Measurements through [`fvm_object_call`](../include/flashvm/value.h#L184) borrow the clip and coordinate-space object in that context; `getBounds` returns a managed object which must be rooted across later VM calls. Fixtures copy its four numbers immediately before any later calls. `getRect` retains stroke-free extents. Dimensions and non-shape hitTest share the corrected bounds branch. Shape hits already used the stable norm and are checked for consistency.

Fixtures use a separate identity clip as their target space. Singular source transforms are admitted, but rejection of noninvertible target transforms is unchanged. Existing allocation, callback, thread and GC restrictions remain; fixtures explicitly retain and release managed roots. Four workers use separate runtimes/contexts, without concurrent mutation of one clip.

Coverage includes independent straight-segment equations, reflected transforms, admitted exponents -1000..17, normal/subnormal transitions and rejected out-of-domain matrices, varying X-column dominance, six cap/join combinations, four solid/gradient paints, four quality settings, dimensions, both point-hit modes, getRect, linear quadratic commands and consecutive edges. The quadratic fixture edits native command metadata; the existing C89 example separately demonstrates a real curveTo. Forty-eight graphs travel through binary and channel transfer after sender teardown; four OS workers run 96 bounds/GC epochs.

The pixel renderer still chooses X-column length for normal strokes: general nonuniform/shear raster-metric agreement remains open. General non-round curves, arbitrary double geometry, miter/hinting/non-scaling, Cairo, full Flash reference acceptance and physical platforms remain open. All nine roadmap stages remain open. Ordinary binary arithmetic is required; FTZ/fast-math configurations are not validated. Public C89 ABI, formats, callback IDs and native JIT emission do not change.

The existing [C89 tutorial](../examples/embedding/stroke_styles/README.md) remains executable. This series adds numeric regressions rather than a raster demonstration of practically invisible coordinates.


## Validation v0.1464

360 native, 114 no-graphics and 114 ASan/UBSan tests pass. LeakSanitizer is disabled. Source-link and C89 tutorial/catalog checks pass. This is not physical-device or complete Flash reference acceptance.

| Fixture | Scope / Область |
| --- | --- |
| [movieclip_secondary_axis_underflow](../tests/movieclip_secondary_axis_underflow.cpp) | Nonzero tiny Y columns |
| [movieclip_secondary_axis_subnormal](../tests/movieclip_secondary_axis_subnormal.cpp) | Reduced-precision subnormal squares |
| [movieclip_secondary_axis_overflow](../tests/movieclip_secondary_axis_overflow.cpp) | Preserved rejection of excessive affine components |
| [movieclip_secondary_axis_normal](../tests/movieclip_secondary_axis_normal.cpp) | Ordinary admitted transforms |
| [movieclip_secondary_axis_transition](../tests/movieclip_secondary_axis_transition.cpp) | Arithmetic and admission boundaries |
| [movieclip_secondary_axis_reflection](../tests/movieclip_secondary_axis_reflection.cpp) | Reflected source geometry |
| [movieclip_secondary_axis_dominance](../tests/movieclip_secondary_axis_dominance.cpp) | Secondary-column maximum |
| [movieclip_secondary_axis_paints](../tests/movieclip_secondary_axis_paints.cpp) | Solid and gradient paints |
| [movieclip_secondary_axis_quality](../tests/movieclip_secondary_axis_quality.cpp) | Four stage qualities |
| [movieclip_secondary_axis_dimensions](../tests/movieclip_secondary_axis_dimensions.cpp) | Named width and height |
| [movieclip_secondary_axis_hitbox](../tests/movieclip_secondary_axis_hitbox.cpp) | Bounds and shape point hits |
| [movieclip_secondary_axis_rect](../tests/movieclip_secondary_axis_rect.cpp) | Stroke-free getRect |
| [movieclip_secondary_axis_curves](../tests/movieclip_secondary_axis_curves.cpp) | Axis-linear quadratic metadata |
| [movieclip_secondary_axis_contours](../tests/movieclip_secondary_axis_contours.cpp) | Consecutive segments |
| [movieclip_secondary_axis_gc](../tests/movieclip_secondary_axis_gc.cpp) | Pinned roots and collection |
| [movieclip_secondary_axis_transfer](../tests/movieclip_secondary_axis_transfer.cpp) | Binary and channel graphs after sender teardown |
| [movieclip_secondary_axis_threads](../tests/movieclip_secondary_axis_threads.cpp) | Separate runtime OS workers |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_set_matrix` | function | [include/flashvm/movieclip.h:138](../include/flashvm/movieclip.h#L138) |
| `fvm_object_call` | function | [include/flashvm/value.h:184](../include/flashvm/value.h#L184) |
<!-- source-links:end -->
