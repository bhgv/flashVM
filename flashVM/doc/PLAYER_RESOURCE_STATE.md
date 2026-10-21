# Copied player state over resource channels

The additive C89 `frontend/common/resource_state.h` API observes the existing
playback safe points. It exposes a READ snapshot operation and independent
SUBSCRIBE queues under a host-chosen path, for example `/timeline/state`.
[Playback control](PLAYBACK_CONTROL.md), [resource files](RESOURCE_FILES.md),
[9P2000](RESOURCE_9P.md), [example](../examples/player/frame_capture/README.md),
[RU](PLAYER_RESOURCE_STATE.ru.md).

## Snapshot and event contract

FVM_DesktopResourceSnapshot owns its source byte prefix and scalar fields. It
contains no VM/context/root/value pointer. It remains usable after runner return
and SDK teardown. Epoch starts at one at the first READY, increases for each new
root READY (including whole-root navigation), never wraps, and is scoped to the
adapter lifetime. TURN or TICK before READY returns BUSY. Source is copied up to
FVM_DESKTOP_STATE_SOURCE bytes with native NUL termination and explicit
SOURCE_TRUNCATED. Truncation is bytewise, not a promise of complete UTF-8 code
points. Counters greater than Uint32 saturate with COUNTERS_TRUNCATED; a value
exactly at Uint32 max is exact. The adapter reports received native counter
values; it cannot reconstruct overflow that already occurred in the host.

READY and TICK publish each observation. A TURN publishes POLICY only when
paused or step_pending changed since the last observation. Quiet turns still
refresh the READ cache but do not consume event sequence numbers or queue slots.
The cache is committed only after any required publication succeeds. Exhausted
epoch/global publication sequence returns LIMIT and leaves the cache unchanged.
No rollback of timeline changes or previously presented pixels is attempted.
Independent observer overflow remains visible in each event's missed counter.
Read a new snapshot or reopen a stream after loss as appropriate; this is not a
persistent replay log or reliable delivery acknowledgement.

A TICK observation means the existing runner has completed its timeline advance
and presentation boundary. Command reply OK means earlier owner delivery, not a
prediction of this later snapshot. A queued STEP can still await an eligible
foreground turn. PAUSE does not freeze timer/input/load services or all mutable
VM state. The snapshot reports policy and host tick counters, not an atomic
snapshot of every scene property, script playhead or rendered pixel buffer.

The payload is ten canonical nonnegative S integers in order:
version, epoch, phase, paused, step_pending, movie_ticks, total_ticks,
presentations, flags, source_size; then exactly source_size bytes. It is carried
inside the existing checked resource reply/event envelopes. This is not a new
native-struct wire ABI. Event kinds are READY, TICK and POLICY; phase values are
the existing playback notification phases. Elapsed floating-point time is not
included; no native double representation or clock-domain assumption is sent.

## API and lifetime

All adapter mutations and resource handlers execute on the playback owner.
Workers may post/take copied dispatcher requests/events with working dispatcher
mutex hooks, or decode copied payloads; they never call observe or access a live
player state. Serialize adapter create/observe/destroy with the owner and avoid
reentry. Input/output objects must not overlap each other or opaque storage.
The resource handler only reads the owner cache and never queries the VM.

| Function | Contract |
| --- | --- |
| `fvm_desktop_resource_state_create` | Dispatcher, canonical path, adapter output and target output. Allocates one fixed native object, registers one READ snapshot operation and borrows dispatcher until destroy. Outputs change only on OK. Invalid path/duplicate/registration limit propagate; allocation NOMEM. No SDK startup, grant or notify installation. Keep output storage separate and stable. |
| `fvm_desktop_resource_state_observe` | Adapter and borrowed legitimate playback notification. Copies scalars/source synchronously and never retains context/root. Requires exact descriptor size, non-NULL source, defined phase and 0/1 pause/step. READY increments epoch; TURN/TICK before READY BUSY. Publish READY/TICK or changed TURN policy, then commit cache. Errors leave cache intact; publication failure propagates. No allocation, pump, I/O, console or player policy change. |
| `fvm_desktop_resource_state_read` | Counted payload and native snapshot output. Decode complete versioned canonical integers and exact source extent. Unknown schema/flags, malformed integers, bad booleans/phase, embedded NUL, invalid truncation extent or trailing bytes INVALID and preserve output. No VM calls, borrowed result or allocation; suitable for a client with its own copied bytes. |
| `fvm_desktop_resource_state_destroy` | Pointer to adapter after notification/consumer users quiesce. Owner unregisters the exact generation, discards watches and frees only adapter storage, nulling pointer. BUSY during its resource callback; owner/registration errors preserve pointer. Queued old jobs later complete UNKNOWN and must still be taken/disconnected by the host. Dispatcher/session/SDK remain owned by host. |

The SNAPSHOT operation requires READ and an empty request. Before the first
successful observation it returns terminal BUSY; nonempty payload INVALID.
The host grants READ and/or SUBSCRIBE explicitly and decides whether the source
prefix is suitable for that session. SUBSCRIBE alone permits copied events.
No session ID is accepted from a wire claim and no ambient filesystem is opened.

Call observe before fvm_desktop_resource_control_notify in the same host notify.
That copies the state before returning the next command. Do not pump control
operations outside their control notify: they retain the existing BUSY rule.
Observation itself never pumps, so the control adapter's one-job-per-boundary
FIFO behavior is preserved. If the host returns STOP after an observation error,
normal player cleanup still joins outstanding load jobs and releases VM roots.

Close file/9P clients first, quiesce notifications, destroy state adapter,
unregister control resources, acknowledge/disconnect outstanding requests, then
destroy dispatcher. The adapter does not retain or extend runner/SDK lifetimes.
[GC:] Existing runner pins protect the root during notify; the new adapter adds
no managed references, roots, callbacks or collection work.

## Example and build

The existing frame_capture example's `--resource-control` mode now publishes and
locally decodes independent state events before queuing STOP. It checks decode
and loss errors, owns native snapshots after SDK teardown, and releases the
adapter before the dispatcher. Default capture mode creates no new resource.

```sh
cmake -S flashVM -B build-state -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_TESTS=ON
cmake --build build-state --target flashvm_frontend_resource_state flashvm_example_frame_capture flashvm_frontend_host_abi
ctest --test-dir build-state -R '^(flashvm_resource_state_contract|flashvm_resource_capture_example|flashvm_frontend_host_abi)$' --output-on-failure
```

Use the documented static SDL SDK/provider paths and build definitions from
[host integration](DESKTOP_FRONTEND.md) and [platform validation](PLATFORM_VALIDATION.md).
The standalone contract fixture needs no SDL/device startup and checks independent
observers, quiet-turn suppression, strict prefix decoding, root epochs,
source/counter saturation, allocation failure, invalid notifications, epoch
exhaustion and queued stale teardown. Fixture-created notifications test the
codec/adapter contract; actual playback evidence comes from the runnable player
example and live-player tests. No OS mount or foreign-platform execution is
implied. All nine roadmap stages remain open.
