# Capture a player frame into C89 host memory

[Русский](README.ru.md) · [Catalog](../../README.md#example-player-frame_capture) ·
[Frame contract](../../../doc/FRAME_CAPTURE.md).

The standalone [main.c](main.c) demonstrates a logical RGBA raster callback for an
IDE, custom encoder or native C/C++ host. It copies a frame into native memory,
stops successfully and writes PPM only after VM/window/transfers and SDK cleanup.
The library does not bind captures to files or console argv; arguments and output
policy belong to this small executable adapter.

[make_movie.py](make_movie.py) encodes a real two-frame [SWF8](start.swf) and
identical immutable [linked bytes](movie.inc) using Python stdlib only. Odd ticks
draw a blue square on the left, even ticks an orange square on the right, erasing
old geometry. [start.as](start.as) is an explanatory AS2 sketch, not AS/FIR compiler
acceptance. Every nonempty C/AS/linked-byte line is commented.

## Native build and run

Requirements: C89/C++98 toolchain, CMake, graphics-enabled FlashVM profile and SDL2
2.0.9+ SDK for the native CPU/OS. The C executable needs no JIT, image codecs,
Python or Pillow. Local/memory use needs no networking. From repository root:

```sh
cmake -S flashVM -B build-capture -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DSDL2_DIR=/path/to/native-sdl/lib/cmake/SDL2
cmake --build build-capture --target flashvm_example_frame_capture \
  flashvm_frontend_frame_copy flashvm_frontend_frame_capture \
  flashvm_frontend_offscreen_window flashvm_frontend_offscreen_capture --parallel
./build-capture/frontend/flashvm_example_frame_capture flashVM/examples/player/frame_capture/start.swf first.ppm
./build-capture/frontend/flashvm_example_frame_capture --tick 2 flashVM/examples/player/frame_capture/start.swf second.ppm
./build-capture/frontend/flashvm_example_frame_capture --embedded --tick 3 "$PWD/not-read.swf" embedded.ppm
ctest --test-dir build-capture -R 'frame_capture|frontend_frame_copy' --output-on-failure
python3 flashVM/examples/player/frame_capture/make_movie.py
```

Expect status 0, captured_tick=1/2/3 and width=32 height=32. Outputs are binary
P6 RGB PPM: first/third show the blue left square, second the orange right square,
on white. frames equals the selected host tick, not an AVM1 instruction or child
clip frame ordinal. --tick accepts 1..1000. Embedded mode never reads not-read.swf;
its absolute path supplies explicit origin/base for relative script loads. No
temporary startup SWF is created.

The example opens the ordinary SDL window and closes it soon after copying. For
a display-free check set `SDL_VIDEODRIVER=dummy`; this is not hardware acceptance.
Captured pixels represent the logical stage, excluding window chrome/DPI/letterbox.
PPM drops alpha without extra compositing; this movie has an opaque background.
The public copy helper preserves all four RGBA channels.

## Networking and other platforms

Enable HTTP with `-DFVM_PLAYER_SOCKET_HTTP=ON`. HTTPS also requires
`-DFVM_PLAYER_SOCKET_TLS=ON` and target TLS libraries/trust. The alternate provider
uses `-DFVM_PLAYER_HTTP=ON` with worker-safe libcurl. Without a provider URL startup
fails explicitly before startup; local/embedded modes remain available. Eight
resources share the load budget and each network job has a cooperative deadline,
not a hard DNS/provider kill.

```sh
python3 -m http.server 8000 --bind 127.0.0.1 --directory flashVM/examples/player/frame_capture
# Another terminal / Другой терминал:
./build-capture/frontend/flashvm_example_frame_capture --tick 2 http://127.0.0.1:8000/start.swf network.ppm
./build-capture/frontend/flashvm_example_frame_capture --embedded http://127.0.0.1:8000/not-read.swf linked.ppm
```

The first run performs one startup GET; the embedded run skips it. This movie has
no child resources; independent live regressions check root navigation and active
HTTP cancellation on observer failure. SDK/native/cross and Android surface setup:
[DESKTOP_FRONTEND.md](../../../doc/DESKTOP_FRONTEND.md),
[SOCKET_TRANSPORT.md](../../../doc/SOCKET_TRANSPORT.md).
Use a separate build directory/toolchain and target SDL SDK for another CPU/OS:

```sh
cmake -S flashVM -B build-capture-target \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/target-toolchain.cmake \
  -DSDL2_DIR=/path/to/target-sdl/lib/cmake/SDL2 \
  -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=OFF \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-capture-target --target flashvm_example_frame_capture --parallel
```

For a shared host add `-DFVM_PLAYER_HOST_SHARED=ON` and set runtime search paths
under the [SDK guide](../../../doc/PLAYER_HOST_OWNERSHIP.md). SDL/TLS/curl dependencies
must match the target. Android excludes this desktop target and requires the
Activity/surface adapter in the general guide. ARM64/RISC-V64/Windows x64 cross-
linking does not imply execution there. A detached VM component needs its own
-S/profile and explicit SDKs, without sibling compilers/linker.

## API, lifetime and errors

The frame observer receives no VM pointer. pixels/source/frame remain borrowed
only inside the callback; independent storage is allocated/copied before return.
A background consumer needs the host's own synchronization around that copy,
never a borrowed buffer. [GC:] Native pixels are not roots/pins and are explicitly
freed. Same-owner control reads the native captured flag and returns successful
STOP. After runner return the SDK is cleaned, and the encoder uses owned memory only.

| C API | Purpose, ownership and blocking |
| --- | --- |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Fill scalar defaults without startup. [PLAYER_CONFIGURATION.md](../../../doc/PLAYER_CONFIGURATION.md). |
| [fvm_desktop_playback_control_init](../../../frontend/common/playback.h#L68) | Initialize owner command policy without allocation; callback may successfully STOP after copying. [PLAYBACK_CONTROL.md](../../../doc/PLAYBACK_CONTROL.md). |
| [fvm_desktop_frame_observer_init](../../../frontend/common/frame.h#L57) | Initialize exact observer ABI size and null fields without startup. [FRAME_CAPTURE.md](../../../doc/FRAME_CAPTURE.md). |
| [fvm_desktop_frame_copy](../../../frontend/common/frame.h#L65) | Validate both complete row extents before writing independent visible RGBA rows; padding untouched, no allocation. [FRAME_CAPTURE.md](../../../doc/FRAME_CAPTURE.md). |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Initialize explicit native-byte provider policy without workers/I/O. [BACKGROUND_TRANSFER.md](../../../doc/BACKGROUND_TRANSFER.md). |
| [fvm_desktop_load_monitor_init](../../../frontend/common/load_monitor.h#L49) | Initialize async/deadline owner observation without creating jobs. [LOAD_MONITOR.md](../../../doc/LOAD_MONITOR.md). |
| [fvm_desktop_loader_init](../../../frontend/common/source.h#L51) | Initialize ordinary resource policy; explicitly increase local resource budget before running. [PLAYER_SOURCES.md](../../../doc/PLAYER_SOURCES.md). |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L65) | Selected native worker HTTP/TLS provider with cooperative stop; never receives pixels/VM. [SOCKET_TRANSPORT.md](../../../doc/SOCKET_TRANSPORT.md). |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L35) | Alternate worker-safe curl provider with SDK global-safety check. [BACKGROUND_TRANSFER.md](../../../doc/BACKGROUND_TRANSFER.md). |
| [fvm_desktop_run_source_observed](../../../frontend/common/frame.h#L76) | Synchronous local/memory/URL run with optional loader/control and required observer; borrows descriptors to return, status 0/1/2. [FRAME_CAPTURE.md](../../../doc/FRAME_CAPTURE.md). |
| [fvm_desktop_run_source_background_observed](../../../frontend/common/frame.h#L83) | Require provider/monitor and observer; callbacks remain owner-only, jobs joined before buffer/root disposal. [FRAME_CAPTURE.md](../../../doc/FRAME_CAPTURE.md). |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L54) | Explicit session teardown after runner-owned resources are gone; output copy remains native-owned. [PLAYER_HOST_OWNERSHIP.md](../../../doc/PLAYER_HOST_OWNERSHIP.md). |

Argument errors return 2; source/render/observer/encoding/I/O errors return 1;
a completed copy and full write return 0. Invalid source never opens output. User
window-close before the requested tick produces no image and returns 1. Observer
failure does not roll back committed VM/drawing; SDK joins pending jobs. Output
failure may leave partial PPM under this example policy; the library writes no file.
Output filenames follow the native C runtime. Files close and copied storage is
freed on every path.

Limits: SDL window remains part of playback; this is not a widget/windowless API.
No foreign SDL-event router, GPU screenshot, audio/video encoder or full debugger
is added. No reentrant/concurrent runner; VM/SDL stay with the UI owner. Captured
RGBA does not certify complete Flash/SWF8/reference compatibility. Native foreign
OS/device acceptance and all stages, including stage 9, remain open.

## Windowless / Без собственного окна (v0.721)

```sh
./build-capture/frontend/flashvm_example_frame_capture --offscreen --tick 2 flashVM/examples/player/frame_capture/start.swf offscreen.ppm
./build-capture/frontend/flashvm_example_frame_capture --offscreen --embedded --tick 3 "$PWD/not-read.swf" linked.ppm
ctest --test-dir build-capture -R 'offscreen|frame_capture|frontend_frame_copy' --output-on-failure
```

`--offscreen` selects timer-only software output and leaves the application's SDL
events/cursor/video alone. The selected tick still follows real-time scheduling.
The same copied PPM remains valid after SDK cleanup. SDL is still linked. Audio is
muted by this tutorial. Native video is the default without this flag. No input
injection or native OS/widget certification is implied.

`--offscreen` выбирает программный растр без окна и не читает SDL-события хоста,
не меняет курсор и не запускает video. Такт по-прежнему зависит от часов реального
времени. PPM записывается из своей копии после завершения SDK; SDL остаётся
линкуемой зависимостью. В примере звук отключён. Без флага сохраняется окно.
Инъекция ввода/приёмка native OS/widget здесь не заявляются.

| C API | Purpose / Назначение и владение |
| --- | --- |
| [fvm_desktop_run_source_offscreen](../../../frontend/common/frame.h#L95) | Synchronous FILE/MEMORY source/control/observer borrows through return; no video/events/cursor. Синхронный local/linked запуск; заимствования до возврата. [EN](../../../doc/OFFSCREEN_PLAYER.md) / [RU](../../../doc/OFFSCREEN_PLAYER.ru.md). |
| [fvm_desktop_run_source_background_offscreen](../../../frontend/common/frame.h#L100) | Explicit provider/monitor version; worker cancellation/join precedes return, pixels borrowed only inside observer. Явный provider/monitor, отмена/join до возврата. [EN](../../../doc/OFFSCREEN_PLAYER.md) / [RU](../../../doc/OFFSCREEN_PLAYER.ru.md). |

## Resource control / Ресурсное управление (v0.728)

```sh
./build-capture/frontend/flashvm_example_frame_capture --offscreen --resource-control --tick 2 flashVM/examples/player/frame_capture/start.swf resource.ppm
ctest --test-dir build-capture -R '^flashvm_resource_capture_example$' --output-on-failure
```

The optional flag creates a single-owner native queue, grants local session 1 CONTROL
on /timeline/ctl, posts one STOP after pixel capture and acknowledges its delivery.
Unregister/destroy occur after runner return, before SDK shutdown and output.
No network resource-control endpoint or GUI filesystem is created. Input SWF HTTP
loading is still the existing separate provider. The generic C request needs no
S-record encoding; live integration fixtures cover the bound binary route too.
The output pixels/header/exit statuses match ordinary capture. Duplicate selectors
fail before startup. [Contract](../../../doc/RESOURCE_DISPATCH.md).

| C API | Purpose and ownership |
| --- | --- |
| [`fvm_rdispatch_create`](../../../include/flashvm/rdispatch.h#L109) | Own a bounded native queue in single-owner mode; no VM/SDK startup. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [fvm_desktop_resource_control_init](../../../frontend/common/resource_control.h#L29) | Register the CONTROL endpoint and borrow Capture adapter storage until unregister. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_grant`](../../../include/flashvm/rdispatch.h#L128) | Grant only the trusted local session CONTROL, not arbitrary remote authority. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_post`](../../../include/flashvm/rdispatch.h#L139) | Copy one empty STOP request; acceptance is not execution. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [fvm_desktop_resource_control_notify](../../../frontend/common/resource_control.h#L39) | Deliver queued command at the player owner boundary; return native intent, retaining no VM pointers. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_take`](../../../include/flashvm/rdispatch.h#L156) | Copy/acknowledge the delivery result and release its identity/capacity. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_cancel`](../../../include/flashvm/rdispatch.h#L149) | On failure cancel a queued STOP before taking the retained result. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_unregister`](../../../include/flashvm/rdispatch.h#L123) | After runner return release callback registration before Capture disposal. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_destroy`](../../../include/flashvm/rdispatch.h#L163) | Free only the native dispatcher after all accepted requests are acknowledged. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
## Resource state v0.734

With `--resource-control`, owner notify copies state into `/timeline/state`;
a local observer independently takes and decodes events. The snapshot owns its
bytes after SDK teardown; no root/context is retained. Source truncation is
explicit. Quiet turns do not fill queues. Destroy state adapter before control
registration and dispatcher. This example creates no network client.

| API | Ownership / execution |
| --- | --- |
| [fvm_desktop_resource_state_create](../../../frontend/common/resource_state.h#L41) | Own native cached state, borrowing dispatcher without starting SDK. [doc/PLAYER_RESOURCE_STATE.md](../../../doc/PLAYER_RESOURCE_STATE.md). |
| [fvm_desktop_resource_state_observe](../../../frontend/common/resource_state.h#L50) | Owner copies the current safe point before returning the next command. [doc/PLAYER_RESOURCE_STATE.md](../../../doc/PLAYER_RESOURCE_STATE.md). |
| [fvm_desktop_resource_state_read](../../../frontend/common/resource_state.h#L55) | Decode owned event bytes into a standalone scalar/source snapshot. [doc/PLAYER_RESOURCE_STATE.md](../../../doc/PLAYER_RESOURCE_STATE.md). |
| [fvm_desktop_resource_state_destroy](../../../frontend/common/resource_state.h#L61) | Unregister watches and free state before ending dispatcher lifetime. [doc/PLAYER_RESOURCE_STATE.md](../../../doc/PLAYER_RESOURCE_STATE.md). |
| [`fvm_rdispatch_subscribe`](../../../include/flashvm/revents.h#L63) | Own one independent copied queue for the trusted local session. [doc/RESOURCE_EVENTS.md](../../../doc/RESOURCE_EVENTS.md). |
| [`fvm_rdispatch_event_take`](../../../include/flashvm/revents.h#L79) | Consume only this reader; quiet-turn PENDING is not failure. [doc/RESOURCE_EVENTS.md](../../../doc/RESOURCE_EVENTS.md). |
## Frame export v0.736

Use `--resource-frames` instead of `--resource-control` to also create native
`/display/frame` storage: three 4-MiB slots, twelve MiB total pixels. The limit is
explicit in [resource_limits.h](resource_limits.h), not a hidden SDK default.
Every presented frame is copied without padding; the local metadata subscription
does not pin pixels. BUSY increases dropped while preserving prior export.
Geometry/capacity/publication errors explicitly fail this example. Ordinary and
resource-control modes do not allocate this cache. Cleanup revokes the local
session, destroys the frame adapter, then state/control/dispatcher. PPM remains
an owned copy after SDK teardown. This is a publisher/metadata tutorial; the
guide and separate client fixtures cover CHUNK/leases.

```sh
./build-player/frontend/flashvm_example_frame_capture --offscreen --resource-frames --embedded flashVM/examples/player/frame_capture/start.swf frame.ppm
```

| API | Ownership / execution |
| --- | --- |
| [fvm_desktop_resource_frame_create](../../../frontend/common/resource_frame.h#L70) | Own three explicit-capacity pixel slots, borrowing dispatcher. [doc/PLAYER_RESOURCE_FRAMES.md](../../../doc/PLAYER_RESOURCE_FRAMES.md). |
| [fvm_desktop_resource_frame_capture](../../../frontend/common/resource_frame.h#L79) | Owner copies visible rows and publishes metadata without retaining borrowed pixels. [doc/PLAYER_RESOURCE_FRAMES.md](../../../doc/PLAYER_RESOURCE_FRAMES.md). |
| [fvm_desktop_resource_frame_read](../../../frontend/common/resource_frame.h#L89) | Decode copied event metadata into independent native storage. [doc/PLAYER_RESOURCE_FRAMES.md](../../../doc/PLAYER_RESOURCE_FRAMES.md). |
| [fvm_desktop_resource_frame_revoke](../../../frontend/common/resource_frame.h#L94) | Remove local rights/watches and release all session pins before teardown. [doc/PLAYER_RESOURCE_FRAMES.md](../../../doc/PLAYER_RESOURCE_FRAMES.md). |
| [fvm_desktop_resource_frame_destroy](../../../frontend/common/resource_frame.h#L100) | Unregister before freeing native pixel buffers; dispatcher remains host-owned. [doc/PLAYER_RESOURCE_FRAMES.md](../../../doc/PLAYER_RESOURCE_FRAMES.md). |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_rdispatch_cancel` | function | [include/flashvm/rdispatch.h:149](../../../include/flashvm/rdispatch.h#L149) |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:109](../../../include/flashvm/rdispatch.h#L109) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:163](../../../include/flashvm/rdispatch.h#L163) |
| `fvm_rdispatch_event_take` | function | [include/flashvm/revents.h:79](../../../include/flashvm/revents.h#L79) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:128](../../../include/flashvm/rdispatch.h#L128) |
| `fvm_rdispatch_post` | function | [include/flashvm/rdispatch.h:139](../../../include/flashvm/rdispatch.h#L139) |
| `fvm_rdispatch_subscribe` | function | [include/flashvm/revents.h:63](../../../include/flashvm/revents.h#L63) |
| `fvm_rdispatch_take` | function | [include/flashvm/rdispatch.h:156](../../../include/flashvm/rdispatch.h#L156) |
| `fvm_rdispatch_unregister` | function | [include/flashvm/rdispatch.h:123](../../../include/flashvm/rdispatch.h#L123) |
<!-- source-links:end -->
