# Bounded native paragraph justification

v0.877 adds [FVM_TEXT_ALIGN_JUSTIFY](../include/flashvm/text_layout.h#L26) to [FVM_TextLayoutOptions](../include/flashvm/text_layout.h#L29) for the existing
[fvm_text_layout_create](../include/flashvm/text_layout.h#L81), [fvm_text_layout_create_configured](../include/flashvm/text_layout.h#L89) and
[fvm_text_layout_create_spaced](../include/flashvm/text_layout.h#L103) calls. Public structure layout and previous enum
values are unchanged. These calls borrow the font, counted UTF-8 and options;
they return independently owned [FVM_TextLayout](../src/capi/text_layout_c.cpp#L13) data, destroyed with
[fvm_text_layout_destroy](../include/flashvm/text_layout.h#L84). Exported paths remain separately owned after layout/font
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

v0.880 verifies actual loaded SWF6/7 rejection and SWF8 admission, plus native restoration of the same state. Conflicting low alignment bits, reserved and unknown flags reject before native attachment. A C caller tests unknown enum bytes and explicit RTL admission; NO_FONTS retains its documented opaque-options behavior.

v0.881 adds seven ordinary-build C++ allocation sweeps: native layout plus current-format admission, text assignment, replacement, automatic fitting, width and wrap-policy changes. Failed calls preserve text, current alignment, dimensions and cached metrics, with balanced GC/native scopes. Fonts remain reusable after failed construction. Bundled C allocator failures are not injected.

v0.882 drives actual AVM1 alignment writes and method/property calls in interpreter, attached native code and automatic JIT. Justify/right alternate 90/72 widths with the same 94px automatic height. Four independent font-owner OS workers perform 64 builds each with distinct widths and fractional tracking, verifying expanded glyph positions and separately owned paths.

v0.883 adds the annotated C89 `--justify` tutorial and executes its two-line 90/60px observation in the example checker. The path remains separately owned after the font/layout release.

v0.884 validation: 96 native/tutorial, nine targeted ASan/UBSan and 12 NO_FONTS checks pass. The seven allocation sweeps run in the ordinary build. Native JIT availability was checked separately and the AVM1 fixture verifies native entry counters. LeakSanitizer is disabled because /proc is unavailable; this is no leak-acceptance claim. A tracked-file copy under a renamed component configures; its copied checker passes against the native tutorial executable, without claiming a detached build. Source-link and annotated-example checks pass. Full Unicode/bidi justification, Flash reference comparisons, devices/platforms and all nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_TEXT_ALIGN_JUSTIFY` | enum constant | [include/flashvm/text_layout.h:26](../include/flashvm/text_layout.h#L26) |
| `fvm_text_layout_create` | function | [include/flashvm/text_layout.h:81](../include/flashvm/text_layout.h#L81) |
| `fvm_text_layout_create_configured` | function | [include/flashvm/text_layout.h:89](../include/flashvm/text_layout.h#L89) |
| `fvm_text_layout_create_spaced` | function | [include/flashvm/text_layout.h:103](../include/flashvm/text_layout.h#L103) |
| `fvm_text_layout_destroy` | function | [include/flashvm/text_layout.h:84](../include/flashvm/text_layout.h#L84) |
| `FVM_TextLayout` | struct | [src/capi/text_layout_c.cpp:13](../src/capi/text_layout_c.cpp#L13) |
| `FVM_TextLayoutOptions` | struct | [include/flashvm/text_layout.h:29](../include/flashvm/text_layout.h#L29) |
<!-- source-links:end -->
