# Bounded HTTP GET redirects (v0.690)

[Русский](HTTP_REDIRECTS.ru.md) · [Socket transport](SOCKET_TRANSPORT.md) ·
[Load monitoring](LOAD_MONITOR.md).

[fvm_desktop_http_policy_init](../frontend/common/http_policy.h#L31) initializes [FVM_DesktopHTTPPolicy](../frontend/common/http_policy.h#L21): no redirects,
10s connect/30s total budget, default trust. Explicit native callbacks
[fvm_desktop_socket_fetch_policy](../frontend/common/socket_http.h#L72) and [fvm_desktop_socket_transfer_policy](../frontend/common/socket_http.h#L76) take this
policy as required userdata. The worker adapter additionally borrows [FVM_NetControl](../ports/network/socket.h#L21).
Existing socket options, controlled requests and callbacks keep their ABI/behavior.

Choose max_redirects=0..[FVM_HTTP_REDIRECT_MAX](../frontend/common/http_policy.h#L6) (8), connect_ms>0 and <=total_ms,
total_ms<=[FVM_NET_MAX_MS](../ports/network/socket.h#L8) (120s), optional ca_file. Invalid policy starts no I/O.
The total budget spans every connection/header/body, including elapsed DNS, with
monotonic rollover-safe samples. DNS/CA/user callbacks remain non-preemptible.
Job deadlines/cancellation from the host still apply around the complete chain.

Only GET 301/302/303/307/308 are followed. Each nonempty, single Location is resolved
against the current effective URL using the existing strict ASCII HTTP(S) resolver.
Relative paths, queries and authority references work; credentials, controls,
spaces, malformed escapes and unsupported schemes fail before the next connection.
HTTPS-to-HTTP downgrade is rejected. Other origins may be followed within bounds;
this is a host transport policy, not Flash sandbox/domain authorization.

Intermediate bodies are neither forwarded nor awaited; each hop closes its own
connection after bounded headers. Missing/duplicate/overlong Location, unsupported
3xx, self-reference or exhausted hop budget fails. Multi-URL cycles are bounded
by the same hop budget; there is no persistent redirect cache. Final bytes may
still be partial on later sink/network failure: discard them on any non-OK result.
The exact failing sink result survives. Resolved output remains unchanged until
complete final success, then contains the final fragment-free canonical URL.

The native regression uses independent raw HTTP responses and binary NUL bytes,
all five statuses, relative/query/authority/other-host destinations, malicious
headers/URIs, disabled/overflow limits, loops, sink failure, timeout and cancellation.
Existing native URL/framing tests still pass. This does not certify a complete
browser/Flash networking stack or foreign runtime/device acceptance.

Protocol reference: [RFC9110, Location and redirection](https://www.rfc-editor.org/rfc/rfc9110.html#section-10.2.2).

## Matching libcurl profile and verified TLS (v0.688)

[fvm_desktop_http_fetch_policy](../frontend/common/http.h#L43) and [fvm_desktop_http_transfer_policy](../frontend/common/http.h#L47) use the same
policy/URI/chain loop as the socket provider. Worker calls retain the linked SDK's
thread-safe-global prerequisite. libcurl does not auto-follow: bounded header
callbacks abort before intermediate bodies; each next hop is explicitly validated.
The explicit profile disables environment proxies and does not add authentication,
cookies or decompression. Existing curl callbacks keep their old behavior.

ca_file applies to every TLS hop via the provider's verified trust API; null retains
that provider's defaults. No peer/hostname verification is disabled. Tests use a
local temporary CA to prove HTTP→HTTPS upgrade, verified HTTPS→HTTPS, wrong-certificate
failure on a later hop and rejection of HTTPS→HTTP before plain-server contact.
Both providers pass the same raw HTTP redirect regression and TLS chain tests.
Old callbacks still reject redirects. DNS/CA/provider code can delay final cleanup;
no hard preemption or full Flash/browser policy is claimed.

## Player/IDE and console integration (v0.689)

[fvm_desktop_network_policy_init](../frontend/common/network_loader.h#L13) is the always-linkable host initializer, even in a
provider-less SDK. [fvm_desktop_run_source_network](../frontend/common/network_loader.h#L22) and [fvm_desktop_run_network](../frontend/common/network_loader.h#L28)
combine required HTTP policy and load monitor with the compiled default worker
provider. Old source/options/loader/monitor layouts remain unchanged. Invalid
policy/monitor fails before I/O/callback/window. Provider-less local/memory playback
remains available; network input is unsupported. Observer stop ends the playlist.

`--redirects N` accepts strict integer 0..8 and requires --async-loads or
--background-loads. Missing/duplicate/negative/fractional/out-of-range input fails
before startup; after -- the spelling is a literal filename. It selects the same
C policy runner, preserving optional --load-timeout/--load-progress. Existing CLI
without this opt-in still rejects redirects. Every successful final URL becomes
the source base for that movie, including memory-origin descendants and root
replacement. max_loads counts logical resources; hops have a separate limit.

The [annotated C/server tutorial](../examples/player/redirect_loading/README.md)
exercises four logical resources/eight HTTP requests with moved root/child/form
bases. Actual player regressions cover interpreter/JIT, waiting/async, native HTTPS,
C source/memory entrypoints, stop-before-I/O, malformed policy and old no-redirect
negative controls. This remains bounded GET loading, not full Flash network APIs.

## Explicit console trust files (v0.690)

`--http-ca-file PATH` selects the same network-policy runner and borrows a nonempty
bounded PEM path. It requires --async-loads or --background-loads; duplicate/missing/
empty paths are argument errors. No redirects are enabled implicitly. A supplied
PEM applies independently to every TLS connection, with peer/hostname checks still
required. Missing/unreadable/untrusted files fail playback before importing bytes.
Local sources do not open trust files until they actually request HTTPS.

```sh
./build-redirect/frontend/flashvm_player --async-loads --redirects 3 --http-ca-file /path/to/ca.pem --loads 4 --frames 700 --fps 1000 --mute https://example.test/start.swf
```

C/IDE callers set policy.ca_file directly. The tutorial alone reads FLASHVM_CA_FILE
before runner entry; the SDK does not read that custom environment variable or
install global trust state. Keep the path and provider SDK alive/immutable through
joined return. Native and libcurl tests use the explicit file instead of relying
on ambient system CA variables. They cover full redirected HTTP/HTTPS playback,
HTTPS without redirects, missing trust, C/memory origins and CLI argument failures.
A curl SDK without HTTPS explicitly skips TLS-only tests; it is not certified for TLS.

## Validation of this series

The selected native run has 99 checks: 98 pass directly, and the loading example
passes after restoring an executable permission lost on its build artifact.
Four redirect HTTP/TLS/player tests pass again on the final sources. Shared
native-TLS passes 24 checks; shared libcurl passes 20 plus the final four redirect
reruns. Three provider-less ABI/metadata/job checks also pass. C-only UBSan covers player/transfer, native HTTP framing, URI/redirect and
clock modules with actual HTTP/TLS, interpreter/JIT, C/memory playback fixtures.
The VM, SDL, native socket/TLS primitives and OpenSSL remain ordinary dependencies
in that sanitizer probe; no whole-process leak/ThreadSanitizer claim is made.

Player and the new C example cross-link for Linux ARM64/RISC-V64 and Windows
x86_64. Windows DLL and C/C++98 ABI/source-policy consumers link. Foreign binaries
are not executed. Tutorial comments/API tables, source links and sorted catalog
pass. These are bounded transport/player results; full Flash semantics and all
nine roadmap stages, including stage9 native platform/device acceptance, stay open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_NET_MAX_MS` | macro / define | [ports/network/socket.h:8](../ports/network/socket.h#L8) |
| `FVM_NetControl` | struct | [ports/network/socket.h:21](../ports/network/socket.h#L21) |
<!-- source-links:end -->
