# Owned horizontal text layout
Current v0.826 adds explicit pair-kerning flags without changing the legacy native
API default or options ABI. Wrapping, placed paths and transferred TextFields use
the selected feature consistently. [Contract and evidence](TEXT_KERNING.md).


`flashvm/text_layout.h` builds a single-font text block from validated UTF-8.
It uses the static FreeType/HarfBuzz/libunibreak/FriBidi targets selected by
[`FONTS`](../cmake/BuildDefinitions.cmake#L15) in DEFS.txt. No platform font lookup or downloaded dependency is used.

Create a font with [`fvm_font_read`](../include/flashvm/font.h#L64), then call [`fvm_text_layout_create`](../include/flashvm/text_layout.h#L94). NULL
options select unlimited width, the font's natural line height, AUTO paragraph
direction and START alignment. The example in
`examples/embedding/text_layout/main.c` shows wrapping, mixed Latin/Hebrew/Arabic,
measurement, export and drawing. It runs in headless/no-graphics profiles too.

## Layout and measurement

The decoder retains original UTF-8 byte boundaries. CRLF is one hard separator;
CR, LF, VT, FF, NEL, LINE SEPARATOR and PARAGRAPH SEPARATOR also create hard
lines. Empty input yields one empty line. Consecutive and trailing separators
preserve empty lines. Each hard line starts a fresh bidi paragraph in this API.

FriBidi resolves the whole paragraph before wrapping. Each proposed line gets
its own L1 trailing-whitespace reset and visual run order. Runs are split by
embedding level and Unicode script. Leading common/inherited script characters
inherit the next script; interior ones inherit the preceding script. HarfBuzz
shapes logical segments with surrounding line context, then segments are placed
in visual order. The implementation never applies codepoint visual ordering to
already-shaped ligatures or marks. Default-ignorable characters are removed
from glyph output while original input byte offsets are retained.

Wrapping uses libunibreak opportunities and greedy width trials. Each candidate
is shaped to measure its actual advance. At the first overflowing opportunity,
the last fitting candidate is chosen. An oversized first word falls back to
extended grapheme boundaries. One oversized indivisible grapheme occupies a
line even if it exceeds max_width. Final lines are reshaped at their actual
boundaries, so ligatures and Arabic joining reflect the chosen line. Whitespace
is preserved, including leading/trailing spaces; there is no trimming or
justification. This greedy policy does not search beyond the first overflow for
non-monotonic shaping widths. A max_width of zero disables wrapping.

| Query | Meaning |
| --- | --- |
| metrics.advance_width | Maximum actual line advance, excluding alignment offsets |
| metrics.height | Ascent + descent + (line_count - 1) * baseline spacing |
| metrics.ink | Exact union of placed outline curve bounds; excludes strokes |
| line.byte_start / byte_end | Original byte range, excludes its hard separator |
| line.byte_next | Start of following line; consumes CRLF/hard separator when present |
| line.x / baseline | Final line origin in pixels, x right/y down |
| glyph.shaped.cluster | Original UTF-8 byte offset; multiple glyphs may share it |
| glyph.x / y | Final glyph origin, including shaping offsets |
| glyph.bidi_level | Resolved level after this line's trailing-whitespace reset |

Baseline zero starts at the font ascent. Explicit line_height is baseline
spacing and may produce overlapping lines. LEFT/CENTER/RIGHT align within the
requested width; START chooses LEFT for LTR paragraphs and RIGHT for RTL ones.
Oversized lines are placed at x=0. With unlimited width, alignment offsets are
zero. Logical advances are distinct from ink bounds: spaces have advance but
no outline, and curves can extend beyond their advances.

## Ownership and editable vectors

Layout construction copies each unhinted glyph outline and retains its own
merged block path. The font, input, options and language can be freed after a
successful call. Neither metrics nor path export uses those resources again.
The resulting layout can be queried concurrently while no thread destroys it;
font use during construction still follows the per-handle serialization rule.

[`fvm_text_layout_path`](../include/flashvm/text_layout.h#L199) returns an independent block path;
[`fvm_text_layout_glyph_path`](../include/flashvm/text_layout.h#L218) returns an independent glyph path at its final
position. Editing either cannot change the layout or another exported path.
Select words using original cluster byte ranges and append selected glyph
paths. A ligature is one indivisible outline: splitting its component letters
requires shaping without that ligature, not slicing the glyph by bytes.
Paths support affine transforms and control-point edits, AGG/Cairo fill/stroke,
and image masks through the existing path API. NO_GRAPHICS retains layout,
measurement and editable geometry; drawing/masking return UNSUPPORTED.

## Bounds and remaining scope

This is horizontal single-font, single-style layout. It is not a completed
SWF TextField/TextFormat implementation. Font fallback, style spans, vertical
text, selection/caret hit testing, shaping-aware editing, justification,
configurable tabs, discretionary hyphens and full Unicode script-extension /
bracket script itemization remain separate work. The simple script inheritance
policy is documented here rather than claimed as a full itemizer.

Tabs and SOFT HYPHEN return UNSUPPORTED. Limits are 4096 decoded scalars,
16384 UTF-8 bytes, 65536 output glyphs, and one million trial-shaped scalars.
The path API also bounds total commands and coordinates to one million.
Input validation/option errors return INVALID_ARGUMENT; unsupported features or
layout-work limits return UNSUPPORTED. On any construction/export failure the
output pointer is NULL. Options with non-finite, negative or excessive dimensions
are rejected; invalid C enum byte representations are checked before C++ reads.

Inherited font/data-version limitations are in FONT_LIBRARIES.md. Windows,
macOS and Android execution is not established by the Linux tests. NO_FONTS
retains linkable layout creation with UNSUPPORTED and null output.
See TEXT_LAYOUT_CHECKPOINT.md for recorded acceptance evidence.

## Signed tracking (v0.830–0.836)

[`fvm_text_layout_create_spaced`](../include/flashvm/text_layout.h#L122) adds bounded signed fractional pixels after each supported character while preserving the existing option ABI. Wrapping, alignment, glyph advances and owned paths agree. Zero retains legacy shaping; nonzero ambiguous clusters and negative adjusted advances return UNSUPPORTED. Arguments, statuses, lifetime, thread restrictions and tests: [contract](TEXT_SPACING.md).

## Independent underline bars (v0.837–0.844)

[`fvm_font_underline_metrics`](../include/flashvm/font.h#L90) copies font-defined scaled center/thickness. [`fvm_text_layout_line_underline_path`](../include/flashvm/text_layout.h#L213) returns an owned bar for a placed line, including whitespace/tracking and alignment. Layout/glyph metrics remain unchanged. Font/layout may be destroyed before rendering copied paths. Contracts, nonzero fill restrictions, missing metrics and tests: [underline](TEXT_UNDERLINE.md).

## Optional ligatures in tracking (v0.861–v0.868)

[fvm_text_layout_create_spaced](../include/flashvm/text_layout.h#L122) disables optional OpenType liga/clig for nonzero
spacing, using the same feature policy for wrapping and final placement. Its
zero-spacing path and configured flags remain explicit. Required/ambiguous
clusters still return UNSUPPORTED. [Contract and evidence](TEXT_LIGATURE_TRACKING.md).

## Bounded paragraph justification (v0.877–0.884)

[`FVM_TEXT_ALIGN_JUSTIFY`](../include/flashvm/text_layout.h#L28) expands internal U+0020 spaces on nonfinal wrapped LTR lines. Trailing spaces retain their records with zero advance; final/hard-ended/no-gap lines stay natural. Bidi and ambiguous space clusters reject explicitly. Measurement, glyph positions, tracking, underline and separately owned paths agree. [Arguments, statuses, ownership and evidence](TEXT_JUSTIFY.md).

The C89 [fvm_text_layout_create_indented](../include/flashvm/text_layout.h#L142) API adds signed first-line offsets per nonempty hard paragraph without changing option structures. It preserves zero-indent shaping, owned paths and existing byte ranges; nonzero values currently require bounded LTR paragraphs. [Arguments, metrics, errors and ownership](TEXT_INDENT.md).

[fvm_text_layout_create_tabbed](../include/flashvm/text_layout.h#L163) adds an explicit bounded custom table to indented layout without changing existing option structures. Tabs carry source clusters, empty outlines and real advances; tables restart on each visual line. [Full arguments, errors, ownership and limits](TEXT_TABS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FONTS` | DEFS switch | [cmake/BuildDefinitions.cmake:15](../cmake/BuildDefinitions.cmake#L15) |
| `fvm_font_read` | function | [include/flashvm/font.h:64](../include/flashvm/font.h#L64) |
| `fvm_font_underline_metrics` | function | [include/flashvm/font.h:90](../include/flashvm/font.h#L90) |
| `FVM_TEXT_ALIGN_JUSTIFY` | enum constant | [include/flashvm/text_layout.h:28](../include/flashvm/text_layout.h#L28) |
| `fvm_text_layout_create` | function | [include/flashvm/text_layout.h:94](../include/flashvm/text_layout.h#L94) |
| `fvm_text_layout_create_indented` | function | [include/flashvm/text_layout.h:142](../include/flashvm/text_layout.h#L142) |
| `fvm_text_layout_create_spaced` | function | [include/flashvm/text_layout.h:122](../include/flashvm/text_layout.h#L122) |
| `fvm_text_layout_create_tabbed` | function | [include/flashvm/text_layout.h:163](../include/flashvm/text_layout.h#L163) |
| `fvm_text_layout_glyph_path` | function | [include/flashvm/text_layout.h:218](../include/flashvm/text_layout.h#L218) |
| `fvm_text_layout_line_underline_path` | function | [include/flashvm/text_layout.h:213](../include/flashvm/text_layout.h#L213) |
| `fvm_text_layout_path` | function | [include/flashvm/text_layout.h:199](../include/flashvm/text_layout.h#L199) |
<!-- source-links:end -->
