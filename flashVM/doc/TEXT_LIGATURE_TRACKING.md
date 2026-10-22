# Optional ligatures and tracking

v0.861 adds [FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES](../include/flashvm/font.h#L10) to [fvm_font_shape_configured](../include/flashvm/font.h#L90).
It disables the OpenType liga and clig features for the complete input run.
Required rlig substitutions, joining and mark positioning retain their defaults.
[FVM_FONT_SHAPE_NO_KERNING](../include/flashvm/font.h#L9) remains independent; bitwise OR selects both policies.
Flags zero and [fvm_font_shape](../include/flashvm/font.h#L76) preserve legacy shaping. Flags are per call and do
not mutate the font's persistent state. This is OpenType feature selection, not
a promise to split every script cluster or every font technology.

The API borrows font/text/language only during the call and returns an owned
[FVM_TextRun](../src/capi/font_c.cpp#L75) released by [fvm_text_run_destroy](../include/flashvm/font.h#L94). Use the same font/size when exporting
its path with [fvm_text_run_path](../include/flashvm/font.h#L98). Mutable font handles require caller serialization;
independent handles may run on separate threads. No VM, GC, callback or device
is used. Unknown flags, invalid UTF-8/direction/input are INVALID_ARGUMENT and
clear a valid output pointer. Allocation failure is OUT_OF_MEMORY. NO_FONTS
validates flag/direction representations then returns UNSUPPORTED for valid flags.
The existing bounded shaping and outline coordinate limits remain unchanged.

The original CC0 fixture independently gives fi=28px at 40px with a ligature,
f+i=24px without it, and AV=44.8px with kerning versus 48px without. Tests exercise
all four flag combinations and verify that later legacy calls retain ligatures.

[HarfBuzz feature documentation](https://harfbuzz.github.io/shaping-opentype-features.html)
describes whole-buffer feature overrides. Flash reference conformance and actual
platform/device acceptance remain separate; all roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_font_shape` | function | [include/flashvm/font.h:76](../include/flashvm/font.h#L76) |
| `fvm_font_shape_configured` | function | [include/flashvm/font.h:90](../include/flashvm/font.h#L90) |
| `FVM_FONT_SHAPE_NO_KERNING` | macro / define | [include/flashvm/font.h:9](../include/flashvm/font.h#L9) |
| `FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES` | macro / define | [include/flashvm/font.h:10](../include/flashvm/font.h#L10) |
| `fvm_text_run_destroy` | function | [include/flashvm/font.h:94](../include/flashvm/font.h#L94) |
| `fvm_text_run_path` | function | [include/flashvm/font.h:98](../include/flashvm/font.h#L98) |
| `FVM_TextRun` | struct | [src/capi/font_c.cpp:75](../src/capi/font_c.cpp#L75) |
<!-- source-links:end -->
