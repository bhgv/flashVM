# Native socket SWF loading

[Русский](README.ru.md) · [Catalog](../../README.md#example-player-socket_loading) ·
[Transport contract](../../../doc/SOCKET_TRANSPORT.md) · [Source contract](../../../doc/PLAYER_SOURCES.md).

This executable C89 host selects the POSIX/Winsock HTTP callback explicitly. It
runs a startup SWF and permits child movies and variable requests from that SWF.
It is useful for an IDE, a native app or a player library consumer that does not
want a libcurl dependency. This is synchronous playback on the UI/VM owner thread.

Requirements: C89/C++98 compilers, CMake, a target SDL2 SDK and the normal player
render profile. From the repository root, using your installed SDL2 package:

```sh
cmake -S flashVM -B build-socket -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_PLAYER_HTTP=OFF -DFVM_PLAYER_SOCKET_HTTP=ON
cmake --build build-socket --target flashvm_example_socket_loading
python3 -m http.server 8000 --directory flashVM/examples/player/loading
```

In another terminal:

```sh
./build-socket/frontend/flashvm_example_socket_loading http://127.0.0.1:8000/start.swf
```

Expected: the generated startup movie imports `nested/child.swf`, loads root and
child form data, checks their values, and the host finishes after three ticks
with status 0. The HTTP server records startup, child and both variable requests.
Failure to import or fetch a resource returns a nonzero status. No files are
written. On a headless test host only, set SDL_VIDEODRIVER=dummy and
SDL_RENDER_DRIVER=software; that is not physical window acceptance.

[main.c](main.c) explains every line. The source descriptor, loader and network
options live on the main stack until the synchronous runner returns. The provider
closes each socket on every success/error path; the runner owns temporary resource
bytes and GC pins. The caller ends the matching player library session afterwards.

| API | Purpose and ownership/blocking behavior |
| --- | --- |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Initialize caller-owned scalar player options; no allocation or display access. |
| [fvm_desktop_loader_init](../../../frontend/common/source.h#L51) | Initialize caller-owned callback policy with no transport and one load. |
| [fvm_desktop_socket_options_init](../../../frontend/common/socket_http.h#L32) | Initialize caller-owned native transport deadlines; opens no sockets. |
| [fvm_desktop_socket_fetch](../../../frontend/common/socket_http.h#L39) | Synchronously resolve/connect/fetch each resource through a borrowed sink; closes owned sockets and retains no pointers. |
| [fvm_desktop_run_source](../../../frontend/common/source.h#L60) | Run on the UI/VM thread, borrowing all descriptors until return; resource callbacks execute at safe points. |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L45) | End the matching player-library session after playback; refuses active SDL subsystems. |

Each API's detailed arguments/status/lifetime contract is linked in the
[transport guide](../../../doc/SOCKET_TRANSPORT.md#c-api) and
[source guide](../../../doc/PLAYER_SOURCES.md). The tutorial uses explicit 2s TCP
and 5s total per-resource I/O budgets and eight resource loads. DNS is a blocking
system resolver operation and cannot be interrupted by those deadlines. The
provider supports a conservative HTTP/1.x subset; no redirects, proxies, POST or
automatic outgoing variable serialization. HTTPS requires a separate verified TLS
build. Cross-compilation requires a matching target SDL SDK; see the transport
build guide. Foreign runtime acceptance remains separate.
