# Controlled and background transfers

[Русский](BACKGROUND_TRANSFER.ru.md) · [Transport](SOCKET_TRANSPORT.md) · [Sources](PLAYER_SOURCES.md).

## Cancellation since v0.680

Original options/loader structures keep their ABI. [FVM_NetControl](../ports/network/socket.h#L21) in
[socket.h](../ports/network/socket.h) contains cancelled and borrowed userdata.
A null callback disables cancellation; nonzero requests it. Keep callback/state
alive until return. It runs on the transfer thread: it must be short, thread-safe,
nonthrowing, and must not enter VM/SDL UI or destroy provider state.

FVM_SocketHTTPRequest in [socket_http.h](../frontend/common/socket_http.h) wraps
unchanged socket options and control. fvm_desktop_socket_request_init initializes
all fields without startup (1 success, 0 null).
fvm_desktop_socket_fetch_controlled uses the ordinary fetch signature; userdata
is null for defaults or points to this request. fvm_desktop_http_fetch_controlled
in [http.h](../frontend/common/http.h) instead accepts null or [FVM_NetControl](../ports/network/socket.h#L21).
The original libcurl callback still rejects nonnull userdata.

Cancellation returns [FVM_ERROR](../include/flashvm/flashvm.h#L43) without publishing resolved. Sink errors retain
their exact result; discard partial bytes on every failure. Native waits check
cancellation in 50ms slices under the original total deadline. Buffered HTTP
and TLS I/O also check it. Libcurl uses its own progress/sink cadence and can be
slower. DNS, CA reads, crypto work and user callbacks cannot be forcibly interrupted.
Cancellation during DNS is observed after resolver return; this is not a hard
realtime completion guarantee. No thread is killed or library unloaded during
active execution. Cleanup balances every owned socket/Winsock/TLS reference.

Loopback regressions cancel before connect, a silent response, byte-drip HTTP and
TLS bodies. A separate libcurl probe exercises its real progress callback. These
are protocol tests, not complete Flash or native foreign OS/device acceptance.
## Background C89 jobs since v0.681

[transfer.h](../frontend/common/transfer.h) provides an opaque
FVM_DesktopTransfer and caller-owned FVM_DesktopTransferOptions/View.
fvm_desktop_transfer_options_init sets max_bytes=64MiB and no provider (1 success,
0 null). Supply a worker-safe FVM_DesktopTransferFetch callback and optional
immutable userdata. Its signature extends the usual fetch with an explicit
borrowed cancellation control. No VM object, managed string or UI object may
cross this worker boundary. Native bytes are imported only by the owner later.

fvm_desktop_transfer_begin(reference,base,options,out) copies bounded text and
options, reserves one of 64 live handles in this SDK and starts an SDL worker.
Userdata and nested options/CA filenames remain borrowed through release.
max_bytes must be 1..64MiB. Invalid input returns INVALID_ARGUMENT, quota ERROR,
allocation/thread failure OUT_OF_MEMORY; *out is unchanged on failure. This does
not initialize video/audio or perform provider/network work on the calling thread.

fvm_desktop_transfer_poll copies a mutex-protected view (OK, or INVALID_ARGUMENT
for null). PENDING has result=NOT_FOUND and exposes no mutable bytes/provenance.
DONE exposes immutable counted bytes (possibly empty) and nonempty provenance,
borrowed until release. FAILED exposes the exact first sink/provider error and
no payload. CANCELLED returns ERROR and no payload. received retains accumulated
progress even when a failed/cancelled buffer has been discarded. Providers that
ignore a sink error cannot publish success. Missing/unterminated provenance fails.

fvm_desktop_transfer_cancel returns 1 for a pending request and 0 for terminal/null;
it requests a stop without blocking, freeing bytes or revoking a completed view.
Cancellation before terminal publication wins over a late provider success.
fvm_desktop_transfer_release cancels, joins and frees the handle on its creating
thread: OK, null also OK, wrong thread INVALID_ARGUMENT. It may block for an
uncooperative provider/DNS; no detach/forced kill is used. Never release concurrently
with poll/cancel or unload a library while its worker is alive. All argument and
callback storage remains live until release returns.

fvm_desktop_transfer_active reports live handles, including terminal handles not
yet released. fvm_desktop_shutdown refuses cleanup while any remain, even when
SDL video/audio were never initialized. The caller must release every job before
ending the matching SDK instance. No hidden global provider registration is used.

fvm_desktop_socket_transfer_fetch adapts native options (null defaults) to the job
signature. fvm_desktop_http_transfer_fetch requires null userdata and a libcurl
SDK reporting CURL_VERSION_THREADSAFE; older global init/cleanup implementations
return UNSUPPORTED instead of being called from an arbitrary worker. The original
synchronous libcurl API retains its older SDK support and coordination rules.

Deterministic worker fixtures check copied input, owner/worker separation, binary
NUL, empty bodies, pending views, late cancellation, provider/sink errors, ignored
sink failure, bad provenance, wrong-thread release, 64-handle quota and shutdown
before/after joined cleanup. Network cancellation tests remain separate. These
are bounded concurrency checks, not a ThreadSanitizer/leak or foreign OS claim.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_NetControl` | struct | [ports/network/socket.h:21](../ports/network/socket.h#L21) |
<!-- source-links:end -->
