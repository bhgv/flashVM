# Editor event waits and unchanged scenes

[Русский](EDITOR-IDLE.ru.md) · [Runtime](README.md)

## Purpose

The graphical editor has two native owners: FlashVM/SDL on main and Dis on its
own OS thread. An empty input queue should leave both owners waiting. A stable
scene does not need a new raster job or a new SDL display presentation. This
applies to the single editor and desktop application; it is not an IDE policy.

## Wake sources

[The launcher](editor_thread.c) wakes UI main when Dis publishes a copied frame,
a native RPC request, shutdown or completion. Publication uses the session mutex
and a condition generation to avoid losing a wake between checking and waiting.
The same publication pushes a private SDL event while the session lifetime is
pinned. The event contains no VM heap pointer.

[The UI wait helper](editor/view/idle_wait.inc) uses SDL_WaitEventTimeout on
modern window backends. It retains an ordinary event in the existing postponed
FIFO slot so waiting does not consume input. Input blocked by presentation or
Dis acknowledgement does not force a zero-delay retry.

The next wait is shortened for a real FlashVM timer/Async callback, outstanding
raster work, input batching or active interpolation. The
[fvm_timers_next_delay API](../../flashVM/include/flashvm/timers.h) includes
[Async.post](../../flashVM/include/flashvm/async.h) records. Merely waking or
querying a deadline does not invalidate the scene. The existing dirty/pending
frame pipeline decides when drawing is necessary.

A 1000 ms ceiling recovers from a failed native wake; it is not a frame timer.
Raster work and actual animation use an 8 ms check. Dummy/offscreen and SDL
versions before 2.0.18 use the session condition with a 16 ms ceiling, because
SDL's own fallback event wait can poll internally every millisecond. This
fallback still checks platform input; Dis notifications wake it immediately.
It trades bounded legacy-input latency for fewer idle wakeups. A backend may
also poll internally for devices; that depends on the SDL build and host.

## Verification

Build the normal SDL-enabled runtime, then run from the repository root:

```sh
ctest --test-dir build/runtime-clean-on -R 'disvm_(sdl_idle_window|editor_idle_cpu|desktop_workflow_)' --output-on-failure
```

Replace the build path with your configured directory. The SDL test requires
Linux, Python, a compatible C compiler/linker and an SDL-enabled editor build.
It relinks test wrappers with the actual runtime objects; it does not substitute
the renderer, guest interpreter or queue implementation.

[The regression](test_sdl_idle.py) uses an independent event producer, measures
two quiet seconds with visible and minimized dummy windows, and requires zero
raster submissions and zero SDL_RenderPresent calls in each interval. It bounds
CPU and UI iteration count, checks a real 75 ms callback, then restores/resizes,
types text, saves and exits. It validates the saved document and rendered image.
The iteration bound rejects the previous 1 ms UI loop independently of CPU speed.

For the callback check, wrappers install the scalar standard builtins in a
single-view context that does not otherwise need them.

For additional coverage of the native-wait branch with the same dummy SDK:

```sh
DVM_TEST_NATIVE_WAIT=1 python3 disVM/runtime/test_sdl_idle.py build/runtime-clean-on/disvm-editor build/runtime-clean-on/desktop-ide.dis build/runtime-clean-on/fonts/NotoSansMono-Regular.ttf
```

This flag is interpreted only by test wrappers. SDL still uses its dummy queue;
its internal polling CPU is not a measurement of native X11/Wayland performance.
Dummy measurements do not establish physical display/input or GPU idle cost.
The no-SDL replay build is also compiled and tested separately.
