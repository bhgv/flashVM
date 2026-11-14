# Independent diagnostic mailboxes

Copyright (c) 2026 bhgv

[Русский](DIAGNOSTIC-MAILBOX.md) · [Owner locations](OWNER-LOCATION.md)

Each resident has an independent native mailbox for read-only probes. Its
permanent owner services requests. Progress replies remain scalar; location
captures add bounded copied UTF-8 labels, never managed pointers or retained
Module references. External transport, stack/typed-variable capture, pause/resume,
priority and mutating-control capabilities remain separate work.

## Guest API and credit lifetime

Matched Processctl provides:

| Method | Result |
| --- | --- |
| residentrequest(exact ID, cookie) | status, ticket |
| residentreply(exact ID, ticket) | status, owner PID, cookie, dispatches, turns, waits |
| residentforget(exact ID, ticket) | status |

The ticket belongs to the original requester PID and this target mailbox lifetime.
Numbers increase monotonically up to signed-big max and are not reused after
consume/forget. Another Prog cannot read or forget the ticket by learning its
number. This is not authority to run arbitrary guest code or control a thread.

| Status | Meaning |
| --- | --- |
| DiagReady = 1 | Request accepted, complete reply consumed or credit forgotten |
| DiagPending = 0 | Owner has not answered; polling keeps the credit |
| DiagInvalid = -1 | Invalid scalar argument |
| DiagFull = -2 | All eight Pending/Ready/Closed credits are occupied |
| DiagClosed = -3 | Owner retired; no new requests; pending reply became terminal |
| DiagMissing = -4 | Exact target/ticket is no longer registered |
| DiagDenied = -5 | Ticket belongs to another requester |
| DiagExhausted = -6 | Lifetime ticket counter is exhausted |

Refused submissions return ticket=0. Every non-Ready reply zeros the entire
payload. Ready/Closed consumption frees one credit; repeated reads are Missing.
Closing preserves existing Ready snapshots until consume/forget/reaping. A target
may disappear before polling: a ticket does not keep its Prog, heap or OS thread
alive. Native effects precede guarded publication; Busy retry never repeats them.

Nonconsuming location/header/byte projections are described in OWNER-LOCATION.md.
They retain the same bounded credit, including on Closed, until ordinary reply
consumption or forget. This is an in-VM protocol, not cross-VM authentication.

## Isolation and servicing

Eight slots and a native metadata mutex are allocated with the resident. Submit,
poll and forget do not allocate. Ready replies continue to occupy the same quota.
An atomic pending hint avoids mailbox locking on an inactive path. Ordinary
channels and owner cancellation/reply queues do not share diagnostic nodes or
payload buffers.

Registry lifetime protects the target during enqueue/copy. Enqueue signals only
that owner's private condition. A channel-blocked guest can therefore answer
without receiving an application payload. The oldest pending ticket wins even
after physical slot reuse. Each servicing point answers at most two tickets
before permitting ordinary dispatch; this is not a hard realtime latency bound.

The sampler checks permanent TLS/context identity and copies native publications.
Location production reads only the owner's rooted module/live PC while ACTIVE;
idle service copies the resulting native record. No foreign Prog/REG, getter,
remote RPC or arbitrary guest callback is executed. A blocking native callback
can keep the query Pending until its safe return. Forgetting the diagnostic query
does not cancel that sleep, application RPC or channel operation.

## Locking and cleanup

Registry guard precedes mailbox lock. Mailbox code never takes registry/heap
locks or calls back into the VM. Producers release mailbox lock before taking
the wake mutex; the idle predicate reads the atomic pending hint. There is no
new global execution lock, exclusive admission or stop-the-world.

Requester retirement releases its credits across still registered mailboxes.
An empty global credit hint skips the registry scan entirely. Otherwise cleanup
is O(residents*8); requester indexing and per-client/global quotas remain scaling
work. The original single-owner requester has already finished its submissions.

Owner retirement closes Pending and leaves Ready readable until reaping. Startup
refusal, reaping and full shutdown destroy native boxes after owner lifetime ends.
NATIVEDEBUGMAILBOX must report created=destroyed and live/requests/pending=0 after
joins. Native copied bytes are not GC roots; target identity never redirects to
the next registry card.

## Acceptance

The real-VM fixture uses six guests and C/JIT with fast/locked polling: blocked
channel responses, exact owner/cookie including signed-big endpoints, eight-credit
quota, foreign requester denial, monotonic tickets, forgetting, automatic sender
cleanup, 32 ordinary transfers during probes, pending native sleep and retirement.
A watcher concurrently reads a shared 48-byte scalar reply between canaries.

The actual production queue unit covers completed-reply quota, FIFO after slot
reuse, invalid arguments/budgets, zero failure outputs, forgetting and idempotent
close preserving Ready/closing Pending. Its extra native box also drains. Two
omissions hide pending wake or sender cleanup: only exact DETECTED diagnostics
with normal exit and complete native/GC drain count, never crashes/timeouts or
sanitizer failures. Workload debug-mailbox is mandatory; owner-location extends
capture acceptance independently. See the RU contract and owner-location documents
for the recorded selected-suite scope; neither substitutes for new architecture CI.
