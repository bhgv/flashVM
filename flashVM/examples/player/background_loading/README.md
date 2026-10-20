# Background provider loading from C

[Русский](README.ru.md) · [Catalog](../../README.md#example-player-background_loading) ·
[Contracts](../../../doc/BACKGROUND_TRANSFER.md) · [Sources](../../../doc/PLAYER_SOURCES.md).

This executable C89 example selects a worker-safe provider per run. It loads the
startup/child/form tutorial through native POSIX/Winsock or optional libcurl.
The UI owner keeps handling its active native window while network bytes arrive.
The timeline and script timers wait at the resource safe point; this is not fully
asynchronous Flash loading. Startup and whole-root transitions have no active
player view; use the lower-level transfer API for a host-owned startup progress UI.

Requirements: C89/C++98 compilers, CMake and target SDL2 SDK; native sockets need
no libcurl. From the repository root:

```sh
cmake -S flashVM -B build-background -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON -DFVM_PLAYER_SOCKET_HTTP=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-background --target flashvm_player flashvm_example_background_loading
python3 -m http.server 8000 --directory flashVM/examples/player/loading
```

In another terminal:

```sh
./build-background/frontend/flashvm_example_background_loading http://127.0.0.1:8000/start.swf
./build-background/frontend/flashvm_player --background-loads --loads 8 --frames 3 --mute http://127.0.0.1:8000/start.swf
```

Expected: startup imports its child and root/child form values, assertions pass,
then status 0. Closing during an active child fetch requests cancellation, joins
the worker and returns 0 without importing partial bytes. DNS/noncooperative
callbacks can delay join. No native worker is detached and no files are written.
Dummy SDL drivers may be used only for headless protocol tests, not device acceptance.

For HTTPS additionally enable FVM_PLAYER_SOCKET_TLS with the target OpenSSL SDK;
trust uses OpenSSL defaults (including SSL_CERT_FILE) or explicit native options.
For libcurl disable SOCKET_HTTP/TLS and enable FVM_PLAYER_HTTP; background mode
requires CURL_VERSION_THREADSAFE in the linked SDK. See the transport guide for
static SDK and foreign OS/CPU build instructions. Cross-linking is not target execution.

[fvm_desktop_background_supported](../../../frontend/common/background.h#L49)() checks default worker support before startup.
The asynchronous companion is [here](../async_loading/README.md).

[main.c](main.c) explains every line. The options, source URL and loader remain
borrowed until return; nested userdata must remain immutable/thread-safe. Native
bytes are accumulated on the worker; VM import and setters remain on the owner.
No callback receives a VM object. All jobs join before the SDK shutdown call.

| API | Purpose, ownership and blocking |
| --- | --- |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Initialize caller scalar playback options without startup. |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Initialize caller background policy without starting a worker. |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L65) | Worker-safe native provider with borrowed options and cooperative cancellation; DNS can block. |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L35) | Worker-safe libcurl adapter; rejects a linked SDK lacking thread-safe global APIs. |
| [fvm_desktop_run_source_background](../../../frontend/common/background.h#L28) | Synchronous owner loop, pumping native events during network waits and importing only completed bytes. |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L53) | End the matching SDK only after joined workers and window cleanup; live jobs make it fail. |

The detailed [C API contract](../../../doc/BACKGROUND_TRANSFER.md) lists arguments,
errors and lifetime rules for these functions and the lower-level begin/poll/
cancel/release API used by IDE hosts with their own event loop.
