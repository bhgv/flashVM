# Native socket player transport

[Русский](SOCKET_TRANSPORT.ru.md) · [Sources and loading](PLAYER_SOURCES.md) ·
[Desktop/native/cross build](DESKTOP_FRONTEND.md) · [Example](../examples/player/socket_loading/README.md).

Since v0.677 the player can load HTTP resources directly through POSIX or Winsock
TCP sockets without libcurl. Startup URL movies, SWF-requested child movies,
root replacement and loadVariables use the same provider and per-movie canonical
base. Embedded movies can use it for absolute URLs or an explicitly supplied HTTP
origin. This does not implement XMLSocket, a sandbox/security policy, asynchronous
fetch, progressive SWF parsing or a complete Flash networking API.

## Build and provider selection

```sh
cmake -S flashVM -B build-socket -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON \
  -DFVM_PLAYER_HTTP=OFF -DFVM_PLAYER_SOCKET_HTTP=ON
cmake --build build-socket --target flashvm_player flashvm_example_socket_loading
./build-socket/frontend/flashvm_player --loads 8 --frames 3 --mute http://127.0.0.1:8000/start.swf
```

Supply SDL2_DIR for a non-system target SDK. FVM_PLAYER_HTTP selects the existing
libcurl HTTP(S) provider; FVM_PLAYER_SOCKET_HTTP selects this native provider.
They are mutually exclusive default-provider choices; enabling both gives a
configuration error. Both default OFF. The core VM remains independent of SDL
and either transport. An embedding host may still replace loader.fetch with its
own callback. The separate flashvm_socket_transport static target exports the
callback and needs no VM runtime or SDL initialization.

For a shared player host set FVM_PLAYER_HOST_SHARED=ON; the transport remains a
PIC static dependency available to C/C++ clients through the CMake target. On
Windows link ws2_32 (propagated by that target); every request balances its own
WSAStartup/WSACleanup reference. The library does not shut down someone else's
Winsock ownership and never installs a process-wide SIGPIPE handler.

For cross-compilation use the [explicit toolchain](../../toolchains/explicit.cmake)
with FVM_TARGET_SYSTEM, FVM_TARGET_PROCESSOR, FVM_TARGET_CC/FVM_TARGET_CXX and a
matching target SDL2_DIR; see the complete SDK/toolchain examples in the desktop
guide. Native sockets use target system headers/libraries, never host libcurl.
Linux ARM64/RISC-V64 and Windows x86_64 compile/link checks are separate from
actual execution. macOS/Android still need their real SDK and application/device
acceptance. Android additionally requires the manifest INTERNET permission and
an actual Activity/surface host; cleartext network policy belongs to that host.

## C API

The public callback declarations are in [socket_http.h](../frontend/common/socket_http.h).
The loader contract is in [source.h](../frontend/common/source.h).

| API | Contract |
| --- | --- |
| [fvm_desktop_socket_options_init](../frontend/common/socket_http.h#L32) | Nonnull caller-owned FVM_SocketHTTPOptions receives 10000ms connect and 30000ms total. Returns 1; null returns 0. No I/O/allocation or retained pointers. |
| [fvm_desktop_socket_fetch](../frontend/common/socket_http.h#L39) | reference, write, resolved and nonzero capacity are required. base is optional for absolute URLs. userdata is null for defaults or a borrowed options pointer. Synchronously sends GET and streams decoded binary chunks through write(bytes,size,write_userdata). Resolved receives canonical provenance plus NUL only on success. |

Options require 0 < connect_ms <= total_ms <= 120000. DNS is synchronous
getaddrinfo: elapsed resolver time counts, but the resolver call itself cannot
be interrupted. Nonblocking connect, partial sends/reads and readiness waits use
one monotonic deadline rather than restarting a timeout for each byte. Sink code
is caller code and cannot be preempted by the provider. The complete synchronous
callback can therefore block beyond total_ms in DNS or a slow sink. A future
asynchronous resolver/cancellation API is a separate task.

[FVM_OK](../include/flashvm/flashvm.h#L42) means framing and resource delivery completed. [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) means
bad input, limits, insufficient canonical output capacity or an excessive body.
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) rejects HTTPS when no TLS transport is built. [FVM_ERROR](../include/flashvm/flashvm.h#L43) covers
native connection, I/O, deadline, status and malformed response failures. An
allocation failure in the platform socket constructor currently maps to [FVM_ERROR](../include/flashvm/flashvm.h#L43).
A sink error is returned exactly and ends delivery immediately. Some payload
bytes may already have reached the sink before a later framing/network error;
callers must discard their partial resource on any non-OK result. The player
accumulator already does so. Output provenance is unchanged on failure.

All argument strings/options and sink state are borrowed until return. Every
socket/buffer is released on every path. There is no VM allocation, GC root or
hidden callback registration in the transport. When used by the player, callbacks
run synchronously at owner-thread safe points: do not recurse the player, call
VM/SDL or destroy host state from a sink/fetch callback. External host side effects
in a sink are not transactional and cannot be rolled back.

## Supported protocol and explicit limits

The URI resolver accepts escaped ASCII HTTP(S), DNS names, IPv4 and bracketed
IPv6, numeric ports 1..65535, relative paths, network-path references and queries.
It removes literal dot segments, lowercases host/scheme, normalizes the numeric
port and removes fragments. Percent escapes remain encoded, including escaped
dots/NUL. Default ports are omitted from canonical provenance/Host. Credentials,
spaces/controls, backslashes, malformed escapes, non-ASCII unescaped text,
unsupported schemes and scoped IPv6 identifiers are rejected. International names
must be supplied as ASCII IDNA and paths as percent-encoded bytes. This is a
bounded HTTP URI subset, not a general URI library.

The provider sends HTTP/1.1 GET with Host, Connection: close and
Accept-Encoding: identity. It accepts HTTP/1.0 or 1.1 successful final 2xx responses,
bounded informational responses except 101, Content-Length, chunked coding with
validated extensions/trailers, or EOF framing. 204/205 must have no nonzero body
length or chunk coding. Framed bodies finish without waiting for connection close.
No connection reuse, redirects, proxy discovery, cookies, authentication, POST,
automatic outgoing variables or compressed content decoding occurs.

The parser deliberately rejects duplicate Content-Length (even equal values),
Content-Length combined with Transfer-Encoding, unknown/multiple transfer codings,
non-identity content encodings, folded fields, bare LF, forbidden controls,
whitespace before a field colon and framing fields in trailers. These strict
policies can reject permissive servers; they never silently reinterpret ambiguity.
Limits: 4096-byte URI, 8192-byte line, 32768 aggregate status/header/trailer bytes,
256 fields, 8 interim responses, 65536 chunks and 64MiB decoded body. Oversized
announced bodies fail before their payload is delivered. Truncation before a
framed end fails even when the partial bytes happen to form a valid SWF.

## Evidence and remaining acceptance

Transport-only tests use an independent raw loopback TCP server with binary NUL,
split writes, valid/invalid framing, trailers, limits, exact callback failure,
slow-byte and stalled-response deadlines. URI tests include RFC3986 resolution
examples and adversarial input. Player tests run actual interpreter/JIT root and
child/form loading against a loopback HTTP server with the socket provider.

```sh
cmake --build build-socket --target flashvm_socket_url flashvm_socket_client flashvm_frontend_memory_origin
ctest --test-dir build-socket -R 'socket_|player_http|player_child_loading' --output-on-failure
```

Native dummy-display loopback success is not remote Internet, native foreign OS,
physical window or complete Flash compatibility acceptance. The nine roadmap
stages remain open. TLS, system DNS interruption and asynchronous loading need
separate implementation/acceptance evidence.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
