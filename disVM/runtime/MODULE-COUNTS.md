# Module/link ownership — runtime0.69

The hosted load.c/xec.c views now use checked signed-int Module retains for
lookmod and $self, plus a checked LAST/LIVE release in unload. The latter returns
immediately after nonfinal release without rereading count storage. Only LAST
unlinks the registry and performs the retained MP/export/type/code cleanup.
Unpublished Module initial assignments remain plain. Exact source matching
refuses changed pinned boundaries rather than omitting a conversion.

Module counting is separate from Type counting. Hosted link.c/load.c additionally
convert3 Type retains and8 preflight observations; the $self Module preflight
uses2 atomic observations. Existing initialization, source roots, allocation,
link failure and clone publication order remain. A preflight load is not a
reservation for a future parallel allocator.

Counts use existing checked native int primitives, including the registry
fallback. Helpers allocate nothing, invoke no VM/destructor and never read count
storage after successful release. Zero/negative/overflow refusal is a terminal
native invariant, not recovery after partial construction. Caller must protect
source lifetime and registry access and must not already hold the port registry
mutex. Counter atomicity does not make registry lookup/unlink or native/JIT code
retirement independently safe.

## Actual registry and data-instance diagnostic

module_counts_probe.c uses the real legacy builtin constructor with an empty
Runtab: no guest export or new format becomes available. Under exclusion it
attaches an actual scalar Type/MP to that private diagnostic module. It checks:

- Actual lookmod acquires a distinct owner; nonfinal unload preserves creator.
- Real linkmod clones a separate MP and holds its Module/Type ownership.
- The clone starts with value7; writing9 leaves the original MP at7.
- Fast retirement releases clone/Modlink owners to exact original baselines.
- Nil-import failure consumes only its caller's owner; process diagnostic text
  is restored before continuing.
- Final unload removes the registry entry, then Fast retires the original MP.
  No Module/Type storage is read after final owner consumption.

This exercises real constructor/lookup/link/unload and collector bodies, not a
replacement registry or injected registration callback. Probe metadata setup
is native C under existing initialization/admission exclusion, not file import
or a general public API for editing published module metadata.

```sh
export TMPDIR="${TMPDIR:-$PWD/build/tmp}"
mkdir -p "$TMPDIR"
python3 disVM/runtime/test_type_counts.py --modules \
    build/workspace1735/disvm-editor/disvm \
    build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap \
    disVM/upstream/module
```

The optional native GNU-linker/Makefiles driver intercepts only this probe's
first Module retain or nonfinal release. Exact semantic status10 must occur at
its expected count predicate before unsafe freeing. Startup stays unchanged;
compiler/signal/timeout/sanitizer failures do not count as successful controls.
Previous Type/Heap/opcode selectors remain. C/JIT application modes both execute
this diagnostic's retained C bodies, not instrumented JIT machine instructions.
SDK sources require matching private runtime headers/build metadata.

## Remaining exclusion

Other C/JIT/GC Module/Type/Heap accesses remain to audit. Registry nodes, code and
metadata, root maps, snapshots, allocation and graph publication still require
serialized managed admission. A shared code Module can have separate Modlink MP
instances; spawn/$self/explicit aliases can still share an instance. This series
proves neither arbitrary module privacy nor automatic miniheaps. No native
pointer serialization, layout or shared Dis64 ID/file activation changes.
