# Conditional prepared-owner publication / runtime0.112

`dvm_runtime_heap_commit(cell,observed,incoming,retired)` is an internal hosted C
primitive for publishing fallibly prepared work without allocating under the
commit guard. It does not introduce an instruction/module/native-extension/wire
ABI. NativeWorld remains required; general managed constructors still use their
existing paths. This API is a prerequisite, not a completed shared constructor.

The destination is an owning reference field in a live GC-rooted mapped container
or frame, not merely native storage that happens to contain a pointer. observed
and incoming are distinct exclusively owned successful heap_hold/previous-displaced
leases, with unmodified active one-entry reservations. retired is zeroed private
output. All control spans/destination are independently mapped/live and disjoint.
No live lease/reservation may be copied or externally changed during the call.
These provenance/lifetime requirements cannot be inferred from pointer geometry.
The caller also checks destination language/native-type assignability; a GC map
or a held Heap identity does not infer that semantic permission.

Under one actual destination-cell interval, compare its current pointer identity
with observed.value. A retained/rooted observed owner prevents address reuse
while held; the operation does NOT distinguish an intervening write away and back
to the same still-live identity. H has no fabricated Heap header or strong count.
Stale comparison returns0 before mark/store/retarget, preserving every input,
output, owner count and root. Wrong role/geometry/alias/live-output, guard BUSY or
barrier refusal returns-1 without publication. A barrier is monotone, not rolled
back. No allocation/free, MANAGED retain/release/destructor/VM callback is allowed.
Native guard-domain borrow accounting retains its existing private counters.

On match, mark incoming, publish its pointer while its explicit root still covers
it, then retarget that reservation to the displaced old value (or active-empty H).
Observed independently roots the old value through this transition. If retarget
refuses unchanged, restore the temporary slot store before unlock and return-1;
compliant readers see no intermediate value. Success moves incoming's owner into
the mapped slot, the displaced slot owner plus reused reservation into retired,
clears incoming fields, leaves observed unchanged and returns1 after full unlock.
No managed reference count is incremented/decremented: existing owners are moved.
The caller closes observed/retired only AFTER return, never under cell exclusion.
Prepared work that depends on mutable payload also needs payload versioning or
revalidation; pointer-identity comparison alone does not validate that payload.

After success, the destination's real map/root must make incoming reachable.
A hold is lifetime, not permission to read/write mutable payload without guards.
ACTIVE/world, container/frame/Type/referent/code/GC lifetime, participation of
other writers and compatible lock order remain external. Existing Fast retirement
may temporarily own a separate root until drainage. This is not a snapshot lease,
GC graph epoch, concurrent allocator/adoption gate, arbitrary pointer validator
or permission to remove NativeWorld.

## What stale means for constructors

An independent prepared value can retry against a new valid observation. A node
whose tail/data was derived from the old snapshot cannot blindly refresh only
observed and publish: it must rebuild/refit the candidate and its owners/barriers.
Shared constructor commit/retry/rollback, graph barriers, pool/Type lifetimes and
error-preserving cleanup remain separate work. No constructor is wired to this
primitive in this series. Never report decline/retry after a successful commit
just because later callback/retirement fails; ownership effects have happened.

## Actual evidence

A real rooted Tptr List supplies the mapped destination; actual Strings/ordinary
construction pins/heap_hold supply independent old/new owners. Tests cover six
transitions: ordinary replacement, H→value, value→H, H→H, identical identity and
stale observation after actual compliant MOVP. Stale byte snapshots of control
storage are observers, not copied owner leases. The independent String candidate
retries against the refreshed snapshot; no stale-tail constructor is claimed.

Six scenarios each execute32 actual Cycles (192 per diagnostic invocation).
Published values remain reachable through the mapped container; old values remain
held until caller retirement. Final Fast restores Tstring/Tlist/Tptr baselines.
Wrong aliases/NULL/live output, BUSY, mark failure and unchanged root refusal
preserve state; refusal after store rolls back under the same guard. Hooks verify
store precedes root retarget, both referents remain protected, exact root identity,
full release and no managed count change/allocation/destruction inside commit.
These are deterministic interleavings, not simultaneous managed mutators.

Seven private actual-source omissions detect compare/mark/retarget/store/rollback/
consumed-field/exclusion failures at exact semantic predicates before unsafe GC
or cleanup. Signals/compiler/resource/sanitizer failures never count. C/JIT guests
and both collector polls pass; the API itself is native C, not a new JIT opcode.
TSan positives instrument C, not emitted guest machine code. No physical ARM64/
RV64, full GUI, concurrent collector or all mutable aliases/destructors are proved.

[Complete commented C worked fixture](../examples/heap-commit/README.md).

```sh
python3 disVM/runtime/test_heap_commit.py RUNTIME LIMBO INCLUDE_DIR
```

[HEAPCOMMIT-112 timing scope/raw samples](PERFORMANCE-COSTS.md) measures only
preallocated successful native transfers. No old baseline, JIT/IDE speed or
component-level lock/mark/retarget attribution is inferred.

Runtime0.114 follow-up: [guarded CONSP](POINTER-TRANSACTION.md) is the first
constructor coordinator using this primitive. It discards/rebuilds stale-tail
candidates and drains all temporary holds across destructor errors. The API's
existing mapping/lifetime/NativeWorld preconditions remain mandatory.
