# Asynchronous child movies and form variables

[Русский](README.ru.md) · [Catalog](../../README.md#example-player-async_loading) ·
[Contracts](../../../doc/BACKGROUND_TRANSFER.md) · [Sources](../../../doc/PLAYER_SOURCES.md).

The C89 host selects [fvm_desktop_run_source_async](../../../frontend/common/background.h#L40). Network child/form requests
run on one FIFO worker at a time while the parent's frames, timers, input and GC
safe points continue. Completed bytes are imported/decoded only on the owner.
Startup and whole-root replacement still use the background runner's lifecycle;
this is a bounded async loading subset, not complete Flash loader compatibility.

This standalone example includes [C](main.c), commented [root AS sketch](start.as),
[child AS sketch](nested/child.as), generated SWFs and both form files. The
standard-library-only [generator](make_movies.py) emits deterministic AVM1 bytes
without importing test files or relying on an unfinished AS/FIR backend. The AS
files explain timeline frames; they are sketches, not parser/backend acceptance.
The ordinary fast-server example allows replies before its second frame. The
separate delayed-server regression deliberately requires a still-pending request
there to distinguish real asynchronous execution from waiting mode.

Requirements: C89/C++98 compilers, CMake and a target SDL2 SDK. From repository root:

```sh
cmake -S flashVM -B build-async -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON -DFVM_PLAYER_SOCKET_HTTP=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-async --target flashvm_player flashvm_example_async_loading
python3 flashVM/examples/player/async_loading/make_movies.py
python3 -m http.server 8000 --directory flashVM/examples/player/async_loading
```

In another terminal:

```sh
./build-async/frontend/flashvm_example_async_loading http://127.0.0.1:8000/start.swf
./build-async/frontend/flashvm_player --async-loads --loads 8 --frames 700 --fps 1000 --mute http://127.0.0.1:8000/start.swf
```

Expected: a plain stage, four HTTP requests (startup, child, root vars, child
vars), successful internal frame/timer/value assertions, then exit 0. This is a
fast bounded loopback demonstration, not an animation or a general Internet
latency benchmark. The root waits on two frames without reissuing initialization,
checks both origins and an actual script timer, clears that timer and stops.
Wrong/missing values trigger its bounded wait assertion. The host still joins
unfinished work on frame limit/close/error. Use a normal frame rate and suitable
application completion policy for a real movie. Dummy SDL proves only protocol
behavior; physical windows/foreign OS acceptance is separate.

For HTTPS enable FVM_PLAYER_SOCKET_TLS and supply the target OpenSSL SDK/trust;
for libcurl disable SOCKET_HTTP/TLS and enable FVM_PLAYER_HTTP. The linked curl
SDK must report thread-safe global APIs. The example reports unavailable worker
support before startup. Static SDK, target CPU/toolchain and trust-file rules are
in the transport guide. This example uses no mutable global provider setter.

| API | Purpose, ownership and blocking |
| --- | --- |
| [fvm_desktop_background_supported](../../../frontend/common/background.h#L49) | Query default provider worker capability without network/display startup; custom callbacks are separate. |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Initialize caller-owned scalar playback settings. |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Initialize caller-owned worker policy without starting a job. |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L64) | Native worker adapter, immutable borrowed options and cooperative cancellation. |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L34) | Curl worker adapter; rejects older unsafe global initialization. |
| fvm_desktop_run_source_async | Synchronous whole-player call with nonblocking child/form delivery at owner safe points. |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L49) | End this SDK session only after joined jobs and window/audio cleanup. |

All stack descriptors and nested provider userdata remain live/immutable until
return. No VM object crosses a worker boundary. Each active target stays pinned
until delivery or joined cancellation. Byte storage belongs to the job until
owner import finishes, then release frees it. DNS can delay final join; no worker
is forcibly killed or detached. Errors retain the strict player behavior rather
than inventing unimplemented MovieClipLoader progress/failure events.
