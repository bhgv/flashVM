# Round stroke bounds and hairlines

The managed bounds traversal now sends every straight stroke through the existing private cap/join walker, including absent selectors (round caps and round joins). This removes the separate legacy round rectangle policy. An isolated move does not enlarge a stroke-only extent; a path containing only zero-length edges contributes no stroke extent. A filled paint still retains its underlying path bounds and unions any nonempty stroke.

Zero authored width uses one destination unit for both straight bounds and the conservative curve envelope. Positive width uses the existing maximum-axis transform metric; complete positive-width collapse contributes no stroke extent. getBounds, MovieClip dimensions and non-shape point hitTest share this traversal. getRect retains the underlying path geometry. These are geometric bounds, not antialias pixel extents.

The implementation is in [bounds](../src/standard/movieclip.cpp); straight geometry uses [fvm_path_stroke_bounds_style](../src/capi/path_hit_c.cpp#L323). The latter remains allocation-free, context-free and transactional, with caller-owned outputs and an immutable borrowed path. Managed traversal still allocates snapshots and converted paths, uses its existing native GC barrier and propagates allocation errors. No API, serialization field or callback ID changes.

Quadratic/cubic paths continue to use the path bounding box expanded by half the effective width, multiplied by sqrt(2) for square caps. This is deliberately conservative, not an exact offset-curve bound. Existing nonuniform/shear width metric differences, raster/reference acceptance, unsupported LINESTYLE2 styles, Cairo and physical devices remain open. All nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_stroke_bounds_style` | function | [src/capi/path_hit_c.cpp:323](../src/capi/path_hit_c.cpp#L323) |
<!-- source-links:end -->
