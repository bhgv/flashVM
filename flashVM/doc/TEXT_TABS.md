# Explicit custom tab stops

Current v0.917 behavior: empty AS2 tables now select font-derived default stops; see [default contract](TEXT_DEFAULT_TABS.md). The zero-count native create_tabbed contract below is unchanged. Default restrictions below describe the v0.903–v0.914 checkpoint.

v0.903 adds [fvm_text_layout_create_tabbed](../include/flashvm/text_layout.h#L140). It borrows the same font, counted UTF-8,
options, shaping flags, signed letterSpacing and first-line indent as
[fvm_text_layout_create_indented](../include/flashvm/text_layout.h#L121), plus stops and count. The table contains at most
[FVM_TEXT_LAYOUT_MAX_TAB_STOPS](../include/flashvm/text_layout.h#L14) (64) finite, nonnegative, strictly increasing native
pixel positions no greater than 1,000,000. NULL is allowed only when count is zero.
Zero count preserves the old indented API, including unsupported U+0009 input.
Old signatures and public structure layouts remain unchanged.

Custom U+0009 advances to the first stop strictly beyond the current logical pen,
including first-line indent and prior character tracking. A tab receives no extra
letterSpacing. Stops restart on each visual line and are measured before line
alignment from the paragraph content origin. No default/repeating stops are
inferred. Exhausted stops can force a wrap before the tab; if a tab at the new
line's origin still has no stop, layout is UNSUPPORTED. Oversized tab advances
follow the existing indivisible-grapheme overflow policy. Hard paragraphs reset
first-line indentation as before.

Each tab preserves a source byte cluster and synthetic glyph-0 entry with its
real advance and an empty outline. Export never draws the font's .notdef glyph
for tab records. Word shaping, optional ligatures and required cluster restrictions
remain intact between tab boundaries. Actual tab paragraphs currently require
LTR bidi levels zero and reject JUSTIFY; unused tables do not restrict otherwise
supported paragraphs. Left/center/right/START move the complete placed line.
Metrics exclude alignment padding, include intrinsic indentation, and retain
source ranges. Underline spans the real tab advances.

Creation borrows font/input/options/table only for the call. The returned
[FVM_TextLayout](../src/capi/text_layout_c.cpp#L14) independently owns metadata, glyphs and outlines and is released
with [fvm_text_layout_destroy](../include/flashvm/text_layout.h#L84). [fvm_text_layout_path](../include/flashvm/text_layout.h#L168)/fvm_text_layout_glyph_path
return separate [FVM_Path](../src/capi/path_c.cpp#L28) objects released with [fvm_path_destroy](../include/flashvm/path.h#L52), allowed to outlive
layout/font. The font must be live; text may be NULL only for zero byte length.
NULL options selects unlimited width, natural height and AUTO/START. Flags and
spacing/indent bounds match the indented API. No VM/GC state is involved. Serialize
mutable font access; immutable layouts support independent readers.

Bad table/pointer/count/order/geometry, invalid flags/options/UTF-8 and invalid
spacing/indent return INVALID_ARGUMENT. Unsupported bidi/justify/tab exhaustion
or shaping/extent limits return UNSUPPORTED; allocation failure OUT_OF_MEMORY.
Failures clear out, which must be non-NULL. NO_FONTS validates table, flags,
spacing and indent before UNSUPPORTED and does not inspect text/options/font.
Automatic default tab width, full Unicode/bidi tab layout, Flash reference and
physical-device/platform acceptance remain open.

v0.906 applies TextFormat.tabStops from a same-context native Array. Length is
captured once (0..64), then indexed properties are read in order through the normal
property bridge. Positions must be Number integers in [0,1000000], strictly
increasing; holes and bad values are INVALID_ARGUMENT, other receiver types
UNSUPPORTED. Getter errors/work cancellation propagate. [GC:] the borrowed Array
is rooted through getters; only numeric copies are retained. Getter side effects
are observable and are not rolled back when field admission fails.

Null/undefined preserves the prior style; an empty Array clears the custom table,
which leaves default tabs unsupported. Full assignment uses the future style;
current formatting/replacement uses the current uniform style. Nonempty tables
force candidate geometry validation before publication, even for manual fields.
getTextFormat/getNewTextFormat return fresh managed Arrays (including an empty
Array for no custom table); mutation of source/snapshot Arrays does not edit the
native field. Keep managed results rooted across GC on the context owner thread.

[FVM_TEXT_STYLE_HAS_TABS](../src/standard/text_constants.h#L28) adds a bounded u32 count and tagged numeric positions after
optional indent. Legacy records restore an empty table, current/future transfer
separately and old readers reject the unknown flag. Reader count is checked before
allocation; restore checks table bounds/order/integer values and exact flag/table
agreement before that style's font import. Existing graph-import font lifetime
rules still apply. No public C structure or wire version changes.

Ordinary native collector barriers remain in force during getters; recursive [fvm_gc_collect](../include/flashvm/gc.h#L44) is rejected there. Getter side effects/errors and post-call collection are tested without weakening those barriers. Tab-bearing text is validated even when a candidate table is empty, so unsupported default-tab edits reject before publication. Unwrapped TextField justify retains its existing left-aligned natural-layout policy; the unsupported native tab+JUSTIFY combination matters when justification is actually selected for layout.

v0.914 verification: 132 native/tutorial checks pass, including 14 new tests in this series. Thirty-nine targeted ASan/UBSan checks pass (27 previous checks plus 12 new tab-layout/state checks). Seven new C++ allocation-failure sweeps run in the ordinary build only. Seventeen NO_FONTS checks pass, including the new C89 table/admission calls; both tutorials compile in that profile. LeakSanitizer remains disabled because /proc is unavailable, so leak acceptance is not claimed.

Cases cover stop exhaustion and wrapping, empty tab outlines/source clusters, all supported alignments, indentation/fractional tracking, preserved word ligatures, four real faces/underline, automatic fitting, independent source/snapshot Arrays, getter side effects/errors with preserved native collector barriers, post-call GC and malformed tables. The 64-position wire oracle checks 1672 added bytes for both styles, including number metadata references; graph reservation tracks that exact variable payload. Oversize wire counts and truncation reject. Current/future binary and channel transfer survive sender teardown and receiver GC. Four OS workers each own a native font and perform 64 layout/path iterations while mutating the borrowed input table after creation. AVM1 tests interpreter, attached native and automatic JIT with entry counters; native JIT availability is checked separately.

The synthetic tab branch shares the aggregate glyph cap before insertion. Both annotated C89 tutorials execute their new modes and preserve the earlier checks. A renamed detached component configures and both copied tutorial checkers pass against the native executables; a separate detached build is not claimed. Source-link and annotated-example/catalog checks pass. Default/repeating stops, full bidi/tab+justify semantics, styled spans, original Flash reference comparison, SWF font loading, physical devices/platforms and all nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:44](../include/flashvm/gc.h#L44) |
| `FVM_Path` | struct | [src/capi/path_c.cpp:28](../src/capi/path_c.cpp#L28) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../include/flashvm/path.h#L52) |
| `fvm_text_layout_create_indented` | function | [include/flashvm/text_layout.h:121](../include/flashvm/text_layout.h#L121) |
| `fvm_text_layout_create_tabbed` | function | [include/flashvm/text_layout.h:140](../include/flashvm/text_layout.h#L140) |
| `fvm_text_layout_destroy` | function | [include/flashvm/text_layout.h:84](../include/flashvm/text_layout.h#L84) |
| `fvm_text_layout_glyph_path` | function | [include/flashvm/text_layout.h:183](../include/flashvm/text_layout.h#L183) |
| `FVM_TEXT_LAYOUT_MAX_TAB_STOPS` | macro / define | [include/flashvm/text_layout.h:14](../include/flashvm/text_layout.h#L14) |
| `fvm_text_layout_path` | function | [include/flashvm/text_layout.h:168](../include/flashvm/text_layout.h#L168) |
| `FVM_TEXT_STYLE_HAS_TABS` | macro / define | [src/standard/text_constants.h:28](../src/standard/text_constants.h#L28) |
| `FVM_TextLayout` | struct | [src/capi/text_layout_c.cpp:14](../src/capi/text_layout_c.cpp#L14) |
<!-- source-links:end -->
