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

## AS2 uniform fields and transfers (v0.831)

TextFormat `letterSpacing` is null on construction; uniform field formats expose
zero by default. Numeric coercion accepts signed fractional values within the
native limit. Null/undefined leave the selected style unchanged. Current and
future styles are independent: setNewTextFormat does not alter current metrics,
while replacing text applies future tracking. Failed style or text admission
preserves the previous field and cache. `textWidth`, getTextExtent, wrapping and
path export share the native layout path. Complex clusters remain UNSUPPORTED.

Native field snapshots and binary/channel transfers retain both tracking values.
A HAS_SPACING bit in private packed alignment announces one optional encoded
number; zero spacing retains the old byte layout. Old records restore zero.
Old readers do not support the new flagged records. No public ABI or graph
header version changes. Transfer byte accounting includes the optional number.
This adds uniform styles only, not styled ranges, system fonts or full SWF text.

v0.832: `textfield_spacing_transfer.cpp` checks signed current/future binary and channel transfer, legacy zero records, inconsistent state rejection, source destruction and receiver GC. `text_layout_spacing_boundaries.c` checks RTL original byte clusters, center/right alignment, kerning composition, embedded NUL, CRLF/trailing empty lines, zero adjusted advances and combining-sequence rejection.

v0.833: `textfield_spacing_allocation.cpp` sweeps current-format, assignment and replaceText allocation boundaries, preserving text/style/cached width and balanced GC scopes on failure. `text_layout_spacing_allocation.cpp` checks cleared output, partial-path cleanup and immediate font reuse after failed construction. Fault injection runs in the ordinary native build; it is not sanitizer fault-unwind acceptance.

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
