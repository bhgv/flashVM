# Default tab metrics

The native [fvm_font_average_width](../include/flashvm/font.h#L65) function reads the positive OS/2 xAvgCharWidth and scales it through FreeType to unhinted 26.6 pixels. It never substitutes a space, maximum advance or sample-string average. It borrows the live font synchronously; serialize mutable face access. No allocation, VM state or GC roots are involved. A required out scalar is zeroed on failure. Invalid pointers return INVALID_ARGUMENT; absent/nonpositive metrics return UNSUPPORTED; NO_FONTS returns UNSUPPORTED with valid out. Existing font structures and ABIs remain unchanged.

The fixture independently declares 547 units at 1000 units/em: 40px gives 21.875px, 20px gives 10.9375px. This metric alone does not enable default tabs.

Reference: [OpenType OS/2 specification](https://learn.microsoft.com/en-us/typography/opentype/spec/os2#xavgcharwidth). Flash reference/device acceptance remains open.

v0.916: [fvm_text_layout_create_periodic](../include/flashvm/text_layout.h#L155) accepts a finite positive interval up to 1,000,000 pixels. It places U+0009 at positive multiples strictly beyond the logical pen including indent and tracking. Stops restart per visual line and do not allocate or use the 64-entry table. It retains ordinary glyph/work/extent limits, empty synthetic outlines, wrapping, ownership and status rules of the custom API. Tabs still require LTR levels zero and non-JUSTIFY. Existing APIs still reject tabs without a custom table. NO_FONTS validates period/flags/spacing/indent then returns UNSUPPORTED without reading font/text/options. NULL out and invalid interval give INVALID_ARGUMENT; output is always cleared.

v0.917: TextField and TextFormat.getTextExtent now use four declared average widths when the owned custom table is empty and actual U+0009 exists. Interval is derived from the selected real face and size; missing metrics are UNSUPPORTED. No tab in the input means no extra metric requirement. Custom tables still take precedence. Empty Arrays clear to defaults; null/undefined format properties preserve the prior style. Current/future styles remain independent, snapshots still return empty Arrays for default stops. Editing eagerly validates tab geometry even without autoSize, so failures preserve text/style/cache/anchors. No new snapshot field or wire bit is required: defaults are derived again from embedded font data. Bidi/justify tabs remain unsupported. This bounded OS/2 metric policy has not been compared to a Flash Player binary. Reference: [Adobe AS2 TextFormat](https://open-flash.github.io/mirrors/as2-language-reference/TextFormat.html#tabStops).

v0.918: repeating stop extent exhaustion participates in wrap trials, so two million-pixel tabs can occupy separate million-pixel visual lines instead of failing before a valid break. Native boundary/admission tests cover this recovery, oversized tabs, CRLF resets, signed indentation, tracking, four alignments, fractional periods, tiny/nonfinite inputs, bidi/justify rejection and independently owned empty tab paths. Unlimited overflowing text remains UNSUPPORTED.

v0.919: independent mutated-font fixtures check missing OS/2, its FreeType sentinel, zero and negative average widths; ordinary shaping remains usable. TextField assignment, replaceText and clearing an explicit table reject missing default metrics while preserving text, current table, path, autoSize dimensions and right anchor. An explicit table still works with that font and text without tabs adds no average-metric dependency.

v0.920: binary and channel tests transfer default current styles with either default or explicit future styles. They verify absent custom-table wire bits, size-dependent recomputation after source-context destruction and receiver GC, preserved right anchors and independent exported paths. Font transfer uses the existing validated embedded-font protocol; this change adds no wire format.

v0.921: ordinary C++ allocation-boundary sweeps cover periodic native construction, current-style size changes, text assignment and replaceText. Every failed construction clears output and keeps the font reusable; failed field edits preserve text, measured width/height, autoSize dimensions and usable paths through post-call GC. These fault sweeps are ordinary builds, separate from sanitizer acceptance.

v0.922: actual AVM1 size writes followed by setTextFormat produce equal 40px/20px default tab measurements in interpreter, attached native JIT and automatic JIT modes (entry counters checked by the shared runner). Four real OS workers independently own fonts and derive 40px/20px intervals across 64 iterations each, checking tracking, indent and path lifetime. This does not authorize sharing a mutable font or VM context across threads.

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
