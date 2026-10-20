# Redirected SWFs with final per-movie origins

[Русский](README.ru.md) · [Catalog](../../README.md#example-player-redirect_loading) ·
[HTTP contract](../../../doc/HTTP_REDIRECTS.md) · [Build/SDK/CPU guide](../../../doc/SOCKET_TRANSPORT.md).

The [C89 host](main.c) calls the structured network-policy runner, without parsing
player command-line options. Every C line is explained. The [standalone server](serve.py)
uses only Python standard library and deterministic resources from the
[async tutorial](../async_loading/README.md), including commented AS and its generator.
No test directory or unfinished AS/FIR backend is imported.

The startup URL redirects to `/movies/main/start.swf`; its relative child redirects
to `/sprites/child.swf`. Root form data must resolve under `/movies/main/`, child
form data under `/sprites/`; both form endpoints redirect again. These four logical
resources produce eight HTTP requests. The movie asserts both values and real
frame/timer progress; wrong source bases fail. No intermediate redirect body is imported.

Requirements: C89/C++98, CMake, SDL2 target SDK, Python3. From repository root:

```sh
cmake -S flashVM -B build-redirect -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON -DFVM_PLAYER_SOCKET_HTTP=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-redirect --target flashvm_player flashvm_example_redirect_loading
python3 flashVM/examples/player/redirect_loading/serve.py 8000
```

In another terminal:

```sh
./build-redirect/frontend/flashvm_example_redirect_loading http://127.0.0.1:8000/start.swf
./build-redirect/frontend/flashvm_example_redirect_loading http://127.0.0.1:8000/start.swf --cancel-startup
./build-redirect/frontend/flashvm_player --async-loads --redirects 3 --loads 4 --load-progress --frames 700 --fps 1000 --mute http://127.0.0.1:8000/start.swf
```

Expected normal run: blank assertion stage, eight requests, four logical load IDs,
exit 0. The stop example emits BEGIN/CANCELLED with no HTTP requests or player view,
exit 0. Logical max_loads=4 and per-resource max_redirects=3 are independent. This
short loopback tutorial uses 700 ticks/1000FPS; choose normal FPS and your own frame/
stop policy for real films. Server binds loopback only and stops with Ctrl-C.

All descriptors, ca_file and observer userdata remain immutable/borrowed until the
runner returns. Observers run on owner, network work on joined workers. DNS or a
noncooperative callback can delay join. FETCHED means complete fetch, before SWF
import; errors still fail playback. There is no percentage or total-size promise.

HTTPS requires native TLS and a target OpenSSL/trust SDK. Curl builds disable
SOCKET_HTTP/TLS and enable FVM_PLAYER_HTTP with a thread-safe linked curl SDK.
Set policy.ca_file in a C host when an explicit PEM trust file is required. Static/
shared SDKs, target CPU/toolchains and trust setup are documented in the build guide.
Cross-linking is not foreign execution; dummy SDL is protocol coverage only.

| API | Purpose / Назначение |
| --- | --- |
| [fvm_desktop_background_supported](../../../frontend/common/background.h#L49) | Default worker capability, no network/window startup. |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Initialize caller playback settings. |
| [fvm_desktop_network_policy_init](../../../frontend/common/network_loader.h#L13) | Initialize redirect/deadline/trust policy, including provider-less SDKs. |
| [fvm_desktop_load_monitor_init](../../../frontend/common/load_monitor.h#L49) | Async delivery and caller-owned observer policy. |
| [fvm_desktop_run_source_network](../../../frontend/common/network_loader.h#L22) | Whole-player source call; default provider honors HTTP policy on every logical resource. |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L54) | Cleanup after workers join and owner disposes the active view. |

For an external HTTPS server, the tutorial host accepts an explicit PEM via
FLASHVM_CA_FILE; this is an example-host setting, not SDK global configuration:

```sh
FLASHVM_CA_FILE=/path/to/ca.pem ./build-redirect/frontend/flashvm_example_redirect_loading https://example.test/start.swf
```

CLI callers use --http-ca-file PATH; C/IDE callers assign policy.ca_file. The local
serve.py is deliberately plain HTTP and does not terminate TLS. Keep trust settings
borrowed/immutable through return; redirects reverify each destination certificate.
