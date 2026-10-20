# Thread-safe player command mailbox

[Русский](PLAYBACK_MAILBOX.ru.md) · [Owner commands](PLAYBACK_CONTROL.md) ·
[SDK ownership](PLAYER_HOST_OWNERSHIP.md) · [Roadmap](ROADMAP_STATUS.md).

## Native foundation since v0.713

[FVM_DesktopPlaybackMailbox](../frontend/common/playback_mailbox.h#L12) owns one mutex, latest intent and scalar/text copies.
It retains no context/root, managed values, SDL window or caller callback. It is
for an IDE controller thread; only the player owner enters VM. v0.713 introduces
the foundation and native tests; runner adapters are documented as they are added.

This is a latest-intent mailbox, not an instruction queue. PAUSE, RESUME and STEP
replace a pending nonterminal command. KEEP changes nothing. Multiple STEP posts
before receipt coalesce; an accepted STEP permits one future tick under the
ordinary control contract. STOP supersedes every intent and remains terminal.
To guarantee N steps, observe completion before posting each subsequent step;
a burst of N post(STEP) calls does not promise N advances.

## API and errors

[fvm_desktop_playback_snapshot_init](../frontend/common/playback_mailbox.h#L32)(out)` fills exact struct_size and zero fields;
returns 1 for nonnull out, 0 for null. No allocation or SDK startup.
[FVM_DesktopPlaybackSnapshot](../frontend/common/playback_mailbox.h#L14) is an independent value including NUL-terminated
source. It survives runner/mailbox release and contains no hidden VM pointers.
paused/step_pending describe the last owner observation before applying its
returned command; pending/stop_requested are sampled during poll. Posting is not
immediate acknowledgement of application or a frozen VM snapshot.

[fvm_desktop_playback_mailbox_create](../frontend/common/playback_mailbox.h#L38)(out)` requires an output-pointer address and
returns OK, INVALID_ARGUMENT, OUT_OF_MEMORY, or ERROR for native mutex/quota failure.
*out is unchanged on failure. It allocates native memory/mutex only, without a VM,
window, audio device, network request or worker. At most
[FVM_DESKTOP_PLAYBACK_MAILBOX_MAX_HANDLES](../frontend/common/playback_mailbox.h#L5) may coexist. Every failed allocation
rolls back reserved quota. Use one mailbox per source run.

[fvm_desktop_playback_mailbox_post](../frontend/common/playback_mailbox.h#L47)(mailbox, command)` atomically replaces pending
intent. Returns OK, INVALID_ARGUMENT for null/unknown command, or ERROR for mutex
failure and any non-STOP command after STOP. Repeated STOP succeeds. No allocation,
VM entry or user callback occurs under its mutex.

[fvm_desktop_playback_mailbox_poll](../frontend/common/playback_mailbox.h#L52)(mailbox, out)` requires exact struct_size.
Returns OK with a consistent copy; NOT_FOUND before the first owner observation;
INVALID_ARGUMENT for null/size mismatch; ERROR on lock failure. All output bytes,
including padding, stay unchanged on failure. It waits only for a short native
mutex, never a frame or network request, and never enters VM. Scalar counts keep
the per-root/per-run meanings from PLAYBACK_CONTROL.md.

[fvm_desktop_playback_mailbox_stop_requested](../frontend/common/playback_mailbox.h#L57)(mailbox)` reads terminal cancellation
without consuming intent. Returns 1 for STOP, null or mutex failure, otherwise 0.
It works before the first snapshot, including during startup fetching.

[fvm_desktop_playback_mailbox_active](../frontend/common/playback_mailbox.h#L69)()` returns the live native-handle count without
SDK startup. [fvm_desktop_shutdown](../frontend/common/player.h#L53)()` returns ERROR while any mailbox remains, even
without a window/transfer, leaving the active SDK untouched.

[fvm_desktop_playback_mailbox_release](../frontend/common/playback_mailbox.h#L42)(mailbox)` returns OK; null is accepted.
The runner and every post/poll/other caller thread must finish first. It does not
join external threads. Concurrent use/release, double release and racing
create/release against process-wide shutdown are forbidden. [GC:] No managed root
exists here; mailbox lifetime never extends VM lifetime.

## Verification and limits

`frontend/tests/playback_mailbox.c` checks native mutexes without SDL subsystem
startup, consistent copies with four actual OS threads, terminal precedence,
coalescing, source bounds, untouched outputs, quota and shutdown protection.
Separate `playback_mailbox_allocation.c` injects native memory/mutex failures and
checks quota rollback. This is bounded protocol testing, not endurance/real GUI
acceptance. Foreign OS/device execution and all nine roadmap stages remain open.

## Runner adapters since v0.714

[fvm_desktop_playback_mailbox_notify](../frontend/common/playback_mailbox.h#L62)(state, userdata)` is an owner callback for
control.notify, with userdata equal to the mailbox. It publishes the copied
observation and returns pending intent without storing or dereferencing the
borrowed context/root pointers. Invalid snapshot/handle/source fails closed with
STOP. Normal callbacks return KEEP when no command is waiting. Owner publication
consumes nonterminal intent; STOP remains latched for subsequent observations.

[fvm_desktop_playback_mailbox_load_notify](../frontend/common/playback_mailbox.h#L66)(event, userdata)` is the owner-only load
monitor callback. It returns the same nonconsuming terminal latch, including
before READY and during blocking safe-point fetch waits; null event/userdata or
lock failure stops. PAUSE/RESUME/STEP wait for playback notification and do not
pause provider work. Load-monitor terminal return values retain their ordinary
ignored-return convention. Use an explicit background provider and bind both
callbacks to stop startup I/O, rather than expecting a playback READY callback
before import.

```c
fvm_desktop_playback_control_init(&control);
control.initially_paused = 1;
control.notify = fvm_desktop_playback_mailbox_notify;
control.userdata = mailbox;
fvm_desktop_load_monitor_init(&monitor);
monitor.notify = fvm_desktop_playback_mailbox_load_notify;
monitor.userdata = mailbox;
/* Pass control/monitor and explicit provider to the controlled background runner. */
```

This is a binding sketch; the executable tutorial documents complete cleanup.
post returning OK acknowledges native handoff, not completion of a VM tick. A
snapshot records the state immediately before its callback command is applied;
only later observations acknowledge the new pause state/completed tick. STOP is
cooperative: it does not preempt an executing instruction/native callback, DNS,
or a provider that does not cooperate. With the existing owner loop it is polled
through paused/slow-frame waits, including final-frame visibility waits.

A mailbox does not own the runner or know that the synchronous call returned.
On every exit (success, early source error, window close, cancellation), the host
must signal its own controller-completion flag and join its worker before release.
Otherwise a worker awaiting first READY after an import failure could wait forever.
A retained snapshot is historical, not proof that its former VM is still alive.

The live interpreter/JIT tests use a real controller OS thread, FILE/MEMORY/HTTP
origins, owner-only GC/querying, exact confirmed steps and resume/pause, timer-free
slow stop, stop before startup I/O, response-body cancellation at startup and in
both async/blocking child modes, and runtime/early-import failure cleanup. Network
subcases are explicitly omitted when no provider is compiled. Native foreign-OS,
physical display/audio, race-detector/endurance and full debugger acceptance remain
open; no additional roadmap stage is declared complete.

[Executable C89/AS/SWF tutorial](../examples/player/playback_mailbox/README.md):
confirmed steps from a separate OS thread, early errors, native/cross builds.
