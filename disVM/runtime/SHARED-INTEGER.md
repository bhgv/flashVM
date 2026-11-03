# Guarded integer conversions — runtime0.95

Explicit `--gc-worker --guarded-copies` now selects CVTBW/WB/WS/SW/WL/LW through
shared_integer.c. Default retained bodies and default emission remain unchanged.
integer_value.c contains only the conversion mechanics; the admission/guard layer
is separate. This is not a new file opcode, feature ID or WORD-width extension.

## Transaction

Preflight the exact ACTIVE worker context before payload reads. Validate opcode
and the full input/output geometry, rejecting nil/H and arithmetic wrap. These
are geometric checks only; the caller owns independently mapped, live storage.
No root/owner count is acquired and arbitrary native pointers are not admitted.

Claim the exact input/output byte union through the same ordered cell domain used
by shared scalar/pointer copies. Capture the complete source into an aligned local,
perform the retained native integer assignment, publish the exact result width,
then release guards. Identical and partially overlapping slots use one transaction;
there is no read-unlock-convert-relock gap. No allocation, error callback, blocking
VM service, reference retain/drop or floating operation occurs under the guards.
Native guard acquisition itself may wait. Invalid context/opcode/geometry or refused
admission returns a static error before effects; actual opcode raises after return.

Widths are sizeof(BYTE/SHORT/WORD/LONG) in the admitted runtime. Signed narrowing
retains the original host C assignment behavior, including implementation-defined
out-of-range conversion; no new cross-host narrowing specification is introduced.
NativeWorld remains necessary for all unconverted operations and root/GC epochs.

The source adapter hashes all six complete final bodies before insertion; repeated
adaptation/source drift refuses. The x64 leaf emitter's reviewed body/layout binding
routes six selected operations into the existing native C tail gate. Exact REG,
process identity and no-held-guard preflight precede dispatch. Tail calls keep their
return PC outside disposable JIT mappings. Other architectures' guarded JIT refusal
and explicit-C target validation are unchanged.

## Evidence and reproduction

The actual C fixture invokes all six production opcodes under real managed-owner
admission and kprotect. It checks 23400 conversions: all 256 byte values plus
signed/large boundaries across 15 relative offsets per operation, complete result
bits and surrounding canaries. The native layout oracle is WORD64/SHORT16/LONG64.
Malformed spans, unknown opcode, held-guard preflight and protected opcode errors
must leave destination untouched. Real guard refusal is tested unchanged/unlocked.
Two semantic controls omit the claim or the conversion and must stop at the exact
failure before unsafe continuation; link/compile failures never count as detection.

A compiled Limbo guest reaches four real conversion tails, with coverage before
cache destruction, individual mapping unmap, error/combined cases, return-PC ABI,
foreign/NULL/held-guard refusals and bad-CALL/old-emitter omission controls. SHORT
C operations are covered, but this guest is NOT a real emitted SHORT-tail proof.
Generated machine code is uninstrumented, including in TSan runs. No physical
ARM64/RV64 or whole parallel-managed execution claim follows.

```sh
python3 disVM/runtime/test_shared_integer.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_integer_tail.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_shared_integer.py RUNTIME LIMBO INCLUDE_DIR
```

Private tests require matching GNU/CMake native console objects and accept
`--positive-only` for instrumented positives. Set TMPDIR if /tmp is unavailable.
The full source/test closure is installed under source/threading; the commented
[Limbo example](../examples/guarded-integer/README.md) is installed under examples.
Added measured cost is retained in [PERFORMANCE-COSTS.md](PERFORMANCE-COSTS.md).

Still open: list construction, other arithmetic/reference writers, channel and
scheduler transfers, graph/root/GC epochs, frame/module/code lifetime and compiler
local/shared selection. General managed computation remains serialized by
NativeWorld. No shared Dis64 agreement/default admission or retained vendor pin
changes; no peer Draw/OS implementation import.

Release validation: 29 affected CTests pass in 193.96 seconds. After isolating the
old MOV-tail observer from newly admitted conversions, all three affected tail
suites pass again (54.91 seconds). Both new suites pass TSan positives; the
integer-tail run was repeated after observer isolation. Installed source closure
and all three example profiles match/pass. Retained vendors verify 1096/312 files.

INT-095 local median cost: C 0.174222527→0.191245283 seconds (+9.77%); JIT
0.126487650→0.167297898 (+32.26%). Same image/binary, warmup then median of three
startup-inclusive runs of 20000 iterations. This measures the whole guarded
profile, not isolated conversion, and is not additive with prior workloads.
