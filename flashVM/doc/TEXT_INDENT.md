# First-line paragraph indentation

v0.893 adds [fvm_text_layout_create_indented](../include/flashvm/text_layout.h#L121). It accepts the same borrowed font,
counted UTF-8, layout options, shaping flags and letterSpacing as
[fvm_text_layout_create_spaced](../include/flashvm/text_layout.h#L103), plus finite first_line_indent in [-4096,4096]
native pixels. The C API accepts fractional offsets. No public structure or
previous function signature changes; zero forwards the old spaced behavior.

The font must be a valid live handle. text may be NULL only for size zero; size is the complete counted UTF-8 byte length, including any embedded NUL. Invalid UTF-8 returns INVALID_ARGUMENT, while existing byte/scalar limits return UNSUPPORTED. options may be NULL for unlimited width, natural line height and AUTO/START. flags must be a subset of [FVM_FONT_SHAPE_FLAGS](../include/flashvm/font.h#L11); letterSpacing retains its signed finite [-4096,4096] contract. out must be non-NULL and receives the independent layout only on success.

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
released with [fvm_text_layout_destroy](../include/flashvm/text_layout.h#L84). [fvm_text_layout_path](../include/flashvm/text_layout.h#L153) and
[fvm_text_layout_glyph_path](../include/flashvm/text_layout.h#L168) return independent [FVM_Path](../src/capi/path_c.cpp#L26) objects, destroyed with
[fvm_path_destroy](../include/flashvm/path.h#L52); either may outlive font/layout. Font, input and options are
borrowed only during creation. No VM/GC state is involved. Mutable font access
requires caller serialization; immutable layout readers can run concurrently.

Missing output/font, invalid flags/options, nonfinite or out-of-range spacing
or indent return INVALID_ARGUMENT. Unsupported bidi/complexity/extent bounds
return UNSUPPORTED; allocation failure returns OUT_OF_MEMORY. Failures clear
*out. NO_FONTS validates flags/spacing/indent only and returns UNSUPPORTED,
without inspecting font/input/options. Existing C89/C++98 and byte/scalar limits
are unchanged. Physical platforms and original Flash reference remain separate.

v0.895 applies integer TextFormat.indent in [-4096,4096] to current/future uniform
styles, getTextExtent and automatic geometry. Positive values work in the existing
SWF6+ text subset. Negative values require unloaded host/SWF8 and enough
leftMargin + blockIndent to keep the first glyph origin inside the left gutter;
otherwise UNSUPPORTED. Noninteger/nonfinite/out-of-range values are INVALID_ARGUMENT.
Null/undefined preserve the prior style. Nonzero indent validates candidate layout
before manual or automatic edits publish. Cached layout reads recheck the negative
version gate. TextWidth/extent.width include intrinsic first-line indent, excluding
paragraph margins; autoSize includes margins and gutter as before.

[FVM_TEXT_STYLE_HAS_INDENT](../src/standard/text_constants.h#L24) stores one optional signed number after optional insets.
Legacy records restore zero; public structures and wire versions stay unchanged.
Current/future styles transfer separately; old readers reject the unknown flag.
Restore checks exact flag/value agreement, integer range and negative policy before
that style's font import. Native attachment follows both styles and geometry.
Already imported fonts retain existing context graph-import lifetime rules.

v0.902 verification: 118 native/tutorial checks pass, including 12 new tests in this series. Twenty-seven targeted ASan/UBSan checks pass (17 preceding justification/inset checks and ten new indentation checks); seven new C++ allocation-failure sweeps run only in the ordinary build. Fifteen NO_FONTS checks pass, including new C89 indentation/admission calls, and both tutorials compile in that profile. LeakSanitizer is disabled because /proc is unavailable; leak acceptance is not claimed.

The new cases cover first/continuation origins, all supported alignments, CRLF/empty paragraphs, fractional/hanging native offsets, ligature/tracking preservation, four real faces and underline, automatic anchors, independent current/future binary and channel transfer, sender teardown/receiver GC, malformed packed state and actual loaded SWF6/7/8 admission. Direct restoration rejects negative offsets in SWF6/7 before attachment. A cached SWF8 hanging field rejects measurement/path reads after loading SWF7 and recovers through valid positive formatting. Native JIT availability is checked separately; AVM1 tests assert native entry counters in attached/automatic modes. Four OS workers each own a font and perform 64 layout/path iterations.

Both annotated C89 tutorials execute the new modes and retain the previous checks. A renamed detached component configures and both copied tutorial checkers pass against the native executables; a separate detached build is not claimed. Source-link and annotated-example/catalog checks pass. Original Flash reference comparison, full bidi paragraph indentation, styled spans, bullets/tabStops, SWF font loading, physical devices/platforms and all nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_FONT_SHAPE_FLAGS` | macro / define | [include/flashvm/font.h:11](../include/flashvm/font.h#L11) |
| `FVM_Path` | struct | [src/capi/path_c.cpp:26](../src/capi/path_c.cpp#L26) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../include/flashvm/path.h#L52) |
| `fvm_text_layout_create_indented` | function | [include/flashvm/text_layout.h:121](../include/flashvm/text_layout.h#L121) |
| `fvm_text_layout_create_spaced` | function | [include/flashvm/text_layout.h:103](../include/flashvm/text_layout.h#L103) |
| `fvm_text_layout_destroy` | function | [include/flashvm/text_layout.h:84](../include/flashvm/text_layout.h#L84) |
| `fvm_text_layout_glyph_path` | function | [include/flashvm/text_layout.h:168](../include/flashvm/text_layout.h#L168) |
| `fvm_text_layout_path` | function | [include/flashvm/text_layout.h:153](../include/flashvm/text_layout.h#L153) |
| `FVM_TEXT_STYLE_HAS_INDENT` | macro / define | [src/standard/text_constants.h:24](../src/standard/text_constants.h#L24) |
| `FVM_TextLayout` | struct | [src/capi/text_layout_c.cpp:13](../src/capi/text_layout_c.cpp#L13) |
<!-- source-links:end -->
