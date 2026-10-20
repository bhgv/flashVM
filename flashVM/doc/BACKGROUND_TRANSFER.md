# Controlled and background transfers

[Русский](BACKGROUND_TRANSFER.ru.md) · [Transport](SOCKET_TRANSPORT.md) · [Sources](PLAYER_SOURCES.md).

## Cancellation since v0.680

Original options/loader structures keep their ABI. [FVM_NetControl](../ports/network/socket.h#L21) in
[socket.h](../ports/network/socket.h) contains cancelled and borrowed userdata.
A null callback disables cancellation; nonzero requests it. Keep callback/state
alive until return. It runs on the transfer thread: it must be short, thread-safe,
nonthrowing, and must not enter VM/SDL UI or destroy provider state.

FVM_SocketHTTPRequest in [socket_http.h](../frontend/common/socket_http.h) wraps
unchanged socket options and control. [fvm_desktop_socket_request_init](../frontend/common/socket_http.h#L55) initializes
all fields without startup (1 success, 0 null).
[fvm_desktop_socket_fetch_controlled](../frontend/common/socket_http.h#L59) uses the ordinary fetch signature; userdata
is null for defaults or points to this request. [fvm_desktop_http_fetch_controlled](../frontend/common/http.h#L29)
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
[FVM_DesktopTransfer](../frontend/common/transfer.h#L37) and caller-owned [FVM_DesktopTransferOptions](../frontend/common/transfer.h#L30)/View.
[fvm_desktop_transfer_options_init](../frontend/common/transfer.h#L50) sets max_bytes=64MiB and no provider (1 success,
0 null). Supply a worker-safe [FVM_DesktopTransferFetch](../frontend/common/transfer.h#L24) callback and optional
immutable userdata. Its signature extends the usual fetch with an explicit
borrowed cancellation control. No VM object, managed string or UI object may
cross this worker boundary. Native bytes are imported only by the owner later.

[fvm_desktop_transfer_begin](../frontend/common/transfer.h#L54)(reference,base,options,out) copies bounded text and
options, reserves one of 64 live handles in this SDK and starts an SDL worker.
Userdata and nested options/CA filenames remain borrowed through release.
max_bytes must be 1..64MiB. Invalid input returns INVALID_ARGUMENT, quota ERROR,
allocation/thread failure OUT_OF_MEMORY; *out is unchanged on failure. This does
not initialize video/audio or perform provider/network work on the calling thread.

[fvm_desktop_transfer_poll](../frontend/common/transfer.h#L66) copies a mutex-protected view (OK, or INVALID_ARGUMENT
for null). PENDING has result=NOT_FOUND and exposes no mutable bytes/provenance.
DONE exposes immutable counted bytes (possibly empty) and nonempty provenance,
borrowed until release. FAILED exposes the exact first sink/provider error and
no payload. CANCELLED returns ERROR and no payload. received retains accumulated
progress even when a failed/cancelled buffer has been discarded. Providers that
ignore a sink error cannot publish success. Missing/unterminated provenance fails.

[fvm_desktop_transfer_cancel](../frontend/common/transfer.h#L75) returns 1 for a pending request and 0 for terminal/null;
it requests a stop without blocking, freeing bytes or revoking a completed view.
Cancellation before terminal publication wins over a late provider success.
[fvm_desktop_transfer_release](../frontend/common/transfer.h#L79) cancels, joins and frees the handle on its creating
thread: OK, null also OK, wrong thread INVALID_ARGUMENT. It may block for an
uncooperative provider/DNS; no detach/forced kill is used. Never release concurrently
with poll/cancel or unload a library while its worker is alive. All argument and
callback storage remains live until release returns.

[fvm_desktop_transfer_active](../frontend/common/transfer.h#L81) reports live handles, including terminal handles not
yet released. [fvm_desktop_shutdown](../frontend/common/player.h#L50) refuses cleanup while any remain, even when
SDL video/audio were never initialized. The caller must release every job before
ending the matching SDK instance. No hidden global provider registration is used.

[fvm_desktop_socket_transfer_fetch](../frontend/common/socket_http.h#L64) adapts native options (null defaults) to the job
signature. [fvm_desktop_http_transfer_fetch](../frontend/common/http.h#L34) requires null userdata and a libcurl
SDK reporting CURL_VERSION_THREADSAFE; older global init/cleanup implementations
return UNSUPPORTED instead of being called from an arbitrary worker. The original
synchronous libcurl API retains its older SDK support and coordination rules.

Deterministic worker fixtures check copied input, owner/worker separation, binary
NUL, empty bodies, pending views, late cancellation, provider/sink errors, ignored
sink failure, bad provenance, wrong-thread release, 64-handle quota and shutdown
before/after joined cleanup. Network cancellation tests remain separate. These
are bounded concurrency checks, not a ThreadSanitizer/leak or foreign OS claim.
## Player integration since v0.682

Enable the mode explicitly with `--background-loads`, or call the additive C API
in [background.h](../frontend/common/background.h). Original synchronous APIs and
options/source/loader layouts remain unchanged; existing custom callbacks are
never silently moved onto another thread. The CLI adapter translates this flag
into [fvm_desktop_run_background](../frontend/common/background.h#L34); the original scalar-options parser has no field
for it. An IDE calls the explicit background runner or the lower-level job API.

```sh
./build-socket/frontend/flashvm_player --background-loads --loads 8 --frames 3 --mute http://127.0.0.1:8000/start.swf
```

[fvm_desktop_background_loader_init](../frontend/common/background.h#L21) initializes [FVM_DesktopBackgroundLoader](../frontend/common/load_monitor.h#L59):
transfer defaults/no provider, one resource load, 1 success or 0 null, no startup.
[fvm_desktop_run_source_background](../frontend/common/background.h#L28) takes ordinary options/source, a required
background loader and optional static error output. Provide a worker-safe fetch,
max_bytes=1..64MiB and max_loads=1..1024. The loader, options/URL/memory input and
nested immutable provider userdata remain borrowed through synchronous return.
Status rules stay 0 success/close, 1 playback/provider error, 2 invalid input.
A null/malformed policy fails before starting a worker or VM/window. Failed
partial bytes are never imported. Native import/setters/GC remain on the owner.

[fvm_desktop_run_background](../frontend/common/background.h#L34) is the playlist adapter using the compiled native or
thread-safe libcurl provider; without one, local playback uses the ordinary
runner and URL input remains unsupported. Duplicate CLI flags are rejected;
after `--` the same text is a literal filename. Help/version still need no
network, file, VM or display startup. This is opt-in, not the default network mode.

During a child/form network wait the owner processes bounded native window events,
repaints cached frames on resize/expose, retains input snapshots and coordinates
foreground/background audio. It does not execute script/timers or import bytes
until the worker completes. The timeline remains paused at that safe point:
this is responsive native UI with background provider execution, not fully
asynchronous Flash resource/timeline semantics. Native input queues retain their
existing bounds and overflow/error rules. No progressive SWF decoding is added.

Closing the active player view cancels, joins, discards that partial resource,
releases all queued GC pins and exits the playlist with status 0. Native/provider
errors still return playback failure. Worker code never sees a VM object/window
pointer. Startup and whole-root transitions have no active player view; they
fetch on workers but offer no built-in progress window or startup close control.
Filesystem reads remain on the owner. DNS/noncooperative code may delay a final
join, and the synchronous runner does not return before its workers stop. For
an independently responsive startup UI use begin/poll/cancel/release directly
from the host event loop, retaining all provider storage until release.

[Annotated C89 example](../examples/player/background_loading/README.md) selects
the provider explicitly. Tests inject a real cross-thread SDL close after the
first child body bytes, verify prompt cancellation before the server finishes,
no second queued fetch, successful cleanup and no live job. Separate tests run
normal/background HTTP and HTTPS startup/child/form movies in interpreter/JIT,
plus the executable C example. They do not certify native windows on foreign OS.

Final evidence for this series: 86 selected native tests, 11 shared native-TLS
checks, 9 shared libcurl checks and 3 no-provider metadata/job checks. The transfer
implementation passes UBSan linked to the real SDK/shutdown fixture; SDL/VM/crypto
libraries were not instrumented by that probe. Linux ARM64/RISC-V64 and Windows
x86_64 player/example cross-builds link; Windows DLL C/C++98 consumers and transfer
fixtures link, without target execution. Complete asynchronous loading, DNS
interruption, foreign runtime/device and stage 9 release acceptance remain open.


## Asynchronous child and form delivery (v0.683)

Use `--async-loads --loads 8` or [fvm_desktop_run_source_async](../frontend/common/background.h#L40) /
[fvm_desktop_run_async](../frontend/common/background.h#L45) to keep frames, timers, input and GC running while a
network child SWF or loadVariables response is pending. The same
`FVM_DesktopBackgroundLoader` supplies an explicit worker-safe provider and
budgets. Existing options/source/loader layouts and synchronous APIs are unchanged.
Only one network request is in flight; later requests retain FIFO order and their
source bases. The pending target stays explicitly GC-pinned across frames.

The worker publishes immutable bytes only after complete successful fetch.
At an owner-thread safe point the player imports the child or applies form
variables/onData, then releases the job and target pin. No worker calls VM,
window or audio objects. Pending completion is polled at intervals capped at
5ms even in a low-frame-rate movie; this is a scheduling bound, not a realtime
guarantee. Closing, reaching the frame limit or an error cancels and joins the
job before releasing pins/context; incomplete bytes are never imported.

`--background-loads` retains its v0.682 waiting-at-safe-point behavior. The two
mode flags are mutually exclusive; after `--` either is a literal filename.
Startup and whole-root replacement retain the prior background lifecycle without
an active player view. Local file reads stay synchronous on the owner. There is
no progressive decoding or complete MovieClipLoader event protocol; network/import
errors still fail playback. DNS/noncooperative providers may delay final join.

[fvm_desktop_background_supported](../frontend/common/background.h#L49)() and `--version`'s
`background_provider=yes/no` query the compiled default provider without startup.
A custom callback has its own contract. Unsafe old libcurl global initialization
is rejected for workers; unavailable-provider tests explicitly skip with status77.
See the [standalone annotated C/AS example](../examples/player/async_loading/README.md)
and [cross-platform build guide](SOCKET_TRANSPORT.md). The tutorial generator is
standard-library Python and does not depend on unfinished compiler backends.

The v0.683 native selection passes 91 tests, including HTTP/HTTPS interpreter/JIT,
real close/cancel, short frame limits, timer/frame progress and a 1Hz onData
completion deadline. Waiting-mode and wrong-data negative controls must fail.
Linux ARM64/RISC-V64 and Windows x86_64 player/example builds and Windows DLL
consumers cross-link; foreign binaries are not executed. These bounded results
do not close stage9 or all nine roadmap stages.

Additional current validation: 12 shared-libcurl and 16 shared native-TLS checks
pass. Player/transfer C implementations pass UBSan on the real async HTTP fixture;
SDL, core VM and crypto dependencies were not instrumented by that probe.

## Cooperative total job deadlines (v0.684)

[fvm_desktop_transfer_begin_timed](../frontend/common/transfer.h#L62) adds a total elapsed-millisecond budget without
changing existing options/view layouts or the unlimited begin API. Zero disables
the budget; the maximum is [FVM_DESKTOP_TRANSFER_MAX_TIMEOUT_MS](../frontend/common/transfer.h#L13). Invalid values
preserve *out and start no worker. Unsigned SDL tick subtraction tolerates rollover.
The budget includes provider work from launch, not just each socket operation.

Expiry requests cooperative cancellation; it is checked by polling, sink writes,
provider cancellation queries and final publication. A late successful provider
cannot publish bytes. The terminal [FVM_DESKTOP_TRANSFER_TIMED_OUT](../frontend/common/transfer.h#L12) view has [FVM_ERROR](../include/flashvm/flashvm.h#L43),
no payload/provenance and retained received progress. First latched explicit
cancel/deadline wins; completed views remain immutable. A pending view may persist
past the budget while DNS/noncooperative code runs. Release always joins, so this
is not hard preemption and never authorizes unloading borrowed provider storage.

Native worker tests cover partial received bytes, a late ignored-success provider,
explicit cancellation winning first, invalid timeout and all previous job cases.
This does not change default player or provider-specific I/O deadlines.

[Owner monitoring and startup stop](LOAD_MONITOR.md).

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
