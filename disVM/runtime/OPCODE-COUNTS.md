# Explicit opcode retains — runtime0.68

The final hosted xec.c view converts16 explicit count increments:11 Heap and
5 Type retains. Site families include pointer moves, self module MP ownership,
channel buffers/messages, list constructors/heads/tails and array slices.
Checked native helpers are the same ones exercised by the prior heap conversion.
Marking, captures, allocation order, root fields and publication statements keep
their existing positions. Exact source matching refuses changed boundaries.
Pinned source is byte-identical; only hosted build output is adapted.

This is deliberately an increment conversion. Module.ref remains separate.
Preflight observations and local whole-Heap snapshots still have plain accesses;
they must stay under serialized admission. A native C retain does not make its
surrounding field publication atomic or grant the object lifetime of a source
that was not already protected. JIT-emitted retain sequences remain separate.

## Real opcode diagnostic

opcode_counts_probe.c saves the ACTIVE controller's complete TLS REG, runs the
actual retained movp function against a real managed String, then restores REG
before leaving admission. It checks:

- First assignment acquires a second distinct owner before publication.
- Self-assignment retains before releasing and leaves exactly two owners.
- Assigning H removes the destination owner while the source owner remains.
- Final source destruction is followed by no object read; actual Fast retirement
  completes with admission released.

The probe runs through C operation bodies even when the surrounding application
uses --jit. This does not claim that emitted machine instructions are atomic.
Console/editor owner drivers require its marker alongside Type/Heap/root markers.

```sh
export TMPDIR="${TMPDIR:-$PWD/build/tmp}"
mkdir -p "$TMPDIR"
python3 disVM/runtime/test_type_counts.py --opcodes \
    build/workspace1735/disvm-editor/disvm \
    build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap \
    disVM/upstream/module
```

The optional native GNU-linker/Makefiles driver relinks actual objects. Only
inside the real opcode probe it removes the first Heap retain or nonfinal drop.
Missing retain must fail after first assignment; missing drop must fail after
self-assignment. Both require exact semantic status10 before invalid freeing.
Signals, compiler errors, timeout and sanitizer failures do not count as detected
controls. Without flags the Type controls remain; --heap selects Heap controls.

The adapter/probe/preparer ship in the source SDK as hosted runtime sources,
requiring matching private runtime headers/build flags rather than a standalone
public native-module ABI. TSan checks instrumented C paths, not generated JIT
machine instructions. Local x64 runs do not certify ARM64/RV64 execution.

[Heap sites](HEAP-COUNTS.md), [Type sites](TYPE-COUNTS.md),
[remaining managed-heap work](HEAP-PARALLELISM-STATUS.ru.md).
