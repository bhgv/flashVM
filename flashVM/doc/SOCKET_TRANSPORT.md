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
| [fvm_desktop_socket_options_init](../frontend/common/socket_http.h#L36) | Nonnull caller-owned [FVM_SocketHTTPOptions](../frontend/common/socket_http.h#L27) receives 10000ms connect and 30000ms total. Returns 1; null returns 0. No I/O/allocation or retained pointers. |
| [fvm_desktop_socket_fetch](../frontend/common/socket_http.h#L43) | reference, write, resolved and nonzero capacity are required. base is optional for absolute URLs. userdata is null for defaults or a borrowed options pointer. Synchronously sends GET and streams decoded binary chunks through write(bytes,size,write_userdata). Resolved receives canonical provenance plus NUL only on success. |

Options require 0 < connect_ms <= total_ms <= 120000. DNS is synchronous
getaddrinfo: elapsed resolver time counts, but the resolver call itself cannot
be interrupted. Nonblocking connect, partial sends/reads and readiness waits use
one monotonic deadline rather than restarting a timeout for each byte. Sink code
is caller code and cannot be preempted by the provider. The complete synchronous
callback can therefore block beyond total_ms in DNS or a slow sink. A future
asynchronous resolver/cancellation API is a separate task.

[FVM_OK](../include/flashvm/flashvm.h#L44) means framing and resource delivery completed. [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47) means
bad input, limits, insufficient canonical output capacity or an excessive body.
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48) rejects HTTPS when no TLS transport is built. [FVM_ERROR](../include/flashvm/flashvm.h#L45) covers
native connection, I/O, deadline, status and malformed response failures. An
allocation failure in the platform socket constructor currently maps to [FVM_ERROR](../include/flashvm/flashvm.h#L45).
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
stages remain open. System DNS interruption and asynchronous loading still need separate implementation/acceptance evidence.
## Verified HTTPS since v0.678

Enable FVM_PLAYER_SOCKET_TLS in addition to FVM_PLAYER_SOCKET_HTTP. Supply an
OpenSSL SDK version 1.1.1 or later (tested with 3.0). SSL_VERIFY_PEER, chain
verification, DNS/IP identity, DNS SNI and minimum TLS1.2 are always enabled.
No option disables verification. A custom BIO routes every encrypted read/write
through the same native TCP adapter, preserving total deadline and local SIGPIPE
suppression. TLS1.2 and TLS1.3 loopback tests are distinct.

```sh
cmake -S flashVM -B build-socket -DFVM_PLAYER_SOCKET_HTTP=ON \
  -DFVM_PLAYER_HTTP=OFF -DFVM_PLAYER_SOCKET_TLS=ON -DOPENSSL_USE_STATIC_LIBS=TRUE
cmake --build build-socket --target flashvm_player flashvm_example_socket_loading
./build-socket/frontend/flashvm_example_socket_loading https://localhost:8443/start.swf /absolute/test-ca.pem
```

This assumes the player/SDL/render options from the initial build command.
OPENSSL_USE_STATIC_LIBS selects target static OpenSSL archives; it does not make
the entire OS executable fully static. Set OPENSSL_ROOT_DIR and CMake's target
find-root/sysroot for a foreign SDK. Never mix host OpenSSL with a cross target.
With TLS OFF no OpenSSL headers/libraries are required; HTTPS returns
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48) before DNS/TCP. TLS ON without SOCKET_HTTP is a configuration error.

[FVM_SocketHTTPOptions](../frontend/common/socket_http.h#L27) now includes ca_file, borrowed until fetch returns. Always
initialize the structure with [fvm_desktop_socket_options_init](../frontend/common/socket_http.h#L36), which sets ca_file
to null. Null selects OpenSSL default trust paths, including its standard
SSL_CERT_FILE/SSL_CERT_DIR environment configuration. A nonnull filename selects
only that PEM CA bundle for this request; missing/empty/unreadable/invalid trust
or verification failure returns [FVM_ERROR](../include/flashvm/flashvm.h#L45) before HTTP payload delivery. Trust
paths use the OpenSSL SDK path encoding, not SDL's UTF-8 file adapter. CA paths
belong to the native host filesystem, not the movie URL base. Do not accept a
trust override from untrusted movie content. No mutable global setter exists.
Android/Windows apps may need a deployed PEM bundle: OpenSSL default paths do not
automatically mean the Android/Windows native certificate store.

TLS setup/CA reads, like DNS and sink code, cannot be preempted by this synchronous
API. Elapsed time is checked before/after handshake and by all underlying native
I/O. Independent context/session/BIO objects are released before their socket.
The provider never calls OPENSSL_cleanup or changes global trust. OpenSSL's
calling-thread error queue is cleared for each TLS operation and may contain
transport errors afterwards; consume host errors before invoking this provider.
Other threads' queues are untouched.

Known-length/chunked HTTPS finishes at its framed boundary without waiting for
TLS shutdown. EOF-framed HTTPS requires authenticated close_notify; abrupt TCP
EOF fails, including with OpenSSL versions exposing it as SSL_ERROR_SYSCALL.
Cleanup closes the one-use connection without an extra blocking SSL_shutdown.
No session cache, client certificates, proxies, revocation fetching, pinning API
or HTTP/2 is added. Verification follows the selected OpenSSL trust/policy setup.

Tests create a temporary CA and valid, wrong-name and expired certificates. They
check DNS/IP identity, SNI, custom/default trust, untrusted rejection, TLS1.2/1.3,
clean/truncated EOF, slow-body/handshake deadlines, exact sink errors,
interpreter/JIT HTTPS child/form origins and the executable C host. Temporary
private keys are deleted and never committed. CMake registers this integration
test when a host openssl executable, examples and native tests are available.
The transport probe also passes UBSan in Linux; OpenSSL and the rest of the VM
are not instrumented by that probe. Foreign TLS SDK builds/runs remain unverified;
plain HTTP cross-build evidence is not foreign TLS acceptance.
## Build capability query since v0.679

[network.h](../frontend/common/network.h) defines [fvm_desktop_network_capabilities](../frontend/common/network.h#L19),
which returns [FVM_DESKTOP_NETWORK_HTTP](../frontend/common/network.h#L5), [FVM_DESKTOP_NETWORK_HTTPS](../frontend/common/network.h#L6),
[FVM_DESKTOP_NETWORK_SOCKET](../frontend/common/network.h#L7) and [FVM_DESKTOP_NETWORK_CURL](../frontend/common/network.h#L8) bits, or zero with no
default provider. It opens no file, socket, SDL subsystem, VM or trust store.
It describes the compiled default, not a custom loader.fetch or current Internet/
certificate/device readiness. A separate function avoids changing the existing
FVM_DesktopBuildInfo layout. Use the native host thread; with older libcurl,
coordinate concurrent initialization/queries according to that SDK's thread rules.
The standalone transport functions [fvm_desktop_socket_capabilities](../frontend/common/socket_http.h#L47) and
[fvm_desktop_http_capabilities](../frontend/common/http.h#L26) report their own linked provider. The libcurl query
checks the actual linked protocol list/SSL feature rather than assuming HTTPS.

`--version` prints network_provider=none/socket/curl, http=yes/no, https=yes/no and
fetch=synchronous without display/network startup. It works with deliberately
invalid SDL video/audio driver names, as checked by the TLS and metadata fixtures.
These flags are build metadata, not an external-service health check.

The final native selection covers 78 tests; after fixing the new catalog entry
order and rebuilding a local helper executable, focused reruns passed all failed
cases. Shared-host TLS selection passes 12 tests, libcurl selection 9 and
no-provider C/C++98 metadata selection 2. ARM64/RISC-V64 Linux and Windows x86_64
plain HTTP player/tutorial cross-builds link; the Windows DLL exports the new
query and its C/C++98 consumers link. Foreign binaries are not executed.
Ten thousand generated URI references additionally pass canonical round-trip
checks under ASan/UBSan. LeakSanitizer is disabled in that harness because the
sandbox lacks /proc task access; this is not a leak-check claim. TLS/raw-HTTP
transport probes pass UBSan, without instrumenting the OpenSSL SDK or VM.
[Controlled cancellation / Управляемая отмена](BACKGROUND_TRANSFER.md), v0.680.
[Explicit bounded GET redirect policy](HTTP_REDIRECTS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
