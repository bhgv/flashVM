# Player primary contact and ordered input

v0.596 connects the [Mouse listener core](MOUSE.md) to the SDL2 player used by
desktop and Android hosts. A physical mouse is optional: the first accepted touch
contact produces standard onMouseDown/onMouseMove/onMouseUp notifications.
[Русский контракт](PLAYER_POINTER.ru.md).

## Event collection and touch policy

The main-thread window adapter records native events without entering the VM.
SDL mouse motion and primary-button transitions share one FIFO with supported
keyboard transitions, preserving their collection order. Middle/right buttons
still update HostInput, but do not invent primary-button Mouse notifications.

The first valid SDL_FINGERDOWN owns the primary contact until its matching
SDL_FINGERUP or focus/background reset. Additional fingers/devices are ignored
while it is active; lifting a secondary finger cannot end the primary contact.
An already-held physical primary button prevents a touch from taking ownership.
Physical mouse events are ignored while a touch owns the contact. No remaining
secondary finger is automatically promoted after primary release; a new down is
required. Finger events identified by the SDK as synthesized from a mouse, and mouse events
synthesized from touch, are ignored; synthesis hints are also disabled to prevent duplicates.

SDL's logical-size renderer event filter already adjusts normalized finger
coordinates for its viewport/letterboxing. The adapter multiplies those values
by the fixed logical stage extent once. Mouse events already contain SDL logical
pixels. During delivery the host adds the authored SWF stage origin; it does not
rewrite script MovieClip matrices. Invalid nonfinite/out-of-range contact
coordinates or missing logical dimensions latch [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before
changing host contact/position. This is one primary contact, not gesture or full
multitouch recognition. SDL's own viewport edge clamping remains in force.

## Shared FIFO and delivery

FVM_DESKTOP_PENDING_EVENTS bounds the copied key/pointer FIFO to 128 events.
There are no managed VM pointers or borrowed event buffers in it. Each event
retains the pointer position at that transition, plus its relevant key snapshot
or pointer type. Repeated motion is not silently coalesced. Overflow latches
[FVM_ERROR](../include/flashvm/flashvm.h#L43), and the player reports failure rather than claiming delivery.

`fvm_desktop_window_pump_input(view, context, origin_x, origin_y)` borrows the
window/context on their common owner thread and synchronously processes the
entry batch before a movie tick. Null owners, nonfinite origin or recursive
pumping return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before consumption. Key callbacks see that
key's saved pointer position and keyboard snapshot; pointer events use
[fvm_input_dispatch_mouse](../include/flashvm/mouse.h#L25), including native drag update before handlers.
No separate keyboard-first/pointer-first reordering occurs. HostInput/final
polling state is published afterwards using the existing player loop.

Callbacks can enqueue later work; it waits for a subsequent pump. Focus loss or
backgrounding cancels pending events and clears held host keys/buttons/contact,
without fabricating key-up/mouse-up callbacks. An epoch change also stops an
active entry batch, so a callback that resets focus and enqueues new input cannot
accidentally consume that new work as part of the old batch. Cancellation does
not clear an existing overflow error. Escape and Android Back still close.

The current event is removed before entering the VM. Any callback/VM error,
including preparation OOM, consumes that event and retains later events; committed
VM effects follow the core dispatch contracts. The regular player reports the
error and closes the movie. A latched collection error prevents pumping until
window teardown. The pump restores its reentry flag on every normal status path.
Callbacks must not destroy/reenter the host presentation lifecycle or throw
across C. `fvm_desktop_window_pump_keyboard` remains an internal compatibility
wrapper over the same shared FIFO with zero stage origin.

Mouse namespace installation is explicit in the player for SWF6–8. Older movies
remain playable without the new namespace. The bridge does not add wheel,
cursor visibility, Button/ClipActions, automatic focus/hit routing, IME,
multitouch gestures or security-origin policy. Core-only builds still need no SDL.

## Reproduce the bounded checks

From the FlashVM component with a real SDL2 SDK:

```sh
cmake -S . -B build -DFVM_BUILD_DESKTOP_FRONTEND=ON -DSDL2_DIR=/path/to/SDL2/cmake
cmake --build build --target flashvm_frontend_pointer flashvm_frontend_keyboard
ctest --test-dir build -R '^flashvm_frontend_(pointer|keyboard)$' --output-on-failure
```

`frontend/tests/pointer.c` uses the actual SDL library with its explicit dummy
video driver. It checks mixed mouse/key order, stage origin, two fingers,
synthetic-event suppression, short taps, callback append/error/reset behavior,
recursive pumping and FIFO overflow. SDL_PushEvent through a resized logical
renderer checks the actual letterbox coordinate filter, not a second manual
projection. The existing keyboard fixture checks polling/short taps/reset and
overflow through the shared queue. These are native library/runtime fixtures,
not physical screen/touch/audio or Android/Windows/macOS device acceptance.
No new APK or physical-device result is supplied by this checkpoint. All roadmap
stages remain open; see [status](ROADMAP_STATUS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_dispatch_mouse` | function | [include/flashvm/mouse.h:25](../include/flashvm/mouse.h#L25) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
<!-- source-links:end -->
