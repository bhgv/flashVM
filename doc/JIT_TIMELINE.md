# Native timeline controls — v0.365

[Русское описание](JIT_TIMELINE.ru.md)

NextFrame, PreviousFrame, Play, Stop, GotoFrame, GotoLabel and GotoFrame2 lower to
native helpers using the shared intrinsic frame service, bypassing replaceable
script methods. Fixed commands use the active display target. A missing selection
inside an existing timeline is a no-op; execution without a native base timeline
returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). A missing label is ignored, matching interpretation.

GotoFrame snapshots its zero-based UI16 request as a one-based numeric constant.
GotoLabel validates the complete terminated record and copies its text into code
storage. At runtime the literal request is retained in temporary GC roots, with
label bytes adopted into the context. Neither literal operation uses extra script
operand slots. GotoFrame2 validates flags and optional scene offset at compile
time. At runtime it keeps the source operand rooted through shared request
conversion, resolves the selected target afterwards, consumes the input and
applies the play policy. Missing selected targets use the interpreter's root
recovery for this dynamic operation. Numeric requests receive the optional bias;
labels do not. The shared service retains all existing clamping/coercion behavior.

| Test | Coverage |
| --- | --- |
| `jit_timeline` | Isolated host/SWF4–8 contexts and three execution modes; frame boundaries, UI16 zero/max, play/stop, direct frames, labels/missing labels, decimal requests, numeric bias and ignored label bias, maximum bias, negative frames, selected child, missing selection and dynamic root recovery, absent inputs/base timelines, unchanged lower stack values, bypass of overridden script methods |
| `jit_timeline_callbacks` | Exactly one collecting/throwing toString, no valueOf, detached request receiver, numeric/label bias, unchanged frame on conversion exception, native coercion callback changing the selected target before the seek |
| `jit_timeline_allocation` | All seven Actions in host/loaded contexts; compile failures preserve existing code handles; invocation failures restore roots/depths/active code; successful state survives code release and GC |
| `jit_invalid` | Malformed direct frame widths, empty/unterminated/embedded-NUL labels, invalid dynamic flags and inconsistent scene-offset records preserve the existing handle |

The compiler rejection fixture now uses unsupported ToggleQuality instead of
Play. WaitForFrame still exercises automatic interpreter fallback. These changes
extend native execution of existing FlashVM scene semantics; they do not complete
SWF display-list integration, frame-event/reference-player compatibility, the
remaining Actions, or real foreign-platform acceptance. All roadmap stages remain
open.

Linux Release: all 744 AGG, 744 Cairo and 725 minimal checks pass after restoring
generated ELF execute permissions before graphical-profile runs. Four timeline/
allocation/callback/invalid checks pass restricted heap/global ASan+UBSan; stack
ASan and LeakSanitizer are disabled. No foreign OS/VM execution is claimed.

As of v0.367 WaitForFrame is native; automatic fallback uses an unused anonymous
DefineFunction. See JIT_WAIT_FRAMES.md.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
