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
## Precise position (v0.653)

SDL 2.26+ carries window-pixel mouseX/mouseY on each wheel event. The player
converts these through SDL_RenderWindowToLogical, preserving resizing, letterbox
and logical stage scale, then snapshots the result. Older SDKs keep the last
logical pointer position; no global pointer query races queued events. The test
checks a 64×32 window containing a centered 32×32 logical stage: window (26,20)
becomes logical (10,20), then the authored origin is applied at delivery.
Fractional wheel units are checked independently (0.5 becomes 1.5 lines).
## Cursor visibility (v0.654)

The player now queries the existing VM cursor request after input/timers and after
frame execution, before rendering or waiting. Mouse.hide/show therefore reach
SDL_ShowCursor without requiring a later timeline tick. Repeated unchanged
requests do not repeat the native operation. Background calls defer application
until foreground. This SDL host owns its SDL session and cursor policy; the SDL
visibility switch is global to that session, not an independently scoped cursor
for every possible foreign SDL window.

Opening saves native visibility; closing restores it before destroying the window
and shutting down SDL, so a hidden movie does not contaminate the next playlist
entry. Teardown restoration is best effort because close returns void. Active
query/application failures propagate; a failed SDL application is also latched.
No physical cursor is guaranteed on touch-only systems. The C fixture checks
actual SDL state, redundant-call suppression, deferral and the cleanup operation.
## Actual movie checks (v0.655)

flashvm_player_wheel_interpreter/jit execute self-checking SWF6, 7 and 8 movies
through the real player, real SDL dummy event queue and VM. Script hides the
cursor in frame one; a test-only adapter verifies applied native state, injects
motion then two ordered wheel events, and verifies script-originated show before
the next wait. The second frame checks wheel accumulation and exactly one move
callback. Wrong-value controls must fail. Ordinary player builds inject nothing.
This is native Linux dummy-video/SDL state evidence, not a physical cursor test.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_dispatch_mouse_wheel` | function | [include/flashvm/mouse.h:36](../include/flashvm/mouse.h#L36) |
<!-- source-links:end -->
