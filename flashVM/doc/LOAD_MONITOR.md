# Owner-thread load monitoring (v0.686)

[Русский](LOAD_MONITOR.ru.md) · [Background jobs](BACKGROUND_TRANSFER.md) ·
[Annotated example](../examples/player/monitored_loading/README.md).

[fvm_desktop_load_monitor_init](../frontend/common/load_monitor.h#L49) initializes [FVM_DesktopLoadMonitor](../frontend/common/load_monitor.h#L39): async child/form
delivery, no deadline/callback. [fvm_desktop_run_source_monitored](../frontend/common/load_monitor.h#L57) takes existing
options/source/background loader plus this new policy; old structure layouts and
runners remain unchanged. asynchronous must be 0 or 1; timeout_ms is 0 (unlimited) or
at most [FVM_DESKTOP_TRANSFER_MAX_TIMEOUT_MS](../frontend/common/transfer.h#L13). Validation precedes notice/worker/device
startup. Scalar/status rules remain 0 success/stop, 1 playback error, 2 invalid input.

Each network request gets a source-run ID, kind ROOT/CHILD/VARIABLES and exactly
BEGIN, zero or more WAIT heartbeats, then FETCHED/FAILED/CANCELLED/TIMED_OUT.
Local reads and requests discarded while still queued are not observed. ROOT
includes startup and whole-root replacement; an embedded movie itself needs no
network notice, but its network descendants do. FIFO and load budgets are shared.

[FVM_DesktopLoadEvent](../frontend/common/load_monitor.h#L21) is borrowed for the callback only. reference/base identify
the copied request. location is nonnull only for FETCHED. received is provider
body bytes, not total/wire size. BEGIN/WAIT result is NOT_FOUND. FETCHED means
successful complete fetch/sink validation, before VM import; import/script errors
can still fail playback with no second transport notice. There are no VM pointers.
These host notices are not Flash MovieClipLoader or script lifecycle events.

[FVM_DesktopLoadNotify](../frontend/common/load_monitor.h#L37) runs on the synchronous runner's owner thread outside VM
callbacks, even during startup before an SDL view exists. It may update a host-owned
progress UI. It must not enter the VM, recurse a runner, destroy borrowed provider/
SDK state, throw through C or steal the active player's SDL events. WAIT cadence
is bounded by owner-loop work and is not realtime. Returning nonzero at BEGIN/WAIT
stops the entire source successfully, discards partial bytes and joins before
runner return. BEGIN stop avoids provider launch. Terminal return values are ignored.
A cancellation notice marks abandonment; final join may still be pending during
that callback. Event strings/job storage never transfer ownership to the observer.

The per-job deadline includes startup, replacement and child/form network work,
using cooperative timed jobs. Timeout/provider failures remain strict playback
errors. Close/frame bound/error abandons active requests and releases all target
pins only after joining. DNS/noncooperative providers can still delay return.
Old background/async APIs have no new deadline or callback implicitly installed.

Tests exercise real HTTP and verified HTTPS, owner/worker separation, startup and
child stop before/after body bytes, timeout-specific notices,404, short frame bound,
waiting/async success, ignored terminal stop returns and malformed policy before
startup. The C++98 consumer links the new API; the example uses real standalone
movies. Full Flash loader semantics and foreign runtime/device acceptance remain open.

## Playlist and console adapters (v0.686)

[fvm_desktop_run_monitored](../frontend/common/load_monitor.h#L64)(options, monitor, error)` selects the compiled default
worker provider for a playlist. IDs restart for each source; a host stop ends the
whole playlist. Without a compiled network provider local files still play normally;
URL input is unsupported. The per-source API keeps custom provider injection explicit.

```sh
./build-monitor/frontend/flashvm_player --async-loads --loads 8 --load-timeout 5000 --load-progress --frames 700 --fps 1000 --mute http://127.0.0.1:8000/start.swf
```

`--load-timeout MS` accepts strict decimal integer 0..2147483647, zero disables the
additional total job budget. Signs, spaces, fractions, exponents, overflow, duplicates
and missing values are argument errors before VM/display/I/O. `--load-progress`
logs coalesced metadata on stderr; both flags require `--async-loads` or
`--background-loads`. After `--`, their spelling is a literal filename. Existing
scalar option layouts are unchanged: CLI translates these flags into the C monitor.
No timeout is installed implicitly by old APIs. Provider I/O timeouts remain separate.

Progress lines contain id/kind/phase/received/result; numeric phases 1..6 correspond
to BEGIN, WAIT, FETCHED, FAILED, CANCELLED, TIMED_OUT. Kinds 1..3 are ROOT, CHILD,
VARIABLES. Result is the signed [FVM_Result](../include/flashvm/flashvm.h#L42) (NOT_FOUND during BEGIN/WAIT). Repeated
WAIT with unchanged body count is suppressed. Structured lines contain no URL or
response body; ordinary player diagnostics can still name a movie/source URL.
Help/version work without devices, including with these valid monitor options.
The console has no independent network or scheduler implementation.

The owner checks [fvm_desktop_transfer_stop_requested](../frontend/common/transfer.h#L72) even while the native job
is PENDING. At a deadline it stops further VM turns and performs the final join,
so a slow provider cancellation acknowledgement cannot extend script execution.
The public job view stays PENDING until worker exit; no mutable payload leaks.
TIMED_OUT notification can precede join just as CANCELLED can. The getter returns
zero for no stop/null, otherwise CANCELLED or TIMED_OUT, with the first reason latched.
A semaphore fixture proves the stop is visible while worker publication remains pending.

## Validation of this series

95 selected native checks pass, followed by 17 focused checks after the stop-latch
correction. Shared hosts pass 20 native-TLS checks, 16 libcurl checks and 3 no-provider
checks. Player/transfer C code passes UBSan with the real monitored HTTP, CLI and
job fixtures; SDL/core VM/crypto dependencies are not instrumented. Windows DLL
and C/C++98 consumers/monitor fixtures link. Players and the annotated monitor
example cross-link for Linux ARM64/RISC-V64 and Windows x86_64, without foreign
execution. Source links, tutorial comments/API tables and sorted catalog pass.
Full Flash conformance, physical device/foreign runtime and all roadmap stages
remain open; these results do not certify stage9 release acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->
