# Default tab metrics

The native [fvm_font_average_width](../include/flashvm/font.h#L65) function reads the positive OS/2 xAvgCharWidth and scales it through FreeType to unhinted 26.6 pixels. It never substitutes a space, maximum advance or sample-string average. It borrows the live font synchronously; serialize mutable face access. No allocation, VM state or GC roots are involved. A required out scalar is zeroed on failure. Invalid pointers return INVALID_ARGUMENT; absent/nonpositive metrics return UNSUPPORTED; NO_FONTS returns UNSUPPORTED with valid out. Existing font structures and ABIs remain unchanged.

The fixture independently declares 547 units at 1000 units/em: 40px gives 21.875px, 20px gives 10.9375px. This metric alone does not enable default tabs.

Reference: [OpenType OS/2 specification](https://learn.microsoft.com/en-us/typography/opentype/spec/os2#xavgcharwidth). Flash reference/device acceptance remains open.

v0.916: [fvm_text_layout_create_periodic](../include/flashvm/text_layout.h#L155) accepts a finite positive interval up to 1,000,000 pixels. It places U+0009 at positive multiples strictly beyond the logical pen including indent and tracking. Stops restart per visual line and do not allocate or use the 64-entry table. It retains ordinary glyph/work/extent limits, empty synthetic outlines, wrapping, ownership and status rules of the custom API. Tabs still require LTR levels zero and non-JUSTIFY. Existing APIs still reject tabs without a custom table. NO_FONTS validates period/flags/spacing/indent then returns UNSUPPORTED without reading font/text/options. NULL out and invalid interval give INVALID_ARGUMENT; output is always cleared.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_font_average_width` | function | [include/flashvm/font.h:65](../include/flashvm/font.h#L65) |
| `fvm_text_layout_create_periodic` | function | [include/flashvm/text_layout.h:155](../include/flashvm/text_layout.h#L155) |
<!-- source-links:end -->
