# Bounded paragraph insets

v0.885 applies TextFormat `leftMargin`, `rightMargin` and `blockIndent` to the
uniform native TextField style and TextFormat.getTextExtent. Each accepts finite
integers in [0,4096] native pixels; null/undefined preserves the previous value.
Negative, fractional, NaN and infinite values return INVALID_ARGUMENT without
changing a field. First-line `indent` is now supported separately: [contract](TEXT_INDENT.md). Custom tabStops are supported separately: [contract](TEXT_TABS.md). Bullets remain unsupported.
This is a bounded uniform-style implementation, not full Flash conformance.

The left origin is leftMargin + blockIndent. Wrapped multiline layout subtracts
all three insets from its available width, clamped to the existing minimum inner
width so oversized clusters make progress. Left/center/right/justify operate in
that content box. Unwrapped alignment uses the same box. Glyphs and underline
paths receive the same inset translation. Field `_x`/`_y` stay separate from local
outlines. textWidth and extent.width report glyph advance, excluding insets;
unwrapped autoSize and extent.textFieldWidth include both insets plus the gutter.
Wrapped automatic width stays fixed; automatic height follows the new wrapping.
Empty text has natural empty-line height and fitting width equal to insets + gutter.

Current and future formatting remain independent. Full assignment uses the future
style; replacement retains the current uniform style except initial insertion into
an empty field. Admission and candidate geometry finish before publication. Fonts,
layout limits, UTF-8/UTF-16 restrictions and context ownership are unchanged.

Native graph state uses [FVM_TEXT_STYLE_HAS_INSETS](../src/standard/text_constants.h#L22) with three optional numbers after
optional letterSpacing. Legacy records default to zero; no wire version or public
C structure changes. Current/future values transfer separately. Unknown older
readers reject the new flag rather than silently discarding visible formatting.
Each style is validated for integer bounds and exact flag/payload consistency
before its own font registration; native attachment happens after both styles.
Already admitted context font sources follow the existing graph-import lifetime. Cached layouts are rebuilt, never shared.

Use [fvm_object_set](../include/flashvm/value.h#L74) on a TextFormat to store values, then [fvm_object_call](../include/flashvm/value.h#L126) with
setTextFormat or setNewTextFormat to apply them. These borrowed context-owned
objects must belong to the same context. The returned getTextFormat/getNewTextFormat
objects are managed snapshots; retain them with ordinary roots/pins across GC.
Calls run on the context owner thread and may allocate; OUT_OF_MEMORY leaves the
field's text, style and geometry unchanged. [fvm_textfield_path](../include/flashvm/standard_text.h#L48) returns an independent
owned local path, destroyed with [fvm_path_destroy](../include/flashvm/path.h#L52) and allowed to outlive the context.
No new C API is introduced. Native context concurrency still requires separate owners.

[Adobe TextFormat reference](https://open-flash.github.io/mirrors/as2-language-reference/TextFormat.html)
describes paragraph margins and whole-block indentation. Original Player reference
comparisons, full SWF font loading, real devices/platforms and all nine stages remain open.

v0.892 verification: 106 native/tutorial checks pass, including eight runtime/transfer/admission tests, a positional-constructor fixture and the C89 enabled/disabled profile test added by this series. Seventeen targeted ASan/UBSan checks pass (nine preceding justification checks plus eight inset/profile checks); six new C++ allocation-failure sweeps run in the ordinary build only. Thirteen NO_FONTS checks pass, and both C89 tutorials compile there. LeakSanitizer stays disabled because /proc is unavailable; leak acceptance is not claimed. Four OS workers each own a separate runtime, context and font sources and perform 32 edit/GC/path cycles. AVM1 checks interpreter, attached native and automatic JIT with native entry assertions. Source links and annotated example/catalog checks pass. A renamed detached component configures, and its copied checker passes against the native tutorial executable; a separate detached build is not claimed. Original Flash reference, full SWF font loading, first-line indent, bullets/tabStops, physical devices/platforms and all nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../include/flashvm/value.h#L126) |
| `fvm_object_set` | function | [include/flashvm/value.h:74](../include/flashvm/value.h#L74) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:52](../include/flashvm/path.h#L52) |
| `FVM_TEXT_STYLE_HAS_INSETS` | macro / define | [src/standard/text_constants.h:22](../src/standard/text_constants.h#L22) |
| `fvm_textfield_path` | function | [include/flashvm/standard_text.h:48](../include/flashvm/standard_text.h#L48) |
<!-- source-links:end -->
