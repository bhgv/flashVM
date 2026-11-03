# Reserved retirement metadata — runtime0.82

Hosted deferred RC now uses a separate native queue module instead of a global
unlocked ring. Its mutex protects free slots, reservation generations, committed
FIFO entries and the combined direct-byte budget. Native metadata does not read
referents or call the VM, callbacks, roots or destructors. The runtime keeps its
existing512-entry/4 MiB bounds; these limit queued direct extents, not OS threads
or the total descendant graph freed by one destructor.

## Reserve, root, commit

1. Under existing managed admission, protect the source and check the retained
   last-owner condition. This preflight is NOT a concurrent ownership reservation.
2. Reserve one native slot and its direct-byte budget. Unlock queue metadata.
3. Register the actual Heap root and mark under the existing GC protocol. Any
   VM error unwinds with no queue lock held; cancel the native reservation,
   restore the caller error text and leave its original owner for inline RC.
4. Commit the identity into the FIFO only after root setup succeeds. Commit moves
   the caller's existing last owner to the queue, without an extra RC increment.
5. Collector detaches a committed entry under the queue mutex, then removes its
   root and runs the retained destructor AFTER unlock, under stopped-world GC.

Slot capacity and bytes count both reservations and ready entries. FIFO order is
commit order, not reservation order. Empty take leaves outputs untouched.
A valid token includes queue identity, slot and a nonwrapping generation. NULL,
foreign, consumed/stale tokens refuse without mutation. Tokens are private and
noncopyable; identity checks do not extend the queue's lifetime or constitute
managed capabilities. New reservations close immediately; accepted reservations
must still commit/cancel. Close neither cancels nor joins producers. Quiescent
destroy refuses ready/reserved/nonzero-budget state. Join/exclude all users before
destroy; a mutex cannot protect its own lifetime. Generation exhaustion refuses
without wrapping; the host falls back to inline RC on reservation refusal.

Initialization allocates native resources before publishing the queue. Allocation
failure selects ordinary inline RC. A session cannot restart over an unfinished
queue. Failed finish retains queue storage/roots and reports failure rather than
silently dropping managed owners. Existing NATIVERC diagnostics retain their
field names. No Heap/Type/Bhdr/public Dis/Dis64 layout changes occur.

## Threading boundary

The metadata module supports concurrent native producers/consumer. The hosted
adapter STILL requires serialized NativeWorld admission for source/last-owner
checks, Setmark, roots, producer statistics and shutdown state; actual Fast/Cycles
work still owns a stopped world. Reservation is capacity bookkeeping, not a pin,
root, strong owner, COW permission or payload lock. Root publication/graph barriers,
mutator payload access, channels/scheduler/JIT and parallel collector coordination
remain before removing global managed exclusion. The new module alone does not
permit bare native threads to enqueue arbitrary managed pointers.

There is no queue/root/pool lock nesting across fallible work: reserve returns
before ptradd; take returns before ptrdel/destroy; native resource frees occur
after metadata unlock. Callers may not hold port registry or recursively acquire
this queue mutex. Ownership and callback policy stay outside the native queue.

## Checks and reproduction

`python3 disVM/runtime/test_retirement_queue.py` compiles actual queue/Linux ports.
Eight joined OS producers publish16000 immutable native identities; one consumer
checks exact once-only delivery. Keys remain native parent-owned objects through
all joins and are NOT managed Heap fixtures. Producers also cancel drafts.
Serial checks cover capacities/bytes/overflow, FIFO commit order, hidden reserved
entries, foreign/stale/NULL tokens, close with outstanding tokens and refusal to
destroy nonempty queues. Four actual constructor allocation refusals (queue,
slots, ring, port mutex) restore an explicit native allocation ledger to zero.
Three actual-source controls omit ready publication, omit cancelled-byte return
or bypass token generation; exact semantic10 is required before unsafe work.

`python3 disVM/runtime/test_rc_reservation.py RUNTIME LIMBO INCLUDE` uses matching
GNU Makefiles console build objects for private relinking. An interception refuses
one actual Heap root setup BEFORE ptradd. The actual hosted waserror path must
restore ready/reserved/byte metadata and preserve the caller's source count1
before inline destruction can proceed. Real managed-owner/root/Fast/Cycles probes
and normal guest execution then continue, with pending0. A private actual-source
missing-cancel control must fail at semantic10 before inline fallback. C/JIT and
both polling backends run. This is actual runtime rollback evidence, not proof of
concurrent managed enqueuing or every allocator failure inside ptradd.

Both scripts support `--positive-only` for instrumented positives. Native joined
metadata and actual RC-refusal positives pass TSan; native positives also pass
nonrecovering UBSan. ASan/LeakSanitizer was attempted but not accepted here:
LeakSanitizer cannot inspect /proc/task in this environment and terminates fatally,
including with requested leak detection disabled. No ASan/LSan success is claimed.
Constructor ledger checks are bounded rollback evidence, not a global leak scan.

Installed SDK carries native module/test/ports and this contract; run
`python3 source/threading/test_retirement_queue.py` inside the package. The actual
runtime relink diagnostic additionally needs its matching build objects/flags.
Set TMPDIR to a writable directory if /tmp is unavailable.
