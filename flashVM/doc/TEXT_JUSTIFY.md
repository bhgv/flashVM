# Bounded native paragraph justification

v0.877 adds [FVM_TEXT_ALIGN_JUSTIFY](../include/flashvm/text_layout.h#L24) to [FVM_TextLayoutOptions](../include/flashvm/text_layout.h#L27) for the existing
[fvm_text_layout_create](../include/flashvm/text_layout.h#L79), [fvm_text_layout_create_configured](../include/flashvm/text_layout.h#L87) and
[fvm_text_layout_create_spaced](../include/flashvm/text_layout.h#L101) calls. Public structure layout and previous enum
values are unchanged. These calls borrow the font, counted UTF-8 and options;
they return independently owned [FVM_TextLayout](../src/capi/text_layout_c.cpp#L13) data, destroyed with
[fvm_text_layout_destroy](../include/flashvm/text_layout.h#L82). Exported paths remain separately owned after layout/font
teardown. Mutable font handles require caller serialization; immutable layouts
may be read by independent threads. No VM, GC or device is involved.

This policy currently accepts LTR paragraphs with bidi levels zero. ASCII U+0020
spaces need one independently shaped glyph per space cluster. Word ligatures and
combining glyph order remain unchanged. Trailing ASCII-space advances are zeroed;
leading spaces retain advances, and internal spaces on nonfinal wrapped lines
share remaining width equally. Final lines, hard paragraph ends and lines without
internal spaces remain natural. Oversized indivisible clusters are not compressed.
Whitespace byte/glyph records remain present. Trial wrapping uses the same trimmed
measurement as final placement. Other alignments retain their previous behavior.

Unknown option enum values are INVALID_ARGUMENT with a cleared output in the
font-enabled profile; unsupported bidi/space clusters or complexity/coordinate
limits return UNSUPPORTED. Allocation failure returns OUT_OF_MEMORY. NO_FONTS
retains its existing flag/spacing validation and returns UNSUPPORTED without
inspecting layout options. Existing scalar, byte, shaping-work and extent bounds
apply. This is not complete Arabic/CJK/Unicode justification or Flash conformance.

The original CC0 font at 40px gives A/V 24px and a space 12px without kerning.
At width 90, A V A V makes two lines: the first fills 90 with V at x=66 and
zero trailing advance; the final line retains width 60. Source clusters survive.
All roadmap stages and physical platform/device acceptance remain open.

[Adobe AS2 TextFormat reference](https://open-flash.github.io/mirrors/as2-language-reference/TextFormat.html)
defines justify as a Flash8 paragraph alignment; exact Flash reference matching
is a separate acceptance step.

## AS2 field policy and whitespace edges — v0.878

TextFormat align accepts justify for unloaded host contexts and loaded SWF8;
older loaded movies return UNSUPPORTED when applying/measuring that format.
Wrapped multiline TextFields use the same native policy, including auto-size
and underline advances. Unwrapped fields retain left placement for this policy.
Manual justified fields also validate layout before text/policy/format publication;
unsupported wrapped bidi edits leave the previous field intact. Current/future
format snapshots return justify. Packed graph state uses a separate style bit,
not the existing low alignment/kerning bits; no extra payload numbers are added.
Legacy records retain their alignment, old readers reject the new style flag.

Whitespace tests cover consecutive/leading/trailing spaces, hard ends, CRLF,
empty final paragraphs and word ligatures. Natural width sums retained glyph
advances directly, avoiding cancellation under fractional all-space tracking.
Space/combining ambiguity and bidi runs reject with a cleared output.

v0.879 verifies current justification with future right-aligned bold text through binary/channel transfer, sender teardown, receiver GC and later edits. Font-face and +1 tracking tests check the same 90px underline span for all four faces, plus zero-width all-space paths. Selected fonts, current/future alignment and automatic height remain independent.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_TEXT_ALIGN_JUSTIFY` | enum constant | [include/flashvm/text_layout.h:24](../include/flashvm/text_layout.h#L24) |
| `fvm_text_layout_create` | function | [include/flashvm/text_layout.h:79](../include/flashvm/text_layout.h#L79) |
| `fvm_text_layout_create_configured` | function | [include/flashvm/text_layout.h:87](../include/flashvm/text_layout.h#L87) |
| `fvm_text_layout_create_spaced` | function | [include/flashvm/text_layout.h:101](../include/flashvm/text_layout.h#L101) |
| `fvm_text_layout_destroy` | function | [include/flashvm/text_layout.h:82](../include/flashvm/text_layout.h#L82) |
| `FVM_TextLayout` | struct | [src/capi/text_layout_c.cpp:13](../src/capi/text_layout_c.cpp#L13) |
| `FVM_TextLayoutOptions` | struct | [include/flashvm/text_layout.h:27](../include/flashvm/text_layout.h#L27) |
<!-- source-links:end -->
