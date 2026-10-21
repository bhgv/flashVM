# Explicit text kerning

[Русский](TEXT_KERNING.ru.md).

## Native shaping (v0.820)

[fvm_font_shape_configured](../include/flashvm/font.h#L74) adds flags to the existing owned native shaping
contract. Zero preserves legacy HarfBuzz defaults; [FVM_FONT_SHAPE_NO_KERNING](../include/flashvm/font.h#L9)
disables the kern feature while preserving ligatures and mark placement. The
original [fvm_font_shape](../include/flashvm/font.h#L62) API is unchanged. Unknown flags return INVALID_ARGUMENT
and clear the owned output pointer. Valid calls in NO_FONTS return UNSUPPORTED.
Input UTF-8, direction and font ownership follow the existing font contract;
font handles must not be shared concurrently. Returned runs are independent.
No VM, GC, callbacks or device access is involved.


## Layout and wrapping (v0.821)

[fvm_text_layout_create_configured](../include/flashvm/text_layout.h#L80) accepts the same font shaping flags without
changing [FVM_TextLayoutOptions](../include/flashvm/text_layout.h#L26) or the legacy create API. Every candidate line
and final script/bidi segment uses identical features. Width, alignment, glyph
positions and exported paths therefore agree. The result owns all copied paths
and survives font destruction; mutable fonts still have a single owner. Failures
clear the result pointer, unknown flags are INVALID_ARGUMENT, and valid NO_FONTS
requests are UNSUPPORTED. Tabs, styled spans and other old layout limits remain.


## AS2 TextFormat/TextField (v0.822)

New TextFormat objects and dynamic TextFields default to kerning=false, as the
Flash 8 AS2 contract specifies. Explicit non-null values use the VM Boolean
conversion; null/undefined retain the prior style during merging. Current and
future formats carry independent selections, getters return the selected Boolean,
and edits/cached metrics/path export use the same feature. Partial styled spans,
full device-font behavior and complete TextField semantics remain outside this subset.
Legacy metric fixtures now explicitly request their previously implicit kerning.


## Transfer compatibility (v0.823)

Current/future native field styles retain kerning through binary serialization
and channels. The existing alignment word carries an explicit NO_KERNING flag;
no record length, allocation charge or public ABI structure changes. Old field
records without the flag restore their historically enabled kerning. New dynamic
fields default disabled. Other unknown bits/invalid low alignment values reject
with existing graph rollback. Old readers reject the new disabled flag rather
than silently drawing a different width. This field-record compatibility does
not preserve implicit behavior of old generic TextFormat objects with null flags.


## C89 raster tutorial (v0.824)

The [text layout example](../examples/embedding/text_layout/README.md) now accepts
`--no-kerning` and draws its independently owned paths into caller RGBA storage.
The default invocation preserves legacy native shaping. The reusable text-jit
profile enables pinned static font libraries, rasterization and the available
native JIT without requiring SDL/system fonts. No window or output file is created.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_font_shape` | function | [include/flashvm/font.h:62](../include/flashvm/font.h#L62) |
| `fvm_font_shape_configured` | function | [include/flashvm/font.h:74](../include/flashvm/font.h#L74) |
| `FVM_FONT_SHAPE_NO_KERNING` | macro / define | [include/flashvm/font.h:9](../include/flashvm/font.h#L9) |
| `fvm_text_layout_create_configured` | function | [include/flashvm/text_layout.h:80](../include/flashvm/text_layout.h#L80) |
| `FVM_TextLayoutOptions` | struct | [include/flashvm/text_layout.h:26](../include/flashvm/text_layout.h#L26) |
<!-- source-links:end -->
