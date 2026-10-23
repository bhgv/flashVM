# Uniform TextField scrolling

Dimension/layout-policy observer transactions extend this bridge: [contract](TEXT_GEOMETRY_EVENTS.md). Native query/export/capture remain no-script.


This series adds bounded viewport scrolling to host-created TextField objects.
The [Adobe AS2 reference](https://open-flash.github.io/mirrors/as2-language-reference/TextField.html)
defines one-based vertical visual-line indices and zero-based horizontal pixels.
This implementation uses the existing uniform-style native layout, not a Flash
Player binary oracle. HTML/mixed-font fields, text/style-triggered scroll events, input,
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
normalizes both offsets. Text/style edits keep lazy layout admission; dimension
and layout-policy admission follows TEXT_GEOMETRY_EVENTS.md;
effective bounds are measured at the next scroll observation, export or capture;
only normalizing observations/exports publish source offsets. Explicit offset changes now notify onScroller and listeners under the
[observer transaction contract](TEXT_SCROLL_EVENTS.md). Text/style-only changes and native observations do not synthesize notifications. Legacy callback selectors retain their identities;
new selectors are appended and only rootless retired native stubs are upgraded.


## Binary and channel ownership

Nondefault scroll positions add the [FVM_TEXT_STATE_HAS_SCROLL](../src/standard/text_constants.h#L49) policy bit and two
numeric values after optional position values. Default offsets omit these bytes;
older payloads restore scroll=1/hscroll=0. Snapshot charges include the optional
26 wire bytes. Capture measures/clamps active positions before copying; default
fields keep legacy lazy capture. Restore rejects nonfinite/fractional/negative,
portable-limit or actual-geometry-exceeding positions before field publication.
Fonts and text are independently owned on receivers. No font/cache/VM pointer is
serialized. Newer payloads with this bit require an updated receiver. Existing
rootless scroll stubs are upgraded; captured/custom accessors retain their semantics.


## Lazy edits and observational capture

Text/format/dimension writes preserve existing lazy-layout admission. Stored scroll
requests can therefore temporarily exceed a newly smaller viewport range. A scroll
property/native query or successful path export normalizes both stored offsets;
failed operations preserve them. Graph capture copies effective clamped offsets
without publishing them back into the source, so a later serialization failure
cannot change source viewport requests. If geometry shrinks and grows again before
any normalizing observation, the old request can become effective again. This is
an explicit bounded bridge policy; full original-player edit/event parity remains
open. Wrapped whitespace follows existing layout preservation and may occupy its
own visual line. Non-multiline hard-break layout remains UNSUPPORTED.


## Validation checkpoint

Uniform TextField scrolling v0.1090–v0.1103: the host-created text bridge implements writable one-based scroll and zero-based hscroll plus read-only maxscroll/maxhscroll/bottomScroll. An additive C89 observation copies effective ranges, line spacing, scroll displacement and the inner host clip rectangle. Exported glyph and underline contours include the same translation; intrinsic metrics and parent _x/_y remain independent. Optional binary/channel state preserves active positions and legacy defaults, upgrades only retired rootless native stubs, validates actual restore bounds and independently owns receiver fonts/text. Graph capture copies effective positions without mutating pending source requests. Validation passes 153 selected native text/font/graph/tutorial checks, nine focused full-font ASan/UBSan checks plus an explicit no-fault allocation-success run. Ordinary builds sweep query/path allocation failures and verify conversion exceptions; LeakSanitizer is disabled. Tests cover wrapping and preserved whitespace, CRLF/empty/count-NUL lines, leading/underline, autoSize, zero viewports, clamping, exact contour shifts, stale query outputs, legacy accessors, sender destruction and GC. SWF6–8 AVM1 GetMember/SetMember agrees across interpreter/manual/native JIT with actual native entries asserted; reentrant numeric conversion measures updated geometry and retains the GC barrier. Four OS workers own separate VMs/fonts and check 128 scroll/export/GC epochs; cached native observations run 128 times without C++ allocation. The C89 --scroll tutorial checks analytic ranges and actual exported bounds. SDK-free C89 ABI/admission passes and both font tutorials explicitly skip. A renamed detached text component configures with its bundled static font dependencies; the copied scroll checker passes against the native executable, without a separate detached build. Clipping is explicitly host-owned; full original-player rounding/edit semantics, onScroller/listeners, mixed styled/HTML fields, SWF font/text display-list integration, input, physical platforms/devices and all nine roadmap stages remain open. [Contract](TEXT_SCROLL.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_TEXT_STATE_HAS_SCROLL` | macro / define | [src/standard/text_constants.h:49](../src/standard/text_constants.h#L49) |
| `fvm_textfield_scroll_info` | function | [include/flashvm/standard_text.h:68](../include/flashvm/standard_text.h#L68) |
| `FVM_TextFieldScrollInfo` | struct | [include/flashvm/standard_text.h:44](../include/flashvm/standard_text.h#L44) |
<!-- source-links:end -->
