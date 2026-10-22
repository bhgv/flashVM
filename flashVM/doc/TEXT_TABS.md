# Explicit custom tab stops

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
[FVM_TextLayout](../src/capi/text_layout_c.cpp#L13) independently owns metadata, glyphs and outlines and is released
with [fvm_text_layout_destroy](../include/flashvm/text_layout.h#L84). [fvm_text_layout_path](../include/flashvm/text_layout.h#L153)/fvm_text_layout_glyph_path
return separate [FVM_Path](../src/capi/path_c.cpp#L26) objects released with [fvm_path_destroy](../include/flashvm/path.h#L52), allowed to outlive
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

[FVM_TEXT_STYLE_HAS_TABS](../src/standard/text_constants.h#L26) adds a bounded u32 count and tagged numeric positions after
optional indent. Legacy records restore an empty table, current/future transfer
separately and old readers reject the unknown flag. Reader count is checked before
allocation; restore checks table bounds/order/integer values and exact flag/table
agreement before that style's font import. Existing graph-import font lifetime
rules still apply. No public C structure or wire version changes.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Path` | struct | [src/capi/path_c.cpp:26](../src/capi/path_c.cpp#L26) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../include/flashvm/path.h#L52) |
| `fvm_text_layout_create_indented` | function | [include/flashvm/text_layout.h:121](../include/flashvm/text_layout.h#L121) |
| `fvm_text_layout_create_tabbed` | function | [include/flashvm/text_layout.h:140](../include/flashvm/text_layout.h#L140) |
| `fvm_text_layout_destroy` | function | [include/flashvm/text_layout.h:84](../include/flashvm/text_layout.h#L84) |
| `fvm_text_layout_glyph_path` | function | [include/flashvm/text_layout.h:168](../include/flashvm/text_layout.h#L168) |
| `FVM_TEXT_LAYOUT_MAX_TAB_STOPS` | macro / define | [include/flashvm/text_layout.h:14](../include/flashvm/text_layout.h#L14) |
| `fvm_text_layout_path` | function | [include/flashvm/text_layout.h:153](../include/flashvm/text_layout.h#L153) |
| `FVM_TEXT_STYLE_HAS_TABS` | macro / define | [src/standard/text_constants.h:26](../src/standard/text_constants.h#L26) |
| `FVM_TextLayout` | struct | [src/capi/text_layout_c.cpp:13](../src/capi/text_layout_c.cpp#L13) |
<!-- source-links:end -->
