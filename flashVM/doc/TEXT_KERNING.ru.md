# Явное управление кернингом

[English](TEXT_KERNING.md).

## Нативное формирование (v0.820)

[fvm_font_shape_configured](../include/flashvm/font.h#L74) добавляет flags к прежнему контракту владения native
run. Ноль сохраняет HarfBuzz defaults; [FVM_FONT_SHAPE_NO_KERNING](../include/flashvm/font.h#L9) отключает kern,
сохраняя лигатуры и размещение marks. Старый [fvm_font_shape](../include/flashvm/font.h#L62) не меняется.
Неизвестные flags дают INVALID_ARGUMENT и очищают выходной owner. В NO_FONTS
корректные flags дают UNSUPPORTED. UTF-8, direction и владение font соответствуют
прежнему API; font handle нельзя использовать конкурентно. Возвращённый run
независим. Нет VM, GC, callbacks и устройств.


## Размещение и переносы (v0.821)

[fvm_text_layout_create_configured](../include/flashvm/text_layout.h#L80) принимает те же shaping flags без изменения
[FVM_TextLayoutOptions](../include/flashvm/text_layout.h#L26) и старого create. Все пробные строки и конечные script/bidi
сегменты используют одинаковые features: ширина, выравнивание, позиции и пути
согласованы. Результат владеет скопированными путями и переживает font; сам font
остаётся однопоточным. Ошибки очищают результат, неизвестные flags дают
INVALID_ARGUMENT, корректные NO_FONTS-запросы — UNSUPPORTED. Табуляция, styled spans
и остальные прежние ограничения не снимаются.

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
