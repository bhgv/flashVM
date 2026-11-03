# Hosted C scalar GC fields — runtime0.83

Hosted C Setmark now calls a native scalar helper. Heap.color, the mutator color
selector and pending-propagation flag use acquire/release int operations when
native int atomics are lock-free; otherwise the existing port registry mutex
protects each operation. The low-level mark operation preserves the conditional
behavior: a color matching selector0..2 stays unchanged; another valid color
becomes propagator3 and then pending becomes1. Helpers never allocate, call the
VM, invoke callbacks or acquire managed admission. Malformed native geometry or
invalid mark color/selector returns refusal before writes; hosted wrappers make
refusal a terminal host invariant after all field locks have been released.

This is scalar metadata access, NOT a graph/epoch transaction. Source/selector/
pending storage must remain independently live, aligned and protected; result
storage is private/disjoint. Address checks are not provenance, a strong owner,
a root or payload permission. Caller must not hold the port registry mutex.
Native mark's two stores are not one atomic multi-field snapshot. Selector
changes and flag clearing remain under existing collector exclusion; allowing
concurrent epoch reset/graph mutation now could still lose progress.

## Exact hosted scope

`DVM_RUNTIME_GC_FIELDS` enables the Setmark override only in hosted runtime targets,
leaving standalone retained source fixtures/owning Inferno untouched. Macro
argument is evaluated once. Reviewed final gc.c and heap.c views are hash-pinned
before scalar adaptations; native interp.h is also pinned to bind Heap layout
and propagator3. Drift/repeated conversion refuses. Explicit C collector color
reads/writes, allocation color stores, selector update, pending reads/writes and
the checked pointer-builder's new-header color initialization use these helpers.
No Heap/Type/native header size, shared Dis64 ID or wire layout changes.

Generated native JIT marking sequences still include plain color/flag accesses;
this series does NOT claim to convert or instrument all of them. Existing
NativeWorld serialized admission makes those accesses nonconcurrent with the
C mutator/collector paths. Removing that exclusion is still unsafe. JIT mark/
epoch handling needs its own checked profile and actual target ABI/cache/error
proof. Marker/sweeper/quanta/cursor/gchalt and graph traversal state remain under
stopped-world collection. No parallel collector/graph protocol is inferred.

## Header snapshots

Heap snapshots now EXCLUDE BOTH color and ref cells from ordinary byte copies.
Each is loaded through its native field/count primitive. Remaining header bytes
still require frozen metadata/managed exclusion. The private snapshot is not a
coherent multi-field graph observation and owns none of its referenced objects.
Count-only observation continues to read ref alone. Layout geometry refuses
before output/source access if the expected color-before-count relation changes.

Actual diagnostic keeps a real String owner and blocks collector admission while
8 joined native workers receive ONLY protected color/count cell addresses plus
an immutable copied selector and a separate native pending flag. Workers do not
read payload/Type/other Heap fields, enter a managed role or change runtime GC's
selector/pending globals. They perform16000 mark/count pairs while the controller
makes4000 private snapshots; other metadata stays frozen. After joins, original
color is restored BEFORE ordinary destruction/Fast. This is field-access and
snapshot evidence, not arbitrary bare-native managed access or parallel GC.

## Checks and reproduction

`python3 disVM/runtime/test_gc_fields.py` compiles actual native helper/ports.
Both native-selected and forced-lock backends run8 joined OS writers/16000 marks
on native scalar fixtures with fixed selector/sticky pending flag. No native
fixture is registered as a managed Heap. Serial checks cover invalid/aliased
geometry, conditional no-mark behavior, propagation, pending publication and
invalid selector. Three actual-source controls per backend omit color, omit
pending or force marking on matching color; exact semantic10 is required before
worker launch. Compiler/signal/timeout/sanitizer failures never count as controls.

Both backends' joined positives pass TSan and nonrecovering UBSan. Actual runtime
C/JIT/both-poll owner/root/Fast/Cycles tests, including the new real-String
color/count snapshot probe, pass TSan; emitted JIT bytes remain uninstrumented
and do not run concurrently with those scalar workers. Four private snapshot
controls check count/Type/color preservation and overlap refusal before unsafe
work. Pointer-builder refusal/rollback controls and x64-tail positives also pass.
40 selected console/editor CTests pass. These do not certify concurrent graph
writes, root publication, selector reset or generated JIT marking.

Installed SDK carries the native helper/ports/test and this contract. Run
`python3 source/threading/test_gc_fields.py` inside the package. Private runtime
relink diagnostics need matching GNU Makefiles console objects/flags. Set TMPDIR
to a writable directory if /tmp is unavailable. `--positive-only` excludes
semantic source controls when using instrumented CFLAGS.
