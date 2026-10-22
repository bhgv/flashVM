# Optional ligatures and tracking

v0.861 adds [FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES](../include/flashvm/font.h#L10) to [fvm_font_shape_configured](../include/flashvm/font.h#L90).
It disables the OpenType liga and clig features for the complete input run.
Required rlig substitutions, joining and mark positioning retain their defaults.
[FVM_FONT_SHAPE_NO_KERNING](../include/flashvm/font.h#L9) remains independent; bitwise OR selects both policies.
Flags zero and [fvm_font_shape](../include/flashvm/font.h#L76) preserve legacy shaping. Flags are per call and do
not mutate the font's persistent state. This is OpenType feature selection, not
a promise to split every script cluster or every font technology.

The API borrows font/text/language only during the call and returns an owned
[FVM_TextRun](../src/capi/font_c.cpp#L75) released by [fvm_text_run_destroy](../include/flashvm/font.h#L94). Use the same font/size when exporting
its path with [fvm_text_run_path](../include/flashvm/font.h#L98). Mutable font handles require caller serialization;
independent handles may run on separate threads. No VM, GC, callback or device
is used. Unknown flags, invalid UTF-8/direction/input are INVALID_ARGUMENT and
clear a valid output pointer. Allocation failure is OUT_OF_MEMORY. NO_FONTS
validates flag/direction representations then returns UNSUPPORTED for valid flags.
The existing bounded shaping and outline coordinate limits remain unchanged.

The original CC0 fixture independently gives fi=28px at 40px with a ligature,
f+i=24px without it, and AV=44.8px with kerning versus 48px without. Tests exercise
all four flag combinations and verify that later legacy calls retain ligatures.

[HarfBuzz feature documentation](https://harfbuzz.github.io/shaping-opentype-features.html)
describes whole-buffer feature overrides. Flash reference conformance and actual
platform/device acceptance remain separate; all roadmap stages remain open.

## Tracking policy — v0.862

[fvm_text_layout_create_spaced](../include/flashvm/text_layout.h#L94) automatically adds the optional-ligature flag when
spacing is nonzero. The same effective flags are used for trial wrapping and
final lines, including paths and underline advances. Zero spacing retains the
caller's configured flags exactly. [fvm_text_layout_create_configured](../include/flashvm/text_layout.h#L80) accepts the
new flag even without tracking. Existing output cleanup, borrowing, independent
layout ownership, thread serialization and INVALID_ARGUMENT/OUT_OF_MEMORY/
UNSUPPORTED contracts in text_layout.h apply unchanged.

Tracking still requires one glyph per scalar, distinct clusters and nonnegative
adjusted advances. Required ligatures, combining clusters and other ambiguous
runs remain UNSUPPORTED. No codepoint-by-codepoint shaping fallback is used.
TextField and TextFormat use this same layout path, so genuine fi text edits now
work with letterSpacing; failed combining-cluster edits still preserve prior state.
Current/future formatting and graph wire format are unchanged.

v0.863 adds a separate deterministic CC0 GSUB oracle: optional clig ff decomposes under the new flag; required rlig VA remains one glyph under every flag combination and nonzero tracking rejects it. Existing ordinary/mixed fixture bytes remain unchanged.

v0.864 verifies TextFormat extents, current/future tracking, centered auto-size, binary/channel transfer after source destruction, wrapped right anchoring, underline ownership and combining-cluster rollback. Expected fi at 40px/+1 is 26px advance and 30px outer width; later 20px/-1 fifi is 20px advance and 24px outer width. Wrapped 40px/+1 fifi at outer width 30 gives height 94; disabling wrapping restores width 56 while preserving the right edge.

v0.865 adds four ordinary-build C++ allocation sweeps: tracked fifi layout plus current-format, assignment and replaceText field paths. Failed construction clears outputs; failed field edits preserve counted text, spacing and measured geometry, with balanced GC/native scopes and reusable fonts. These sweeps do not inject failures into bundled C-library malloc sites.

v0.866 checks actual AVM1 SetMember/CallMethod/GetMember in interpreter, attached native code and automatic JIT. At 40px, fi widths are 28 with zero spacing, 30 with +3 and 20 with -2. Four OS workers independently alternate legacy shaping and tracked layouts for 64 passes, preserving one legacy fi glyph before each tracked call.

v0.867 extends the C89 text-layout tutorial with --no-ligatures (fi: two glyphs, 24.00px) and --ligature-tracking (two glyphs, 26.00px). Its executable checker observes both and the existing owned-path rasterization. Negative tracking permits a zero adjusted advance but rejects the next negative boundary.

## Verification — v0.868

Release text-jit: 75 font/text/GC/graph/JIT/tutorial checks pass, including nine
new feature/ligature checks. Seven new success/validation tests pass with full-core
ASan/UBSan; leak checking is unavailable here. Four new C++ allocation-failure
sweeps run in the ordinary build. Nine NO_FONTS API checks pass, and the C89
text-layout example builds in that profile. All three font/header pairs are
reproduced byte-for-byte by two isolated generator runs. A renamed detached
component configures and its copied tutorial checker passes against the tested
native binary: configuration/path evidence, not another full detached build.

The field graph format and public structure layout are unchanged. The new flags
extend feature admission; zero flags/zero spacing preserve prior glyph selection.
Full Flash reference conformance, complex-cluster tracking, SWF text loading,
platform/device testing and the other roadmap stages remain open. This series
does not certify full SWF≤8 compatibility.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_font_shape` | function | [include/flashvm/font.h:76](../include/flashvm/font.h#L76) |
| `fvm_font_shape_configured` | function | [include/flashvm/font.h:90](../include/flashvm/font.h#L90) |
| `FVM_FONT_SHAPE_NO_KERNING` | macro / define | [include/flashvm/font.h:9](../include/flashvm/font.h#L9) |
| `FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES` | macro / define | [include/flashvm/font.h:10](../include/flashvm/font.h#L10) |
| `fvm_text_layout_create_configured` | function | [include/flashvm/text_layout.h:80](../include/flashvm/text_layout.h#L80) |
| `fvm_text_layout_create_spaced` | function | [include/flashvm/text_layout.h:94](../include/flashvm/text_layout.h#L94) |
| `fvm_text_run_destroy` | function | [include/flashvm/font.h:94](../include/flashvm/font.h#L94) |
| `fvm_text_run_path` | function | [include/flashvm/font.h:98](../include/flashvm/font.h#L98) |
| `FVM_TextRun` | struct | [src/capi/font_c.cpp:75](../src/capi/font_c.cpp#L75) |
<!-- source-links:end -->
