# First-line paragraph indentation

v0.893 adds [fvm_text_layout_create_indented](../include/flashvm/text_layout.h#L119). It accepts the same borrowed font,
counted UTF-8, layout options, shaping flags and letterSpacing as
[fvm_text_layout_create_spaced](../include/flashvm/text_layout.h#L101), plus finite first_line_indent in [-4096,4096]
native pixels. The C API accepts fractional offsets. No public structure or
previous function signature changes; zero forwards the old spaced behavior.

Every nonempty hard paragraph offsets only its first visual line. CRLF remains
one separator, other existing hard separators and trailing empty lines retain
their ranges. Empty paragraphs have no offset. First-line wrapping/alignment
uses max_width minus indent, clamped to 1/64px; later lines use full max_width.
An indivisible oversized grapheme still makes progress. No wrapping occurs for
width zero. Nonzero indent currently requires LTR paragraphs with bidi levels
zero. Complex supported word shaping stays intact; bidi paragraphs explicitly
return UNSUPPORTED. This is not complete Flash/Unicode paragraph conformance.

Line.x, glyph positions, exported glyph/layout paths and underline bars include
the first-line offset. Line.advance measures glyph advance without indentation.
Metrics.advance_width is max(0, advance + first-line indent) across all lines,
excluding alignment padding. Negative indent may put native outlines at negative
x; callers select their viewport. Height, source clusters and byte ranges retain
the existing contract. Coordinate and shaping-work limits remain enforced.

The returned [FVM_TextLayout](../src/capi/text_layout_c.cpp#L13) owns copied glyphs, metadata and outlines and is
released with [fvm_text_layout_destroy](../include/flashvm/text_layout.h#L82). [fvm_text_layout_path](../include/flashvm/text_layout.h#L132) and
[fvm_text_layout_glyph_path](../include/flashvm/text_layout.h#L147) return independent [FVM_Path](../src/capi/path_c.cpp#L26) objects, destroyed with
[fvm_path_destroy](../include/flashvm/path.h#L52); either may outlive font/layout. Font, input and options are
borrowed only during creation. No VM/GC state is involved. Mutable font access
requires caller serialization; immutable layout readers can run concurrently.

Missing output/font, invalid flags/options, nonfinite or out-of-range spacing
or indent return INVALID_ARGUMENT. Unsupported bidi/complexity/extent bounds
return UNSUPPORTED; allocation failure returns OUT_OF_MEMORY. Failures clear
*out. NO_FONTS validates flags/spacing/indent only and returns UNSUPPORTED,
without inspecting font/input/options. Existing C89/C++98 and byte/scalar limits
are unchanged. Physical platforms and original Flash reference remain separate.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Path` | struct | [src/capi/path_c.cpp:26](../src/capi/path_c.cpp#L26) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../include/flashvm/path.h#L52) |
| `fvm_text_layout_create_indented` | function | [include/flashvm/text_layout.h:119](../include/flashvm/text_layout.h#L119) |
| `fvm_text_layout_create_spaced` | function | [include/flashvm/text_layout.h:101](../include/flashvm/text_layout.h#L101) |
| `fvm_text_layout_destroy` | function | [include/flashvm/text_layout.h:82](../include/flashvm/text_layout.h#L82) |
| `fvm_text_layout_glyph_path` | function | [include/flashvm/text_layout.h:147](../include/flashvm/text_layout.h#L147) |
| `fvm_text_layout_path` | function | [include/flashvm/text_layout.h:132](../include/flashvm/text_layout.h#L132) |
| `FVM_TextLayout` | struct | [src/capi/text_layout_c.cpp:13](../src/capi/text_layout_c.cpp#L13) |
<!-- source-links:end -->
