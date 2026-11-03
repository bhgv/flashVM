# Retained heap Heap counters — runtime0.67

The hosted heap.c view now uses checked native ulong primitives for all its
Heap count operations:3 retains,2 release sites and5 observations. Together
with runtime0.66 Type conversion, only unpublished count initializations remain
plain in this file. Pinned source is unchanged. Exact source-boundary matching
refuses drift before producing a hosted view.

Converted retains: checktype, mapped incmem and arraycopy-owned fields. Their
existing Setmark calls remain after acquisition. Observations: heapref, backing
root/slice validity and arraycopy zero/overflow guards. An atomic count load does
not acquire lifetime or prove uniqueness of mutable payload. All these callers
still require serialized managed admission and valid source ownership.

## Release and C-stack protection

Destroy keeps the existing deferred-owner hook first. Otherwise a checked release
returns LIVE or LAST. LIVE returns without another count/object read. LAST only
transfers retirement responsibility; the retained `gchalt > 64` condition still
suppresses destruction to protect C-stack references. A zero-count protected
block is not resurrected or automatically freed by the helper.

The iterative freelist tail similarly breaks immediately after a nonfinal release;
only LAST enters tail destruction. Type/element cleanup, monitor callbacks,
marking and allocator order remain unchanged. Helpers invoke no destructor or VM
callback themselves. Native zero/overflow invariant refusal is terminal; it does
not recover into guest code after partial ownership mutation. Registry fallback
works with the same rule that caller must not already hold the registry mutex.

## Actual runtime evidence

heap_counts_probe.c runs through actual retained functions:

- String checktype retain and nonfinal destroy, with heapref observations.
- A real dtype map retaining a String through incmem, then releasing that owner.
- Two-node list destruction with iterative tail LAST retirement.
- The same shape with a distinct tail alias: LIVE must preserve the tail, then
  final alias destruction returns the static Tlist baseline exactly.

The C-stack alias has both a strong owner and an explicit ptradd/Setmark root
before admission is released. Root removal and final destroy occur only after
reentry; no object is read after final owner consumption. Real Fast retirement
runs between phases. The normal console/editor C/JIT owner driver requires the
Heap probe marker as well as its Type/root markers.

```sh
export TMPDIR="${TMPDIR:-$PWD/build/tmp}"
mkdir -p "$TMPDIR"
python3 disVM/runtime/test_type_counts.py --heap \
    build/workspace1735/disvm-editor/disvm \
    build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap \
    disVM/upstream/module
```

This optional Makefiles/native GNU-linker driver relinks actual runtime objects.
Only during the real Heap probe does it omit the first retain/nonfinal drop;
exact semantic status10 must precede unsafe freeing. Startup stays unmodified.
Signals, compiler errors, timeouts and sanitizer failures do not count as
successful controls. Without --heap the earlier Type controls remain available.

## Remaining boundary

All other retained C/JIT Heap/Type count sites, GC field accesses, allocator lists,
payload writes and source ownership require their remaining conversion. A count
being atomic in heap.c does not permit concurrency with plain accesses elsewhere.
COW/refcount==1 checks are not locks; concurrent graph mutation and allocation
need their own protocol. Managed admission stays serialized. This does not alter
Heap/Type layouts, Dis64 IDs, native pointer representation or file activation.

[Type sites](TYPE-COUNTS.md), [inventory](REFCOUNT-CONVERSION.md),
[parallel heap status](HEAP-PARALLELISM-STATUS.ru.md).
