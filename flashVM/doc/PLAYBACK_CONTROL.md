# Owner-thread playback control

[Русский](PLAYBACK_CONTROL.ru.md) · [Player C API](PLAYER_CONFIGURATION.md) ·
[Network jobs](BACKGROUND_TRANSFER.md) · [Stage status](ROADMAP_STATUS.md).

## Command policy since v0.710

[fvm_desktop_playback_control_init](../frontend/common/playback.h#L65)(out)` initializes caller-owned
[FVM_DesktopPlaybackControl](../frontend/common/playback.h#L54): exact struct_size, initially running, no callback or
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

Commands use [FVM_DesktopPlaybackCommand](../frontend/common/playback.h#L10). Unknown values fail instead of silently
resuming. Stop is terminal. A step means one ordinary root advance, including its
existing descendant/EnterFrame semantics; it is not an AVM1 instruction step, a
seek, or an override of script play/stop state.

[FVM_DesktopPlaybackState](../frontend/common/playback.h#L32) is a stack snapshot. Its context/root/source are borrowed
only during notify; the root is already pinned by the runner. Scalar counters can
be copied. Do not retain raw VM pointers in a worker, unpin the player's root,
replace/destroy the active root/context, shut down the SDK or recurse playback.
No C++ exception may cross the C callback boundary. Query APIs and bounded ordinary
owner-thread VM work obey their normal C/GC contracts at these safe points.

The command policy is allocation-free C89. The dedicated
`flashvm_frontend_playback_policy` test verifies validation, atomic rejection,
pause/resume, coalesced single-step permits and terminal stop without VM/SDL startup.
Owner-loop integration is an additive layer over this policy, not a new VM debugger.
