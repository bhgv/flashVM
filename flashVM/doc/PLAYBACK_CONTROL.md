# Owner-thread playback control

[Русский](PLAYBACK_CONTROL.ru.md) · [Player C API](PLAYER_CONFIGURATION.md) ·
[Network jobs](BACKGROUND_TRANSFER.md) · [Stage status](ROADMAP_STATUS.md).

## Command policy since v0.710

[fvm_desktop_playback_control_init](../frontend/common/playback.h#L68)(out) initializes caller-owned
[FVM_DesktopPlaybackControl](../frontend/common/playback.h#L57): exact struct_size, initially running, no callback or
userdata. It returns one on success, zero for null, without allocating or opening
VM/window/audio/network resources. This descriptor is separate from existing
source/options/loader structures; their ABI layouts are unchanged.

The descriptor and its userdata remain borrowed until a controlled runner returns.
The runner copies the descriptor, so callback-driven state belongs in userdata.
Only zero/one is accepted for initially_paused; an initial pause requires a notify
callback that can eventually resume, step or stop. A permanently paused controller
is the host's scheduling decision, not an implicit timeout.

| Command | Scheduling effect |
| --- | --- |
| KEEP | Preserve state; it does not resume a paused timeline. |
| PAUSE | Pause future host ticks and cancel any pending step permit. |
| RESUME | Resume continuous ticking; discard a pending step. |
| STEP | Pause and permit one host tick; repeated requests coalesce to one permit. |
| STOP | Successfully terminate this entire source run, including whole-root navigation. |

Commands use [FVM_DesktopPlaybackCommand](../frontend/common/playback.h#L11). Unknown values fail instead of silently
resuming. Stop is terminal. A step means one ordinary root advance, including its
existing descendant/EnterFrame semantics; it is not an AVM1 instruction step, a
seek, or an override of script play/stop state.

[FVM_DesktopPlaybackState](../frontend/common/playback.h#L33) is a stack snapshot. Its context/root/source are borrowed
only during notify; the root is already pinned by the runner. Scalar counters can
be copied. Do not retain context/root/managed pointers beyond notify: pinning cannot extend
the runner-owned context lifetime. Copy scalars/text or serialize during notify.
Do not unpin the player's root,
replace/destroy the active root/context, shut down the SDK or recurse playback.
No C++ exception may cross the C callback boundary. Query APIs and bounded ordinary
owner-thread VM work obey their normal C/GC contracts at these safe points.

The command policy is allocation-free C89. The dedicated
`flashvm_frontend_playback_policy` test verifies validation, atomic rejection,
pause/resume, coalesced single-step permits and terminal stop without VM/SDL startup.
Owner-loop integration is an additive layer over this policy, not a new VM debugger.

## Controlled runners since v0.711

[fvm_desktop_run_source_controlled](../frontend/common/playback.h#L77)(options, source, loader, control, error)`
executes a FILE, MEMORY or URL source with ordinary loader policy. options, source
and control are required; loader is optional. All pointed-to strings, input bytes,
callbacks and userdata remain borrowed until return. Memory sources retain their
explicit origin for relative child clips and loadVariables requests. Without an
explicit origin the existing executable-directory policy applies.

[fvm_desktop_run_source_background_controlled](../frontend/common/playback.h#L86)(options, source, loader, monitor,
control, error)` requires a background provider and load monitor in addition to
the same options/source/control. monitor.asynchronous chooses continuing owner
turns or blocking safe-point waits. Initialize every descriptor with its own init
function before assigning fields. No SDL type appears in the public control ABI.

Both return zero on normal completion, frame limit, window close or STOP; one on
playback/provider/runtime-command failure; two for invalid arguments before
startup. error is optional and receives borrowed diagnostic text; copy it before
another host operation. Neither function shuts down the process-wide SDK for its
caller. Existing runners keep their previous behavior and structure layouts.

### Notification order and scheduling

READY observes the imported, pinned root after window/audio setup, before its
first host tick. TURN runs before owner services; background turns can accept
STOP, while STEP waits for an eligible foreground turn. TICK follows a successful
root advance and presentation. A callback runs on the owner thread outside an
active AVM1 frame, never on a fetch/audio worker. Its stack snapshot is valid only
until callback return. Do not run concurrently or reenter the player.

movie_ticks and presentations reset on whole-root replacement; total_ticks and
elapsed_ms continue across replacements. elapsed_ms is monotonic milliseconds
since the first READY and excludes startup fetching. Presentations include paused
redraws; they are not timeline ticks. Counters describe completed operations.

Pause suppresses automatic timeline advances. Foreground owner turns continue
input, timers, load completion, collection and dirty redraws at a bounded polling
cadence. Scripts/services can still change objects or explicitly seek: this is
not a frozen VM debugger. Platform background handling retains its existing
service restrictions. Resume resets the frame deadline without catching up time
spent paused. STEP allows one tick immediately, even at a slow authored FPS,
and remains paused afterwards. Script play/stop semantics remain effective.

Native PCM output and its producer are suspended on pause; existing queue and
voices remain owned by the audio subsystem. Initial suspension is set before
worker creation. Stepping leaves audio suspended. Resume reuses the normal audio
path. A dummy audio test checks ordering, not audible device acceptance.

STOP ends the complete source run, prevents further root navigation, discards
outstanding load publication and joins jobs before releasing roots/audio. There
is no control notification during startup fetch or a blocking provider wait;
use the load monitor's cancellation mechanism there. Callback return values are
commands, not a thread-safe queue; an IDE must arrange its own bounded owner-thread
command handoff and keep worker threads away from VM pointers.

### Executable regression coverage

`frontend/tests/playback_control.c` and `playback_control.py` exercise real SWF8
interpreter/JIT runs from files, embedded bytes and HTTP origins: exact steps,
paused timers/redraw/network completion, immediate slow-FPS steps, root navigation,
stop before navigation, invalid commands, pinned-root collection and cancellation
after a response body has started. SDK shutdown checks worker cleanup. Policy
checks also run without starting VM/SDL. These fixtures are not a full debugger,
reference Flash comparison or native foreign-OS/device acceptance.

[Standalone C89/AS/SWF tutorial](../examples/player/playback_control/README.md):
native/cross builds, CLI as an adapter and callback ownership.

Since v0.712, TURN also runs during final bounded-frame visibility waits.
With notify installed, timer-free slow-frame waits are capped at a 10 ms polling
interval. This is a polling policy, not a real-time guarantee: VM/provider/platform
work may postpone delivery.
