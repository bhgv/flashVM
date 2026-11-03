# Retained heap Type counters — runtime0.66

The hosted native-cell heap view now uses checked atomic primitives for every
Type counter retain/drop and allocation-preflight load in pinned heap.c. The
Type/Heap structure layout, maps, destruction order and collector algorithm do
not change. The source snapshot itself is unchanged; prepare_type_counts.py
adapts only the hosted build view and refuses changed source boundaries.

Converted sites:4 retains,4 ordinary releases,1 static nonfinal release and8
preflight loads. dtype's private initial assignment and static Type initializers
stay plain before publication. freearray/freelist retain their original final
cleanup; freetype handles nil before touching its count. A successful nonfinal
release never rereads the counter, since another owner may already retire it.
The static list-header Type keeps its existing baseline. Removing that baseline
is a fatal invariant failure, not permission to free a static descriptor.

The private adapter delegates to native signed-int primitives with acquire loads
and acqrel CAS, or the forced/platform registry fallback. No registry mutex may
already be held. Zero/negative/overflow refusal is a fatal host ownership error;
it must not recover into guest code after partial native allocation. Existing
allocation preflight guest failures remain before allocation. Helpers allocate
nothing, invoke no managed callbacks and do not acquire/release heap admission.

## Real execution and controls

The optional managed-owner probe also runs the separate type_counts_probe.c:
actual dtype/freetype, array element ownership and real Fast retirement. The
source Type owner remains live through the array's asynchronous retirement;
final freetype is never followed by a read of released Type storage.

The normal console/editor C/JIT owner drivers require the probe marker. The
Makefiles control driver relinks actual runtime objects with linker wrappers:
only while the real Type probe runs, omit its first retain or nonfinal drop.
Both must reach exact semantic status10 at the expected count predicate, before
an invalid final free. Startup operations stay unmodified. Signals, compiler
failures, timeouts and sanitizer reports do not count as successful controls.

```sh
export TMPDIR="${TMPDIR:-$PWD/build/tmp}"
mkdir -p "$TMPDIR"
python3 disVM/runtime/test_type_counts.py \
    build/workspace1735/disvm-editor/disvm \
    build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap \
    disVM/upstream/module
```

Use matching build paths. The control driver requires native GNU-style linker
wrapping and CMake Makefiles metadata; it is not a Windows/cross-run assertion.

## Remaining exclusion

Other retained C files and generated JIT Type operations still use plain counts.
Heap.ref operations in heap.c are unchanged. Mixing the converted sites with
these accesses is safe ONLY under existing serialized managed admission. This
is an incremental count conversion, not general permission for parallel Type or
Heap use. Preflight checks do not atomically reserve allocation counts for a
future parallel allocator. Type metadata, native allocator lists, marking,
root tables and source-slot ownership require their remaining protocols.

[Conversion inventory](REFCOUNT-CONVERSION.md),
[remaining managed-heap work](HEAP-PARALLELISM-STATUS.ru.md).
