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
<!-- source-links:end -->
