# Packet cycle seeds and rooted ticket handoff

Copyright (c) 2026 bhgv

EN: Shared Fast preserves packet owners across candidate/root allocation failures,
replays refused snapshots and stages rooted tickets before cycle discovery.
RU: Shared Fast сохраняет владельцев пакета при отказах очереди/корня,
повторяет отклонённые снимки и задерживает обход до снятия корня ticket.

## Ownership changes

A child packet previously tried indexed cycle publication and could then drop a
nonfinal record reference when publication failed. That consumed a possible last
cycle seed. The new [packet rescue](gc/cycles/packet_rescue.inc) reserves mandatory
FIFO storage, publishes an individual replacement root, withdraws exactly the
source batch occurrence and commits the existing strong reference. It does not
retain or decrement the object. Only a successful handoff advances the cursor.
FIFO/root allocation refusal preserves the source reference, root and cursor;
shared Fast reports retry. The existing duplicate-index rescue retains that owner
until a fresh candidate can be published. Closed admission retains its old route.

A real refused graph snapshot returns its held references through the same packet
path. Moving the snapshot into packet work now completes its accepted queue stage.
Subsequent queued child owners count as new accepted stages. `NATIVERC` counters
measure queue stages, not unique allocations or physical references;
`NATIVEPACKETCYCLE` reports preserved owners, refusals and replayed snapshots.

Repeated full instrumentation exposed another seed loss: shared ticket handoff
published a candidate while the old ticket root still existed. Cycles could
classify this temporary overlap as external reachability and discard the seed
before Fast withdrew the ticket root. Shared RC producers now stage candidate
owners in the mandatory FIFO. The sole Fast consumer drains it after finishing
ticket handoff, so Cycles cannot observe this source-root overlap. ACTIVE producer
publication remains indexed. This adds no execution gate or global stop.

## Actual runtime evidence

`disvm_packet_cycle_retirement` creates ordinary managed records, genuine strong
edges, descriptors and a last-owner envelope. Only the envelope enters through a
native ticket. Production field packets must carry the cycle seeds. Two permanent
resident peers alternate their real shared epochs while the other stays admitted;
physical envelope and node pool frees must precede their release. Allocation
identity includes the real generation. Root withdrawal and exact descriptor counts
are checked independently of diagnostic queue accounting.

Eight scenarios each run with C/JIT and both poll modes:

| Scenario | Required evidence |
| --- | --- |
| Indexed queue full | Original owner rescued through mandatory FIFO |
| Duplicate envelope fields | Duplicate index waits before another seed is offered |
| Replacement root OOM | Source owner/root survive refusal and retry |
| Mandatory FIFO full | Repeated real Fast retries without stopped Fast admission |
| Refused snapshot | Actual published graph and pins replay through packet rescue |
| Refused snapshot plus OOM | Replay retains ownership through root failure |
| 2300-node ring | Resumed Duke discovery and physical release of all nodes |
| External native owner | Actual live scan preserves both fields; later owner release collects |

Snapshot cases inject one confirmation refusal into a real rooted graph. They do
not modify its fields or claim coverage of concurrent field mutation. Observers
and allocation fault injection forward actual allocator, queue, collector and
scheduler operations. Five private semantic omission controls require explicit
ownership failures: dropping the seed, omitting its replacement root, omitting
retry, advancing the refused cursor, or publishing a ticket seed too early.
Compiler failures, timeouts, signals and sanitizer reports do not count as passes.

Validation: the focused 83-test selection and ten longer GC tests are recorded
alongside this series. Ten complete positive repetitions under full C
ThreadSanitizer instrumentation passed after the rooted-ticket fix (320 app
runs); two additional repetitions of both packet and existing cycle-owner
profiles passed (96 more app runs). The cycle-owner fixture now identifies the
staging FIFO separately and injects pressure into the later Cycles reservation;
its exact root checks account for both staged owners in duplicate mode.
Generated JIT instructions are not C-TSan instrumented. All execution was
Linux x86-64; ARM64 and RV64 remain unverified.

## Remaining scope

This extends [cycle-owner rescue](CYCLE-OWNER-RESCUE.md), not the complete no-STW
runtime. General tracing, cycles containing Modlink, unsupported native finalizers
and closed-admission shutdown still require work. The remaining acceptance limits
in [CONCURRENT-GC.md](CONCURRENT-GC.md), [FAST-FINALIZATION.md](FAST-FINALIZATION.md)
and [MODULE-RETIREMENT.md](MODULE-RETIREMENT.md) still apply.
