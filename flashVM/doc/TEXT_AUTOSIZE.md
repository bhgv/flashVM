# Left-anchored TextField automatic sizing

TextField autoSize supports "none" (default), "left", false and true. Booleans
select none/left. The [Adobe AS2 reference](https://open-flash.github.io/mirrors/as2-language-reference/TextField.html)
describes the fixed left anchor and the height-only behavior with wordWrap.
Center/right remain UNSUPPORTED because native field positioning is incomplete;
other values are also UNSUPPORTED. This is a bounded native-field subset.

Enabling left measures current text immediately. Without wordWrap the outer
width becomes textWidth+4; height becomes textHeight+4. With wordWrap the width
stays fixed and only height fits content. Current-format changes, text assignment
and replaceText measure before publishing text/style/dimensions. The prepared
layout is retained as the field cache. Failure leaves previous field state intact.
Future-format changes do not resize current text; subsequent edits apply them.

Native wrapping currently requires multiline and wordWrap together; existing
single-line newline rejection, Unicode, font and shaping bounds still apply.
Policy toggles and wrapped width changes remeasure automatic fields. Explicit
height writes and unwrapped width writes while left is active are UNSUPPORTED;
disable autoSize first for arbitrary manual dimensions. Disabling retains the
last dimensions. TextFormat alignment remains independent of the fixed origin.

All calls run on the owning context thread. Normal native-call roots protect
arguments and fields; prepared geometry owns partial paths until publication.
Exported paths remain independent. No device/window is required. `textfield_autosize.cpp`
checks analytical 40px AV (48px advance, 52x48 outer dimensions), edits, format
changes, Boolean modes, unsupported anchors and GC. Full Flash reference/display
and physical platform/device acceptance remain open.
Native implementation uses `PreparedGeometry` and `prepare_field_geometry` before
[`fvm_textfield_create`](../include/flashvm/standard_text.h#L27) fields publish edited state; [`fvm_textfield_path`](../include/flashvm/standard_text.h#L33) exports
independent geometry through the existing public API.

v0.846: snapshots and binary/channel transfers preserve auto-size in a free multiline
policy bit without extra bytes. Legacy 0/1 restores manual policy; old readers reject
the new bit. Restored automatic fields are remeasured before publication.
`textfield_autosize_transfer.cpp` checks source destruction, receiver GC, future
formatting and exact wire length. v0.847 adds `textfield_autosize_wrap.cpp` and
`textfield_autosize_formats.cpp`: width-constrained height, policy rollback,
kerning/tracking/underline composition, NUL, empty text and insertion.

v0.848: `textfield_autosize_allocation.cpp` sweeps six C++ allocation paths: enabling automatic mode, text assignment, replaceText, current formatting, wordWrap and wrapped width changes. Failed calls preserve text/mode/dimensions and balanced GC frames/native depth; successful calls publish measured geometry. Fault injection runs in the ordinary build.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_textfield_create` | function | [include/flashvm/standard_text.h:27](../include/flashvm/standard_text.h#L27) |
| `fvm_textfield_path` | function | [include/flashvm/standard_text.h:33](../include/flashvm/standard_text.h#L33) |
<!-- source-links:end -->
