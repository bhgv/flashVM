# Font-defined underline geometry

[`fvm_font_underline_metrics`](../include/flashvm/font.h#L63) copies the scalable font's underline center and full
thickness in unhinted pixels, with positive y downward relative to the baseline.
FreeType normalizes source-format metrics; [its tutorial](https://freetype.org/freetype2/docs/tutorial/step2.html)
explains underline centers and font-unit scaling. No guessed thickness is used.

Arguments are a borrowed font and required [`FVM_FontUnderlineMetrics`](../include/flashvm/font.h#L32) output.
[FVM_OK](../include/flashvm/flashvm.h#L42) publishes copied scalars that outlive the font. Output is cleared before
failure: missing output/font is INVALID_ARGUMENT with fonts enabled; absent or
nonpositive scaled metrics are UNSUPPORTED. NO_FONTS rejects a missing output,
otherwise clears it and returns UNSUPPORTED. This query allocates nothing and
accesses no VM/GC state. A shared mutable font handle needs caller serialization;
independent handles and copied results can be used by independent workers.

`font_underline.c` uses the CC0 geometric fixture with 1000 units/em and an
explicit TrueType post top-edge value -75, thickness 50. FreeType's normalized
center is -100 units: at 40 pixels the y-down center is 4 and thickness 2.
Three pixel sizes are checked against fixed analytical expectations. Existing
glyph outlines, advances and substitutions are unchanged. This is native metric
evidence, not complete Flash text or platform/device acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_font_underline_metrics` | function | [include/flashvm/font.h:63](../include/flashvm/font.h#L63) |
| `FVM_FontUnderlineMetrics` | struct | [include/flashvm/font.h:32](../include/flashvm/font.h#L32) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->
