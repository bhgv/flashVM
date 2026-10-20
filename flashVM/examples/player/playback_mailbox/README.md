# A separate C89 player controller thread

[Русский](README.ru.md) · [Catalog](../../README.md#example-player-playback_mailbox) ·
[Mailbox contract](../../../doc/PLAYBACK_MAILBOX.md).

The executable [main.c](main.c) is an IDE/native C/C++ host template. The main UI
thread runs playback; a separate OS thread posts native commands and reads copied
scalars/text only. Default playback performs five confirmed steps and then STOP;
`--steps N` selects 1..1000 steps. The controller waits for each completed tick
before posting the next STEP. Rapid duplicate STEP requests coalesce, rather than
count commands. The library never parses argv; only the thin main adapter does.

[make_movie.py](make_movie.py) independently encodes real two-frame
[start.swf](start.swf) and identical immutable [movie.inc](movie.inc) using Python
stdlib. Frames alternate a blue square on the left and orange on the right.
[start.as](start.as) is an explanatory AS2 sketch, not AS/FIR compiler acceptance.
Every nonempty C/AS/linked-byte line is commented.

## Requirements, build and run

C89/C++98 toolchain, CMake, graphics-enabled FlashVM profile, SDL2 2.0.9+ SDK for the
target CPU/OS. Local/embedded modes need no JIT, image codecs or networking. Here
SDL is a replaceable OS-thread/atomic/clock adapter; the public mailbox ABI exposes
no SDL types. Hosts may use pthread/Win32/other threads under the same ownership
contract.

```sh
cmake -S flashVM -B build-mailbox -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DSDL2_DIR=/path/to/native-sdl/lib/cmake/SDL2
cmake --build build-mailbox --target flashvm_example_playback_mailbox --parallel
./build-mailbox/frontend/flashvm_example_playback_mailbox flashVM/examples/player/playback_mailbox/start.swf
./build-mailbox/frontend/flashvm_example_playback_mailbox --steps 7 flashVM/examples/player/playback_mailbox/start.swf
./build-mailbox/frontend/flashvm_example_playback_mailbox --embedded --steps 3 "$PWD/not-read.swf"
ctest --test-dir build-mailbox -R playback_mailbox --output-on-failure
python3 flashVM/examples/player/playback_mailbox/make_movie.py
```

Without early user window-close, expect status 0 and `completed=5`, `completed=7`,
`completed=3`. frames match and actual interpreter counters are nonzero. Embedded
mode never reads not-read.swf: its absolute path supplies explicit origin/base.
No temporary SWF is written. copied_source prints after release from independent
text storage. Early window-close reports fewer completed advances. For a display-
free protocol run set `SDL_VIDEODRIVER=dummy`; this is not OS/display hardware acceptance.

Enable HTTP with `-DFVM_PLAYER_SOCKET_HTTP=ON` at configure time. HTTPS also needs
`-DFVM_PLAYER_SOCKET_TLS=ON` and target TLS libraries/trust. The alternate provider
uses `-DFVM_PLAYER_HTTP=ON` with worker-safe libcurl. Without a provider URL startup
fails explicitly; local/embedded modes remain available. The monitor shares the
playback mailbox, so STOP is visible before READY and during blocking fetch waits.

```sh
python3 -m http.server 8000 --bind 127.0.0.1 --directory flashVM/examples/player/playback_mailbox
# Another terminal / Другой терминал:
./build-mailbox/frontend/flashvm_example_playback_mailbox --steps 4 http://127.0.0.1:8000/start.swf
./build-mailbox/frontend/flashvm_example_playback_mailbox --embedded --steps 4 http://127.0.0.1:8000/not-read.swf
```

The first form performs one startup GET; the embedded form skips it. This movie
has no child requests; independent live regressions check startup/child cancellation
after body receipt and paused loadVariables delivery. If source failure occurs
before READY, owner signals done, joins controller, releases mailbox and SDK;
no worker remains waiting for a snapshot that cannot arrive.

## Another platform or processor

Use a separate build directory, CMake toolchain and SDL SDK for the target CPU/OS.
TLS/curl and transitive dependencies must also match the target. Full SDK/platform
setup: [DESKTOP_FRONTEND.md](../../../doc/DESKTOP_FRONTEND.md), transport policy:
[SOCKET_TRANSPORT.md](../../../doc/SOCKET_TRANSPORT.md).

```sh
cmake -S flashVM -B build-mailbox-target \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/target-toolchain.cmake \
  -DSDL2_DIR=/path/to/target-sdl/lib/cmake/SDL2 \
  -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=OFF \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-mailbox-target --target flashvm_example_playback_mailbox --parallel
```

Android excludes this desktop target and requires the Activity/surface adapter in
the general guide. ARM64/RISC-V64/Windows x64 cross-linking does not prove execution.
For a detached VM component, point -S and profile at its own root; no sibling
compiler/linker is required. The target inherits native SDL dependencies from
flashvm_player_host; SDK configuration selects static/shared linkage.

For a shared host add `-DFVM_PLAYER_HOST_SHARED=ON` when configuring; arrange
runtime DLL/shared-library search paths under the [SDK ownership guide](../../../doc/PLAYER_HOST_OWNERSHIP.md).

## API and ownership

Each row describes its individual function and links to the full contract.

| C API | Purpose, ownership and blocking |
| --- | --- |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Fill caller scalar defaults without allocation or startup. [PLAYER_CONFIGURATION.md](../../../doc/PLAYER_CONFIGURATION.md). |
| [fvm_desktop_playback_control_init](../../../frontend/common/playback.h#L68) | Fill the owner command descriptor and exact ABI size without startup. [PLAYBACK_CONTROL.md](../../../doc/PLAYBACK_CONTROL.md). |
| [fvm_desktop_playback_snapshot_init](../../../frontend/common/playback_mailbox.h#L32) | Fill exact copy size/zero fields; no resource acquisition. [PLAYBACK_MAILBOX.md](../../../doc/PLAYBACK_MAILBOX.md). |
| [fvm_desktop_playback_mailbox_create](../../../frontend/common/playback_mailbox.h#L38) | Allocate one native mutex/bounded record; out unchanged on failure, no VM/thread. [PLAYBACK_MAILBOX.md](../../../doc/PLAYBACK_MAILBOX.md). |
| [fvm_desktop_playback_mailbox_post](../../../frontend/common/playback_mailbox.h#L47) | Thread-safe latest intent; accepted STEP is not a completed tick, STOP terminal. [PLAYBACK_MAILBOX.md](../../../doc/PLAYBACK_MAILBOX.md). |
| [fvm_desktop_playback_mailbox_poll](../../../frontend/common/playback_mailbox.h#L52) | Brief mutex-protected value copy; NOT_FOUND before READY, no VM access. [PLAYBACK_MAILBOX.md](../../../doc/PLAYBACK_MAILBOX.md). |
| [fvm_desktop_playback_mailbox_release](../../../frontend/common/playback_mailbox.h#L42) | Dispose native record after runner/controller have finished; never joins them itself. [PLAYBACK_MAILBOX.md](../../../doc/PLAYBACK_MAILBOX.md). |
| [fvm_desktop_playback_mailbox_notify](../../../frontend/common/playback_mailbox.h#L62) | Owner callback copies observations/consumes pending intent; retains no VM pointer. [PLAYBACK_MAILBOX.md](../../../doc/PLAYBACK_MAILBOX.md). |
| [fvm_desktop_playback_mailbox_load_notify](../../../frontend/common/playback_mailbox.h#L66) | Owner load callback reads nonconsuming stop latch before/during fetch waits. [PLAYBACK_MAILBOX.md](../../../doc/PLAYBACK_MAILBOX.md). |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Initialize provider policy without workers/I/O. [BACKGROUND_TRANSFER.md](../../../doc/BACKGROUND_TRANSFER.md). |
| [fvm_desktop_load_monitor_init](../../../frontend/common/load_monitor.h#L49) | Initialize async/deadline observation defaults without jobs. [LOAD_MONITOR.md](../../../doc/LOAD_MONITOR.md). |
| [fvm_desktop_run_source_background_controlled](../../../frontend/common/playback.h#L86) | Synchronously borrow provider/monitor/control/source until joined owner cleanup, status 0/1/2. [PLAYBACK_CONTROL.md](../../../doc/PLAYBACK_CONTROL.md). |
| [fvm_desktop_run_source_controlled](../../../frontend/common/playback.h#L77) | Local/memory controlled runner with optional loader; borrows input until return, status 0/1/2. [PLAYBACK_CONTROL.md](../../../doc/PLAYBACK_CONTROL.md). |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L65) | Configured native worker byte provider; cooperative cancellation, no VM/SDL events. [SOCKET_TRANSPORT.md](../../../doc/SOCKET_TRANSPORT.md). |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L35) | Alternate worker-safe curl provider with SDK global-safety check. [BACKGROUND_TRANSFER.md](../../../doc/BACKGROUND_TRANSFER.md). |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L54) | Explicit SDK session teardown after every native mailbox/worker/window/transfer is gone. [PLAYER_HOST_OWNERSHIP.md](../../../doc/PLAYER_HOST_OWNERSHIP.md). |

The example native adapter uses: [SDL_CreateThread](https://wiki.libsdl.org/SDL2/SDL_CreateThread), [SDL_WaitThread](https://wiki.libsdl.org/SDL2/SDL_WaitThread), [SDL_AtomicGet](https://wiki.libsdl.org/SDL2/SDL_AtomicGet), [SDL_AtomicSet](https://wiki.libsdl.org/SDL2/SDL_AtomicSet), [SDL_GetTicks](https://wiki.libsdl.org/SDL2/SDL_GetTicks), [SDL_Delay](https://wiki.libsdl.org/SDL2/SDL_Delay).

GetTicks is initialized on main before the worker; short durations use unsigned
Uint32 subtraction. WaitThread is called once and its handle is never reused.
Every runner return signals done, including failure before READY. Cleanup order:
join → snapshot copy → mailbox release → shutdown. SDK teardown refuses live
mailboxes. Snapshot source owns its text; it is not a borrowed VM string.

Limits: playback stays on the native UI owner, without concurrent/reentrant runs.
Workers never enter VM/GC, draw or drain UI events. Timeline pause does not freeze
timers/network/explicit script seeks. STOP is cooperative and cannot preempt a
native callback/DNS. A five-second idle controller failure returns status 1; this
is not a hard provider-join deadline. The tutorial is muted, with no IDE GUI,
instruction debugger or complete Flash/SWF8 compatibility claim. Native/hardware
acceptance on other OSes and roadmap stage 9 remain open.
