# Guarded hosted MOVP — runtime0.88

The actual hosted MOVP opcode now protects the source pointer cell through retain
and the destination cell through publication. Its displaced owner is destroyed
only AFTER every cell guard has been released. This applies to the C executor
and existing hosted x64 MOVP tail entries. General managed admission is still
serialized: other pointer writes, collector publication, payload access and frame/
module lifetime are not automatically converted by this opcode change.

## Native range domain

`cell_access.c` is separate from canonical per-referent `guard_registry.c`.
Referent locks protect object identity; cell locks protect the bytes holding a
reference. A runtime session owns a native domain containing256 guard stripes
and one shared parking table. These are collision locks, NOT an object/thread
quota. No metadata is allocated during begin/end. Each pointer-sized range can
cover two aligned cells; source and destination therefore require at most four
stripes. Sort/deduplicate before claim, including exact aliases, partial overlap
and modulo collisions. Hashing never reads the operand memory.

Guard claims use bounded existing native try-enter. Failure releases the acquired
prefix and native domain borrow before returning BUSY/REFUSED. No waiting for an
object owner occurs while ACTIVE. Internal metadata fallback mutexes can block.
End checks exact TLS owner and exact held depth before changing anything; extra
guards or foreign contexts refuse unchanged. Last domain destruction requires
external quiescence and no active/partial accesses. The borrow count cannot
protect a race with the first lookup. Native contexts cannot unbind or transition
world state while their guard depth remains nonzero.

The generic native API supplies exclusion only. Caller independently owns mapped
ranges and domain lifetime; geometry checks are not mapping/provenance validation,
managed roots or pointed-to ownership. All competing cell accesses must follow
this same protocol. Ordinary unguarded accesses are not made concurrent-safe by
another accessor using a guard. Do not retain an access across callbacks, VM/
error/native waits, scope transitions or arbitrary unwinding.

## Actual MOVP transaction

`prepare_shared_pointer.py` hash-binds the complete reviewed final C MOVP body,
after prior native/count/mark adaptations. Drift or repeated application refuses.
Only the owned hosted view is changed; vendor files remain byte-identical.
The opcode calls `dvm_runtime_movp_copy`, raises a returned static exception only
after native rollback, or destroys the displaced owner after success. There is
no decline/retry after successful retain/publication. A retirement/error callback
never runs under a cell guard, including callbacks that reenter or retire JIT
mappings. Existing C return-PC/tail lifetime constraints remain unchanged.

Worker worlds require exact acknowledged ACTIVE process/context/service identity.
Detached inline execution uses the checked copy without cell guards under its
existing single-owner model. Wrong role, held guards, bad geometry or output
alias refuses before cell reads. The private previous-owner output must be
disjoint from both source and destination. Unaligned slots use byte copies.

Under guards, capture source, validate its existing owner/count, capture and
validate the displaced destination owner, retain source, mark it, then publish.
Nil source, invalid predecessor and count refusal leave destination/output
unchanged. An invalid mark rolls back only the new retained count before unlock;
its independently owned source remains live. Heap/Type/payload provenance is
still a caller/VM verification premise, not inferred from a positive count.
After success the destination owns the incoming reference and the private
previous value carries exactly the displaced retirement responsibility.

Unexpected contention maps to heap refusal AFTER complete native rollback. The
current global gate makes contention between legitimate managed executors absent;
future parallel admission needs a scheduler/root-aware retry policy, not a busy
loop or unsafe fallback. Source/retirement roots, frame/module/code lifetimes,
remaining opcodes/JIT families, graph publication/GC epochs, channels/scheduler
and compiler locality/escape analysis remain open. No shared Dis64 ID/layout,
file FRAME/CALL/RET/LEA activation or ordinary WORD32 LEA change is introduced.

## Verification

The native test deliberately uses unaligned pointer-width native counters and
known boundary stripes. It checks overlap coverage, ascending/deduplicated order,
foreign end refusal, extra-guard refusal and partial BUSY rollback after an earlier
stripe was already acquired. Main does no native wait while guarded; competitors
are joined after its end. Eight OS writers perform16000 exact paired updates,
using both source/destination argument orders. These are native storage tests,
not forged managed heaps or parallel managed execution.

Both native and forced-lock backends sweep66 domain constructor allocation
refusals with a zero native ledger. A fail-first allocator interval proves no
allocation attempt during initialized begin/end. Eight source controls damage
boundary coverage, order, deduplication or metadata cleanup and require semantic10
before unsafe continuation. Extra-guard/no-allocation checks are repeated after
the selected suite, with both sanitizer backends.

Actual-runtime checks use real independently owned/rooted Strings and the actual
MOVP opcode/helper. They cover aligned/unaligned/self/nil/partial-overlap slots,
invalid mark rollback, actual kprotect nil-source error, output alias and IDLE
role refusal. Monitors require source retain under guards and helper return,
retirement and error callbacks unlocked. Five strict controls omit guards,
publication, rollback, retire before unlock or restore the exact hash-reviewed
old C MOVP. Private mutation controls are enabled only for their diagnostic
interval; old opcode/root tests retain their own independent assertions.

49 selected console/editor CTests pass. Native ranges/constructor ledger pass
TSan and O3 nonrecovering UBSan; actual hosted positives pass TSan with C/JIT and
both polling modes. Emitted JIT bytes themselves remain uninstrumented. Existing
owner/aggregate tail cache/error controls also pass. This is not acceptance of
parallel GC/payload/other-opcode execution or a new physical architecture.

## Cost and reproduction

Local short comparison uses identical current objects/services and replaces only
MOVP with the reviewed old body for baseline. After a warmup, median of three
samples of20000 string swaps: interpreter0.135s guarded vs0.131s old; JIT0.130s
vs0.118s. About3%/10% overhead in that run includes process startup and is not a
whole-IDE, isolated opcode, cross-target or statistically general speed claim.
Conservative cell guarding does not yet use compiler-proved locality.

Run `python3 disVM/tests/cell_access.py` or installed
`python3 source/tests/cell_access.py`. For actual private relinks:

```sh
python3 disVM/runtime/test_shared_pointer.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_shared_pointer.py RUNTIME LIMBO INCLUDE_DIR
```

Use the matching GNU Makefiles console runtime/compiler and retained module
include paths. Set TMPDIR if /tmp is unavailable. Instrumented positives accept
`--positive-only`; strict controls reject compiler/signal/timeout/sanitizer outcomes.
The [native example](../examples/native-cell-access/README.md) explains independent
storage lifetime and exact checkout/SDK builds. See [object guards](GUARD-REGISTRY.md),
[GC work](GC-PENDING.md) and [threading](THREADING.md).

Measured costs and pending investigations: [PERFORMANCE-COSTS.md](PERFORMANCE-COSTS.md).
