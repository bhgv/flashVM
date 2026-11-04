# Guarded pointer-list constructor — runtime0.114

With immutable startup selection `--guarded-copies`, the actual hosted CONSP C
entry and its existing SysV x64 tail target now use `pointer_transaction.c` under
ACTIVE/NativeWorld admission. Unselected execution keeps the serialized builder;
serial inline mode also retains that body. Invalid guarded admission raises before
operand reads rather than falling back into an unguarded path.

This integrates the previously separate [Heap holds](HEAP-HOLD.md),
[candidate preparation](POINTER-PREPARE.md) and [conditional commit](HEAP-COMMIT.md).
It does **not** release NativeWorld or admit parallel allocation/collection. The
caller still supplies genuinely mapped owning source/destination cells in live
rooted frames/containers, stable Type/frame/code/operand lifetime through all
callbacks, and no object guards. Geometry/count checks are not provenance.

## Algorithm and failure semantics

1. Capture source once into an exclusive rooted hold, before any constructor
   monitor callback. If source and destination alias, this is the original value.
2. Capture the current destination tail (Tlist or H), prepare a complete rooted
   candidate with that source and tail, then attempt guarded identity publication.
3. Commit success moves the candidate owner into the destination and displaces
   its former owner into a rooted retirement lease. Publication is now complete.
4. Stale identity leaves the destination untouched. Consume the candidate and
   tail leases outside guards, capture the fresh tail and construct a new node.
   Never simply refresh the observation and reuse a node containing the old tail.
   At most eight preparations are attempted. Persistent callback-induced writes
   raise `pointer constructor destination changed repeatedly`; intervening writes
   remain intact. This is a starvation bound, not proof of concurrent VM admission.
5. Ordinary capture/preparation/commit refusal raises under the outer protected
   body. The stable native ledger owns every partial stage through error unwind.
6. Drain **all** still-live candidate/retired/tail/source leases, each under its
   own protected consuming release. Preserve the primary body error; if the body
   succeeded, preserve the first cleanup error. Rethrow only after the full drain.
   Hold fields clear before consuming destruction; no release is retried.

A destructor error after successful publication does not undo that publication.
A failed constructor does not undo writes performed by an independent compliant
writer from a monitor. Monitors may inspect/raise, not retain a private candidate,
modify borrowed holds/private state or leave admission. The source snapshot stays
fixed across tail retries even when the original operand cells alias.

The existing deferred last-owner queue may retain discarded candidate graphs
until Fast. Closing a lease does not promise immediate recursive destruction.
All such queue ownership remains rooted; tests check final counts after joined
Fast, not by pretending pending nodes have already been reclaimed.

## Tests and runnable worked example

`test_pointer_transaction.c` is a complete commented native fixture. Its Python
runner links the exact matching runtime objects and runs C/JIT guests with both
collector poll settings. Eight cases cover ordinary creation, one stale retry,
eight repeated stale attempts, primary plus multiple secondary retirement errors,
aliased cells, H, and aliased cells with a stale tail, and committed publication with multiple cleanup
errors preserving the first. Nine injected refusals cover
each of six root reservation allocations, node allocation and both Type retains.
Actual Strings, mapped List containers and stopped-world Cycles/Fast are used.
Three scoped source omissions detect missing stale-candidate retirement, incomplete
cleanup after error and primary-error overwrite at exact semantic exit10.

```
python3 disVM/runtime/test_pointer_transaction.py \
  build/runtime/disvm build/install1804/disvm-runtime/limbo-native disVM/upstream/module
python3 disVM/runtime/test_pointer_tail.py --guarded-copies \
  build/runtime/disvm build/install1804/disvm-runtime/limbo-native disVM/upstream/module
```

The existing x64 tail fixture separately checks foreign REG refusal, actual code
mapping retirement, VM errors, combined mapping/error paths and a strict bad-CALL
control. The new constructor uses saved native cell addresses after callbacks;
it does not read borrowed decoded REG or return through retired JIT code.
These are bounded Linux/x64 results, not Android/ARM64/RV64 or full IDE acceptance.

Candidate/commit omission controls are scoped to their own test calls now that
ordinary guarded constructors also invoke these helpers during startup probes.
Compiler failures/crashes/timeouts/sanitizer reports never count as semantic
control detection. [Cost measurements and raw samples](PERFORMANCE-COSTS.md)
record this path for later optimization.
