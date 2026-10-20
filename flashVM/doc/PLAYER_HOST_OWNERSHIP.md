# SDL ownership in an embedding host

Since v0.670, [fvm_desktop_window_open](../frontend/common/window.h#L53) acquires TIMER and VIDEO subsystem
references independently. [fvm_desktop_window_close](../frontend/common/window.h#L88) destroys only its own native
resources and releases references actually acquired by that view. Null close is
harmless. Neither normal cleanup nor failed startup calls global SDL_Quit.
A host's windows and preexisting subsystem references remain alive. Multiple
views can be created and destroyed without terminating each other.

The embedding application owns final process-wide SDL shutdown. Calls belong on
the native UI thread. This ownership contract does not provide a multi-window
application event router: the synchronous player still consumes the SDL queue.
Cursor and other process-global policies need separate coordination.

SDL itself can retain an EVENTS dependency when VIDEO initialization fails
(SDL 2.30.12). FlashVM releases its successfully acquired references; it does not
guess or decrement internal SDL dependency counts. The host ultimately calls
SDL_Quit when ending its own SDL session.

The C89 sdl_ownership regression checks failed VIDEO startup with live host timer
and event references, then starts a separate dummy-video session. It opens two
player views beside a host window, closes them in repeated cycles, verifies host
and remaining player window identities, and checks complete subsystem shutdown
once the host releases its own references. No physical display is certified.

```sh
cmake --build build-player --target flashvm_frontend_sdl_ownership
ctest --test-dir build-player -R frontend_sdl_ownership --output-on-failure
```

[Interface](../frontend/common/window.h) · [Implementation](../frontend/common/sdl_window.c) ·
[Test](../frontend/tests/sdl_ownership.c) · [Build guide](DESKTOP_FRONTEND.md) ·
[Roadmap status](ROADMAP_STATUS.md)

## Host input policy since v0.671

Opening a player window no longer writes SDL_TOUCH_MOUSE_EVENTS or
SDL_MOUSE_TOUCH_EVENTS. Existing values and their priority remain owned by the
embedding application, including on startup failure. Synthetic mouse events
(SDL_TOUCH_MOUSEID) and, with supporting SDKs, synthetic finger events
(SDL_MOUSE_TOUCHID) are rejected locally by the input adapter. Real primary
finger tracking still suppresses duplicate mouse transitions. The ownership
fixture checks normal-priority hint values and their protection against later
lower-priority writes; the pointer fixture checks duplicate input suppression.
The player does not save/restore hints, which could overwrite a host update.
