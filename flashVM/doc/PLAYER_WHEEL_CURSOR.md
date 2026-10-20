# SDL player wheel and cursor bridge

[Русский](PLAYER_WHEEL_CURSOR.ru.md). Wheel entries share the existing bounded
main-thread key/pointer FIFO. Vertical units become three host scroll lines;
flipped direction is normalized once. SDL 2.0.18+ uses preciseY, older SDKs use y.
Zero vertical movement is ignored, including horizontal-only events. Synthetic
touch-mouse wheels, active primary-touch wheels, background and foreign-window
wheels are ignored. Invalid non-finite deltas and FIFO overflow latch an error.

At delivery, saved logical pointer coordinates receive the authored stage origin,
then [fvm_input_dispatch_mouse_wheel](../include/flashvm/mouse.h#L36) publishes the pointer/drag state and calls
onMouseWheel(delta). There is no fabricated MouseMove/Up ClipAction, hit target,
focus change or automatic scrolling. The existing VM wheel contract does not make
wheel callbacks updateAfterEvent-eligible. A handler failure consumes that entry;
later events remain queued, exactly as for other input. No VM pointers live in
native queued records. Script Mouse listeners are the supported SWF6–8 subset.

The wheel C fixture uses a real SDL dummy window and actual VM listeners, checks
origin restoration, ordering, cancellation/error consumption, synthesis/touch/
background/foreign filtering and overflow. No physical wheel/device is certified.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_dispatch_mouse_wheel` | function | [include/flashvm/mouse.h:36](../include/flashvm/mouse.h#L36) |
<!-- source-links:end -->
