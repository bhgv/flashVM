# Shared managed-ticket retirement

A managed ticket owns one strong managed reference, one root occurrence and a
canonical native guard lease. Native aliases share the ticket reference count;
they do not create additional managed roots. The last native alias now records
its reader cutoff before publishing the ticket to Fast. Its producer pin remains
live until the collector notification returns, even if Fast already frees the ticket.

The implementation is separated into readable modules:

| Responsibility | Source |
| --- | --- |
| Capture, native alias retain/drop and current-role validation | [managed_owner.c](managed_owner.c) |
| Native queue, close/finish and retained reference drain | [managed_owner_service.c](managed_owner_service.c) |
| Canonical guard identity and checked cell/range access | [managed_access.c](managed_access.c) |
| Private service state and FIFO consumer interfaces | [managed_queue.h](gc/common/managed_queue.h) |
| Bounded shared Fast handoff | [managed_retirement.c](gc/fast/managed_retirement.c) |

`dvm_managed_owner_drain_shared` registers a Fast service participant and uses
nonblocking shared admission. It waits for the native reader cutoff without
parking a guest. RC must accept the existing strong reference before the ticket
root or guard lease is removed. A queued RC transfer publishes another root first;
a nonfinal drop leaves the independently owned referent alive. Refusal leaves the
pending ticket, its root and its guard lease untouched. Only the sole Fast consumer
can remove FIFO heads. Native queue exclusion never spans RC publication or root
operations. Completion accounting happens after the actual handoff.

Fast divides a bounded pass between tickets, emergency reserves and ordinary RC
work, returning unused category capacity when that category has no work. A reader
retry is distinct from a completed ticket in the diagnostics: `MANAGEDSHARED`
counts only completed handoffs and how many overlapped an admitted guest.

This removes the ticket metadata/root handoff's dependency on stopped-world
ownership during normal execution. It does **not** convert every managed destructor
or cyclic graph. A final object may move to the existing RC queue and still require
its retained fallback. Closed RC admission, exhausted epoch numbering and refused
service admission preserve the existing retained ticket drain. The main runtime
still uses the mixed collector profile, not the closed concurrent-only backend.

## Real heap and failure controls

[test_managed_retirement.py](test_managed_retirement.py) relinks real runtime
objects with a native observer, without substituting a heap, RC implementation or
collector. Two permanent residents alternate their admission epochs while at
least one always remains active. A third native thread receives a real exported
ticket alias, begins an explicit reader epoch and drops the last native alias
without managed admission. Fast must preserve both queued tickets while that old
reader remains. After read_end it must retire both tickets with no world stop.

The two values cover a final managed owner transferred to RC and an independent
external managed owner. The test checks exact root occurrence counts, zero pending
tickets, zero canonical guard leases, unchanged external ownership, continuation
of the two resident processes and complete teardown. A forced RC refusal must
leave all three ticket resources unchanged and succeed on retry.

Private omission controls must fail at specific semantic predicates when they:

- disable shared ticket processing;
- bypass the reader epoch check;
- omit removal of the ticket's root.

Ordinary acceptance runs C and x86-64 JIT with both polling modes. The optional
`--positive-only` run keeps the normal/refusal profiles for instrumentation and
excludes intentional ownership violations. Compiler errors, sanitizer reports and
timeouts never count as successful omission detection. The existing managed-owner
controls still verify root retention and the producer pin across a forced overlap;
the balanced-count check permits additional guard-probe tickets while retaining
exact captured/released equality and empty joined state.

## Scheduler defects exposed by instrumentation

A fully instrumented runtime initially reported mixed ordinary/atomic reads of
published dispatch ownership, ordinary reads of the ready flag and a channel
state-publication race. These are actual runtime defects, not suppressed reports.
The corresponding fixes and validation are described in
[RUNQUEUE-PUBLICATION.md](RUNQUEUE-PUBLICATION.md).

No ARM64/RV64 execution or complete absence of runtime races is claimed by these
x86-64 tests. The remaining work is the [no-STW plan](MULTITHREADING-ROADMAP.ru.md).
