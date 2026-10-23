# Stable secondary-axis stroke bounds

Bounds now use the existing `vector_length` for both affine columns. The former Y formula sqrt(xy*xy+yy*yy) could erase a nonzero scale through underflow or produce infinity through overflow while the length itself remained finite. The X column and shape hitTest already used the stable norm. This changes numerical robustness, not the selected metric: positive authored width multiplies the maximum column length; a hairline remains one destination-space unit.

The underflow regression fails on v0.1445 before this correction. Large-scale scenarios inversely scale source coordinates and authored width so transformed geometry and effective width remain inside existing admission limits. No limits are expanded.

## Contract and validation limits

[`fvm_movieclip_set_matrix`](../include/flashvm/movieclip.h#L107) accepts finite affine matrices under its existing checks and updates clip-owned state; the clip belongs to its context. Measurements through [`fvm_object_call`](../include/flashvm/value.h#L126) borrow the clip and coordinate-space object in that context; `getBounds` returns a managed object which must be rooted across later VM calls. Fixtures copy its four numbers immediately before any later calls. `getRect` retains stroke-free extents. Dimensions and non-shape hitTest share the corrected bounds branch. Shape hits already used the stable norm and are checked for consistency.

Fixtures use a separate identity clip as their target space. Singular source transforms are admitted, but rejection of noninvertible target transforms is unchanged. Existing allocation, callback, thread and GC restrictions remain; fixtures explicitly retain and release managed roots. Four workers use separate runtimes/contexts, without concurrent mutation of one clip.

Coverage includes independent straight-segment equations, reflected transforms, exponents -1000..900, normal/subnormal/overflow transitions, varying X-column dominance, six cap/join combinations, four solid/gradient paints, four quality settings, dimensions, both point-hit modes, getRect, linear quadratic commands and consecutive edges. The quadratic fixture edits native command metadata; the existing C89 example separately demonstrates a real curveTo. Forty-eight graphs travel through binary and channel transfer after sender teardown; four OS workers run 96 bounds/GC epochs.

The pixel renderer still chooses X-column length for normal strokes: general nonuniform/shear raster-metric agreement remains open. General non-round curves, arbitrary double geometry, miter/hinting/non-scaling, Cairo, full Flash reference acceptance and physical platforms remain open. All nine roadmap stages remain open. Ordinary binary arithmetic is required; FTZ/fast-math configurations are not validated. Public C89 ABI, formats, callback IDs and native JIT emission do not change.

The existing [C89 tutorial](../examples/embedding/stroke_styles/README.md) remains executable. This series adds numeric regressions rather than a raster demonstration of practically invisible coordinates.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_set_matrix` | function | [include/flashvm/movieclip.h:107](../include/flashvm/movieclip.h#L107) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../include/flashvm/value.h#L126) |
<!-- source-links:end -->
