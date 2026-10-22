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

v0.849: `jit_textfield_autosize.cpp` drives actual AVM1 string/Boolean mode writes and text edits in interpreter, attached native code and automatic JIT, checking measured dimensions, native entry counts, unsupported-anchor propagation and GC scopes. `text_autosize_os_ports.cpp` runs four actual OS workers, each owning its runtime/context/font/field, with 64 edits and GC passes. No mutable VM state is shared.

v0.850: the [C89 tutorial](../examples/embedding/standard_text/README.md) demonstrates --auto-size with exact 52x48, width 76 and wrapped 30x140 observations, followed by independent path export. The executable checker validates all dimensions and early invalid-option rejection. The AS companion remains uncompiled host-integration source.

v0.851: restored automatic dimensions must match a fresh measurement within one 1/64-pixel font subpixel; mismatches are INVALID_ARGUMENT before native attachment. Only retired rootless built-in autoSize accessors are upgraded, both in native restore and graph callback restoration. Captured roots/custom userdata remain untouched. `textfield_autosize_restore.cpp` checks legacy stubs, malformed dimensions and host-capture preservation.

v0.852: `textfield_autosize_limits.cpp` exercises large-font paragraphs beyond native width/height limits, verifies unchanged automatic fields on rejected edits, rejects enabling automatic mode over already-stored oversized manual text and proves recovery after valid text replacement. Field extent/gutter limits are scoped constants shared by measurement and lazy layout.

## Verification at v0.852

Release text-jit: 58 font/text/GC/transfer/JIT/tutorial tests pass. Full-core C/C++
ASan/UBSan: eight new runtime tests pass without injected allocation exceptions.
NO_FONTS: seven API/standard-text tests pass. Six allocation-failure sweeps run
in the ordinary build. A renamed detached component configures and registers its
own tutorial checker; the copied checker runs against the tested native binary.
This is a path/configuration check, not another full detached native build.

The native policy bit adds no state payload bytes, but new autoSize getter/setter
selectors also require updated callback tables: old readers are not guaranteed
to accept new field graphs even with autoSize disabled. New-reader restoration
of retired built-in stubs is explicitly tested. All nine roadmap stages,
center/right anchors, full Flash display/reference and physical-device acceptance
remain open.

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
