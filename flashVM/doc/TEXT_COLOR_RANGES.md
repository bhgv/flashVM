# Colour ranges from one TextField layout

[Русский](TEXT_COLOR_RANGES.ru.md) · [Asynchronous rendering](RENDER_ASYNC.md) ·
[Public declarations](../include/flashvm/standard_text.h)

[`fvm_textfield_path_ranges`](../include/flashvm/standard_text.h#L119) exports selected glyph contours from the same cached
whole-field layout used by [`fvm_textfield_path`](../include/flashvm/standard_text.h#L91). Hosts can paint independently
coloured paths without reshaping each token, changing advances, splitting a
ligature or maintaining a second source buffer. This is a native presentation
extension. AS mixed-range TextFormat, mixed fonts and HTML are not implemented
by this API.

| API/type | Contract |
| --- | --- |
| [`FVM_TextPathRange`](../include/flashvm/standard_text.h#L99) | Half-open UTF-8 byte interval in counted field source; not AS UTF-16 or Dis scalar coordinates |
| [`FVM_TEXT_PATH_RANGE_LIMIT`](../include/flashvm/standard_text.h#L95) | At most 1024 borrowed intervals per call |
| [`fvm_textfield_path_ranges`](../include/flashvm/standard_text.h#L119) | Export detached selected glyph contours; caller owns successful output |
| [`fvm_textfield_path`](../include/flashvm/standard_text.h#L91) | Existing complete uniform contours, including uniform underline |

The matching context and field must be live/reachable on their VM owner thread.
The descriptor array is borrowed only during the call and is never retained.
Intervals must be nonempty, sorted, nonoverlapping, within the counted source,
and aligned to UTF-8 scalar boundaries. Adjacent intervals are permitted. Zero
count accepts a null array and returns an empty owned path after ordinary layout
validation. The source may contain embedded NUL; the byte extent is counted.

Membership uses each shaped glyph's source **cluster start**. A boundary inside
a shaping cluster does not split the glyph: the interval containing its cluster
start owns it. This also avoids assuming that visual bidi order is source order;
each glyph searches the sorted interval table independently. Applications should
use grapheme/cluster boundaries when they need one colour per displayed cluster.

Placement, whole-field kerning, wrapping, alignment and effective scroll remain
shared. The export includes the field-local gutter and scroll translation, with
no viewport clipping; hosts apply their usual world transform and clip once.
Uniform underline is excluded from selected paths to avoid painting a complete
underline in every colour. Draw decorations separately or use the existing full
export. Source/style, caret, selection, Undo and saved state are not modified.
Successful observation may clamp stored scroll requests, matching the existing
full export. Failure leaves scroll requests unchanged and clears the output.

Invalid context/object/output/ranges/count/bounds return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47).
Font/layout/allocation errors propagate, including unsupported no-font profiles.
No script callbacks, channels or device work run during export. Serialize all
field/layout mutation and GC. Destroy successful paths with [`fvm_path_destroy`](../include/flashvm/path.h#L64);
font/VM handles must not accompany them to raster workers. The detached native
contours may be copied into immutable render jobs and survive source replacement.

The IDE maps revision-bound Dis scalar spans to UTF-8 byte ranges on the owner,
then batches native contours by palette class. Selection is painted first, glyph
colours next and the caret last. Colour-only publication keeps the completed
input/view ticket and uses the existing detached dirty-tile raster pipeline.
It does not acknowledge an input edit or wait for a legacy Flash timeline frame.
[IDE lexical service](../../ide/SYNTAX-LAZY.md) describes that separate owner.

[Actual range tests](../tests/textfield_path_ranges.cpp) check whole-layout
partition, UTF-8 boundary/bounds rejection, quotas, empty output and detached
ownership. Existing underline/scroll/counting and JIT field regressions remain
separate compatibility checks. This closes no Flash compatibility or physical
platform milestone.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `fvm_path_destroy` | function | [include/flashvm/path.h:64](../include/flashvm/path.h#L64) |
| `FVM_TEXT_PATH_RANGE_LIMIT` | macro / define | [include/flashvm/standard_text.h:95](../include/flashvm/standard_text.h#L95) |
| `fvm_textfield_path` | function | [include/flashvm/standard_text.h:91](../include/flashvm/standard_text.h#L91) |
| `fvm_textfield_path_ranges` | function | [include/flashvm/standard_text.h:119](../include/flashvm/standard_text.h#L119) |
| `FVM_TextPathRange` | struct | [include/flashvm/standard_text.h:99](../include/flashvm/standard_text.h#L99) |
<!-- source-links:end -->
