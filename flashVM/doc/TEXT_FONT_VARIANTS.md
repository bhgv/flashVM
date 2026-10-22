# Explicit embedded font variants

v0.869 adds [fvm_standard_text_register_font_variant](../include/flashvm/standard_text.h#L35)(context, family, variant,
bytes, size, face_index). It validates and copies a real face into one family slot:
[FVM_TEXT_FONT_REGULAR](../include/flashvm/standard_text.h#L10) (zero), [FVM_TEXT_FONT_BOLD](../include/flashvm/standard_text.h#L11), [FVM_TEXT_FONT_ITALIC](../include/flashvm/standard_text.h#L12) or their
bitwise combination. The host supplies the intended face; flags bind a slot and
do not synthesize outlines or infer a style from font metadata. The original
[fvm_standard_text_register_font](../include/flashvm/standard_text.h#L25) registers the regular slot unchanged.

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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_standard_text_register_font` | function | [include/flashvm/standard_text.h:25](../include/flashvm/standard_text.h#L25) |
| `fvm_standard_text_register_font_variant` | function | [include/flashvm/standard_text.h:35](../include/flashvm/standard_text.h#L35) |
| `FVM_TEXT_FONT_BOLD` | macro / define | [include/flashvm/standard_text.h:11](../include/flashvm/standard_text.h#L11) |
| `FVM_TEXT_FONT_ITALIC` | macro / define | [include/flashvm/standard_text.h:12](../include/flashvm/standard_text.h#L12) |
| `FVM_TEXT_FONT_REGULAR` | macro / define | [include/flashvm/standard_text.h:10](../include/flashvm/standard_text.h#L10) |
<!-- source-links:end -->
