# Child retirement owns its reply storage

The parent previously embedded a single `child_retired` node. Distinct children
could write and enqueue that same allocation when an interrupted wait was
followed by another `killwait`, with stale replies still delaying the consumer.
The second producer overwrote a node still owned by the first queue entry;
re-enqueuing it could duplicate the node or create a queue cycle.

`test_retirement_storage.py` reproduced the original implementation's failure:
`B reused A's queued retirement node` (exit 12, before corrupting the router).
The fixture executes real Limbo parent cancellation and real A/B native owner
retirement. A native observer explicitly adds 32 owned scalar replies with
unissued ID zero ahead of A. **This mailbox debt is adversarial protocol input,
not 32 claimed Limbo operations.** It ensures the consumer cannot drain A before
B is posted. Neither guest state nor registers are fabricated; a real empty
mailbox observation is held across publication, modeling a legal concurrent poll.

The reply allocation now belongs to the child event:

- `prepare_dedicated_child_wait` reserves one node before parking the caller.
  Allocation failure leaves the caller runnable. A cancelled/rearmed wait on the
  same registered child may reuse this still-untransferred reserve.
- Child teardown takes the reserve out of its process wrapper before freeing
  the process. Publication uses only copied PIDs and the native node.
- A matching parent `killwait` receives exclusive ownership. A parent waiting
  for a cancellation result is not a retirement recipient.
- If interest was withdrawn or the parent is gone, the child frees the reserve.
  No allocation is required in retirement, after process storage is released.
- The consumer frees both accepted and stale retirement replies. Endpoint
  cleanup and resident stop also free queued/staged replies and unused reserves.
  Embedded channel notices retain their existing lifetime protocol.

The observer requires distinct A/B allocations, exact B acknowledgement, stable
native owners and strict no-stop GC evidence. All 34 nodes (two real retirement
replies and 32 explicit debt nodes) must be freed. Interpreter/JIT and both GC
polls execute. A second positive variant refuses the first reply-reserve allocation,
catches the genuine out-of-memory exception, verifies that the parent remains
runnable with no active wait, and successfully retries the whole protocol.
A private omission control restores one shared reply slot and
must reproduce the exact aliasing failure; timeout/compiler/sanitizer failures
do not count as detection. The positive probe is in the 44-workload Linux64 wide
profile for AMD64, ARM64 and RV64/QEMU.

Регрессия использует настоящие завершения двух детей и явно добавленные старые
ответы mailbox. Общая ячейка родителя заменена резервом конкретного ребёнка;
выделение происходит до парковки, освобождение — у текущего владельца. Новых
глобальных блокировок, переноса Prog или восстановления рабочих REG нет.

## Validation checkpoint

Production commit: `74fbab4f2aa066ba850554f9bcb5607d6832c8c0`.
Local AMD64 completed uninterrupted 44/44 wide workloads in ordinary and fully
TSan-instrumented builds, plus 100/100 native/boundary CTests. The constructor
oracle passed separately under TSan. Targeted final checks passed 3/3 after a
comment-only header clarification; executable header text is unchanged. The final
post-build TSan subset passed 5/5, including both omission controls.
The new probe's positive allocation-refusal/retry and shared-slot negative
control passed under TSan, as did the preceding cancellation/old-reader controls.

[CI for this production change](https://github.com/bhgv/flashVM/actions/runs/43384574676)
completed all four jobs: native AMD64, native ARM64, AMD64 TSan and RV64 QEMU
with 44/44 each. The additional AMD64 constructor oracle passed both normally
and under TSan. RV64 remains emulated functional coverage,
not certification of hardware memory ordering or throughput. Changed handwritten
source is at most 397 lines per file; upstream snapshots are unchanged.
