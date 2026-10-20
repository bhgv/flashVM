# Host-observed loading and startup cancellation

[Русский](README.ru.md) · [Catalog](../../README.md#example-player-monitored_loading) ·
[API contract](../../../doc/LOAD_MONITOR.md) · [Cross-platform builds](../../../doc/SOCKET_TRANSPORT.md).

This independent C89 host uses a five-second total budget for each network job,
logs request IDs/kinds/phases/body bytes, and can stop startup before any provider
or window is created. Every C line is explained in [main.c](main.c). It reuses the
[standalone async movies and commented AS/generator](../async_loading/README.md):
there is no duplicate asset copy or dependency on test modules/unfinished compilers.

Requirements: C89/C++98 compilers, CMake and target SDL2 SDK. From repository root:

```sh
cmake -S flashVM -B build-monitor -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON -DFVM_PLAYER_SOCKET_HTTP=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-monitor --target flashvm_example_monitored_loading flashvm_player
python3 -m http.server 8000 --directory flashVM/examples/player/async_loading
```

In another terminal:

```sh
./build-monitor/frontend/flashvm_example_monitored_loading http://127.0.0.1:8000/start.swf
./build-monitor/frontend/flashvm_example_monitored_loading http://127.0.0.1:8000/start.swf --cancel-startup
```

First run: four network requests, live frame/timer assertions, notice log and exit 0.
Second run: BEGIN/CANCELLED, no HTTP request and exit 0. FETCHED is byte-fetch success;
a malformed SWF can still fail import afterward. There is no percentage estimate:
received counts provider body bytes, not wire size or a guaranteed Content-Length.
The callback copies scalar progress and borrows strings only during that call.

The example is a short loopback test with 700 ticks at 1000 FPS. For real films use
normal FPS/frame bounds and your application's stop policy. To exercise timeout,
point at a deliberately delayed server; it reports TIMED_OUT and playback failure.
Cancellation/deadline are cooperative: DNS/user callbacks can delay final join.
No worker detaches; caller state/provider storage stays live until runner returns.

For HTTPS enable FVM_PLAYER_SOCKET_TLS and supply target OpenSSL/trust. For curl
turn SOCKET_HTTP/TLS off and FVM_PLAYER_HTTP on; unsafe older curl global APIs are
rejected. Static/shared linking, SDK/CPU/toolchain/trust setup are in the build guide.
Cross-compilation is not target execution. Dummy SDL is protocol coverage only.

| API | Ownership / Владение |
| --- | --- |
| [fvm_desktop_background_supported](../../../frontend/common/background.h#L49) | Default worker capability, no network/window startup. |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Initialize caller playback scalars. |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Initialize caller worker/byte/load policy. |
| [fvm_desktop_load_monitor_init](../../../frontend/common/load_monitor.h#L49) | Initialize async delivery, zero deadline and no callback. |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L65) | Worker-safe native provider; nested userdata borrowed through return. |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L34) | Worker-safe curl alternative, requires thread-safe global SDK. |
| [fvm_desktop_run_source_monitored](../../../frontend/common/load_monitor.h#L57) | Whole-player call; notify runs on owner, provider on worker. |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L50) | Cleanup only after joined workers/window/audio disposal. |

The same policy is available through the [CLI/playlist adapter](../../../doc/LOAD_MONITOR.md#playlist-and-console-adapters-v0686).
