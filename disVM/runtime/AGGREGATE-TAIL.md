# Checked aggregate tail entries — runtime0.85

Hosted SysV x64 MOVMP, HEADMP, CONSMP and CONSM now tail-enter their retained
C opcodes. The old mapped-copy fast path accepted only pointer-free records;
the checked C path also handles pointer-containing records with the retained
count and marking policy. Other architecture emitters are unchanged.

`aggregate_fast.c` checks the current native context through the shared owner
preflight before reading operands. REG identity/size, host process and ACTIVE,
nonpending role are required. These checks do not grant heap access or prove
source-cell, module, frame or payload lifetime. Existing NativeWorld admission
still serializes general managed execution.

`prepare_aggregate_tail.py` verifies the three original emitter body hashes and
actual REG/Heap/Type layout and GC identities. A MOV/JMP tail preserves the
original C return PC. The adapter performs no cache access after entering C,
including when a callback retires that JIT mapping. Another executor retaining
code still requires a lifetime pin and grace-period protocol. No new shared
LeafLayout field, Dis64 ID, instruction encoding or file admission is introduced.

Each entry returns zero only before operands, and one after normal completion.
VM errors propagate through the retained exception boundary; no decline/retry
occurs after ownership effects. Constructors retain their existing C failure
semantics; this change does not promise transactional rollback for all aggregates.

## Verification

The actual generated guest copies string-containing records, reads mapped list
heads and builds pointer-containing and pointer-free lists. C/no-JIT and all
four JIT entries agree for 32 elements. Per-opcode cache unmap, invalid-type
failure before ownership effects, combined failure/unmap and both polling modes
pass. NULL/foreign REG checks preserve live state. An ABI-aligned CALL and the
restored original emitter are strict negative controls requiring semantic exit10.
Positive actual-entry cases also pass TSan; generated machine bytes themselves
are uninstrumented. The selected console/editor regression set passes42 tests.

## Remaining multithreading work

These checked C entries remove another unchecked inline ownership path. They do
not remove global admission. Shared source-cell/payload guards, publication and
GC epochs, pending-reset/collector state, remaining JIT families, channel and
scheduler transitions, native callback ownership and executable-code retirement
across multiple executors still require implementation and stress validation.

See [owner entries](OWNER-TAIL.md), [scalar GC fields](GC-FIELDS.md) and
[threading contract](THREADING.md).
