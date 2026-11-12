[Current cursor-based allocator and root metadata progress](METADATA-PROGRESS.md)

# Hosted allocator metadata — runtime0.79

The hosted runtime's fixed 64 MiB first-fit arena now lives in `heap_pool.c`,
separate from VM startup. Every allocation split, free transition and live-size
lookup takes the pool's native port mutex. Initialization privately creates the
arena and mutex before publishing the Pool fields. Capacity/overflow refusal
returns nil. Invalid headers and corrupt allocation walks report through the
existing poolfault hook only after unlocking; a returning hook does not permit
the invalid operation. Hooks may reenter a valid pool or raise a VM error.
Neither allocation nor free invokes a managed payload destructor under this lock.

Pool's private native owner adds extent and mutex fields; Bhdr, Heap, Type and
serialized Dis/Dis64 layouts do not change. Splitting preserves old block header
addresses. Freed blocks are not coalesced: the retained incremental collector
can preserve a block cursor between quanta. Pool metadata exclusion does not make
that raw collector walk concurrent. `poolchain` borrows the stable arena header;
a raw traversal requires existing stopped-world admission or complete quiescence.
Concurrent initialization, close and use-after-free are outside this contract.
Quiescent destruction refuses live or corrupt blocks and destroys native resources
only after releasing exclusion. The production arena keeps process lifetime.

`poolfree` and `poolmsize` require independent live allocation ownership. Range,
alignment and magic checks are diagnostics, not pointer provenance, an ownership
acquisition or COW permission. This mutex does not protect managed payloads,
reference publication, roots, mark bits or the reference graph. NativeWorld still
serializes managed execution. Safe parallel mutators still need payload guards,
compiler-selected local/shared operations, graph/root barriers and coordinated
GC/channel/scheduler/JIT paths. Removing NativeWorld exclusion now is unsafe.

## Reproduce

From the checkout, run `python3 disVM/runtime/test_heap_pool.py`. The installed
runtime/editor SDK also carries the actual sources, exact retained header closure
and Linux ports; run `python3 source/threading/test_heap_pool.py` inside it.
Set `TMPDIR` to a writable directory when the environment lacks `/tmp`.

Eight joined OS threads perform16000 allocation/write/size/free rounds with
independently owned native byte buffers, then a quiescent walk checks all blocks
and exact arena coverage. These are actual allocator/port sources, not simulated
managed objects. Serial checks cover invalid extents, oversized refusal, live
close refusal, invalid pointers, corrupt headers and returning/reentrant error
callbacks. Three actual-source controls omit allocation/free publication or
invoke the hook while locked; each must fail at exact semantic10 before unsafe
continuation. No signal/timeout is accepted as a successful negative control.

For instrumented joined positives: `CFLAGS='-O1 -g -fsanitize=thread
-fno-omit-frame-pointer' python3 disVM/runtime/test_heap_pool.py --positive-only`.
The normal runtime tests separately exercise real String/List/Type/Module owners,
Cycles/Fast collectors, native waits and the checked pointer-builder C/JIT tail.
These tests do not establish concurrent collector/mutator safety.
