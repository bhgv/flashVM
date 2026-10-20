# Deferred child unload, cancellation and reload

[Русский](README.ru.md) · [Catalog](../../README.md#example-player-unload_loading) ·
[Exact lifecycle contract](../../../doc/MOVIE_UNLOAD.md).

This executable C89 host starts a SWF8 which creates child/grandchild/peer clips,
starts a delayed form request, then clears the child on the next timeline frame.
The old active request is cancelled, the grandchild's queued request never starts,
the peer request survives, and a later child request reloads the same instance.
Script assertions check identity, an own property, deleted child bindings, absence
of the cancelled poison field and successful unrelated/fresh data delivery.

Files: fully annotated [main.c](main.c), annotated timeline [start.as](start.as),
standalone standard-library [bytecode generator](make_movies.py), executable
[start.swf](start.swf), [slow](slow.txt)/[never](never.txt)/[peer](peer.txt)/
[fresh](fresh.txt) form files and delayed loopback [server](serve.py).
The AS file is an explanatory sketch; the executable SWF is emitted directly by
this generator. No test imports or unfinished compiler backend are required.

## Native build and run

Requirements: C89/C++98 compilers, CMake, Python3 for generation/server, target SDL2
SDK and a worker-capable native socket or libcurl provider. From repository root:

```sh
cmake -S flashVM -B build-unload -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON -DFVM_PLAYER_SOCKET_HTTP=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-unload --target flashvm_player flashvm_example_unload_loading
python3 flashVM/examples/player/unload_loading/make_movies.py
python3 flashVM/examples/player/unload_loading/serve.py
```

From a second terminal:

```sh
./build-unload/frontend/flashvm_example_unload_loading http://127.0.0.1:8000/start.swf
./build-unload/frontend/flashvm_player --interpreter --async-loads --loads 8 --frames 600 --fps 1000 --mute http://127.0.0.1:8000/start.swf
ctest --test-dir build-unload -R 'unload_loading' --output-on-failure
```

Expected: blank stage and exit 0; HTTP startup, peer and fresh requests in that
order, optionally interleaved with the cancelled slow request. `/never.txt` must
not be fetched. Cancellation can occur before the slow socket reaches the server.
The server intentionally delays `slow.txt`; the client closes that obsolete socket.
A wrong fresh reply fails the bounded script assertion. The fast frame rate and
600-frame limit suit loopback demonstration, not an Internet timeout guarantee.

## Ownership and API

All calls use the native UI/VM owner thread. The synchronous player call borrows
stack descriptors, URL and provider userdata until return. Workers only manipulate
native bytes; the owner pins managed targets and joins workers before unpinning.
The script only queues unloading; the owner commits it after leaving VM execution.
No script value or VM pointer enters the network worker. [fvm_desktop_shutdown](../../../frontend/common/player.h#L51)
ends the SDK session after all request and window cleanup.

| C API | Purpose, ownership and blocking; guide |
| --- | --- |
| [fvm_desktop_background_supported](../../../frontend/common/background.h#L49) | Query worker capability without starting I/O; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Initialize caller-owned scalar settings; PLAYER_CONFIGURATION.md. |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Initialize caller-owned provider policy without a worker; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L65) | Native worker adapter with borrowed immutable policy and cooperative stop; SOCKET_TRANSPORT.md. |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L35) | Alternate worker adapter; checks safe curl global initialization; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_run_source_async](../../../frontend/common/background.h#L40) | Borrow descriptors throughout synchronous playback; child/form delivery occurs at owner safe points; BACKGROUND_TRANSFER.md. |
| `fvm_desktop_shutdown` | End native SDK ownership only after synchronous return and joined jobs; PLAYER_HOST_OWNERSHIP.md. |

Detailed [background policy](../../../doc/BACKGROUND_TRANSFER.md),
[scalar settings](../../../doc/PLAYER_CONFIGURATION.md),
[socket transport](../../../doc/SOCKET_TRANSPORT.md),
[SDK ownership](../../../doc/PLAYER_HOST_OWNERSHIP.md).

## Cross-build and limits

Use the target SDL2 SDK and toolchain from [native/cross guide](../../../doc/DESKTOP_FRONTEND.md)
and [socket guide](../../../doc/SOCKET_TRANSPORT.md); build the same
`flashvm_example_unload_loading` target. Windows uses `.exe`. Linux ARM64/RISC-V64
and Windows x86_64 cross-link checks do not establish target runtime acceptance.
Libcurl builds disable SOCKET_HTTP and enable FVM_PLAYER_HTTP with a thread-safe
static target SDK. HTTPS additionally requires explicit target TLS libraries/trust;
the supplied server intentionally uses HTTP loopback.

Root/_level0 unloading and synthetic onUnload/MovieClipLoader events remain
unsupported. Independent audio-device lifetime and uninterruptible DNS remain
host/provider concerns. Close/error joins native jobs; no worker is forcibly killed.
Dummy SDL verifies protocol/state behavior, not a physical window or remote OS.
