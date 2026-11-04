# Atomic replacement of an active root group

Runtime0.107 adds `dvm_runtime_root_batch_replace(batch,keys,new_count)` to the
existing runtime C root API. The opaque reservation layout, old publish/remove/
clear contracts, serialized managed admission and Dis64 wire/ABI stay unchanged.
It is not a new bytecode instruction or JIT entry.

## Visibility and ownership

An active exclusively owned batch can replace its entire multiset under one
actual registry-mutex interval. Visitors observe the old or the new group, never
the remove/publish gap. Validate capacity, vector presence and every nonnull key
before any mutation; malformed input returns0 unchanged and acquires no root
mutex. Successful replacement returns1. Empty replacement remains active and
requires clear before destroy. Old sparse occurrences are removed; new index
numbering is exactly the new vector order. Duplicate keys remain independent
occurrences, and ordinary remove cannot consume batch entries.

Key storage must remain stable, mapped and disjoint from private batch metadata.
The caller separately retains valid old/new referents and strong owners through
return. New referents keep publish-lifetime obligations until withdrawal; old
retirement still requires collector-lifetime exclusion after the call. No retain/release, payload read, root callback, allocator/free, destructor,
VM error or object-owner park occurs inside. Metadata mutex waiting retains its
existing policy; no wall-clock bound follows. Distinct externally owned batches
can update concurrently. One batch cannot be concurrently operated on without
external caller exclusion; quiescent handoff across OS owners is allowed.

Hosted use still requires current ACTIVE/stopped-world publication exclusion,
compatible lock ordering and root/graph epochs. Registration is not ownership,
provenance, type validation, a collector snapshot lease or permission to free
copied root addresses after visitor return. The existing visitor keeps keys stable
only within its locked callback. Concurrent traversal of a changing graph and
retirement require additional protocols; NativeWorld remains required.

## Withdrawal order and cost

Nodes are inserted at bucket heads in increasing vector index. With duplicate
keys, withdrawing the lowest index first repeatedly scans the same chain.
Replacement therefore withdraws in reverse index order, as clear already does.
This avoids self-group quadratic scans without changing atomic visibility or
index semantics. Other groups/ordinary roots sharing a bucket can still add
lookup work; this component is not measured separately. Benchmark provenance,
samples and the same-series equivalent forward-order experiment are recorded as
ROOTREPLACE-107 in PERFORMANCE-COSTS.md. That is not a published old-runtime,
whole-VM, JIT or IDE speed comparison. Neither variant opens a publication gap.

## Evidence

Native fixtures use actual registry/ports with native and forced-lock publication
backends: eight joined OS publishers perform16000 replacements alongside a locked
scanner. A halfway rendezvous outside all root/API intervals requires a complete
scan after8000 transitions and before the remaining8000. Each visit checks full
per-owner generation/count/index coverage. Native opaque keys never enter a VM
collector. Allocation/free wrappers and actual lock/unlock counts require exactly
one exclusion interval on success and none on input refusal. Sparse, duplicate,
capacity/null/late-null, new-index and active-empty cases are explicit.

Six private source controls per backend detect split exclusion, missing withdrawal,
missing publication, skipped input validation, wrong published count and allocation
attempt. A fixture-only hook inspects real RootEntry links while already locked,
before a controlled missing-unlink could corrupt a reused list node. No hook is
compiled into the production helper. Controls require semantic10 and exact marker;
crashes/compiler/sanitizer/resource failures never count.

The hosted fixture uses actual String allocations and actual GC roots. Every
new C-local owner is pinned immediately before subsequent managed allocations.
Replacement runs while real source/destination cell guards are held. After
complete unlock, old owners retire and temporary ordinary pins are removed;
only the new batch roots remain during32 real Cycles. New payloads survive,
new index removal works and final Fast restores the real Type baseline. Owner
counts are conservation observations, not RC1 uniqueness tests. Omission stops
before missing roots reach GC; refusal preserves old roots. No fake Heap/native
key enters the collector. C/JIT refer to the surrounding real guest modes; the
new API itself is native C, not a newly compiled Limbo/JIT root instruction.

```sh
python3 disVM/runtime/test_root_replace.py
python3 disVM/runtime/test_managed_root_replace.py RUNTIME LIMBO disVM/upstream/module
```

`--positive-only` omits deliberate defects while retaining actual refusal checks.
TSan positives instrument native and hosted C, not machine-code guest instructions.
No physical ARM64/RV64, concurrent managed graph collector or full editor proof
is inferred. The installed SDK includes source, diagnostics and
[a commented native example](../examples/root-group-replace/README.md).
