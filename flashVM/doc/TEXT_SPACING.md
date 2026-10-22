# Signed text tracking

[`fvm_text_layout_create_spaced`](../include/flashvm/text_layout.h#L93) adds finite pixels after each supported character,
including the last character on a line. The native limit is -4096 to 4096 pixels.
This follows the [Adobe AS2 Flash 8 letterSpacing description](https://open-flash.github.io/mirrors/as2-language-reference/TextFormat.html).
The original C layout APIs keep zero spacing and unchanged option structure ABI.

The arguments are the borrowed font, counted UTF-8 input, optional existing layout
options, shaping flags, signed spacing and output pointer. Unknown flags, invalid
spacing or missing output return INVALID_ARGUMENT; successful construction returns
an independently owned layout. Allocation failure returns OUT_OF_MEMORY. Failure
clears the output. Destroy it with [`fvm_text_layout_destroy`](../include/flashvm/text_layout.h#L75); exported paths have
independent ownership. NO_FONTS validates flags/spacing and returns UNSUPPORTED.
No VM or GC roots are involved. Calls with independent fonts/buffers may run
concurrently; a mutable font handle needs caller serialization.

Wrapping trials and final geometry use identical adjusted advances. Alignment,
line metrics, glyph positions and copied paths agree. Nonzero tracking currently
requires one glyph per scalar with unique original-byte clusters and nonnegative
adjusted advances. Ligatures, combining sequences, ignored controls and other
ambiguous mappings return UNSUPPORTED rather than inventing internal positions.
Zero retains all existing shaping, including ligatures. Full complex-script
tracking and Flash reference equivalence remain open.

`text_layout_spacing.c` uses the repository CC0 geometric font: at 40 pixels AV
has two 24-pixel advances without kerning. Spacing 2.5 gives 53 pixels; -2 gives
44 pixels. Width 50 wraps positive tracking into two lines. Paths outlive the
font/layout. These are native analytical checks, not physical-device acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_text_layout_create_spaced` | function | [include/flashvm/text_layout.h:93](../include/flashvm/text_layout.h#L93) |
| `fvm_text_layout_destroy` | function | [include/flashvm/text_layout.h:75](../include/flashvm/text_layout.h#L75) |
<!-- source-links:end -->
