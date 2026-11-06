# Jointly guarded binary integer arithmetic — runtime0.96

Explicit --gc-worker --guarded-copies adds ADD/SUB/AND/OR/XOR for BYTE, native
WORD and LONG: fifteen existing opcodes. Default operation bodies/emission remain
intact. The complete final bodies are hashed before startup selection insertion.
No shared Dis64 encoding, numeric ID, WORD32 behavior or admission is changed.

## Transaction and native three-span API

shared_arithmetic.c preflights exact ACTIVE service identity and validates opcode
and all three nonempty geometries before payload reads. It calls the new
managed_three_begin gate, which claims the canonical domain shared with pointer,
two-cell and arbitrary-range operations. One union covers first input, second
input and output until publication. There is no read-unlock-evaluate-relock gap.
Aliased/unaligned inputs and output are captured fully before any store.

arithmetic_value.c implements only computation: BYTE promotions fit int;
WORD/LONG use unsigned native types for defined modular add/subtract and bitwise
operations. No division, shifts, FP, reference ownership, allocation or VM callback
is introduced under these locks. Retained operand order is m op s, output d.
Errors return after refusal/unwind; the actual opcode raises outside guards.

Public native dvm_range_access_three_begin extends the existing range service:
three arbitrary-sized spans, empty sizes ignore addresses, one deduplicated bitset,
ascending claims, reverse rollback/release and one domain borrow. The old two-span
entry delegates with an empty extra input. DVM_RangeAccess layout/ABI is unchanged.
Calls allocate no metadata; a native allocator monitor checks begin/end. Native
metadata locks may wait; object guard acquisition remains try-only. BUSY clears
every partial claim. The hosted experimental arithmetic maps refusal/BUSY to
exHeap before effects; a future parallel scheduler retry policy is still required.

Caller independently supplies live/mapped spans and domain lifetime. No mapping,
root, referent lifetime, graph epoch or unguarded-competitor permission follows.
No managed call/error/OS wait may span an access. NativeWorld remains necessary.

## Evidence and reproduction

The native range fixture mixes two-cell and three-span access on eight joined OS
writers. It checks 16000 paired updates and 8000 disjoint third-counter updates,
coherent values, busy rollback, exact six-stripe union, aliases, invalid middle
geometry, empty/full/wrapped selections, final borrow retirement and no allocation
in serial begin/end. Native and forced-lock backends run strict omission controls;
compile/link failure never counts as semantic failure. This is native data, not
forged Heap/GC evidence or parallel managed-program acceptance.

The actual managed C fixture invokes all fifteen opcodes via real kprotect and
checks 877500 value/source-source/output alias combinations with complete canaries.
Guard refusal, invalid geometry, unknown opcode, held-guard preflight and protected
errors leave output unchanged. Independent controls omit admission or publication.
The WORD64/SHORT16/LONG64 native oracle reuses the protected-op harness, not an
opcode implementation. The real Limbo guest reaches all fifteen actual x64 tails;
coverage, each mapping unmap, return PC, combined errors, held/foreign/NULL contexts
and bad-CALL/old-emitter controls are checked. Machine code is uninstrumented.

```sh
python3 disVM/tests/range_access.py
python3 disVM/runtime/test_shared_arithmetic.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/test_arithmetic_tail.py RUNTIME LIMBO INCLUDE_DIR
python3 disVM/runtime/benchmark_shared_arithmetic.py RUNTIME LIMBO INCLUDE_DIR
```

Private runtime tests require matching GNU/CMake console objects and accept
--positive-only for instrumented positives. Set TMPDIR if /tmp is unavailable.
The SDK installs source/test closure, updated native header and a fully commented
[Limbo example](../examples/guarded-arithmetic/README.md). Performance costs are
retained in [the ledger](PERFORMANCE-COSTS.md); no safety is removed for timing.

Still open: other arithmetic/reference writers and list construction, channel and
scheduler ownership, root/graph/GC epochs, frame/module/code lifetime and compiler
local/shared selection. No new physical ARM64/RV64 or whole parallel managed claim.
Inferno Frame64 Int32/Ref64 and shared numeric profiles do not inherit our hosted
native WORD width. Retained vendor sources and Draw import restrictions stand.

Release validation: 33 affected CTests pass in 241.61 seconds. TSan positives pass
for actual arithmetic, all fifteen tail entries and native/forced-lock mixed range
writers. The installed native suite passes all twelve semantic omission controls;
source/header/test closure and ordinary C/guarded C/guarded JIT examples pass.
Vendors verify 1096/312 files. No new hosted UBSan or physical target proof.

ARITH-096 whole-profile local cost: C 2.514192918→3.296930437 seconds (+31.13%);
JIT 1.647544784→3.721847356 (+125.90%). Same image/binary, warmup then median of
three startup-inclusive runs, 20000 iterations of fifteen explicit operations plus
checks/loop/conversions. Generic 256-entry range metadata and scanning are recorded
optimization candidates; no isolated attribution or cross-workload addition.

Runtime0.97 uses [compact metadata](SMALL-RANGES.md) for the same three live small
spans and same canonical domain. The generic path/cost above remains historical
evidence; no arithmetic semantics, default admission or NativeWorld change.

Runtime0.98 adds nine [product/division/remainder operations](PRODUCT-DIVISION.md)
to this compact domain. Earlier fifteen-op measurements remain historical; their
benchmark guest is explicitly frozen. No global parallel admission is enabled.

With `--native-spawn --guarded-copies --parallel-integer`, a dedicated
guest can evaluate a verified private integer operation and publish it
in one restricted shared interval. It captures the inputs before suspension
and revalidates them under a new range union before publication. A refused
shared claim falls back under exclusive admission. Errors are delivered only
after exclusive reentry. The hosted overlap fixture runs forty dedicated guests and verifies overlapping admissions with a
bounded test-only barrier.

The dedicated integer mode also groups up to sixteen consecutive BYTE, WORD or LONG
arithmetic and bitwise instructions with a direct scalar frame destination
and frame, typed module-data or immediate inputs. The implicit middle operand can alias the
destination. One shared admission covers the verified prefix. A busy range
ends the batch before the conflicting instruction, preserving every prior
publication. The hosted dispatcher accounts for the completed prefix and
honors the remaining quantum. Explicit instruction quotas and static
translated modules keep ordinary instruction dispatch.

The same bounded trace follows direct same-module `JMP` and integer comparisons
with immediate targets. Each taken target is checked against the retained
instruction span before use. A loop can execute multiple iterations within
one admission, up to the instruction/quantum bound. The common executor
accounts for each completed branch and resumes normal dispatch at the first
unsupported instruction.
