# Resident readiness and owner publication

ThreadSanitizer on the real managed-ticket workload identified three boundaries
that were still relying on the old serialized execution model:

1. `finish` release-stored `dispatch_owner`, while `append_ready` read it twice
   with ordinary loads. Besides a data race, the two reads could observe different
   ownership states during one wake decision.
2. Ready publication release-stored `queued`, while selection used ordinary reads.
3. Channel completion wrote `Prog.state` while the resident inspected it for
   mailbox delivery and saved-register validity without matching exclusion.

Published dispatch and native-continuation owner identities now use acquire
queries for every production read and release stores for their writes. A wake
uses one dispatch-owner snapshot. `dvm_runqueue_is_ready` provides the same
acquire observation for ready flags; it does not acquire process lifetime.
Callers must still independently retain the process allocation.

The existing channel queue mutex is configured at startup as the borrowed
readiness-publication mutex. Channel completion drains messages and runs callbacks
**before** taking it, publishes state/ready metadata under it, then unlocks before
notifying workers. This avoids recursive acquisition from cancellation reply
callbacks. Resident wait-request dispatch checks its mailbox first, takes a short
state snapshot under that same mutex, then unlocks before handlers run. Saved
register validity samples state under matching exclusion. Kill checkpoints use
published readiness rather than unsynchronized state reads.

This is a short channel-metadata protocol, not a mutator execution gate or a GC
world stop. Other instructions continue on their OS threads. Native return keeps
the original resident and live register set; no ready-queue migration, quantum or
register reload has been introduced. Legacy FIFO reference behavior remains.

| Responsibility | Source |
| --- | --- |
| Readiness, FIFO coordination and publication | [runqueue.c](runqueue.c) |
| Original-thread native continuation leases | [runqueue_native.c](runqueue_native.c) |
| Process pins and acquire owner/ready queries | [process_lifetime.c](process_lifetime.c) |
| Shared native lease accounting | [runqueue_private.h](runqueue_private.h) |
| Runtime source composition | [host_runtime/README.md](host_runtime/README.md) |

The actual FIFO unit still checks 20,000 oracle operations and eleven semantic
ownership controls: early table wake, native return accounting, synchronous holds,
foreign restoration/claim/finish, duplicates, reserved native-return capacity,
withdrawal, nested dispatch and early unbind. It now configures a real publication
mutex and reacquires it at notification, verifying that notification follows unlock.
`--positive-only` supports ThreadSanitizer runs without omission mutations.

After the fixes, ten instrumented managed-ticket repetitions completed all eight
normal/refusal × C/JIT × polling profiles: 80 application executions, 160 permanent
peer residents and 80 registered native observers. No sanitizer warnings appeared
in those repetitions. The standalone FIFO positive also passed ThreadSanitizer.
Generated JIT machine instructions themselves are not instrumented by C TSan;
this evidence covers the instrumented native protocol and its observed behavior,
not every generated memory access or every scheduler path.

Final normal compilation and all 76 selected ownership, root, retirement, resident,
channel, capture and native-boundary CTests passed after source formatting. A
final full-runtime TSan rebuild also passed all eight positive/refusal profiles.
This focused result does not replace the broader acceptance failures recorded
in [CONCURRENT-GC.md](CONCURRENT-GC.md), or validate ARM64/RV64 execution.

## Controller fallback lifetime

The subsequent Fast finalization workload exposed a further retirement race:
foreign residents compared their process against the shared `initial_guest` while
the controller cleared that pointer before freeing its own process. The fallback
is now native thread-local. The controller alone initializes, uses and clears its
fallback; other residents continue using their permanent dedicated slot. No
foreign pointer lifetime or register restoration is added. Repeated full-runtime
TSan verification is recorded in [FAST-FINALIZATION.md](FAST-FINALIZATION.md).
