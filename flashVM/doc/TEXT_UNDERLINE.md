# Font-defined underline geometry

[`fvm_font_underline_metrics`](../include/flashvm/font.h#L63) copies the scalable font's underline center and full
thickness in unhinted pixels, with positive y downward relative to the baseline.
FreeType normalizes source-format metrics; [its tutorial](https://freetype.org/freetype2/docs/tutorial/step2.html)
explains underline centers and font-unit scaling. No guessed thickness is used.

Arguments are a borrowed font and required [`FVM_FontUnderlineMetrics`](../include/flashvm/font.h#L32) output.
[FVM_OK](../include/flashvm/flashvm.h#L42) publishes copied scalars that outlive the font. Output is cleared before
failure: missing output/font is INVALID_ARGUMENT with fonts enabled; absent or
nonpositive scaled metrics are UNSUPPORTED. NO_FONTS rejects a missing output,
otherwise clears it and returns UNSUPPORTED. This query allocates nothing and
accesses no VM/GC state. A shared mutable font handle needs caller serialization;
independent handles and copied results can be used by independent workers.

`font_underline.c` uses the CC0 geometric fixture with 1000 units/em and an
explicit TrueType post top-edge value -75, thickness 50. FreeType's normalized
center is -100 units: at 40 pixels the y-down center is 4 and thickness 2.
Three pixel sizes are checked against fixed analytical expectations. Existing
glyph outlines, advances and substitutions are unchanged. This is native metric
evidence, not complete Flash text or platform/device acceptance.

## Owned per-line paths (v0.838)

[`fvm_text_layout_line_underline_path`](../include/flashvm/text_layout.h#L117) borrows an immutable layout and line index,
font-derived center/thickness and required output pointer. It returns one owned
closed rectangle spanning line.x through line.x+advance at baseline+center.
Whitespace and signed tracking are included. A zero-advance line returns an
owned empty path. Glyph-only paths, glyph counts and layout metrics are unchanged.

Geometry arguments and resulting coordinates must remain within the native
1,000,000-pixel envelope; thickness must be positive. Missing layout/output,
invalid index or geometry is INVALID_ARGUMENT; memory failure is OUT_OF_MEMORY.
Failures clear output. Destroy returned paths independently; they outlive fonts
and layouts. No VM/GC state is accessed. Independent immutable layout readers
can run concurrently, but each output path belongs to its caller.
`text_layout_underline.c` checks exact rectangle bounds, alignment, tracking,
empty trailing lines, invalid parameters and lifetime after source destruction.

## Uniform AS2 fields (v0.839)

TextFormat underline remains null on construction. Applied values use AS2
ToBoolean; null/undefined preserve the existing style. Uniform current/future
field snapshots expose actual Boolean values. setTextFormat toggles current
geometry and invalidates its cache; setNewTextFormat affects later text only.
The current font must supply usable underline metrics before format admission.

[`fvm_textfield_path`](../include/flashvm/standard_text.h#L33) appends one bar per positive-advance line after glyph paths,
using the same field gutter and wrapped/unwrapped alignment. Bars include spaces
and signed letter spacing. textWidth/textHeight/getTextExtent advances and
baseline spacing remain unchanged; glyph-only paths remain undecorated. The
combined owned path outlives the VM and font. The UNDERLINE bit in private
packed field alignment preserves both styles in snapshots and binary/channel
transfer without extra payload bytes. Absent legacy flags restore false; older
readers reject the new flag. This is uniform underline, not styled ranges,
hyperlink behavior, full SWF display integration or Flash pixel equivalence.

v0.840: `textfield_underline_transfer.cpp` checks current/future binary and channel transfer, unchanged wire size, legacy false restoration, receiver GC and source destruction. `textfield_underline_alignment.cpp` checks both wrapped and unwrapped left/center/right placement on multiple lines with signed tracking and a trailing empty line.

v0.841: `textfield_underline_missing.cpp` edits the fixture post thickness to zero/negative values and verifies explicit UNSUPPORTED from metrics, current/future format admission and extent measurement without field changes. `textfield_underline_allocation.cpp` sweeps current/future admission and decorated path allocation boundaries, checks cleared failed outputs, balanced GC state and immediately usable fields. Fault injection is ordinary-build evidence.

v0.842: `jit_textfield_underline.cpp` drives AVM1 SetMember/CallMethod through interpreter, attached native code and automatic JIT. It checks the exported five-command bar and unchanged metrics, native entry counts and GC scope balance. `text_underline_os_ports.cpp` shares one immutable layout across four actual OS workers after font destruction; 128 iterations per worker export independently parameterized owned bars.

v0.843: the [C89 tutorial](../examples/embedding/text_layout/README.md) adds --underline, copied font metrics, independent bars and an actual RGBA pixel oracle at (1,40), alpha 255. Original glyph/bidi metadata and metrics stay identical; three bars add 15 commands. New demonstrated APIs include [`fvm_path_append`](../include/flashvm/path.h#L67) (copy temporary bars with an identity transform, release source after return) and [`fvm_path_count`](../include/flashvm/path.h#L56) (allocation-free command inspection). Paths outlive font/layout; no VM, window or device is required.

## Overlapping ink and fill rules (v0.844)

Line bars match the aggregate control-polygon winding of nondegenerate glyph
outlines, so nonzero fill does not cancel ordinary glyph/bar overlaps. Conflicting
orientations within a layout return UNSUPPORTED rather than selecting an arbitrary
bar direction. Direction is captured once during layout construction, including a shared fallback
for whitespace-only lines. Fully degenerate layouts use the default orientation.
Fill combined TextField/tutorial paths with the nonzero rule. Even-odd filling
has XOR overlap semantics and is not an ink union. Unusual self-intersecting or
inconsistent font contours still require reference acceptance.
A one-pixel baseline step also checks intersecting bars on an ink line and a
whitespace-only line, so negative-leading overlap does not cancel decoration.
The reproducible `geometry_mixed.ttf` fixture reflects V while preserving its
rectangle bounds; the same test checks explicit conflicting-orientation rejection.
`text_layout_underline_overlap.c` places a bar across the original A stem and
counter and checks filled stem, filled counter, new exterior coverage and an
unfilled point outside the bar, without relying on a graphics backend.

## Verification at v0.844

Release text-jit: 48 font/text/GC/transfer/JIT/tutorial tests pass. Full-core C/C++
ASan/UBSan: nine underline runtime tests pass without injected allocation exceptions.
NO_FONTS: seven API/standard-text tests pass. Allocation-fault sweeps run in the
ordinary build. Both opposite current/future underline selections survive binary
round trips. Both CC0 font fixtures regenerate byte-identically, including in a
renamed independent component from an unrelated working directory. The C89
example checks real RGBA coverage, not a physical display or original Flash player.
All nine roadmap stages and physical platform/device acceptance remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_font_underline_metrics` | function | [include/flashvm/font.h:63](../include/flashvm/font.h#L63) |
| `FVM_FontUnderlineMetrics` | struct | [include/flashvm/font.h:32](../include/flashvm/font.h#L32) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_path_append` | function | [include/flashvm/path.h:67](../include/flashvm/path.h#L67) |
| `fvm_path_count` | function | [include/flashvm/path.h:56](../include/flashvm/path.h#L56) |
| `fvm_text_layout_line_underline_path` | function | [include/flashvm/text_layout.h:117](../include/flashvm/text_layout.h#L117) |
| `fvm_textfield_path` | function | [include/flashvm/standard_text.h:33](../include/flashvm/standard_text.h#L33) |
<!-- source-links:end -->
