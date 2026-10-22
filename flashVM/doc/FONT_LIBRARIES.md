# Static fonts and editable vector outlines

FONTS is enabled in DEFS.txt; select NO_FONTS to omit font libraries and their
programs. All bundled libraries are explicit STATIC CMake targets, including
AGG/Cairo, image codecs, zlib and SQLite. BUILD_SHARED_LIBS=ON cannot change them
to shared libraries; configuration checks every selected bundled target type.
System runtimes, threads and OS frameworks are not vendored library targets.
There are no configure/build downloads or system font-library fallbacks.

| Dependency | Pin | Selected profile |
| --- | --- | --- |
| FreeType | 2.14.3 | ANSI C sources, stdio/malloc adapter, bundled zlib |
| HarfBuzz | 1.8.8 + recorded C++98 adaptations | OpenType, fallback, built-in UCDN, FreeType integration |
| libunibreak | 6.1 + C89 adaptations | line, word and grapheme break APIs |
| FriBidi | 1.0.16 + C89 adaptations | Unicode 16.0 bidi algorithm, bundled release tables |

HarfBuzz 10.4.0 explicitly requests C++11 (C++14 for some compiler configurations)
in its upstream meson.build; it was not selected. The 1.8.8 pin is a compatibility
choice, not a claim of current Unicode coverage or a complete upstream/security
audit. Its OpenType shapers and bundled Unicode data retain that historical
version's behavior. libunibreak 6.1 documents Unicode 15.0 line breaking and 15.1
word/grapheme breaking, with incomplete 15.1 line-breaking rules. Different
libraries' Unicode datasets are not a unified latest-Unicode implementation.

Native clients link to the flashvm target and inherit all selected public header
paths and static dependencies: ft2build.h/FreeType headers, hb.h/hb-ot.h/hb-ft.h,
linebreak.h/wordbreak.h/graphemebreak.h, fribidi.h. The FreeType bridge in AGG is
included with GRAPHICS_AGG+FONTS; Cairo's FreeType backend is enabled whenever
Cairo is built with FONTS. Fontconfig/native font discovery is disabled: hosts
supply font bytes or native FT_Face handles explicitly. FreeType's PNG, bzip2 and
Brotli integrations are disabled. HarfBuzz calls FreeType, while FreeType does not
call HarfBuzz, avoiding a static dependency cycle.

## FlashVM C89 embedding API

flashvm/font.h owns font bytes, a face and an unhinted pixel size. Font input is
copied, bounded to 32 MiB; size is (0,4096] pixels. Font handles are mutable and
must not be used concurrently. Independent handles work on independent threads;
HarfBuzz uses pthread mutexes on POSIX and native locking on Windows. Font loads
require a Unicode character map. The facade exports outline glyphs, not embedded
bitmap/SVG/color glyph rendering. More specialized operations remain accessible
through the libraries' native APIs.

| Operation | API / behavior |
| --- | --- |
| Font metrics and character mapping | [fvm_font_metrics](../include/flashvm/font.h#L56) / [fvm_font_char](../include/flashvm/font.h#L64); missing mapping returns glyph 0 |
| Independent glyph geometry | [fvm_font_glyph](../include/flashvm/font.h#L68) copies lines/quadratic/cubic curves and closes contours |
| OpenType shaping | [fvm_font_shape](../include/flashvm/font.h#L74) returns glyph IDs, advances, offsets and UTF-8 byte clusters |
| Positioned text contours | [fvm_text_run_path](../include/flashvm/font.h#L94) combines positioned glyph contours in a new path |
| Allowed Unicode breaks | [fvm_text_breaks](../include/flashvm/font.h#L100) outputs native libunibreak classifications per input byte |
| One-line bidi resolution | [fvm_text_bidi](../include/flashvm/font.h#L115) returns resolved direction, logical levels and visual→logical map |

Positions are pixels, x right/y down. FreeType's scaled unhinted outlines use 26.6
precision; exported double coordinates do not add precision that was absent in
the font engine. Metrics ascent/descent/line-height are FreeType size metrics,
including its rounding. FreeType clamps character-size requests below one
pixel; transform exported vectors for smaller geometry. Logical advances differ
from geometric ink bounds.

[fvm_font_shape](../include/flashvm/font.h#L74) accepts one homogeneous script/direction run of valid UTF-8,
bounded to 4 MiB. AUTO guesses run properties; it does not lay out a multilingual
paragraph. Bidi takes UTF-32 Unicode scalar values, at most 1,000,000 entries;
its visual output reorders code points without mirroring or Arabic presentation
form substitution. HarfBuzz must shape logical script/direction runs, then the
host arranges those runs visually; do not simply shape the reordered code points.
Levels remain in logical paragraph order. Use native FriBidi APIs to resolve
individual wrapped lines and mirroring as needed. Text runs retain copied glyph
positions after closing the font, but exporting their outlines requires the same
font/face/size used for shaping. Input/output arrays must not partially overlap.

libunibreak identifies legal break positions, not the width at which a line fits.
Choosing lines, fallback fonts, alignment, line spacing, justification and text
block grouping remain host layout policy. A ligature or combining sequence does
not necessarily map one character to one glyph; preserve the cluster mapping
when selecting/editing text. TextField/ActionScript integration and complete SWF
text layout are not declared complete by this checkpoint.

## Owned vector model

flashvm/path.h stores MOVE, LINE, QUADRATIC, CUBIC and CLOSE commands in FlashVM's
own vector container, without STL containers. A path survives font destruction.
Each command exposes its endpoint/control coordinates for editing. Point counts
are 1/1/2/3/0 respectively; quadratic points are control/end, cubic points are
control/control/end. Only those coordinates need initialization. Paths begin
with MOVE; CLOSE requires an open contour. Begin a new MOVE after closing.

Affine matrices support translation, scale, rotation, reflection and shear.
Transform and append are transactional, reject nonfinite/out-of-range results,
and append copies contours. Keep per-glyph/per-word paths to transform groups
independently; combine copies for a line/block. Arbitrary point edits allow
shape distortion, but a nonlinear warp is not implemented as a separate API.
[fvm_path_bounds](../include/flashvm/path.h#L71) computes Bezier extrema, not the control-point rectangle, using
double arithmetic. It includes MOVE positions and excludes stroke thickness.
Coordinates are bounded to +/-1,000,000 and command count to 1,000,000.

[fvm_path_draw](../include/flashvm/path.h#L81) renders curves through the selected AGG/Cairo backend: nonzero or
even/odd fill, or a positive-width stroke with round joins/caps. Quadratic curves
are converted exactly to cubic control points for Cairo, and retained natively
for AGG. Rendering shares raster.h's 4096-pixel surface/coordinate bounds. Both
backends approximate curves for rasterization; low-alpha rounding can differ.
NO_GRAPHICS still supports vector/font geometry but drawing returns UNSUPPORTED.

[fvm_path_mask](../include/flashvm/path.h#L86) uses filled contours as an antialiased coverage mask for an equally
sized RGBA8 image, compositing source-over the destination. Render a vector scene
into that source image to use it as a vector-content mask. Source/destination can
have identical pixel mappings; other overlapping buffer mappings are unsupported.
Row padding is preserved. These are native vector APIs, not a SWF shape decoder.

## Examples, licensing and validation

examples/embedding/font_libraries demonstrates shaping → path export → shear →
point edit → fill/stroke/image mask using the committed CC0 geometric test TTF.
The fixture contains a hole, quadratic curve, composite glyph, fi ligature,
AV GPOS kerning, Hebrew and Arabic mappings/positional substitutions. The normal
build needs no installed fonts or fontTools. scripts/generate_test_font.py needs
fontTools only for developer regeneration.

All upstream license notices are retained. FreeType: FTL or GPL alternative;
HarfBuzz: Old MIT plus per-component notices (UCDN ISC); libunibreak: zlib/libpng
plus Unicode data notices; FriBidi: LGPL-2.1-or-later. Portions of this software
are copyright © 2026 The FreeType Project (https://freetype.org). All rights reserved.
The empty-buffer position API also has a recorded null-memset fix and a
direct regression test; the sanitizer driver fails on recovered diagnostics.
PROVENANCE.json records archive hashes and complete local source changes in
flashvm.patch. C89/C++98 modes stay strict; supported native integer/compiler
facilities remain necessary for upstream internals. See
[checkpoint validation](FONT_LIBRARIES_CHECKPOINT.md).

Native owned paragraph/line layout: [TEXT_LAYOUT.md](TEXT_LAYOUT.md).

## Bidi admission and output lifetime (v0.827)

[`fvm_text_bidi`](../include/flashvm/font.h#L115) accepts one borrowed UTF-32 paragraph of at most 1,000,000
Unicode scalars and a valid AUTO/LTR/RTL direction. Invalid enum representations
from C are checked without a C++ enum load. Invalid scalars, missing required
arrays and over-limit lengths return INVALID_ARGUMENT before output changes.
NO_FONTS performs the same admission then returns UNSUPPORTED.

Direction, visual text, visual-to-logical indices and logical embedding levels
are published only on success. Empty input permits null arrays; enabled fonts
resolve AUTO to LTR. Logical and visual arrays may coincide; other output arrays
must be disjoint. No input/output memory is retained. Independent buffers need no
VM, GC roots or shared font handle. `text_bidi_admission.c` checks these boundaries.
This does not complete Unicode layout or SWF acceptance.

v0.828: `text_bidi_allocation.cpp` sweeps C++ allocation boundaries, checks unchanged in-place input and all outputs on OOM, then retries against explicit RTL order/map/logical-level expectations. Native allocation faults are checked in the ordinary build.

v0.829: `text_bidi_os_ports.cpp` runs four actual OS workers for 256 paragraphs each with independent buffers and explicit LTR/RTL maps and logical levels. No shared mutable font or VM is required. This is native concurrency evidence, not device/platform acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_font_char` | function | [include/flashvm/font.h:64](../include/flashvm/font.h#L64) |
| `fvm_font_glyph` | function | [include/flashvm/font.h:68](../include/flashvm/font.h#L68) |
| `fvm_font_metrics` | function | [include/flashvm/font.h:56](../include/flashvm/font.h#L56) |
| `fvm_font_shape` | function | [include/flashvm/font.h:74](../include/flashvm/font.h#L74) |
| `fvm_path_bounds` | function | [include/flashvm/path.h:71](../include/flashvm/path.h#L71) |
| `fvm_path_draw` | function | [include/flashvm/path.h:81](../include/flashvm/path.h#L81) |
| `fvm_path_mask` | function | [include/flashvm/path.h:86](../include/flashvm/path.h#L86) |
| `fvm_text_bidi` | function | [include/flashvm/font.h:115](../include/flashvm/font.h#L115) |
| `fvm_text_breaks` | function | [include/flashvm/font.h:100](../include/flashvm/font.h#L100) |
| `fvm_text_run_path` | function | [include/flashvm/font.h:94](../include/flashvm/font.h#L94) |
<!-- source-links:end -->
