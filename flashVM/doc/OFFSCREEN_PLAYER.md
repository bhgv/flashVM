# Windowless logical-stage playback

[Русский](OFFSCREEN_PLAYER.ru.md) · [Frame API](FRAME_CAPTURE.md) · [Roadmap](ROADMAP_STATUS.md).

The timer-only presentation adapter owns an SDL timer reference and validates the
same raster geometry as native presentation. It creates no window/renderer/texture,
does not initialize video/events, never polls or removes the embedding application's
SDL events and never queries or changes its cursor. Close releases only its own
reference. Logical stage dimensions remain bounded by the software rasterizer.

This is software RGBA output, not GPU capture or an operating-system widget.
SDL remains a build/link dependency for timing, threading and optional audio;
no device or foreign native-OS acceptance follows from this adapter alone.

## Public runners (v0.720)

The additive C89 entrypoints are declared in [frame.h](../frontend/common/frame.h).
Existing observer structs and all existing native-window runners keep their layout
and behavior. Include `player.h` or `frame.h`; no SDL types cross these APIs.

| Function | Arguments and behavior |
| --- | --- |
| [fvm_desktop_run_source_offscreen](../frontend/common/frame.h#L95) | Required options, source and frame observer; optional ordinary loader and playback control; optional error output. A null loader retains the ordinary startup-only resource budget. Use an explicit loader for child requests. |
| [fvm_desktop_run_source_background_offscreen](../frontend/common/frame.h#L100) | Required options, source, background provider, load monitor and frame observer; optional control and error output. The monitor selects asynchronous owner turns or blocking resource waits. The explicit provider supplies startup/child bytes. |

Both run synchronously on the VM owner thread. They preserve FILE/MEMORY/URL
origin resolution, whole-root navigation, timers, GC safe points, deadline/cancel
handling and pause/resume/step/STOP semantics from the existing runners. Network
base directories remain URL-based, and linked bytes retain their explicit origin.
A source buffer must remain valid until return; frame/pixels/source pointers are
borrowed only inside the observer. Use the checked copy helper during that borrow.
Copying pixels does not copy provenance text. Caller callback policies/userdata
remain valid through return and may not reenter VM/player or destroy SDK resources.

No SDL video/events subsystem is initialized. Native events, focus changes and
quit/Escape are not consumed; there is no automatic window-close termination or
OS input delivery. HostInput contains neutral held-key/pointer state. Native
cursor requests do not modify a host cursor. This release does not supply input
injection or a GUI-thread event adapter. An IDE can copy completed frames and
control playback through its own synchronized mailbox.

Scheduling remains real-time paced at the selected movie/host rate; this is not
an unbounded-speed offline encoder or a single-step-return API. Use options.frames,
once or a control callback returning STOP to ensure bounded completion. Initially
paused runs without a resuming controller intentionally remain paused. CPU-side
rendering and source callbacks can still take time; no hard timeout is promised.

Frame format/reason/tick semantics are unchanged: REFRESH covers an initial/dirty
paused redraw, TICK follows a completed host advance. Presentation counters count
completed logical rasters in these runners, with no native swap. Unneeded frames
can be ignored by the observer. Callback failure stops playback after committed
VM/raster effects and cancels/joins active resource workers before release.

Audio is independent: options.mute acknowledges event sound without opening a
device. Sound-enabled runs still require a supported audio backend/device and
preserve their existing errors. Windowless output does not add missing codecs,
streaming/video support or native Android lifecycle/permissions handling.

Return 0 means normal completion or STOP; 1 means source/VM/raster/provider/observer
failure; 2 means malformed arguments rejected before source/provider/VM startup.
Optional error points to borrowed static diagnostic text. Owned worker/context/
raster/timer resources are released on return; copied native pixel storage remains
owned by the caller. Existing SDK shutdown ownership still applies: shut down only
when host SDK references, transfers and mailboxes have been released. The runner
does not tear down unrelated SDL subsystems belonging to the embedding application.

## Building and verification

Use the [desktop SDK instructions](DESKTOP_FRONTEND.md), an SDL2 2.0.9+ SDK and a
graphics-enabled profile. SDL remains linked even with no window. From workspace root:

```sh
cmake -S flashVM -B build-offscreen -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DSDL2_DIR=/path/to/target-sdl/lib/cmake/SDL2
cmake --build build-offscreen --target flashvm_frontend_offscreen_window \
  flashvm_frontend_offscreen_capture flashvm_frontend_host_abi --parallel
ctest --test-dir build-offscreen -R 'offscreen|frontend_host_abi' --output-on-failure
```

For shared host linkage add `-DFVM_PLAYER_HOST_SHARED=ON` and configure the target
runtime library search path as in [host ownership](PLAYER_HOST_OWNERSHIP.md).
Cross builds additionally supply `-DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake`
and an SDL SDK built for that OS/CPU; SDL headers/libraries and all optional audio/
HTTP dependencies must match the target. Tests execute on the target, not during
cross compilation. Android still needs the explicit application integration and
owner/lifecycle adapter described in the desktop guide.

The timer-only fixture uses an invalid video driver, verifies exact external SDL
subsystem ownership and leaves a queued host event untouched. Real SWF interpreter/
JIT fixtures exercise white initial raster, step/paused redraw, root navigation,
file/linked bytes and HTTP, including cancellation on observer failure. Dummy-video
native-window regressions remain separate from windowless assertions. Neither is
native Windows/macOS/Android display/device certification.
