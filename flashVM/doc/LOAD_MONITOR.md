# Owner-thread load monitoring (v0.685)

[Русский](LOAD_MONITOR.ru.md) · [Background jobs](BACKGROUND_TRANSFER.md) ·
[Annotated example](../examples/player/monitored_loading/README.md).

[fvm_desktop_load_monitor_init](../frontend/common/load_monitor.h#L47) initializes [FVM_DesktopLoadMonitor](../frontend/common/load_monitor.h#L37): async child/form
delivery, no deadline/callback. [fvm_desktop_run_source_monitored](../frontend/common/load_monitor.h#L55) takes existing
options/source/background loader plus this new policy; old structure layouts and
runners remain unchanged. asynchronous must be0 or1; timeout_ms is0 (unlimited) or
at most [FVM_DESKTOP_TRANSFER_MAX_TIMEOUT_MS](../frontend/common/transfer.h#L13). Validation precedes notice/worker/device
startup. Scalar/status rules remain0 success/stop,1 playback error,2 invalid input.

Each network request gets a source-run ID, kind ROOT/CHILD/VARIABLES and exactly
BEGIN, zero or more WAIT heartbeats, then FETCHED/FAILED/CANCELLED/TIMED_OUT.
Local reads and requests discarded while still queued are not observed. ROOT
includes startup and whole-root replacement; an embedded movie itself needs no
network notice, but its network descendants do. FIFO and load budgets are shared.

[FVM_DesktopLoadEvent](../frontend/common/load_monitor.h#L20) is borrowed for the callback only. reference/base identify
the copied request. location is nonnull only for FETCHED. received is provider
body bytes, not total/wire size. BEGIN/WAIT result is NOT_FOUND. FETCHED means
successful complete fetch/sink validation, before VM import; import/script errors
can still fail playback with no second transport notice. There are no VM pointers.
These host notices are not Flash MovieClipLoader or script lifecycle events.

[FVM_DesktopLoadNotify](../frontend/common/load_monitor.h#L35) runs on the synchronous runner's owner thread outside VM
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
