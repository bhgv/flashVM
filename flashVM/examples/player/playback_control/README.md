# Pause and single-step playback from C89

[Русский](README.ru.md) · [Catalog](../../README.md#example-player-playback_control) ·
[Control contract](../../../doc/PLAYBACK_CONTROL.md).

The standalone [main.c](main.c) demonstrates the IDE/host C interface: initial
pause, one step, a short nonblocking pause, another step, resume and STOP after
five ticks. `--step N` instead performs exactly N individual steps. Every nonempty
C/linked-byte/AS line is annotated. This controls the timeline, not individual
VM instructions. The thin CLI only fills descriptors; the library does not parse argv.

[make_movie.py](make_movie.py) uses Python stdlib and independently emits a real
two-frame [SWF8](start.swf): a blue square on the left, then an orange square on
the right. [movie.inc](movie.inc) contains identical immutable linked bytes.
[start.as](start.as) is an explanatory AS2 sketch, not AS/FIR compiler acceptance.

## Build and run

Requirements: C89/C++98 toolchain, CMake, graphics-enabled FlashVM profile and SDL2
2.0.9+ SDK for the target platform. Local/embedded modes need no networking, image
codecs or JIT. From the repository root:

```sh
cmake -S flashVM -B build-control -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DSDL2_DIR=/path/to/native-sdl/lib/cmake/SDL2
cmake --build build-control --target flashvm_example_playback_control --parallel
./build-control/frontend/flashvm_example_playback_control flashVM/examples/player/playback_control/start.swf
./build-control/frontend/flashvm_example_playback_control --step 3 flashVM/examples/player/playback_control/start.swf
./build-control/frontend/flashvm_example_playback_control --embedded --step 7 "$PWD/not-read.swf"
python3 flashVM/examples/player/playback_control/make_movie.py
ctest --test-dir build-control -R 'playback_control|playback_policy' --output-on-failure
```

Expected status 0 and `completed=5`, `completed=3`, `completed=7` respectively.
The stderr statistics contain matching frames and actual interpreter counters.
Embedded mode never reads not-read.swf: the absolute path supplies provenance/base
for possible relative requests. Do not use a relative path as an explicit override
of the platform executable-directory default.

For HTTP add `-DFVM_PLAYER_SOCKET_HTTP=ON` when configuring; HTTPS also requires
`-DFVM_PLAYER_SOCKET_TLS=ON` and target TLS libraries/trust. The alternate provider
uses `-DFVM_PLAYER_HTTP=ON` with worker-safe libcurl. Provider choice is a build
setting. Without one, local/embedded modes work and URL playback reports an error.

```sh
python3 -m http.server 8000 --bind 127.0.0.1 --directory flashVM/examples/player/playback_control
# Another terminal / Другой терминал:
./build-control/frontend/flashvm_example_playback_control --step 4 http://127.0.0.1:8000/start.swf
./build-control/frontend/flashvm_example_playback_control --embedded --step 4 http://127.0.0.1:8000/start.swf
```

The first run requests /start.swf; the embedded run skips startup I/O. This movie
has no child network requests; the separate control regressions check paused
publication/cancellation. For display-free checking use
`SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software`; that is not hardware acceptance.

## Other platforms and CPUs

Use a separate build directory, CMake toolchain and SDL SDK for the target CPU/OS.
TLS/curl and their transitive dependencies must also target that platform.
SDK preparation and Windows/Linux/macOS/Android setup are documented in
[DESKTOP_FRONTEND.md](../../../doc/DESKTOP_FRONTEND.md); static/shared host ownership
is described in [PLAYER_HOST_OWNERSHIP.md](../../../doc/PLAYER_HOST_OWNERSHIP.md).

```sh
cmake -S flashVM -B build-control-target \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/target-toolchain.cmake \
  -DSDL2_DIR=/path/to/target-sdl/lib/cmake/SDL2 \
  -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=OFF \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-control-target --target flashvm_example_playback_control --parallel
```

This desktop target is excluded on Android, which requires the Activity/surface
adapter documented in the general guide. ARM64/RISC-V64/Windows cross-linking is
not execution on those systems. For a detached VM component, replace `-S flashVM`
with its root path and select its profile explicitly; sibling projects are not required.

## API and ownership

Each function below links to its guide; init functions return 1/0 for ordinary/null
output pointers without allocation or native service startup.

| C API | Purpose, lifetime and blocking; guide |
| --- | --- |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Fill caller-owned scalar playback defaults; no I/O. [PLAYER_CONFIGURATION.md](../../../doc/PLAYER_CONFIGURATION.md). |
| [fvm_desktop_playback_control_init](../../../frontend/common/playback.h#L68) | Fill exact control ABI size, running state and null callback/userdata; no I/O. [PLAYBACK_CONTROL.md](../../../doc/PLAYBACK_CONTROL.md). |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Fill borrowed provider policy without starting workers. [BACKGROUND_TRANSFER.md](../../../doc/BACKGROUND_TRANSFER.md). |
| [fvm_desktop_load_monitor_init](../../../frontend/common/load_monitor.h#L49) | Fill asynchronous observation/deadline defaults without creating jobs. [LOAD_MONITOR.md](../../../doc/LOAD_MONITOR.md). |
| [fvm_desktop_run_source_controlled](../../../frontend/common/playback.h#L77) | Synchronously run source with optional loader and required control; borrow all storage until return; status 0/1/2. [PLAYBACK_CONTROL.md](../../../doc/PLAYBACK_CONTROL.md). |
| [fvm_desktop_run_source_background_controlled](../../../frontend/common/playback.h#L86) | Synchronously own playback; require provider/monitor/control; join workers before return, status 0/1/2. [PLAYBACK_CONTROL.md](../../../doc/PLAYBACK_CONTROL.md). |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L52) | Release SDK session after runner-owned windows/jobs are gone; caller controls process lifetime. [PLAYER_HOST_OWNERSHIP.md](../../../doc/PLAYER_HOST_OWNERSHIP.md). |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L65) | Selected native worker adapter; cooperatively fetch immutable bytes without VM/SDL calls. [SOCKET_TRANSPORT.md](../../../doc/SOCKET_TRANSPORT.md). |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L35) | Alternate curl worker adapter; checks SDK global thread safety before fetching. [BACKGROUND_TRANSFER.md](../../../doc/BACKGROUND_TRANSFER.md). |

observe borrows its snapshot only until return and retains scalars alone. It
returns exactly one command; KEEP does not resume. [GC:] The runner pins root;
do not unpin it or retain context/root. Copying a pointer or adding a pin cannot
extend the runner-owned context lifetime. Descriptors, argv/bytes and userdata
live until synchronous return; workers never enter VM. STOP ends the entire run
and joins cancelled jobs before cleanup. error is borrowed text; status 2 means
arguments, 1 runtime/provider failure, 0 normal completion.

Limits: native UI owner thread, no concurrent/reentrant runs; no ready-made IDE
GUI or interthread command queue. Timers/input/loading may mutate VM while paused.
Startup fetching/blocking waits do not call observe: use a monitor for cancellation
there. The tutorial is muted; separate dummy-audio fixtures check PCM suspension.
Full SWF8/reference and hardware acceptance on every OS remain open. This advances
the roadmap without completing stage 9.
