# Jointly guarded shifts — runtime0.99

Explicit `--gc-worker --guarded-copies` adds eight existing opcodes: SHL/SHR for
BYTE/native WORD/LONG and logical LSR for WORD/LONG. No new opcode, wire layout,
production Dis64 admission or default operation is introduced. Complete retained
bodies are hashed before adapter insertion; the original leaf emitter stays intact.

## Transaction and errors

`shared_shift.c` validates exact ACTIVE service identity, then opcode and geometry.
The count slot always occupies native WORD, including a BYTE value. One compact
canonical union covers value, count and output. A captured count is checked before
any value/output payload access: valid ranges are 0..7 or 0..63 on this native64
runtime. If outer geometry is nil/H/wrapped, only the valid count slot is claimed
through three aliases. Invalid count gives exInval before outer exBounds; invalid
count geometry gives exBounds. Admission/BUSY still gives exHeap before semantics.
Every error is raised by the actual opcode after release, without held guards.

`shift_value.c` computes from snapshots before stores, including unaligned aliases.
Left shifts use unsigned modular arithmetic. BYTE right shift is logical;
WORD/LONG SHR explicitly fills the sign bits, while LSR never sign-fills. Zero
count never evaluates a shift by the type width. No allocation, references,
managed callback or wait occurs while object guards are held. Native metadata
locks retain their existing waiting contract. Callers independently supply live
mapped cells and domain lifetime: guards do not prove roots, mapping or ownership.

## Evidence and reproduction

The actual C opcode fixture checks 99000 value/count/alias cases, including 45000
invalid counts, full canaries, all eight entries and the WORD-width byte counter.
Separate controls omit admission/publication or corrupt the invalid-count error;
protected VM error entry must have no guards. LSRW/L also reach the actual native
fast dispatcher. The real Limbo guest emits six SHL/SHR entries; it does not prove
LSR machine-code emission. Six actual x64 tails have individual cache-unmap,
coverage, error/combined and bad-CALL/old-emitter controls, with both poll modes.
Twelve invalid-count guests check exit1/invalid-argument/no after-result output in
default/guarded C/JIT. Compile or link failure never counts as a semantic control.

```sh
python3 disVM/runtime/test_shared_shift.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_shift_tail.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_shift_errors.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_shared_shift.py RUNTIME LIMBO INCLUDE_DIR
```

These developer scripts need the matching native build's object/source closure.
TSan instruments C/native helpers, not generated machine code. Actual evidence in
[v0.1797](../../docs/releases/v0.1797.md) is Linux/SysV x64; no physical ARM64/RV64
or hosted UBSan acceptance is inferred. Native WORD64 is not shared Int32/Ref64.
[Commented example](../examples/guarded-shifts/README.md).
[Cost ledger](PERFORMANCE-COSTS.md). NativeWorld still serializes general managed
execution; readers/comparisons/FP, construction, channels, root/graph/GC epochs and
frame/module/code lifetime remain before general parallel admission.

Runtime0.100 follow-up: the v0.1797 cumulative actual-C-fixture TSan failure
is isolated to fortified __longjmp_chk bypassing this runtime's interceptor.
The [separate error-transfer boundary](NATIVE-UNWIND.md#sanitizer-aware-posix-transfer--runtime0100)
restores siglongjmp interception in TSan builds only. The full99000-case fixture
now passes C/JIT and both polls without sharding or dropping invalid-count cases.
Retain v0.1797's original failure report; this is new bounded evidence.
