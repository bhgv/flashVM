# Indexed dimension writes — v0.385

SetProperty indices 8/9 and the shared C display-property service now resize
MovieClip vector geometry. Value coercion follows existing index→target→value
ordering; null/undefined and numeric NaN do not resize. Coercion exceptions and
errors propagate. The selected clip remains rooted if valueOf detaches it and
actually collects. Named _width/_height setters are not installed by this change.

Untransformed root vector bounds include descendant local matrices and existing
stroke inflation. The queried clip's own and ancestor matrices are excluded;
no inverse is needed, so a singular ancestor or existing zero scale does not
block resize. Previous intrinsic scales and rotation are read after bounds
collection. The pinned reference's rotated scale rule computes both scales;
this can differ from simply setting the resulting bounding width/height equal
to the requested value. Negative requests/reflections, infinities and degenerate
geometry follow that formula, with nonfinite computed unit scales becoming zero.
Zero-denominator ratios propagate to the same final zero fallback safely.

[GC:] Geometry uses stable node snapshots and the native collection barrier.
Descendant transform getters can run; the root's transform is not evaluated to
obtain untransformed bounds. Both scale properties are staged and published by
one nonallocating swap. OOM/errors leave both scales at their state immediately
before publication; earlier callback side effects remain. Output is committed
only on success and may alias an input. Partial work restores roots/native depth.

Finite computed scales too large for the existing percent representation return
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) before scale publication. Authored shear keeps its existing
basis but exact Flash skew/rounding behavior is not certified. Named intrinsic
properties, exact twip/stroke/cap/join bounds, masks/filter/text/bitmap extents,
mixed player versions and full geometry conformance remain open. This uses the
existing bounded double vector model. No ABI, callback IDs or wire changes.

Tests: movieclip_dimension_write covers host/SWF4–8 empty/flat/ordinary geometry,
negative requests, rotated scale constants, descendants, singular ancestors,
NaN/infinity/null/undefined and representational overflow. Allocation sweeps for
both dimensions check atomic scales/output, GC and retry. Callback cases really
collect while detaching during value coercion, test exceptions/errors/aliasing,
retain geometry callback side effects and prove root-transform/named-width
setter bypass. jit_dimension_write compares indexed writes/reads in interpreter,
explicit and automatic native execution for SWF4–8. Previous display/geometry
regressions remain required.

Primary comparison: [pinned width/height scale formulas](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/display_object.rs)
and [property coercion](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/stage_object.rs).
This is implementation comparison, not a new original Flash Player oracle pass.
All nine roadmap stages remain open.

Validation v0.385: all 807 AGG / 807 Cairo / 788 minimal Linux Release checks
have passing evidence after six ELF execution-permission repairs and targeted
reruns (one AGG, two Cairo, three minimal). Eight restricted heap/global
ASan+UBSan checks pass: four new dimension write/allocation/callback/native
cases plus previous dimension reads, AVM1 display properties, native indexed
reads and geometry allocation. Stack ASan and LeakSanitizer remain disabled.
The revised MovieClip constants header passes a strict C89 syntax probe.
No new original external SWF oracle or foreign-OS run is claimed.
All nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
