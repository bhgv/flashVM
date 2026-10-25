# Explicit embedded font variants

v0.869 adds [fvm_standard_text_register_font_variant](../include/flashvm/standard_text.h#L41)(context, family, variant,
bytes, size, face_index). It validates and copies a real face into one family slot:
[FVM_TEXT_FONT_REGULAR](../include/flashvm/standard_text.h#L10) (zero), [FVM_TEXT_FONT_BOLD](../include/flashvm/standard_text.h#L11), [FVM_TEXT_FONT_ITALIC](../include/flashvm/standard_text.h#L12) or their
bitwise combination. The host supplies the intended face; flags bind a slot and
do not synthesize outlines or infer a style from font metadata. The original
[fvm_standard_text_register_font](../include/flashvm/standard_text.h#L29) registers the regular slot unchanged.

Arguments require a live owner-thread context, a nonempty terminated family name
of at most 255 bytes, supported flags and readable face data/index. Input bytes
are borrowed during registration and independently owned after success. Duplicate
family/variant and unknown bits return INVALID_ARGUMENT. Font validation errors
propagate. Allocation failure returns OUT_OF_MEMORY without publishing a slot.
The existing limit of 16 registered faces and 64 MiB aggregate source bytes counts
all variants together; exceeding it returns UNSUPPORTED. NO_FONTS returns
UNSUPPORTED for a valid registration request. Calls during GC/teardown are invalid;
there is no implicit system lookup, callback or cross-thread synchronization.
Only the first regular registration selects the default family. Styled-only
families do not silently become regular defaults. Context destruction releases
all copied sources. Registrations cannot replace existing family/style slots.

Private counted variant keys start with NUL, isolating them from public C family
names. Native field snapshots carry public family names, never those private keys.
Full SWF embedded-font loading, device/system fonts, fallback, styled spans and
complete Flash conformance remain open. All roadmap stages remain open.

[Adobe AS2 TextFormat reference mirror](https://open-flash.github.io/mirrors/as2-language-reference/TextFormat.html)
requires separate embedded faces for bold/italic. This implementation uses explicit
host registration to provide those faces; physical devices/platforms are separate.

## Native TextFormat selection — v0.870

bold/italic now use the existing primitive Boolean conversion and independently
select their family-slot bits. Null/undefined properties retain the previous
selection when merging a format. Field getTextFormat/getNewTextFormat snapshots
return actual Boolean selections; a new unattached TextFormat still starts null.
Current/future format changes validate the selected face before publication.
Missing variants return NOT_FOUND without changing text, formatting or geometry.
Both measurement and glyph/underline export use the same precomputed face key.
No mutable font handles are retained across calls, and no layout-time key
allocation or system fallback is introduced.

Native graph snapshots pack bold/italic into separate style bits, embedding each
selected current/future face. No new payload numbers or public structure layout
changes are needed. Legacy records restore regular; old readers reject new bits.
Same family/style already present in a receiver must match source bytes/index;
other styles in that family remain independent. Full old-reader compatibility is
not promised. The owner-thread, GC and rollback contracts remain unchanged.

v0.871 adds three original CC0 styled faces with independent AV advances at 40px: regular 48, bold 56, italic 52, bold-italic 60. Field/TextFormat metrics and exported bounds prove actual source selection; enabling kerning subtracts 3.2 for each face. Registry tests also exercise the aggregate 16-face limit across all family slots.

v0.872 verifies bold current/italic future transfer through binary graphs and channels, source teardown, receiver GC and later auto-size edits. Only selected faces transfer; switching to absent regular remains NOT_FOUND. A conflicting future source rolls back an earlier current-face import. Four actual faces drive wrapped line counts and underline paths: outer width 60 gives heights 94/94/94/186; width 64 gives 94 for every face.

v0.873 adds eight ordinary-build C++ allocation sweeps: four registration slots and current/future format, assignment and replaceText edits. Failed registration preserves the map/default; failed field edits preserve text, selected faces and anchored geometry, with balanced GC/native scopes. Existing replaceText on nonempty uniform text preserves the current face; ordinary text assignment uses the future face. Bundled C-library malloc failures are not injected.

v0.874 drives real AVM1 bold/italic writes through interpreter, attached native code and automatic JIT, checking all four independent face widths. Four OS workers own their runtime/context/source map and perform 64 text edits/GC passes each, retaining the right anchor and selecting one distinct face per worker.

v0.875 adds --font-variants to the C89 standard-text tutorial: three explicit registrations, ordinary Boolean TextFormat selection, exact 48/56/52/60 advances and independently owned final path export. Every demonstrated API has an individual ownership/status/argument contract and the executable checker covers the new mode.

## Verification — v0.876

Release text-jit: 85 font/text/GC/graph/JIT/tutorial checks pass, including ten new
font-family checks. Seven new success/validation checks pass with full-core
ASan/UBSan. Ten NO_FONTS API checks pass; both C89 text tutorials compile there.
Eight new C++ allocation-failure sweeps run in the ordinary build. Family-name
boundaries exercise 255-byte private keys and reject 256-byte public names.
All six original font/header pairs reproduce byte-for-byte in two isolated runs.
A renamed detached component configures, registers its own checker and runs the
copied checker against the tested native binary. This is configuration/path
evidence, not another full detached native build or physical-device execution.

The exceptional conflicting-graph oracle runs separately in the ordinary build.
After graph rejection, this environment's ASan runtime warns that the external
__asan_handle_no_return hook was ignored and false reports may follow, then
reports a later FreeType stack write; that path
is not claimed as ASan acceptance. ASan success/validation coverage excludes
that exception oracle and injected allocation failures. Leak checking is also
unavailable here. The ordinary oracle verifies complete source-map rollback and
subsequent successful registration, with balanced GC/native scopes.

Real embedded variants are a bounded native bridge. SWF font extraction,
style-name discovery, fallback/system fonts, mixed styled spans, HTML, full Flash
reference equivalence and physical platforms/devices remain open. All nine
roadmap stages remain open; version numbers do not certify SWF≤8 compatibility.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_standard_text_register_font` | function | [include/flashvm/standard_text.h:29](../include/flashvm/standard_text.h#L29) |
| `fvm_standard_text_register_font_variant` | function | [include/flashvm/standard_text.h:41](../include/flashvm/standard_text.h#L41) |
| `FVM_TEXT_FONT_BOLD` | macro / define | [include/flashvm/standard_text.h:11](../include/flashvm/standard_text.h#L11) |
| `FVM_TEXT_FONT_ITALIC` | macro / define | [include/flashvm/standard_text.h:12](../include/flashvm/standard_text.h#L12) |
| `FVM_TEXT_FONT_REGULAR` | macro / define | [include/flashvm/standard_text.h:10](../include/flashvm/standard_text.h#L10) |
<!-- source-links:end -->
