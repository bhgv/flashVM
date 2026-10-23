# Uniform TextField scrolling

This series adds bounded viewport scrolling to host-created TextField objects.
The [Adobe AS2 reference](https://open-flash.github.io/mirrors/as2-language-reference/TextField.html)
defines one-based vertical visual-line indices and zero-based horizontal pixels.
This implementation uses the existing uniform-style native layout, not a Flash
Player binary oracle. HTML/mixed-font fields, onScroller/listener dispatch, input,
SWF display-list text integration and physical platform acceptance remain open.
All nine roadmap stages remain open.

[fvm_textfield_scroll_info](../include/flashvm/standard_text.h#L68) copies geometry on the owning context thread. Set exact
[FVM_TextFieldScrollInfo](../include/flashvm/standard_text.h#L44).struct_size and retain the managed object. It may build a
cached layout; no script, callback or device executes. Invalid descriptors return
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); layout/font/extent and allocation statuses propagate.
Failure preserves output and scroll offsets; a successfully built cache may remain.
Serialize field/context changes and GC. No pointer/root is retained in the result.

The inner viewport subtracts the four-pixel total gutter and clamps at zero.
Uniform baseline spacing is font ascent + descent + leading. maxscroll uses the
number of complete line bodies that fit, with a minimum of one line for tiny or
zero viewports; bottom_scroll follows that same policy. maxhscroll is the ceiling
of positive logical width overflow including paragraph insets. These are explicit
bounded geometry rules, not claims about every Flash rounding/version edge case.
Exported contours remain unclipped; the host uses the returned inner clip rectangle
and applies parent-space _x/_y once. Intrinsic text metrics remain unscrolled.


## AS2 properties

scroll and hscroll are writable; maxscroll, maxhscroll and bottomScroll are
read-only. Finite numeric conversions are truncated and clamped to geometry.
NaN/infinity or failed conversion rejects without this setter changing offsets;
side effects of an explicit user conversion remain visible. Geometry is measured
after conversion so reentry cannot publish stale layout. A successful query/set
normalizes both offsets. Text/style/dimension edits keep lazy layout admission;
clamping occurs at the next scroll observation, export or capture. No onScroller
notification is synthesized. Legacy callback selectors retain their identities;
new selectors are appended and only rootless retired native stubs are upgraded.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_textfield_scroll_info` | function | [include/flashvm/standard_text.h:68](../include/flashvm/standard_text.h#L68) |
| `FVM_TextFieldScrollInfo` | struct | [include/flashvm/standard_text.h:44](../include/flashvm/standard_text.h#L44) |
<!-- source-links:end -->
